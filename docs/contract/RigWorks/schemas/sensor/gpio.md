# `rig.sensor.gpio`

| Field | Type | Meaning |
|-------|------|---------|
| `pin` | int | GPIO index |
| `mode` | enum | in, out |
| `level` | float | Optional. 0–1 reading or drive; absent = 0 |
| `device` | entity | Optional board / network entity; absent or none = local |

Digital hosts threshold `level` themselves (e.g. ≥ 0.5).
