#pragma once

/**
 * @file
 * @brief Window-icon pixels: read / write a `.ico` file or derive one from an app name.
 * @details Hands raw RGBA buffers to the window layer and sets no icon itself.
 * Core carries no PNG decoder, so PNG-compressed `.ico` entries are skipped
 * on load. Write path always emits uncompressed 32 bpp BMP entries.
 */

#include <string>
#include <vector>

namespace AppIcon {

/// One icon-size RGBA pixel buffer (8 bit per channel, row-major, top-down).
struct IconImage {
	int width = 0;
	int height = 0;
	std::vector<unsigned char> rgba;
};

/**
 * @brief Load window-icon pixels from a `.ico` file.
 * @details Parses uncompressed BMP entries (32 / 24 / 8 / 4 / 1 bpp).
 * PNG-compressed entries (usually only the 256 px one) are skipped — window /
 * taskbar icons use the small sizes, and core carries no PNG decoder.
 * @param path Absolute path to the `.ico` file.
 * @return One IconImage per usable entry; empty when nothing was usable.
 */
std::vector<IconImage> loadIco(const std::string& path);

/**
 * @brief Write icon images as an uncompressed 32 bpp `.ico`.
 * @details Used by the Windows host tool to embed the default identicon as the
 * PE `GLFW_ICON` resource so Explorer shows it on the exe when the app is closed.
 * @param path Destination path (overwrites).
 * @param icons One or more RGBA images (width/height 1–256).
 * @return true when the file was written.
 */
bool writeIco(const std::string& path, const std::vector<IconImage>& icons);

/**
 * @brief Deterministic fallback icon from the app name.
 * @details An FNV-1a hash of the name picks a curated palette (max 5 colours,
 * lightest as background) and draws a small mirrored block pattern
 * (identicon), so apps without a custom icon still look different in the
 * taskbar. Same name = same icon, every launch.
 * @param appName App name from `app.json` (any string, empty is fine).
 * @return 16 / 32 / 48 px images for glfwSetWindowIcon / writeIco.
 */
std::vector<IconImage> makeDefaultIcon(const std::string& appName);

} // namespace AppIcon
