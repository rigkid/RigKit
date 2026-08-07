# `rig.paint.stroke`

Stroke by reference — this drawable's outline is painted by a shared paint entity. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `paint` | entity | Entity carrying [`rig.paint.solid`](solid.md) or [`rig.paint.gradient`](gradient.md) |
| `width` | float | Optional. Stroke width in content units; absent = 1 |

The width stays here rather than on the paint, because two shapes sharing one swatch usually do not share a thickness.

Carrying both this and an inline stroke on one entity is a document error. The fill counterpart is [`rig.paint.fill`](fill.md); the inline form is [`rig.paint.fill_stroke`](fill-stroke.md).
