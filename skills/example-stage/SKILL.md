---
name: example-stage
description: >-
  Stage host example (examples/example-stage). Use when editing Stage, its
  play script, prefab recipes, glTF prop, editor/play split, stage Python
  module, or --smoke / example-stage-tests. Covers which pack owns present,
  physics, import, and gizmos, and which renderer features Stage does not add.
---

# Stage

Read [examples/example-stage/AGENTS.md](../../examples/example-stage/AGENTS.md) and follow it. Repo rules stay in [AGENTS.md](../../AGENTS.md).

Stage is `examples/example-stage`. Compose packs. Do not add a second ECS, renderer, or physics world in the example.

- Present: **rigRender3D** Lambert (smooth or banded). No PBR, IBL, SSAO, shadow maps, or HDR tonemap in this example.
- Sim: **rigSimComponent** + **rigPhysics** + **rigJolt**. Spheres, planes, cloth. No boxes (`CSimShape::Box` is reserved).
- Import: **rigAssimp**. Gizmo: **rigMeshEdit** (editor target only).
- Scripts: **rigPython**, `stage` module, `data/play.py` `update(dt)`.
- App data: `CStagePrefab`, `CStageScript` only.
- Tests: `example-stage-tests` (doctest, no GPU) and `example-stage --smoke` (physics + the whole `stage` API). Both pass before calling the change done.
- Distribute with `example-stage-play`, which does not link the editor packs.

`set_position` must keep refusing a pose **rigJolt** writes. Launch velocity is not live steering.
