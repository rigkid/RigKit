# Build Instructions

## Prerequisites

* CMake ≥ 3.19
* A C++20-capable compiler (MSVC, Clang, or GCC)
* Git (for submodules)

Third-party code is vendored as git submodules (plus vendored GLAD). Prefer those over an external package manager.

Each example owns its build tree under `examples/<name>/build/`. Do not keep parallel `build-*` scratch dirs — reconfigure in place or delete. Those folders are gitignored but still litter the working copy. For a clean rebuild (e.g. after a stale dock layout sticks), delete `examples/<name>/build/` — this wipes objects, the exe, and deployed `<exeDir>/data/user/` (including saved UI layout `workspaces/imgui.ini`) — then reconfigure.

## Manual CMake (examples)

Examples are standalone projects (same shape as [`templates/app`](../templates/app/)): they nest RigKit via `add_subdirectory` and call `add_rigkit_application`.

```bash
git clone https://github.com/rigkid/RigKit.git && cd RigKit
git submodule update --init --recursive
cmake -S examples/oscHost -B examples/oscHost/build
cmake --build examples/oscHost/build --config Release --target oscHost
```

Output: `examples/oscHost/build/bin/` (`oscHost` exe + `data/` next to it).

### Optional: desktop ANGLE (GLES parity)

Validate Pi-like **OpenGL ES** on Windows/Linux desktop without bloating the Pi product path:

```bash
# Prebuilt ANGLE drop (include/ + libGLESv2 + libEGL), or vcpkg `angle`:
cmake -S examples/oscHost -B examples/oscHost/build \
  -DRIGKIT_USE_ANGLE=ON -DRIGKIT_ANGLE_ROOT=/path/to/angle
cmake --build examples/oscHost/build --target oscHost
```

- **Never** set `RIGKIT_USE_ANGLE=ON` on Raspberry Pi / ARM — native GLES is used automatically there.
- Default builds stay desktop OpenGL (fast authoring); ANGLE is opt-in for GLES parity checks.
- See [docs/contract/pi-host.md](contract/pi-host.md) and `cmake/RigKitAngle.cmake`.

### Iterate on one example

Prefer building **only** the app under edit so pack/core stay incremental:

```bash
cmake --build examples/oscHost/build --target oscHost
# or Debug:
cmake --build examples/oscHost/build --config Debug --target oscHost
```

Packs are separate static libraries (`rigComponent`, `rigSystems`, `rigImGui`, …). Edit a pack `.cpp` when possible so `librigkit` does not recompile.

### Runtime data next to the binary

Apps resolve assets from **`<exeDir>/data/`** (not the process working directory):

```
examples/oscHost/build/bin/oscHost.exe
examples/oscHost/build/bin/data/fonts/Roboto-Regular.ttf
examples/oscHost/build/bin/data/user/workspaces/...
```

CMake deploys `assets/` into each target’s `data/` on build. `AppPaths::init` runs from `RigKitEngine` using `argv[0]` / platform exe path APIs.

## Root tree (library docs / optional tools)

Root `cmake -S . -B build` does **not** build examples. Use it for API docs, headless contract smoke, or the ESP32 contract host self-test:

```bash
# Headless doctest smoke (core + spine packs; default ON at top level)
cmake -S . -B build -DRIGKIT_BUILD_CONTRACT_SMOKE=ON
cmake --build build --target contract_smoke
ctest --test-dir build --output-on-failure -R contract_smoke
```

See [tools/contract_smoke/README.md](../tools/contract_smoke/README.md).

```bash
cmake -S . -B build
cmake --build build --target docs
# open build/docs/api/html/index.html
```

```bash
cmake -S . -B build -DRIGKIT_BUILD_ESP32_CONTRACT_HOST=ON
cmake --build build --target esp32_contract_host
```

Optional API docs require [Doxygen](https://www.doxygen.nl/) on `PATH` (Windows: `winget install DimitriVanHeesch.Doxygen`). Graphviz `dot` is optional (class diagrams). Vendored GLFW has `GLFW_BUILD_DOCS=OFF` so it does not claim that name. Config: [`docs/api/Doxyfile.in`](api/Doxyfile.in).

```bash
cmake -S . -B build
cmake --build build --target docs
# open build/docs/api/html/index.html
```

Published sites (GitHub Actions → Pages on push to `main`):

| Site | URL |
|------|-----|
| Host landing | [https://rigkid.github.io/rigkit/](https://rigkid.github.io/rigkit/) |
| Host API (Doxygen) | [https://rigkid.github.io/rigkit/api/](https://rigkid.github.io/rigkit/api/) |
| One pack | `https://rigkid.github.io/<packName>/` |

Pack-scoped local generate: `./tools/generate-pack-docs.sh <packName>` ([`PackDoxyfile.in`](api/PackDoxyfile.in)). Host CI fetches optional remotes via [`tools/fetch-packs-for-docs.sh`](../tools/fetch-packs-for-docs.sh) and [`docs/api/pack-remotes.txt`](api/pack-remotes.txt). See [packs/README.md](../packs/README.md#api-docs--github-pages).

## Updating submodules

```bash
git pull
git submodule update --init --recursive
```

In-org packs are first-party packs in the rigkid org (own remotes), checked out under `packs/` as submodules or local clones. `git submodule update --init --recursive` covers them when tracked as submodules. Optional packs: pin with `"ref"` in `app.json`; refresh with `tools/update-packs`. New pack scaffold: `templates/rigTemplate` (see [packs/README.md](../packs/README.md)).
