# ECS conventions

Builds on [SUDE](sude.md) creating **Rig**.

No particular registry library. No GPU or UI toolkit types inside Contract-facing components.

## Why ECS

We use entity–component composition because we build for change. Composition over inheritance. Portable fields stay reusable across hosts and packs.

## Registry

- The **app** owns the logical registry (scene / document).
- The host must not require a second hidden registry for app entities.
- Registry implementation is host-specific.

## Components

- Portable meaning is **POD / plain data** (numbers, small structs, strings, entity ids).
- No window pointers, GPU handles, UI toolkit types, or behavior callbacks in Contract-facing components.
- No dirty flags, pending queues, or ephemeral edge state (hover/press) in portable components — those stay in the host and are not serialized.
- One type per field. Host-only resources may exist keyed by opaque ids; they are not Rig portable meaning.
- Creative layouts live under [`../schemas/`](../schemas/) — formats when present. Compose [`meta.named`](../schemas/meta/named.md) for labels; [`media.asset_ref`](../schemas/media/asset-ref.md) for paths.
- Editor-visible fields use portable [property datatypes](properties.md).

## Systems and phases

| Phase | When | Intent |
|-------|------|--------|
| Simulation | during `Update(dt)` | Mutate state |
| Present | during `Draw()` | Read mostly; present |

Light hosts may run only simulation.

## Hierarchy (optional shape)

When you have a scene graph:

- Parent / child as entity ids
- Local transform: position (vec3), rotation (quat), scale (vec3)
- World matrix as a cache written by a transform system

Exact type names are host-specific. See [schemas/spatial/transform.md](../schemas/spatial/transform.md).

## Light / Embedded hosts

It is Rig when SUDE + these rules hold.

> **Schema alignment matters more than identical registry libraries.**

See [honors.md](honors.md). UI is separate — [ui.md](ui.md).
