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

## History

| Version | Notes |
|---------|-------|
| unreleased | `CCompositePass::BlendMode` ordinals match `CRenderBlend` (`add`=6, `color-dodge`=12, `disabled`=18). Tokens unchanged; raw ints in old documents remap. |
| unreleased | `CPlotLayer` no longer owns mask; `CCompositePass` does. Wire: `x.rigkit.composite_pass`. Old `plot_layer` mask keys migrate on load. |
| unreleased | Removed `IMui::registerFontAtlasHook`. Packs that add fonts call `Mui::registerFontAtlasHook`. |
| unreleased | `PagesWindow` lives on **rigDocumentShell** (`PagesWindow.h`). |
| unreleased | Removed `IPack` event / parameter maps (`addEventListener`, `setParameter`, `onParameterChanged`, ...) and `MPack` `addGlobalEventListener` / `triggerGlobalEvent`. Pack identity stays on `pack.json`; live data stays on ECS. |
| unreleased | Removed `author/rigDraw.h` (`rig::` immediate helpers over Graphics). Use `rig/sketch.h` (`rig::circle` and friends) or `rig::make*` creators. |
| unreleased | Removed `Canvas` draw / text / matrix methods (`fill`, `rect`, `circle`, `text`, `pushMatrix`, ...). Use `rig::` sketch or creators. `Graphics` stays as the internal facade. |
| unreleased | Removed `Canvas::background` (alias of `clear`). Use `clear`. |
| **1.0.0** | Host SemVer source of truth. CMake, Doxygen, About, and `rigkit::version()` read `cmake/VERSION`. Pack `pack.json` version is applied onto `IPack`. |

