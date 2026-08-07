# `rig.geometry.ring`

Annulus — a disc with a concentric hole. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `cx` | float | Centre X, local space |
| `cy` | float | Centre Y, local space |
| `outerRadius` | float | Outer edge |
| `innerRadius` | float | Inner edge; must not exceed `outerRadius` |

Distinct from a stroked [`rig.geometry.ellipse`](ellipse.md): a ring's hole is part of its geometry, so fills and hit tests exclude it. A stroked circle has no hole, only an unpainted middle.

Appearance: [`rig.paint.fill_stroke`](../paint/fill-stroke.md).
