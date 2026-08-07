# `rig.media.text`

Scene / canvas text run. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `text` | string | Content |
| `font` | entity | Optional. [`rig.media.asset_ref`](asset-ref.md) with kind font; absent = host default face |
| `fontSize` | float | Optional. Point or pixel size as the host documents; absent = host default |
| `rgba` | vec4 | Optional. Text colour (0–1); absent = black |
| `axes` | axis[] | Optional. OpenType variable-font axis values; absent = font defaults |
| `features` | string | Optional. Comma-separated OpenType feature tags (e.g. `liga,ss01`); absent = host default |
| `useKerning` | bool | Optional. Apply kerning; absent = true |

## Axis

| Field | Type | Meaning |
|-------|------|---------|
| `tag` | uint32 | 4-char OpenType tag packed as big-endian uint32 (e.g. `'wght'` = `0x77676874`) |
| `value` | float | Authored axis value. Min/max/name come from the font file at fulfillment time |

Face handles, glyph caches, and GPU atlases stay in the host.