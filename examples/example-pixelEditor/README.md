# example-pixelEditor

Layered raster editor on **rigPixelPlotter** + **rigPixelPlotterUi**. Paint on overlay layers with Rig brush packs. File Open takes a still image (PNG, JPEG, BMP, TGA, GIF, HDR, PIC, PNM), a layered 8-bit RGB PSD via **rigPsd**, or a `.rig` document. Save still writes `.rig`; Save PSD writes the layers. File New / Open open parallel documents via `MEcs::exchangeDocument` (one live world for Update/Draw; parked sessions keep their own `PixelDoc` + GPU).

Not a Photoshop clone. The Pixel workspace docks Main View in the center, with Layers, Brushes, Colour, and Properties. Canvas, Resources, Source, and Projection stay in the Windows menu. The document stays.

## Build

```bash
cmake -S examples/example-pixelEditor -B examples/example-pixelEditor/build
cmake --build examples/example-pixelEditor/build --target example-pixelEditor
./examples/example-pixelEditor/build/bin/example-pixelEditor
./examples/example-pixelEditor/build/bin/example-pixelEditor --smoke
```

## Tools

Main View tool rail groups similar tools into flyouts (hold or right-click):
paint/eraser, marquee select (rect/ellipse/lasso/wand), and move/transform.
Crop (C) stays alone. Marquee selection draws marching ants (outline), not a
filled tint. Crop shows a box you can move and resize by the corners. Type a
proportion such as 3:1, or a width and height in pixels. Enter or Apply commits
it. Image > Rotate Canvas 90 CW/CCW (Ctrl+] / Ctrl+[) spins the whole document
and swaps width/height; Rotate Layer keeps canvas size. Undo covers paint,
selection, paste, crop, canvas/image size, and canvas rotate (64 MB byte budget).

## Brushes

Load a folder that contains `brushes.json` (File-style dialog on the Brushes panel, or drop under `data/brushes/`). A brush `tipPath` PNG (relative to the pack folder) is honored on CPU and GPU, with rotation, scatter, flow, and angle jitter; a missing tip falls back to a procedural round.

True Grit Texture Supply partnership is a **Rig SKU** (same idea as their Procreate sets). Do not install their Photoshop `.abr` packs here.

## Packs

- **rigPixelPlotComponent** - layer / canvas / brush POD
- **rigPixelPlotter** - PixelDoc, paint, PNG / PSD map
- **rigPixelPlotterUi** - docks (Main View, Layers, Brushes, Colour)
- **rigPsd** - layered 8-bit RGB IO
