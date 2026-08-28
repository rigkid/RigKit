---
name: rigkit-build
description: >-
  RigKit build and contribution workflow. Use when configuring CMake, building
  examples, Raspberry Pi compile/run verify, formatting with clang-format / tools/format,
  installing git hooks, CI style checks, running CI locally before commit, submodules,
  git commits, opening pull requests against main, or the optional ESP32 contract host
  target.
---

# RigKit build

Capability units are **packs** (`packs/`, `pack.json`, `cmake/RigKitPacks.cmake`). Spoken name = folder id = `pack.json` `"name"` = `app.json` `"name"` - [packs/README.md#naming](../../packs/README.md#naming).

## Pi gate (always)

Raspberry Pi is the minimum full host. Before calling work done:

1. **Will this compile on Pi?** (arm64 Raspberry Pi OS, CMake Release, GLES path)
2. **Will this run smooth on Pi?** (install Update/Draw loop - no desktop-only weight)

On-device checklist: [docs/contract/pi-host.md](../../docs/contract/pi-host.md).

```bash
# On Raspberry Pi
cmake -S examples/oscHost -B examples/oscHost/build -DCMAKE_BUILD_TYPE=Release
cmake --build examples/oscHost/build -j$(nproc) --target oscHost
./examples/oscHost/build/bin/oscHost --show
./examples/oscHost/build/bin/oscHost --author
```

Until a device is available: desktop smoke of `oscHost` + explicit Pi-risk review. Do not ship Windows/x86-only first-party code.

## Fast rebuilds (prototyping)

Slow full-tree rebuilds are a failure mode. Prefer:

- Build the **one example** under edit in its own tree: `cmake -S examples/oscHost -B examples/oscHost/build`
- Keep changes in **pack** STATIC libs when possible so `rigkit` does not recompile.
- Avoid widening widely included headers in `src/` (forces cascade). New weight goes in a pack `.cpp`.
- **Header vs implementation:** keep method bodies in `.cpp`, not in headers. Declarations (and trivial `= default` / pure virtual) stay in `.h`; constructors, getters with logic, `render()`, and anything that pulls heavy includes belong in `.cpp`. Empty `Foo.cpp` with “methods are inline in the header” is a rebuild-cost bug - fix it before commit. Templates / header-only third_party are the exception.
- Do not add heavy deps to core "for convenience."
- **Tidy trees:** examples use `examples/<name>/build/` only; root `build/` is optional (docs / ESP32). Never leave `build-*` scratch dirs - reconfigure in place or delete.
- ANGLE desktop GLES parity: `-DRIGKIT_USE_ANGLE=ON` + `RIGKIT_ANGLE_ROOT` or vcpkg `angle` - **never on Pi** (native GLES). Default desktop stays OpenGL.

## Prerequisites

CMake ≥ 3.19, C++20 compiler, Git. Init submodules before configure.

```bash
git submodule update --init --recursive
```

## Configure and build

Examples are standalone CMake projects (same shape as `templates/app`):

```bash
cmake -S examples/oscHost -B examples/oscHost/build
cmake --build examples/oscHost/build --config Release --target oscHost
# run: examples/oscHost/build/bin/
```

Clean rebuild: delete `examples/<name>/build/` (wipes objects, the exe, and deployed `data/user`, including saved UI layout `imgui.ini`), then reconfigure.

`add_rigkit_application` deploys each processed pack’s `pack.json` to `<exeDir>/data/packs/<name>/` so `MPack` can fill `IPack` identity (description / license / url) at register - [docs/packs.md](../../docs/packs.md).

Root tree (docs / optional tools only):

```bash
cmake -S . -B build
cmake --build build --target docs
```

Headless contract smoke (doctest + CTest; default ON at top level):

```bash
cmake -S . -B build -DRIGKIT_BUILD_CONTRACT_SMOKE=ON
cmake --build build --target contract_smoke
ctest --test-dir build --output-on-failure -R contract_smoke
```

ESP32 contract host self-test:

```bash
cmake -S . -B build -DRIGKIT_BUILD_ESP32_CONTRACT_HOST=ON
cmake --build build --target esp32_contract_host
```

Details: [docs/build_instructions.md](../../docs/build_instructions.md), [tools/contract_smoke/README.md](../../tools/contract_smoke/README.md).

## Pack pins and remotes

- Manifest pin: `"ref": "v0.1.0"` (tag / branch / SHA). Deprecated alias: `"branch"`.
- In-org packs (rigkid GitHub org remotes) live under `packs/`.
- Local `packs/<name>/` (submodule or clone) wins; else CPM clones at `ref`.
- Refresh: `./tools/update-packs.sh` (optional `--app examples/.../app.json`).
- Status: `./tools/pack-status.sh` (optional `--app`, `--build`, `--strict`) — dirty/ahead/behind and `RIGKIT_DIR` drift.
- New pack: survey [packs/README.md](../../packs/README.md) (table + Naming) before scaffolding; prefer growing an existing pack. Then [`templates/rigTemplate`](../../templates/rigTemplate/); publish template with `./tools/publish-template.sh`.
- Details: [packs/README.md](../../packs/README.md).

## API docs (optional)

Requires Doxygen on `PATH`. CMake adds target `docs` when found (GLFW docs disabled so the name is free):

```bash
cmake -S . -B build
cmake --build build --target docs
# open build/docs/api/html/index.html

./tools/generate-pack-docs.sh rigComponent
```

Published: [https://rigkid.github.io/rigkit/](https://rigkid.github.io/rigkit/) (landing), [https://rigkid.github.io/rigkit/api/](https://rigkid.github.io/rigkit/api/) (host Doxygen), and `https://rigkid.github.io/<pack>/` (pack `docs.yml`). Config: [docs/api/Doxyfile.in](../../docs/api/Doxyfile.in), [PackDoxyfile.in](../../docs/api/PackDoxyfile.in). Site source: [`site/`](../../site/). Rollout / Pages: [packs/README.md](../../packs/README.md#api-docs--github-pages). Comment tags: [rigkit-comments](../rigkit-comments/SKILL.md).

## API change sweep

When a public signature changes (`MPack`, `IApp`, `IMui`, pack headers), grep before building: `src/`, `packs/`, `examples/`, `tools/`, `templates/`, `docs/`, `.cursor/skills/`, `skills/`. Product apps live in their own repos and no gate here covers them - list the call sites you cannot reach and ask which to update.

## Local CI before commit (required)

**Do not commit until the local equivalent of CI passes.** GitHub CI has been failing when this gate is skipped. Run it yourself; fix failures; then commit.

**Every time before commit**, also:

0. **Header / impl split** - for every first-party class you touched (host `src/`, pack `src/`, example `app.*`), confirm non-trivial method bodies live in `.cpp`, not the header. No “stub `.cpp` + all inline in `.h`.” Keeps include cascades and incremental rebuilds cheap. See [Fast rebuilds](#fast-rebuilds-prototyping).

Mirror [`.github/workflows/ci.yml`](../../.github/workflows/ci.yml) for host / core / examples work:

```bash
# 0) Ten Commandments machine gates
./tools/check-invariants.sh   # Windows: tools\check-invariants.bat

# 1) style - format first-party trees, then dry-run on touched C/C++ (exclude third_party/)
./tools/format.sh   # Windows: tools\format.bat
clang-format --dry-run --Werror <changed .cpp/.h/.c/.hpp under src/ examples/ tools/>

# 2) headless - contract smoke
cmake -S . -B build -G Ninja -DRIGKIT_BUILD_CONTRACT_SMOKE=ON
cmake --build build --target contract_smoke
ctest --test-dir build --output-on-failure -R contract_smoke

# 3) examples - same matrix as CI
cmake -S examples/minimal -B examples/minimal/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build examples/minimal/build
cmake -S examples/oscHost -B examples/oscHost/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build examples/oscHost/build
```

Commandments tablet: [docs/contract/commandments.md](../../docs/contract/commandments.md).

On Windows, omit `-G Ninja` if Ninja is not installed (default generator is fine). Scope builds to what you touched when the change is narrow, but **never skip** style + `contract_smoke` for core / spine / format-sensitive edits, and never skip the example(s) whose `app.json` / sources you changed.

When changing an in-org pack, also build that pack’s examples (same as the pack’s `.github/workflows/ci.yml`):

| Pack | Example | Configure / build from host root |
|------|------|----------------------------------|
| rigComponent | `creators` | `packs/rigComponent/examples/creators` |
| rigSystems | `present` | `packs/rigSystems/examples/present` |
| rigProject | `document` | `packs/rigProject/examples/document` |
| rigImGui | `host_shell` | `packs/rigImGui/examples/host_shell` |
| rigManifold | `csg` | `packs/rigManifold/examples/csg` |
| rigMeshEdit | `gizmo` | `packs/rigMeshEdit/examples/gizmo` |
| rigPlotter (integration) | `plot` + `*_smoke` | `packs/rigPlotter/examples/plot` |
| Every other pack | its `examples/<name>/` | see pack README (`img/preview.png` required) |

```bash
cmake -S packs/<pack>/examples/<name> -B packs/<pack>/examples/<name>/build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build packs/<pack>/examples/<name>/build
# plot integration + unit smokes:
cmake --build packs/rigPlotter/examples/plot/build --target plot plot_unit_smokes
```


If local CI fails, stop and fix - do not commit “and let CI catch it.”

## GitHub Actions private remotes

Host + in-org pack remotes are private. Jobs that `checkout` with `submodules: recursive` (or pack CI that checks out `rigkid/RigKit`) need secret **`RIGKIT_CI_TOKEN`** - a fine-grained PAT with Contents:read on those remotes (org secret preferred). Without it, GitHub CI fails at clone with `Repository not found`. See [docs/contributing.md](../../docs/contributing.md). Local CI above does not need this secret.

## Git commits

- **Run [Local CI before commit](#local-ci-before-commit-required) first.** Do not create the commit while style, `contract_smoke`, required examples, or touched pack examples are red.
- Don't trail in git commits. No `Made-with: Cursor`, `Co-authored-by: Cursor`, or other AI / tool attribution trailers unless the user explicitly asks.
- Commit only when asked. Message = why (1-2 sentences); match recent `git log` style.
- Never `--no-verify`, force-push main/master, or amend unless the user’s commit rules allow it.

## Pull requests

Contribute does not end at commit. [docs/contributing.md](../../docs/contributing.md) step 6: **open a PR against `main`**.

**When to open a PR** (use `gh`):

- The user asks to contribute, land, ship, merge, review, or **open / create a PR**
- You finished a focused change set the user wanted committed **for review** on a feature branch (not “commit only” / stage-only WIP)

**Do not** open a PR unprompted for local experiments, “set up without commit,” or pack/host publishes the user only asked to push to a private remote.

**How:**

1. Local CI green; commit(s) on a feature branch (not directly on `main` unless the user said so).
2. `git push -u origin HEAD` if the branch is not on the remote yet.
3. `gh pr create` against `main`. Body follows [`.github/pull_request_template.md`](../../.github/pull_request_template.md): Summary (why, 1-3 bullets), Ten Commandments checklist, Pi / rebuild-cost callouts when relevant, Test plan (`check-invariants`, format + `contract_smoke`, examples / pack examples touched).
4. Return the PR URL when done.

**Packs:** if the change lives in an in-org pack checkout (`packs/<name>/` with its own remote), open the PR on **that** pack repo (or commit+push `main` only when the user explicitly wants a direct pack land). Host submodule SHA bumps are a separate host PR when basics are pinned that way.

## Format

- Canonical: `.clang-format` (LLVM base, **tabs**, C++20, 100-col).
- Include sort: `.clang-format` `IncludeCategories` put host (`core/` / `ecs/` / `rendering/`) before `packs/`. Policy: [docs/includes.md](../../docs/includes.md).
- Run `tools/format.sh` or `tools/format.bat` on first-party trees: `src/`, `examples/`, `tools/`.
- **Never** format `third_party/` or `packs/` from the host. Packs format their own `src/` + `examples/` (`packs/<name>/tools/format.*`).
- Pre-commit: `tools/install-hooks.sh` (Windows: `tools\install-hooks.bat`) installs the host hook **and** the pack hook into every `packs/<name>` git checkout. CI `style` job runs `clang-format --dry-run --Werror` on changed first-party files.

## Layout reminders

- Product apps: separate repos; start from [`templates/app`](../../templates/app/).
- In-tree example apps: `examples/` (`minimal`, `oscHost`).
- Reference example: `examples/oscHost` (`--author` / `--show`).

## Sources

[docs/contributing.md](../../docs/contributing.md), [docs/build_instructions.md](../../docs/build_instructions.md), [docs/contract/pi-host.md](../../docs/contract/pi-host.md).
