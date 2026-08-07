# `rig.io.osc`

OSC UDP listen / send endpoints. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `listenEnabled` | bool | Optional. Listen active; absent = false |
| `listenPort` | int | Listen port; required when `listenEnabled` |
| `sendEnabled` | bool | Optional. Send active; absent = false |
| `sendHost` | string | Send target host; required when `sendEnabled` |
| `sendPort` | int | Send port; required when `sendEnabled` |
| `addressPrefix` | string | Optional. Address prefix |

Listen and send sides are independent — enable one, both, or neither, and emit only the fields the enabled side needs.

Message queues and sockets are host runtime.
