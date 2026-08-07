# `rig.music.pattern`

Ordered steps. Format when present.

Compose [`rig.meta.named`](../meta/named.md) for the label. Tempo comes from transport — no `bpm` here.

A [`rig.music.sequencer`](sequencer.md) **has** a pattern. A pattern **has** steps.

| Field | Type | Meaning |
|-------|------|---------|
| `steps` | step[] | Ordered [`rig.music.step`](step.md) cells; length = pattern length |
| `rootNote` | int | Optional. MIDI pitch 0–127 (scale root); absent = 60 |
| `scale` | enum | Optional. chromatic, major, minor, dorian, pentatonic; absent = chromatic |
| `lane` | int | Optional. Voice index when several patterns play together |
| `stepsPerBeat` | float | Optional. Steps advanced per transport beat; absent = 4 (16th notes) |
| `loopStartStep` | int | Optional. First step of the loop region; absent = 0 |
| `loopEndStep` | int | Optional. Step after the last looped step; absent = `steps.length` (loop the whole pattern) |

No parallel `numSteps` — use `steps.length`.

`stepsPerBeat` is musical time: the wall-clock rate comes from transport bpm, so a tracker's absolute per-pattern speed converts through the transport (a PICO-8 sfx at speed *s* with the transport at 1800⁄*s* bpm is `stepsPerBeat` 4; a sfx at a different speed scales it proportionally).

`steps` is a single row. A drum grid is several pattern entities sharing one [`rig.music.sequencer`](sequencer.md), each with its own `lane` — not one pattern holding a matrix. Lanes advance together, so patterns in one stack should be the same length; a host running unequal lengths is polymetric, which the Contract does not describe.

Omit `lane` for a single-voice pattern. Two patterns in one stack sharing a lane number is a document error, not a layering instruction.
