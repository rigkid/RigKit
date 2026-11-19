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
