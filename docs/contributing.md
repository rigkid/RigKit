# Contributing

## Automatic formatting

| Mechanism | What it does |
|-----------|----------------|
| [`.clang-format`](../.clang-format) | Canonical C++ style (LLVM base, **tabs**, C++20, 100-col) |
| [`.editorconfig`](../.editorconfig) | Tabs, UTF-8, LF, trim trailing whitespace, one blank line at EOF |
| [`tools/format.bat`](../tools/format.bat) / [`tools/format.sh`](../tools/format.sh) | Format all of `src/`, `examples/`, `tools/` (never `packs/`) |
| Pre-commit hook | Formats staged C/C++ before commit. Host hook skips `packs/`; each pack repo has its own hook |
| CI (`style` job) | `clang-format --dry-run --Werror` on changed first-party files |

**Requires:** `clang-format` on your PATH (LLVM / Visual Studio / MSYS).

```bash
# Format everything first-party
./tools/format.sh          # Unix
tools\format.bat           # Windows

# Install pre-commit (once per clone)
./tools/install-hooks.sh          # Unix — host + every packs/<name> git checkout
tools\install-hooks.bat           # Windows
```

Do **not** run the host formatter on `third_party/` or `packs/`. Each pack formats its own `src/` + `examples/` (`packs/<name>/tools/format.*`, or the pack pre-commit hook). Never format pack `third_party/`.

## Editor setup

`.vscode/` (and any other editor's project folder) is gitignored — your editor config is yours, not repo content. `.editorconfig` covers the cross-editor basics (tabs, LF, trailing newline) natively in most editors. `tools/format.*` and the pre-commit hook are the actual enforcement; format-on-save is a convenience, not a requirement.

If you use VS Code, install `ms-vscode.cpptools`, `ms-vscode.cmake-tools`, and `EditorConfig.EditorConfig`, then add this to your own local `.vscode/settings.json` to wire format-on-save to `.clang-format`:

```json
{
  "C_Cpp.formatting": "clangFormat",
  "C_Cpp.clang_format_style": "file",
  "[cpp]": { "editor.defaultFormatter": "ms-vscode.cpptools", "editor.formatOnSave": true },
  "[c]": { "editor.defaultFormatter": "ms-vscode.cpptools", "editor.formatOnSave": true },
  "[h]": { "editor.defaultFormatter": "ms-vscode.cpptools", "editor.formatOnSave": true }
}
```

## Style rules

- **Headers:** `#pragma once` only (no include guards).
- **Indentation:** tabs, width 4 (never spaces for indent).
- **EOF:** exactly one blank line at the end of the file (trailing newline; no extra empty lines stacked after the last content).
- **Language:** modern C++20; prefer clear code over cleverness.
- **Includes:** clang-format `SortIncludes` only ([includes.md](includes.md)). Do not auto-strip; a missing include is worse than an extra one.
- **Casts:** prefer explicit casts for narrowing and enums.
- **Naming:** interfaces `I*` (`IApp`, `IMui`); managers often `M*` (`MEcs`); files match type names.
- **ECS:** core components live in `src/ecs/components/`; packs may define their own.
- **Data over hardcoding:** prefer config / properties where it keeps Canvas flexible.
- **UI:** no Dear ImGui includes in `src/` — see [contract/ui.md](contract/ui.md). Complete Contract = SUDE–ECS–UI.
- **UI text:** no emoji in UI or comments; rigImGui may use IconFont icons.

## Headless contract smoke

CI runs a doctest + CTest lane (`tools/contract_smoke`) that guards core + spine packs (data registration, `GetProperties`, systems boundary, header purity). No window / UI.

```bash
cmake -S . -B build -DRIGKIT_BUILD_CONTRACT_SMOKE=ON
cmake --build build --target contract_smoke
ctest --test-dir build --output-on-failure -R contract_smoke
```

Details: [tools/contract_smoke/README.md](../tools/contract_smoke/README.md).

## Versioning

Host SemVer is [`cmake/VERSION`](../cmake/VERSION). Bump rules and History: [versioning.md](versioning.md). App and pack versions stay in `app.json` / `pack.json`. The Contract number is [`contract/RigWorks/VERSION`](contract/RigWorks/VERSION).

## Pull requests

1. Fork / branch, keep changes focused.
2. Format with `tools/format.*` or rely on format-on-save + pre-commit.
3. **Run CI locally before commit / push** — `tools/check-invariants`, style dry-run, `contract_smoke`, and the examples (or pack example) you touched. See [rigkit-build](../skills/rigkit-build/SKILL.md#local-ci-before-commit-required). Do not rely on GitHub CI to catch build breaks.
4. Use the PR template checklist (Ten Commandments + Pi / rebuild-cost callouts).
5. Document public APIs with Doxygen tags (`@brief`, …) — see [rigkit-comments](../skills/rigkit-comments/SKILL.md). Generate HTML with `cmake --build build --target docs` ([build_instructions.md](build_instructions.md)). Published: [https://rigkid.github.io/rigkit/api/](https://rigkid.github.io/rigkit/api/) (host API), [https://rigkid.github.io/rigkit/](https://rigkid.github.io/rigkit/) (landing), and `https://rigkid.github.io/<pack>/` (each pack). See [packs/README.md](../packs/README.md#api-docs--github-pages).
6. Open a PR against `main` (agents: [`skills/rigkit-build`](../skills/rigkit-build/SKILL.md#pull-requests) — `gh pr create`, fill the PR template, return the URL).

## GitHub Actions (private pack remotes)

The host and the basics (`rigComponent`, `rigSystems`, `rigProject`, `rigImGui`) are public; most optional packs are private. Default `GITHUB_TOKEN` cannot clone a sibling private repo, so any job that pulls one — `examples/oscHost`, `examples/glEditor`, `angle-gles`, pack example CI — needs an Actions secret:

1. Create a fine-grained PAT with **Contents: Read** on `rigkid/RigKit` and the in-org pack remotes (basics + any packs CI builds via CPM).
2. Set `RIGKIT_CI_TOKEN` as an org secret with visibility **All repositories**, or as a per-repo secret.
3. Workflows pass that token to `actions/checkout` for basic submodules, and rewrite `github.com` URLs so CPM can clone optional packs.

Two visibility traps. `gh secret set --org` defaults to *private repositories only*, which skips public repos such as `rigkid/RigKit` — pass `--visibility all`. And on **GitHub Free**, [org secrets are never delivered to private repos](https://docs.github.com/actions/security-guides/using-secrets-in-github-actions); they arrive as an empty string with no warning, so each private pack repo needs its own repo secret until the org is on Team.

Without the secret, workflows warn and continue. Jobs that only touch public remotes still pass; jobs that reach a private pack fail at clone with `Repository not found`.

## AI collaboration

**We invite AI collaboration.** Agents and humans share one playbook — the Ten Commandments, the same gates, no special track.

- **[contract/commandments.md](contract/commandments.md)** — constitution (start here).
- **[AGENTS.md](../AGENTS.md)** — commandments + commentary (**NO CODE JUST DATA**, **Pi floor**, **fast rebuilds**, **author friendliness**).
- **[`tools/check-invariants`](../tools/check-invariants.sh)** — machine gates for ImGui-in-`src/`, data/code pack shape, banned vocabulary, archaeology phrasing.
- **[`tools/check-docs`](../tools/check-docs.sh)** — the Doxygen warning log must be empty. Run it after `cmake --build build --target docs`; the Docs workflow fails on any warning. A warning means text is missing from the HTML, not merely untidy.
- **`.cursor/rules/rigkit-core.mdc`** — Cursor's always-on hard-invariants reminder. Gitignored (local, not repo content) — see [AGENTS.md#tool-specific-folders-are-local-not-shared](../AGENTS.md#tool-specific-folders-are-local-not-shared).
- **[authoring.md](authoring.md)** — how users should code (helpers / creators, not manager ceremony).
- Iterate with `cmake -S examples/<name> -B examples/<name>/build` — see [build_instructions.md](build_instructions.md).
- **[skills/](../skills/)** — task skills (`rigkit-data`, `rigkit-contract`, `rigkit-build`, `rigkit-minimal`); tool-specific auto-discovery folders (`.cursor/skills/`, `.claude/skills/`) are local pointers, not tracked — see [AGENTS.md#tool-specific-folders-are-local-not-shared](../AGENTS.md#tool-specific-folders-are-local-not-shared).
- Packs: **rigComponent** (data) / **rigSystems** (code) / **rigProject** — see [packs_using.md](packs_using.md).

Contract docs remain the source of truth under [contract/](contract/README.md).
