# `rig.ui.control`

A view over one POD field on a target entity — never a second store. Addressing reuses the [`rig.mod.binding`](../mod/binding.md) convention (`target` + `propertyKey`, including dotted paths like `position.y`).

| Field | Type | Meaning |
|-------|------|---------|
| `panel` | entity | Owning [`rig.ui.panel`](panel.md) |
| `group` | entity | Optional. Owning [`rig.ui.group`](group.md); absent or `null` = direct child of the panel |
| `order` | int | Sort key within the panel or group |
| `target` | entity | Entity whose field is shown / edited |
| `propertyKey` | string | Field name or dotted path on the target |
| `type` | propertyType | Datatype id from [properties.md](../../docs/properties.md) (or host-prefixed) |
| `min` | float | Optional lower bound for numeric widgets |
| `max` | float | Optional upper bound |
| `step` | float | Optional step for numeric widgets |
| `enabled` | bool | Whether the control accepts input |
| `readOnly` | bool | Show value without editing |
| `options` | string[] | Optional enum choices when the target schema does not constrain them |
| `widget` | enum | Advisory hint: `auto`, `slider`, `knob`, `toggle`, `field`, `dropdown`, `color`, `xy` |

`widget` is a **hint a host may ignore**. Colour pickers and knobs vs sliders remain fulfillment — see [properties.md](../../docs/properties.md). Domain meaning stays in schema field names, not extra datatype ids.

Required: `panel`, `order`, `target`, `propertyKey`, `type`.
