# `rig.music.midi_input`

MIDI input port binding. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `portName` | string | Platform port name as reported by the host |
| `portIndex` | uint | Host API port index |
| `open` | bool | Optional. Whether the document expects this port open; absent = true |
| `channel` | int | Optional. 0–15; absent means omni |

Ship whichever identifier the host API exposes — `portName` when it names ports, `portIndex` when it numbers them; at least one must be present.

The mirror of [`rig.music.midi_output`](midi-output.md), and it carries the same caveat: both identifiers are machine-local. Moving a document between machines will not find the same port, and a host should fail visibly rather than silently bind to the wrong one.

Incoming notes and controller values are host state and are never serialized. To route a controller onto a property, use [`rig.mod.binding`](../mod/binding.md) rather than adding mappings here.
