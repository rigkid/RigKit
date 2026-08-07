# `rig.pixel.source`

Playback / capture settings for a raster input. Format when present.

File identity is [`rig.media.asset_ref`](../media/asset-ref.md) via `asset`. Compose [`rig.meta.named`](../meta/named.md) for a label.

| Field | Type | Meaning |
|-------|------|---------|
| `kind` | enum | none, imageFile, generator, imageSequence, webcam, videoFile |
| `asset` | entity | Asset ref when kind needs a path; absent otherwise |
| `generatorName` | string | Generator id; required when `kind=generator` |
| `sequenceFps` | float | Optional. Sequence playback |
| `sequenceFrame` | int | Optional. Current frame index; absent = 0 |
| `webcamDevice` | int | Optional. Device index when `kind=webcam`; absent = default device |
| `webcamWidth` | int | Optional. Capture width hint |
| `webcamHeight` | int | Optional. Capture height hint |
| `videoTime` | float | Optional. Seconds into clip when `kind=videoFile`; absent = 0 |

Only `kind` is required — emit the fields the chosen kind needs and leave the rest absent.

Loop comes from the asset_ref when applicable. Decode / grab / bake = fulfillment.
