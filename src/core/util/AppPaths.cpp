#include "AppPaths.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <pwd.h>
#include <unistd.h>
#include <vector>
#else
#include <pwd.h>
#include <unistd.h>
#include <vector>
#endif

namespace AppPaths {
namespace {

std::filesystem::path g_exeDir;
std::filesystem::path g_exePath;		  // full path to the running binary (for app id)
std::filesystem::path g_userDataOverride; // empty = use getDataDir()
bool g_ready = false;

std::filesystem::path join(const std::filesystem::path& a, const std::string& b) {
	return a / b;
}

std::string pathStr(const std::filesystem::path& p) {
	return p.lexically_normal().string();
}

std::filesystem::path resolveExeDirFromArgv0(const char* argv0) {
	namespace fs = std::filesystem;
	if (!argv0 || !*argv0) {
		return fs::current_path();
	}
	std::error_code ec;
	fs::path p(argv0);
	if (p.is_relative()) {
		p = fs::absolute(p, ec);
	}
	if (!ec && fs::exists(p, ec)) {
		return p.parent_path();
	}
	return fs::current_path();
}

std::filesystem::path resolveExePathPlatform(const char* argv0) {
	namespace fs = std::filesystem;
#if defined(_WIN32)
	wchar_t buf[MAX_PATH];
	DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
	if (n > 0 && n < MAX_PATH) {
		return fs::path(buf);
	}
#elif defined(__APPLE__)
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	if (size > 0) {
		std::vector<char> buf(size);
		if (_NSGetExecutablePath(buf.data(), &size) == 0) {
			std::error_code ec;
			fs::path p = fs::weakly_canonical(fs::path(buf.data()), ec);
			if (!ec) {
				return p;
			}
			return fs::path(buf.data());
		}
	}
#else
	char buf[4096];
	ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (n > 0) {
		buf[n] = '\0';
		return fs::path(buf);
	}
#endif
	fs::path fromArgv = resolveExeDirFromArgv0(argv0);
	// resolveExeDirFromArgv0 returns a directory; keep stem fallback as "rigkit".
	(void)fromArgv;
	return {};
}

void ensureInit() {
	if (!g_ready) {
		// Fallback if main forgot init - cwd-based, last resort only.
		g_exeDir = std::filesystem::current_path();
		g_ready = true;
	}
}

std::string appId() {
	ensureInit();
	if (!g_exePath.empty()) {
		return g_exePath.stem().string();
	}
	return "rigkit";
}

std::filesystem::path osConfigRoot() {
	namespace fs = std::filesystem;
#if defined(_WIN32)
	const char* appData = std::getenv("APPDATA");
	if (appData && *appData) {
		return fs::path(appData) / "RigKit";
	}
	const char* profile = std::getenv("USERPROFILE");
	if (profile && *profile) {
		return fs::path(profile) / "AppData" / "Roaming" / "RigKit";
	}
#elif defined(__APPLE__)
	const char* home = std::getenv("HOME");
	if (!home || !*home) {
		if (const passwd* pw = getpwuid(getuid())) {
			home = pw->pw_dir;
		}
	}
	if (home && *home) {
		return fs::path(home) / "Library" / "Application Support" / "RigKit";
	}
#else
	const char* xdg = std::getenv("XDG_CONFIG_HOME");
	if (xdg && *xdg) {
		return fs::path(xdg) / "rigkit";
	}
	const char* home = std::getenv("HOME");
	if (!home || !*home) {
		if (const passwd* pw = getpwuid(getuid())) {
			home = pw->pw_dir;
		}
	}
	if (home && *home) {
		return fs::path(home) / ".config" / "rigkit";
	}
#endif
	return fs::path{};
}

std::filesystem::path datapathPointerFile() {
	const auto root = osConfigRoot();
	if (root.empty()) {
		return {};
	}
	return root / appId() / "datapath";
}

std::filesystem::path resolveUserDataPath(const std::string& path) {
	namespace fs = std::filesystem;
	if (path.empty()) {
		return {};
	}
	fs::path p(path);
	if (p.is_relative()) {
		p = fs::path(getExecutableDir()) / p;
	}
	return p.lexically_normal();
}

} // namespace

void init(int argc, char* argv[]) {
#if defined(_WIN32)
	// spdlog writes UTF-8; the Windows console defaults to an OEM page (CP437),
	// which turns em dashes into mojibake. Safe to call more than once.
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif
	if (g_ready) {
		return;
	}
	const char* argv0 = (argc > 0 && argv && argv[0]) ? argv[0] : nullptr;
	g_exePath = resolveExePathPlatform(argv0);
	if (!g_exePath.empty()) {
		g_exeDir = g_exePath.parent_path();
	} else {
		g_exeDir = resolveExeDirFromArgv0(argv0);
	}
	g_ready = true;
}

std::string getExecutableDir() {
	ensureInit();
	return pathStr(g_exeDir);
}

std::string joinPath(const std::string& base, const std::string& relative) {
	return pathStr(join(std::filesystem::path(base), relative));
}

std::string getDataDir() {
	ensureInit();
	return pathStr(join(g_exeDir, "data"));
}

std::string getUserDataDir() {
	ensureInit();
	if (!g_userDataOverride.empty()) {
		return pathStr(g_userDataOverride);
	}
	return getDataDir();
}

std::string getOsConfigDir() {
	ensureInit();
	const auto root = osConfigRoot();
	if (root.empty()) {
		return joinPath(getUserDataDir(), "user");
	}
	return pathStr(root / appId());
}

void setUserDataDir(const std::string& path) {
	ensureInit();
	g_userDataOverride = resolveUserDataPath(path);
}

std::string getUserDataDirOverride() {
	if (g_userDataOverride.empty()) {
		return {};
	}
	return pathStr(g_userDataOverride);
}

void loadUserDataDirOverride() {
	ensureInit();
	const auto ptr = datapathPointerFile();
	if (ptr.empty()) {
		return;
	}
	std::ifstream in(ptr);
	if (!in.is_open()) {
		return;
	}
	std::string line;
	std::getline(in, line);
	// Trim trailing whitespace / CR.
	while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ' ||
							 line.back() == '\t')) {
		line.pop_back();
	}
	if (line.empty()) {
		g_userDataOverride.clear();
		return;
	}
	g_userDataOverride = resolveUserDataPath(line);
}

bool saveUserDataDirOverride() {
	ensureInit();
	const auto ptr = datapathPointerFile();
	if (ptr.empty()) {
		return false;
	}
	std::error_code ec;
	if (g_userDataOverride.empty()) {
		std::filesystem::remove(ptr, ec);
		return true;
	}
	std::filesystem::create_directories(ptr.parent_path(), ec);
	std::ofstream out(ptr);
	if (!out.is_open()) {
		return false;
	}
	out << pathStr(g_userDataOverride);
	return true;
}

std::string getFontsDir() {
	return joinPath(getDataDir(), "fonts");
}

std::string getAssetsDir() {
	return getDataDir();
}

std::string getWorkspacesDir() {
	return joinPath(getUserDataDir(), "user/workspaces");
}

std::string getThemesDir() {
	return joinPath(getUserDataDir(), "user/themes");
}

std::string getUserSettingsFile() {
	return joinPath(getUserDataDir(), "user/rigkit_settings.json");
}

std::string getUiIniPath() {
	return joinPath(getWorkspacesDir(), "imgui.ini");
}

std::string getManifestPath() {
	ensureInit();
	return pathStr(join(g_exeDir, "app.json"));
}

} // namespace AppPaths
