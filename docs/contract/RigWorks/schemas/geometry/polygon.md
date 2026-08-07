# `rig.geometry.polygon`

Freeform polygon with explicit vertices. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `points` | vec2[] | Vertices in order; at least 3 |
| `closed` | bool | Optional. Absent means closed |

Winding is not specified and carries no meaning. A host that needs a fill rule uses its own.

For an n-gon defined by a radius and a side count rather than by vertices, use [`rig.geometry.regular_polygon`](regular-polygon.md) — it survives editing as a parametric shape, where this one has already been resolved to points.

Curved contours use [`rig.geometry.path`](path.md).

Appearance: [`rig.paint.fill_stroke`](../paint/fill-stroke.md).
