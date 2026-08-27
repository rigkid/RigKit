# Settings and preferences

RigKit has two related seams:

1. **`ISettings`** - JSON get/set on managers, packs, canvas (serialize blobs).
2. **User preferences** - pack/host POD sections registered on `MSettings`, edited in the Preferences panel, persisted to `<userData>/user/rigkit_settings.json` (default user data root = `<exeDir>/data`).

## Data Path

**Shipped data** (`AppPaths::getDataDir()` = `<exeDir>/data`) holds fonts and app samples redeployed on build.

**User data** (`AppPaths::getUserDataDir()`) holds settings, workspaces, and themes under `user/`. Preferences > Application > **Data Path** overrides that root (empty = default `<exeDir>/data`).

The override is also written to an OS config pointer so it survives a clean rebuild that wipes `<exeDir>/data`:

| Platform | Pointer file |
|----------|----------------|
| Windows | `%APPDATA%/RigKit/<appId>/datapath` |
| Linux / Pi | `~/.config/rigkit/<appId>/datapath` |
| macOS | `~/Library/Application Support/RigKit/<appId>/datapath` |

`<appId>` is the executable stem. File browsers (rigImGui) expose both **App data** and **User data** in Quick Access.

## User preferences (packs)

Preference meaning is plain POD with `GetProperties()` (same `sProp` types as ECS components). Packs own the data; the host registry persists it.

```cpp
struct MyPrefs {
	float inkWidth = 0.35f;
	bool showGuides = true;

	std::vector<sProp> GetProperties() {
		return {
			{0, "Ink Width", EPT_FLOAT, &inkWidth},
			{1, "Show Guides", EPT_BOOL, &showGuides},
		};
	}
};

// In pack setup() - after engine is available:
engine->getSettingsManager()->registerPreferences("myPack.tool", "My Pack", &m_prefs);

// In pack cleanup():
engine->getSettingsManager()->unregisterPreferences("myPack.tool");
```

Flow:

1. Engine loads the OS Data Path pointer (if any), then `rigkit_settings.json` (`MSettings::loadFromDisk`).
2. On `registerPreferences`, stored values for that section id are applied onto the live POD.
3. Preferences panel (`rigImGui`, File > Preferences...) lists categories on the left and edits the selected section’s live fields via `sProp` on the right.
4. Save button or process exit writes registered sections (merged so unloaded packs keep their keys).

JSON shape:

```json
{
  "recentFiles": [
    "D:/projects/dogs.rig",
    "D:/projects/test.rig"
  ],
  "workspace": "plotting",
  "sections": {
    "host.app": { "Debug Mode": false, "VSync": true, "Data Path": "" },
    "rigImGui.ui": {
      "Theme": 0,
      "Theme File": "custom.json",
      "Font File": "",
      "Font Size": 16.0,
      "Confirm Quit": false,
      "Notification Seconds": 3.0,
      "Notification Width": 320.0,
      "Ruler Unit": 0,
      "Progress In Status Bar": true,
      "Progress Auto-Hide Seconds": 2.0,
      "Show Status Bar": true,
      "FPS Display": 0
    }
  }
}
```

`recentFiles` is a top-level blob value (not a Preferences section) written by `IMui::noteRecentFile` / File > Open Recent. `workspace` is the active named dock layout (View > Workspace in `rigImGui`); the layout files themselves live under `data/user/workspaces/<name>.ini` (dock tree plus `[RigVisibility]`). On startup, rigImGui loads that named file when present, otherwise `Standard.ini`.

Built-in section: `host.app` (`AppSettings` - debug, vsync, FPS, clear color, window size/fullscreen, Data Path). Window size, fullscreen, vsync, clear color, debug, and Data Path all apply **immediately** when changed.

`rigImGui.ui` also includes chrome prefs: **Show Status Bar**, **FPS Display** (Status Bar / Menu Bar / Off), progress placement, theme, fonts, rulers.

Access: `engine->getSettingsManager()` returns `MSettings*`.

Paths: `AppPaths::getUserSettingsFile()` under `getUserDataDir()`.

## ISettings (manager / pack blobs)

Any class that supports settings export/import can inherit `ISettings`:

```cpp
class ISettings {
public:
	virtual ~ISettings() = default;
	virtual rigkit::json getSettings() const = 0;
	virtual void setSettings(const rigkit::json& settings) = 0;
};
```

Example manual save/load of an object blob:

```cpp
rigkit::json canvasJson = canvas->getSettings();
std::ofstream out("canvas.json");
out << canvasJson.dump(4);
```

`sProp` / JSON helpers live in `src/ecs/PropertyJson.h` (`propsToJson` / `jsonToProps`) and power preference persistence.

### Manager blob scope

| Type | Blob means | Does not |
|------|------------|----------|
| `MSettings` / prefs sections | User preferences on disk | Scene / document |
| `AppSettings` | Window / graphics POD for the app | Renderer GPU state |
| `Canvas` | Surface size / clear / samples (`-1` inherits `window.samples`) | Engine pointer |
| `MRendering` | Entity-to-renderer type and canvas settings metadata | Main window renderer (engine-owned); `Graphics` wrappers; ECS scene (use **rigProject**) |
| `MEcs` | Entity id/name list only | Components - scene IO is **rigProject** |
| `Mui` | Theme / workspace hints | Live UI prefs (`rigImGui.ui` via `MSettings`) |

## Notes

- No Dear ImGui in `src/` - Preferences UI is `rigImGui` only.
- Headless hosts still load/save the preference file; they simply have no Preferences panel.
- Pack `IPack::parameters` / `customSettings` remain for pack-internal config; user-facing knobs should use `registerPreferences`.
