# TO_CHECK

Scratchpad for smells and follow-ups. Dated bullets; no archaeology.

## 2026-11-10 — named surfaces / ImGuiPanel

- [ ] Out-of-tree apps (KidVJ, PixelPlotter, product apps): bump **rigImGui** / **rigNodeEditor**; rename `IWindow` → `ImGuiPanel`; prefer `windowManager(ui)` / held pointers.
- [ ] Finish in-tree example sweep if any `IWindow` callers remain after this change.
- [ ] Peel toolkit-shaped methods off `IMui` onto `Mui` when next touched (dock/kerning/`drawContents` menus) — freeze only in this pass.
- [ ] Properties should follow ECS selection end-to-end so packs stop poking `PropertiesWindow*`.
- [ ] Patch bay ECS override without hard-coded `"Node Editor"` title lookup.
- [ ] ESP web honesty: document which `rig.ui` / property surfaces [examples/example-esp32](examples/example-esp32) already honors (README / port-map).
- [ ] Host submodule pointer bumps after pack pushes (`rigImGui`, then `rigNodeEditor`).
- [ ] `nodeEditorPrefs()` is a process-static for `MSettings` chrome — OK for session prefs, but smells like a singleton if ever used for scene/graph state; keep prefs on settings, graph on `CNodeGraph`.
- [x] Was a byte-identical copy of `README.md` before this pass (wrong file role) — replaced with this scratchpad.
- [ ] Pack submodule commits: push **rigImGui** (`IWindow` → `ImGuiPanel`, `asMui`/`windowManager`), then **rigNodeEditor** (helpers + SpawnPropRefOpts + shared catalog param drawer), then bump host pointers.
- [ ] In-tree examples migrated to `asMui`/`windowManager` (oscHost, glEditor, calendar, ufomat, example-stage); other pack examples still may say `ImGuiPanel` after bulk rename but keep raw `dynamic_cast` until touched.
