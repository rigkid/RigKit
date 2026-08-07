# `rig.led.uv_map`

LED → UV sampling map. Format when present.

Compose [`rig.meta.named`](../meta/named.md) for the label.

| Field | Type | Meaning |
|-------|------|---------|
| `width` | int | Optional. Grid width; absent = normalized against the host output |
| `height` | int | Optional. Grid height; absent = normalized against the host output |
| `pixels` | {index:int, u:float, v:float}[] | LED → UV |

Sampling / present in code. Colours are runtime cache.
