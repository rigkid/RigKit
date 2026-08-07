# `rig.geometry.regular_polygon`

Equilateral n-gon about a centre. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `cx` | float | Centre X, local space |
| `cy` | float | Centre Y, local space |
| `radius` | float | Centre to vertex, not centre to edge |
| `sides` | int | Vertex count; at least 3 |
| `rotationDegrees` | float | Optional. Absent = 0; first vertex at angle 0 points along +X |

A triangle is `sides: 3`. There is no separate triangle schema.

`rotationDegrees` exists so the shape's own phase survives independently of the entity's rotation in [`rig.spatial.transform`](../spatial/transform.md). A host that has only one of the two composes them.

Appearance: [`rig.paint.fill_stroke`](../paint/fill-stroke.md).
