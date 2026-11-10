#pragma once

#include "App.h"

#include <cstddef>
#include <string>

namespace rigkit::espapp {

/// @brief What the desktop stub remembers. Null on the board.
struct HalView {
	RadioMode mode = RadioMode::Off;
	char apSsid[33]{};
	float r = 0;
	float g = 0;
	float b = 0;
	float a = 0;
	int applyCount = 0;
	int ledCount = 0;
};

void halLockInit();
void halLock();
void halUnlock();
void halPrint(const char* text);
bool halReadLine(char* buf, size_t cap);
void halRadioApply(const Radio& radio);
void halRadioRead(RadioCache& cache);
void halLedShow(float r, float g, float b, float a);
bool halLoadBlob(const char* key, std::string& out);
void halSaveBlob(const char* key, const std::string& value);

const HalView* halView();
void halStubPushLine(const char* line);

} // namespace rigkit::espapp
