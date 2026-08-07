# `rig.spatial.layer`

Document / render-order layer chrome. Format when present.

Compose [`rig.meta.named`](../meta/named.md) for the label.

Compositor stacks use [`rig.pixel.layer`](../pixel/layer.md) on the same entity when needed — do not duplicate these chrome fields there.

| Field | Type | Meaning |
|-------|------|---------|
| `order` | int | Optional. Draw / list order; absent = 0 |
| `visible` | bool | Optional. Drawn when true; absent = true |
| `locked` | bool | Optional. Edit lock hint; absent = false |
| `rgba` | vec4 | Optional. Tint / label colour (0–1); absent = no label colour. Hosts may store as four floats |

Parenting stays on [`rig.spatial.relationship`](relationship.md).
