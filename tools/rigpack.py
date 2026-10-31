#!/usr/bin/env python3
"""RigKit pack manager: resolve, check, update, and sync pack pins.

Manifests stay the one source of truth: `app.json` (apps) and `pack.json`
(packs) already carry {"name", "url", "ref"} per dependency. rigpack makes
those pins mean something:

  status    Resolve the dependency graph and show local checkout state.
  check     Offline gate: unresolved conflicts, bad SemVer. Floating
            branch refs (`main`) are allowed; --pinned fails them.
  outdated  Compare manifest refs against each pack remote.
  update    Advance refs per current kind (see below), optionally flatten
            transitive packs into an app.json, optionally --sync.
  sync      Clone or check out packs/<name> at the manifest refs.

A `ref` is either a track or a pin:

  branch (`main`)   float: `update --latest --sync` pulls origin/main
                    and does not rewrite the manifest.
  tag (`v0.2.0`)    pin: `update` moves to the newest compatible tag.
  SHA               pin: `update --latest` rewrites to default-branch HEAD.

`--float` writes `main` (unpin). `--pin` snapshots floats to a SHA, or to a
tag when the pack has SemVer tags. `--latest` on a tag pin also crosses
MAJOR (same as --allow-breaking).

Pin rules: the root manifest (the app, or the pack being developed) wins
over nested pack.json requests. A local packs/<name> checkout wins over
the pin at build time (that is the development path); `check` and the
CMake loader report when it does not match a pinned tag or SHA.

Python stdlib + git only. No registry, no solver, no runtime updater:
configure-time CMake (RigKitPacks.cmake) still fetches what is missing,
and honours the same root-wins rule.
"""

import argparse
import json
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

SEMVER_RE = re.compile(r"^v?(\d+)\.(\d+)\.(\d+)$")
SHA_RE = re.compile(r"^[0-9a-fA-F]{7,64}$")
FLOATING_DEFAULT = "main"


def run_git(args, cwd=None, timeout=120):
    """Run git, return (returncode, stdout stripped). Never raises."""
    try:
        proc = subprocess.run(
            ["git", *args],
            cwd=str(cwd) if cwd else None,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return proc.returncode, proc.stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        return 1, ""


def parse_semver(text):
    if not text:
        return None
    m = SEMVER_RE.match(text.strip())
    if not m:
        return None
    return tuple(int(g) for g in m.groups())


def classify_ref(ref):
    """Return 'tag', 'sha', or 'branch' for a manifest ref."""
    if not ref:
        return "branch"
    if parse_semver(ref):
        return "tag"
    # Pure hex of length >= 7 cannot be a branch name we use (main,
    # develop, ... all contain non-hex letters).
    if SHA_RE.match(ref):
        return "sha"
    return "branch"


def load_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def dep_ref(dep):
    ref = dep.get("ref") or dep.get("branch") or ""
    return ref


class PackNode:
    def __init__(self, name):
        self.name = name
        self.url = ""
        self.ref = ""            # effective ref (root manifest wins)
        self.version = ""        # pack.json version, when checked out
        self.direct = False      # listed in the root manifest
        self.missing_local = False
        self.requests = []       # (source, url, ref) in encounter order

    def request(self, source, url, ref):
        self.requests.append((source, url, ref))
        if not self.url and url:
            self.url = url


class Resolver:
    def __init__(self, packs_dir):
        self.packs_dir = Path(packs_dir)
        self.nodes = {}          # name -> PackNode (insertion ordered)
        self.errors = []

    def local_manifest(self, name):
        path = self.packs_dir / name / "pack.json"
        if not path.is_file():
            return None
        try:
            return load_json(path)
        except (OSError, json.JSONDecodeError) as exc:
            self.errors.append(f"{name}: cannot read {path}: {exc}")
            return None

    def resolve(self, manifest_path):
        manifest_path = Path(manifest_path)
        try:
            root = load_json(manifest_path)
        except (OSError, json.JSONDecodeError) as exc:
            self.errors.append(f"cannot read {manifest_path}: {exc}")
            return {}
        root_deps = root.get("dependencies") or []
        root_name = root.get("name") or manifest_path.name

        # Root manifest first: its pins are the effective ones.
        queue = []
        for dep in root_deps:
            if not isinstance(dep, dict) or not dep.get("name"):
                self.errors.append(f"{root_name}: dependency entry missing 'name'")
                continue
            node = self._node(dep["name"])
            node.direct = True
            node.request(root_name, dep.get("url", ""), dep_ref(dep))
            node.url = dep.get("url", "") or node.url
            node.ref = dep_ref(dep)
            queue.append(dep["name"])

        # Depth-first expansion through local pack.json files, matching
        # RigKitPacks.cmake. Missing checkouts stay leaves with a flag.
        seen = set()
        while queue:
            name = queue.pop(0)
            if name in seen:
                continue
            seen.add(name)
            node = self.nodes[name]
            local = self.local_manifest(name)
            if local is None:
                node.missing_local = True
                continue
            node.version = str(local.get("version", ""))
            if not node.url:
                node.url = local.get("url", "")
            for dep in local.get("dependencies") or []:
                if not isinstance(dep, dict) or not dep.get("name"):
                    self.errors.append(f"{name}: dependency entry missing 'name'")
                    continue
                child = self._node(dep["name"])
                child.request(name, dep.get("url", ""), dep_ref(dep))
                queue.append(dep["name"])

        # Effective ref for nested-only packs: the single nested request,
        # or "" when nested requests disagree (a real conflict).
        for node in self.nodes.values():
            if node.direct:
                continue
            refs = {r for (_s, _u, r) in node.requests if r}
            node.ref = refs.pop() if len(refs) == 1 else ""
            urls = {u for (_s, u, _r) in node.requests if u}
            if len(urls) == 1 and not node.url:
                node.url = urls.pop()
        return root

    def _node(self, name):
        if name not in self.nodes:
            self.nodes[name] = PackNode(name)
        return self.nodes[name]

    def conflicts(self):
        """name -> list of distinct (url, ref) requests the root does not settle."""
        out = {}
        for node in self.nodes.values():
            distinct = {(u, r) for (_s, u, r) in node.requests}
            if node.direct:
                continue  # root pin wins; CMake notes the override
            if len(distinct) > 1:
                out[node.name] = sorted(distinct)
        return out

class RemoteInfo:
    def __init__(self, url):
        self.url = url
        self.ok = False
        self.head_sha = ""
        self.branches = {}       # name -> sha
        self.tags = {}           # tag name -> sha (peeled)
        self.error = ""

    @property
    def semver_tags(self):
        out = []
        for tag in self.tags:
            ver = parse_semver(tag)
            if ver:
                out.append((ver, tag))
        out.sort()
        return out


_remote_cache = {}


def remote_info(url):
    if url in _remote_cache:
        return _remote_cache[url]
    info = RemoteInfo(url)
    _remote_cache[url] = info
    if not url:
        info.error = "no url"
        return info
    code, out = run_git(["ls-remote", url], timeout=180)
    if code != 0 or not out:
        info.error = "ls-remote failed"
        return info
    info.ok = True
    raw_tags = {}
    for line in out.splitlines():
        parts = line.split("\t")
        if len(parts) != 2:
            continue
        sha, refname = parts
        if refname == "HEAD":
            info.head_sha = sha
        elif refname.startswith("refs/heads/"):
            info.branches[refname[len("refs/heads/"):]] = sha
        elif refname.startswith("refs/tags/"):
            tag = refname[len("refs/tags/"):]
            if tag.endswith("^{}"):
                raw_tags[tag[:-3]] = sha  # peeled wins
            else:
                raw_tags.setdefault(tag, sha)
    info.tags = raw_tags
    return info


def tracked_branch(node):
    if classify_ref(node.ref) == "branch" and node.ref:
        return node.ref
    return FLOATING_DEFAULT


def newest_tag(tags, node, allow_breaking):
    """Return (tag, 'tag') or (None, reason)."""
    if not tags:
        return None, "no tags"
    kind = classify_ref(node.ref)
    baseline = None
    if kind == "tag":
        baseline = parse_semver(node.ref)
    elif node.version:
        baseline = parse_semver(node.version)
    best = None
    for ver, tag in tags:
        if baseline and not allow_breaking:
            if ver[0] != baseline[0]:
                continue
            if baseline[0] == 0 and ver[1] != baseline[1]:
                continue
        if best is None or ver > best[0]:
            best = (ver, tag)
    if best:
        return best[1], "tag"
    return None, "no compatible tag (try --latest or --allow-breaking)"


def branch_head(info, branch):
    sha = info.branches.get(branch) or info.head_sha
    if sha:
        return sha, "sha"
    return None, f"branch {branch} not on remote"


def candidate_for(node, allow_breaking=False, latest=False, float_refs=False, pin=False):
    """Next ref for a node, or (None, reason).

    Returns (ref_value, kind) where kind is 'tag', 'sha', or 'branch'.
    Honours the current ref's kind unless --float or --pin overrides it.
    """
    info = remote_info(node.url)
    if not info.ok:
        return None, info.error or "remote unreachable"
    kind = classify_ref(node.ref)
    branch = tracked_branch(node)
    cross = allow_breaking or latest

    if float_refs:
        return branch, "branch"

    if pin:
        if kind == "tag" or (kind != "sha" and info.semver_tags):
            tag, tag_kind = newest_tag(info.semver_tags, node, cross)
            if tag:
                return tag, tag_kind
        return branch_head(info, branch)

    if kind == "branch":
        return (node.ref or FLOATING_DEFAULT), "branch"

    if kind == "tag":
        return newest_tag(info.semver_tags, node, cross)

    # SHA pin: only move when asked for latest.
    if latest:
        return branch_head(info, branch)
    return node.ref, "sha"


def is_current(node, candidate):
    if candidate is None:
        return True
    kind = classify_ref(node.ref)
    cand_kind = classify_ref(candidate)
    if kind == "tag" or cand_kind == "tag":
        return node.ref == candidate or (
            parse_semver(node.ref) and parse_semver(candidate)
            and parse_semver(node.ref) == parse_semver(candidate)
        )
    if kind == "sha" or cand_kind == "sha":
        return candidate.startswith(node.ref) or node.ref.startswith(candidate)
    return (node.ref or FLOATING_DEFAULT) == candidate


def local_git_state(packs_dir, name):
    """(head, dirty, track) for packs/<name>, or None when not a git checkout."""
    path = Path(packs_dir) / name
    if not path.is_dir():
        return None
    if not (path / ".git").exists():
        return {"head": "", "dirty": False, "track": "not-git", "tag": ""}
    code, head = run_git(["rev-parse", "--short", "HEAD"], cwd=path)
    if code != 0:
        return {"head": "", "dirty": False, "track": "git?", "tag": ""}
    _c, porcelain = run_git(["status", "--porcelain"], cwd=path)
    dirty = bool(porcelain)
    track = "clean"
    code, counts = run_git(["rev-list", "--left-right", "--count", "@{u}...HEAD"], cwd=path)
    if code == 0 and counts:
        try:
            behind, ahead = (int(x) for x in counts.split())
        except ValueError:
            behind = ahead = 0
        if ahead and behind:
            track = f"diverged(+{ahead}/-{behind})"
        elif ahead:
            track = f"ahead({ahead})"
        elif behind:
            track = f"behind({behind})"
    else:
        track = "clean(no-upstream)"
    _c, tag = run_git(["describe", "--tags", "--exact-match"], cwd=path)
    return {"head": head, "dirty": dirty, "track": track, "tag": tag}


def fmt_ref(ref):
    if not ref:
        return "(none)"
    if classify_ref(ref) == "sha" and len(ref) > 12:
        return ref[:12]
    return ref


# ---------------------------------------------------------------- commands

def find_default_packs_dir(rigkit_root):
    return Path(rigkit_root) / "packs"


def cmd_status(args):
    packs_dir = args.packs_dir
    fail = False
    if args.build:
        cache = Path(args.build) / "CMakeCache.txt"
        if not cache.is_file():
            print(f"[build] no CMakeCache.txt at {args.build}", file=sys.stderr)
            fail = True
        else:
            cached = ""
            for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
                if line.startswith("RIGKIT_DIR:PATH="):
                    cached = line.split("=", 1)[1].strip()
                    break
            if cached:
                same = Path(cached).resolve() == Path(args.rigkit).resolve()
                print(f"[build] {args.build} RIGKIT_DIR={cached} "
                      f"({'OK' if same else 'WARN - points elsewhere'})")
                fail = fail or not same
            else:
                print(f"[build] RIGKIT_DIR not in cache at {args.build}")
        print()

    rows = []
    if args.manifest:
        resolver = Resolver(packs_dir)
        resolver.resolve(args.manifest)
        for err in resolver.errors:
            print(f"ERROR: {err}", file=sys.stderr)
            fail = True
        conflicts = resolver.conflicts()
        for name, node in resolver.nodes.items():
            rows.append((name, node, node.direct))
        if conflicts:
            print("Unresolved conflicts (add an explicit pin to the root manifest):")
            for name, reqs in sorted(conflicts.items()):
                print(f"  {name}: " + ", ".join(f"{u}@{fmt_ref(r)}" for u, r in reqs))
            print()
            fail = True
    else:
        if not Path(packs_dir).is_dir():
            print(f"no packs dir at {packs_dir}", file=sys.stderr)
            return 1
        for child in sorted(Path(packs_dir).iterdir()):
            if child.is_dir() and (child / "pack.json").is_file():
                node = PackNode(child.name)
                try:
                    data = load_json(child / "pack.json")
                    node.version = str(data.get("version", ""))
                    node.ref = data.get("ref", "")
                    node.url = data.get("url", "")
                except (OSError, json.JSONDecodeError):
                    pass
                rows.append((child.name, node, False))

    print(f"{'PACK':28} {'PIN':14} {'LOCAL':10} {'VERSION':10} STATE")
    print(f"{'-' * 28} {'-' * 14} {'-' * 10} {'-' * 10} -----")
    for name, node, direct in rows:
        state = local_git_state(packs_dir, name)
        if state is None:
            local, track = "missing", "cpm"
        elif state["track"] == "not-git":
            local, track = "tree", "not-git"
        else:
            local, track = state["head"], state["track"]
            if state["dirty"]:
                track += ",dirty"
            if state["tag"]:
                track += f",at {state['tag']}"
        pin = fmt_ref(node.ref) + ("*" if direct else "")
        print(f"{name:28} {pin:14} {local:10} {(node.version or '-'):10} {track}")
        if args.strict and state and (state["dirty"] or track.startswith(("diverged", "behind"))):
            fail = True
    print("\n(* = pinned directly by the root manifest)")
    return 1 if fail else 0


def collect_problems(args, manifest):
    """Offline policy check. Returns (errors, warnings, resolver)."""
    resolver = Resolver(args.packs_dir)
    root = resolver.resolve(manifest)
    errors = list(resolver.errors)
    warnings = []
    manifest_name = Path(manifest).name
    root_name = (root or {}).get("name") or manifest_name

    for dep in (root or {}).get("dependencies") or []:
        if not isinstance(dep, dict):
            continue
        name = dep.get("name", "?")
        if not dep.get("url"):
            errors.append(f"{name}: dependency in {root_name} has no 'url'")
        if not dep_ref(dep):
            errors.append(f"{name}: dependency in {root_name} has no 'ref' (floats on {FLOATING_DEFAULT})")
        if dep.get("branch") and not dep.get("ref"):
            warnings.append(f"{name}: 'branch' is a deprecated alias; use 'ref'")
        elif classify_ref(dep_ref(dep)) == "branch" and dep_ref(dep):
            if getattr(args, "pinned", False):
                errors.append(
                    f"{name}: ref '{dep_ref(dep)}' floats on a branch "
                    f"(rigpack update --pin to snapshot, or drop --pinned)"
                )

    for name, reqs in sorted(resolver.conflicts().items()):
        errors.append(
            f"{name}: nested packs request different pins ("
            + ", ".join(fmt_ref(r) for _u, r in reqs)
            + "); add an explicit pin to the root manifest")

    for name, node in resolver.nodes.items():
        if node.missing_local:
            warnings.append(f"{name}: not checked out under {args.packs_dir}; "
                            "nested deps and version unverified (rigpack sync)")
            continue
        if not parse_semver(node.version):
            errors.append(f"{name}: pack.json version '{node.version or '(missing)'}' is not SemVer")
        state = local_git_state(args.packs_dir, name)
        if state and state["tag"] and parse_semver(state["tag"]) and node.version:
            if parse_semver(state["tag"]) != parse_semver(node.version):
                errors.append(
                    f"{name}: checkout is tagged {state['tag']} but pack.json says "
                    f"{node.version}; tag and version must match")
        if state and node.direct and classify_ref(node.ref) in ("tag", "sha"):
            code, pin_sha = run_git(["rev-parse", "--verify", f"{node.ref}^{{commit}}"],
                                    cwd=Path(args.packs_dir) / name)
            _c, head_sha = run_git(["rev-parse", "HEAD"], cwd=Path(args.packs_dir) / name)
            if code == 0 and head_sha and pin_sha != head_sha:
                warnings.append(
                    f"{name}: local checkout {head_sha[:12]} does not match pin "
                    f"{fmt_ref(node.ref)}; local wins at build time (rigpack sync to match)")
        if state and state["dirty"]:
            warnings.append(f"{name}: local checkout is dirty")
    return errors, warnings, resolver


def cmd_check(args):
    errors, warnings, _resolver = collect_problems(args, args.manifest)
    for w in warnings:
        print(f"WARN: {w}")
    for e in errors:
        print(f"FAIL: {e}", file=sys.stderr)
    if errors:
        print(f"\ncheck failed: {len(errors)} problem(s)", file=sys.stderr)
        return 1
    print(f"OK: {args.manifest} pins resolve cleanly"
          + (f" ({len(warnings)} warning(s))" if warnings else ""))
    return 0


def prefetch_remotes(nodes):
    """Hit pack remotes in parallel so outdated/update is not N serial clones."""
    urls = []
    seen = set()
    for node in nodes:
        if node.url and node.url not in seen:
            seen.add(node.url)
            urls.append(node.url)
    if not urls:
        return
    workers = min(8, len(urls))
    with ThreadPoolExecutor(max_workers=workers) as pool:
        list(pool.map(remote_info, urls))


def update_flags(args):
    latest = getattr(args, "latest", False)
    float_refs = getattr(args, "float_refs", False)
    pin = getattr(args, "pin", False)
    allow_breaking = getattr(args, "allow_breaking", False)
    if float_refs and pin:
        print("error: --float and --pin cannot be combined", file=sys.stderr)
        return None
    return {
        "allow_breaking": allow_breaking,
        "latest": latest,
        "float_refs": float_refs,
        "pin": pin,
    }


def update_plan(args, manifest):
    """Resolve + compute candidates. Returns (root, resolver, plan) or (None, None, None)."""
    flags = update_flags(args)
    if flags is None:
        return None, None, None
    resolver = Resolver(args.packs_dir)
    root = resolver.resolve(manifest)
    prefetch_remotes(resolver.nodes.values())
    plan = []  # (node, candidate, kind, note)
    wanted = set(args.pack or [])
    for name, node in resolver.nodes.items():
        if wanted and name not in wanted:
            continue
        if not node.url:
            plan.append((node, None, "", "no url known"))
            continue
        candidate, kind_or_err = candidate_for(node, **flags)
        if candidate is None:
            plan.append((node, None, "", kind_or_err))
        else:
            plan.append((node, candidate, kind_or_err, ""))
    return root, resolver, plan


def cmd_outdated(args):
    _root, _resolver, plan = update_plan(args, args.manifest)
    if plan is None:
        return 2
    stale = 0
    print(f"{'PACK':28} {'CURRENT':16} {'CANDIDATE':16} NOTE")
    print(f"{'-' * 28} {'-' * 16} {'-' * 16} ----")
    for node, candidate, kind, note in plan:
        current = fmt_ref(node.ref) if node.ref else "(nested)"
        if candidate is None:
            print(f"{node.name:28} {current:16} {'-':16} {note}")
            continue
        if kind == "branch":
            info = remote_info(node.url)
            remote_sha = (info.branches.get(candidate) or info.head_sha)
            state = local_git_state(args.packs_dir, node.name)
            local = (state or {}).get("head") or ""
            if remote_sha and local and (
                    remote_sha.startswith(local) or local.startswith(remote_sha[:12])):
                print(f"{node.name:28} {current:16} {candidate:16} current (float)")
            elif remote_sha:
                stale += 1
                print(f"{node.name:28} {current:16} {candidate:16} "
                      f"pull --sync (HEAD {remote_sha[:12]})")
            else:
                print(f"{node.name:28} {current:16} {candidate:16} float")
            continue
        if is_current(node, candidate):
            print(f"{node.name:28} {current:16} {fmt_ref(candidate):16} current")
        else:
            stale += 1
            verb = "pin" if kind == "sha" and classify_ref(node.ref) == "branch" else "update"
            print(f"{node.name:28} {current:16} {fmt_ref(candidate):16} {verb} ({kind})")
    print(f"\n{stale} pack(s) can move.")
    print(f"  stay floating:  rigpack update {args.manifest} --latest --sync")
    print(f"  pin snapshot:   rigpack update {args.manifest} --pin")
    return 0


def cmd_update(args):
    manifest_path = Path(args.manifest)
    root, resolver, plan = update_plan(args, args.manifest)
    if plan is None:
        return 2
    if resolver.errors:
        for err in resolver.errors:
            print(f"ERROR: {err}", file=sys.stderr)
        return 1
    by_name = {node.name: (candidate, kind, note) for node, candidate, kind, note in plan}

    deps = root.get("dependencies")
    if not isinstance(deps, list):
        deps = []
        root["dependencies"] = deps
    changed = []

    def apply_entry(dep):
        name = dep.get("name")
        if name not in by_name:
            return
        candidate, _kind, note = by_name[name]
        if candidate is None:
            print(f"  {name}: skipped ({note})")
            return
        if is_current(resolver.nodes[name], candidate):
            return
        old = dep_ref(dep) or "(none)"
        dep["ref"] = candidate
        dep.pop("branch", None)
        changed.append(f"{name}: {fmt_ref(old)} -> {fmt_ref(candidate)}")

    for dep in deps:
        if isinstance(dep, dict):
            apply_entry(dep)

    # Flatten transitive packs into app manifests so the root pin governs
    # every pack the build will load (root wins in RigKitPacks.cmake).
    is_app = manifest_path.name == "app.json"
    if is_app and not args.no_flatten:
        listed = {d.get("name") for d in deps if isinstance(d, dict)}
        for name, node in resolver.nodes.items():
            if name in listed:
                continue
            if args.pack and name not in by_name:
                continue
            candidate, _kind, note = by_name.get(name, (None, "", "not planned"))
            if candidate is None:
                print(f"  {name}: transitive, skipped ({note})")
                continue
            deps.append({"name": name, "url": node.url, "ref": candidate})
            changed.append(f"{name}: (transitive) -> {fmt_ref(candidate)}")

    if not changed:
        print("Manifest refs are current.")
        floating = any(classify_ref(node.ref) == "branch" for node, _c, _k, _n in plan)
        if floating and not args.sync:
            print("Floating branch refs keep their name. "
                  "Pass --sync to check out origin/<branch>, or --pin to snapshot SHAs.")
    else:
        print("\n".join(changed))
    if args.dry_run:
        print("(dry run; manifest not written)")
        return 0
    if changed:
        with open(manifest_path, "w", encoding="utf-8") as f:
            json.dump(root, f, indent=2, ensure_ascii=False)
            f.write("\n")
        print(f"Wrote {manifest_path}")
    if args.sync:
        return sync_manifest(args, manifest_override=str(manifest_path))
    if changed:
        print(f"Next: rigpack sync {args.manifest} && rebuild")
    return 0


def sync_one(args, node):
    """Clone or check out one pack at its effective ref. Returns ok bool."""
    packs_dir = Path(args.packs_dir)
    path = packs_dir / node.name
    ref = node.ref or FLOATING_DEFAULT
    if not path.is_dir():
        if not node.url:
            print(f"  {node.name}: no url; cannot clone")
            return False
        print(f"  {node.name}: cloning {node.url}")
        code, _out = run_git(["clone", "--quiet", node.url, str(path)], timeout=600)
        if code != 0:
            print(f"  {node.name}: clone failed")
            return False
    if not (path / ".git").exists():
        print(f"  {node.name}: plain tree (not git); left as-is")
        return True
    state = local_git_state(packs_dir, node.name)
    if state and state["dirty"]:
        print(f"  {node.name}: dirty checkout; skipped (commit or stash first)")
        return False
    run_git(["fetch", "--tags", "--quiet", "origin"], cwd=path, timeout=300)
    code, _out = run_git(["checkout", "--quiet", ref], cwd=path)
    if code != 0:
        print(f"  {node.name}: cannot check out {fmt_ref(ref)}")
        return False
    if classify_ref(ref) == "branch":
        code, _out = run_git(["pull", "--ff-only", "--quiet"], cwd=path, timeout=300)
        if code != 0:
            print(f"  {node.name}: checked out {ref}; fast-forward pull skipped/failed")
    _c, head = run_git(["rev-parse", "--short", "HEAD"], cwd=path)
    print(f"  {node.name}: at {fmt_ref(ref)} ({head})")
    return True


def sync_manifest(args, manifest_override=None):
    manifest = manifest_override or args.manifest
    ok = True
    done = set()
    for _round in range(4):
        resolver = Resolver(args.packs_dir)
        resolver.resolve(manifest)
        todo = [n for n in resolver.nodes.values() if n.name not in done]
        if not todo:
            break
        for node in todo:
            done.add(node.name)
            if not sync_one(args, node):
                ok = False
        # Newly cloned packs may reveal deeper nested deps; loop re-resolves.
    return 0 if ok else 1


def cmd_sync(args):
    if not args.manifest:
        print("sync needs a manifest (app.json or pack.json). "
              "Host basics are submodules: git submodule update --init --recursive",
              file=sys.stderr)
        return 2
    return sync_manifest(args)


def main(argv=None):
    script_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(
        prog="rigpack",
        description="RigKit pack manager: resolve, check, update, sync pack pins.")
    parser.add_argument("--rigkit", default=str(script_root),
                        help="RigKit root that owns packs/ (default: this tool's repo)")
    parser.add_argument("--packs-dir", default=None,
                        help="packs directory (default: <rigkit>/packs)")
    sub = parser.add_subparsers(dest="command", required=True)

    def add_manifest(p, required=False):
        p.add_argument("manifest", nargs=None if required else "?",
                       help="app.json or pack.json to operate on")

    p_status = sub.add_parser("status", help="resolved graph + local checkout state")
    add_manifest(p_status)
    p_status.add_argument("--build", default=None, help="build dir to check RIGKIT_DIR against")
    p_status.add_argument("--strict", action="store_true",
                          help="exit 1 on dirty/diverged/behind checkouts or bad RIGKIT_DIR")
    p_status.set_defaults(fn=cmd_status)

    p_check = sub.add_parser("check", help="offline gate for a manifest")
    add_manifest(p_check, required=True)
    p_check.add_argument("--pinned", action="store_true",
                         help="fail on floating branch refs (product release gate)")
    p_check.set_defaults(fn=cmd_check)

    def add_update_policy(p):
        p.add_argument("--latest", action="store_true",
                       help="newest tag (crosses MAJOR) or default-branch HEAD for SHA pins")
        p.add_argument("--float", dest="float_refs", action="store_true",
                       help="write branch names (unpin); do not snapshot SHAs")
        p.add_argument("--pin", action="store_true",
                       help="snapshot floating refs to SHAs (or tags when the pack has them)")
        p.add_argument("--allow-breaking", action="store_true",
                       help="allow crossing MAJOR (or 0.x MINOR) lines on tag pins")
        p.add_argument("--pack", action="append", help="only this pack (repeatable)")

    p_out = sub.add_parser("outdated", help="compare refs with pack remotes")
    add_manifest(p_out, required=True)
    add_update_policy(p_out)
    p_out.set_defaults(fn=cmd_outdated)

    p_upd = sub.add_parser("update", help="advance refs in a manifest")
    add_manifest(p_upd, required=True)
    add_update_policy(p_upd)
    p_upd.add_argument("--no-flatten", action="store_true",
                       help="do not add transitive packs to an app manifest")
    p_upd.add_argument("--dry-run", action="store_true", help="print changes, write nothing")
    p_upd.add_argument("--sync", action="store_true",
                       help="check out packs at the refs afterwards")
    p_upd.set_defaults(fn=cmd_update)

    p_sync = sub.add_parser("sync", help="clone/check out packs at manifest pins")
    add_manifest(p_sync)
    p_sync.set_defaults(fn=cmd_sync)

    args = parser.parse_args(argv)
    if not args.packs_dir:
        args.packs_dir = str(find_default_packs_dir(args.rigkit))
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
