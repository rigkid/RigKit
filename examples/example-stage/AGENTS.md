# Stage

Load [the repo AGENTS.md](../../AGENTS.md) first. This file is the Stage-specific guide.

Stage is the host example `examples/example-stage`. It is not a second engine. ECS, present, physics, mesh import, Python, and the gizmo already have packs. New code here wires them and owns only the two app components.

## Ownership

| Need | Owner |
|------|--------|
| Entities, transform, camera, light, mesh, selection | **rigComponent** + host ECS |
| Hierarchy, host systems | **rigSystems** |
| Draw | **rigRender3D** (`SMeshPresent3D`) |
| Gravity, plane, sphere, cloth recipes | **rigSimComponent** |
| Step | **rigPhysics** `SPhysics` |
| Solver | **rigJolt** |
| glTF / Assimp import | **rigAssimp** |
| Scripts | **rigPython**, module injected by `StageBindings.cpp` |
| Gizmo, Mesh Edit | **rigMeshEdit** on **rigImGui** |
| Prefab recipe, script path | `CStagePrefab`, `CStageScript` in this example |

Do not add a renderer, an ECS, a physics world, or a Lua runtime in this folder. A defect in a pack is fixed in that pack.

## Present limits

**rigRender3D** lights one enabled `CLight`: directional or point, smooth Lambert or banded shade, optional Bayer dither. Albedo is `CMesh` face color, `CDrawStyle` fill, or an atlas. UVs on `CMesh::texcoords` are ignored until the scene has a `CIndexedAtlas`.

Do not stub, and do not start, any of these inside Stage:

- PBR metallic-roughness
- IBL from Poly Haven HDRIs
- Soft shadow maps
- SSAO
- HDR framebuffer and tonemapping

They are not the GLES2 Pi path. A private desktop renderer would be a second present.

`CSimShape::Kind::Box` is reserved. **rigJolt** logs and does not build it. Do not spawn boxes.

## Pose

**rigJolt** writes `CTransform` for a dynamic sphere and writes cloth `CMesh` positions. `CRigidbody.velocity` is the launch velocity applied when the body is created. The solver does not copy the live speed back, and changing that field later does not steer the body.

`stage.set_position` refuses entities `sim_owns` reports (sphere with `CRigidbody` + `CSimShape`, or cloth). Camera, light, marker, and the static prop accept it. Spawn sets the launch pose before the first step.

## Script API

One enabled `CStageScript`. Path is relative to `<exeDir>/data`. Each Update loads the file when its mtime changes, then calls `update(dt)` in `__main__`.

| Call | Effect |
|------|--------|
| `find(name)` | Entity id, or `None` |
| `create(name)` | Named entity with a transform at the origin |
| `destroy(id)` | Destroy. Bad id raises |
| `count()` | Live entity count |
| `name(id)` | Name from `createEntity` |
| `position(id)` | `(x, y, z)` |
| `set_position(id, x, y, z)` | Writes transform. Raises `RuntimeError` with `sim owns pose` when the solver owns it |
| `velocity(id)` | Launch velocity on `CRigidbody`. Raises when there is no body |
| `spawn(prefab, x, y, z, vx=0, vy=0, vz=0)` | Dynamic sphere from `CStagePrefab.name` |
| `sim_owns(id)` | Bool |

`data/play.py` calls every one of these. `--smoke` runs that file for 120 steps at 1/60 s and checks the ball landed, the cloth moved, the marker moved, `temp` is gone, and a second rigidbody exists.

## Builds

- `example-stage` links the editor packs (`STAGE_WITH_EDITOR=1`).
- `example-stage --play` keeps that binary and does not register editor packs.
- `example-stage-play` compiles the same sources with `STAGE_WITH_EDITOR=0` and does not link **rigImGui** or **rigMeshEdit**. That is the distributable.
- `example-stage-tests` is doctest over `StageScene` (no Python, no GPU, no Jolt).
- `--smoke` is the integration test. It needs the CPython embed.

There is no Export menu. Shipping a game is: put the play in `data/play.py`, build `example-stage-play`, distribute that executable and the `data/` beside it.

## Making a game from here

1. Change `CStagePrefab` fields or add another recipe entity in `addFeatureActors` if the game needs a new ball recipe.
2. Put static meshes in `data/meshes` and load them with `addProp` / `rig::assimp::makeMeshFromFile`.
3. Move markers, cameras, and static props from `update(dt)`. Spawn and destroy bodies there. Do not `set_position` a live sphere.
4. Run `example-stage-tests` and `example-stage --smoke`.
5. Run `example-stage` to place the selected mesh with the gizmo. Run `example-stage-play` to confirm the editor is gone.

Keep the feature scene able to pass `--smoke`. A new system or panel starts as fields on a POD, then the thinnest code that reads them.

## Pi and rebuild

Pi floor is the GLES present plus a small Jolt scene. Do not widen this example onto a desktop-only GL profile. Assimp and CPython are already the heavy, optional leaves; do not add another.

Build this directory only: `cmake -S examples/example-stage -B examples/example-stage/build`. Do not add a `build-*` tree. First configure compiles Jolt and Assimp; say so when that cost shows up.
