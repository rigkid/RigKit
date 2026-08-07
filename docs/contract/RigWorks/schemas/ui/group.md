# `rig.ui.group`

Section or row inside a [`rig.ui.panel`](panel.md). Groups carry structure that survives swapping UI packs — not dock slots or toolkit trees. Labels compose [`rig.meta.named`](../meta/named.md).

| Field | Type | Meaning |
|-------|------|---------|
| `panel` | entity | Owning [`rig.ui.panel`](panel.md) |
| `parent` | entity | Optional. Parent group entity; absent or `null` = top-level in the panel |
| `order` | int | Sort key among siblings (same `parent`) |
| `orientation` | enum | Optional. Flow hint: `vertical` \| `horizontal`; absent = `vertical`. Hosts may ignore |
| `collapsed` | bool | Optional. Disclosure hint; absent = false. Hosts without disclosure ignore it |

Controls and actions may set `group` to this entity. Nest groups via `parent` — do not invent a second graph.

Required: `panel`, `order`.
