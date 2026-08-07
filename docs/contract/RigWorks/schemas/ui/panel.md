# `rig.ui.panel`

Portable **tool surface** — a window/panel of controls that any UI pack can redraw. Labels compose [`rig.meta.named`](../meta/named.md) — do not re-declare `name` here.

| Field | Type | Meaning |
|-------|------|---------|
| `role` | string | Optional. Stable tool id (e.g. `led.install`, `mod.lfo`) so another host can find or merge the tool |
| `order` | int | Optional. Sort key among sibling panels; absent = 0 |
| `visible` | bool | Optional. Whether the panel is shown; absent = true |
| `preferredWidth` | float | Optional. Advisory width hint in the document's `defaultUnit`; hosts may ignore |
| `preferredHeight` | float | Optional. Advisory height hint; hosts may ignore |

Controls, actions, and [`rig.ui.group`](group.md) reference this entity via their `panel` field. Layout chrome (docks, tabs, ESP page shells, toolkit window flags) is fulfillment — not this schema.

A host that implements six schemas and ignores the rest may still ship a panel over the fields it understands. Ship what you support.
