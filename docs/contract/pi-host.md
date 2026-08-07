# Raspberry Pi — minimum full host

Per Contract + Host vision, **Raspberry Pi is the floor for a Rig + UI fulfillment** (host + UI, usually `rigImGui`, + Draw path). Show mode may run **Rig** only (no UI). If it does not run here, it is not the floor.

**Agent / contributor gate:** before finishing a change, ask (1) will this compile on Pi? (2) will this run smoothly on Pi? See [AGENTS.md](../../AGENTS.md).

## Requirements

- Build RigKit (`rigkit` library + examples) on Raspberry Pi OS (arm64 preferred).
- GL ES path: RigKit already selects `glad` GLES2 on `__arm__` / `__aarch64__` (see `RigKitEngine` init).
- At least one example (`examples/oscHost`) runs in **show** and **author** modes.
- Long-running Update/Draw loop suitable for media installs.

## Verify (on device)

```bash
git clone --recursive <rigkit-repo> && cd rigkit
cmake -S examples/oscHost -B examples/oscHost/build -DCMAKE_BUILD_TYPE=Release
cmake --build examples/oscHost/build -j$(nproc) --target oscHost
./examples/oscHost/build/bin/oscHost --show
# Ctrl+C / close window; then try author mode:
./examples/oscHost/build/bin/oscHost --author
# UDP OSC loopback (rigOsc):
./examples/oscHost/build/bin/oscHost --smoke-osc
```

Until hardware is available, desktop build of `examples/oscHost` is the integration smoke test.

### Plot pack family (optional product path)

Integration app: `packs/rigPlotter/examples/plot`. Each plot pack also has its own thin hero under `packs/<pack>/examples/<hero>/` (README screenshot). Headless unit smokes live under each pack’s `tools/*_smoke.cpp` and build with the plot tree (`plot_unit_smokes` target).

```bash
cmake -S packs/rigPlotter/examples/plot -B packs/rigPlotter/examples/plot/build -DCMAKE_BUILD_TYPE=Release
cmake --build packs/rigPlotter/examples/plot/build -j$(nproc) --target plot plot_unit_smokes
# Run smokes (paths next to the build tree; names are paths_smoke, svg_smoke, …):
find packs/rigPlotter/examples/plot/build -type f -name '*_smoke' -executable -print -exec {} \;
./packs/rigPlotter/examples/plot/build/bin/plot
# Per-pack hero (example):
cmake -S packs/rigSvg/examples/svg -B packs/rigSvg/examples/svg/build
cmake --build packs/rigSvg/examples/svg/build --target svg
```

Out-of-tree PaintPlotter uses the same packs; verify with its own CMake tree when shipping installs. `rigPlotFinders` vendors Potrace (**GPL-2.0-or-later**) — keep that license in the product binary notice. ANGLE is never required on Pi (native GLES).

## Verify log

| Date | Board | OS | Build | `--show` / smokes | `--author` / UI | Notes |
|------|-------|-----|-------|-------------------|-----------------|-------|
| _pending_ | | | | | | Run `examples/oscHost` on device |
| 2026-08-02 | — | desktop interim | OK | all `*_smoke` OK (Win) | PaintPlotter + `plot` rebuilt | **Pi blocked on hardware** this session — run plot family checklist above on arm64 when a board is available; do not treat desktop as a Pi pass |
| 2026-08-02 | — | desktop interim | OK | + gcode_import / contours / maze / toolpath3d smokes | Toolpath 3D Kit panel | v2 software closed (import + 20 finders + Toolpath 3D). **Pi/Grbl still blocked on hardware** |
| 2026-08-02 | — | desktop interim | — | — | PaintPlotter 1.0.0 ship bar | Live preview + envelope soft-check + LICENSE/NOTICE. **Pi/Grbl still blocked on hardware** |
| 2026-08-02 | — | desktop interim | — | + bezier_edit_smoke | Full bed bezier overlay | Main-viewport Direct Select / handles / Pen add-delete. **Pi/Grbl still blocked on hardware** |
| _pending_ | | | | | | Real Grbl soak (short job + Soft Reset / Unlock) — **blocked on controller** |

## Desktop GLES parity (ANGLE)


Optional: on **non-ARM** desktops, configure with `-DRIGKIT_USE_ANGLE=ON` (and `RIGKIT_ANGLE_ROOT` or vcpkg `angle`) so authoring can exercise an ES2 path closer to Pi. **Do not** enable ANGLE on Pi — CMake will error. Default desktop builds remain OpenGL; Pi builds always use native GLES.

## CI note

Where GitHub runners lack Pi hardware, treat this doc as the checklist and run the same CMake configure for `aarch64` cross or native self-hosted runners when available. Optional ANGLE job (`angle-gles` in `.github/workflows/ci.yml`): configure with `RIGKIT_USE_ANGLE=ON` only when `RIGKIT_ANGLE_ROOT` points at a prebuilt drop on the runner; skips otherwise (not required for merge). Never enable on ARM.
