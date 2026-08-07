# `rig.paint.fill`

Fill by reference — this drawable is painted by a shared paint entity. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `paint` | entity | Entity carrying [`rig.paint.solid`](solid.md) or [`rig.paint.gradient`](gradient.md) |

This is the referenced spelling of what [`rig.paint.fill_stroke`](fill-stroke.md) does inline — the same duality as SVG's `fill="#f00"` versus `fill="url(#g)"`. Use the reference when several drawables share one paint, so editing the paint restyles all of them; use the inline form when the colour belongs to this entity alone.

Carrying both this and an inline fill on one entity is a document error — there is no rule for which wins.

Stroke is separate: [`rig.paint.stroke`](stroke.md).
