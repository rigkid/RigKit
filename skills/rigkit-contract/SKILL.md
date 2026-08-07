---
name: rigkit-contract
description: >-
  RigKit Contract and Host boundaries. Use when working on the SUDE loop
  (Setup/Update/Draw/Exit), ECS conventions, UI / IMui, Raspberry Pi full
  host, ESP32 subset, packs, distribution vs Contract, or
  parallel hosts when cross-host Contract work applies.
---

# RigKit contract

## Portable rules live upstream

The Contract itself — Terms, SUDE, ECS, UI, property datatypes — is defined once in vendored **RigWorks**, not restated here. Edit rules there, not in this skill:

- [terms.md](../../docs/contract/RigWorks/docs/terms.md) — Contract vs fulfillment, roles, data words
- [sude.md](../../docs/contract/RigWorks/docs/sude.md) — Setup / Update / Draw / Exit
- [ecs.md](../../docs/contract/RigWorks/docs/ecs.md) — registry + component rules
- [ui.md](../../docs/contract/RigWorks/docs/ui.md) — Rig + UI (optional layer)
- [properties.md](../../docs/contract/RigWorks/docs/properties.md) — portable property datatypes

RigKit's own short working copies under [docs/contract/](../../docs/contract/) (`sude-loop.md`, `rigkit.md`, `ui.md`, `port-map.md`) restate the same rules with RigKit specifics folded in — keep both in sync when a rule changes upstream. This skill holds only what is RigKit-specific: pack vocabulary, pillar mapping, the target ladder, and hard boundaries in this host's code.

Data layer is mandatory before Contract work that touches entities — see [rigkit-data](../rigkit-data/SKILL.md).

**Raspberry Pi is the floor for the full host.** If a change would not compile or run smoothly on Pi, it fails Contract intent — see [pi-host.md](../../docs/contract/pi-host.md).

## Vocabulary (do not regress)

| Say | Mean |
|-----|------|
| **pack** | Capability unit (`rigImGui`, `rigComponent`, …). Folder `packs/<id>/`, manifest `pack.json`, types `IPack` / `MPack`. |
| **Host** | RigKit's runtime, in `src/` — this repo's fulfillment of Rig |

**Never say "addon" or "module" for these packs.** Not in docs, comments, APIs, or new code. ("Module" only for git submodules / Doxygen / generic modular builds.)

**Match the word the code already uses.** `notification`, not `toast`; `pack`, not `addon`. A synonym in a heading becomes a synonym in an API.

**One id.** Spoken name = folder under `packs/` = `pack.json` `"name"` = `app.json` dependency `"name"` = CMake target (e.g. **rigImGui**). No display title or mismatched class/repo name. Before scaffolding a new pack: survey the pack table in [packs/README.md](../../packs/README.md); prefer growing an existing seam. Suffixes (`*Component` / `*Editor` / `*Ui`) and data-pack bounds: [packs/README.md#naming](../../packs/README.md#naming).

## Pillars (RigKit fulfillment)

| Pillar | RigKit fulfillment |
|--------|------|
| SUDE | `RigKitEngine` + `IApp` Setup/Update/Draw/Exit |
| ECS | `MEcs` over EnTT (desktop/Pi); POD table (ESP) |
| UI | `rigImGui` via `IMui` |
| Distribution | Default ship set; `rigImGui` fulfills UI — not the Contract itself |

`Canvas` in code = render surface / FBO (`MCanvas`) — not the Host pillar.

## Hard boundaries (RigKit-specific)

- **UI / ImGui:** no Dear ImGui in `src/`; attach via `IMui`. Default UI fulfillment = `rigImGui`. Chrome rules (DPI, work rect, prefs): [packs/rigImGui/README.md](../../packs/rigImGui/README.md#chrome-rules).
- **GPU / UI types:** stay out of Contract-facing components; host packs may hold resources keyed by opaque ids.
- **Registry:** app owns logical scene/document; host may hold a pointer and run systems — no second hidden registry for app entities.
- **Systems:** simulation in `Update(dt)`; present in `Draw()`. Host always calls Draw after Update (empty body OK; skip is not SUDE). Draw can be LEDs / GPIO, not only GPU.

## Target ladder

| Target | Role |
|--------|------|
| Desktop | SUDE–ECS–UI fulfillment (author tools + install content) |
| Raspberry Pi | **Minimum full host** — complete Contract on device; compile + smooth Update/Draw ([pi-host.md](../../docs/contract/pi-host.md)) |
| ESP32 / MCU | Default subset: SUDE + **RigKit-esp32-core** (POD, no UI). With web UI over ECS: full **SUDE–ECS–UI**. Firmware builds use `#define RIGKIT_MCU` ([rigkit.md](../../docs/contract/rigkit.md)) |

## Packs

Prefer POD for portable meaning. Receive `MEcs*` from the engine; define pack-local components as plain structs. [docs/packs.md](../../docs/packs.md).

## Parallel hosts

Other runtimes may fulfill the same Contract rules. They do not own Contract — POD/schemas travel; library types do not. RigKit remains the reference coded host in this repo — see [docs/contract/README.md](../../docs/contract/README.md).
