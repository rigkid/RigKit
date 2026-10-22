#pragma once

/**
 * @file
 * @brief Attached-monitor identity for output windows.
 * @details Identity is display name, video mode, and position. A saved route
 * resolves back to the same physical screen after a cable swap when the name
 * and mode are unique. Twin displays (same name and mode) stay distinct by
 * position. This does not open windows or draw a lineup pattern.
 */

#include <string>
#include <vector>

namespace rigkit {

/** @brief One attached monitor, or a remembered one with `index` -1. */
struct MonitorInfo {
	int index = -1; ///< Current GLFW index, or -1 when offline.
	std::string name;
	int width = 0;
	int height = 0;
	int refresh = 0;
	int x = 0;
	int y = 0;
	std::string identity;
	bool twin = false; ///< Another live monitor has the same name and mode.
};

/** @brief `name|WxH@Hz|x,y`. `|` inside the name is stored as `/`. */
std::string monitorIdentity(const MonitorInfo& monitor);

/** @brief `name|WxH@Hz` without position. An `index:N` key is unchanged. */
std::string monitorStem(const std::string& identity);

/** @brief True when `identity` is the `index:N` fallback. */
bool identityIsIndex(const std::string& identity, int& indexOut);

/** @brief Fallback key when a display has no live identity. */
std::string indexIdentity(int index);

/** @brief Currently attached monitors. Empty before GLFW is initialized. */
std::vector<MonitorInfo> enumerateMonitors();

/**
 * @brief Live GLFW index for a saved identity, or -1.
 * @details Exact identity wins. A unique name+mode match ignores position, so
 * a cable swap still hits. Two twins with the same stem do not match that way.
 * `index:N` resolves only while that index is attached.
 */
int resolveMonitorIndex(const std::string& identity, const std::vector<MonitorInfo>& live);

/** @brief `index: name WxH @Hz`, plus position for a twin and `(offline)` when absent. */
std::string monitorLabel(const MonitorInfo& monitor);

} // namespace rigkit
