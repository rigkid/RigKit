# Interchange

Rig portable documents are **JSON** using the [`rig.document`](../schemas/document.md) envelope. Component object keys are full schema ids (`rig.spatial.transform`). The conventional file extension is **`.rig`**.

## Encoding

| POD type | JSON |
|----------|------|
| `bool` / `int` / `uint` / `float` / `string` | JSON primitives |
| `vec2` / `vec3` / `vec4` / `quat` | Number arrays (`quat` order x, y, z, w) |
| `entity` | String id within the file, or `null` for none |
| Enums | lowerCamelCase string literals from the schema |

Formal grammar: [`schemas/json/`](../schemas/json/). Validate with [`tools/rig-validate`](../tools/rig-validate/).

## Legacy RigKit PascalCase keys

[RigKit](https://github.com/rigkid/RigKit) currently still saves `{ "document", "entities" }` with **PascalCase short keys** (`Transform`, `Shape`, …) — a host alias of the same field payloads, not the Contract key spelling. That alias layer is being retired: Contract JSON uses full `rig.*` schema ids, and `.rig` is the portable format.

| Rig schema id | Typical RigKit key (legacy) |
|---------------|-----------------------------|
| `rig.spatial.transform` | `Transform` |
| `rig.spatial.relationship` | `Relationship` |
| `rig.spatial.group` | `Group` |
| `rig.geometry.rectangle`, `.ellipse`, `.star`, … | `Shape` (tagged union, split in 0.5.0) |
| `rig.geometry.mesh` | `Mesh` |
| `rig.paint.fill_stroke` | `DrawStyle` |
| `rig.interact.selectable` | `Selectable` |
| `rig.spatial.camera` | `Camera` |
| `rig.render.light` | `Light` |

Field names inside those objects should match Rig (e.g. transform `rotation` as quat; do not emit editor Euler). AI and validators should emit / check **Rig schema ids**. Hosts may translate at the boundary. Migrating RigKit off PascalCase keys is a RigKit change, not this Contract.
