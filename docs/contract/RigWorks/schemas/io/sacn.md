# `rig.io.sacn`

E1.31 (sACN) pixel output over the network. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `universe` | int | 1–63999 |
| `startChannel` | int | Optional. 1–512; first DMX slot used in the universe; absent = 1 |
| `host` | string | Optional. Unicast target; absent means multicast |
| `fps` | float | Optional. Send rate; absent means the host's frame rate |
| `enabled` | bool | Optional. Whether output is running; absent = true |
| `uvMap` | entity | Optional. Entity carrying [`rig.led.uv_map`](../led/uv-map.md) |

Completes the LED path already covered by [`rig.led.uv_map`](../led/uv-map.md) and [`rig.io.serial`](serial.md): the UV map says where each pixel samples the canvas, this says where the bytes go.

Channel count is not stored — it follows from the pixel count of the referenced UV map. A fixture spanning more than one universe uses one entity per universe.

Colour order (RGB versus GRB and friends) is a property of the fixture, not of the document, and is not carried here. Two hosts pointed at the same rig will agree on the pixels and may still disagree on the wire order.
