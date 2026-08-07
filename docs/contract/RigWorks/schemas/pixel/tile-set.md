# `rig.pixel.tile_set`

Indexed pixel sheet for tiles / sprites. Format when present.

Compose [`rig.meta.named`](../meta/named.md) for a label. Colour identity is [`rig.pixel.palette`](palette.md) via `palette`. Prefer this over shipping large [`rig.pixel.raster`](raster.md) RGBA when the host is palette-indexed.

| Field | Type | Meaning |
|-------|------|---------|
| `palette` | entity | Entity with [`rig.pixel.palette`](palette.md) |
| `tileWidth` | int | Tile width in pixels |
| `tileHeight` | int | Tile height in pixels |
| `tilesAcross` | int | Columns in the sheet (≥ 1) |
| `indices` | uint8[] | Row-major palette indices for the full sheet; length = `tilesAcross * tileRows * tileWidth * tileHeight` |
| `tileRows` | int | Rows in the sheet (≥ 1) |
| `flags` | uint8[] | Optional. Per-tile flag byte; length = `tilesAcross * tileRows`; absent or empty = no flags |

No texture ids. Decode / atlas upload = fulfillment.
