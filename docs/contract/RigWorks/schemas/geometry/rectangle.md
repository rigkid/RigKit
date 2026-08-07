# `rig.geometry.rectangle`

Axis-aligned rectangle. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `x` | float | Left edge, local space |
| `y` | float | Top edge, local space |
| `width` | float | Extent along +X; negative is invalid |
| `height` | float | Extent along +Y; negative is invalid |
| `cornerRadius` | float | Optional. Corner rounding in units; absent or 0 = square corners |

Position is the top-left corner, not the centre. A host that models rectangles from the centre converts on read.

Rotation belongs to [`rig.spatial.transform`](../spatial/transform.md) — there is no angle field here.

Appearance: [`rig.paint.fill_stroke`](../paint/fill-stroke.md).
