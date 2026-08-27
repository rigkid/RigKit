#pragma once

#include <string>

namespace rigkit {
struct WindowSettings {
	int width = 800;
	int height = 600;
	std::string title = "RigKit App";
	bool fullscreen = false;
	/// Window / taskbar icon: `.ico` path relative to the exe (from `app.json`
	/// `"icon"`). Linux/X11 uses it at runtime; Windows embeds it at build time.
	std::string icon;
	/// GLFW default-framebuffer MSAA (0 = off). Applied at window create only.
	/// Canvas FBOs inherit this when `CanvasSettings.samples` is -1.
	int samples = 0;
};

} // namespace rigkit
