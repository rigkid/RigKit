#include "Hal.h"

#include <cstdio>
#include <cstring>
#include <map>
#include <string>

#if !defined(ARDUINO)

namespace rigkit::espapp {
namespace {

HalView gView;
char gLines[8][64];
int gHead = 0;
int gCount = 0;
std::map<std::string, std::string> gBlobs;
RadioMode gMode = RadioMode::Off;

} // namespace

void halLockInit() {}
void halLock() {}
void halUnlock() {}

void halPrint(const char* text) {
	(void)text;
}

bool halReadLine(char* buf, size_t cap) {
	if (!buf || cap == 0 || gCount == 0) {
		return false;
	}
	std::snprintf(buf, cap, "%s", gLines[gHead]);
	gHead = (gHead + 1) % 8;
	--gCount;
	return true;
}

void halRadioApply(const Radio& radio) {
	gMode = radio.mode;
	gView.mode = radio.mode;
	std::snprintf(gView.apSsid, sizeof gView.apSsid, "%s", radio.apSsid);
	++gView.applyCount;
}

void halRadioRead(RadioCache& cache) {
	cache = {};
	if (gMode == RadioMode::Off) {
		return;
	}
	if (gMode == RadioMode::Sta) {
		cache.connected = false;
		return;
	}
	cache.connected = true;
	std::snprintf(cache.ip, sizeof cache.ip, "%s", "192.168.4.1");
}

void halLedShow(float r, float g, float b, float a) {
	gView.r = r;
	gView.g = g;
	gView.b = b;
	gView.a = a;
	++gView.ledCount;
}

bool halLoadBlob(const char* key, std::string& out) {
	if (!key) {
		return false;
	}
	const auto it = gBlobs.find(key);
	if (it == gBlobs.end() || it->second.empty()) {
		return false;
	}
	out = it->second;
	return true;
}

void halSaveBlob(const char* key, const std::string& value) {
	if (!key) {
		return;
	}
	gBlobs[key] = value;
}

const HalView* halView() {
	return &gView;
}

void halStubPushLine(const char* line) {
	if (!line || gCount >= 8) {
		return;
	}
	const int tail = (gHead + gCount) % 8;
	std::snprintf(gLines[tail], sizeof gLines[tail], "%s", line);
	++gCount;
}

} // namespace rigkit::espapp

#endif
