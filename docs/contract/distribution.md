# Distribution

The default **RigKit** build aims at a **Rig + UI** fulfillment so the host is usable for author tools and installs.

**RigWorks** (**Rig** for short) = zero-code framework ([rigkid/RigWorks](https://github.com/rigkid/RigWorks)): SUDE + ECS + schemas when present. **Distribution packs** (such as `rigImGui`) are fulfillments; they are not Rig itself.

Artists and install hosts may omit UI packs, drop to **Rig** (show mode), and remain Rig without UI.

## What’s in the default build

| Layer | In default build? | In Rig? | Notes |
|-------|-------------------|--------|-------|
| SUDE | Yes | **Yes** | [sude-loop.md](sude-loop.md) |
| ECS conventions | Yes (host registry) | **Yes** | [rigkit.md](rigkit.md); registry library is fulfillment |
| Shared schemas | As implemented | **Formats when present** | [Rig schemas](https://github.com/rigkid/RigWorks/tree/main/schemas) |
| UI | Yes (via seam) | **Optional companion** | [ui.md](ui.md); rules, not a toolkit |
| Host (`RigKitEngine`, managers) | Yes | **Yes** (coded host) | honors Rig; + UI when attached |
| **`rigImGui`** | **Yes** | **No** | Fulfillment of **UI** via `IMui` |
| Default Draw path (`IRenderer`) | Yes | **No** | Host present path; **rigBlend2D** optional pack |
| Optional packs (OSC/show, tool panels, …) | As needed | **No** | Artist-owned |

## Default UI kit (`rigImGui`)

`rigImGui` is the UI kit shipped in the default RigKit distribution so **author** mode is a full **Rig + UI** fulfillment. It implements [`IMui`](../../src/core/IMui.h) (`Mui`): host chrome (menu, Log, Windows, Properties, optional Debug/Theme) and a place for apps to register their own windows. Portable meaning stays in ECS; panels read and edit that data.

It is an in-org pack under `packs/rigImGui`, not host core and not the UI companion rules. Core must not include a UI toolkit — only the `IMui` seam. Show, headless, and light embedded hosts detach it and remain **Rig**; attach it again (or another UI pack) for **Rig + UI**.

Other UI packs may replace **rigImGui** and still honor Rig + UI. Pack layout and pinning: [packs/README.md](../../packs/README.md).

## Optional vs required for a Rig + UI fulfillment

**Required for the named RigKit author distribution:**

- Host core (`src/`)
- UI attached (default: `rigImGui` implementing `IMui`)
- At least one Draw fulfillment registered

**Optional packs** (examples): `rigBlend2D`, `rigOsc`, plot/pixel/node families — see [packs_catalog.md](../packs_catalog.md).

App-specific panels (e.g. Show Control) live in the app until they are reused enough to extract.

## Layout modes (one app)

Demos and tool apps should prefer **one binary**, choosable UI layout:

- **Author** — Rig + UI fulfillment (`rigImGui` panels visible)
- **Show** — Rig; UI-light or no `rigImGui`; present path only

Not separate downloads per mode.

## Related

- Contract map: [README.md](README.md)
- UI: [ui.md](ui.md)
- Pi floor: [pi-host.md](pi-host.md)
- ESP32 subset: [rigkit.md](rigkit.md#esp32-subset-profile)
