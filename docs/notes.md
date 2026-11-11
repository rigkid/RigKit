# Notes

Running list of things that need attention later: bad or duplicate code, and code that does not line up with the vision. This is not a plan. Work items belong in [pack_boundary_plan.md](pack_boundary_plan.md) or [TODO.md](TODO.md). Entries here are observations with enough context to act on later.

Add an entry when you find something outside the change you are making. Do not fix unrelated entries in passing. Remove an entry when it is fixed or promoted into the plan.

## CI and build

- Host `develop` pins submodule commits that are not on the pack remotes (`docs/contract/RigWorks` `f279bfe`, `packs/rigComponent` `8d98451`, and likely the other basics). A fresh `git submodule update --init` fails with "not our ref". The pack commits must be pushed in the pack repos before the host pointer bumps can land; do not let host pins run ahead of pack remotes again.
- `rigImGui` style CI fails on clang-format across many `src/` and `examples/` files. Separate formatting workstream; do not mix it into boundary changes.
- Actions secret `RIGKIT_CI_TOKEN` is present for some pack repos and missing for others (for example `rigRender3D` failed at the token gate). Verify per repository in repository settings; there is no code fix.
- `rigAudio` example configure fails fetching private packs over FetchContent without credentials. Pack examples should prefer packs already present in the host tree.
- Capture real `examples/<name>/img/preview.png` for **rigSQLite**, **rigMarkdown**, and **rigImage** when those examples next run on a machine with a display; README embeds were removed so `tools/check-invariants` stays green (see [TODO.md](TODO.md)).

## Boundary observations not yet scheduled

- `CPathGpuCache` in `rigPlotComponent` holds dirty flags and counts, no GPU handle. The name promises GPU state the POD does not have; rename when next touching that code.
- Some data packs ship examples that present UI (`rigComponent`, `rigShowComponent`, `rigComposition`, `rigStoryComponent`). Examples in data packs may stay, but new teaching examples belong to code packs.
- Host `CEvent` carries a `std::any` payload. Known impurity, documented in the data skill. Do not widen it; new components stay plain data.

## Code to revisit

- `contractRunAction` was removed from `rigProject` Contract import in Phase 1: it had no caller in this repo, and the live UI path triggers actions through the ECS event system (`SEvent` / `triggerAction`). Out-of-tree hosts that called it should register the action with the event system and trigger it there.
- `rigNode` is a new pack repo, local only. Create `rigkid/rigNode` on GitHub and push before any pack CI run that fetches `rigNode` by URL (`rigNodeEditor`, `rigAudio`, `rigMidi`, `rigPixelPlotter` manifests now name it).
- `rigProjectComponent` is published and tracked as a host submodule. Nested CPM fetches still need `RIGKIT_CI_TOKEN` when a job does not check out that submodule.
- The duplicate `packs/rigPdf` checkout is deleted (same HEAD as `rigPdfWrite`); the layout example now names `rigPdfWrite` and passes `--smoke-layout` end to end. Any other machine with the phantom folder should delete it too - one id, one folder.
- Layout and story serializers keep their pending Contract references in a process-static vector per file (cleared on codec registration). Sequential imports behave; two hosts in one process would share the stash. Instance-owned state belongs on the registry if this ever bites.
- `rigSystems` files the anim codecs in setup whether or not `rigAnimComponent` is loaded. The deserializers then create anim components that reflection (`contractGetString`, Properties) cannot see until the data pack registers them. Real apps get the registration through the pack dependency; the spine contract-smoke fixture does not. Consider having codec filing follow the data pack, or document that codec packs must be loaded with their data pack.
- `packs/rigManifold/examples/cad` may still fail for reasons other than the old `IMui` / Submenu surface (that drift is cleared: pack sources and examples now use `Mui*` + `register*Action(..., group)`). Rebuild and note the next real error when next touching cad.

## Decisions still open inside the plan

- Font codec home decided in Phase 4: the `rig.font.*` codecs live in `rigFont` (the font engine). `rigUfo` stays UFO format IO; its round-trip example never touches `.rig`, so nothing lost IO in the move.
- Cast PODs decided in Phase 5: floor (`rigComponent`) - generic scene data, two independent consumers. Written in `packs_using.md`.
- `rigColorspace` and `rigComposition` keep their shipped ids (Phase 5 audit: both are domain data homes; rename cost is every consumer manifest, benefit is symmetry). Written in `packs_using.md`.
