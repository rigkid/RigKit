# Schemas

Agreed POD field layouts. **Formats when present** — ship what you support.

Being Rig does not require implementing this catalog. If two hosts both speak a schema id, they share fields and units.

- **Prose** (this tree) — field meaning and units for humans.
- **JSON Schema** — [`json/`](json/) machine grammar for tools and AI emission.
- **Documents** — [`document.md`](document.md) envelope; examples under [`../examples/`](../examples/); validate with [`../tools/rig-validate/`](../tools/rig-validate/).

## Rules

- Field **meaning and units** are the contract; C++ type names are host-specific.
- One type per field. No dual representations.
- No device handles, GPU objects, UI toolkit types, callbacks, dirty flags, or ephemeral edge state in portable fields.
- Runtime caches and queues (last sample, pending MIDI, hover) stay in the host.
- Ids are `rig.<domain>.<name>` (snake_case multi-word segments). Fields are camelCase. Enum literals are lowerCamelCase.
- Display names live on [`rig.meta.named`](meta/named.md) — compose it; do not re-declare `name` on every schema.
- File identity lives on [`rig.media.asset_ref`](media/asset-ref.md) — other schemas reference it by `entity` id when they need a path.
- `rgba` / `rgb` / `clearRgba` are floats 0–1. Colour space is the envelope's `document.colorSpace` key (default: srgb) — see [document.md](document.md).
- Domain meaning lives in field names — not extra property datatype ids. See [properties.md](../docs/properties.md).

## Catalog

### Document

| Id | Doc |
|----|-----|
| `rig.document` | [document.md](document.md) |

### Spatial / scene

| Id | Doc |
|----|-----|
| `rig.spatial.transform` | [spatial/transform.md](spatial/transform.md) |
| `rig.spatial.relationship` | [spatial/relationship.md](spatial/relationship.md) |
| `rig.spatial.group` | [spatial/group.md](spatial/group.md) |
| `rig.spatial.camera` | [spatial/camera.md](spatial/camera.md) |
| `rig.spatial.layer` | [spatial/layer.md](spatial/layer.md) |
| `rig.layout.page` | [layout/page.md](layout/page.md) |

### Geometry / paint

| Id | Doc |
|----|-----|
| `rig.geometry.mesh` | [geometry/mesh.md](geometry/mesh.md) |
| `rig.geometry.path` | [geometry/path.md](geometry/path.md) |
| `rig.geometry.rectangle` | [geometry/rectangle.md](geometry/rectangle.md) |
| `rig.geometry.ellipse` | [geometry/ellipse.md](geometry/ellipse.md) |
| `rig.geometry.line` | [geometry/line.md](geometry/line.md) |
| `rig.geometry.polygon` | [geometry/polygon.md](geometry/polygon.md) |
| `rig.geometry.regular_polygon` | [geometry/regular-polygon.md](geometry/regular-polygon.md) |
| `rig.geometry.star` | [geometry/star.md](geometry/star.md) |
| `rig.geometry.arc` | [geometry/arc.md](geometry/arc.md) |
| `rig.geometry.ring` | [geometry/ring.md](geometry/ring.md) |
| `rig.paint.solid` | [paint/solid.md](paint/solid.md) |
| `rig.paint.gradient` | [paint/gradient.md](paint/gradient.md) |
| `rig.paint.fill_stroke` | [paint/fill-stroke.md](paint/fill-stroke.md) |
| `rig.paint.fill` | [paint/fill.md](paint/fill.md) |
| `rig.paint.stroke` | [paint/stroke.md](paint/stroke.md) |
| `rig.paint.library` | [paint/library.md](paint/library.md) |

### Meta / render

| Id | Doc |
|----|-----|
| `rig.meta.named` | [meta/named.md](meta/named.md) |
| `rig.meta.tags` | [meta/tags.md](meta/tags.md) |
| `rig.render.light` | [render/light.md](render/light.md) |
| `rig.render.material` | [render/material.md](render/material.md) |
| `rig.render.visibility` | [render/visibility.md](render/visibility.md) |

### Animation / modulators

| Id | Doc |
|----|-----|
| `rig.anim.tween` | [anim/tween.md](anim/tween.md) |
| `rig.mod.lfo` | [mod/lfo.md](mod/lfo.md) |
| `rig.mod.binding` | [mod/binding.md](mod/binding.md) |

### Music

| Id | Doc |
|----|-----|
| `rig.music.transport` | [music/transport.md](music/transport.md) |
| `rig.music.clock` | [music/clock.md](music/clock.md) |
| `rig.music.sequencer` | [music/sequencer.md](music/sequencer.md) |
| `rig.music.pattern` | [music/pattern.md](music/pattern.md) |
| `rig.music.step` | [music/step.md](music/step.md) |
| `rig.music.note` | [music/note.md](music/note.md) |
| `rig.music.midi_output` | [music/midi-output.md](music/midi-output.md) |
| `rig.music.midi_input` | [music/midi-input.md](music/midi-input.md) |

### Audio

| Id | Doc |
|----|-----|
| `rig.audio.analysis` | [audio/analysis.md](audio/analysis.md) |

### Media

| Id | Doc |
|----|-----|
| `rig.media.asset_ref` | [media/asset-ref.md](media/asset-ref.md) |
| `rig.media.text` | [media/text.md](media/text.md) |
| `rig.media.code` | [media/code.md](media/code.md) |

### Pixel / raster

| Id | Doc |
|----|-----|
| `rig.pixel.canvas` | [pixel/canvas.md](pixel/canvas.md) |
| `rig.pixel.source` | [pixel/source.md](pixel/source.md) |
| `rig.pixel.layer` | [pixel/layer.md](pixel/layer.md) |
| `rig.pixel.raster` | [pixel/raster.md](pixel/raster.md) |
| `rig.pixel.palette` | [pixel/palette.md](pixel/palette.md) |
| `rig.pixel.tile_set` | [pixel/tile-set.md](pixel/tile-set.md) |
| `rig.pixel.tile_map` | [pixel/tile-map.md](pixel/tile-map.md) |
| `rig.pixel.effect_chain` | [pixel/effect-chain.md](pixel/effect-chain.md) |

### Install / I/O

| Id | Doc |
|----|-----|
| `rig.io.osc` | [io/osc.md](io/osc.md) |
| `rig.io.serial` | [io/serial.md](io/serial.md) |
| `rig.io.sacn` | [io/sacn.md](io/sacn.md) |
| `rig.led.uv_map` | [led/uv-map.md](led/uv-map.md) |
| `rig.sensor.gpio` | [sensor/gpio.md](sensor/gpio.md) |
| `rig.input.buttons` | [input/buttons.md](input/buttons.md) |

### Simulation

| Id | Doc |
|----|-----|
| `rig.sim.rigidbody` | [sim/rigidbody.md](sim/rigidbody.md) |
| `rig.sim.particle_emitter` | [sim/particle-emitter.md](sim/particle-emitter.md) |

### Interaction

| Id | Doc |
|----|-----|
| `rig.interact.selectable` | [interact/selectable.md](interact/selectable.md) |

### UI (control surfaces)

| Id | Doc |
|----|-----|
| `rig.ui.panel` | [ui/panel.md](ui/panel.md) |
| `rig.ui.group` | [ui/group.md](ui/group.md) |
| `rig.ui.control` | [ui/control.md](ui/control.md) |
| `rig.ui.action` | [ui/action.md](ui/action.md) |

### Node graph

| Id | Doc |
|----|-----|
| `rig.node.graph` | [node/graph.md](node/graph.md) |
| `rig.node.node` | [node/node.md](node/node.md) |
| `rig.node.pin` | [node/pin.md](node/pin.md) |
| `rig.node.link` | [node/link.md](node/link.md) |
| `rig.node.param` | [node/param.md](node/param.md) |
| `rig.node.publish` | [node/publish.md](node/publish.md) |

### Grouping graphs (do not merge)

| Graph | Parent / nest | Schema |
|-------|---------------|--------|
| Scene pose | `parent` | [`spatial.relationship`](spatial/relationship.md) + optional [`spatial.group`](spatial/group.md) marker |
| Compositor stack | `groupParent` | [`pixel.layer`](pixel/layer.md) (`kind=group`) |
| Effect chain | `parentStep` | [`pixel.effect_chain`](pixel/effect-chain.md) (step `id`) |
| Tile map | `tileSet` | [`pixel.tile_map`](pixel/tile-map.md) → [`pixel.tile_set`](pixel/tile-set.md) → [`pixel.palette`](pixel/palette.md) |
| Node editor | `nested` on a node | [`node.node`](node/node.md) + [`node.publish`](node/publish.md) |
| Control surface | `panel` / optional `group` | [`ui.panel`](ui/panel.md) + [`ui.group`](ui/group.md) + [`ui.control`](ui/control.md) / [`ui.action`](ui/action.md) |

Hosts and packs fulfill subsets — see [RigKit port-map](https://github.com/rigkid/RigKit/blob/main/docs/contract/port-map.md).
