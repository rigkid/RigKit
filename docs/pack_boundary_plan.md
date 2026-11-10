# Pack boundary plan

Status: decisions agreed. Implementation has not started.

This plan repairs pack ownership so the data layer, code packs, and document IO line up with the Ten Commandments. It exists because pack CI exposed a real boundary fault: `rigProject` cannot build in a basics-only host tree, and several packs have grown across the data/code seam in smaller ways.

Rules this plan serves: [contract/commandments.md](contract/commandments.md), [packs_using.md](packs_using.md), and the `rigkit-data` skill. When this plan and those documents disagree during implementation, stop and reconcile the documents in the same change. Do not leave two policies in print.

## Goal

- Data packs own portable POD meaning and nothing else.
- Code packs own systems, evaluation engines, document codecs, and IO.
- `rigProject` walks a codec registry. It does not know domain POD types.
- Every pack can build in the smallest host tree its `pack.json` declares. A build must not depend on sibling packs that happen to be cloned locally.
- New domain references resolve through the owning pack, not through a central list that grows with every domain.

## Naming convention

One id per pack: spoken name, folder, GitHub repo, `pack.json` `"name"`, `app.json` dependency `"name"`, and CMake target are the same camelCase token. Every pack id starts with `rig`.

| Pattern | Role | Examples |
|---------|------|----------|
| `rig<Domain>Component` | Data pack: POD components, `GetProperties`, `registerComponent`, pure helpers over the POD | `rigNodeComponent`, `rigCadComponent`, `rigAnimComponent` |
| `rig<Domain>` | Code pack: engine, systems, codecs, and IO for that domain | `rigNode`, `rigLayout`, `rigFont` |
| `rig<Domain>Ui` | IMui chrome over a separate engine or data pack | `rigPlotterUi`, `rigVectorUi` |
| `rig<Domain>Edit` | Headless operations over existing PODs; no chrome | `rigMeshEdit` |
| `rig<Format>` | IO for one file format | `rigSvg`, `rigObj`, `rigUfo` |

Rules:

1. Data packs use the full word `Component`. Do not use `Comp`, `Pod`, or `Data` as a pack suffix.
2. There is no `System` or `Systems` suffix family. `rigSystems` is the spine pack of generic systems over the floor data; it is a proper noun, not a pattern. Domain systems register from the domain code pack (`rigNode` registers `SGraphEval`).
3. Generic reusable meaning grows `rigComponent`. A new `rig<Domain>Component` needs a real domain seam: its own schema family, a distinct POD set, and a reason the floor cannot own it. Never one pack per feature.
4. A code pack must stay linkable without ImGui. Chrome lives in the `Ui` pack for the domain.
5. Historical `Editor` ids (`rigNodeEditor`, `rigCodeEditor`, `rigVectorEditor`) stay. Do not create new `Editor` packs; use `Edit` for ops and `Ui` for chrome.

## Decisions

These were settled before implementation. Reopening one needs a new decision, recorded here.

### 1. Contract reference fixup lives on the codec

Contract documents name entities with document string ids. PODs store resolved references: a live entity handle, or an entity name in a string field (`CModBinding.source`, `CCadBoolean.operands`). The serializer registry gains one optional hook, `remapContractReferences`, which receives a map from document id to live entity name. Each codec implements the hook for its own reference fields.

- `rigProject` gains the hook and the call site in Contract import. It loses every domain include.
- Anim codecs in `rigSystems` implement it for `CModBinding` and `CTween`.
- Cad codecs in `rigManifold` implement it for `CCadBoolean`, `CCadExtrude`, and `CCadRevolve`. `CCadDimension` joins only after its schema reference semantics are confirmed against RigWorks.
- The existing `remapReferences` hook is a different tool: `ProjectSerializer` uses it with file-local numeric ids (`e42`) for POD fields that hold live entity handles. It stays as it is.
- The hook replaces a key only when the key is in the document id map. Values that are already entity names pass through untouched.

### 2. The CAD box schema id is `rig.cad.cuboid`

RigWorks defines `rig.cad.cuboid` in its schemas, examples, and catalog. The `CCadBox` codec in `rigManifold` registers `rig.cad.box`, which no other document defines. The codec changes to `rig.cad.cuboid`.

- Rename `CCadBox` to `CCadCuboid`: type, header file, serializer functions, registry name, and every consumer.
- No dual-id support. A saved document that carries `rig.cad.box` reports an unknown `rig.*` key on load instead of translating silently.
- The serializer writes all three dimensions. No shorthand for missing dimensions.

### 3. Cross-pack changes land big-bang, verified locally first

Pack CI checks out the host and overlays one pack, so a coordinated change across `rigProject`, `rigSystems`, and `rigManifold` cannot be verified one pack at a time.

- Build and verify the complete set in one local host tree before pushing anything.
- Push in dependency order: registry API, codec implementations, then the import cleanup.
- Pack CI may go red between pushes. That window is accepted; a partially pushed state that silently changes document behavior is not.

### 4. The Node family splits into data, code, and editor

`CNodeGraph` is the only portable meaning in the Node family. Everything that evaluates, advances, or serializes it is code.

| Pack | Owns |
|------|------|
| `rigNodeComponent` | `CNodeGraph` PODs and `GetProperties`. Pack setup registers components only. |
| `rigNode` (new) | Graph evaluation (`GraphEval`), the `SGraphEval` Update system, `ApplyRefs`, audio and MIDI buses, node catalog, composite library, and the `.rig` codecs. |
| `rigNodeEditor` | ImGui editor chrome. Depends on `rigNode` for evaluation. |

- There is no separate `rigNodeSystems` pack. `rigNode` is the domain code pack; the system registers from it.
- System state (graph time, per-graph scratch) lives on the pack instance or on entities, never in process-wide statics.
- The `nodedata` example moves to `rigNode`. Data packs do not ship host examples.
- Consumers update in the same change: `rigNodeEditor` takes `rigNode`; `rigPixelPlotter`, `rigAudio`, and `rigMidi` dependencies on `rigNodeComponent` are audited, and each takes `rigNode` only if it evaluates graphs.

### 5. Codec policy is hybrid, with named exceptions

Default: the code pack that fulfills a POD files its `.rig` codec with `project::addSerializer`. Data packs only register the component.

| POD family | Codec home | Why |
|------------|-----------|-----|
| Floor (`rigComponent`) | `rigSystems` | The spine code pack over floor data. Includes the `CAnchor` codec, which moves out of `rigProject`. |
| Anim (`rigAnimComponent`) | `rigSystems` | It advances the modulators it serializes. |
| Cad (`rigCadComponent`) | `rigManifold` | The bake engine owns the solids. |
| Node (`rigNodeComponent`) | `rigNode` | Decision 4. |
| Pixel plot (`rigPixelPlotComponent`) | `rigPixelPlotter` | Single fulfillment engine. |
| Layout and story (`rigLayoutComponent`, `rigStoryComponent`) | `rigLayout` | The compose engine is the consumer that places story data. |
| Font (`rigFontComponent`) | To decide in Phase 3 | Candidates are `rigFont` (shape engine) and `rigUfo` (IO, already depends on `rigProject`). Decide from which apps load font PODs without the other pack. |
| Page (`rigProjectComponent`) | `rigProject` | Decision 6. |
| Plot (`rigPlotComponent`) | Exception: stays in the data pack | Thirteen peer code packs consume the PODs; no single fulfillment exists. Forcing one engine on every IO leaf costs more than the exception. |
| Show (`rigShowComponent`) | Exception: stays in the data pack | No code pack fulfills the deck data yet. |
| Color (`rigColorspace`) | Exception: stays in the data pack | Color is its own domain home with several peer consumers and no single engine. |

Exception rules:

- An exception exists only where the table names it and says why. New exceptions need a decision recorded here.
- An exception ends when a single fulfillment code pack emerges. The codec moves in the same change that creates or designates that pack.
- Before any codec moves, list the consumers that load the data pack without the destination code pack. If one exists and still needs document IO, the codec does not move yet. Losing document IO silently is worse than a listed exception.

### 6. `rigProject` splits into data and code

`CProject` and `CPage` are PODs. The serializer registry, document serializer, Contract import, `.rigz` packaging, and the deferred load/save tick are code. They separate:

| Pack | Owns |
|------|------|
| `rigProjectComponent` (new) | `CProject`, `CPage`, `FaceInsets`. Setup registers components only. |
| `rigProject` | Codec registry, `ProjectSerializer`, Contract import, `Rigz`, document session, and the `ProjectLoadSave` system. Depends on `rigProjectComponent`. Files the `CPage` codec. |

A code pack may register a system, so `ProjectLoadSave` needs no exception: it is the deferred document IO tick of a code pack, requested from UI callbacks and executed at the Update safe point. Its behavior and contract smoke coverage stay.

## Work plan

Phases run in order. Each phase lists its gate; a phase is not done until the gate passes.

### Phase 0: Baseline

1. Reproduce the CI failure locally in a basics-only tree: host plus submodule packs, with no optional domain pack checkouts beside them. The failing compile of Contract import is the baseline.
2. Record the current green baseline in the full local tree (contract smoke plus the example touched by each phase).

Gate: the basics-only tree fails where CI fails, and the full tree passes.

### Phase 1: Contract import boundary

1. Add `remapContractReferences` to the serializer registry and call it from Contract import after all entities exist. Keep the typed domain fixups in place during development; remove them in step 4.
2. Implement the hook in the anim codecs (`rigSystems`) and cad codecs (`rigManifold`).
3. In `rigManifold`, change the box codec id to `rig.cad.cuboid`. In `rigCadComponent` and consumers, rename `CCadBox` to `CCadCuboid`.
4. Remove from Contract import: domain includes, the typed CAD and anim fixup loops, and the optional domain include directories in `rigProject` CMake.
5. Prove or remove the `CModLfo` special cases in `contractGetString` and `contractSetString` (`waveform` is already a string property). Decide `contractRunAction` on evidence: the live UI path triggers actions through the ECS event system, so find any real caller before keeping a second action path in document IO.
6. Add contract smoke coverage with document ids that differ from entity names (an LFO binding and a CAD boolean), asserting the PODs hold entity names after import.

Gate: the basics-only tree from Phase 0 now builds; contract smoke passes in the full tree, including the writer-to-reader agreement test; a Contract document with CAD and anim references imports with resolved names when those packs are loaded.

### Phase 2: Node family split

1. Scaffold `rigNode` from the pack template. Move evaluation, system, buses, catalog, composite library, codecs, and the `nodedata` example.
2. Reduce `rigNodeComponent` to POD registration. Update its `pack.json` description.
3. Update `rigNodeEditor`, `rigPixelPlotter`, `rigAudio`, `rigMidi`, and the node docs (`docs/nodes.md`, `docs/packs_using.md`, catalog).

Gate: an app that evaluates graphs loads `rigNode` explicitly; the editor preview still evaluates; `rigNodeComponent` contains no `registerSystem`.

Status: done locally. The buses (`AudioBus`, `MidiBus`, quantize) moved with the engine - they are runtime value plumbing between producers (`rigAudio`, `rigMidi`) and graph consumers, not PODs. Pure graph construction helpers (`NodeGraph.h`: `makeGraph`, `addNode`, links) stay in the data pack, same exception class as `rigComponent` creators. The `rigNode` repo is local-only until published under `rigkid/`; pack CI that fetches it by URL stays red until then (see `notes.md`).

### Phase 3: `rigProject` split

1. Scaffold `rigProjectComponent`; move `CProject`, `CPage`, and `FaceInsets`. Its setup registers the two components.
2. Make `rigProject` depend on `rigProjectComponent`, keep document IO and the session system, and file the `CPage` codec.
3. Move the `CAnchor` codec from `rigProject` core serializers to `rigSystems`.
4. Update `SpineFixture`, bootstrap order docs, and every includer of the moved headers.

Gate: contract smoke passes; document save and load round-trip through the Update tick as before.

Status: done locally. `rig/pages.h` stays in `rigProject`: beyond creators it carries board navigation with input polling, which is code, not POD help. The `rigProjectComponent` repo is local-only until published under `rigkid/` (see `notes.md`).

### Phase 4: Codec migrations

One family per change, in this order: pixel plot, layout and story, node (rides Phase 2), font (after its home decision). Each migration:

1. Lists consumers that load the data pack without the destination code pack.
2. Moves the serializer source, files the codec from the code pack setup, and leaves component registration in the data pack.
3. Round-trips one document that uses the family before and after.

Gate per family: the consumer list has no silent IO loss, and the round-trip passes.

Status: pixel family done locally (consumer matrix: only `rigPixelPlotter` and its apps load the pixel PODs for IO; the data pack's own `pixeldata` example does no document IO). The serializer sources moved verbatim, so round-trip behavior is unchanged; both pack targets and both examples build. Layout and story, then font, remain.

### Phase 5: Remaining boundary audit

1. Classify the IO PODs (`COscEndpoint`, `CNetworkIdentity`, `CSerial`, `CNetScan`, `CNetHost`, `COnChainArt`): document data that travels in `.rig` files moves to a data pack; session and transport configuration stays with its transport pack, with the reason written in the pack docs. Do not create a shared IO data pack until a second consumer needs the same POD without the code pack.
2. Move `CGuide` from host `src/ecs/components/` into `rigComponent` and update includers.
3. Decide the cast PODs (`CScreenCast`, `CScreenCastReceive`, `CCastReceiver`) with the same classification: floor, or a data pack beside `rigScreenCast`. Write the decision in `packs_using.md`.
4. Move durable layout bake components out of `rigLayout` into `rigLayoutComponent`; transient bake state stays inside the engine without component names.
5. Audit `rigColorspace` and `rigComposition` against the convention. They are shipped ids; any change needs the same consumer-first treatment as Phase 4, not a rename for its own sake.

Gate: every item above ends in a move or a written decision. No item ends in "leave it" without a reason in the docs.

### Phase 6: Guardrails

1. Extend `tools/check-invariants` on the host side: fail when `rigProject` sources include domain component headers, when `rigProject` CMake adds sibling domain include directories, and when the string `rig.cad.box` appears in first-party sources.
2. Give each pack's own CI a guard that fails when a data pack source registers a system. Host-side checks cannot see optional pack checkouts, so this rule lives with the pack template workflow and is applied to packs as they are touched.
3. Reconcile the written policy in `packs_using.md`, the `rigkit-data` skill, and `AddSerializer.h` so all three describe the hybrid codec policy and the naming convention the same way.

Gate: reintroducing the Phase 1 fault in a test branch fails the invariant check.

## Separate workstreams

These cause red CI today but are not boundary work. They stay out of this plan so its success criteria stay readable:

- Actions secret availability (`RIGKIT_CI_TOKEN`) per repository. This is repository settings, verified per pack.
- `rigImGui` clang-format backlog.
- Private-pack FetchContent authentication in example builds (for example `rigAudio`).

## Success criteria

- A basics-only host tree builds the pack examples that CI builds.
- `rigProject` sources contain no domain component includes and no sibling domain include directories.
- No data pack registers a system. `rigNodeComponent` and `rigProjectComponent` are POD packs.
- Each codec lives in its hybrid-policy home or in a named exception.
- Contract import resolves anim and CAD references when those packs are loaded, with document ids that differ from entity names.
- The invariant checks catch a reintroduced domain include in `rigProject`.
- `packs_using.md`, the `rigkit-data` skill, and `AddSerializer.h` state one codec policy and one naming convention.

