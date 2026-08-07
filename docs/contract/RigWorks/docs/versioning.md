# Versioning

**Current: 0.9.0** ([VERSION](../VERSION))

| Range | Meaning |
|-------|---------|
| **0.x.y** | Draft. Core rules (SUDE + ECS) hold. Schemas may grow or tighten. |
| **1.0.0** | First stable release. Breaking schema or core-rule changes need a major bump. |

Until 1.0.0, prefer additive schema fields. Tag releases `v0.1.0`, `v0.2.0`, … then `v1.0.0`.

## History

| Version | Notes |
|---------|-------|
| **0.9.0** | Additive. Portable tool surfaces: `rig.ui.panel` gained `role` and advisory `preferredWidth` / `preferredHeight`; new `rig.ui.group` for nested sections; controls and actions may parent to a `group`. Cross-app / cross-lib tool documents are first-class — see [ui.md](ui.md). A 0.8.0 document is a valid 0.9.0 document. |
| **0.8.0** | Additive. The required-field audit: every schema was revisited and genuinely optional fields are now optional with documented defaults, so hosts ship what they measured instead of inventing values ([gaps.md](gaps.md) had carried this since 0.4.0). Named `colorSpace` on the document envelope (default `srgb`). `rig.music.pattern` gained optional `stepsPerBeat`, `loopStartStep`, `loopEndStep`; `rig.music.step.waveform` widened to 0–255 (8+ are host instrument slots); `rig.pixel.tile_map` gained optional `originX` / `originY`; `rig.sim.rigidbody.gravity` became an optional per-body override; MIDI ports may be identified by `portIndex` as well as `portName`. A 0.7.0 document is a valid 0.8.0 document. |
| **0.7.0** | Additive. Completed the shared-paint story: `rig.paint.fill` and `rig.paint.stroke` reference a paint entity from a drawable (the counterpart to inline `rig.paint.fill_stroke`), and `rig.paint.library` records which paint entities are the document's swatches and in what order. Added `rig.layout.page` — pages, artboards, and frames with trim size, margins, bleed, and slug. `rig.media.asset_ref.loop` is now optional — it only means something for time-based media. A 0.6.0 document is a valid 0.7.0 document. |
| **0.6.0** | Additive. Component keys may now be `x.<vendor>.<name>` as well as `rig.<domain>.<name>`. Validators pass extension payloads through unchecked and report them as notes, including under `--strict`. Lets a host adopt `.rig` as its native format without dropping components the Contract has not named — see [`rig.document`](../schemas/document.md#extension-components). A 0.5.0 document is a valid 0.6.0 document. |
| **0.5.0** | **Breaking.** Retired `rig.geometry.shape` — the tagged union made every shape carry every other shape's fields. Replaced by eight primitives: `rectangle`, `ellipse`, `line`, `polygon`, `regular_polygon`, `star`, `arc`, `ring`. Circles are ellipses with equal radii, triangles are three-sided regular polygons, and `star.innerRadius` is now an absolute distance rather than a 0–1 ratio. Added `rig.render.visibility`, `rig.meta.tags`, `rig.music.midi_input`, `rig.audio.analysis`, `rig.io.sacn`, `rig.sim.rigidbody`, `rig.sim.particle_emitter`. Added optional `order` to `rig.spatial.relationship` and optional `lane` to `rig.music.pattern`. |
| **0.4.0** | Renamed the project from Stitch to **RigWorks** (Rig for short), under the [rigkid](https://github.com/rigkid) org. Schema ids are now `rig.<domain>.<name>`; the document root key is `rig`; the `$id` base is `https://rig.works/schemas/`. Clean break — no aliases for the old `stitch.*` ids. |
| **0.3.0** | Last release under the Stitch name. |
