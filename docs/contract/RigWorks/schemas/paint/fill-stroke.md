# `rig.paint.fill_stroke`

Inline fill / stroke on a drawable entity. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `fillRgba` | vec4 | Optional. Fill colour (0–1); hosts may store as four floats |
| `strokeRgba` | vec4 | Optional. Stroke colour (0–1); hosts may store as four floats |
| `strokeWidth` | float | Optional. Width in content units; absent = 1 |
| `hasFill` | bool | Optional. Draw fill; absent = whether `fillRgba` is present |
| `hasStroke` | bool | Optional. Draw stroke; absent = whether `strokeRgba` is present |

Host extensions (optional): stroke caps / joins / dash.

Shared library paints use [`rig.paint.solid`](solid.md) / [`rig.paint.gradient`](gradient.md) on their own entities, referenced from drawables via [`rig.paint.fill`](fill.md) / [`rig.paint.stroke`](stroke.md). Carrying the inline and referenced spelling for the same slot on one entity is a document error.
