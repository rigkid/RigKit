# `rig.pixel.palette`

Ordered indexed colour table. Format when present.

Compose [`rig.meta.named`](../meta/named.md) for a label. Canvas clear and paint rgba stay as floats 0–1; this schema documents the discrete palette a host uses when interpreting indexed pixels ([`rig.pixel.tile_set`](tile-set.md)).

| Field | Type | Meaning |
|-------|------|---------|
| `colors` | rgba[] | Ordered entries; index `i` → `colors[i]` (floats 0–1) |

Hosts that ship a fixed console palette still serialize it here when a document needs portable colour identity. Runtime remaps (`pal()` swaps) stay in the host.

An index at or past `colors.length` (e.g. in [`rig.pixel.tile_set`](tile-set.md) `indices`) is a document error. A writer must not emit one; a lenient reader renders it as the last entry rather than crashing, but must not silently rewrite the data.
