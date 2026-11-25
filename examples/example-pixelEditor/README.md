# example-pixelEditor

Layered raster editor on **rigPixelPlotter** + **rigPixelPlotterUi**. Paint on overlay layers with Rig brush packs. File Open takes a still image (PNG, JPEG, BMP, TGA, GIF, HDR, PIC, PNM), a layered 8-bit RGB PSD via **rigPsd**, or a `.rig` document. Save writes `.rig`; Save As also offers PNG / JPG / BMP / TIFF (image picks export the composite and leave the document path alone). File > Export... is the same image path; Export PDF/X writes a single-page sheet via **rigPdfWrite**. Save PSD writes the layers. File New / Open open parallel documents via `MEcs::exchangeDocument` (one live world for Update/Draw; parked sessions keep their own `PixelDoc` + GPU).

Not a Photoshop clone. The Pixel workspace starts with no document; File New /
Open creates a document window in the center. Layers, Brushes, Colour, and
Properties stay docked. Canvas, Resources, Source, and Projection stay in the
Windows menu.

## Colour / print

Document mode is RGB (default) or native CMYK8 (Colour panel > Document colour).
CMYK needs a Dest Output Profile; GPU paint stays RGB-only and is gated off in
CMYK docs (CPU paint + CMYK composite, RGB preview on screen). Soft-proof is
optional on RGB docs. Profiles come from [GitBruno/myProfiles](https://github.com/GitBruno/myProfiles)
deployed to `<exeDir>/data/icc/myProfiles` (CMake copies a sibling
`../myProfiles` checkout when present). Tokens: `srgb`, `display-p3`,
`pso-coated-v3`, `pso-uncoated-v3`. TIFF export embeds ICC; CMYK docs write
native CMYK TIFF. PNG/JPEG get iCCP / APP2 when a working RGB profile resolves.
`--smoke` skips CMYK ICC asserts when the profiles root is empty.

## Build

```bash
cmake -S examples/example-pixelEditor -B examples/example-pixelEditor/build
cmake --build examples/example-pixelEditor/build --target example-pixelEditor
./examples/example-pixelEditor/build/bin/example-pixelEditor
./examples/example-pixelEditor/build/bin/example-pixelEditor --smoke
```

## Tools

Selection tools (rect, ellipse, lasso, wand) sit as their own toolbar buttons.
Paint/eraser and move/transform still share flyouts (hold or right-click).
Crop (C) stays alone; with a selection active it arms around that selection.
Marquee selection draws marching ants (outline), not a filled tint. Crop shows
a box you can move and resize by the corners. Type a proportion such as 3:1,
or a width and height in pixels. Enter or Apply commits it. Image > Rotate
Canvas 90 CW/CCW (Ctrl+] / Ctrl+[) spins the whole document and swaps
width/height; Rotate Layer keeps canvas size. Undo covers paint, selection,
paste, crop, canvas/image size, and canvas rotate (64 MB byte budget).

## Brushes

Load a folder that contains `brushes.json` (File-style dialog on the Brushes panel, or drop under `data/brushes/`). A brush `tipPath` PNG (relative to the pack folder) is honored on CPU and GPU, with rotation, scatter, flow, and angle jitter; a missing tip falls back to a procedural round.

True Grit Texture Supply partnership is a **Rig SKU** (same idea as their Procreate sets). Do not install their Photoshop `.abr` packs here.

## Packs

- **rigPixelPlotComponent** - layer / canvas / brush POD
- **rigPixelPlotter** - PixelDoc, paint, PNG/JPG/BMP/TIFF export / PSD map
- **rigPixelPlotterUi** - docks (document windows, Layers, Brushes, Colour)
- **rigPsd** - layered 8-bit RGB IO
