# Authoring with RigKit

RigKit is data-first and still friendly to write.

**Friendly** means: short `setup` / `update` / `draw`, helpers that do one obvious thing, sensible defaults, and a path where you create a shape and see it without learning managers first. Internals stay available for packs - they are not the default teaching surface.

## Happy path

Default window size, OS title, and MSAA `samples` live in `app.json` `window`. The title is the app name, not the open document. The constructor does not set them.

```cpp
class MyApp : public rigkit::IApp {
public:
	MyApp() {
		settings().graphics.showFps = true; // host corner overlay (Preferences > Show FPS)
	}
	void setup() override {
		// Bootstrap data + systems packs, then create entities:
		//   rig::makeRect / makeCircle / makeMesh*
		// Sketches that want a clean canvas until Ctrl+E:
		//   m_engine->enableEditMode(true);
		// Tool apps (docked GUI): leave the default (feature off).
	}
	void update(float dt) override { /* mutate data */ }
	void draw() override {
		// Optional custom GL. Host already presents ECS each frame.
	}
};
```

Open a file, change a few lines, **see something**.

Proof app: [`examples/minimal`](../examples/minimal/) - creates rect / circle / meshes / parent-child via `CRelationship`; host Draw paints them (selection overlay when `CSelection` is set).

## Creators (data) - `rigComponent`

Depends on packs: `rigComponent` + `rigSystems` in `app.json`, then register them in `setup()` (see `examples/minimal`).

With pack include paths from the manifest, prefer:

```cpp
#include "rig.h"       // umbrella for rig/create.h + rig/sketch.h
// or: #include "rig/create.h"
```

From a repo-rooted example that does not rely on pack include paths:

```cpp
#include "packs/rigComponent/src/rig.h"
```

```cpp
auto* ecs = engine->getECSManager();
rig::makeRect(*ecs, x, y, w, h, rig::fill(1, 0.4f, 0.2f));
rig::makeCircle(*ecs, cx, cy, r, rig::fillAndStroke(0.2f, 0.7f, 1.f, 1, 1, 1, 2.f));
rig::makeLine(*ecs, x1, y1, x2, y2, rig::stroke(1, 1, 1, 1.f, 3.f));

// Meshes are first-class PODs (CMesh).
rig::makeMeshTriangle(*ecs, a, b, c, rig::fill(0.95f, 0.8f, 0.2f));
rig::makeMeshQuad(*ecs, x, y, w, h, rig::fill(0.3f, 0.9f, 0.5f));

// 3D (needs rigRender3D Draw present):
rig::makeOrbitCamera(*ecs, {0.f, 0.f, 0.f}, 5.6f, 0.36f, true, "camera");
// Locked ortho face (`COrthoView`). Open a panel with rigkit::openOrthoView /
// registerOrthoViewMenu (rigImGui OrthoViewWindow) — not an example-local window.
rig::makeOrthoCamera(*ecs, rigkit::ecs::COrthoView::Face::Top);
rig::makeLight(*ecs, {0.f, 0.f, 0.f});
rig::makePalette(*ecs);
rig::makeMeshGrid(*ecs);

#include "rig/cad.h"   // CAD PODs - rigCadComponent
#include "rig/show.h"  // visualizer / analysis - rigShowComponent

// CAD solids (bake via rigManifold::bakeToMesh):
rig::makeCadBox(*ecs, 2.f, 1.5f, 1.2f, true, "stock");
rig::makeCadCylinder(*ecs, 0.4f, 2.f, true, "cutter");
rig::makeCadBoolean(*ecs, rigkit::ecs::CCadBoolean::Op::Difference, {"stock", "cutter"},
					rig::fill(0.95f, 0.55f, 0.25f), "part");

// Driving datum (apply via rigSolveSpace::solve - on edit, not every frame):
rig::makeCadDimension(*ecs, rigkit::ecs::CCadDimension::Kind::Horizontal, "stock", "cutter",
					  4.f, false, "span");

// Preset deck (needs a present pack + rigAudio for switchOn):
auto deck = rig::makeVisualizer(*ecs);
rig::addVisualizerPreset(*ecs, deck, "data/presets/warm.vis");
auto listen = rig::makeAudioAnalysis(*ecs);
rig::makePictureAnalysis(*ecs, output);
ecs->getComponent<rigkit::ecs::CVisualizer>(deck).output = output;
ecs->getComponent<rigkit::ecs::CVisualizer>(deck).analysis = listen;
```

These helpers only write POD (`CTransform` + shape / `CMesh` / `CCad*` / `COrbitDrive` / `COrthoView` + `CDrawStyle` / `CVisualizer` / `CAudioAnalysis` / `CPictureAnalysis`). `makeOrbitCamera` poses the eye; turn `COrbitDrive::enabled` on for a show-mode spin (`SOrbitDrive` yaws from `pitch`). `makeOrthoCamera` stays inactive so it never steals the main bed. `makeVisualizer` is a playlist; a leaf pack (rigMilk, ...) presents entries it understands and draws into `output`. Live `color` / `temperature` are host cache on the audio and picture buses - not fields on the deck.

## Orbit nav (code) - `rigSystems`

Mouse orbit / pan / dolly is `rig::orbitNavigate` (`OrbitNav.h`). Bindings stay in the app. The helper writes `COrbitDrive`. Drag last-xy lives on `OrbitNavState` (not on the component). `orbitFrameMeshes` is F-to-frame. `orbitFromView` is for a view cube.

```cpp
#include "OrbitNav.h"

rig::OrbitNavState nav; // app member

void update(float) {
	rig::OrbitNavFrame frame;
	frame.mouseX = mx;
	frame.mouseY = my;
	frame.orbit = mmb || (alt && lmb);
	frame.pan = (mmb && shift) || (alt && mmb); // truck + pedestal
	frame.dolly = alt && rmb;
	frame.wheel = wheel;
	frame.blocked = uiBlocks || !overBed;
	rig::orbitNavigate(*ecs, cam, frame, nav);
}
```

**Hierarchy:** local TRS lives on `CTransform`. Optional `CRelationship::parent` links a child to another entity. `SHierarchy` (Update + before present) fills `CTransform::world`; Draw uses that. Absent `CRelationship` means root.

```cpp
auto parent = rig::makeRect(*ecs, 200.f, 320.f, 120.f, 80.f, rig::fill(0.5f, 0.3f, 0.9f));
auto child = rig::makeRect(*ecs, 40.f, 30.f, 70.f, 50.f, rig::fill(0.95f, 0.55f, 0.2f));
rigkit::ecs::CRelationship rel;
rel.parent = parent;
ecs->addComponent<rigkit::ecs::CRelationship>(child, rel);
```

**Present path:** each frame the host sets the main OpenGL `IRenderer`, then runs Draw systems (`SCanvasRender` for an active Canvas FBO if any, then `SShapeRendering` for the window). Selected entities (`CSelection`) get a bounds stroke overlay.

**Drawing preview** (rubber-band / in-progress tool geometry) lives as transient fields on `CPathEditSelection` in **rigPlotComponent** (`previewActive` / endpoints); bed/artboard overlays present it. Not a core free function.

**Verify:** run `minimal` for creators/meshes/selection; run `oscHost` for host shell / Pi SUDE.

## Sketch (retained style) - `rig/sketch.h`

Fill and stroke once, then spawn inspectable entities. `circle(x, y, r)` takes radius. Each primitive calls a `make*` creator and tracks the entity so `clearSketch()` can wipe the last run.

```cpp
#include "rig/sketch.h"
#include "rig/pages.h"

rig::makePage(520.f, 380.f);
rig::setFill(1.f, 0.4f, 0.2f);
rig::circle(x, y, r);
rig::setPage(0);
rig::rect(40.f, 40.f, 220.f, 170.f);
// view: rig::focusPage(0);
```

`setFill` / `setStroke` are the retained-style setters. `fill` / `stroke` stay the `CDrawStyle` factories used by `make*`. This layer does not know pages; rigProject parents later sketch entities to `currentPage()`. `setSketchEntityHook` replaces that. `setPage(index)` moves placement; `focusPage` pans the board and updates the placement page too.

Python `sketch.py` is the same vocabulary (`fill`, `circle`, `setPage`, matrix, path) via the rigPython example bindings.

## Node graphs

Patch floats, vec2, colors, or any [standard datatype](contract/RigWorks/docs/properties.md) in the Node Editor.
Artist guide: [nodes.md](nodes.md). Example: `packs/rigNodeEditor/examples/nodes`.

## Crafts and workspaces

One host can link more than one craft pack. **App > Workspace** shows the panels for the job (Vector / Pixel) and docks them on load. The `.rig` document stays. See [`examples/example-studio`](../examples/example-studio/) and [`examples/example-pixelEditor`](../examples/example-pixelEditor/).

Brush packs are a folder with `brushes.json` plus optional tip PNGs - not Adobe ABR. Partner SKUs (True Grit Texture Supply) follow that format; see [partners/true-grit.md](partners/true-grit.md).

## Layers (mental model)

1. **Author API** - `IApp`, entity creators (`makeRect` / `makeMesh*`), optional `rig::` sketch helpers
2. **Data** - `rigComponent` PODs (`CShape`, `CMesh`, `CSelection`, ...) + catalog
3. **Fulfillment** - `rigSystems` Draw systems + host OpenGL `IRenderer`
4. **Host** - `RigKitEngine`, `AppPaths` (`<exeDir>/data`)

Agents changing APIs start at layer 1 and push weight down - never force artists to start at layer 3.
