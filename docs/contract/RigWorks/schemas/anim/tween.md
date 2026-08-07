# `rig.anim.tween`

Drive one float property over time. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `target` | entity | Entity to drive |
| `propertyKey` | string | Field name / path |
| `from` | float | Start |
| `to` | float | End |
| `duration` | float | Seconds |
| `elapsed` | float | Optional. Seconds; absent = 0 |
| `easing` | enum | Optional. linear, easeIn, easeOut, easeInOut; absent = linear |
| `loop` | bool | Optional. Repeat; absent = false |
| `playing` | bool | Optional. Active; absent = true |

No onComplete callbacks — host flags / events only.
