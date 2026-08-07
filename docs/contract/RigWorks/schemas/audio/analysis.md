# `rig.audio.analysis`

Request for spectral analysis of an audio stream. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `source` | enum | `input` (live capture) or `asset` |
| `asset` | entity | Required when `source` is `asset`; an entity carrying [`rig.media.asset_ref`](../media/asset-ref.md) |
| `bandCount` | int | Number of frequency bands to produce |
| `smoothing` | float | Optional. 0–1; 0 = no smoothing between frames |
| `onsetThreshold` | float | Optional. Absent disables onset detection |

This describes the analysis to perform, not its results. Band magnitudes and onset flags change every frame and are host state — writing them into a document would make every save differ.

Band layout is deliberately unspecified beyond the count. Linear and logarithmic spacing both occur in the wild and neither is right for every use, so two hosts will agree on the request and may disagree on the bands. Do not build a document that depends on an exact band centre frequency.

To drive a property from a band, use [`rig.mod.binding`](../mod/binding.md).
