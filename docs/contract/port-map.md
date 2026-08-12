# Port map — Rig ↔ RigKit

How this host relates to [Rig](https://github.com/rigkid/RigWorks) schemas.

**Close** = portable fields match (channel packing OK: `rgba` ↔ `colorR/G/B/A`).  
**Partial** = affinity only — do not advertise as speaking that schema id yet.  
**Planned** = no meaningful POD map.

Schemas were cross-pollinated toward this reference host (v0.1). Data packs hold POD; systems/UI stay in code packs.

## Close

| Schema | RigKit | Honesty |
|--------|--------|---------|
| `rig.spatial.relationship` | **rigComponent** — `CRelationship` | Exact `parent`. |
| `rig.spatial.transform` | **rigComponent** — `CTransform` | Serialize `position` / `rotation` (quat) / `scale`. `euler` is editor cache only — sync both ways; `localMatrix` uses quat. Do not serialize `euler` or `world`. |
| `rig.spatial.group` | **rigComponent** — `CGroup` | Marker only (empty). Children use `CRelationship::parent`. |
| `rig.spatial.camera` | **rigComponent** — `CCamera` (present **rigRender3D**) | Matches projection set (`active`, `projection`, clips, FOV, aspect). |
| `rig.spatial.layer` | **rigComponent** — `CLayer` | `order` / `visible` / `locked` / tint (`rgba` ↔ channels). |
| `rig.paint.fill_stroke` | **rigComponent** — `CDrawStyle` | Core fill/stroke; caps/joins/dash are host extensions. |
| `rig.geometry.mesh` | **rigComponent** — `CMesh` | positions / optional normals / indices / texcoords / mode / optional face colours. |
| `rig.geometry.path` | **rigComponent** — `CPath` | AoS `commands` including `quadTo`. Plot layer bags stay on **rigPlotComponent** `CPaths`. |
| `rig.layout.page` | **rigProject** — `CPage` | `index` / `width` / `height` / `unit`; margins/bleed/slug as scalar channels. Entity title via `rig.meta.named`. |
| `rig.spatial.anchor` | **rigProject** — `CPage::originAnchor` | Which cell of the trim is page-local (0,0), as the Contract string enum. Absent = `topLeft`, so a top-left page writes no component. A page anchors to a corner or `center`; an imported edge cell keeps its side and falls to the nearer corner. |
| `rig.pixel.palette` | **rigComponent** — `CPalette` | `colors` (16 rgba). `shadeNext` travels separately as `x.rigkit.palette_shade`. |
| `rig.render.light` | **rigComponent** — `CLight` | Dir/point + colour / intensity / banded shade. Spot not in v0.1. |
| `rig.io.osc` | **rigOsc** — `COscEndpoint` | Listen/send ports + prefix. |
| `rig.pixel.effect_chain` | **rigPixelPlotComponent** — `CPixelEffectChain` | `stage` image/draw/generate + `effectId` / `enabled` + step `id` / `parentStep` / chain `nextId`; params pack-local. `parentStep` is stored and round-tripped; evaluation currently reads the preceding step (flat compose). |
| `rig.node.graph` | **rigNodeComponent** — `CNodeGraph` / `NodeGraphData` | `nodes` / `links` / `nextId`. Also used as `nested` on group nodes. |
| `rig.node.node` | nested in graph | Supports `nested` + `publishes` for nestable groups (`typeId` e.g. `group`). |
| `rig.node.publish` | `NodePublish` on `GraphNode` | `pin` / `innerNode` / `innerPin`. |
| `rig.interact.selectable` | **rigComponent** — `CSelectable` | Exact `enabled`. Selection *state* stays on `CSelection` (host). Pick honors flag when present; absence = legacy selectable. |
| `rig.media.asset_ref` | **rigComponent** — `CAssetRef` | `kind` / `path` / `loop`. Other schemas take an entity ref or compose this type on the same entity. |
| `rig.media.code` | **rigComponent** — `CCode` | `text` / `language` / `readOnly`. Compose `CAssetRef` on the same entity for disk origin. `name` / `order` until `rig.meta.named`. `dirty` / `epoch` are editor cache — do not serialize. No `.rig` serializer: buffers are derived data. |
| `rig.anim.curve` | **rigComponent** — `CCurve` | `points` / `interp` / `preset`. |
| `rig.media.text` | **rigComponent** — `CText` (present **rigVarFont**) | `text` / `font` (`CAssetRef` kind font) / `fontSize` / `rgba` / `axes` / `features` / `useKerning`. Plot bake-to-path stays on `CTexts` (`TextItem` keeps `fontPath`; `sizeMm` / `baselineMm` plot-local). |

## Partial

| Schema | RigKit | Honesty |
|--------|--------|---------|
| `rig.geometry.rectangle` / `ellipse` / … | **rigComponent** — `CShape` | Host still uses a union POD (`type` / `sides` / `innerRadius`); Rig split primitives in 0.5.0 — do not advertise a single schema id yet. |
| `rig.pixel.canvas` / `source` / `layer` / `raster` | **rigPixelPlotComponent** | Intent only — not a finished field map. Compositor `kind=group` + parent exist on `CPixelLayer`. |
| `rig.meta.named` | *(none)* | No `CName`; names on domain PODs. Grow **rigComponent**. |
| `rig.paint.solid` | **rigColorspace** — `CColor` | `rgba` + optional `cmyk`. `model` / `space` are host authoring lanes. |
| `rig.paint.gradient` | **rigPlotComponent** — `CGradient` | Wire `kind` / stops `t`/`rgba`. `interp` / `spread` / `intensity` / `angle`/`center`/radii are host stand-ins for `p0`/`p1`. |
| `rig.paint.library` | **rigPlotComponent** — `CPaintLibrary` / **rigColorspace** — `CSwatchLibrary` | Still index-based gradients/swatches — not entity paint lists yet. |
| `rig.render.material` | — | Grow **rigComponent** + **rigRender3D**. |
| `rig.media.*` (remaining) | — | Settings-only; decode in code packs. |
| `rig.node.pin` / `link` / `param` | nested in `CNodeGraph` | Nested PODs; pin/param `type` = property datatype table. |

## Planned

| Schema family | Planned pack |
|---------------|--------------|
| `rig.music.*` | **rigMusicComponent** (sequencer → pattern → steps) |
| `rig.anim.curve` | **rigComponent** — `CCurve` | Close for curve POD + presets; tween/LFO still planned. |
| `rig.anim.*` / `rig.mod.*` (remaining) | **rigAnimComponent** or grow **rigComponent** |
| `rig.led.*` | **rigLedComponent** |
| `rig.io.serial` / `rig.sensor.*` | **rigInstallIoComponent** (or split) |

## UI

**rigImGui** fulfills **Rig + UI** (seam `IMui`). Show mode detaches it — Rig only. Pick helpers (`EntityPick`) live here; `CSelectable` data lives in **rigComponent**. **rigNodeEditor** dives nestable groups and publishes pins.

## Checklist

RigKit **is Rig**: SUDE + ECS. Author path is **Rig + UI**. See [Rig honors](https://github.com/rigkid/RigWorks/blob/main/docs/honors.md).

When you change a Close-row POD, update the Rig schema and this table in the same change.

Reach for a `rig.*` id first. Do not mirror a Contract component under `x.rigkit.*` to add one field — that splits meaning and no reader will merge the keys. Host-only meaning (beds, tools, plotter session) stays `x.rigkit.*`. Validate new `.rig` files against RigWorks (`rig-validate --strict`); do not spawn Node on interactive save.
