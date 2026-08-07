# `rig.music.transport`

**Tempo and playhead authority.** One per scene when music schemas are present.

| Field | Type | Meaning |
|-------|------|---------|
| `playing` | bool | Optional. Running; absent = false |
| `bpm` | float | Tempo |
| `timeSigNum` | int | Optional. Beats per bar; absent = 4 |
| `timeSigDen` | int | Optional. Note value of beat; absent = 4 |
| `positionBeats` | float | Optional. Playhead in beats; absent = 0 |
| `loop` | bool | Optional. Loop; absent = false |
| `loopStartBeats` | float | Loop in; required when `loop` |
| `loopEndBeats` | float | Loop out; required when `loop` |

`barIndex` / `beatInBar` are derived from `positionBeats` — do not serialize; recompute.
Pause = `playing=false` while keeping `positionBeats`.

Advance during `Update`. Clock / pattern / sequencer do **not** own bpm or the global playhead.
