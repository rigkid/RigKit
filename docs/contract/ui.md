# UI

**Rig + UI** sits on top of Rig (SUDE + ECS). Portable rules: [Rig UI](https://github.com/rigkid/RigWorks/blob/main/docs/ui.md).

This file adds **RigKit** fulfillment notes. UI packs are not Rig.

## Contract vs fulfillment

| Layer | What |
|-------|------|
| **Rig + UI** | Host seam; panels edit ECS/schemas; SUDE phases; no toolkit in Contract-facing components |
| **rigImGui** | Default RigKit UI pack: dock layout, windows, Properties |

Portable UI description is **data** (sections bound to ECS / property paths). Toolkits (`rigImGui`, a future Web UI pack) are **fulfillments** of that map through `IMui` — not a second scene graph and not per-window packs.

Another UI pack may honor the same Rig + UI rules. Dock chrome is layout, not entity meaning.

## Why UI on ECS

Author/tool surfaces share portable entity data. Skip ECS and you fork Properties, catalogs, and schemas.

**Portable panels:** a particle panel and an audio panel can appear in different apps and still compose (e.g. spatial sound) when both mutate the same POD fields. Layout only arranges views.

**Properties:** portable datatype rows ([Rig properties](https://github.com/rigkid/RigWorks/blob/main/docs/properties.md)) so any Properties surface can show opt-in fields. RigKit: `sProp` / `GetProperties()` drawn by **rigImGui**.

## When it applies

- Host offers author/tool surfaces through a seam.
- Host honors Rig (SUDE + ECS).
- Show / headless / light embedded hosts omit UI → **Rig** only.

## Contract rules

- UI attaches through a host seam (`IMui` in RigKit).
- Surfaces register by name; visibility can change.
- Input in **Update**; UI present in **Draw** (after app `Draw`); teardown on **Exit**.
- Edit ECS POD / schemas — not a second scene graph.
- No toolkit types inside Contract-facing components.

## Non-requirements

No particular UI pack. No particular dock model. No GPU editor on light hosts.

## RigKit fulfillment (**rigImGui**)

| Seam | Role |
|------|------|
| [`IMui`](../../src/core/IMui.h) | Host UI manager interface |
| `attachUiManager` / `detachUiManager` | Attach or omit UI |
| `IApp::setWindowVisibility*` | Show windows by name |
| `IWindow` + `MWindow` | Pack/app panels in the dock layout |
| `IMui::dockPassthroughCentral` | Central node shows GL bed vs solid fill |
| File / export / undo / gizmo hooks | Author chrome |
| File → Open Recent | `IMui::noteRecentFile` + `setRecentFileOpenHandler`; persisted in `rigkit_settings.json` |
| View → Workspace | Named dock layouts (`rigImGui`): save/load/delete `data/user/workspaces/<name>.ini` (docks + window visibility); active name in `rigkit_settings.json`; startup loads last used or `Standard` || [`IMui::progress()`](../../src/core/util/Progress.h) | Progress chrome (status bar or floating); null when UI detached |

### Progress

Author chrome for long jobs (export, load, connect). Portable meaning is plain fields on [`Progress`](../../src/core/util/Progress.h) — title, label, fraction (`< 0` = indeterminate), cancel flag. Apps call through `IMui::progress()` (not a process singleton).

```cpp
if (auto* p = ui->progress()) {
	p->begin("Exporting layers", 80);
	// … per step:
	p->tick("Layer 3 / 80");
	p->finish("Export complete");
}
```

**rigImGui** draws the bar in the status strip by default, or a centered floating window when Preferences → Progress In Status Bar is off. Auto-hide after finish is configurable. Show mode detaches UI → `progress()` is null.

Layout persistence next to the exe is fulfillment. Core must not include a UI toolkit — only `IMui`.

## Coordinates and HiDPI

| Space | Owner |
|-------|--------|
| Window / framebuffer | Host |
| UI / mouse | **rigImGui** (or other UI pack) |
| Content / bed | App + `View2D` |

## Compliance

**Rig + UI:** Honor Rig + the rules above.  
**Rig:** omit UI.

See [Rig honors](https://github.com/rigkid/RigWorks/blob/main/docs/honors.md), [distribution.md](distribution.md), [README.md](README.md).
