# example-studio

One host, two crafts. **rigVectorUi** and **rigPixelPlotterUi** both register. App > Workspace switches chrome; the `.rig` document stays.

| Workspace | Job |
|-----------|-----|
| Vector | Paths, artboard, Pathfinder |
| Pixel | Layers, paint, Rig brushes, PSD |

Relay (page layout) is out of scope until that product is ready.

The union of vector + pixel packs is heavier than svgEditor alone. Rebuild cost sits in those packs, not a second document format.

## Build

```bash
cmake -S examples/example-studio -B examples/example-studio/build
cmake --build examples/example-studio/build --target example-studio
./examples/example-studio/build/bin/example-studio
./examples/example-studio/build/bin/example-studio --smoke
```

## Demo

1. Launch. Vector docks load (Document in the center).
2. App > Workspace > Pixel. Main View is the center canvas. Paint on the overlay layer (Tools > Paint, or Brushes panel).
3. App > Workspace > Vector. The path document is still there.

Not thirteen apps. Select the craft.
