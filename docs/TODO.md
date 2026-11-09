# TODO

Code packs should ship one teaching example. Data packs do not need one. Rules: [packs/README.md](../packs/README.md#pack-example-readme-screenshot).

A data pack id ends in `Component`. Shipped data packs that do not: **rigProject**, **rigColorspace**, **rigComposition**. Leave those ids. Do not add a host app to a data pack to clear this list. Demos already in a data pack (including `rigCadComponent/examples/cadBox` and `rigMusicComponent/examples/clock`, which have no screenshot) can stay.

`tools/check-invariants` fails only when a pack README embeds `preview.png` and that file is missing. Missing examples are not a gate. No placeholder images.

## README links a missing `img/preview.png`

The gate fails on these. Add the PNG, or remove the image line until the shot exists.

| Pack | Example |
|------|---------|
| rigSQLite | `examples/sqlite` |
| rigMarkdown | `examples/markdown` |
| rigImage | `examples/image` |

## Example exists - capture `img/preview.png`

Real window, then `examples/<name>/img/preview.png`. Lead the pack README with `![preview](examples/<name>/img/preview.png)` once the file exists.

| Pack | Example |
|------|---------|
| rigAgentty | `examples/agenttyDemo` |
| rigAcp | `examples/acpDemo` |
| rigAbletonLink | `examples/link` |
| rigPdfWrite | `examples/pdf` |
| rigSolveSpace | `examples/distance` |
| rigImTui | `examples/imtui` |
| rigLayout | `examples/layout` |
| rigMidi | `examples/MidiEditor` |

## No example yet

Prefer a thin app for a pack with a window. A headless pack can ship a smoke and skip the screenshot.

| Pack | What to show |
|------|----------------|
| rigDocumentShell | Viewer/Player chrome: File Open, skipped keys |
| rigPixelPlotterUi | Kit panels (product app is out-of-tree PixelPlotter) |
| rigAudioUi | Time ruler, clip lanes, sampler |
| rigAssetUi | Content browser (host owns the dock) |
| rigCast | App-to-app AV peer |
| rigSlice | Desktop FGF CLI / smoke (not a Pi GUI) |
| rigPsd | Smoke that reads and writes a layered RGB fixture. No window required |
| rigAssetLib | Catalog smoke. No window; **rigAssetUi** is the picture |

## Note only - trunk example already shows the wire

| Pack | Note |
|------|------|
| rigSacn | `examples/sacn` is a note, not an app. `packs/rigDmx/examples/dmx` is the window |
| rigEnttec | `examples/enttec` is a note, not an app. Same dmx demo |

