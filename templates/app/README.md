# Product app starter

Thin host for a **product** RigKit app (separate repo). Not built by RigKit’s own CMake.

## Layout after copy

```
app-MyApp/
  CMakeLists.txt
  app.json                # name, SPDX license, pack dependencies
  app.h / app.cpp / main.cpp
  rigkit/                 # git submodule
```

Edit `app.json` before first build: set `name` to the folder id (`app-<Name>`), SPDX `"license"` (required), and pack `dependencies`. Folder = `app.json` `"name"` = CMake target.

## Setup

```bash
cp -r templates/app /path/to/app-MyApp
cd /path/to/app-MyApp
git init
git submodule add https://github.com/rigkid/RigKit.git rigkit
git submodule update --init --recursive
cmake -S . -B build
cmake --build build --target app-MyApp
# run: build/bin/app-MyApp
```

Override RigKit location: `-DRIGKIT_DIR=/path/to/rigkit`.

Learn creators / hierarchy from the in-repo example [`examples/minimal`](../../examples/minimal/) - do not copy that folder into a product repo; use this starter instead.
