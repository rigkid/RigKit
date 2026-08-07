# `rig.input.buttons`

Logical button state for one player. Format when present.

Compose [`rig.meta.named`](../meta/named.md) for a label. Portable state only — device handles, keybinds, and edge/pulse caches stay in the host.

| Field | Type | Meaning |
|-------|------|---------|
| `player` | int | Optional. Player index (0-based); absent = 0 |
| `left` | bool | Optional. Left / west; absent = false |
| `right` | bool | Optional. Right / east; absent = false |
| `up` | bool | Optional. Up / north; absent = false |
| `down` | bool | Optional. Down / south; absent = false |
| `o` | bool | Optional. Primary action (O / A / confirm); absent = false |
| `x` | bool | Optional. Secondary action (X / B / cancel); absent = false |

Hosts poll hardware into these fields during `Update`. Fantasy-console carts often mirror this layout (d-pad + two face buttons).
