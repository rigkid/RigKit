# Creating a RigKit example or app

## In this repo (example apps)

Every folder under `examples/` is a full example app (`IApp` + `main` + `app.json` + `CMakeLists.txt`). Product apps belong in their own repositories.

```bash
cp -r examples/minimal examples/myNewApp
```

1. Edit `examples/myNewApp/app.json` (`name`, SPDX `license`, optional `dependencies`).
2. Keep / tweak `CMakeLists.txt` (same boilerplate as other examples — nests RigKit from `../..`).
3. Implement `IApp` in `app.h` / `app.cpp`.
4. Configure and build **from that folder**:

```bash
cmake -S examples/myNewApp -B examples/myNewApp/build
cmake --build examples/myNewApp/build --target myNewApp
# output: examples/myNewApp/build/bin/
```

Pack deps are resolved from `app.json` by `cmake/RigKitPacks.cmake` (cloned into `packs/` under RigKit).

## Product app (separate repository)

Use the thin starter under [`templates/app`](../templates/app/) — not an in-repo example:

1. Copy `templates/app` to your new app repo (or clone and rename).
2. Edit `app.json` (`name`, SPDX `license`, pack `dependencies`).
3. Add RigKit as a submodule at `rigkit/` (or set `-DRIGKIT_DIR=`).
4. Configure from the **product** root — it `add_subdirectory`s RigKit and calls `add_rigkit_application(SOURCE_DIR …)` on itself.

```bash
cp -r templates/app ~/MyInstall && cd ~/MyInstall
git init
git submodule add https://github.com/rigkid/RigKit.git rigkit
git submodule update --init --recursive
cmake -S . -B build
cmake --build build --target MyApp
```

The submodule path must be `rigkit/` — that matches `templates/app/CMakeLists.txt` (`RIGKIT_DIR` default). A capital `RigKit/` folder will fail configure unless you pass `-DRIGKIT_DIR=`.

Do **not** copy `examples/minimal` into a product repo. Run examples inside RigKit to learn; start shipping apps from `templates/app`.
