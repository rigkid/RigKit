# Contract + Host

Freedom first: artists own the tools they create.

**[RigWorks](https://github.com/rigkid/RigWorks)** (**Rig** for short) is a portable **shared vocabulary** (zero code): SUDE + ECS rules and POD schemas. Grab Rig without this repo. It is not a finished product by itself — hosts deliver fulfillment.

**RigKit** is a coded **Rig** host (runtime + packs) for **media installations** and **artist tool apps**. Product name stays RigKit. Author path is **Rig + UI**; show path stays Rig without UI.

| Piece | Role |
|-------|------|
| **RigWorks** / **Rig** | Framework playbook — [honors](https://github.com/rigkid/RigWorks/blob/main/docs/honors.md), [SUDE](https://github.com/rigkid/RigWorks/blob/main/docs/sude.md), [ECS](https://github.com/rigkid/RigWorks/blob/main/docs/ecs.md), [schemas](https://github.com/rigkid/RigWorks/tree/main/schemas). Rig in running text. |
| **SUDE** | Setup / Update / Draw / Exit — required for Rig |
| **ECS** | Portable entity data — required for Rig |
| **Schemas** | Agreed POD layouts — ship what you support (not all required) |
| **UI** | Optional companion ([Rig + UI](https://github.com/rigkid/RigWorks/blob/main/docs/ui.md)); author path usually **rigImGui** |
| **Host** | Runtime fulfillment (`src/`) |
| **Distribution** | Default ship set (includes `rigImGui` as UI fulfillment) |

A project **is Rig** when it honors SUDE + ECS. UI is not required. Schemas are formats when present.

In this folder, local copies of the loop / ECS / UI rules stay for RigKit contributors; **Rig is the source of truth** for portable rules.

**Contract / framework** = code-free rules (Rig). **Fulfillment** = RigKit host + packs (e.g. **rigImGui**, **rigComponent**) or any other host that honors those rules. Rig does not require a particular UI pack or registry library.

**Freedom** = license + leverage · **Own** = artist-made tools · **Ship** = installations and studio apps on Pi-class hardware

## Viewer vs Player

**Viewer presents; Player plays.** Same Rig documents, different hosts:

| Product | Job | Runs `rig.media.code`? |
|---------|-----|-------------------------|
| [RigViewer](https://github.com/rigkid/RigViewer) | Present POD (scene / GLSL sketch) | No (GLSL preview only on web) |
| [RigPlayer](https://github.com/rigkid/RigPlayer) | Play documents with a game loop (SUDE + Lua) | Yes (sandboxed Lua runtime) |

Shared desktop chrome (open document, skipped keys) belongs in a pack (`rigDocumentShell`), not in RigKit `src/`.

## Document vs Project (lingo)

| Word | Means | Owner |
|------|-------|-------|
| **Document** | The portable `.rig` file — Contract JSON (`entities[].components` with `rig.*` ids). What Viewer presents and Player plays. | RigWorks schemas |
| **Project** | The host-side working envelope (`CProject` / `CPage` PODs) inside a RigKit host session. | **rigProject** pack |

One file word (**document**, `.rig`), one host word (**project**). The rigProject pack implements document IO for hosts. Known gap: its PascalCase `.rig` writer predates Contract shape — Contract JSON is the canonical dialect; `tools/contract_smoke` "save is readable by Contract import" guards the bridge until the writer emits Contract natively.

## Documents in this folder

| Doc | Purpose |
|-----|---------|
| [commandments.md](commandments.md) | Ten Commandments — constitution (keep short) |
| [sude-loop.md](sude-loop.md) | SUDE — Setup / Update / Draw / Exit (RigKit copy; see Rig) |
| [rigkit.md](rigkit.md) | ECS conventions + ESP32 subset profile (RigKit copy; see Rig) |
| [ui.md](ui.md) | UI companion + RigKit `IMui` fulfillment notes |
| [distribution.md](distribution.md) | Default distribution; Rig + UI on author path |
| [port-map.md](port-map.md) | Rig schema ↔ RigKit pack map |
| [pi-host.md](pi-host.md) | Raspberry Pi as minimum full host |
| [../authoring.md](../authoring.md) | How artists code RigKit |
| [../nodes.md](../nodes.md) | Node graphs — editor + catalog (artist guide) |
| [../packs_catalog.md](../packs_catalog.md) | Known + planned packs |

## Who is what

| Host kind | Speaks |
|-----------|--------|
| Foreign / other runtime | **Rig** (SUDE+ECS) without RigKit; schemas as supported |
| ESP32 light (`RigKit-esp32-core`) | Rig (SUDE + ECS POD profile); not UI |
| ESP32 with author UI over ECS | **Rig + UI** |
| Pi/desktop show (no panels) | Rig |
| Author / tool app (this distribution) | Rig + UI |

## Schema map (RigKit → Rig)

Full catalog: [Rig schemas](https://github.com/rigkid/RigWorks/tree/main/schemas).

**Field-accurate map:** [port-map.md](port-map.md) — Close vs Partial vs Planned, with honesty notes. Do not treat affinity as “speaks this schema.”

Ship what you support. Rig holds the creative vocabulary as formats; RigKit packs fulfill subsets POD-first.

## Host complete

The **host** can finish. **Packs** do not — artists keep adding packs.

The host is complete when it can ship a **Rig + UI** fulfillment on Pi (SUDE–ECS–UI stack):

1. **Rig + UI** — Rig honored; UI via the `IMui` seam (no UI toolkit in `src/`). Default UI pack: **rigImGui**. Show path may omit UI and remain Rig-only.
2. **Seams are enough** — apps and packs extend through `IApp`, `MEcs` register/catalog, `IMui`, `IRenderer` / Draw, pack load — without a second scene graph or systems in `rigComponent`.
3. **Weight is outside core** — new features land in packs; core changes are bugfixes, seam hardening, or Pi/rebuild fixes.
4. **Data layer direction** — portable meaning is POD components + properties; host impurities are not widened.
5. **Pi is the floor** — compiles and runs smoothly on Raspberry Pi.
6. **Distribution is usable** — author path attaches `rigImGui`; show path may drop UI; at least one Draw path.
7. **Rebuild stays cheap** — thin core, narrow includes.
8. **Author path is friendly** — short `setup` / `update` / `draw` + creators that write POD.

Not required for host complete: every optional pack, hot-reload, or closing every init polish item in `TODO.md`. (`rigOsc` ships for install show control; still optional for host-complete.)

**One sentence:** the host is complete when a stable thin core lets artists ship a **Rig + UI** fulfillment on Pi by **adding packs**, and the default answer to “where does this go?” is a pack — not `src/`.

## What we are trying to achieve

A free platform creatives control for:

1. **Media installations** — live, long-running pieces on the floor / gallery / stage (Pi as the normal computer; sensors, LEDs, video, sound, generative visuals).
2. **Artist tool apps** — the custom software artists actually need (sequencers, mappers, show controllers, inspectors) built as packs on the same host.

Begin in a welcoming host. Build and keep your own tools. Run the full piece on desktop and **Raspberry Pi**. Carry an **ESP32 subset** when the install needs embedded endpoints — still Rig, not a fork.

## Target ladder

| Target | Role |
|--------|------|
| **Desktop** | Author tool apps and install content. Rig + UI fulfillment. |
| **Raspberry Pi** | **Minimum full host** for media installations (Rig + UI on device for author; Rig-only for show). |
| **ESP32** | Default: Rig (`RigKit-esp32-core` profile). With author UI over ECS: Rig + UI. |

## Parallel hosts

Parallel hosts are **not** the owner of Rig. Map SUDE to that runtime’s setup/update/draw; keep portable meaning in POD/schemas. Do not extract host GPU types into Rig. RigKit remains the reference coded host in this repo. Artists may grow a personal platform on RigKit or another host — [RigWorks](https://github.com/rigkid/RigWorks) is the shared playbook when pieces must cross.

## One sentence

**RigKit** is a **Rig fulfillment** artists open: Rig is SUDE+ECS (schemas when present), and the default distribution ships **rigImGui** so they can build **media installations** and **tool apps** they own — on Pi as the floor, with ESP32 as a Rig subset (or Rig + UI when author UI edits ECS).
