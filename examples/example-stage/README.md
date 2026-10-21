# example-stage

Stage dresses a 3D scene and plays it. The editor executable is `example-stage`. The play executable is `example-stage-play` (same scene and scripts, no editor packs linked).

## What you see

A lit stage: a checkered ground, a small cloth, an orange metal ball that drops, a green triangle imported from glTF (base color, roughness, checker texture), and a blue sphere spawned by `data/play.py` on the first update. The selected triangle has the Mesh Edit gizmo.

`CRenderMaterial` is shaded by **rigRender3D**: GGX, the albedo image, emissive, and ACES into the window. `data/env/sky.hdr` is the irradiance, on the `CAssetRef` that `CRenderEnvironment.hdri` names. The directional light casts a soft shadow map, and a half-resolution SSAO pass runs when the GL context has depth textures. Cloth stays on Lambert (no material component). Banded shade and Bayer dither remain fields on `CLight` for entities without a material.

`--present` opens the window for a few frames and exits non-zero if the surface shader fails to compile or an albedo / HDRI path is missing.

`CSimShape::Box` is reserved in **rigJolt** and is not spawned.

## Build

```bash
cmake -S examples/example-stage -B examples/example-stage/build
cmake --build examples/example-stage/build --target example-stage
cmake --build examples/example-stage/build --target example-stage-play
cmake --build examples/example-stage/build --target example-stage-tests
ctest --test-dir examples/example-stage/build --output-on-failure
```

```bash
./examples/example-stage/build/bin/example-stage
./examples/example-stage/build/bin/example-stage --play
./examples/example-stage/build/bin/example-stage-play
./examples/example-stage/build/bin/example-stage --smoke
./examples/example-stage/build/bin/example-stage-play --present
```

`--play` on the editor executable skips editor chrome. `example-stage-play` is the build that does not link **rigImGui** or **rigMeshEdit**.

`--smoke` is headless. It checks the feature scene, steps physics, and runs `data/play.py` through every `stage` call. CPython Development.Embed has to be found at configure time or smoke fails. `example-stage-tests` does not need Python or a GPU. It covers prefab spawn, destroy, and the pose guard.

First configure compiles Jolt and Assimp. That is heavy. Later edits stay in this folder.

## Packs

| Pack | Role |
|------|------|
| **rigComponent** / **rigSystems** | Transform, camera, light, mesh, hierarchy |
| **rigRender3D** | GLES mesh present |
| **rigSimComponent** / **rigPhysics** / **rigJolt** | World, plane, sphere, cloth |
| **rigAssimp** | glTF (and the other formats that build enables) into `CMesh` |
| **rigPython** | `stage` module, `update(dt)` from `CStageScript` |
| **rigMeshEdit** / **rigImGui** | Editor only. Gizmo on the selected prop |

`CStagePrefab` and `CStageScript` live in this example. They are app data until a second host needs them.

## Pi

The present is the Pi GLES path. Jolt here is one small cloth (8 by 5) plus two spheres; soft bodies are single-threaded. **rigAssimp** is not the Pi-default mesh path. Embedded CPython is desktop-first and stays off when the embed is missing.

## Agent

Edit `data/play.py` and `data/meshes/`. Recipes are `CStagePrefab` entities (`name`, `radius`, `mass`, color). Run `--smoke` after a script change. Guidelines: [AGENTS.md](AGENTS.md).
