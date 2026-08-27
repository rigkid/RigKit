/**
 * @file
 * @brief Host tool: write the default app identicon as a `.ico` for PE embed.
 * @details Usage: gen-app-icon <appName> <out.ico>
 * Same pixels as AppIcon::makeDefaultIcon - Windows CMake embeds the file as
 * GLFW_ICON so Explorer shows it on the exe when the app is not running.
 */

#include "core/util/AppIcon.h"

#include <cstdio>
#include <string>

int main(int argc, char** argv) {
	if (argc != 3) {
		std::fprintf(stderr, "Usage: %s <appName> <out.ico>\n", argv[0] ? argv[0] : "gen-app-icon");
		return 1;
	}
	const std::string appName = argv[1];
	const std::string outPath = argv[2];
	const auto icons = AppIcon::makeDefaultIcon(appName);
	if (!AppIcon::writeIco(outPath, icons)) {
		std::fprintf(stderr, "gen-app-icon: failed to write '%s'\n", outPath.c_str());
		return 1;
	}
	return 0;
}
