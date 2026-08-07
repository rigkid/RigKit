# `rig.music.sequencer`

Playhead over a pattern. Format when present.

A sequencer **has** a pattern. The pattern **has** steps.

| Field | Type | Meaning |
|-------|------|---------|
| `pattern` | entity | Entity with [`rig.music.pattern`](pattern.md) |
| `currentStep` | int | Optional. Playhead index into `pattern.steps`; absent = 0 |
| `clock` | entity | Optional. Entity with [`rig.music.clock`](clock.md); absent = the host's default clock |

Running state comes from transport (`playing`). Step count = `pattern.steps.length`.
No onStep callbacks.
