# `rig.pixel.raster`

RGBA buffer. Usually runtime-only; may appear in small documents when needed.

| Field | Type | Meaning |
|-------|------|---------|
| `role` | enum | working, output, layerPixels, mask, composite |
| `width` | int | Pixels |
| `height` | int | Pixels |
| `rgba` | uint8[] | Tight RGBA, length ≥ w×h×4; empty = omit pixels |

No texture ids. Prefer rebuilding from source + chain over shipping large buffers.
