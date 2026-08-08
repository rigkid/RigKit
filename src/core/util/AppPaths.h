#pragma once
#include <string>

namespace AppPaths {

/// Call once at process start (from main argv) so all paths are exe-relative.
/// Safe to call again; later calls are ignored once initialized.
void init(int argc, char* argv[]);

/// Directory containing the running executable (never depends on cwd).
std::string getExecutableDir();

/// Join @p base and @p relative with native separators (lexically normal string).
/// Prefer this over string concat with "/" so Windows logs stay consistent.
std::string joinPath(const std::string& base, const std::string& relative);

/// Shipped runtime data next to the app: `<exeDir>/data` (fonts, samples).
/// Never cwd-relative. Not affected by Data Path override.
std::string getDataDir();

/// User data root for settings / workspaces / themes.
/// Override via setUserDataDir / Preferences Data Path; empty override = getDataDir().
std::string getUserDataDir();

/// Set user-data override (empty clears). Relative paths resolve against getExecutableDir().
void setUserDataDir(const std::string& path);
/// Current override string as stored (empty = using default getDataDir()).
std::string getUserDataDirOverride();

/// Load override from OS user config (survives clean builds of `<exeDir>/data`).
void loadUserDataDirOverride();
/// Persist current override to OS user config (or remove the pointer when cleared).
bool saveUserDataDirOverride();

/// UI / content fonts: `<data>/fonts` (shipped tree).
std::string getFontsDir();

/// Alias for getDataDir() — prefer getDataDir() in new code.
std::string getAssetsDir();

std::string getUserSettingsFile();
std::string getWorkspacesDir();
/// Custom ImGui theme JSON files: `<userData>/user/themes`
std::string getThemesDir();
/// UI layout persistence path (rigImGui may write imgui.ini here).
std::string getUiIniPath();
/// @deprecated Prefer getUiIniPath()
inline std::string getImGuiIniPath() {
	return getUiIniPath();
}
std::string getManifestPath();

} // namespace AppPaths
