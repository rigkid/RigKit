#include "core/util/Monitors.h"

#include "rendering/U_gladGlfw.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <sstream>

namespace rigkit {
namespace {

void slashName(std::string& name) {
	for (char& c : name) {
		if (c == '|') {
			c = '/';
		}
	}
}

std::string modePart(const MonitorInfo& monitor) {
	return std::to_string(monitor.width) + "x" + std::to_string(monitor.height) + "@" +
		   std::to_string(monitor.refresh);
}

} // namespace

std::string monitorIdentity(const MonitorInfo& monitor) {
	MonitorInfo copy = monitor;
	slashName(copy.name);
	if (copy.name.empty()) {
		copy.name = "monitor";
	}
	return copy.name + "|" + modePart(copy) + "|" + std::to_string(copy.x) + "," +
		   std::to_string(copy.y);
}

std::string monitorStem(const std::string& identity) {
	int idx = 0;
	if (identityIsIndex(identity, idx)) {
		return identity;
	}
	const auto last = identity.rfind('|');
	if (last == std::string::npos || last == 0) {
		return identity;
	}
	return identity.substr(0, last);
}

bool identityIsIndex(const std::string& identity, int& indexOut) {
	if (identity.rfind("index:", 0) != 0) {
		return false;
	}
	try {
		indexOut = std::stoi(identity.substr(6));
		return true;
	} catch (...) {
		return false;
	}
}

std::string indexIdentity(int index) {
	return "index:" + std::to_string(index);
}

std::vector<MonitorInfo> enumerateMonitors() {
	std::vector<MonitorInfo> out;
	int count = 0;
	GLFWmonitor** mons = glfwGetMonitors(&count);
	if (!mons || count <= 0) {
		return out;
	}
	out.reserve(static_cast<size_t>(count));
	for (int i = 0; i < count; ++i) {
		MonitorInfo monitor;
		monitor.index = i;
		const char* name = glfwGetMonitorName(mons[i]);
		monitor.name = name ? name : "monitor";
		glfwGetMonitorPos(mons[i], &monitor.x, &monitor.y);
		if (const GLFWvidmode* vm = glfwGetVideoMode(mons[i])) {
			monitor.width = vm->width;
			monitor.height = vm->height;
			monitor.refresh = vm->refreshRate;
		}
		monitor.identity = monitorIdentity(monitor);
		out.push_back(std::move(monitor));
	}
	for (size_t i = 0; i < out.size(); ++i) {
		const std::string stem = monitorStem(out[i].identity);
		for (size_t j = 0; j < out.size(); ++j) {
			if (i != j && monitorStem(out[j].identity) == stem) {
				out[i].twin = true;
				break;
			}
		}
	}
	return out;
}

int resolveMonitorIndex(const std::string& identity, const std::vector<MonitorInfo>& live) {
	if (identity.empty()) {
		return -1;
	}
	for (const MonitorInfo& monitor : live) {
		if (monitor.identity == identity) {
			return monitor.index;
		}
	}
	int indexed = 0;
	if (identityIsIndex(identity, indexed)) {
		if (indexed >= 0 && indexed < static_cast<int>(live.size())) {
			return indexed;
		}
		return -1;
	}
	const std::string stem = monitorStem(identity);
	int found = -1;
	int hits = 0;
	for (const MonitorInfo& monitor : live) {
		if (monitorStem(monitor.identity) == stem) {
			found = monitor.index;
			++hits;
		}
	}
	if (hits == 1) {
		return found;
	}
	return -1;
}

std::string monitorLabel(const MonitorInfo& monitor) {
	std::ostringstream os;
	if (monitor.index >= 0) {
		os << monitor.index << ": ";
	}
	os << (monitor.name.empty() ? "monitor" : monitor.name) << " " << monitor.width << "x"
	   << monitor.height << " @" << monitor.refresh;
	if (monitor.twin) {
		os << "  pos " << monitor.x << "," << monitor.y;
	}
	if (monitor.index < 0) {
		os << "  (offline)";
	}
	return os.str();
}

} // namespace rigkit
