# `rig.paint.library`

Ordered swatch collection. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `paints` | entity[] | Paint entities in panel order; each carries [`rig.paint.solid`](solid.md) or [`rig.paint.gradient`](gradient.md) |

A library answers the question a reader otherwise cannot: which paint entities are the document's swatches, and in what order they should be presented. Without it, shared paints are just entities floating in the scene.

Order is presentation order, nothing more. Membership does not affect rendering — a paint referenced by [`rig.paint.fill`](fill.md) or [`rig.paint.stroke`](stroke.md) paints its users whether or not any library lists it.

A document usually has one library entity, but nothing forbids several (per page, per theme). A paint may appear in more than one.
