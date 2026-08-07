# `rig.music.midi_output`

MIDI sink identity. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `portName` | string | Platform port name as reported by the host |
| `portIndex` | uint | Host API port index |
| `open` | bool | Optional. Port open; absent = true |

Ship whichever identifier the host API exposes — `portName` when it names ports, `portIndex` when it numbers them; at least one must be present. Both are machine-local: moving a document between machines will not find the same port, and a host should fail visibly rather than silently bind to the wrong one.

Pending byte queues stay in the host — do not serialize. Flush in a code pack.
