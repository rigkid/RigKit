# `rig.node.param`

Inline property row on a [`rig.node.node`](node.md).

Same datatype vocabulary as [properties](../../docs/properties.md). One active storage field per `type`.

| Field | Type | Meaning |
|-------|------|---------|
| `key` | string | Param name / path key |
| `type` | string | Property datatype id (`float`, `vec2`, `vec4`, `enum`, …) |
| `f` | float | When `type` is `float` or `double` |
| `i` | int | When `type` is `bool`, `int`, `uint`, `enum`, or `entity` |
| `s` | string | When `type` is `string` |
| `v` | vec4 | When `type` is `vec2`, `vec3`, `vec4`, or `quat` (unused lanes 0; quat order x, y, z, w) |

Serialize `key`, `type`, and **only** the storage field that `type` selects. Omit the others.

`enum` literals are catalog / host-documented for that node `typeId` + param `key` (same idea as schema enum fields).

Default `type` when omitted on read: `float` (legacy float-only params).
