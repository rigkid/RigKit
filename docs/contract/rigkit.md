# ECS conventions

**RigKit floor:** SUDE + ECS. Document composition (entity/component POD) is also the RigWorks Contract floor; runtime registry and systems are required here because RigKit is always a live host.

Portable source of truth: [Rig ECS](https://github.com/rigkid/RigWorks/blob/main/docs/ecs.md). Shared schemas: [Rig schemas](https://github.com/rigkid/RigWorks/tree/main/schemas). RigWorks honors (grammar floor): [Rig honors](https://github.com/rigkid/RigWorks/blob/main/docs/honors.md). This file is the RigKit working copy (includes the ESP32 profile).

Builds on [sude-loop.md](sude-loop.md). [UI](ui.md) is an optional companion (**Rig + UI**), not required to be Rig or RigKit.

Code-free rules: no particular registry library, no GPU types inside Contract-facing components.

## Why ECS

We use an Entity-Component-System because we build for change. We don't know what the next tool is going to be — we want to be ready for it. And reuse what we already built.

## Registry ownership

- The **app** owns the logical registry (scene / document).
- The host may hold a pointer and run systems, but must not require a second hidden registry for app entities.
- Host fulfillment: `MEcs` holds the registry; apps create entities through it.

## Components

- Components used for portable meaning are **POD / plain data** (numbers, small structs, strings, entity ids).
- No window pointers, GPU handles, or UI toolkit types inside Contract-facing components.
- Runtime resources live in host stores keyed by opaque ids (future); until then, host-specific components are allowed in host packs but are **not** part of the ECS contract.

## Systems and phases

| Phase | When (SUDE) | Intent |
|-------|-------------|--------|
| Simulation systems | during `Update(dt)` | Mutate state |
| Present systems | during `Draw()` | Read mostly; present |

MCU hosts may run only simulation systems.

## Hierarchy (optional module)

Recommended portable shape (inspired by common scene graphs):

- Parent / child links as entity ids
- Local transform: position (vec3), orientation (quat), scale (vec3)
- World / global matrix written by a transform system after local edits

Exact type names are host-specific; the **shape** is the contract.

## Registry fulfillment

Desktop/Pi RigKit uses a registry inside `MEcs`. Rig does not require a particular library. A static or tiny entity table on embedded hosts may comply with the [ESP32 subset profile](#esp32-subset-profile).

## ESP32 subset profile

Named profile: **`RigKit-esp32-core`** (first concrete MCU subset; ESP32 is the reference board).

Firmware builds that omit the desktop `main` harness define **`RIGKIT_MCU`** (chip-agnostic). The profile name stays ESP32-specific until a second board profile exists.

| Include | Exclude |
|---------|---------|
| Entity ids + tags | Font shaping / UI |
| Transforms (2D or 3D POD) | GPU textures / FBOs |
| State / modulator floats | Document pagination |
| LED / GPIO / sensor POD | rigImGui / any UI pack |

A device that **is Rig** via entity/component POD under **RigKit-esp32-core** (and SUDE as a live host) shares vocabulary with the desktop host without shipping the editor. An embedded host that runs author UI over the same ECS POD data may be **Rig + UI**. That is a heavier stack than `RigKit-esp32-core`.

### What travels / what does not

- **Travels:** [SUDE](sude-loop.md) (`Setup` / `Update` / `Draw` / `Exit` — Draw always called; LED/GPIO present lives there); tags, POD transforms, state floats, LED/sensor fields.
- **Does not:** **rigImGui** / any UI pack; optional render packs; document / font packs.

### Proof sketch

See [`tools/esp32_contract_host/`](../../tools/esp32_contract_host/) — a tiny C++ unit implementing SUDE hooks and a minimal entity table. Build with `-DRIGKIT_BUILD_ESP32_CONTRACT_HOST=ON`, or copy into an ESP-IDF / Arduino project.

Prefer a small JSON or binary slice for entity POD fields when sharing state with the desktop host. Schema alignment matters more than identical ECS libraries.
