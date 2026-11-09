#include "Airplay.h"

#include "rig_esp_features.h"

#include <cstdio>

#if defined(RIGKIT_ESP_AIRPLAY)

namespace rigkit::espapp {
namespace {

AudioCache gCache;

} // namespace

void airplayApply(bool enabled, const char* name) {
	(void)name;
	gCache = {};
	gCache.enabled = enabled;
	gCache.linked = false;
	std::snprintf(gCache.note, sizeof gCache.note, "%s",
				  enabled ? "receiver is not linked" : "off");
}

void airplayRead(AudioCache& out) {
	out = gCache;
}

} // namespace rigkit::espapp

#endif
