#include "Pcm.h"

#include "rig_esp_features.h"

#if defined(RIGKIT_ESP_I2S)
#include <Arduino.h>
#endif

namespace rigkit::espapp {

#if defined(RIGKIT_ESP_I2S)
namespace {
constexpr size_t kCap = 4096;
int16_t gBuf[kCap];
size_t gRead = 0;
size_t gWrite = 0;
portMUX_TYPE gMux = portMUX_INITIALIZER_UNLOCKED;
} // namespace
#endif

void pcmWrite(const int16_t* interleavedStereo, size_t frames) {
#if !defined(RIGKIT_ESP_I2S)
	(void)interleavedStereo;
	(void)frames;
#else
	if (!interleavedStereo || frames == 0) {
		return;
	}
	const size_t count = frames * 2;
	portENTER_CRITICAL(&gMux);
	for (size_t i = 0; i < count; ++i) {
		const size_t next = (gWrite + 1) % kCap;
		if (next == gRead) {
			break;
		}
		gBuf[gWrite] = interleavedStereo[i];
		gWrite = next;
	}
	portEXIT_CRITICAL(&gMux);
#endif
}

size_t pcmRead(int16_t* interleavedStereo, size_t frames) {
#if !defined(RIGKIT_ESP_I2S)
	(void)interleavedStereo;
	(void)frames;
	return 0;
#else
	if (!interleavedStereo || frames == 0) {
		return 0;
	}
	portENTER_CRITICAL(&gMux);
	const size_t available = (gWrite + kCap - gRead) % kCap;
	size_t got = available / 2;
	if (got > frames) {
		got = frames;
	}
	for (size_t i = 0; i < got * 2; ++i) {
		interleavedStereo[i] = gBuf[gRead];
		gRead = (gRead + 1) % kCap;
	}
	portEXIT_CRITICAL(&gMux);
	return got;
#endif
}

} // namespace rigkit::espapp
