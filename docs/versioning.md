# Versioning

RigKit (this host) uses [Semantic Versioning](https://semver.org/) (`MAJOR.MINOR.PATCH`). **One source of truth:** [`cmake/VERSION`](../cmake/VERSION). CMake `project()`, Doxygen `PROJECT_NUMBER`, and `rigkit::version()` read that file. Do not copy the number into README, the site, or comments.

The file lives under `cmake/` because `${RIGKIT_ROOT}` is a public include path. A repo-root `VERSION` would be picked up as C++ `<version>` on Windows (case-insensitive includes).

App versions live in `app.json`. Pack versions live in `pack.json`. The Contract version is [`docs/contract/RigWorks/VERSION`](contract/RigWorks/VERSION) - a different number.

| Change | Bump |
|--------|------|
| Breaking public host API (`IApp` / `IMui` / `IPack` / public `src/` headers) | `MAJOR` |
| Additive public host API | `MINOR` (`1.1.0`, ...) |
| Fix with no API shape change | `PATCH` |

When you bump `cmake/VERSION`, add a History row below (release notes, not a second source of truth). A removed or renamed public symbol must appear there with its replacement (callers update; no forever shim — [contributing.md](contributing.md#api-changes)). Tag releases `v1.0.0`, `v1.1.0`, .... `tools/check-invariants` requires `cmake/VERSION` to be SemVer.

## Pack versions

Each pack versions itself in its own `pack.json` `"version"` — that field is the pack's source of truth, and `tools/check-invariants` requires it to be SemVer. The host number above does not version packs.

| Pack change | Bump |
|-------------|------|
| Breaking public pack API, or a POD / wire change (field or token removed, renamed, or re-meaned; migration in the owning codec does not make it additive) | `MAJOR` (on `0.x`: `MINOR`) |
| Additive API or POD field with a safe default | `MINOR` (on `0.x`: `PATCH`) |
| Fix with no API, POD, or wire shape change | `PATCH` |

Release a pack from its own repo: bump `pack.json`, commit, tag that commit `vMAJOR.MINOR.PATCH`, then push the commit and the tag. The tag and the `pack.json` version at that commit must match; `rigpack check` flags a checkout whose exact tag disagrees with its `pack.json`.

## Pins and updates (rigpack)

Apps and packs declare intent in `app.json` / `pack.json` dependencies (`name` + `url` + `ref`). The root manifest's pin wins over nested `pack.json` requests; `cmake/RigKitPacks.cmake` says so at configure time when they differ, and warns on conflicting requests no root pin settles. A local `packs/<name>/` checkout wins over the pin at build time (pack development); configure warns when it does not match a pinned tag or SHA.

Manage pins with `tools/rigpack.py` (Python + git, no registry):

```bash
python tools/rigpack.py status examples/oscHost/app.json
python tools/rigpack.py outdated <app.json>
python tools/rigpack.py update <app.json> --latest --sync   # pull origin/main; keep floating refs
python tools/rigpack.py update <app.json> --pin             # snapshot floats to SHAs / tags
python tools/rigpack.py update <app.json> --float           # rewrite pins back to main
python tools/rigpack.py check <app.json>                    # conflicts + SemVer; floating refs are allowed
python tools/rigpack.py check <app.json> --pinned           # product release: fail on floating refs
```

A `"ref": "main"` (or any branch) is a **track**, not a pin: `update` leaves it alone and `--latest --sync` fast-forwards the checkout. A tag or SHA is a **pin**: `update` moves tags to the newest compatible SemVer (same `MAJOR`; on `0.x` also same `MINOR`; `--latest` / `--allow-breaking` cross). `--pin` snapshots a track to a SHA (or a tag when the pack has them). `--float` is the reverse. Product apps that need a reproducible install pin, then `check --pinned`. In-tree examples stay on `main`. Details: [packs/README.md](../packs/README.md#pinning).

## History

| Version | Notes |
|---------|-------|
| unreleased | `IWindow` renamed to `ImGuiPanel` (rigImGui panel base — not a host `I*`). `MWindow` stays the dock manager. Packs use `asMui` / `windowManager(IMui*)` or held pointers; do not grow `IMui` with a panel factory. Named surfaces on `IMui` remain the portable show/hide agreement; ESP web fulfills POD without `IMui`. |
| unreleased | `CUiPanel` / `CUiGroup` / `CUiControl` / `CUiAction` match `rig.ui.*`. Creators: `rig::makeUiPanel` / `makeUiControl` / `makeUiAction`. Contract import writes the POD (refs from document ids). **rigImGui** presents visible panels. Actions fire `SEvent`. `oscHost --author` Look panel binds the `look` entity. |
| unreleased | `IMui::getWindowManager` removed. Create and show panels through `Mui::getWindowManager` (include rigImGui). `IMui::chromeId()` default is `"none"`; `Mui` returns `"imgui"`. |
| unreleased | Removed `IStrokeUI`. Removed `IManager`; `MEcs` / `MPack` / `MRendering` / `MCanvas` / `MSettings` keep `init` / `shutdown` as ordinary methods. |
| unreleased | `IRenderer`, `Paint`, `RendererType`, `GradientType`, `GradientStop`, `createRenderer`, and `ISettings` live in `namespace rigkit`. |
| unreleased | Pack version management: `tools/rigpack.py` (`status` / `check` / `outdated` / `update` / `sync`). `"ref": "main"` is a track (`update --latest --sync` pulls without SHA-ifying); `--pin` / `--float` switch snapshot vs track; `check --pinned` is the product release gate. Root `app.json` pins win over nested `pack.json` requests in `RigKitPacks.cmake`. Pack `pack.json` versions are SemVer-gated by `check-invariants`. Replaces `tools/pack-status.sh` and `tools/update-packs.sh`. |
| unreleased | `CCompositePass::BlendMode` ordinals match `CRenderBlend` (`add`=6, `color-dodge`=12, `disabled`=18). Tokens unchanged; raw ints in old documents remap. |
| unreleased | `CPlotLayer` no longer owns mask; `CCompositePass` does. Wire: `x.rigkit.composite_pass`. Old `plot_layer` mask keys migrate on load. |
| unreleased | Removed `IMui::registerFontAtlasHook`. Packs that add fonts call `Mui::registerFontAtlasHook`. |
| unreleased | `PagesWindow` lives on **rigDocumentShell** (`PagesWindow.h`). |
| unreleased | Removed `IPack` event / parameter maps (`addEventListener`, `setParameter`, `onParameterChanged`, ...) and `MPack` `addGlobalEventListener` / `triggerGlobalEvent`. Pack identity stays on `pack.json`; live data stays on ECS. |
| unreleased | Removed `author/rigDraw.h` (`rig::` immediate helpers over Graphics). Use `rig/sketch.h` (`rig::circle` and friends) or `rig::make*` creators. |
| unreleased | Removed `Canvas` draw / text / matrix methods (`fill`, `rect`, `circle`, `text`, `pushMatrix`, ...). Use `rig::` sketch or creators. `Graphics` stays as the internal facade. |
| unreleased | Removed `Canvas::background` (alias of `clear`). Use `clear`. |
| **1.0.0** | Host SemVer source of truth. CMake, Doxygen, About, and `rigkit::version()` read `cmake/VERSION`. Pack `pack.json` version is applied onto `IPack`. |

