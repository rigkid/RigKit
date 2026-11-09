# Host seams

How this coded host stays open to apps that do not exist yet.

**RigWorks** is the portable agreement: schema ids, POD, SUDE / ECS / UI rules. Packs interoperate through those ids, not through a shared C++ API ([terms.md](RigWorks/docs/terms.md)).

**RigKit `src/`** is the reference live host. It has a **floor** (concrete runtime) and **seams** (`I*` plus register). Apps and packs are fulfillment. The host can finish; packs do not.

Unknown apps are new documents and packs. They register POD, systems, and named surfaces. They do not get a new vtable in `src/`.

## Floor vs seam

| Kind | What | Swap? |
|------|------|--------|
| **Floor** | `RigKitEngine`, `MEcs`, `MPack`, `MRendering`, `MCanvas`, `MSettings`, GLFW window, default `OpenGLRenderer` | No. EnTT vs ESP POD is a compile target, not a virtual base. |
| **Seam** | `IApp`, `IPack`, `IMui`, `IRenderer`, `ISettings` | Yes, or more than one fulfillment exists / is expected. |
| **Register** | `MEcs::registerComponent` / `registerSystem`, `RendererRegistry`, `project::addSerializer`, `registerUiChrome` | How packs add behavior without a virtual forest. |

`M*` stays (managers only). Do not add `IEngine`, `IEcs`, `IPackManager`, `IRendering`, `ICanvas`, `IInput`, or `IWindowing`. Wrapping the floor freezes it: the next real change becomes a breaking interface change.

**Host `I*` live in RigKit `src/`.** Pack fulfillment types do not wear a host `I*` badge. `ImGuiPanel` (dock panel base) and `MWindow` (dock registry manager) live in **rigImGui** — not host seams. `GLFWwindow*` on the engine is this host's OS window.

## Test for a new `I*`

Add a host interface only when **something else implements it, or a second fulfillment is a known next pack** (another chrome, another present backend). One implementor and no second in sight: keep it on the concrete type (`Mui`, `OpenGLRenderer`, a trunk pack).

Trunk-pack seams (`ISynth` on **rigAudio**, `ISimBackend` on **rigPhysics**) stay on the trunk pack. They are fulfillment talking to fulfillment.

## Seams

| Type | Role | Does not |
|------|------|----------|
| [`IApp`](../../src/core/IApp.h) | App SUDE hooks. Host owns `rigSetup` / `rigUpdate` / `rigDraw` / `rigExit`. | Own the registry or the window. |
| [`IPack`](../../src/core/pack/IPack.h) | Pack lifecycle + `onUiAttached`. Identity from `pack.json`. | Hold systems or portable components. |
| [`IMui`](../../src/core/IMui.h) | Optional author chrome. Detach for show / headless. | Include a UI toolkit. Own dock widgets. |
| [`IRenderer`](../../src/rendering/IRenderer.h) | 2D present. Default fulfillment is host OpenGL. **rigBlend2D** is the optional second. | Create a window or swap buffers. |
| [`ISettings`](../../src/core/ISettings.h) | JSON get/set blobs on engine, packs, managers. | Be the user-preferences registry (`MSettings` is). |
| [`ITextBackend`](../../src/rendering/ITextBackend.h) | Optional filled text for a present backend. | Outline stroking. |

`IApp` / `IPack` holding `RigKitEngine*` is the floor pointer, not a missing `IEngine`.

## `IMui` stays thin (named surfaces, no panel factory)

Keep on `IMui` what every chrome must honor: `init` / `shutdown` / `handleInput` / `render`, `chromeId`, **named surface** visibility (show / hide / list by name), file and folder dialogs (path in, path out), named menu **actions**, shortcuts, recent files, named workspaces, edit mode, undo bind, progress.

That named-surface API is the portable “window” agreement on the host. Do **not** add `getWindowManager`, `createWindow`, or typed dock panels to `IMui` — that moves ImGui bloat onto the seam every chrome (TUI, ESP web) would have to fake.

Keep on **`Mui`** (rigImGui) what assumes Dear ImGui: `register*Submenu` `drawContents` callbacks, tool-rail icon / group, dock passthrough and central-node rect, chrome kerning, `addHostPanel`, `setDockLayoutBuilder`, notifications, `ImGuiPanel` / `MWindow`, viewport overlays. Packs that need typed panels already link **rigImGui** and use `windowManager(ui)` / held `shared_ptr`s.

Some toolkit-shaped methods still sit on `IMui`. New work does not add more. When you touch one of those methods, move it to `Mui` in the same change.

ESP author UI is **web** over the same POD / document ([example-esp32](../../examples/example-esp32)); it does not implement `IMui` or `ImGuiPanel`. `#ifdef RIGKIT_MCU` may strip desktop weight — it does not license a dock factory in `src/`.

## Present

`IRenderer` is the swappable Draw backend. Font packs attach through that seam (`ITextBackend`), not through `OpenGLRenderer*`. Install the backend on `IRenderer` (default no-op) or on `MRendering` so a second present does not force a font-pack rewrite. `Graphics` is the artist façade over `IRenderer`, not a second backend.

## Unknown apps attach as data

A later tool is a document plus a pack, not a new host interface.

1. Name the POD fields. Register the component. [rigkit-data](../../skills/rigkit-data/SKILL.md).
2. Portable tool chrome is [`rig.ui.panel`](RigWorks/schemas/ui/panel.md) / `group` / `control` / `action` - controls bind `target` + `propertyKey`. Layout docks are fulfillment ([Rig UI](RigWorks/docs/ui.md)).
3. `IMui` shows and hides **named surfaces**. Properties already draws `GetProperties()` / `sProp`.
4. Custom `ImGuiPanel` subclasses (rigImGui only) are the exception for chrome that cannot be a bound field.

`CUiPanel` / `CUiGroup` / `CUiControl` / `CUiAction` live in **rigComponent**. **rigImGui** presents visible panels (`presentUiPanels`). Actions fire `SEvent` ids from `IApp::registerUIActions`. Unknown action ids stay hidden. Do not invent a parallel panel `I*` in `src/`. `examples/oscHost --author` ships a Look panel on the `look` entity.

## Later app

| App | Uses | Does not need |
|-----|------|----------------|
| Show / install | SUDE + `IRenderer`; no `IMui` | Dock API, `ImGuiPanel` |
| New author tool | `IApp` + POD + `IMui` actions / named surfaces | `Mui` / `ImGuiPanel` unless it only needs names |
| New chrome | Same `IMui` named surfaces + `rig.ui.*` | `ImGui::MenuItem` inside a host callback; fake `MWindow` |
| New present / font | `IRenderer` + `ITextBackend` | `OpenGLRenderer.h` |
| Domain not named yet | New `C*` in a data pack + register | A new host interface |

A different windowing stack is a different host that still speaks Rig documents. It does not belong as an `I*` under this GLFW floor.

See [commandments.md](commandments.md) 1, 8, 9; [ui.md](ui.md); [authoring.md](../authoring.md).
