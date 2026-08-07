# `rig.music.clock`

Tick derivation from transport (or external sync). Format when present.

Does **not** own bpm — read [`rig.music.transport`](transport.md).

| Field | Type | Meaning |
|-------|------|---------|
| `ticksPerQuarter` | int | Pulses per quarter (e.g. 96) |
| `phaseTicks` | float | Optional. Running tick count; absent = 0 |
| `swingAmount` | float | Optional. 0 = straight; absent = 0 |
| `swingSubdiv` | int | Optional. Swing subdivision; absent = 2 |
| `externalSync` | bool | Optional. External writer owns phase; absent = false |
| `syncBeat` | float | External beat; required when `externalSync` |
| `syncPhase` | float | External phase 0–1; required when `externalSync` |
| `syncPeriodBars` | float | Optional. External sync period in bars |

Product protocol names stay out of the schema id.
