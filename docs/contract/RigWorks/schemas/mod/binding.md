# `rig.mod.binding`

| Field | Type | Meaning |
|-------|------|---------|
| `source` | entity | Modulator entity (e.g. LFO) |
| `target` | entity | Driven entity |
| `propertyKey` | string | Field name / path (same style as tween) |
| `depth` | float | Optional. Scale; absent = 1 |
| `min` | float | Optional. Clamp; absent = unclamped |
| `max` | float | Optional. Clamp; absent = unclamped |
| `additive` | bool | Optional. Add vs replace; absent = false (replace) |
