# Pixel stack alignment plan

Living checklist for getting the pixel editor stack in line: every capability
owned by the right pack, every heavy or GPL dependency an explicit app opt-in,
paint / crop / undo honest (nothing pretends to work), and a running gate
behind each claim.

Scope: `rigPixelPlotComponent` (data), `rigPixelPlotter` (engine),
`rigPixelPlotterUi` (chrome), `rigVideo`, and their consumers
(`examples/example-pixelEditor`, `examples/example-studio`, PixelPlotter,
RigVJ). Rules: [contract/commandments.md](contract/commandments.md).

Status snapshot: 2026-11-19. Re-score after each phase.

## Done before this plan

- GPU brush stamp (`GpuPingPong::stampBrushToTexture`), dirty-rect layer
  uploads, composite-only repaint (`PixelDoc::recomposite`).
- Canvas crop tool (`PixelDoc::cropCanvas`, `tool.crop`).
- `yt-dlp` / `qjs` page-URL tools opt-in (`RIGVIDEO_FETCH_TOOLS=OFF` default
  in rigVideo); orphan `rigImage` removed from `example-pixelEditor`.

## Status 2026-11-19 (Phases 0-4 landed, 5 in progress)

- Phase 0: done, committed per repo.
- Phase 1: done except three deferrals, kept visible here rather than
  silently dropped:
  - 1.5 `rigVideo` header seam (`PixelDoc.h` forward-declares webcam types,
    `RIGKIT_HAS_RIGVIDEO`): deferred. `PixelDoc.h` still includes
    `VideoHub.h` publicly, so rigVideo stays a core link. Do it as its own
    change; it touches every PixelDoc consumer's include graph.
  - 1.6 Ui optionals (`RIGKIT_HAS_RIGSVG` / `RIGKIT_HAS_RIGVECTOR`):
    deferred. `rigSvg` / `rigVectorEditor` stay core deps of
    rigPixelPlotterUi; nothing is blocked on splitting them out.
  - 1.4b `AudioBus` as its own pack: deferred. The `rigNodeBus` leaf target
    inside rigNode already keeps the node graph out of pixel binaries;
    extracting a separate pack is a repo-creation decision, not a code one.
  Landed: `rigNodeBus` leaf; `rigPainty` GPL firewall and `rigObj` as opt-in
  leaves (`RIGKIT_HAS_RIGPAINTY` / `RIGKIT_HAS_RIGOBJ`); consumer manifests
  updated (studio, pixelui example, RigVJ opt in explicitly).
- Phase 2: done. Stroke + crop raster undo (`RasterPatch` /
  `CropSnapshot`), byte-budgeted `UndoStack` (64 MB in the Ui). Still open:
  feeder `fillRegion` / `paintSbr` do not push raster records - they ride
  the POD snapshot, so project undo after a feeder fill restores structure,
  not those pixels. Fix in the pack or stop offering undo there.
- Phase 3: done with one deliberate deviation from the text below: pages
  are NOT all resized. Pages are independent artboards with their own
  sizes; only the active page bridges to the canvas
  (`syncActivePageFromCanvas`). Regions (`fillXy` / outline) shift and
  off-size layer rasters crop proportionally, as specified.
- Phase 4: done. `tipPath` honored (CPU + GPU stamp, cached pack-side),
  `angleJitter` + seeded per-stroke RNG (no more `std::rand`), eraser
  (`tool.eraser`, dest-out both paths), Brushes panel gained flow / scatter /
  angle-jitter controls and the tip name.
- Phase 5: in progress. `video_smoke` fixed in rigVideo (Media Foundation
  display-aperture size; stepped `update(0)` / `readNext` no longer drop
  every other frame to the pump race) and passes standalone. Host CI gains
  an `example-pixelEditor` matrix leg with an xvfb `--smoke`;
  rigPixelPlotter CI gains hero + pixelEditor smokes under xvfb and a
  pixelEditor build. Two pre-existing GCC/Linux breaks fell out of wiring
  that up and were fixed where they live (lambda/`nullptr` ternary in
  `ImageFilters.cpp`; MSVC-only `reinterpret_cast<ImTextureID>` in
  rigPixelPlotterUi). The full pixelEditor tree builds and `--smoke` passes
  on Linux (WSL, Mesa) - the same llvmpipe path the CI smoke uses. Docs
  synced (pack READMEs, example README, packs catalog). Not yet run:
  GitHub CI itself, and Phase 6.
- Phase 6: not started. Pi risk stays open until the ANGLE build and a Pi
  run actually happen; nothing in Phases 0-5 was verified on arm64/GLES
  hardware.

## Phase 0 - Land the work above

- Install clang-format; run each pack's `tools/format.sh` (host format never
  touches `packs/`), then host `./tools/format.sh`.
- Build `example-pixelEditor`, run `--smoke`; confirm `bin/` ships no
  `qjs.exe` / `yt-dlp.exe`.
- One commit per pack repo, then the host. No trailers.

Gate: all four trees clean, smoke green.

## Phase 1 - Dependency alignment

Pattern: copy the existing `rigPsd` optional (`RIGKIT_HAS_RIGPSD` + `#ifdef`).
Optional defaults stay ON while splitting so PixelPlotter / RigVJ do not
break; thin examples then opt OFF explicitly in `app.json`.

| # | Change | Notes |
|---|--------|-------|
| 1.1 | Manifest sync: `rigPsd` into `rigPixelPlotter/pack.json`; drop redundant `rigProject` from `rigPixelPlotterUi/pack.json` | Manifests must match CMake |
| 1.2 | `rigObj` optional (`RIGKIT_HAS_RIGOBJ`): `ObjGen`, `kPixelFxObj`, Ui model preview | One GENERATE step + one dock |
| 1.3 | GPL firewall: remove PUBLIC `rigPainty` link from MIT `rigPixelPlotter`; CPU `flowBasedDoG` behind `RIGKIT_HAS_RIGPAINTY`, GPU Painty stays in-core | Today every pixel binary inherits GPL via Painty + rigPlotFinders for one effect fallback |
| 1.4 | Slim `rigNode`: move `AudioBus` to `rigNodeComponent` or a tiny MIT leaf; plotter links only that | Six files use `AudioBus.h` only; no app registers `rigNode` |
| 1.5 | `rigVideo` seam: forward-declare webcam types in `PixelDoc.h` (`VideoHub.h` moves to `.cpp`), then `RIGKIT_HAS_RIGVIDEO` for source kinds, `kPixelFxVideo`, `kPixelFxWebcam`, plates | Also `WebcamCapture.h` in `PixelPlotterPanel.h` |
| 1.6 | Ui optionals (`RIGKIT_HAS_RIGSVG`, `RIGKIT_HAS_RIGVECTOR`), mirroring `RIGKIT_HAS_RIGAUDIO` | SVG path load, curve editors, model preview |
| 1.7 | Consumer manifests: pixelEditor / studio opt OFF explicitly; PixelPlotter / RigVJ opt ON explicitly | Opt-in visible in `app.json`, not inherited |

Gate: build matrix - pixelEditor, studio, `example-pixel`, `example-pixelui`,
`pixeldata`, `video`, PixelPlotter `--smoke`, RigVJ. Success metric:
pixelEditor's deployed `data/packs/` list matches what it uses.

## Phase 2 - Honest raster undo

`ProjectState` snapshots are POD-only, so undo after paint / crop / feeder
fill restores structure and silently loses pixels. Fix in the pack, never as
ECS POD:

- Stroke undo: at stroke end (after `syncPaintLayer`), one `UndoStack`
  record per stroke with before/after dirty-rect RGBA of the painted layer.
- Crop undo: snapshot canvas size, affected rasters, mask-path offsets.
- Undo the auto-created Paint layer on first dab.
- Feeder `fillRegion` / `paintSbr`: raster records, or stop claiming
  project undo.
- Pi memory policy: byte-budgeted history, not only a record count.

Gate: smoke - paint, undo, raster matches original; crop, undo, size and
pixels restored.

## Phase 3 - Crop completeness

`cropCanvas` today handles canvas-sized rasters and non-normalized mask
paths only:

- Shift `CPixelRegion` fill geometry (`fillXy` / `fillCounts`).
- Resize every page in `CPixelPages`, not only the active one.
  ( Landed differently: pages keep their own sizes - see Status. )
- Non-canvas-sized layer rasters: crop proportionally or refuse loudly.
- Audit color-select PODs and step-mask references after crop.

## Phase 4 - Brush completeness

- `CPixelBrush::tipPath` honored: tip PNG loaded relative to
  `CPixelBrushLibrary::packDir`, tip textures cached pack-side (never in
  POD); CPU stamp samples tip alpha; GPU tip variant of the stamp shader
  (scale + rotation). Until this lands, tip brush packs are a format
  contract with no fulfillment.
- `angleJitter` plus a seeded per-stroke RNG (replaces `std::rand`).
- Eraser tool (`tool.eraser`, dest-out on CPU and GPU).
- Brushes UI: scatter, flow, angle jitter, tip preview.

## Phase 5 - Gates and docs

- Pack CI builds heroes but never runs `--smoke`: add a smoke step
  (`example-pixel --smoke` extended with paint dab + crop round-trip).
- Host CI matrix gains `example-pixelEditor` build + smoke.
- Fix `video_smoke` in rigVideo standalone (exe-relative tool lookup, MF
  frame-size check) - in the pack repo, where it lives.
- Sync docs: `docs/packs_catalog.md`, `docs/apps.md`, `docs/authoring.md`,
  pack READMEs (dependency + optional flags), `example-pixelEditor/README.md`.

## Phase 6 - Pi verification

- Desktop GLES-parity build (`-DRIGKIT_USE_ANGLE=ON`) of the pixelEditor
  tree, then Pi build + run per `docs/contract/pi-host.md`.
- Perf: large-brush stroke frame time and undo memory at 320x240 and
  1920x1080 canvases, before/after Phases 1 and 4.

## Ordering

Phase 0, then 1 (GPL firewall first inside the phase), then 2, 3, 4 in any
order, then 5, 6. One PR per affected pack repo plus a host manifest/docs
PR per phase; the Phase 5 matrix is the test plan.
