#include "rendering/ScopeOutput.h"

#include <utility>

namespace rigkit {
namespace {

std::vector<ScopeLayer>& layers() {
	static std::vector<ScopeLayer> g;
	return g;
}

bool g_enabled = false;
bool g_calibrating = false;

} // namespace

void publishScopeLayer(ScopeLayer layer) {
	if (layer.id.empty()) {
		return;
	}
	auto& all = layers();
	for (auto& existing : all) {
		if (existing.id == layer.id) {
			if (layer.hash != 0 && existing.hash == layer.hash) {
				return;
			}
			existing = std::move(layer);
			return;
		}
	}
	all.push_back(std::move(layer));
}

void removeScopeLayer(const std::string& id) {
	auto& all = layers();
	for (auto it = all.begin(); it != all.end(); ++it) {
		if (it->id == id) {
			all.erase(it);
			return;
		}
	}
}

const std::vector<ScopeLayer>& scopeLayers() {
	return layers();
}

void setScopeOutputEnabled(bool enabled) {
	g_enabled = enabled;
}

bool scopeOutputEnabled() {
	return g_enabled;
}

void setScopeOutputCalibrating(bool on) {
	g_calibrating = on;
}

bool scopeOutputCalibrating() {
	return g_calibrating;
}

} // namespace rigkit
