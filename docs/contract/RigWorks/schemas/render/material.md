# `rig.render.material`

Shading parameters. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `albedoRgb` | vec3 | Optional. Base colour (0–1); absent = white |
| `albedoMap` | entity | Optional texture / paint entity; none = unused |
| `metallic` | float | Optional. 0–1; absent = 0 |
| `roughness` | float | Optional. 0–1; absent = 1 |
| `emissive` | vec3 | Optional. Emissive RGB (0–1); absent = black |

GPU programs live in code packs.
