# `rig.spatial.relationship`

Scene-graph parent. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `parent` | entity | Parent entity; none = root |
| `order` | int | Optional. Sibling position under `parent`, ascending. Ties and gaps are allowed; a host sorts by it and breaks ties by document order. |

Emit `order` when sibling sequence carries meaning — draw order, z-order, stacking inside a group. Omit it when children are an unordered set. Without it, sibling sequence is not portable and a reader may present children in any order.

Compositor grouping uses `groupParent` on [`rig.pixel.layer`](../pixel/layer.md) — different graph, different field.

Scene group folders compose [`rig.spatial.group`](group.md) on the parent entity — still use this `parent` field for children.
