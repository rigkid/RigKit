# `rig.geometry.line`

Single straight segment. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `x1` | float | Start X, local space |
| `y1` | float | Start Y, local space |
| `x2` | float | End X, local space |
| `y2` | float | End Y, local space |

A line has no interior. `hasFill` on [`rig.paint.fill_stroke`](../paint/fill-stroke.md) is ignored; stroke settings apply.

Multi-segment runs use [`rig.geometry.polygon`](polygon.md) when open or closed straight chains, or [`rig.geometry.path`](path.md) when curves are involved.
