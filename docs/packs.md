# Packs

Packs extend the host with UI, renderers, and tools. They live in separate repos and are cloned into `packs/` from an example’s `app.json` (see `cmake/RigKitPacks.cmake`). Local checkouts under `packs/` are gitignored except [packs/README.md](../packs/README.md).

**Catalog (Known + Planned):** [packs_catalog.md](packs_catalog.md).

## What a pack is

- Implements `rigkit::IPack` ([src/core/IPack.h](../src/core/IPack.h))
- Ships `pack.json` (name, `description`, `license` with holder, `url`, include paths, optional deps) and `CMakeLists.txt`
- Registers with `MPack` / `registerPack` at runtime when the app boots packs

**Identity owns one file.** `pack.json` is the source for description, license, url, version, and runtime dependency names. `add_rigkit_application` deploys it to `<exeDir>/data/packs/<name>/pack.json`; `MPack::registerPack` applies those fields onto `IPack` (About and settings readers pull from there; `initAll` topo-sorts from the dependency list). Do not call `setDescription` / `setLicense` / `setUrl` / `setVersion` / `addDependency` in the pack constructor - that is a second authoring copy and CI rejects it.

## Creating a pack

1. Create a folder (own repo or `packs/rigMyPack` while developing).
2. Inherit `IPack` and implement hooks (`init`, `setup`, `update`, `draw`, `cleanup`).
3. Add `pack.json` (`license` with holder is required - CI / `check-invariants` read it):

```json
{
  "name": "rigMyPack",
  "url": "https://github.com/rigkid/rigMyPack.git",
  "ref": "main",
  "version": "0.1.0",
  "author": "Rigkid",
  "license": "MIT Rigkid Contributors",
  "description": "My pack",
  "include_paths": ["src"]
}
```

`license` is SPDX plus the copyright holder from `LICENSE` (`MIT Rigkid Contributors`).
Use `GPL-2.0-or-later Rigkid Contributors` when the pack ships or links GPL
code - e.g. **rigPlotFinders** (Potrace). App manifests may keep bare `MIT`.

4. Minimal `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.19)
file(READ "${CMAKE_CURRENT_LIST_DIR}/pack.json" PACK_JSON)
string(JSON PACK_NAME GET "${PACK_JSON}" name)

add_library(${PACK_NAME} STATIC src/rigMyPack.cpp)
target_link_libraries(${PACK_NAME} PUBLIC rigkit)
target_include_directories(${PACK_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)
```

5. Optional: declare further packs via CPM inside the pack’s CMake so consumers only pull one package. Put pack demos in that pack’s `examples/` (not in this repo).

6. Ship **one example** under `examples/<name>/` (nest host + `add_rigkit_application`, same shape as `examples/minimal`). Link `examples/<name>/img/preview.png` from the pack README. See [packs/README.md](../packs/README.md).

## Using a pack from an example

List it under `dependencies` in `examples/<name>/app.json`:

```json
{
  "name": "my_example",
  "version": "0.1.0",
  "license": "MIT",
  "dependencies": [
    {
      "name": "rigImGui",
      "url": "https://github.com/rigkid/rigImGui.git",
      "ref": "main"
    }
  ]
}
```

SPDX `"license"` is required on `app.json` as well (CI / `check-invariants`).

In the app, register the pack in `bootstrapPacks()` / `setup()` (see `examples/oscHost`).

## Using ECS from a pack

Receive `MEcs*` from the engine (or via your pack’s setup). Prefer the pack split:

- **rigComponent** - data-only PODs + `ecs->registerComponent<T>(...)` in `setup()`
- **rigSystems** - `ecs->registerSystem(name, Update|Draw, fn)`
- **rigProject** - host project envelope (`CProject`/`CPage`) + `.rig` document IO

See [packs_using.md](packs_using.md). Prefer POD for anything portable ([docs/contract/rigkit.md](contract/rigkit.md)).

## rigImGui boundary

Host core (`src/`) must not include Dear ImGui. UI packs such as `rigImGui` implement `IMui`. See [contract/ui.md](contract/ui.md).
