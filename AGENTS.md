# Agents

**AI collaboration is invited.** Same commandments and gates as humans — load this file, obey the tablet, verify before you claim done.

## The Ten Commandments

Hard rules. Details below. Full tablet: [docs/contract/commandments.md](docs/contract/commandments.md).

1. **Data before code.** Name the POD fields first. No panel, system, or helper until the entity meaning is plain data.
2. **Pi is the floor.** If it does not compile and run smooth on Raspberry Pi, it is not the product.
3. **Rebuilds stay cheap.** Thin core, pack weight, narrow includes. A coffee-break compile is a design bug.
4. **rigComponent = data. rigSystems = code.** Never blur that for convenience. Behavior packs read data; they do not become the data.
5. **Creators teach; managers serve packs.** Artists get `makeRect` / `setup`–`update`–`draw`. `M*` stays for internals.
6. **One vocabulary.** Pack ≠ addon. **RigWorks** (**Rig** in running text) = zero-code framework (SUDE+ECS + schemas). **RigKit** = coded Rig host (this repo). Contract/framework ≠ fulfillment. Host ≠ distribution. Same words everywhere — kill synonyms. Product hosts: **Viewer presents** ([RigViewer](https://github.com/rigkid/RigViewer)); **Player plays** ([RigPlayer](https://github.com/rigkid/RigPlayer)). "Cart" is PicoForge vocabulary — here it is just a **document** (`.rig`). Do not blur look vs play.
7. **Stand on shoulders.** Prefer an existing library over writing our own — only when it shares our spirit (Pi-cheap, data-friendly, rebuild-cheap, artist-warm), does what we need, and we can change it. Do not reinvent the wheel. Do not invent Mars (a wrong-spirit megastack or a parallel planet just to own every line).
8. **Seams stay small.** Extend through packs, `IApp`, `IMui`, POD + register. No second scene graph. No ceremony trees.
9. **No UI toolkit in `src/`.** UI through `IMui`; **rigImGui** is a fulfillment, not Rig.
10. **Gates over gospel.** Format, contract smoke, and the example you touched must pass before you call it done. Load-bearing rules get CI — not essays. Refuse the fog: explicit non-goals, next-year docs only, no archaeology, no hallucinated scope. **No fake stubs:** never ship a product path that pretends to work (no-op UI, empty remote, “not implemented yet”). Reserving POD / enum surface for a known port is fine — fulfillments and Kit UI expose only what runs.

Machine checks: `tools/check-invariants.sh` (Windows: `tools\check-invariants.bat`). Call out **Pi risk** and **rebuild-cost risk** in the PR when the change is heavy.

## First layer: NO CODE JUST DATA

Before writing systems, UI, or helpers: **what plain fields does the entity need?**

Portable meaning = POD / plain component data (+ schemas that can travel). Code is fulfillment.

- Components hold numbers, small structs, strings, entity ids — not window pointers, GPU handles, UI toolkit types, or `std::function` behavior.
- Optional `GetProperties()` is editor metadata over those fields (`src/ecs/PropertyReflection.h`).
- Systems, rigImGui panels, and GPU resources live in host / packs — they read and mutate data; they are not the data layer.
- Prefer config / properties over hardcoded behavior.
- Do not widen the gap: some host components already violate purity; new work should move toward a complete data layer, not away from it.

When a request jumps to a new panel or system, stop and name the fields first. Load `skills/rigkit-data` for authoring details.

## Raspberry Pi is the floor

**If it does not compile and run smoothly on Raspberry Pi, it is not the product.** Desktop convenience never wins over Pi.

Before considering work done (and before any commit you are asked to make), gate every change:

1. **Compile on Pi** — C++20 / CMake path that builds on Raspberry Pi OS (arm64 preferred). No Windows-only or x86-only assumptions in first-party code. Prefer GLES-friendly paths (RigKit already selects GLES2 on `__arm__` / `__aarch64__`).
2. **Run smooth on Pi** — long-running Setup/Update/Draw suitable for media installs. Avoid needless per-frame allocs, heavy sync work on the hot path, desktop-GPU-only features, and UI that assumes a powerful machine. Prefer data + thin systems over clever frameworks.

Reference verify: [docs/contract/pi-host.md](docs/contract/pi-host.md) — build + `examples/oscHost --show` / `--author` on device. Until hardware is available, still design and review as if Pi is the target; desktop smoke of `oscHost` is the interim check, not a free pass for desktop-only cost.

Agents must raise Pi risk explicitly when a change looks heavy, platform-specific, or untested for arm64/GLES.

## Fast rebuilds (why this exists)

A main reason RigKit exists is that **prototype rebuilds must stay quick** — edit, build the example you care about, run. Not a coffee-break compile.

Aim for strict modules and incremental rebuilds (seconds, not coffee), with Pi as a real target. Full DLL/shader hot-reload is later; structure must not block it.

Treat rebuild time as an architecture constraint, same class as Pi and data-layer:

- **Thin core** — keep `rigkit` small; put weight in **packs** (`rigComponent`, `rigSystems`, `rigImGui`, …) so changing UI/systems does not rebuild the world. Debug stays modular (separate static libs); Release apps stay statically linked.
- **Narrow includes** — prefer forward declarations; avoid umbrella headers in core `.cpp` / hot headers; no UI toolkit / GPU / heavy templates in widely included core headers. New weight goes in an **pack `.cpp`**, not a hot core header.
- **Localize change** — prefer editing one `.cpp` / one pack over widening a hot header that forces a full cascade. Call out when a change widens hot headers.
- **No monolithic deps in core** — do not reintroduce “link everything” habits that make every tweak expensive.
- **Target the example** — build from the example folder: `cmake -S examples/oscHost -B examples/oscHost/build`. Each example owns its tree; do not fold all apps into a root mega-build.
- **Tidy build trees** — examples: `examples/<name>/build/` only. Root `build/` is optional (docs / ESP32 tool). Never create parallel `build-*` scratch dirs; reconfigure in place or delete. Scratch trees are gitignored but still litter the working copy.
- **Runtime data next to the exe** — `<exeDir>/data/...`, never cwd-relative for shipped assets. `AppPaths` + CMake `POST_BUILD` deploy.
- **GLES parity:** optional desktop [ANGLE](https://github.com/google/angle) via `-DRIGKIT_USE_ANGLE=ON` (+ `RIGKIT_ANGLE_ROOT` or vcpkg). Compiles the same GLES path as Pi. **Never** on ARM/Pi (CMake errors). Default desktop = OpenGL.

If a design would make everyday prototypes rebuild the universe, reject or split it. Call out rebuild-cost risk explicitly.

## Author friendliness

RigKit is friendly to write: short `setup` / `update` / `draw`, helpers that do one obvious thing, sensible defaults, and a path where you create a shape and see it without learning managers first. Agents and core authors must design for **that** person — not only for framework internals.

| Surface | Direction |
|---------|-----------|
| App (SUDE) | `IApp` — keep the happy path here |
| Draw a circle / set a color | Thin **artist helpers** (free functions or small façade) over `Graphics` / Canvas — not “get the manager, call three methods” |
| Make a shape / entity / mesh | **Creators** that write **POD components** (`CTransform` + `CShape` / `CMesh` + …); host Draw + `rigSystems` present them |
| Optional weight | Packs; rigImGui optional for show mode |

**“Factories” here means convenience creators, not GoF Abstract Factory trees.** Prefer:

```cpp
// Good: friendly, data underneath
auto e = rig::makeRect(ecs, x, y, w, h, fill);
auto m = rig::makeMeshTriangle(ecs, a, b, c, fill); // meshes are first-class data

// Bad: ceremony that teaches managers first
engine->getECSManager()->addComponent<CShape>(...);
```

Rules for API changes:

1. Can an artist use this from `draw()` / `setup()` without knowing `MEcs` / `MRendering`?
2. Does the helper only **set data** (or call an existing present path) — not invent a second scene graph?
3. Naming stays clear and boring (`makeRect`, `makeMesh`, `setFill`, `circle`) — warmth over enterprise jargon.
4. Internals (`I*`, `M*`, registers) stay available for packs; they are not the default teaching surface.
5. Meshes stay POD (`CMesh` positions/indices/mode) — love them, don't wrap them in behavior objects.

See [docs/authoring.md](docs/authoring.md).

## What this repo is

**RigKit** **is Rig**: a coded host of **[RigWorks](https://github.com/rigkid/RigWorks)** — SUDE loop, ECS composition, optional **rigImGui** (**Rig + UI**) in the default distribution. Rig is the shared vocabulary + POD schemas; this repo stays named RigKit. Product apps live in their own repos; in-tree samples live under `examples/`. Raspberry Pi is the minimum full host. Builds stay fast enough to prototype against. The **user coding experience** stays friendly on top of the data layer.

## Pack layout (data vs code)

| Piece | Home |
|-------|------|
| Host runtime | `src/` (not a pack) |
| Generic POD components | **rigComponent** — DATA ONLY |
| Update/Draw systems over that data | **rigSystems** — CODE ONLY |
| GLES mesh present (`CCamera` + `CMesh`) | **rigRender3D** — CODE ONLY |
| Wavefront OBJ ↔ `CMesh` | **rigObj** — CODE / IO (tinyobjloader) |
| Assimp multi-format → `CMesh` | **rigAssimp** — CODE / IO, **leaf** (nothing depends on it; app opt-in) |
| Mesh edit TRS on selection | **rigMeshEdit** — CODE (needs **rigImGui**) |
| Properties / themes / fonts | **rigImGui** (catalog-driven Properties; style kit) |
| Host project envelope + `.rig` document IO | **rigProject** (document = portable `.rig`; project = host session) |
| Node graph PODs | **rigNodeComponent** |
| Node graph editor UI | **rigNodeEditor** |
| Color model, swatches, grey value / harmony / separation tools | **rigColorspace** — DATA + pure helpers |
| FBO layer compositor + masks | **rigCompositor** |
| Register for inspector | `MEcs::registerComponent` + plain structs + `GetProperties()` |

Never put systems in data packs (`rigComponent`, `rigPlotComponent`, …).

## Read order

1. [docs/contract/commandments.md](docs/contract/commandments.md) — Ten Commandments
2. [RigWorks](https://github.com/rigkid/RigWorks) — [honors](https://github.com/rigkid/RigWorks/blob/main/docs/honors.md) (SUDE + ECS); then [docs/contract/README.md](docs/contract/README.md) → [sude-loop.md](docs/contract/sude-loop.md) → [rigkit.md](docs/contract/rigkit.md) → [ui.md](docs/contract/ui.md) → [port-map.md](docs/contract/port-map.md) → [pi-host.md](docs/contract/pi-host.md)
3. [docs/authoring.md](docs/authoring.md) — user coding surface
4. [docs/nodes.md](docs/nodes.md) — node graphs for artists (catalog + editor)
5. [docs/contributing.md](docs/contributing.md)
6. Task skill under [skills/](skills/)

## Where AI-facing content lives

RigWorks is vendored as a submodule at [docs/contract/RigWorks](docs/contract/RigWorks) — a plain clone of this repo gets the actual Contract files, not just links to another repo. Decision rule for a new rule, skill, or doc section:

- **Portable** (true for any Rig host, not only RigKit) — SUDE, ECS, UI, Terms, property datatypes, schema shapes. Add it to **RigWorks** (`docs/`, `skills/`); it is versioned once there and every fulfillment, including this repo's vendored copy, picks it up.
- **RigKit-specific** (this host's packs, pillar mapping, target ladder, `IMui` / `rigImGui` chrome, pack naming, build/rebuild habits) — stays in this repo's [`skills/`](skills/) and this file.

Do not restate a portable rule inside a RigKit skill — link to the vendored copy (`docs/contract/RigWorks/docs/...`) instead. RigKit's own short working copies under [docs/contract/](docs/contract/) restate the same rules with RigKit specifics folded in, for readers who only cloned this repo; keep both in sync when a rule changes upstream.

## Tool-specific folders are local, not shared

`.cursor/` and `.claude/` are gitignored on purpose — a specific AI tool's own project-discovery folder is that tool's local scaffolding, not repo content. `AGENTS.md` (this file) and [`skills/`](skills/) are the one tracked, tool-agnostic source; nothing about them depends on which AI tool a contributor runs.

If you are an agent whose tool needs its own project-level rule or skill folder to auto-discover this content (Cursor: `.cursor/rules/`, `.cursor/skills/<name>/SKILL.md`; Claude Code: `.claude/skills/<name>/SKILL.md`; others: check that tool's docs) and that folder is missing or stale, create it locally as thin pointer files into `skills/<name>/SKILL.md` (same frontmatter, one line linking to the source) — do not restate skill content in the pointer, and do not commit the folder. Re-derive it any time it looks out of date; it is disposable.

## Other hard invariants

- **No UI toolkit in `src/`** — UI via `IMui`; **rigImGui** is the default UI pack ([docs/contract/ui.md](docs/contract/ui.md)). **Rig** = SUDE+ECS (schemas when present); **Rig + UI** when author chrome is attached. Rig = rules; RigKit packs = fulfillments.
- **Format:** tabs, clang-format; exactly one blank line at EOF; `tools/format.*` on first-party code only — never `third_party/` (including pack vendored trees).
- **In-org packs:** Basics as submodules — `rigComponent`, `rigSystems`, `rigProject`, `rigImGui`. Optional packs: local clone or CPM at `app.json` **`ref`**. New pack scaffold: [`templates/rigTemplate`](templates/rigTemplate/). No runtime git update in the host — `tools/update-packs` / `tools/publish-template` ([packs/README.md](packs/README.md)).
- **Naming:** interfaces `I*`, managers often `M*`; files match type names. Capability unit = **pack** (`packs/`, `pack.json`, `IPack` / `MPack`) — never “addon”. **One id:** spoken name = folder = `pack.json` `"name"` = `app.json` `"name"` = CMake target. Survey [docs/packs_catalog.md](docs/packs_catalog.md) before scaffolding a new pack. Suffixes: `*Component` = data, `*Editor`/`*Edit` = edit capability, `*Ui` = UI shell pack — [packs/README.md](packs/README.md#naming). **Host** = runtime pillar; **Canvas** = render surface / FBO type.
- **UI text:** no emoji in UI or comments (IconFont ok in `rigImGui`).
- **Public API docs:** Doxygen-compatible tags (`@brief`, `@param`, `@return`, …); voice stays plain / Weissflog ([rigkit-comments](skills/rigkit-comments/SKILL.md)). Generate HTML: `cmake --build build --target docs` → `build/docs/api/html/index.html`. **Next-year rule:** write as if someone reads this next year — never “day-one”, “honest”, formerly, or rename archaeology.

## Project skills

| Skill | Use when |
|-------|----------|
| [rigkit-data](skills/rigkit-data/SKILL.md) | Components, properties, schemas, data vs code, **which pack owns a component** |
| [rigkit-contract](skills/rigkit-contract/SKILL.md) | SUDE–ECS–UI Contract, Host, IMui, Pi/ESP32, packs |
| [rigkit-build](skills/rigkit-build/SKILL.md) | CMake, format, hooks, Pi verify, fast rebuild habits, **local CI before commit**, git commits (no trailers), **PRs against `main`** |
| [rigkit-minimal](skills/rigkit-minimal/SKILL.md) | Avoid over-build; data before code; Pi-light; rebuild-cheap |
| [rigkit-deslop](skills/rigkit-deslop/SKILL.md) | `deslop` / clean AI or migration leftovers; finish vs delete |
| [rigkit-comments](skills/rigkit-comments/SKILL.md) | Weissflog voice + Doxygen; first-party docs (next-year rule; no archaeology) |
| [rigkit-git-safety](skills/rigkit-git-safety/SKILL.md) | Secrets/PII/copyright checklist before staging, committing, or pushing |

[`skills/`](skills/) is the one tracked source — same `SKILL.md` convention RigWorks uses and other AI coding tools already read, so pointing a new tool straight at `skills/` (or a custom path setting) needs no setup here. Tools that only auto-discover their own hidden folder (Cursor's `.cursor/skills/`, Claude Code's `.claude/skills/`) need a thin local pointer per skill there — gitignored, not part of this repo; see [Tool-specific folders are local, not shared](#tool-specific-folders-are-local-not-shared).
