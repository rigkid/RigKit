#pragma once

#include <string>

namespace rigkit {
struct WindowSettings {
	int width = 800;
	int height = 600;
	std::string title = "RigKit App";
	bool fullscreen = false;
};
} // namespace rigkit
