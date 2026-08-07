# `rig.mod.lfo`

Low-frequency oscillator. Format when present.

Replaces a separate “pulse” schema — use waveform + binding.

| Field | Type | Meaning |
|-------|------|---------|
| `waveform` | enum | sine, tri, saw, square |
| `frequency` | float | Hz |
| `amplitude` | float | Optional. Depth; absent = 1 |
| `offset` | float | Optional. Bias; absent = 0 |
| `phase` | float | Optional. 0–1 (wrap); absent = 0 |

Last sample is a runtime cache — do not serialize. Drive targets through [`rig.mod.binding`](binding.md).
