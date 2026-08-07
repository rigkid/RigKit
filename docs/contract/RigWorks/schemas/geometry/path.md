# `rig.geometry.path`

Freeform 2D path command stream. Format when present.

Not a parametric shape. A contour that stays editable as parameters belongs to one of the primitives — [`rectangle`](rectangle.md), [`ellipse`](ellipse.md), [`line`](line.md), [`polygon`](polygon.md), [`regular_polygon`](regular-polygon.md), [`star`](star.md), [`arc`](arc.md), [`ring`](ring.md). Use a path once it has been resolved to commands, and never both for the same contour.

| Field | Type | Meaning |
|-------|------|---------|
| `commands` | command[] | Ordered path commands |

## Command

| Field | Type | Meaning |
|-------|------|---------|
| `type` | enum | moveTo, lineTo, cubicTo, quadTo, close |
| `p` | vec2 | Endpoint (move/line); unused for close |
| `c1` | vec2 | Cubic/quad control 1 |
| `c2` | vec2 | Cubic control 2 |

Closed contours use a `close` command. Unknown types → skip / reject. Extend only by appending enum values.
