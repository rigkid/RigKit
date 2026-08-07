# Packs catalog

Known and planned RigKit packs. Operational detail (naming, pinning, heroes, CI, scaffold): [`packs/README.md`](../packs/README.md). How to create/use a pack: [packs.md](packs.md), [packs_using.md](packs_using.md).

**One id.** Spoken name = folder under `packs/` = `pack.json` `"name"` = `app.json` dependency `"name"` = CMake target.

Survey this list before scaffolding a new pack. Prefer growing an existing seam.

## Known

In-org packs are separate remotes in the [rigkid](https://github.com/rigkid) org. **Host basics** (submodules): `rigComponent`, `rigSystems`, `rigProject`, `rigImGui`. Other packs: local clone or CPM at a pinned `ref`.

### Core / distribution (host basics)

| Pack | Remote | Role |
|------|--------|------|
| **rigComponent** | [rigkid/rigComponent](https://github.com/rigkid/rigComponent) | Data-only ECS components — no systems (includes general `CPath` / `rig.geometry.path`) |
| **rigSystems** | [rigkid/rigSystems](https://github.com/rigkid/rigSystems) | Update/Draw systems over that data |
| **rigProject** | [rigkid/rigProject](https://github.com/rigkid/rigProject) | Host project envelope (`CProject`/`CPage`) + `.rig` document IO (document = portable `.rig`) |
| **rigImGui** | [rigkid/rigImGui](https://github.com/rigkid/rigImGui) | UI fulfillment (`IMui`); host shell; Properties catalog; themes/fonts |

### 3D / mesh

| Pack | Remote | Role |
|------|--------|------|
| **rigRender3D** | [rigkid/rigRender3D](https://github.com/rigkid/rigRender3D) | GLES mesh present over `CCamera` + `CMesh` |
| **rigObj** | [rigkid/rigObj](https://github.com/rigkid/rigObj) | Wavefront OBJ ↔ `CMesh` (tinyobjloader) |
| **rigAssimp** | [rigkid/rigAssimp](https://github.com/rigkid/rigAssimp) | Optional Assimp multi-format → `CMesh` (**leaf** — nothing depends on it; not Pi-default) |
| **rigMeshEdit** | [rigkid/rigMeshEdit](https://github.com/rigkid/rigMeshEdit) | ImGuizmo TRS, face pick/paint, extrude; OBJ + `.rig` scene |

### Nodes

| Pack | Remote | Role |
|------|--------|------|
| **rigNodeComponent** | [rigkid/rigNodeComponent](https://github.com/rigkid/rigNodeComponent) | Node-graph PODs (`CNodeGraph`) + catalog/eval helpers — data pack |
| **rigNodeEditor** | [rigkid/rigNodeEditor](https://github.com/rigkid/rigNodeEditor) | ImGui node editor over `CNodeGraph` |

Artist guide: [nodes.md](nodes.md).

### Plot / vector / machine

| Pack | Remote | Role |
|------|--------|------|
| **rigPlotComponent** | [rigkid/rigPlotComponent](https://github.com/rigkid/rigPlotComponent) | Plotter PODs (`CPaths` bags, layers, zones, pen, path-edit selection; commands are `CPath` from **rigComponent**) |
| **rigColorspace** | [rigkid/rigColorspace](https://github.com/rigkid/rigColorspace) | RGB + CMYK color POD, swatch library, conversion helpers |
| **rigCompositor** | [rigkid/rigCompositor](https://github.com/rigkid/rigCompositor) | FBO layer compositor — SVG blend modes + path masks |
| **rigSvg** | [rigkid/rigSvg](https://github.com/rigkid/rigSvg) | SVG import/export ↔ `ParsedPathDoc` / paints |
| **rigDxf** | [rigkid/rigDxf](https://github.com/rigkid/rigDxf) | DXF LINE to `ParsedPathDoc` / `CPaths` |
| **rigGCode** | [rigkid/rigGCode](https://github.com/rigkid/rigGCode) | `CPaths` ↔ G-code text emit/import (no serial) |
| **rigGrbl** | [rigkid/rigGrbl](https://github.com/rigkid/rigGrbl) | GRBL serial send |
| **rigPlotter** | [rigkid/rigPlotter](https://github.com/rigkid/rigPlotter) | PlotDoc orchestration |
| **rigPlotterUi** | [rigkid/rigPlotterUi](https://github.com/rigkid/rigPlotterUi) | Plotter Kit panels on rigImGui |
| **rigSvgEditorUi** | [rigkid/rigSvgEditorUi](https://github.com/rigkid/rigSvgEditorUi) | SVG editor shell (toolbar / artboard / layers) |
| **rigPlotFinders** | [rigkid/rigPlotFinders](https://github.com/rigkid/rigPlotFinders) | Image → stroke finders (hatch / grid / Potrace; more finder types reserved in POD). **GPL-2.0-or-later** — vendors Potrace |
| **rigPlotProcessors** | [rigkid/rigPlotProcessors](https://github.com/rigkid/rigPlotProcessors) | Prepare pipeline (merge / sort / simplify) |
| **rigPlotGenerators** | [rigkid/rigPlotGenerators](https://github.com/rigkid/rigPlotGenerators) | Path generators (cropmarks / border) |
| **rigVectorEditor** | [rigkid/rigVectorEditor](https://github.com/rigkid/rigVectorEditor) | Edit `CPaths` (translate / delete / nudge) |

Integration app: `packs/rigPlotter/examples/plot`. SVG IO pack hero: `packs/rigSvg/examples/svg`.

### Pixel plotter

| Pack | Remote | Role |
|------|--------|------|
| **rigPixelPlotComponent** | [rigkid/rigPixelPlotComponent](https://github.com/rigkid/rigPixelPlotComponent) | PixelPlotter PODs (source, canvas, effect chain, rasters). DATA ONLY |
| **rigPixelPlotter** | [rigkid/rigPixelPlotter](https://github.com/rigkid/rigPixelPlotter) | PixelDoc pipeline (IMAGE/DRAW, GPU, layers, PNG) |
| **rigPixelPlotterUi** | [rigkid/rigPixelPlotterUi](https://github.com/rigkid/rigPixelPlotterUi) | Kit UI panels (Main View, Canvas, Resources, Layers) |

Product app **PixelPlotter** lives out of tree (e.g. next to RigKit), not under `examples/`. See [apps.md](apps.md). Thin host — packs own pipeline + UI.

### Document hosts (Viewer / Player)

| Pack | Remote | Role |
|------|--------|------|
| **rigDocumentShell** | [rigkid/rigDocumentShell](https://github.com/rigkid/rigDocumentShell) (local `packs/` until published) | Document-host chrome (Viewer/Player): File Open `.rig`, skipped keys, Edit Mode defaults. Not rigProject load; not present/play. |

### Install / show

| Pack | Remote | Role |
|------|--------|------|
| **rigOsc** | [rigkid/rigOsc](https://github.com/rigkid/rigOsc) | UDP OSC + network identity / show bus — `oscHost --smoke-osc` |

### Scripting / CAD

| Pack | Remote | Role |
|------|--------|------|
| **rigCodeEditor** | [rigkid/rigCodeEditor](https://github.com/rigkid/rigCodeEditor) | Shared ImGui code editor — TextEditorPanel chrome + JetBrains Mono (opt-in for author tools; not for lean Pi installs) |
| **rigAcp** | [rigkid/rigAcp](https://github.com/rigkid/rigAcp) | [ACP](https://agentclientprotocol.com) client — stdio JSON-RPC + `CCode` fs bridge (opt-in; no UI) |
| **rigAgentty** | [rigkid/rigAgentty](https://github.com/rigkid/rigAgentty) | [agentty](https://agentty.org/docs/) over **rigAcp** — ImGui panel + Code Editor sync (opt-in; submodule ExternalProject or PATH binary) |
| **rigPython** | [rigkid/rigPython](https://github.com/rigkid/rigPython) | Embedded Python host for artist sketches (`eval` when CPython embed found) |
| **rigManifold** | [rigkid/rigManifold](https://github.com/rigkid/rigManifold) | Manifold CSG → `CMesh` (FetchContent kernel; opt-in / desktop-first) |
| **rigCad** | [rigkid/rigCad](https://github.com/rigkid/rigCad) | Live-scripted 3D CAD (`rigcad` sketch module when Python + Manifold available) |

### Optional renderer

| Pack | Remote | Role |
|------|--------|------|
| **rigBlend2D** | [rigkid/rigBlend2D](https://github.com/rigkid/rigBlend2D) | Blend2D `IRenderer` fulfillment (in-org / CPM; `blend2d` + `asmjit` as submodules) |

```bash
git clone --recurse-submodules https://github.com/rigkid/rigBlend2D.git packs/rigBlend2D
```

### Fonts

| Pack | Remote | Role |
|------|--------|------|
| **rigVarFont** | [rigkid/rigVarFont](https://github.com/rigkid/rigVarFont) | Optional VF / FreeType fulfillment of `CText` + `IRenderer` filled text (ImVarFont `varfont_core` + `varfont_gl`). Axis values on POD; Face/atlas in the pack. GLES2 → CPU FreeType fallback. |

`CText` lives in **rigComponent** and speaks `rig.media.text` (`font` → `CAssetRef` kind font; `axes` / `features` / `useKerning` on the schema).

## Planned

Not remotes yet. Scaffold only when the seam is real — survey Known first; prefer growing an existing pack.

| Idea | Notes |
|------|--------|
| **JSON UI sections** | Portable panel map (layout + widget ids + bindings to ECS / `sProp` paths) consumed by `IMui` fulfillments. Host windows stay in **rigImGui**; domain chrome in `*Ui` / `*Editor`. No per-window `rigWin*` packs. Share a tool = ship data (entity meaning + UI JSON), not C++ panels. |
| **Web UI fulfillment** | Alternate `IMui` over the same JSON / ECS map (browser panels) — Contract allows it; no pack yet |
| **Node anim / channels** | Broader timeline/channel products — grow **rigNodeComponent** / **rigNodeEditor** (or a thin companion) when product need is clear |
| **rigMusicComponent** | Transport / clock / sequencer / pattern / step PODs — [Rig music schemas](https://github.com/rigkid/RigWorks/tree/main/schemas/music); data only |
| **rigAnimComponent** | Tween / LFO / binding PODs — Rig `rig.anim.*` / `rig.mod.*` |
| **rigLedComponent** | UV map / LED sample PODs — Pi install path |
| **rigInstallIoComponent** | Serial / GPIO / network device PODs (sACN later) — or split when product needs clear |
| **Blend2D demos** | Examples / heroes beyond the **rigBlend2D** pack itself — not a new pack id |

RigWorks schema catalog: [rigkid/RigWorks](https://github.com/rigkid/RigWorks). Pack map: [docs/contract/port-map.md](contract/port-map.md).

## Related

- Naming and bootstrap: [`packs/README.md`](../packs/README.md)
- Create / use: [packs.md](packs.md), [packs_using.md](packs_using.md)
- Template: [`templates/rigTemplate`](../templates/rigTemplate/)
