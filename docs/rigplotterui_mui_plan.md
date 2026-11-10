# rigPlotterUi Mui repair plan

## Outcome

`rigPlotterUi` compiles and runs against the current `IMui` / `Mui` API again.
No host API grows for this: the current menu actions already express
everything the pack registers, except one widget-in-menu, which moves to a
preference. Behavior (menus, enablement, shortcuts) stays the same.

## Evidence

- `rigPlotterUi.cpp` fails first: `IMui` has no `getWindowManager` (9 calls)
  and no `registerAppSubmenu` / `registerFileSubmenu` / `registerEditSubmenu`
  (6 calls). `rigImGui` renamed the submenu registrations to flat actions
  with a `group` parameter and dropped the free `windowManager(IMui*)`
  helper without a caller sweep (see `notes.md`).
- The current API covers the need: `registerFileAction` /
  `registerAppAction` / `registerViewAction` take
  `(label, action, shortcut, group)`; `registerEditAction` takes
  `(label, shortcut, isEnabled, action, group)`. A non-empty `group` nests
  the row under that submenu label; chrome draws, packs supply hooks.
- Window access is `dynamic_cast<Mui*>(engine->getUiManager())` then
  `mui->getWindowManager()` - the pattern already applied in
  `rigNodeEditor` and the node / pixel examples.
- One call does not map: the "Fit Curves" Edit submenu draws a tolerance
  `DragFloat` (with tooltip) inside the menu body before its
  "Apply to Selection" item. Actions are label + callback; no widget
  surface exists, and growing `IMui` for one menu widget is out of bounds
  (Commandment 9 - seams stay small).

## Decisions

1. **Submenus become grouped actions.** Each old submenu body is decomposed
   into one action per menu item, with `group` set to the old submenu
   label: App to Import ("Resource..."), File to Place (SVG / DXF /
   G-code), File to Rotate (90 CW / 90 CCW / 180), Edit to Rotate (same
   three, layer scope), Edit to Align (Left / H Center / Right / Top / ...).
   Enablement predicates move to `registerEditAction`'s `isEnabled`;
   File/App actions that were conditionally registered stay conditionally
   registered.
2. **Fit Curves tolerance becomes a preference.** Register a
   `rigPlotterUi.fit` preference section (tolerance in mm, clamped) through
   `MSettings`, the same mechanism as the node editor prefs; the menu keeps
   one grouped action "Apply to Selection" that reads it. The tolerance is
   still adjustable (Preferences UI), just not from inside the menu.
   Record the old default (0.2 mm) and clamp range (0.01 to 5 mm).
3. **Window manager via `Mui` cast** at each of the 9 sites, no helper
   reintroduced in the host. If a third pack needs the same two lines,
   that is a `rigImGui` concern, not an `IMui` one.
4. **No other modernization in this change.** File-by-file compile fixes
   only where the API moved; no refactors, no renames.

## Phases

### Phase 1: Menu registration

Rewrite the six submenu registrations in `rigPlotterUi.cpp` as grouped
actions (Decisions 1-2), including the Fit Curves preference.

Gate: `rigPlotterUi.cpp` compiles; menu structure in the running app
matches the old tree (Import / Place / Rotate / Align groups, Delete
enablement, Fit Curves present).

### Phase 2: Window manager sites

Apply the `Mui` cast at the 9 `getWindowManager` sites.

Gate: the pack library target builds (`packs/rigPlotterUi/examples/plotui`
tree, `--target rigPlotterUi`).

### Phase 3: Compile-driven sweep

The compiler stopped at the first file; more drift may sit behind it
(other pack sources, the `plotui` example). Build the full example and
fix what surfaces, one file at a time, same rule: current API only, fix
where it lives.

Gate: `packs/rigPlotterUi/examples/plotui` builds and launches; the
plotter workflow (open, place, rotate, align, fit, export) is reachable
from the menus.

### Phase 4: Close out

- Update `notes.md` (Mui drift entry names this pack as worst case).
- Grep the remaining packs for `register*Submenu` and
  `IMui::getWindowManager` so the next pack does not rediscover this.

## Risks

- **Menu behavior drift:** grouped actions render through `Mui` chrome;
  ordering inside a group follows registration order. Register items in
  the old visual order.
- **Enablement:** File/App actions have no `isEnabled` slot; the old code
  registered them unconditionally too, so nothing changes. Edit actions
  keep their predicates.
- **Pi risk:** none - registration-time code only, no per-frame cost.
- **Rebuild cost:** confined to one pack plus its example.

## Out of scope

- Growing `IMui` with submenu or widget surfaces.
- The same sweep in other packs (Phase 4 only inventories them).
- Any plotter behavior change (fit algorithm, rotate/align semantics).
