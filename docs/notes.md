# Notes

Running list of things that need attention later: bad or duplicate code, and code that does not line up with the vision. This is not a plan. Work items belong in [pack_boundary_plan.md](pack_boundary_plan.md) or [TODO.md](TODO.md). Entries here are observations with enough context to act on later.

Add an entry when you find something outside the change you are making. Do not fix unrelated entries in passing. Remove an entry when it is fixed or promoted into the plan.

## CI and build

- `tools/check-invariants` is red before any boundary work: three pack READMEs embed a missing `preview.png` (also tracked in [TODO.md](TODO.md)), and `docs/contract/port-map.md` has no rows for the RigWorks `rig.synth.*` and `rig.modular.*` schema ids. Treat this as the red baseline until each is fixed where it lives.

- `rigImGui` style CI fails on clang-format across many `src/` and `examples/` files. Separate formatting workstream; do not mix it into boundary changes.
- Actions secret `RIGKIT_CI_TOKEN` is present for some pack repos and missing for others (for example `rigRender3D` failed at the token gate). Verify per repository in repository settings; there is no code fix.
- `rigAudio` example configure fails fetching private packs over FetchContent without credentials. Pack examples should prefer packs already present in the host tree.

## Boundary observations not yet scheduled

- `rigShowComponent` files codecs under ids such as `rig.visualizer.deck`. Check every show codec id against the RigWorks catalog before Phase 4; the cad id fault may not be the only invented id.
- `CPathGpuCache` in `rigPlotComponent` holds dirty flags and counts, no GPU handle. The name promises GPU state the POD does not have; rename when next touching that code.
- Some data packs ship examples that present UI (`rigComponent`, `rigShowComponent`, `rigComposition`, `rigStoryComponent`). Examples in data packs may stay, but new teaching examples belong to code packs.
- Host `CEvent` carries a `std::any` payload. Known impurity, documented in the data skill. Do not widen it; new components stay plain data.

## Decisions still open inside the plan

- Font codec home (`rigFont` or `rigUfo`) is decided in Phase 4 from consumer evidence, per the hybrid codec policy.
- `CCadDimension` reference fields join the Contract fixup hook only after the RigWorks schema semantics for `a` and `b` are confirmed.
- Cast PODs (`CScreenCast`, `CScreenCastReceive`, `CCastReceiver`) get a floor-or-domain decision in Phase 5.
- `rigColorspace` and `rigComposition` do not follow the `Component` naming convention. Audit in Phase 5 with consumers first; do not rename for symmetry alone.
