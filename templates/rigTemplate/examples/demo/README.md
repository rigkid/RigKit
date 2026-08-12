# demo

![preview](img/preview.png)

Example scaffold for a new pack. After copying `rigTemplate` → `packs/<yourName>/`:

1. Rename this folder / `app.json` `name` if you want.
2. Fix `CMakeLists.txt` `RIGKIT_DIR` to `../../../..` (four levels up from `packs/<name>/examples/demo`).
3. Register your pack in `app.cpp` and capture `img/preview.png` for the pack README.
