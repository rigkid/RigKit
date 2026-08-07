# `rig.pixel.layer`

Compositor stack entry. Format when present.

**Chrome** (`index`, `visible`, `locked`, tint, label) lives on [`rig.spatial.layer`](../spatial/layer.md) + [`rig.meta.named`](../meta/named.md) on the same entity. Do not duplicate those fields here.

| Field | Type | Meaning |
|-------|------|---------|
| `kind` | enum | vector, overlayImage, solid, group |
| `blendMode` | enum | Optional. normal, multiply, screen, overlay, add; absent = normal |
| `opacity` | float | Optional. 0–1; absent = 1 |
| `image` | entity | Overlay [`rig.media.asset_ref`](../media/asset-ref.md) when kind needs it |
| `rgba` | vec4 | Solid fill; required when `kind=solid` |
| `maskSource` | enum | Optional. none, luma, alpha, path; absent = none |
| `maskAsset` | entity | Luma image asset when `maskSource=luma` |
| `maskLayer` | entity | Alpha source layer when `maskSource=alpha` |
| `maskPathEntity` | entity | Path entity when `maskSource=path` |
| `invertMask` | bool | Optional. Invert mask; absent = false |
| `groupParent` | entity | Optional. Compositor group parent; absent or none = root |

`groupParent` is **not** `parent` on [`rig.spatial.relationship`](../spatial/relationship.md) — different graph.

Rasters: [raster.md](raster.md).
