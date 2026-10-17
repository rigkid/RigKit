# example-pixelEditor

Layered raster editor on **rigPixelPlotter** + **rigPixelPlotterUi**. Paint on overlay layers with Rig brush packs. Open and save 8-bit RGB PSD via **rigPsd**. Document path is `.rig`.

Not a Photoshop clone. The Pixel workspace docks Main View in the center on load; the document stays.

## Build

```bash
cmake -S examples/example-pixelEditor -B examples/example-pixelEditor/build
cmake --build examples/example-pixelEditor/build --target example-pixelEditor
./examples/example-pixelEditor/build/bin/example-pixelEditor
./examples/example-pixelEditor/build/bin/example-pixelEditor --smoke
```

## Brushes

Load a folder that contains `brushes.json` (File-style dialog on the Brushes panel, or drop under `data/brushes/`). Tip PNGs are optional; missing tips use a procedural round.

True Grit Texture Supply partnership is a **Rig SKU** (same idea as their Procreate sets). Do not install their Photoshop `.abr` packs here.

## Packs

- **rigPixelPlotComponent** - layer / canvas / brush POD
- **rigPixelPlotter** - PixelDoc, paint, PNG / PSD map
- **rigPixelPlotterUi** - docks (Main View, Layers, Brushes)
- **rigPsd** - layered 8-bit RGB IO
