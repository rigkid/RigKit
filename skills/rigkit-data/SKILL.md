---
name: rigkit-data
description: >-
  RigKit first layer: NO CODE JUST DATA. Use when adding or changing ECS
  components, GetProperties / property inspector, serialization or schema
  slices, deciding data vs systems/UI, choosing which pack owns a component
  (host / rigComponent / domain data pack), or when a feature request jumps to
  a new panel or system without defining entity fields. Also use for POD /
  portable component design across Canvas, Pi, ESP32, or other Contract hosts.
---

# RigKit data layer

Capability units are **packs** (e.g. `rigComponent`). **One id:** spoken name = folder = `pack.json` `"name"` = `app.json` `"name"`. Suffixes (`*Component`, `*Editor`, `*Ui`) and when to create a pack: [packs/README.md](../../packs/README.md#naming).

## Principle

**NO CODE JUST DATA.** The portable rule itself lives upstream in vendored **RigWorks** — [ecs.md](../../docs/contract/RigWorks/docs/ecs.md) (components, registry, systems/phases) and [properties.md](../../docs/contract/RigWorks/docs/properties.md) (portable property datatypes). Do not restate those rules here; this skill is RigKit's application of them — which pack owns a component, and the checklist/pattern for writing one.

Systems, `rigImGui`, EnTT, and GPU resources are **fulfillment** — not the Contract. ECS rules stay library-agnostic (desktop/Pi: EnTT; ESP: POD table).

Before writing a system or panel: name the fields the entity needs.

## Data pack

A **data pack** (`*Component`, plus project envelope **rigProject**) is **components-first**, not "only structs and nothing else."

| Owns | Does not own |
|------|----------------|
| `C*` POD components + `GetProperties()` | `registerSystem` Update/Draw |
| `registerComponent` in pack `setup()` | ImGui / `IWindow` panels |
| Codecs that serialize those PODs on **rigProject** | GPU / window / socket handles in components |
| Pure helpers over POD (catalog, eval, flatten) | Engines / second scene graphs |

If the helper starts looking like a pipeline or UI, move it to a **code** pack (`*Editor`, `*Ui`, short IO name, product engine).

## Component authority

Where a new component lives — walk this ladder and stop at the first fit:

1. **Host** [`src/ecs/components/`](../../src/ecs/components/) — only host-bound or impure leftovers (`CEvent`'s type-erased payload). Do **not** grow this for new portable meaning.
2. **`rigComponent`** — generic reusable PODs (transform, canvas, shape, mesh, draw style, selection, …). Grow here. Do **not** split every feature area into its own data pack.
3. **Domain data pack** — product-specific PODs (`rigProject` envelope/pages; `rigPlotComponent` paths/layers/zones/pen). New pack only when the meaning is not generic **and** no existing pack owns that seam (survey [packs/README.md](../../packs/README.md) first).
4. **Code packs** — systems / I/O / UI only (`rigSystems`, `rigPlotter`, `rigSvg`, `rigImGui`, …). Do not park portable components there (transient helpers only).
5. **App-local** — one-off prototype fields may live in the app until they prove reusable; then promote to (2) or (3).

Also:

- **Survey before a new pack** — read the pack table + Naming rules; grow an existing pack when the role already exists. Lock one camelCase spoken name for folder / manifests / CMake.
- Keep **`rigComponent` thin** — domain weight goes in domain data packs, not a kitchen-sink core.
- Domain types serialize by registering codecs on **`rigProject`** (or root extensions), not by stuffing plot/MIDI/… types into `rigComponent`. RigKit's `.rig` writer still uses a PascalCase alias layer today ([docs/interchange.md](../../docs/interchange.md)); once it moves to Contract JSON, a host component with no Rig schema id should travel as `x.rigkit.<name>` — see [extension components](../../docs/contract/RigWorks/schemas/document.md#extension-components) — not as a reason to invent a fake `rig.*` id.
- Naming: `C*` structs; files match type names; `GetProperties()` + `registerComponent` in the **owning** pack's `setup()`.
- Engines (e.g. PlotDoc) are **code**, not components — they live in code packs and mutate domain PODs.

Human mirror: [docs/packs_using.md](../../docs/packs_using.md).

## Checklist

1. **Fields first** — list the state as public data on a plain struct.
2. **Authority** — pick host / `rigComponent` / domain data pack / app using the ladder above.
3. **Keep portable components clean** — no window pointers, GPU handles, ImGui / UI toolkit types, or `std::function` when the meaning should be data ([ecs.md](../../docs/contract/RigWorks/docs/ecs.md)).
4. **`GetProperties()` when editor-visible** — return `std::vector<sProp>` with pointers into **member** fields (not locals). Types from `src/ecs/PropertyReflection.h` (`EPT_FLOAT`, `EPT_VEC3`, `EPT_COLOR`, …) map to the portable [property datatypes](../../docs/contract/RigWorks/docs/properties.md). A named-choice `enum class` field is `EPT_ENUM` with `enumNames`/`enumCount` set — never `EPT_INT` (that silently draws a raw draggable integer instead of a combo). Wire the reader in the same change — a property row that nothing reads is a control that lies (Commandment 10). Settings rows follow the same rule as component fields.
5. **Schema travel** — if Pi / ESP32 / another host must share meaning, prefer a small JSON or binary slice of POD fields; schema alignment matters more than identical ECS libraries ([docs/contract/rigkit.md](../../docs/contract/rigkit.md)).
6. **Behavior elsewhere** — `rigSystems` / document systems via `registerSystem`; never inside data packs (`rigComponent`, `rigPlotComponent`, …).
7. **`registerComponent` in setup** — announce the type on `MEcs` so rigImGui Properties can discover it.
8. **Push back** — if the request starts with a new ImGui window or system, redefine it as data first, then the thinnest fulfillment.
9. **Author path** — prefer short creators/helpers that write POD data (`makeRect`, `makeMesh*`, …) over exposing `MEcs` first. Meshes are first-class (`CMesh`). See [docs/authoring.md](../../docs/authoring.md).

## Pattern

```cpp
struct MyComponent {
	float brightness = 1.0f;

	std::vector<sProp> GetProperties() {
		return {{0, "Brightness", EPT_FLOAT, &brightness}};
	}
};

// In pack/app setup (code), not inside the struct:
ecs->registerComponent<MyComponent>("MyComponent", true);
```

- Generic data: `packs/rigComponent`.
- Domain data: e.g. `packs/rigProject`, `packs/rigPlotComponent`.
- Host leftover: `CEvent` — do not widen impurity.
- See [docs/packs_using.md](../../docs/packs_using.md).

## Known gap

`CEvent`'s payload is a type-erased `std::any` (needed for an in-process event bus) — it does not travel and is not meant to serialize. New work must not widen that gap by adding host types (window pointers, GPU handles, callbacks) to other components. Project serialize lives in rigProject (JSON registry + core codecs; default `.rig`).

## Sources

- [docs/contract/RigWorks/docs/ecs.md](../../docs/contract/RigWorks/docs/ecs.md) — portable ECS rules
- [docs/contract/RigWorks/docs/properties.md](../../docs/contract/RigWorks/docs/properties.md) — portable property datatypes
- [docs/contract/rigkit.md](../../docs/contract/rigkit.md)
- [docs/packs_using.md](../../docs/packs_using.md)
- [src/ecs/PropertyReflection.h](../../src/ecs/PropertyReflection.h)
- [AGENTS.md](../../AGENTS.md)
