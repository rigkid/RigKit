# Pack public graduation

Living checklist for [packs/README.md](../packs/README.md) **Visibility**. Private until green pack CI, useful example (or data contract), and safe for outside clones; then public + [`docs/api/pack-remotes.txt`](api/pack-remotes.txt).

Score snapshot: 2026-11-18. Re-score after each wave.

## Tiers

| Tier | Meaning | Next level |
|------|---------|------------|
| **far** | Missing CI and/or docs.yml and/or example (code) / LICENSE | Scaffold CI+docs from `templates/rigTemplate`; add example or confirm data; LICENSE |
| **near** | CI+README but missing example | Add `examples/example-<Name>` (or reclassify as data) |
| **close** | Structurally ready but private deps | Wait for trunk public, then re-score |
| **ready-red** | Checklist OK, CI red | Fix CI (token / apt / example path) |
| **ready-green** | CI green, public deps only | Enable Pages, flip public, add to `pack-remotes.txt` |
| **public-gap** | Already public, not in aggregate | Append remotes only |

## Shared blockers

1. Older pack `ci.yml` hard-fails without `RIGKIT_CI_TOKEN`. Host RigKit is public — use template warn + `secrets.RIGKIT_CI_TOKEN || github.token`. Private sibling deps still need a **repo** secret on GitHub Free.
2. Wayland apt gap (`wayland-scanner`) — install `libwayland-dev libwayland-bin libxkbcommon-dev`.
3. Docs deploy 404 until Pages source is **GitHub Actions** (one-time per remote).

## Waves

### Wave 0 — Shared infra

Sync private pack CI/docs to template patterns (optional token, wayland apt). Repo secrets where private deps remain.

### Wave 1 — Public gap + trunks

**1a public-gap (remotes only):** `rigAbletonLink`, `rigArtboards`, `rigCadComponent`, `rigDocumentShell`, `rigNetScan`.

**1b flip after CI green:** `rigSerial`, `rigPlotComponent`, `rigNode`, `rigColorspace`, `rigTrueType`, `rigFontComponent`, `rigBlend2D`, `rigMeshEdit`, plus already-green ready isolates `rigMarkdown`, `rigSQLite`, `rigJoystick`.

### Wave 2 — Unblocked leaves

| After public | Graduate |
|--------------|----------|
| rigSerial | rigGrbl |
| rigPlotComponent | rigGCode, rigCompositor, rigPlotFinders, rigPlotProcessors, rigPlotGenerators; then rigVectorEditor (needs TrueType); then rigPainty |
| rigTrueType | rigImTui |
| rigColorspace | rigPdfWrite |
| rigNode + rigMusicComponent | rigAudio → rigDexed |

### Wave 3 — Remaining READY isolates

`rigAgentty`, `rigEthereum`, `rigManifold`, `rigOpenCV`, `rigPython`, `rigPixelPlotComponent`, `rigSolveSpace`, `rigTouch`, `rigVaseRenderer` (+ any Wave 1b still red once fixed).

### Wave 4 — FAR / NEAR +1 tier

**NEAR:** `rigPixelPlotterUi` — add `examples/example-*`.

**FAR (scaffold CI+docs; public-deps potential):** `rigComposition`, `rigMusicComponent`, `rigPrintComponent`, `rigShowComponent`, `rigSimComponent`, `rigStoryComponent`, `rigDmx`, `rigScreenCast`, `rigPdfRead`, `rigAtlas`, `rigVarFont`, `rigVideo`, `rigPsd`, `rigCast`, `rigSlice`, `rigAssetLib`.

**FAR (scaffold CI; wait on parents):** `rigLayout`, `rigLayoutComponent`, `rigFont`, `rigUfo`, `rigJolt`, `rigPhysics`, `rigEnttec`, `rigSacn`, `rigTrace`, `rigTraceUi`, `rigAudioUi`, `rigAssetUi`, `rigOpen303`, `rigMidi`.

### Wave 5 — Product stacks

`rigPlotter` → `rigPlotterUi` / `rigVectorUi` → `rigPixelPlotter` (+ Ui). Update catalog wording; host `pack-remotes.txt`.

## Per-pack next action (2026-11-18)

### public-gap

| Pack | Next |
|------|------|
| rigAbletonLink | Append remotes |
| rigArtboards | Append remotes |
| rigCadComponent | Append remotes |
| rigDocumentShell | Append remotes |
| rigNetScan | Append remotes |

### READY (public deps)

| Pack | Next |
|------|------|
| rigSerial | CI green → Pages → public → remotes (Wave 1b) |
| rigPlotComponent | Fix wayland apt → CI green → public (Wave 1b) |
| rigNode | CI green → public (Wave 1b) |
| rigColorspace | CI green → public (Wave 1b) |
| rigTrueType | CI green → public (Wave 1b) |
| rigFontComponent | CI green → public (Wave 1b) |
| rigBlend2D | Sync CI token pattern → green → public (Wave 1b) |
| rigMeshEdit | CI green → public (Wave 1b) |
| rigMarkdown | Already CI green → public (Wave 1b) |
| rigSQLite | Already CI green → public (Wave 1b) |
| rigJoystick | Already CI green → public (Wave 1b) |
| rigAgentty | Wave 3 |
| rigEthereum | Wave 3 |
| rigManifold | Wave 3 |
| rigOpenCV | Wave 3 (no recent runs — trigger CI) |
| rigPython | Wave 3 |
| rigPixelPlotComponent | Wave 3 |
| rigSolveSpace | Wave 3 |
| rigTouch | Wave 3 |
| rigVaseRenderer | Wave 3 |

### CLOSE

| Pack | Blocked on | Next |
|------|------------|------|
| rigGrbl | rigSerial | Wave 2 |
| rigGCode | rigPlotComponent | Wave 2 |
| rigCompositor | rigPlotComponent | Wave 2 |
| rigPlotFinders | rigPlotComponent | Wave 2 |
| rigPlotProcessors | rigPlotComponent | Wave 2 |
| rigPlotGenerators | rigPlotComponent, rigVectorEditor | Wave 2 |
| rigVectorEditor | rigPlotComponent, rigTrueType | Wave 2 |
| rigPainty | plot + finders | Wave 2 |
| rigImTui | rigTrueType | Wave 2 |
| rigPdfWrite | rigColorspace | Wave 2 |
| rigAudio | rigNode, rigMusicComponent | Wave 2 after music |
| rigDexed | rigAudio | After audio |
| rigPlotter | plot family | Wave 5 |
| rigPlotterUi | plotter + grbl + … | Wave 5 |
| rigVectorUi | plotter + editor | Wave 5 |
| rigPixelPlotter | many | Wave 5 |

### NEAR / FAR

See Wave 4 list. Next level for each: add CI+docs (and example/LICENSE where noted); do not flip public until dep closure is public.

## Execution rules

- One wave at a time; systemic CI fixes go into the template then roll.
- No force-push.
- Visibility flip only after **CI success** on default branch; Docs/Pages may lag one commit.
- Skip `RigVJ` / `RigVJ-Content`.
