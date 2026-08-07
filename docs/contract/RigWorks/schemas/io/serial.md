# `rig.io.serial`

| Field | Type | Meaning |
|-------|------|---------|
| `port` | string | Device path / name |
| `baud` | int | Optional. Baud rate; absent = 9600 |
| `enabled` | bool | Optional. Open when true; absent = false |

Byte buffers and device handles stay in code packs.
