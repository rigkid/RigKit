# Port map - Rig / RigKit

How this host relates to [Rig](https://github.com/rigkid/RigWorks) schemas.

**Close** = portable fields match (channel packing OK: `rgba` / `colorR/G/B/A`).  
**Partial** = affinity only - do not advertise as speaking that schema id yet.  
**Planned** = no meaningful POD map.

Schemas were cross-pollinated toward this reference host (v0.1). Data packs hold POD; systems/UI stay in code packs.

Every id in the vendored catalog ([schemas/README.md](RigWorks/schemas/README.md)) is a row here, or sits under a `` `rig.<domain>.*` `` family row. `tools/check-invariants.sh` fails when a catalog id is missing.

## Close

| Schema | RigKit | Notes |
|--------|--------|---------|
| `rig.spatial.relationship` | **rigComponent** - `CRelationship` | Exact `parent`. |
| `rig.spatial.transform` | **rigComponent** - `CTransform` | Serialize `position` / `rotation` (quat) / `scale`. `euler` is editor cache only - sync both ways; `localMatrix` uses quat. Do not serialize `euler` or `world`. |
| `rig.spatial.group` | **rigComponent** - `CGroup` | Marker only (empty). Children use `CRelationship::parent`. |
| `rig.spatial.camera` | **rigComponent** - `CCamera` (present **rigRender3D**) | Matches projection set (`active`, `projection`, clips, FOV, aspect). |
| `rig.spatial.layer` | **rigComponent** - `CLayer` | `order` / `locked` / tint (`rgba`). Hide bit is `rig.render.visibility` when present; otherwise `x.rigkit.layer_visible`. |
| `rig.paint.fill_stroke` | **rigComponent** - `CDrawStyle` | Core fill/stroke; caps/joins/dash are host extensions. |
| `rig.geometry.mesh` | **rigComponent** - `CMesh` | positions / optional normals / indices / optional n-gon `loops`+`loopSizes` / texcoords / mode / optional face colours. Edges are consecutive loop pairs (no parallel edge table). |
| `rig.geometry.spline` | **rigComponent** - `CSpline` | NURBS curve in the plane. |
| `rig.geometry.spline3d` | **rigComponent** - `CSpline3d` | NURBS curve in 3-space. |
| `rig.geometry.nurbs_surface` | **rigComponent** - `CNurbsSurface` | Control net; tessellation is fulfillment. |
| `rig.geometry.path` | **rigComponent** - `CPath` | AoS `commands` including `quad-to`. Plot layer bags stay on **rigPlotComponent** `CPaths`. |
| `rig.geometry.path3d` | **rigComponent** - `CPath3d` | AoS `commands` (`vec3` points). No `ArcTo` - circular arcs stay on `rig.geometry.arc`. |
| `rig.geometry.rectangle` | **rigComponent** - `CRectangle` | `x` / `y` / `width` / `height` / `cornerRadius`. Top-left. |
| `rig.geometry.ellipse` | **rigComponent** - `CEllipse` | `centerX` / `centerY` / `radiusX` / `radiusY`. Members are `cx` / `cy` / `rx` / `ry`. |
| `rig.geometry.line` | **rigComponent** - `CLine` | `x1` / `y1` / `x2` / `y2`. |
| `rig.geometry.regular_polygon` | **rigComponent** - `CRegularPolygon` | `centerX` / `centerY` / `radius` / `sides` / `rotationDegrees`. Members `cx` / `cy`. |
| `rig.geometry.star` | **rigComponent** - `CStar` | `centerX` / `centerY` / `radius` / `innerRadius` / `points` / `rotationDegrees`. Members `cx` / `cy`. |
| `rig.geometry.ring` | **rigComponent** - `CRing` | `centerX` / `centerY` / `outerRadius` / `innerRadius`. Members `cx` / `cy`. |
| `rig.geometry.arc` | **rigComponent** - `CArc` | `centerX` / `centerY` / `radiusX` / `radiusY` / angles / `pie`. Members `cx` / `cy` / `rx` / `ry`. A lone `radius` sets both axes. |
| `rig.layout.page` | **rigProject** - `CPage` | `index` / `width` / `height` / `unit`; margins/bleed/slug as scalar channels. Entity title via `rig.meta.named`. |
| `rig.layout.master` | **rigLayoutComponent** - `CLayoutMaster` | Optional `side` `left` / `right` / `single`. |
| `rig.layout.applied_master` | **rigLayoutComponent** - `CLayoutAppliedMaster` | `master` entity. |
| `rig.layout.facing` | **rigLayoutComponent** - `CLayoutFacing` | `enabled` / `binding`. Absent = singles. |
| `rig.layout.paragraph_style` | **rigLayoutComponent** - `CLayoutParagraphStyle` | Visual map for a story paragraph style (`storyStyle`, font, size, leading, ...). |
| `rig.layout.character_style` | **rigLayoutComponent** - `CLayoutCharacterStyle` | Visual map for a story character style. |
| `rig.layout.frame_chain` | **rigLayoutComponent** - `CLayoutFrameChain` | `story` / `frames` / optional `master`. |
| `rig.layout.section` | **rigLayoutComponent** - `CLayoutSection` | `name` / `numberingStyle` / `startAt` / `prefix` / `startSide`. Enums packed as ints. |
| `rig.layout.table_style` | **rigLayoutComponent** - `CLayoutTableStyle` | Name, border, cell type, paint-entity fills and inks. |
| `rig.story.flow` | **rigStoryComponent** - `CStoryFlow` | Ordered `blocks` of paragraphs / tables. |
| `rig.story.paragraph` | **rigStoryComponent** - `CStoryParagraph` | Style + runs (`text` + optional character style). |
| `rig.story.paragraph_style` | **rigStoryComponent** - `CStoryParagraphStyle` | Identity only (`basedOn`, `listKind`). |
| `rig.story.character_style` | **rigStoryComponent** - `CStoryCharacterStyle` | Identity only (`basedOn`). |
| `rig.story.table` | **rigStoryComponent** - `CStoryTable` | `columnCount` / cells with spans + nested blocks. |
| `rig.spatial.anchor` | **rigProject** - `CPage::originAnchor` | Which cell of the trim is page-local (0,0), as the Contract 3×3 string enum. Absent = `top-left`, so a top-left page writes no component. |
| `rig.pixel.palette` | **rigComponent** - `CPalette` | `colors` (16 rgba). `shadeNext` travels separately as `x.rigkit.palette_shade`. |
| `rig.render.light` | **rigComponent** - `CLight` | Dir/point + colour / intensity / banded shade. Spot not in v0.1. |
| `rig.io.osc` | **rigOsc** - `COscEndpoint` | Listen/send ports + prefix. |
| `rig.io.serial` | **rigSerial** - `CSerial` | `port` / `baud` / `enabled`. Byte buffers and handles stay in the pack. |
| `rig.pixel.effect_chain` | **rigPixelPlotComponent** - `CPixelEffectChain` | `stage` image/draw/generate + `effectId` / `enabled` + step `id` / `parentStep` / chain `nextId`. Step `blendMode` / `opacity` / `options` travel as `x.rigkit.pixel_effect_step`. |
| `rig.pixel.canvas` | **rigPixelPlotComponent** - `CPixelCanvas` | `width` / `height` / `clearRgba` only. Title is `rig.meta.named` on the scene. `exportWithTimestamp` / `preferGpu` on `x.rigkit.pixel_canvas_meta`. |
| `rig.pixel.source` | **rigPixelPlotComponent** - `CPixelSource` | Contract `kind` + `asset` / `generatorName` / sequence / webcam / `videoTime`. Label is `rig.meta.named`. Loop extras + `cast-receive` stay on `x.rigkit.pixel_source_ext`. |
| `rig.pixel.layer` | **rigPixelPlotComponent** - `CPixelLayer` | `kind` / `rgba` / `maskSource` / entity refs (`image` / `maskAsset` / `maskLayer` / `maskPathEntity` / `groupParent`). No `name` / `visible` / `blendMode` / `opacity` / `index` on this object — those compose `named` + `spatial.layer` + `render.visibility` + `render.blend`. Plot guard is `x.rigkit.pixel_layer_ext.protected`. |
| `rig.meta.named` | **rigComponent** - `CName` + `MEcs::entityName` | Display name / `stableId`. `ProjectSerializer` writes this from `entityName` — do not register a second codec. |
| `rig.render.visibility` | **rigComponent** - `CRenderVisibility` | `visible`. Absent = shown. |
| `rig.render.blend` | **rigComponent** - `CRenderBlend` | Compositing-1 tokens + `add` / `subtract` / `disabled`, plus `opacity`. Defaults omitted. |
| `rig.ui.panel` | **rigComponent** - `CUiPanel` | Portable tool surface. ImGui docks are fulfillment. |
| `rig.ui.group` | **rigComponent** - `CUiGroup` | Group inside a panel. |
| `rig.ui.control` | **rigComponent** - `CUiControl` | Control bound to a property. PixelPlotter emits `pixel.source` / `pixel.layers` / `pixel.canvas`. |
| `rig.node.graph` | **rigNodeComponent** - `CNodeGraph` / `NodeGraphData` | `nodes` / `links` / `nextId`. Also used as `nested` on group nodes. |
| `rig.node.node` | nested in graph | Supports `nested` + `publishes` for nestable groups (`typeId` e.g. `group`). |
| `rig.node.publish` | `NodePublish` on `GraphNode` | `pin` / `innerNode` / `innerPin`. |
| `rig.interact.selectable` | **rigComponent** - `CSelectable` | Exact `enabled`. Selection *state* stays on `CSelection` (host). Pick honors flag when present; absence = legacy selectable. |
| `rig.media.asset_ref` | **rigComponent** - `CAssetRef` | `kind` / `path` / `loop`. Other schemas take an entity ref or compose this type on the same entity. |
| `rig.media.code` | **rigComponent** - `CCode` | `text` / `language` / `readOnly`. Compose `CAssetRef` on the same entity for disk origin. `name` / `order` until `rig.meta.named`. `dirty` / `epoch` are editor cache - do not serialize. No `.rig` serializer: buffers are derived data. |
| `rig.anim.curve` | **rigComponent** - `CCurve` | `points` / `interp` / `preset`. |
| `rig.anim.tween` | **rigAnimComponent** - `CTween` | `target` / `propertyKey` / `from` / `to` / `duration` / `elapsed` / `easing` / `loop` / `playing`. Advance is **rigSystems** `STweens`. |
| `rig.mod.lfo` | **rigAnimComponent** - `CModLfo` | `waveform` / `frequency` / `amplitude` / `offset` / `phase`. Advance is **rigSystems** `SModulators`. |
| `rig.mod.binding` | **rigAnimComponent** - `CModBinding` | `source` / `target` / `propertyKey` / `depth` / `additive` / optional `min` / `max`. |
| `rig.music.clock` | **rigMusicComponent** - `CMusicClock` | `ticksPerQuarter` / `phaseTicks` / swing / `externalSync` + sync fields. Writers: **rigAudio**, **rigAbletonLink**. |
| `rig.music.transport` | **rigMusicComponent** - `CMusicTransport` | `playing` / `bpm` / time signature / `positionBeats` / loop window. |
| `rig.visualizer.deck` | **rigShowComponent** - `CVisualizer` | Playlist + `switchOn` + `output`. Present is **rigMilk**. |
| `rig.audio.analysis` | **rigShowComponent** - `CAudioAnalysis` | Authored request (`source` / `bandCount` / `features`). Live values stay on the bus. |
| `rig.picture.analysis` | **rigShowComponent** - `CPictureAnalysis` | Authored request pointed at `source`. |
| `rig.media.text` | **rigComponent** - `CText` (present **rigVarFont**) | `text` / `font` (`CAssetRef` kind font) / `fontSize` / `rgba` / `axes` / `features` / `useKerning`. Plot bake-to-path stays on `CTexts` (`TextItem` keeps `fontPath`; `sizeMm` / `baselineMm` plot-local). |
| `rig.sim.rigidbody` | **rigSimComponent** - `CRigidbody` | `velocity` / `mass` / `drag` / `gravity`. `hasGravity` false omits `gravity` (scene ambient). |
| `rig.print.material` | **rigPrintComponent** - `CPrintMaterial` | `densityKgM3` and optional barrel zones / grade MVS. |
| `rig.print.process` | **rigPrintComponent** - `CPrintProcess` | Layer height, line width, speeds, `adaptiveLayers`. |

## Partial

| Schema | RigKit | Notes |
|--------|--------|---------|
| `rig.document` | **rigProject** import | Requires `rig`. Keeps `document.title` and `document.language`. Other envelope keys are not stored. |
| `rig.geometry.polygon` | **rigComponent** - `CPolygon` | `points` / `closed`. `fillRule` is not stored. |
| `rig.layout.frame` | **rigLayoutComponent** - `CTextFrame` | Per-edge insets, `columns`, `gutter` on a rectangle. Schema wants `width` / `height`, inset channels, `firstBaseline`. `overset` is host state. |
| `rig.pixel.raster` | **rigPixelPlotComponent** - working / output rasters | Size only (`x.rigkit.pixel_*_raster`). Contract `rgba[]` is not written - derived from source + chain. Do not mark Close. |
| `rig.paint.solid` | **rigColorspace** - `CColor` | `rgba` + optional `cmyk` + optional `ink` + optional overprint flags. `model` / `space` are host authoring lanes. |
| `rig.paint.fill` | **rigColorspace** - `CPaintFill` | Entity paint ref. |
| `rig.paint.stroke` | **rigColorspace** - `CPaintStroke` | Entity paint ref; `width` on the stroke POD. |
| `rig.paint.gradient` | **rigPlotComponent** - `CGradient` | Wire `kind` / stops `t`/`rgba`. `interp` / `spread` / `intensity` / `angle`/`center`/radii are host stand-ins for `p0`/`p1`. |
| `rig.paint.library` | **rigPlotComponent** - `CPaintLibrary` / **rigColorspace** - `CSwatchLibrary` | Still index-based gradients/swatches - not entity paint lists yet. |
| `rig.render.material` | **rigComponent** `CRenderMaterial`; shaded by **rigRender3D** | Import writes albedo, metal, roughness, emissive, albedo-map entity. No schema writer. |
| `rig.render.environment` | **rigComponent** `CRenderEnvironment`; shaded by **rigRender3D** | `hdri` is a `rig.media.asset_ref` entity (lat-long radiance). `intensity` scales; absent is 1. Decode stays in the present pack. |
| `rig.media.*` | - | Remaining media ids. Settings-only; decode in code packs. |
| `rig.ui.action` | **rigProject** import list | `panel` / `group` / `order` / `actionId` / `enabled` collected on import. Not an ECS component. |
| `rig.interact.selected` | **rigComponent** - `CSelection` | Live `selectionIndex`. The document snapshot (`order`) is not read or written. |
| `rig.node.pin` | nested in `CNodeGraph` | `type` is a property datatype id. |
| `rig.node.link` | nested in `CNodeGraph` | Link between pins. |
| `rig.node.param` | nested in `CNodeGraph` | `type` is a property datatype id. |
| `rig.cad.cuboid` | **rigCadComponent** - `CCadBox`; bake **rigManifold** | `sizeX` / `sizeY` / `sizeZ` / `center`. Import recognizes the id and does not write the POD. Pi: bake in Setup. |
| `rig.cad.cylinder` | **rigCadComponent** - `CCadCylinder`; bake **rigManifold** | `radius` / `height` / `circularSegments` / `center`. Import does not write the POD. |
| `rig.cad.sphere` | **rigCadComponent** - `CCadSphere`; bake **rigManifold** | `radius` / `circularSegments`. Import does not write the POD. |
| `rig.cad.boolean` | **rigCadComponent** - `CCadBoolean`; bake **rigManifold** | `op` + entity-name `operands`. Nested booleans recurse. Operand local TRS applies before the op. |
| `rig.cad.fillet` | **rigCadComponent** - `CCadFillet` | Authored intent (`radius`, `{a,b}` edges or `allEdges`). Manifold is a triangle kernel - not evaluated as B-rep rounds. |
| `rig.cad.chamfer` | **rigCadComponent** - `CCadChamfer` | Authored intent (`distance`, edges or `allEdges`). Not evaluated as a B-rep chamfer. |
| `rig.cad.extrude` | **rigCadComponent** - `CCadExtrude` | Profile entity + sweep. No kernel eval yet. |
| `rig.cad.revolve` | **rigCadComponent** - `CCadRevolve` | Profile entity + sweep. No kernel eval yet. |
| `rig.cad.dimension` | **rigCadComponent** - `CCadDimension`; drive **rigSolveSpace** | `kind` / `a` / `b` / `value` / `measurement` / `offset`. Driving (`measurement` false) moves `b`'s `CTransform`. Measurement is label-only. Diameter / angle stored, not solved. Solve on edit, not every frame. |
| `rig.cad.reference_line` | **rigCadComponent** - `CCadReferenceLine`; draw **rigCad** | Datum, not a solid. Coordinates place it; `a` / `b` anchor to entity names and win. |
| `rig.cad.reference_plane` | **rigCadComponent** - `CCadReferencePlane`; draw **rigCad** | Datum, not a solid. Presented as line-mode `CMesh`, so `OrbitNav` framing skips it. Rebuilt on edit, not every frame. |
| `rig.print.machine` | **rigPrintComponent** - `CPrintMachine` | Bed and pellet fields match. Barrel zones also sit on this POD; the schema puts them on `rig.print.material`. |
| `rig.print.compression` | **rigPrintComponent** - `CPrintCompression` | `enabled` / `sagMmPerKg` / `alsoScaleE`. Host also stores `densityKgM3` (0 inherits material). |
| `rig.print.job` | **rigPrintComponent** - `CPrintJob` | Host stores paths and last-run status. Schema wants `input` / `output` asset entities. |

## Planned

| Schema family | Planned pack |
|---------------|--------------|
| `rig.font.*` | **rigFontComponent** (UFO source PODs; local `packs/` until published). Chrome bind is host `x.rigkit.ui_face`, not a `rig.*` id. IO in **rigUfo**. Shape / reconstruct in **rigFont**. Host example `ufomat`. |
| `rig.music.*` (sequencer / pattern / step) | **rigMusicComponent** - clock / transport are Close; grow this pack |
| `rig.anim.*` / `rig.mod.*` (remaining) | **rigAnimComponent** - tween / LFO / binding are Close; grow this pack |
| `rig.led.*` | **rigLedComponent** |
| `rig.io.dmx` | **rigDmx** | Universe / direction / enabled. 512-slot buffer stays in the pack. No sockets, no USB. Transports (`rigSacn`, `rigArtnet`, `rigEnttec`) read this buffer. |
| `rig.io.sacn` | **rigSacn** | E1.31 endpoint. Depends on **rigDmx**. Pack exists; schema still Planned until POD fields Close. |
| `rig.io.artnet` | **rigArtnet** | Art-Net endpoint. No schema yet; add when the pack is real. Depends on **rigDmx**. |
| `rig.sensor.*` | - | No owning pack yet. Do not fold into serial. |
| `rig.book.*` | Document metadata - not layout; no owning pack yet |
| LAN host / TCP scan (no `rig.*` id yet) | **rigNetScan** - `CNetScan` / `CNetHost` as `x.rigkit.net_scan` / `x.rigkit.net_host` |
| `rig.pixel.region` | **rigPixelPlotComponent** - `CPixelRegion` as `x.rigkit.pixel_region` | Local proposal: fit / fill / brush / color. |
| `rig.pixel.mask_path` | **rigPixelPlotComponent** - `CPixelMaskPath` as `x.rigkit.pixel_mask_path` | Local proposal; polygon may later compose `rig.geometry.path`. |
| `rig.pixel.composite` | **rigPixelPlotComponent** - `CPixelComposite` as `x.rigkit.pixel_composite` | Local proposal: enabled / stack policy. |
| `rig.pixel.map_output` | **rigPixelPlotComponent** - `CPixelMapOutput` as `x.rigkit.pixel_map_output` | Local proposal: projector frame size / enabled. |
| `rig.pixel.video_texture` | **rigPixelPlotComponent** - `CPixelVideoTexture` as `x.rigkit.pixel_video_texture` | Local proposal: one video placed on the projector frame. |
| `rig.pixel.source` `cast-receive` | **rigPixelPlotComponent** - `x.rigkit.pixel_source_ext` | Local proposal to add the kind (or compose `rig.media.cast`). |
| `rig.pixel.raster` `derived` / `rebake` | working / output size-only extras | Local proposal so size-only documents validate without fake `rgba[]`. |
| `rig.spatial.vertex` | No marker component. Pose is `rig.spatial.transform`. An unparented vertex is the scene root. |
| `rig.paint.stroke_style` | Caps, joins, dash. No POD yet. |
| `rig.paint.brush` | Physical tip measurements. No POD yet. |
| `rig.meta.tags` | `tags` string list. No POD yet. |
| `rig.audio.bus` | `gain` / `mute`. No POD yet. |
| `rig.pixel.tile_set` | No POD yet. |
| `rig.pixel.tile_map` | No POD yet. |
| `rig.node.bus` | Named rail inside a graph. Not stored on `CNodeGraph`. |
| `rig.dmx.fixture` | Fixture patch. The universe buffer is `rig.io.dmx`. |
| `rig.input.*` | Buttons, axes, pointer, binding. No POD yet. |
| `rig.audio.voice` | One playing asset. No POD yet. |
| `rig.sim.collider` | Contact shape. No POD yet. `CSimShape` is the host solver shape. |
| `rig.sim.joint` | No POD yet. |
| `rig.sim.character` | No POD yet. |
| `rig.sim.navmesh` | No POD yet. |
| `rig.sim.agent` | No POD yet. |
| `rig.play.*` | Health, item, inventory, spawner, save. No owning pack. |
| `rig.ai.*` | Behavior tree. No owning pack. |
| `rig.net.*` | Replica identity. No owning pack. |
| `rig.light.look` | No POD yet. |
| `rig.sim.particle_emitter` | No POD yet. `rig.sim.rigidbody` is Close. |
| `rig.calendar.*` | Weekly, span, event, recurrence. No owning pack. |
| `rig.dev.*` | Machine / op. No owning pack. |
| `rig.place.*` | Address / geo. No owning pack. |
| `rig.person.*` | Name, vital, contact, employment, portrait. No owning pack. |
| `rig.organisation.*` | Identity. No owning pack. |
| `rig.party.*` | Account. No owning pack. |
| `rig.commerce.*` | Price, offer, discount. No owning pack. |
| `rig.plant.*` | Taxon through portrait. No owning pack. |
| `rig.paper.*` | Identifier, article, issue, citation. No owning pack. |
| `rig.rights.*` | Statement. No owning pack. |
| `rig.legal.*` | Agreement / party. No owning pack. |
| `rig.art.*` | Object record. No owning pack. |
| `rig.bim.*` | Classify through facet. No owning pack. |
| `rig.circuit.*` | Part, pin, net. No owning pack. |
| `rig.pcb.*` | Board through keepout. No owning pack. |
| `rig.record.*` | Mark, assertion, event, consent, case, incident, concerns, note. No owning pack. |
| `rig.finance.*` | Payment, position, claim. No owning pack. |
| `rig.clinic.*` | Encounter, observation, study, trial, criterion. No owning pack. |
| `rig.law.*` | Statute or regulation. No owning pack. `rig.legal.*` is the agreement. |
| `rig.vr.*` | Space, headset, eye, tracker, controller, pose, hand, joint, haptic, anchor. No owning pack. |
| `rig.render.sprite` | Textured quad. No POD yet. |
| `rig.sim.actuator` | No POD yet. |
| `rig.sim.episode` | No POD yet. |
| `rig.sim.world` | No POD yet. |
| `rig.sim.articulation` | No POD yet. |
| `rig.sim.domain` | No POD yet. |

## UI

**rigImGui** fulfills **Rig + UI** (seam `IMui`). Show mode detaches it - Rig only. Pick helpers (`EntityPick`) live here; `CSelectable` data lives in **rigComponent**. **rigNodeEditor** dives nestable groups and publishes pins.

## Checklist

RigKit **is Rig**: SUDE + ECS. Author path is **Rig + UI**. See [Rig honors](https://github.com/rigkid/RigWorks/blob/main/docs/honors.md).

When you change a Close-row POD, update the Rig schema and this table in the same change.

Reach for a `rig.*` id first. Do not mirror a Contract component under `x.rigkit.*` to add one field - that splits meaning and no reader will merge the keys. Host-only meaning (beds, tools, plotter session) stays `x.rigkit.*`. Validate new `.rig` files against RigWorks (`rig-validate --strict`); do not spawn Node on interactive save.
