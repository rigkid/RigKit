# `rig.render.light`

| Field | Type | Meaning |
|-------|------|---------|
| `enabled` | bool | Optional. Light on; absent = true |
| `type` | enum | Optional. directional, point; absent = point |
| `rgb` | vec3 | Optional. Light colour (0–1); absent = white. Hosts may store as three floats |
| `intensity` | float | Optional. Scale; absent = 1 |
| `ambient` | float | Optional. Fill / darkest band level; absent = 0 |
| `banded` | bool | Optional. Discrete shade steps when true; absent = false |
| `bands` | int | Band count; required when `banded` |

Pose on transform. Directional shines along local −Z (world). Point uses world translation.

Spot / range / cone are not in the current draft — append later if a host ships them.
No light object handle.
