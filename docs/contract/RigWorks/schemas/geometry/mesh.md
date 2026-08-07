# `rig.geometry.mesh`

Indexed mesh. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `positions` | float[] / vec3[] | xyz tightly packed or vec3 array |
| `indices` | uint32[] | Triangle (or line) indices; empty = sequential |
| `texcoords` | float[] / vec2[] | Per-vertex uv (optional; empty = none) |
| `mode` | enum | triangles, lines, lineStrip |
| `faceColors` | vec4[] | Optional per-face colour |
| `facePalette` | uint8[] | Optional per-face palette index |

Normals may be computed at present; optional authored normals are a host extension until needed.

No GPU buffer handles.
