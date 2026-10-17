# example-svgAnimator

Host example. Loads `data/demo.svg` into `CPath` entities and lets `STweens` slide the last path (`bob`) on `position.x`.

The product host is the sibling checkout `../app-svgAnimator` (next to this repo). This folder stays the in-repo playback reference.

## What you see

Dark window. A stroked rect stays put. A gold circle eases 80 px to the right, then loops.

Paths only. SVG text and images are not placed. There is no timeline panel.

## Build

```bash
cmake -S examples/example-svgAnimator -B examples/example-svgAnimator/build
cmake --build examples/example-svgAnimator/build --target example-svgAnimator
./examples/example-svgAnimator/build/bin/example-svgAnimator
./examples/example-svgAnimator/build/bin/example-svgAnimator --smoke
```

## Packs

- **rigComponent** - `CPath`, `CTransform`, `CDrawStyle`
- **rigAnimComponent** - `CTween`
- **rigSystems** - `STweens` plus shape present
- **rigSvg** - SVG file to path commands (`rigPlotComponent` comes in with it)
