# `rig.media.code`

**Editable source buffer.** A text document the app owns; editors edit it in place. Format when present.

Compose [`rig.meta.named`](../meta/named.md) for the label, and [`rig.media.asset_ref`](asset-ref.md) on the same entity when the buffer has an on-disk origin or export target.

| Field | Type | Meaning |
|-------|------|---------|
| `text` | string | Buffer content — the source of truth, not a cache of a file |
| `language` | string | Optional. Lowercase grammar id (`gcode`, `svg`, `python`); absent or empty = plain text |
| `readOnly` | bool | Optional. Edit lock hint; absent = false |

Distinct from [`rig.media.text`](text.md), which is display copy with font and colour.

Editor state — caret, selection, undo history, modified flags, reload counters — stays in the host.
