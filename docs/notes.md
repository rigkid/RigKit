# Notes

Running list of things that need attention later: bad or duplicate code, and code that does not line up with the vision. This is not a plan. Work items belong in [pack_boundary_plan.md](pack_boundary_plan.md) or [TODO.md](TODO.md). Entries here are observations with enough context to act on later.

Add an entry when you find something outside the change you are making. Do not fix unrelated entries in passing. Remove an entry when it is fixed or promoted into the plan.

## CI and build

- `tools/check-invariants` is red before any boundary work: three pack READMEs embed a missing `preview.png` (also tracked in [TODO.md](TODO.md)), and `docs/contract/port-map.md` has no rows for the RigWorks `rig.synth.*` and `rig.modular.*` schema ids. Treat this as the red baseline until each is fixed where it lives.
- Host `develop` pins submodule commits that are not on the pack remotes (`docs/contract/RigWorks` `f279bfe`, `packs/rigComponent` `8d98451`, and likely the other basics). A fresh `git submodule update --init` fails with "not our ref". The pack commits must be pushed in the pack repos before the host pointer bumps can land; do not let host pins run ahead of pack remotes again.

- `rigImGui` style CI fails on clang-format across many `src/` and `examples/` files. Separate formatting workstream; do not mix it into boundary changes.
- Actions secret `RIGKIT_CI_TOKEN` is present for some pack repos and missing for others (for example `rigRender3D` failed at the token gate). Verify per repository in repository settings; there is no code fix.
- `rigAudio` example configure fails fetching private packs over FetchContent without credentials. Pack examples should prefer packs already present in the host tree.

## Boundary observations not yet scheduled

- `rigShowComponent` files codecs under ids such as `rig.visualizer.deck`. Check every show codec id against the RigWorks catalog before Phase 4; the cad id fault may not be the only invented id.
- `CPathGpuCache` in `rigPlotComponent` holds dirty flags and counts, no GPU handle. The name promises GPU state the POD does not have; rename when next touching that code.
- Some data packs ship examples that present UI (`rigComponent`, `rigShowComponent`, `rigComposition`, `rigStoryComponent`). Examples in data packs may stay, but new teaching examples belong to code packs.
- Host `CEvent` carries a `std::any` payload. Known impurity, documented in the data skill. Do not widen it; new components stay plain data.

## Code to revisit

- `contractRunAction` was removed from `rigProject` Contract import in Phase 1: it had no caller in this repo, and the live UI path triggers actions through the ECS event system (`SEvent` / `triggerAction`). Out-of-tree hosts that called it should register the action with the event system and trigger it there.
- `packs/rigManifold/examples/cad` does not build against the current host: `rigMeshEdit` and the cad app call `IMui` / `Mui` members that no longer exist (`getWindowManager`, `registerViewSubmenu`). Pack and host UI APIs have drifted; reconcile in the packs where the calls live.
- The `Mui` API dropped the free `windowManager(IMui*)` helper without a caller sweep: `rigNodeEditor` (pack + `PatchBayWindow`) and the node examples failed to compile against current `rigImGui` until Phase 2 fixed them locally (`dynamic_cast<Mui*>` then `getWindowManager()`). Worst remaining case: `rigPlotterUi` (`rigPlotterUi.cpp` calls removed `IMui` members dozens of times, including the renamed `register*Submenu` family). Other packs may carry the same stale calls; grep before trusting a pack build.
- `rigNode` is a new pack repo, local only. Create `rigkid/rigNode` on GitHub and push before any pack CI run that fetches `rigNode` by URL (`rigNodeEditor`, `rigAudio`, `rigMidi`, `rigPixelPlotter` manifests now name it).
- `rigProjectComponent` is a new pack repo, local only. Create `rigkid/rigProjectComponent` on GitHub and push before pack CI fetches it by URL (`rigProject` manifest and the document example now name it).
- `packs/rigComponent/examples/pixel/app.cpp` does not compile against current headers: it calls `MEcs` template members with only a forward declaration in scope (`rigProject.h` no longer pulls `ecs/MEcs.h` transitively, if it ever did). `PixelDoc.cpp`, the TU that actually consumes `CPage`, compiles fine. Fix the example's includes where it lives; not caused by the Phase 3 header move (the headers are byte-identical, only their home changed).
- `packs/rigPdf` and `packs/rigPdfWrite` are two working copies of the same pack repo in the host tree (same HEAD, both `pack.json` name `rigPdfWrite`). One id means one folder: delete the `rigPdf` copy or rename the folder to match; anything resolving packs by folder name sees a phantom pack today. `packs/rigLayout/examples/layout` still includes `packs/rigPdf/src/rigPdf.h`, which exists in neither copy.
- `packs/rigStoryComponent/examples/story` does not compile: `MEcs` incomplete type throughout, `StoryRun` / `StoryCell` brace initializers no longer match the PODs, and the stale `IMui::getWindowManager` call. The example has rotted against its own data pack; repair it in `rigStoryComponent` or retire it. The pack libraries build fine.
- Layout and story serializers keep their pending Contract references in a process-static vector per file (cleared on codec registration). Sequential imports behave; two hosts in one process would share the stash. Instance-owned state belongs on the registry if this ever bites.
- `packs/rigSystems/examples/present` does not build: `app.cpp` uses `spdlog` without declaring it. The `rigSystems` library itself builds and passes contract smoke; the example needs its include fixed where it lives.
- `rigSystems` files the anim codecs in setup whether or not `rigAnimComponent` is loaded. The deserializers then create anim components that reflection (`contractGetString`, Properties) cannot see until the data pack registers them. Real apps get the registration through the pack dependency; the spine contract-smoke fixture does not. Consider having codec filing follow the data pack, or document that codec packs must be loaded with their data pack.

## Decisions still open inside the plan

- Font codec home decided in Phase 4: the `rig.font.*` codecs live in `rigFont` (the font engine). `rigUfo` stays UFO format IO; its round-trip example never touches `.rig`, so nothing lost IO in the move.
- Cast PODs decided in Phase 5: floor (`rigComponent`) - generic scene data, two independent consumers. Written in `packs_using.md`.
- `rigColorspace` and `rigComposition` keep their shipped ids (Phase 5 audit: both are domain data homes; rename cost is every consumer manifest, benefit is symmetry). Written in `packs_using.md`.
