# TO_CHECK

Scratchpad for smells and follow-ups. Dated bullets; no archaeology.

## 2026-11-10 — named surfaces / ImGuiPanel

- [ ] Out-of-tree apps (KidVJ, PixelPlotter, product apps): bump **rigImGui** / **rigNodeEditor**; rename `IWindow` → `ImGuiPanel`; prefer `windowManager(ui)` / held pointers. Recompile against menu `register*Action(..., group)` (default empty).
- [x] In-tree `IWindow` callers gone (only a third_party comment left).
- [x] Menu submenus are hooks: `register*Action` + optional `group`; no `register*Submenu(drawContents)`.
- [x] Chrome kerning peeled off `IMui` onto `Mui` (`setChromeKerning` / `setChromeKernFn` / pair count).
- [ ] Preferences drawers without ImGui `draw` callbacks — already on `Mui` (not `IMui`); **zero in-tree callers**. Redesign to POD / `rig.ui.*` when a real prefs page needs data binding; do not invent a parallel system first.
- [x] Properties follows `CSelection` end-to-end; Scene / Node Editor no longer poke `PropertiesWindow*`.
- [x] Patch bay resolves ECS override by scanning `NodeEditorWindow` instances (not the `"Node Editor"` title).
- [x] ESP web honesty: [examples/example-esp32/README.md](examples/example-esp32/README.md) + [docs/contract/port-map.md](docs/contract/port-map.md) — browser honors `rig.ui.panel` / `control` / `action`; no `IMui`; `group` not yet.
- [ ] Host submodule pointer bumps after pack pushes (`rigImGui`, then `rigNodeEditor`).
- [x] `nodeEditorPrefs()` process-static: OK for `MSettings` chrome; keep graph/scene state on `CNodeGraph` (do not grow this into a scene singleton).
- [x] Was a byte-identical copy of `README.md` before this pass (wrong file role) — replaced with this scratchpad.
- [ ] Pack submodule commits: push **rigImGui** (`IWindow` → `ImGuiPanel`, `asMui`/`windowManager`, Properties←`CSelection`, menu hooks, kerning on `Mui`), then **rigNodeEditor** (helpers + SpawnPropRefOpts + shared catalog param drawer + patch-bay type scan), then bump host pointers.
- [x] In-tree examples on `asMui`/`windowManager` (oscHost, glEditor, calendar, ufomat, example-stage); rigImGui internals use `asMui` too. Other pack examples may still raw-cast until touched.
