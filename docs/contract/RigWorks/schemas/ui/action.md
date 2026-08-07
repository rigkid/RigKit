# `rig.ui.action`

A command button on a panel. Edges toward behavior; this schema is the piece most likely to change before 1.0.0.

| Field | Type | Meaning |
|-------|------|---------|
| `panel` | entity | Owning [`rig.ui.panel`](panel.md) |
| `group` | entity | Optional. Owning [`rig.ui.group`](group.md); absent or `null` = direct child of the panel |
| `order` | int | Sort key within the panel or group |
| `actionId` | string | Command catalog id (e.g. `lfo.resetPhase`, `transport.play`) |
| `enabled` | bool | Whether the action can be invoked |

Labels compose [`rig.meta.named`](../meta/named.md). What the action *does* is fulfillment — the Contract only names which command the surface exposes.

**Portability:** prefer shared `actionId` names that both hosts implement. Unknown ids may be hidden or disabled (ship what you support). Host-private ids are valid but non-portable. Cross-app tools should prefer [`rig.ui.control`](control.md) (mutate shared POD) over actions when possible — see [ui.md](../../docs/ui.md).

Required: `panel`, `order`, `actionId`.
