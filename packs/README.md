# RigKit Packs

Packs are separate **STATIC** libraries loaded from app `app.json` manifests via [`cmake/RigKitPacks.cmake`](../cmake/RigKitPacks.cmake).

## Naming

**One id.** Spoken name = folder under `packs/` = `pack.json` `"name"` = `app.json` dependency `"name"` = CMake target.

Say and write the same camelCase token (e.g. **rigImGui** → `packs/rigImGui/`). Do not invent a display title, snake_case folder, or a different class/repo name.

| Pattern | Role | Examples |
|---------|------|----------|
| `*Component` | **Data pack** — domain POD components | `rigComponent`, `rigPlotComponent`, `rigNodeComponent`, `rigPixelPlotComponent`, `rigColorspace` |
| `*Editor` / `*Edit` | Edit **capability** (ops and/or editor window) over existing data | `rigNodeEditor`, `rigVectorEditor`, `rigMeshEdit` |
| `*Ui` | ImGui **Kit / shell panels** over an engine (not the data) | `rigPlotterUi`, `rigPixelPlotterUi` |
| Short / verb name | Single-purpose code, I/O, present, or transport | `rigSvg`, `rigObj`, `rigSystems`, `rigRender3D`, `rigGrbl`, `rigOsc`, `rigCompositor` |
| Product engine name | Orchestration that mutates domain PODs | `rigPlotter`, `rigPixelPlotter` |

Rules:

1. Do **not** invent a `*Component` pack for every feature — grow **rigComponent** for generic PODs; new `*Component` only for a real domain.
2. Prefer `*Editor` over `*EditorUi`. Stacking both (`rigSvgEditorUi`) is historical; new shells: `rigFooEditor` or `rigFooUi`, not both.
3. `*Ui` means chrome on **rigImGui** over a separate engine/data pack — not where `C*` types live.
4. Mesh **data** stays in **rigComponent** (`CMesh`); mesh **tools** are **rigMeshEdit**. Same idea as `rigSvg` (IO) vs SVG shell UI.

## Data pack

A **data pack** owns portable entity meaning. It is **not** “only `.h` structs,” but it is **never** behavior systems or gui.

| Allowed | Forbidden |
|---------|-----------|
| `C*` POD components + `GetProperties()` | Update/Draw `registerSystem` |
| `registerComponent` in pack `setup()` | Dear ImGui / `IWindow` panels |
| Document codecs that serialize those PODs | GPU / window / socket handles in components |
| Pure helpers over POD (catalog tables, eval, flatten) | Second scene graph or manager trees |

Examples: **rigComponent**, **rigPlotComponent**, **rigNodeComponent** (PODs + catalog/eval helpers + `.rig` codecs), **rigPixelPlotComponent**. **rigProject** is the host project envelope + `.rig` document IO (data + codecs; document = portable `.rig`). Engines that *run* pipelines (`rigPlotter`, `rigPixelPlotter`) and editors/UI (`rigNodeEditor`, `rigPlotterUi`) are **code packs**.

See [docs/packs_using.md](../docs/packs_using.md) and [rigkit-data](../skills/rigkit-data/SKILL.md).

## Pinning

Each dependency uses a git **`ref`** (tag, branch, or commit SHA):

```json
{
  "name": "rigBlend2D",
  "url": "https://github.com/rigkid/rigBlend2D.git",
  "ref": "v0.1.0"
}
```

- **`ref`** is canonical. **`branch`** still works as an alias (CMake warns once).
- If `packs/<name>/` exists (submodule or local clone), that tree is used. Refresh with [`tools/update-packs`](../tools/update-packs.sh).
- If missing, CPM clones `url` at `ref`.

## In-org packs

**In-org** means first-party packs in the [rigkid](https://github.com/rigkid) GitHub org — each pack has its own remote.

**Host basics** (git submodules in this repo):

| Pack | Role |
|------|------|
| **rigComponent** | Generic POD data |
| **rigSystems** | Update/Draw systems |
| **rigProject** | Host project envelope + `.rig` document IO |
| **rigImGui** | Default **Rig + UI** fulfillment |

Everything else under `packs/` is optional: local clone or CPM at `app.json` `url` + `ref`. Survey [docs/packs_catalog.md](../docs/packs_catalog.md) before scaffolding a new pack.

**Plot family:** `packs/rigPlotter/examples/plot` remains the **integration** app (full Kit) when that pack is checked out. Every plot pack also ships its own thin example under `examples/<name>/` plus `img/preview.png`. Headless `tools/*_smoke` stay as unit gates.

Pin basics = submodule SHA in the host. Pin optional packs = `ref` in `app.json` (CPM) or a local checkout.

Refresh checkouts:

```bash
./tools/update-packs.sh
./tools/update-packs.sh --app examples/oscHost/app.json
```

## Pack example (README screenshot)

**Every** in-org pack ships **one** example under `examples/<name>/` — a tiny RigKit app that teaches how to use the pack. The window is the pack README screenshot (`examples/<name>/img/preview.png`). Lead the pack README with `![preview](examples/<name>/img/preview.png)`. End the pack README with the Pages link for that pack id:

```md
[API/docs](https://rigkid.github.io/<packName>/)
```

```
packs/<pack>/examples/<name>/
  CMakeLists.txt   # RIGKIT_DIR → host root; add_rigkit_application
  app.json
  app.h / app.cpp / main.cpp
  img/preview.png
  README.md
```

```bash
cmake -S packs/<pack>/examples/<name> -B packs/<pack>/examples/<name>/build
cmake --build packs/<pack>/examples/<name>/build --target <name>
```

Screenshot workflow: build and run the example, capture the main window, save as `examples/<name>/img/preview.png` (no emoji chrome), commit with the example. Host `examples/` stay Contract/product apps; they do **not** replace a pack example.

## Pack CI

Each in-org pack remote runs its own GitHub Actions workflow (`.github/workflows/ci.yml` in the pack repo): checkout the pack under test, clone the RigKit host + submodules, overlay the pack into `packs/<name>/`, then compile the pack example. Commit and push CI changes from the pack submodule (not only the host).

Private host/pack remotes need org (or per-repo) secret **`RIGKIT_CI_TOKEN`** — a fine-grained PAT with Contents:read on `rigkid/RigKit` and the in-org packs. Default `GITHUB_TOKEN` cannot clone sibling private repos. See [docs/contributing.md](../docs/contributing.md).

## API docs / GitHub Pages

Pages publish automatically on push to `main`:

| Who | Workflow | URL |
|-----|----------|-----|
| Host landing | [`.github/workflows/docs.yml`](../.github/workflows/docs.yml) | [https://rigkid.github.io/rigkit/](https://rigkid.github.io/rigkit/) ([`site/`](../site/)) |
| Host API | same workflow | [https://rigkid.github.io/rigkit/api/](https://rigkid.github.io/rigkit/api/) |
| Each pack | `.github/workflows/docs.yml` → reusable [`pack-docs.yml`](../.github/workflows/pack-docs.yml) | `https://rigkid.github.io/<packName>/` |

Local:

```bash
cmake -S . -B build && cmake --build build --target docs
./tools/generate-pack-docs.sh rigComponent
```

**Rollout order:** land host `docs.yml` + `pack-docs.yml` on `rigkid/RigKit` `main` first (pack callers use `@main`). Then commit/push each pack’s `docs.yml` and end its README with `[API/docs](https://rigkid.github.io/<packName>/)`. **One-time per remote:** Settings → Pages → Source: **GitHub Actions**. Private Pages need an org plan that allows them (or public remotes). When adding a new in-org pack, append it to [`docs/api/pack-remotes.txt`](../docs/api/pack-remotes.txt) so the host aggregate can fetch it in CI.

## New pack

**Before you scaffold:** read [docs/packs_catalog.md](../docs/packs_catalog.md) and this README’s Naming rules. Prefer growing an existing pack (especially **rigComponent** for generic PODs) over a new remote. Only create a pack when the seam is real — new domain data, new I/O, new editor/UI shell, or a leaf optional engine.

Then start from **[rigTemplate](../templates/rigTemplate/)** (`https://github.com/rigkid/rigTemplate.git`):

1. Lock the spoken name (one camelCase id). Copy `templates/rigTemplate` → `packs/<id>/` (or clone the template remote).
2. Rename `pack.json`, sources, class, and `PackRegistry` factory string to that same id. Set `license` in `pack.json` to `MIT Rigkid Contributors` (or `GPL-2.0-or-later Rigkid Contributors`; required by CI / `check-invariants`). Keep `description`, `url`, and `dependencies` accurate there — that file owns About / `IPack` identity and runtime init order ([docs/packs.md](../docs/packs.md)). The pack constructor is only `IPack("<id>")` — no `setDescription` / `addDependency`.
3. Add `url` + `ref` to the app `app.json` dependencies (`"name"` must match the id). Set SPDX `license` on the example `app.json` too (`MIT` is fine for apps).
4. Fill `examples/demo/` (example + `img/preview.png` for the README).
5. Publish the pack remote when ready (own repo push, or `./tools/publish-template.sh` for the template itself).

## Optional / external packs

Gitignored under `packs/*` except in-org packs and this README:

| Pack | Role |
|-------|------|
| **rigBlend2D** | Blend2D `IRenderer` fulfillment — [rigkid/rigBlend2D](https://github.com/rigkid/rigBlend2D) (`third_party/blend2d` + `asmjit` are **submodules**, not committed trees) |

```bash
git clone --recurse-submodules https://github.com/rigkid/rigBlend2D.git packs/rigBlend2D
# or CPM via app.json url + ref, then:
git -C packs/rigBlend2D submodule update --init --recursive
```

## Bootstrap order

Register **rigComponent** → **rigSystems** → **rigImGui** (see `examples/oscHost` and `examples/minimal`). For 3D meshes: **rigComponent** → **rigSystems** → **rigRender3D** (+ **rigObj** / **rigMeshEdit** as needed; see `examples/example-lowpoly`). For node graphs: **rigComponent** → **rigNodeComponent** → **rigImGui** → **rigNodeEditor** (see `packs/rigNodeEditor/examples/nodes`). Artist guide: [docs/nodes.md](../docs/nodes.md).

There is **no** runtime git updater in the host — only configure-time CPM and the tools above.
