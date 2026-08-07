---
name: rigkit-minimal
description: >-
  Prefer the laziest correct RigKit change. Use on coding, refactor, review, or
  design tasks to avoid over-engineering, extra dependencies, boilerplate, or
  jumping to systems/UI before data. Also when the user says yagni, do less,
  simplest, state churn, visibility map, m_last*, syncFromPrefs every frame,
  save/restore, one source of truth, duplicated default, or complains about
  bloat. Read it before designing any signature, callback type, or setter:
  it holds the no-pass-back rule (never take an argument the callee already
  reaches), which also fires on "it already has a reference", "no need to pass
  it again", "redundant parameter", "forwarding lambda", "why does it take
  MEcs", or "pass the engine in again". It holds the mirror no-set-then-use rule
  (per-call data is a parameter, not retained state), which fires on "state
  machine", "retained state", "setter then draw", "intent flag", "call order",
  "swappable renderer", or a field written but never read. Data rung before code.
---

# RigKit minimal

Lazy means efficient, not careless. Shortest working diff after understanding the problem.

Capability units are **packs** — never “addon”. Spoken name = folder id = `pack.json` / `app.json` `"name"` ([packs/README.md#naming](../../packs/README.md#naming)).

## Ladder

Stop at the first rung that holds:

1. **Does this need to exist?** Speculative need → skip; say so in one line.
2. **Can it be data on an existing or new plain component / schema?** Prefer fields + `GetProperties()` + `registerComponent` in **rigComponent** (or app data) over a new system or panel. ([rigkit-data](../rigkit-data/SKILL.md))
3. **Already in this codebase?** Reuse managers, components, pack patterns — look before writing. Survey the pack table before a new pack; grow an existing seam when the role fits.
4. **C++20 / stdlib?** Use it.
5. **Stand on shoulders?** Prefer an existing library over writing our own — only if same spirit (Pi-cheap, data-friendly, rebuild-cheap, artist-warm), does the job, and we can change it. Use deps already here (GLFW, EnTT, glm, nlohmann/json, spdlog) before adding new ones. Do not reinvent the wheel; do not invent Mars (wrong-spirit megastack). Do not add a dependency for a few lines.
6. **Pack vs core?** UI → `IMui` / `rigImGui` (UI in SUDE–ECS–UI), not Dear ImGui in `src/`. Optional graphics → existing packs (e.g. Blend2D), not new core weight.
7. **Pi-cheap?** Prefer the path that compiles on arm64/GLES and stays smooth on Raspberry Pi installs. Desktop-only convenience is not a rung.
8. **Rebuild-cheap?** Prefer the change that does not force a full-universe rebuild (narrow includes, pack-local edits, **bodies in `.cpp` not headers**). Slow prototype loops are a design bug. See [rigkit-build](../rigkit-build/SKILL.md) Fast rebuilds + Local CI step 0.
9. **Only then:** the minimum code that works — fewest files, boring over clever.

## Rules

- No unrequested abstractions (one-impl interfaces, factories for one product, config for a constant).
- Deletion over addition. Bug fix = root cause (shared function once), not a guard per caller.
- Question complex asks: ship the lazy version and note what was skipped.
- Do not widen data-layer impurity for convenience.

## State churn (gate, don’t mutate)

Before save/restore maps, hide-by-title scrubbing, `*Applied` / `*Ready` flags, or `syncX()` every frame, ask:

1. Can a **read-time gate** decide? (`if (!show) return;`)
2. Can the **source of truth** be read at use? (prefs / doc — no local mirror)
3. Can one **epoch or dirty counter** replace N `m_last*` fields?
4. Is initial visibility declared **at create**, not scrubbed by title strings after?
5. Is the state a **bool**, not display text? Never branch on `statusBar.left() == "Edit Mode"`.
6. Does the flag have **one owner**? See below.

Anti-example: Edit Mode hid all windows and restored a saved visibility map; tool apps scrubbed host chrome with `hideWindow("Log")` lists.

Correct: skip rendering when gated; create panels with the visibility they should have; never rewrite OS chrome (GLFW window title) from document state — app name stays on the window, document name lives in ImGui tabs.

## No pass-back (never hand over what the callee already reaches)

**A parameter must carry something the callee cannot already get.** If the callee can reach it — from the object it was registered on, from a member it stores, or from a parameter already in the signature — the parameter is noise: two names for one thing, a chance to pass a mismatched pair, and boilerplate at every call site.

Ask before adding a parameter: **can the callee already reach this?** Trace one hop of accessors. If yes, drop it.

Four shapes, all the same bug:

1. **The registrar hands itself back.** `MEcs::registerSystem` stores the call; `MEcs` binds itself once at registration, so a system takes only what it uses. Never make callers write a lambda whose only job is to forward the manager back.

```cpp
// Bad: ecs is the MEcs — the lambda exists only to adapt arity
ecs->registerSystem("SHierarchy", SystemPhase::Update,
					[](MEcs& e, float) { ecs::SHierarchy(e); });

// Good: pass the system straight in
ecs->registerSystem("SHierarchy", SystemPhase::Update, rigkit::ecs::SHierarchy);
```

The name stays: it is the entry's identity, so re-running a pack's `setup()` replaces instead of double-registering. A parameter that carries identity is not pass-back.

2. **A manager plus something reachable through it.** `SShapeRendering(MEcs&)` reads `MEcs::getPresentRenderer()`; taking an extra `IRenderer*` would make callers pull it off the ECS only to hand it back, and let a caller pass a renderer that is not the one presenting.

3. **A stored collaborator passed in again.** A method never takes what `m_*` already holds. `IApp::rigSetup()` takes nothing — the engine wired itself with `setEngine` before the call, same as `IPack::rigSetup()`. `ShaderPreviewWindow::tick(dt)` reads the ECS through `IWindow::getEngine()` rather than taking an `MEcs&`.

4. **A second pointer to one owner.** One owner, one pointer. `Canvas` keeps `m_engine` and reads `m_engine->getECSManager()`; a parallel `m_ecs` plus `setECSManager` would let the two disagree — null or stale on one side while the other is fine.

**Fine, not pass-back:** a hook argument that is the hook's own subject — `IApp::registerUIActions(SEvent&)` gives the override the bus it is there to fill, so no one writes a null-checked chain. A **transient per-pass context** with one writer, like `MEcs::setPresentRenderer` (host Draw / Canvas FBO), is state, not a duplicate owner. And a **system's own `MEcs&`** is its input: `void SHierarchy(MEcs&)` is a free function with no members to reach through, and no caller types it — `registerSystem` binds the world.

**Do not "init the system with the ECS" to drop that parameter.** A system that retains `m_ecs` is shape 4 again: it can dangle, or run against a different registry than the pass that invoked it, and it turns a function over data into a stateful service that contract smoke can no longer call with a scratch `MEcs`. Binding at dispatch (`entry.fn(dt)` closing over `*this`) makes the wrong world unrepresentable.

The line is **lifetime**: a context created and destroyed inside one pass is fine — collapse a long recursive parameter list into one, as `SHierarchy` does with a local walk state carrying `ecs` + visited sets. A member that survives the pass is not.

Fixing one: change the signature, delete the duplicate member and its setter, then sweep call sites (see [rigkit-build](../rigkit-build/SKILL.md) API change sweep). Do not leave a forwarding overload behind.

## No set-then-use (per-call data is a parameter, not stored state)

The mirror of no pass-back. **If a value varies per call, it belongs in that call's signature** — not in a setter the callee stashes for later. `setX()` then `use()` makes call order load-bearing, forces every implementer to hold a copy, and usually grows a hidden mode flag to reconcile the setters.

Ask before adding a setter: **does this value differ between two consecutive calls?** If yes, pass it.

Anti-example: `IRenderer` had `setFillColor` / `setStrokeColor` / `setStrokeWidth` then a bare `drawRect`. `OpenGLRenderer` reconciled them with a private `m_intent` that `setStrokeWidth` also flipped, so `Canvas::rect()` always stroked and never filled; `Canvas::noFill()` wrote a flag nothing read; style was mirrored in `Canvas`, `Graphics`, `IRenderer`, and `OpenGLRenderer`.

Correct: `Paint{mode, color, strokeWidth}` per draw — `drawRect(x, y, w, h, paint)`. No style state in the backend, so call order cannot change the result and a batching backend stays possible (Pi cost, not taste). Retained artist style — `fill()` once, then many shapes — lives in exactly **one** layer above (`Graphics`), which turns it into a Paint per call.

A stateless interface is also what makes a backend swappable: the seam is the abstract base plus POD arguments, never a shared style protocol two implementations must agree on.

Keep set-then-use only for genuine **pass state**: the matrix stack (`pushMatrix`), a per-pass context with one writer (`MEcs::setPresentRenderer`), or a resource bound for many calls.

## One owner (expose a reader, not a setter)

A **setter** on an interface forces every implementer to keep a copy, and the owner to push on every change plus on attach. Expose a **reader** and let the dependent side pull.

Smell: the same default literal written in more than one layer (engine field + impl field + interface `virtual` return). Three defaults = three sources of truth that drift apart silently.

Split state by lifetime, then give each half exactly one home:

- **Capability** — declared once in `setup()`, never toggles → engine / app settings.
- **Transient state** — changes while running → the manager that renders it.

Anti-example: `IMui::enableEditMode(bool)` made `RigKitEngine` push the flag into `Mui`, so both held a `bool` and `IMui` held a third default.

Correct: `RigKitEngine` owns `m_editModeEnabled` (capability); `Mui::editModeEnabled()` reads it through `m_engine`; `Mui` owns only `m_editMode` (transient). Pull also removes the ordering problem — no pending value, no apply-on-attach — as long as the read happens after `setup()`.

## Not lazy about

Understanding the real flow before climbing; trust-boundary validation; preventing data loss; security; **Pi compile + smooth runtime** (see `AGENTS.md` / pi-host); sensor calibration (hardware is not the ideal spec); anything the user explicitly requested.

## Output

Code first. Then at most a few short lines: what was skipped, when to add it.
