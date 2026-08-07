# `rig.node.node`

One node inside a [`rig.node.graph`](graph.md). Nested POD — not a separate ECS entity unless a host chooses that layout.

| Field | Type | Meaning |
|-------|------|---------|
| `id` | uint | Stable id within the graph |
| `typeId` | string | Catalog **behavior** key (host vocabulary, e.g. `float.add`) — not a property datatype |
| `title` | string | Display label |
| `pos` | vec2 | Editor canvas position (not world transform) |
| `pins` | pin[] | See [`rig.node.pin`](pin.md) — pin `type` uses [property datatypes](../../docs/properties.md) |
| `params` | param[] | See [`rig.node.param`](param.md) — same datatype table |
| `nested` | graph? | When present, this node **is a group**; interior is a full [`rig.node.graph`](graph.md) |
| `publishes` | publish[] | Outer pin → interior pin — see [`rig.node.publish`](publish.md) |

## Leaf vs group

- **Leaf:** omit `nested` (and usually `publishes`). `typeId` selects catalog evaluation.
- **Group:** `nested` is present. Outer `pins[]` are the only interface other nodes link to. Author publishes chosen interior pins; host keeps outer pin `name` / `kind` / `type` aligned with (or renamed from) the inner pin. `typeId` is host vocabulary (e.g. `group`); eval runs the nested graph, not a maths catalog entry.

Groups nest recursively: nodes inside `nested` may themselves have `nested`.

`typeId` selects evaluation / catalog entry for leaves. Pin and param `type` fields describe **values**, using the Contract property table.
