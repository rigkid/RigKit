# `rig.paint.gradient`

Shared gradient paint (optional library entity).

| Field | Type | Meaning |
|-------|------|---------|
| `kind` | enum | linear, radial |
| `stops` | {t:float, rgba:vec4}[] | Sorted t in 0–1 |
| `p0` | vec2 | Optional. Start / center; absent = (0, 0) in object space |
| `p1` | vec2 | Optional. End / radius hint; absent = (1, 0) in object space |
