# Includes and umbrella headers

## Umbrella headers

- `core/U_core.h` — core types (`RigKitEngine`, `IApp`, pack host, …)
- `rendering/U_rendering.h` — rendering managers / registries
- `ecs` components and `MEcs` — include from `ecs/` as needed

UI lives in the `rigImGui` pack, not under `src/ui/`. Do not include Dear ImGui from host core.

```cpp
#include "core/U_core.h"
#include "rendering/U_rendering.h"
```

## Core vs packs

Packs may include host headers (`core/`, `ecs/`, `rendering/…`). Host `src/` must not include pack headers — that keeps the thin core and the `IMui` / ImGui boundary.

In app and example `.cpp` files, put host includes before pack includes:

```cpp
#include "core/RigKitEngine.h"
#include "core/pack/MPack.h"
#include "packs/rigComponent/src/rigComponent.h"
#include "packs/rigSystems/src/rigSystems.h"
```

`.clang-format` `IncludeCategories` sort quoted `"core|ecs|rendering/…"` ahead of `"packs/…"`. `IncludeBlocks: Preserve` keeps blank-line groups (e.g. `"app.h"` alone above the rest). Short pack names from `pack.json` include paths (`"rig.h"`, `"Mui.h"`) are not path-classified — prefer repo-relative `packs/…` in host examples when order matters, or keep a blank line between host and pack blocks.

This is include / dependency order, not runtime register order. Bootstrap: [packs_using.md](packs_using.md) (`rigComponent` → `rigSystems` → `rigImGui`).

## CMake include paths

The root `CMakeLists.txt` exposes `src/`, `src/core`, `src/core/pack`, `src/rendering`, `src/ecs`, and `src/core/canvas` on the `rigkit` target. Prefer project-root-relative includes (e.g. `#include "core/RigKitEngine.h"`).

## Best practices

- Add new public headers to the matching umbrella when one exists.
- Prefer forward declarations in headers; include umbrellas in `.cpp` files.
- Pack include paths come from each pack’s `pack.json`.
