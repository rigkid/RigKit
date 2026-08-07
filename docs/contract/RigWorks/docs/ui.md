# UI (optional companion)

**Rig + UI** sits on top of Rig (SUDE + ECS). It is still **zero code**. Layout chrome and UI packs are fulfillment — not this document.

## Why UI on ECS

Author and tool surfaces inspect and edit the **same portable entity data**. A UI that skips ECS forks schemas, Properties, and catalogs.

A panel is a view over schemas, not an app-private island. Load a particle panel and an audio panel in different products — if both speak the same POD fields, spatial sound and particles still compose. Layout only arranges those views.

## Cross-lib / cross-app tools

A **tool** is a Rig document: domain entities (or schema ids the host already has) plus [`rig.ui.*`](../schemas/ui/panel.md) surfaces. Author the controls in App A with UI pack X; save JSON; load in App B with UI pack Y. Same meaning — different chrome.

| Portable | Not portable |
|----------|----------------|
| Panels, groups, controls, shared `actionId`s | Dock slots, ImGui/Qt/DOM trees, window flags |
| `role` tool ids, advisory size / orientation | Pixel positions, DPI, focus / hover |
| Field bindings (`target` + `propertyKey`) | Host-private command catalogs |

Unknown controls or actions: skip or hide. Ship what you support.

## Rig + UI

A project is **Rig + UI** when:

1. It already **is Rig** (SUDE + ECS).
2. Author/tool surfaces attach through a **host seam** (no toolkit types inside Contract-facing components).
3. Surfaces edit ECS POD / schemas — not a second scene graph.
4. Surfaces register by name (or equivalent); visibility can change.
5. Input in **Update**; UI present in **Draw** (after app `Draw`); teardown on **Exit**.
6. Prefer portable [property datatypes](properties.md) so any Properties surface can show opt-in fields.

Omitting UI remains valid **Rig**.

## Portable panels (Contract)

| Rule | Meaning |
|------|---------|
| Panel ↔ schema | Edits named POD / Rig schemas |
| Cross-host reuse | Same panel *role* works wherever those components exist |
| Compose by data | Particles + audio + … compose because entities share fields |
| Layout ≠ meaning | Docks/tabs/workspaces are fulfillment; groups are portable structure |
| Properties ↔ datatypes | A generic property manager draws any field with a known datatype |

Portable tool graphs use [`rig.ui.panel`](../schemas/ui/panel.md), [`rig.ui.group`](../schemas/ui/group.md), [`rig.ui.control`](../schemas/ui/control.md), and [`rig.ui.action`](../schemas/ui/action.md). A control is a view over `target` + `propertyKey` — the same addressing as [`rig.mod.binding`](../schemas/mod/binding.md) — never a second store.

### Actions

Prefer controls that mutate shared POD. When a button is needed:

- Use a **shared** `actionId` both hosts implement (e.g. `lfo.resetPhase`).
- Treat unknown `actionId`s as non-portable — hide or disable them.
- Host-private catalogs are fine inside one product; they will not travel.

## Web and embedded surfaces

The same schemas describe a desktop Properties panel, a generated web form, and a small ESP32-served control page:

| Surface | Typical fulfillment |
|---------|---------------------|
| Desktop author UI | RigKit + rigImGui (or any pack over the host seam) |
| Generated web UI | Browser form / canvas reading `rig.ui.*` and writing target fields |
| ESP32 / MCU panel | Tiny HTTP page that exposes the subset of controls the device understands |

An LED install that only speaks colour and LFO frequency can ship a panel with two controls and ignore every other schema. Ship what you support — partial execution of a shared concept still interoperates.

See [`examples/ui-panel.json`](../examples/ui-panel.json) and [`examples/portable-tool.json`](../examples/portable-tool.json).

## Non-requirements

Rig + UI does **not** require a particular UI pack, dock model, or GPU editor. The Contract is the seam + ECS editing — not the chrome. Do not encode ImGui (or any toolkit) as the wire format.

## Fulfillment (RigKit)

| Pack / host | Role |
|-------------|------|
| RigKit + **rigImGui** | Default author UI: dock layout, windows, Properties over `GetProperties()` — a host that speaks Rig, not "the Rig implementation" |
| Other UI packs | May implement the same seam |
| Show / headless | No UI — Rig only |

See [honors.md](honors.md).
