# Product app starter

Thin host for a **product** RigKit app (separate repo). Not built by RigKit’s own CMake.

## Layout after copy

```
MyApp/
  CMakeLists.txt
  app.json                # name, SPDX license, pack dependencies
  app.h / app.cpp / main.cpp
  rigkit/                 # git submodule
```

Edit `app.json` before first build: set `name` to the folder **brand id** (no `app-` / `example-` prefix), SPDX `"license"` (required), and pack `dependencies`. Folder = `app.json` `"name"` = CMake target.

## Setup

```bash
cp -r templates/app /path/to/MyApp
cd /path/to/MyApp
git init
git submodule add https://github.com/rigkid/RigKit.git rigkit
git submodule update --init --recursive
cmake -S . -B build
cmake --build build --target MyApp
# run: build/bin/MyApp
```

Override RigKit location: `-DRIGKIT_DIR=/path/to/rigkit`.

Learn creators / hierarchy from the in-repo example [`examples/minimal`](../../examples/minimal/) - do not copy that folder into a product repo; use this starter instead.
