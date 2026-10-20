# Examples Directory

The `examples/` directory holds in-repo **example apps** built on RigKit. Product apps that are not framework references should live in separate repositories (see [`templates/app`](../templates/app/) and [apps_creating.md](apps_creating.md)).

## Names

**One id:** folder = `app.json` `"name"` = CMake target (the build keeps `-` in the target).

| Kind | Id | Where |
|------|----|-------|
| Host example | `example-<Name>` | `examples/example-<Name>/` in this repo |
| Product app | `app-<Name>` | Its own repo, started from [`templates/app`](../templates/app/) |

Pack demos under `packs/<id>/examples/<short>/` stay a short id. They are the pack's demo, not a host example and not a product app.

Examples already in this tree keep the id they have: `minimal`, `oscHost`, `glEditor`, `calendar`, `svgEditor`, `ufomat`. New folders use the prefix.

## Structure

- Each example lives in its own subdirectory under `examples/` (e.g., `examples/oscHost/`).
- Each example contains `app.h`, `app.cpp`, `main.cpp`, `app.json`, and a `CMakeLists.txt` that nests RigKit and calls `add_rigkit_application`.
- Configure per example: `cmake -S examples/<name> -B examples/<name>/build`.
- Binaries land under `examples/<name>/build/bin/` with runtime `data/` next to the exe.

## In-tree examples

| Example | Role |
|---------|------|
| `minimal` | Creator + mesh + hierarchy proof |
| `oscHost` | Contract / host reference (`--author` / `--show`; OSC via `rigOsc`, `--smoke-osc`) |
| `glEditor` | GLSL shader preview shell |
| `calendar` | Content scheduler - `CContentItem` month view persisted with **rigSQLite** (needs local `packs/rigSQLite/` until the pack remote exists; not in host CI yet) |
| `ufomat` | UFOMAT - UFO source editor (`rigFontComponent` + `rigFont` + `rigUfo`; `--smoke` / `--smoke-ttf`) |
| `svgEditor` | Artboard Kit - toolbar, layers, SVG / DXF import (`--smoke`) |
| `example-svgAnimator` | SVG path playback - `rigSvg` into `CPath`, `CTween` / `STweens` slides one path (`--smoke`) |
| `example-pixelEditor` | Pixel layers, paint, Rig brush packs, PSD IO (`--smoke`) |
| `example-studio` | Vector + Pixel crafts; App > Workspace switches chrome (`--smoke`) |
| `example-stage` | Stage - 3D scene, materials, Jolt spheres/cloth, glTF prop, Python `stage` API. Editor target plus `example-stage-play` (`--smoke`, `--present`, doctest) |

Product apps (out of tree) live next to RigKit, not under `examples/`. `../app-svgAnimator` plays SVG paths. `../Relay` is [Relay](https://github.com/GitBruno/relayout), the book layout host.

## Adding a new example

1. Copy an example: `cp -r examples/minimal examples/example-myNewDemo`
2. Edit `app.json` so `"name"` is `example-myNewDemo` (folder = name = CMake target), plus SPDX `license` and pack `dependencies`. Implement `IApp` in `app.h` / `app.cpp`.
3. Configure and build from that folder:

```bash
cmake -S examples/example-myNewDemo -B examples/example-myNewDemo/build
cmake --build examples/example-myNewDemo/build --target example-myNewDemo
```

For a product app outside this repo, see [apps_creating.md](apps_creating.md).

## Framework Integration

- Examples use the shared framework in `src/`:
  - Entity-Component-System (ECS)
  - Rendering backends (OpenGL, Blend2D via packs, etc.)
  - UI via optional packs such as `rigImGui`
- Pack deps are declared per-example in `app.json` and resolved by `cmake/RigKitPacks.cmake`.
- `app.json` must include SPDX `"license"` (same gate as `pack.json` - CI / `check-invariants`).

## Minimal skeleton

```cpp
#include "core/U_core.h"

class MyApp : public rigkit::IApp {
public:
    MyApp() {
        window().title = "My Sketch"; // GLFW OS title = app name (not document name)
        window().width = 800;
        window().height = 600;
        window().samples = 4; // MSAA at window create; Canvas FBOs inherit unless CanvasSettings.samples is set
    }

    void setup()  override {
        // Sketches (clean canvas until Ctrl+E): m_engine->enableEditMode(true);
    }
    void update(float dt) override {}
    void draw()   override {}
};

int main(int argc, char *argv[]) {
    auto app = std::make_unique<MyApp>();
    rigkit::RigKitEngine engine(std::move(app), {}, argc, argv);
    engine.run();
}
```

See `examples/oscHost/` for the host reference (author + show modes).

## How to build an example

1. Configure from the example folder:
   ```sh
   cmake -S examples/oscHost -B examples/oscHost/build
   ```
2. Build (target name from `app.json` `name`, or the folder name):
   ```sh
   cmake --build examples/oscHost/build --target oscHost
   ```
3. Run from `examples/oscHost/build/bin/`.

## Framework usage notes

- Each example implements `IApp` and is started from `main.cpp` via `RigKitEngine`.
- Prefer umbrella headers where available (e.g., `#include "core/U_core.h"`).
