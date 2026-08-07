# `rig.media.asset_ref`

**File identity.** One schema for paths. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `kind` | enum | image, audio, video, model, font, other |
| `path` | string | File or directory path (host root rules apply) |
| `loop` | bool | Optional. Playback loop when the asset kind supports it; meaningless for stills, so absent is not false — it is "not applicable" |

Decode / GPU / players are fulfillment. Other schemas reference this entity — they do not invent parallel path fields.
