# RigKit docs

Index for browsing this folder on GitHub. Start with [authoring](authoring.md) if you write apps; [contract/](contract/README.md) if you need the rules; [packs_catalog.md](packs_catalog.md) if you need a pack.

Root overview: [../README.md](../README.md). Agent playbook: [../AGENTS.md](../AGENTS.md).

## Start here

| Doc | Purpose |
|-----|---------|
| [authoring.md](authoring.md) | How artists code RigKit — `setup` / `update` / `draw`, creators, POD |
| [build_instructions.md](build_instructions.md) | CMake, examples, ANGLE, Pi notes |
| [nodes.md](nodes.md) | Node graphs — editor + catalog (artist guide) |
| [contributing.md](contributing.md) | Format, hooks, CI style, how to contribute |

## Contract + Host

Portable Rig rules (with RigKit notes). Full intro: [contract/README.md](contract/README.md).

| Doc | Purpose |
|-----|---------|
| [contract/commandments.md](contract/commandments.md) | Ten Commandments — constitution |
| [contract/sude-loop.md](contract/sude-loop.md) | Setup / Update / Draw / Exit |
| [contract/rigkit.md](contract/rigkit.md) | ECS conventions + ESP32 subset |
| [contract/ui.md](contract/ui.md) | UI companion + `IMui` |
| [contract/distribution.md](contract/distribution.md) | Default ship set; Rig + UI on author path |
| [contract/port-map.md](contract/port-map.md) | Rig schema / RigKit pack map |
| [contract/pi-host.md](contract/pi-host.md) | Raspberry Pi as minimum full host |

## Packs

| Doc | Purpose |
|-----|---------|
| [packs_catalog.md](packs_catalog.md) | Known and planned packs |
| [packs.md](packs.md) | What a pack is; layout and naming |
| [packs_using.md](packs_using.md) | Using ECS packs from apps and other packs |
| [../packs/README.md](../packs/README.md) | pinning, examples, CI, scaffold |

## Apps and examples

| Doc | Purpose |
|-----|---------|
| [apps.md](apps.md) | Examples directory |
| [apps_creating.md](apps_creating.md) | Creating an example or app |
| [../examples/](../examples/) | In-tree sample apps |
| [../templates/app/](../templates/app/) | App template for product hosts |

## Host internals

| Doc | Purpose |
|-----|---------|
| [managers.md](managers.md) | Core managers (`MEcs`, Canvas, …) |
| [settings.md](settings.md) | Settings and preferences |
| [includes.md](includes.md) | Includes and umbrella headers |
| [namespaces.md](namespaces.md) | Namespaces |
| [api/mainpage.md](api/mainpage.md) | Doxygen main page source — build with `cmake --build build --target docs` |
| [../site/](../site/) | GitHub Pages landing ([rigkid.github.io/rigkit](https://rigkid.github.io/rigkit/); API under `/api/`) |

## Elsewhere

| Path | Purpose |
|------|---------|
| [../AGENTS.md](../AGENTS.md) | Agent / contributor playbook |
| [../skills/](../skills/) | Task skills (data, build, contract, …) |
| [../packs/](../packs/) | In-org pack checkouts |
| [RigWorks](https://github.com/rigkid/RigWorks) | Portable framework (SUDE + ECS + schemas) |
