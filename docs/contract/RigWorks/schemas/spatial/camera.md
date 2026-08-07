# `rig.spatial.camera`

Projection parameters (pose lives on transform). Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `active` | bool | Optional. Used for present (typically one); absent = true |
| `projection` | enum | Optional. perspective, orthographic; absent = perspective |
| `fovYDegrees` | float | Optional. Perspective FOV (degrees); absent = 60. Ignored if orthographic |
| `orthoHeight` | float | Ortho frustum height; required when orthographic |
| `nearClip` | float | Optional. Near plane; absent = 0.1 |
| `farClip` | float | Optional. Far plane; absent = 1000 |
| `aspect` | float | Optional. Absent or 0 = use viewport aspect at present |

No camera object / GPU handle.
