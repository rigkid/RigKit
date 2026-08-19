# Using ECS packs from apps and packs

**One id** (spoken name = folder = manifests) plus suffixes (`*Component`, `*Editor`, `*Ui`) and what a **data pack** may contain: [packs/README.md](../packs/README.md#naming).

## Pack split (data vs code)

| Pack | Role |
|------|------|
| **rigComponent** | Data-only POD components + `GetProperties` — **no systems** |
| **rigSystems** | Update/Draw systems that register on `MEcs` |
| **rigRender3D** | GLES mesh present when an active `CCamera` exists |
| **rigObj** | Wavefront OBJ load/save for `CMesh` (tinyobjloader) |
| **rigPdf** | PDF emit from `CPage` / paths / shaped `CText` (PDF-Writer + ImVarFont core; leaf) |
| **rigStoryComponent** | Story PODs (`CStoryFlow`, paragraph, table, named styles) — DATA ONLY |
| **rigLayoutComponent** | Layout PODs (master, facing, visual styles, frame chain) — DATA ONLY |
| **rigLayout** | Compose engine — story to placed `CText` on pages (Setup bake) |
| **rigEthereum** | On-chain art record POD + keccak256 / wallet submit intent (leaf; existing wallet; no keys) |
| **rigAssimp** | Optional Assimp multi-format to `CMesh` (leaf; app opt-in only) |
| **rigMeshEdit** | ImGuizmo TRS edit for selected `CTransform` |
| **rigNodeComponent** | Generic node-graph PODs (`CNodeGraph`) — DATA + catalog/eval helpers. Artist guide: [nodes.md](nodes.md) |
| **rigNodeEditor** | ImGui editor over `CNodeGraph` ([nodes.md](nodes.md)) |
| **rigProject** | Host project envelope + `.rig` document IO (document = portable `.rig`; project = host session) |
| **rigImGui** | Properties/Debug read the **component catalog** |
| **rigNetScan** | LAN TCP connect scan over `CNetScan` / `CNetHost`; `IpScannerWindow` on rigImGui |

Core (`MEcs`) only provides thin `registerComponent` / `registerSystem` glue.

In-org **basics** (`rigComponent`, `rigSystems`, `rigProject`, `rigImGui`) are git submodules on the host. Optional packs (e.g. `rigOsc`, `rigBlend2D`, plotter family) are cloned locally or fetched via CPM at a pinned **`ref`**. Survey [packs_catalog.md](packs_catalog.md) before scaffolding; then [`templates/rigTemplate`](../templates/rigTemplate/) — see [packs/README.md](../packs/README.md#new-pack).

## Component authority (where types live)

Pick a home before writing systems or UI. Agents: [rigkit-data](../skills/rigkit-data/SKILL.md).

| Home | Owns | Examples |
|------|------|----------|
| Host `src/ecs/components/` | Host-bound leftovers only — do not grow for new portable meaning | `CEvent` (menu/UI action data; `std::any` payload keeps it host-only) |
| **rigComponent** | Generic reusable PODs — grow here; keep thin | `CTransform`, `CCanvas`, `CCamera`, `CLight`, `CPalette`, `CIndexedAtlas`, `CFaceSelection`, `CEdgeSelection`, `CShape`, `CMesh`, `CSpline3d`, `CNurbsSurface`, `CCadBox`, `CCadDimension`, `CDrawStyle`, `CSelection`, `CScreenCast`, `CScreenCastReceive`, `CCastReceiver` |
| Domain data pack | Product-specific PODs (+ codecs / pure helpers over that POD) | `rigProject` (`CProject`, `CPage`); `rigPlotComponent` (`CPaths`, …); `rigNodeComponent` (`CNodeGraph`) |
| Code pack | Systems / I/O / UI — not portable component homes | `rigSystems`, `rigRender3D`, `rigObj`, `rigAssimp` (leaf), `rigPdf` (leaf), `rigEthereum` (leaf), `rigScreenCast` (leaf), `rigMeshEdit`, `rigNodeEditor`, `rigPlotter`, `rigPlotFinders`, `rigSvg`, `rigImGui` |
| App | Prototypes until promotion to `rigComponent` or a domain data pack | one-off app structs |

A data pack is **components-first**, not “literally only `struct` files.” Allowed: `C*` PODs, `GetProperties`, `registerComponent`, document codecs for those types, pure helpers (catalog, eval, flatten). Forbidden: Update/Draw systems, ImGui panels, GPU/window handles in components. Engines (e.g. PlotDoc) and editors stay in **code** packs.

Serialize domain types via **rigProject** codecs / root extensions — the **owning** pack (or app) calls `project::addSerializer<T>(...)` / `rigProject::registerSerializer` in `setup()`. **rigProject** only walks the registry and owns the document envelope (`CProject` / `CPage`). Do not grow a kitchen-sink serializer list inside **rigProject**.

```cpp
#include "AddSerializer.h"
#include "rigProject.h"

// In the owning pack's setup(), after getPack succeeds:
if (auto* doc = getPack<rigkit::rigProject>("rigProject")) {
	rigkit::project::addSerializer<MyComponent>(
		doc->serializer().registry(), "MyComponent", "x.rigkit.my_component",
		serializeMyComponent, deserializeMyComponent);
	// Trivial cases:
	// addBoolMemberSerializer<CSelectable, &CSelectable::enabled>(..., "enabled");
	// addMarkerSerializer<CGroup>(..., "rig.spatial.group");
}
```

## Define a data component

```cpp
// In your app or a data pack — plain struct only
struct MyComponent {
	float brightness = 1.0f;
	std::vector<sProp> GetProperties() {
		return {{0, "Brightness", EPT_FLOAT, &brightness}};
	}
};
```

## Register it (from setup code, not inside the struct)

```cpp
ecs->registerComponent<MyComponent>("MyComponent", true);
```

rigImGui Properties iterates the catalog — no hardcoded type list.

## Register a system (rigSystems lane)

```cpp
// A system is a plain function over the data — pass it straight in.
void MySim(MEcs& ecs, float dt);   // mutate component data
void MyPresent(MEcs& ecs);         // ecs.getPresentRenderer() then present

ecs->registerSystem("MySim", SystemPhase::Update, MySim);
ecs->registerSystem("MyPresent", SystemPhase::Draw, MyPresent);
```

The manager binds itself, so a system takes only what it uses — `void(MEcs&, float)`, `void(MEcs&)`, `void(float)` or `void()`. Never write a lambda whose only job is to forward `MEcs&` back to the manager you registered on.

The name is the entry's identity: same name + phase replaces, so a pack that runs `setup()` again (`MPack::reloadPack`) does not end up running its systems twice.

## Bootstrap order

Register **rigComponent** before **rigSystems** before **rigImGui**. 3D present: register **rigRender3D** after **rigSystems**. `initAll` topo-sorts from each pack's `pack.json` `dependencies` (applied onto `IPack` at register); it does not create missing deps.

**Register then get.** `MPack` owns the `shared_ptr`. Apps that need a typed handle after bootstrap look it up by pack class:

```cpp
packs->registerPack<rigkit::rigProject>();
packs->registerPack<rigkit::rigPlotter>();
packs->registerPack<rigkit::rigImGui>();
packs->registerPack<rigkit::rigSvgEditorUi>(); // or rigPlotterUi
packs->initAll();
packs->setupAll();

m_document = packs->getPack<rigkit::rigProject>();
m_plotter  = packs->getPack<rigkit::rigPlotter>();
```

Fire-and-forget packs stay as `registerPack<T>()` with no member. Plotter/SVG UI packs resolve `rigPlotter` / `rigProject` in their own `setup()` when setters are omitted.

`registerPack<T>(args...)` constructs and owns the instance; the `shared_ptr` overload remains for data-driven registration (PackLoader / manifests). `getPack<T>()` keys on the pack class, so the id lives in one place and a wrong type is a compile error. The name overload `getPack("rigProject")` returns `IPack` and stays for enable/disable, settings, and dependency lookups.

> SUDE bootstrap: `examples/oscHost`. Creators + meshes: `examples/minimal`. CAD CSG: `packs/rigManifold/examples/cad`. Sprite sheet: `packs/rigComponent/examples/pixel`.
