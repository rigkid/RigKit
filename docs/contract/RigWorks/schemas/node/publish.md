# `rig.node.publish`

Maps one outer interface pin on a group node to an interior pin inside that node's nested graph. See [`rig.node.node`](node.md).

| Field | Type | Meaning |
|-------|------|---------|
| `pin` | uint | Outer pin id on the group node (must exist in `pins`) |
| `innerNode` | uint | Node id inside `nested` |
| `innerPin` | uint | Pin id on that inner node |

## Rules

- Each outer `pin` has at most one publish.
- Each interior `(innerNode, innerPin)` is published at most once per group.
- Outer pin `kind` / `type` should match the inner pin (hosts may coerce `type` per [pin](pin.md) linking rules).
- Parent-graph links only touch outer pins; interior links stay inside `nested`.
