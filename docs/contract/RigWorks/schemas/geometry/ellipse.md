# `rig.geometry.ellipse`

Axis-aligned ellipse. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `cx` | float | Centre X, local space |
| `cy` | float | Centre Y, local space |
| `rx` | float | Radius along X |
| `ry` | float | Radius along Y |

A circle is an ellipse with `rx` equal to `ry`. There is no separate circle schema — a reader that only draws circles should treat unequal radii as an ellipse rather than pick one radius.

Rotation belongs to [`rig.spatial.transform`](../spatial/transform.md).

Appearance: [`rig.paint.fill_stroke`](../paint/fill-stroke.md).
