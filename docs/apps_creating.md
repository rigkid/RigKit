# Creating a RigKit example or app

## In this repo (example apps)

Every folder under `examples/` is a full example app (`IApp` + `main` + `app.json` + `CMakeLists.txt`). Product apps belong in their own repositories.

```bash
cp -r examples/minimal examples/myNewApp
```

1. Edit `examples/myNewApp/app.json` (`name`, SPDX `license`, optional `dependencies`).
2. Keep / tweak `CMakeLists.txt` (same boilerplate as other examples - nests RigKit from `../..`).
3. Implement `IApp` in `app.h` / `app.cpp`.
4. Configure and build **from that folder**:

```bash
cmake -S examples/myNewApp -B examples/myNewApp/build
cmake --build examples/myNewApp/build --target myNewApp
# output: examples/myNewApp/build/bin/
```

Pack deps are resolved from `app.json` by `cmake/RigKitPacks.cmake` (cloned into `packs/` under RigKit).

## App icon

Every app gets a window / taskbar icon so builds stop looking identical. Two paths:

**No icon file (default):** the host draws a deterministic identicon from the `app.json` `name` - the name hash picks a curated colour palette (max 5 colours, lightest as background) and a small mirrored block pattern. Same name = same icon on every launch, different apps look different, nothing to author. On Windows the build also runs `gen-app-icon` and embeds that same image as PE icon resources (`1` for Explorer / pinned shortcuts, `GLFW_ICON` for GLFW). Explorer and pinned taskbar buttons use the embedded exe icon when the app is not running.

**Custom icon:** put an `.ico` file (multi-size: 16/32/48) in the app folder and name it in `app.json`:

```json
{
  "name": "myNewApp",
  "icon": "icon.ico"
}
```

The path is relative to the app folder; the build deploys the file next to the exe and the runtime sets it via `glfwSetWindowIcon` (Windows and Linux/X11). On Windows the build also embeds it as the `GLFW_ICON` resource so Explorer shows it on the exe file itself. Use uncompressed `.ico` entries (the usual 16/32/48 sizes are); PNG-compressed entries (typically only the 256 px one) are skipped at runtime.

Wayland and macOS take the icon from the `.desktop` entry / app bundle - a distribution concern, not `app.json`; Pi installs usually run fullscreen anyway.

## Product app (separate repository)

Use the thin starter under [`templates/app`](../templates/app/) - not an in-repo example:

1. Copy `templates/app` to your new app repo (or clone and rename).
2. Edit `app.json` (`name`, SPDX `license`, pack `dependencies`).
3. Add RigKit as a submodule at `rigkit/` (or set `-DRIGKIT_DIR=`).
4. Configure from the **product** root - it `add_subdirectory`s RigKit and calls `add_rigkit_application(SOURCE_DIR ...)` on itself.

```bash
cp -r templates/app ~/MyInstall && cd ~/MyInstall
git init
git submodule add https://github.com/rigkid/RigKit.git rigkit
git submodule update --init --recursive
cmake -S . -B build
cmake --build build --target MyApp
```

The submodule path must be `rigkit/` - that matches `templates/app/CMakeLists.txt` (`RIGKIT_DIR` default). A capital `RigKit/` folder will fail configure unless you pass `-DRIGKIT_DIR=`.

Do **not** copy `examples/minimal` into a product repo. Run examples inside RigKit to learn; start shipping apps from `templates/app`.
