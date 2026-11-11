# Blender to RigKit

Load path: Blender evaluates a `.blend`, the [RigWorks blender add-on](contract/RigWorks/tools/blender-addon/README.md) writes a `.rig` / `.rigz`, and RigKit opens that document. Nothing in RigKit parses `.blend`. Contract map: [docs/contract/RigWorks/docs/blender.md](contract/RigWorks/docs/blender.md).

Host for this work: **example-stage** (already presents with **rigRender3D**). Open uses **rigProject** `importContractFile` with codecs so string entity ids (Blender / hand-authored docs) remaps parents correctly. Do not use `ProjectSerializer::load` for those documents - it only remaps numeric `eN` ids.

## Phase 1 - Open and see it

**Done when:** export (or open the stage fixture) shows one lit mesh from the camera under the sun.

| Work | Home |
|------|------|
| `--open=<path.rig>` on example-stage | `examples/example-stage` |
| Import via `importContractFile` + codecs | **rigProject** |
| Accept Contract flat `positions` / `texcoords` on mesh decode | **rigSystems** `deserializeMesh` |
| Keep authored directional rotation (only aim-at-origin when rotation is identity) | **rigProject** `ContractImport` |
| Ensure one active camera; prefer one directional light | example-stage post-open |
| Fixture + `--open=... --smoke` | `data/scenes/basic-cube.rig` |

**In:** mesh + Principled colour (no texture) + camera + one directional + hierarchy.  
**Out:** instances, textures, HDRI, anim, skin, multi-light accumulate, spots.

## Phase 2 - Basic

**Done when:** linked duplicate, hide flags, textured Principled in a `.rigz`, and a usable point-only light read correctly.

| Work | Home |
|------|------|
| Present or expand `rig.spatial.instance` | **rigRender3D** (prefer) or expand on import |
| Honour `rig.render.visibility` in 3D present | **rigRender3D** |
| Albedo map + `.rigz` `data/` | stage + **rigImage** |
| Point light when it is the only light | light picker |
| Material schema writer (save round-trip) | **rigProject** / serializers |

**Still out:** skeleton / skin / clip, product-quality IBL, spots, area lights, multi-light shading.

## Phase 3 - Intermediate

**Done when:** the add-on smoke skinned armature + sampled action plays; world HDRI affects shading.

| Work | Home |
|------|------|
| Import skeleton / bind / bone entities | **rigAnimComponent** (+ port-map Close) |
| Import skin (4 influences) | **rigAnimComponent** |
| Skinned mesh present | **rigRender3D** / thin system (**Pi risk**) |
| Clip / track playback | **rigAnimComponent** + Update |
| HDRI open from exported `.rigz` | stage + **rigImage** |
| Spot lights | grow `CLight` + present |
| Optional headless Blender convert tool | tools (fulfillment; never a Kit `.blend` parser) |

**Still out (Contract):** modifiers, Geometry Nodes, live constraints/drivers, shape keys, area lights until schema, multi-UV, Cycles graphs.

## Risks

- **Pi / rebuild:** skinned present and HDRI decode stay in packs; stage is the verify host.
- **Fix where it lives:** light aim, mesh packing, and instance present are pack fixes - no private viewer workaround.
- **One light:** phases 1-2 stay honest about single-light present; multi-light is a later present design.

## Commands (phase 1)

```bash
cmake -S examples/example-stage -B examples/example-stage/build
cmake --build examples/example-stage/build --target example-stage
./examples/example-stage/build/bin/example-stage --open=data/scenes/basic-cube.rig
./examples/example-stage/build/bin/example-stage --open=data/scenes/basic-cube.rig --smoke
```

Blender: File > Export > Rig Document (`.rig`), then `--open=` that path.
