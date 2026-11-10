# RigKit Packs

Packs are separate **STATIC** libraries loaded from app `app.json` manifests via [`cmake/RigKitPacks.cmake`](../cmake/RigKitPacks.cmake).

## Naming

**One id.** Spoken name = folder under `packs/` = `pack.json` `"name"` = `app.json` dependency `"name"` = CMake target.

Say and write the same camelCase token (e.g. **rigImGui** is `packs/rigImGui/`). Do not invent a display title, snake_case folder, or a different class/repo name.

| Pattern | Role | Examples |
|---------|------|----------|
| `*Component` | **Data pack** - domain POD components | `rigComponent`, `rigPlotComponent`, `rigNodeComponent`, `rigPixelPlotComponent` |
| `*Edit` | CODE **ops** over existing POD. No dock, no product menus. Headless callers can link it. | `rigMeshEdit` |
| `*Ui` | IMui **chrome** - one panel or a Kit. Named after the engine or domain, never a file format. | `rigPlotterUi`, `rigTraceUi`, `rigVectorUi` |
| Short / verb name | Single-purpose code, I/O, present, or transport | `rigSvg`, `rigObj`, `rigSystems`, `rigRender3D`, `rigSerial`, `rigGrbl`, `rigOsc`, `rigCompositor` |
| Product engine name | Orchestration that mutates domain PODs | `rigPlotter`, `rigPixelPlotter` |

Rules:

1. Do **not** invent a `*Component` pack for every feature - grow **rigComponent** for generic PODs; new `*Component` only for a real domain.
2. The cut is **ops vs chrome**: `*Edit` is CODE; `*Ui` is IMui. Do not add new `*Editor` packs (historical ids stay: `rigNodeEditor`, `rigCodeEditor`, `rigVectorEditor`). Never stack `*EditorUi`.
3. `*Ui` means chrome on **rigImGui** / `IMui` over a separate engine or data pack - not where `C*` types live. Name it after that engine or domain, never a file format. Do not fold chrome into the engine under `gui/` (engines must stay linkable without `IMui`; one engine may have more than one Kit).
4. Mesh **data** stays in **rigComponent** (`CMesh`); mesh **tools** are **rigMeshEdit**. Same idea as `rigSvg` (IO) vs **rigVectorUi** (artboard Kit).
5. `*Shell` is not a suffix family. **rigDocumentShell** is a one-off product-host pack (Viewer / Player File Open, Pages, skipped keys). Do not grow `rigOscShell` / `rigPlotterShell`.
6. New data packs end in `Component` (`rigCadComponent`). Do not add a second suffix (`*Data`, `*Pod`). Three shipped data packs do not use the suffix and stay as they are: **rigProject** (host envelope + document IO), **rigColorspace** (color POD + pure helpers), **rigComposition** (composition analysis POD + pure helpers). Do not copy those ids for a new domain.

## Branch, trunk, leaf

Folders stay flat: `packs/<id>/`. The tree is `pack.json` `dependencies`, not nested directories (those break one id, CPM, and remotes).

[cmake/RigKitPacks.cmake](../cmake/RigKitPacks.cmake) loads nested deps depth-first. **The app lists the leaf; the leaf lists parents; the branch comes in.** List an ancestor in `app.json` only when you need the trunk without the leaf (serial monitor without GRBL).

| Word | Meaning |
|------|---------|
| **Trunk** | Shared parent (`rigSerial`, `rigDmx`, `rigAudio`, `rigPlotComponent`). Many leaves may depend on it. An app may depend on a trunk alone. |
| **Leaf** | App-facing tip. Other in-org packs do not depend on it. Heavy / GPL optionals (`rigAssimp`, `rigOpenCV`) are leaves too. |
| **Branch** | Trunk plus the leaves that hang on it. |

Create from the **leaf the app needs**. Extract a trunk when a second leaf would copy it. Catalog headings are drawings, not pack ids. Empty parent packs are fake stubs - the catalog may draw a branch before the trunk pack exists.

Name packs by meaning (medium / domain / protocol), not by routing geography. There is no `rigWan`. LAN vs WAN is not a seam.

I/O target tree: **rigSerial** (medium) and **rigDmx** (universe domain) are trunks. Protocol leaves: **rigGrbl** and **rigEnttec** on serial; **rigSacn**, **rigArtnet**, and **rigEnttec** on DMX. **rigOsc** and **rigNetScan** are siblings, not children of DMX. **rigEspnow** is a sibling of **rigOsc** and **rigSerial**, not a child of Wi-Fi. **rigNet** exists only after two packs would copy IP/interface helpers. See [docs/packs_catalog.md](../docs/packs_catalog.md).

## Data pack

A **data pack** owns portable entity meaning. It is **not** “only `.h` structs,” but it is **never** behavior systems or gui.

| Allowed | Forbidden |
|---------|-----------|
| `C*` POD components + `GetProperties()` | Update/Draw `registerSystem` |
| `registerComponent` in pack `setup()` | Dear ImGui / `ImGuiPanel` docks |
| Document codecs that serialize those PODs | GPU / window / socket handles in components |
| Pure helpers over POD (catalog tables, eval, flatten) | Second scene graph or manager trees |

Examples: **rigComponent**, **rigPlotComponent**, **rigNodeComponent** (PODs + catalog/eval helpers + `.rig` codecs), **rigPixelPlotComponent**, **rigColorspace**, **rigComposition**. **rigProject** is the host project envelope + `.rig` document IO (data + codecs; the document is portable Contract JSON, a `.rig` file or a `.rigz` ZIP when sidecars travel with it; this pack writes the JSON). Engines that *run* pipelines (`rigPlotter`, `rigPixelPlotter`) and editors/UI (`rigNodeEditor`, `rigPlotterUi`) are **code packs**. A data pack does not ship a host example.

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
- The **root manifest wins**: an `app.json` pin overrides the same pack's nested `pack.json` requests, and CMake says so when they differ. Conflicting nested requests with no root pin warn at configure — pin the pack in the app to settle them.
- If `packs/<name>/` exists (submodule or local clone), that tree is used, even when it does not match the pin (pack development). Configure warns when a local checkout drifts from a pinned tag or SHA.
- If missing, CPM clones `url` at `ref`.

Manage pins with [`tools/rigpack.py`](../tools/rigpack.py) (Python + git only; version and bump rules: [docs/versioning.md](../docs/versioning.md#pack-versions)):

```bash
python tools/rigpack.py status examples/oscHost/app.json   # resolved graph, checkout state
python tools/rigpack.py status                             # every local pack checkout
python tools/rigpack.py outdated <app.json>                # tracks vs remotes; floats say pull --sync
python tools/rigpack.py update <app.json> --latest --sync  # pull origin/main; keep "ref": "main"
python tools/rigpack.py update <app.json> --pin            # snapshot floats to SHAs / tags
python tools/rigpack.py update <app.json> --float          # unpin: write main
python tools/rigpack.py sync <app.json>                    # clone / check out at current refs
python tools/rigpack.py check <app.json>                   # conflicts + SemVer; floating refs are allowed
python tools/rigpack.py check <app.json> --pinned          # product release: floats fail
```

`"ref": "main"` is a track: `update` does not rewrite it to a SHA. `--pin` is the snapshot. `--float` goes the other way. `update` also works on a `pack.json` (preparing a pack release: bump nested pins, then bump `version`, commit, tag `v<version>`, push both).

## In-org packs

**In-org** means first-party packs in the [rigkid](https://github.com/rigkid) GitHub org - each pack has its own remote.

**Host basics** (git submodules in this repo):

| Pack | Role |
|------|------|
| **rigComponent** | Generic POD data |
| **rigSystems** | Update/Draw systems |
| **rigProject** | Host project envelope + `.rig` document IO |
| **rigImGui** | Default **Rig + UI** fulfillment. Dear ImGui is a nested submodule ([GitBruno/imgui](https://github.com/GitBruno/imgui) until [ocornut/imgui#9516](https://github.com/ocornut/imgui/pull/9516) merges); init with `--recursive`. |
| **rigImTui** | Alternate **Rig + UI** fulfillment - GPU character-grid compositor (local `packs/rigImTui` until published as its own remote). |

Everything else under `packs/` is optional: local clone or CPM at `app.json` `url` + `ref`. Survey [docs/packs_catalog.md](../docs/packs_catalog.md) before scaffolding a new pack.

**Plot family:** `packs/rigPlotter/examples/plot` remains the **integration** app (full Kit) when that pack is checked out. Plot code packs should ship a thin example under `examples/<name>/` (and `img/preview.png` when the window is the picture). **rigPlotComponent** does not need one; `examples/paths` may stay. Headless `tools/*_smoke` stay as unit gates.

Pin basics = submodule SHA in the host. Pin optional packs = `ref` in `app.json` (CPM) or a local checkout.

Refresh checkouts to an app's pins, or see what is checked out, dirty, or behind:

```bash
python tools/rigpack.py sync examples/oscHost/app.json
python tools/rigpack.py status examples/oscHost/app.json --strict   # non-zero if anything needs attention
python tools/rigpack.py status --build examples/svgEditor/build     # RIGKIT_DIR drift for a build tree
```

In-tree examples configure with `RIGKIT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/../..` so they always build against this checkout. Sibling product apps should point at the same tree (e.g. `../RigKit`) and run rigpack from that RigKit checkout (`--rigkit` defaults to the tool's own repo).

## Pack example (README screenshot)

A **code** pack should ship one example under `examples/<name>/` when there is a window, a mesh, or a transport worth teaching. That app is how someone learns the pack. When the window is the picture, save it as `examples/<name>/img/preview.png` and lead the pack README with `![preview](examples/<name>/img/preview.png)`. A headless pack (CLI, smoke, library with nothing to draw) may ship `tools/*_smoke` and skip the screenshot.

A **data** pack does not need an example. That is every `*Component` pack, plus **rigProject**, **rigColorspace**, and **rigComposition**. Show the POD from the code pack that fulfills it, or from a `.rig` document. An example already in a data pack may stay; do not add one to satisfy a checklist.

This is a preference, not a missing-example failure. `tools/check-invariants` fails only when a README embeds `preview.png` and the file is not there. Backlog: [docs/TODO.md](../docs/TODO.md).

End every pack README with the Pages link for that pack id:

```md
[API/docs](https://rigkid.github.io/<packName>/)
```

```
packs/<pack>/examples/<name>/
  CMakeLists.txt   # RIGKIT_DIR is the host root; add_rigkit_application
  app.json
  app.h / app.cpp / main.cpp
  img/preview.png
  README.md
```

```bash
cmake -S packs/<pack>/examples/<name> -B packs/<pack>/examples/<name>/build
cmake --build packs/<pack>/examples/<name>/build --target <name>
```

Screenshot workflow, when the pack has a window: build and run the example, capture the main window, save as `examples/<name>/img/preview.png` (no emoji chrome), commit with the example. Host `examples/` stay Contract/product apps; they do not replace a pack example.

## Style

Host `tools/format.*` never touches `packs/`. Each pack formats its own first-party `src/` + `examples/` with clang-format (`SortIncludes` - [docs/includes.md](../docs/includes.md)).

From the host, once per clone:

```bash
./tools/install-hooks.sh          # Unix
tools\install-hooks.bat           # Windows
```

That installs the host pre-commit hook and copies the pack hook into every `packs/<name>` that is its own git repo. New packs get `tools/format.*` + `tools/install-hooks.*` from [rigTemplate](../templates/rigTemplate/). Never format `third_party/`.

## Pack CI

Each in-org pack remote runs its own GitHub Actions workflow (`.github/workflows/ci.yml` in the pack repo): checkout the pack under test, clone the RigKit host + submodules, overlay the pack into `packs/<name>/`, then compile the pack example when it has one. A data pack with no example still compiles the static library in that workflow. Commit and push CI changes from the pack submodule (not only the host).

Private host/pack remotes need org (or per-repo) secret **`RIGKIT_CI_TOKEN`** - a fine-grained PAT with Contents:read on `rigkid/RigKit` and the in-org packs. Default `GITHUB_TOKEN` cannot clone sibling private repos. See [docs/contributing.md](../docs/contributing.md).

## API docs / GitHub Pages

Pages publish automatically on push to `main`:

| Who | Workflow | URL |
|-----|----------|-----|
| Host landing | [`.github/workflows/docs.yml`](../.github/workflows/docs.yml) | [https://rigkid.github.io/rigkit/](https://rigkid.github.io/rigkit/) ([`site/`](../site/)) |
| Host API | same workflow | [https://rigkid.github.io/rigkit/api/](https://rigkid.github.io/rigkit/api/) |
| Each pack | `.github/workflows/docs.yml` calls reusable [`pack-docs.yml`](../.github/workflows/pack-docs.yml) | `https://rigkid.github.io/<packName>/` |

Local:

```bash
cmake -S . -B build && cmake --build build --target docs
./tools/generate-pack-docs.sh rigComponent
```

**Rollout order:** land host `docs.yml` + `pack-docs.yml` on `rigkid/RigKit` `main` first (pack callers use `@main`). Then commit/push each pack’s `docs.yml` and end its README with `[API/docs](https://rigkid.github.io/<packName>/)`. **One-time per remote:** Settings > Pages > Source: **GitHub Actions**. Private Pages need an org plan that allows them (or public remotes). When adding a new in-org pack, append it to [`docs/api/pack-remotes.txt`](../docs/api/pack-remotes.txt) so the host aggregate can fetch it in CI.

## New pack

**Before you scaffold:** read [docs/packs_catalog.md](../docs/packs_catalog.md) and this README’s Naming + Branch rules. Prefer growing an existing pack (especially **rigComponent** for generic PODs) over a new remote. Only create a pack when the seam is real - new domain data, new I/O, new editor/UI shell, or a leaf optional engine. Start at the leaf; extract a trunk when a second leaf would copy it.

Then start from **[rigTemplate](../templates/rigTemplate/)** (`https://github.com/rigkid/rigTemplate.git`):

1. Lock the spoken name (one camelCase id). Copy `templates/rigTemplate` to `packs/<id>/` (or clone the template remote).
2. Rename `pack.json`, sources, class, and `PackRegistry` factory string to that same id. Set `license` in `pack.json` to `MIT Rigkid Contributors` (or `GPL-2.0-or-later Rigkid Contributors`; required by CI / `check-invariants`). Keep `description`, `url`, and `dependencies` accurate there - that file owns About / `IPack` identity and runtime init order ([docs/packs.md](../docs/packs.md)). The pack constructor is only `IPack("<id>")` - no `setDescription` / `addDependency`.
3. Add `url` + `ref` to the app `app.json` dependencies (`"name"` must match the id).
4. Code packs: fill `examples/demo/`. Set SPDX `license` on that `app.json` (`MIT` is fine for apps). Add `img/preview.png` when the window is the README picture. Data packs: delete `examples/`. They do not need a demo or a screenshot.
5. Publish the pack remote when ready (own repo push, or `./tools/publish-template.sh` for the template itself).

## Optional / external packs

Gitignored under `packs/*` except the four host-basic submodules (`rigComponent`, `rigSystems`, `rigProject`, `rigImGui`) and this README. Any other checkout stays a local clone or a CPM fetch at `app.json` `url` + `ref`.

| Pack | Role |
|-------|------|
| **rigBlend2D** | Blend2D `IRenderer` fulfillment - [rigkid/rigBlend2D](https://github.com/rigkid/rigBlend2D) (`third_party/blend2d` + `asmjit` are **submodules**, not committed trees) |

```bash
git clone --recurse-submodules https://github.com/rigkid/rigBlend2D.git packs/rigBlend2D
# or CPM via app.json url + ref, then:
git -C packs/rigBlend2D submodule update --init --recursive
```

## Bootstrap order

Register **rigComponent**, then **rigSystems**, then **rigImGui** (see `examples/oscHost` and `examples/minimal`). For 3D meshes: **rigComponent**, then **rigSystems**, then **rigRender3D** (+ **rigObj** / **rigMeshEdit** as needed; see `examples/lowpoly`). For node graphs: **rigComponent**, then **rigNodeComponent**, then **rigNode**, then **rigImGui**, then **rigNodeEditor** (see `packs/rigNodeEditor/examples/nodes`). For a preset deck: **rigComponent**, then **rigShowComponent**, then **rigProject**, then **rigSystems** (`examples/visualizer` loads `.rig` presets). Artist guide: [docs/nodes.md](../docs/nodes.md).

There is **no** runtime git updater in the host - only configure-time CPM and the tools above.
