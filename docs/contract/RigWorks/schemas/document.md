# `rig.document`

JSON document envelope for Rig interchange. Format when present as a file or payload.

This is the **Contract wire format** (JSON). Field meaning remains POD; vectors and quats are JSON number arrays. Hosts may use other encodings internally — portable emission targets this envelope.

## Root

| Field | Type | Meaning |
|-------|------|---------|
| `rig` | string | Contract version this document targets (`MAJOR.MINOR.PATCH`) |
| `document` | object | Optional metadata (`title`, `author`, `createdAt`, `modifiedAt`, `defaultUnit`, `colorSpace`, …) |
| `entities` | entity[] | Scene / graph contents |

`colorSpace` names the colour space every `rgba` / `rgb` value in the file is expressed in; absent = `srgb`. One key for the whole document — per-component colour spaces do not exist.

## Entity

| Field | Type | Meaning |
|-------|------|---------|
| `id` | string | Stable within this file. Fields typed `entity` reference these ids (or `null` for none). |
| `components` | map | Keys are schema ids `rig.<domain>.<name>`, or `x.<vendor>.<name>` for host components — see below. Values match that schema’s serializable fields. |

## Rules

- Component keys **must** be catalog schema ids — not host type names (`Transform`) and not C++ class names.
- Serialize only portable fields listed on each schema. Omit host caches (Euler, world matrix, selection state, dirty flags, GPU handles).
- Unknown `rig.*` schema ids: validators **warn** by default; fail with `--strict`.
- Being Rig still means honoring [SUDE](../docs/sude.md) + [ECS](../docs/ecs.md). This envelope is how shared POD travels between hosts and tools.

## Extension components

A key matching `x.<vendor>.<name>` carries a component the Contract has not named. Validators pass the payload through unchecked and report it — including under `--strict`, where it is a note rather than a failure.

```json
"components": {
  "rig.spatial.transform": { "position": [0, 0, 0], "rotation": [0, 0, 0, 1], "scale": [1, 1, 1] },
  "x.acme.flower_of_life": { "radius": 40, "rings": 3 }
}
```

This exists because "ship what you support" previously worked in one direction only: a host could implement part of Rig, but could not carry anything Rig had not named. A real host has more component types than the catalog has ids, so without an escape hatch it keeps a private format alongside `.rig` — the outcome the Contract exists to prevent.

The bargain is explicit. Extensions **travel**; they do not **port**. Another host will preserve them across a round trip if it is careful, and will not understand them. Nothing in the Contract gives `x.*` meaning, and no validator will ever check one.

So the namespace is a place to put what is genuinely yours, not a way to avoid the catalog:

- Reach for a `rig.*` id first. If a concept is portable, it belongs in the vocabulary, and the catalog grows when a real host needs it.
- `<vendor>` should identify who owns the meaning — an app, an org, a pack.
- Do not mirror a Contract component under `x.*` to add one field. That splits the meaning across two keys and no reader will merge them.
- Runtime state stays out. An extension is still POD you meant to save, not a cache with a new prefix.

Machine grammar: [`schemas/json/rig.document.schema.json`](json/rig.document.schema.json).

See [docs/interchange.md](../docs/interchange.md) for RigKit `.rig` key mapping.
