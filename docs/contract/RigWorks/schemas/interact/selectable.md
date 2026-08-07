# `rig.interact.selectable`

Opt-in selection eligibility. Format when present.

| Field | Type | Meaning |
|-------|------|---------|
| `enabled` | bool | Optional. Eligible for selection when true; absent = true |

Selection *state* (is-selected, multi-select index) stays in the host — do not serialize here.
Hit-test geometry from transform / shape / mesh — not from this component.

Hosts without this component may still pick entities (legacy). When present, pick must honor `enabled`.
