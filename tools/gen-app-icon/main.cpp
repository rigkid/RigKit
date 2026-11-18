/**
 * @file
 * @brief Host tool: write the default app identicon as a `.ico` for PE embed.
 * @details Usage: gen-app-icon <appName> <out.ico>
 * Same pixels as AppIcon::makeDefaultIcon - Windows CMake embeds the file as
 * GLFW_ICON so Explorer shows it on the exe when the app is not running.
 *
 * Writes via a sibling `.tmp` then renames, so a parallel windres that still
 * holds the previous `app.ico` is less likely to see a half-written file.
 */

#include "core/util/AppIcon.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>

namespace {

bool replaceWithRetries(const std::filesystem::path& tmp, const std::filesystem::path& out) {
	namespace fs = std::filesystem;
	std::error_code ec;
	for (int attempt = 0; attempt < 8; ++attempt) {
		ec.clear();
		fs::remove(out, ec);
		ec.clear();
		fs::rename(tmp, out, ec);
		if (!ec) {
			return true;
		}
		// Destination locked (windres / Explorer thumbnail) - brief backoff.
		std::this_thread::sleep_for(std::chrono::milliseconds(100 * (attempt + 1)));
	}
	std::fprintf(stderr, "gen-app-icon: rename '%s' -> '%s' failed: %s\n", tmp.string().c_str(),
				 out.string().c_str(), ec.message().c_str());
	return false;
}

} // namespace

int main(int argc, char** argv) {
	if (argc != 3) {
		std::fprintf(stderr, "Usage: %s <appName> <out.ico>\n", argv[0] ? argv[0] : "gen-app-icon");
		return 1;
	}
	const std::string appName = argv[1];
	const std::filesystem::path outPath = argv[2];
	const std::filesystem::path tmpPath = outPath.string() + ".tmp";

	const auto icons = AppIcon::makeDefaultIcon(appName);
	if (!AppIcon::writeIco(tmpPath.string(), icons)) {
		std::fprintf(stderr, "gen-app-icon: failed to write '%s'\n", tmpPath.string().c_str());
		return 1;
	}
	if (!replaceWithRetries(tmpPath, outPath)) {
		std::error_code ec;
		std::filesystem::remove(tmpPath, ec);
		return 1;
	}
	return 0;
}
