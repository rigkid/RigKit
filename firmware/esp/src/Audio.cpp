#include "Audio.h"

#include "rig_esp_features.h"

#include <cstdio>
#include <cstring>

#if defined(RIGKIT_ESP_A2DP)
#include "A2dp.h"
#endif
#if defined(RIGKIT_ESP_AIRPLAY)
#include "Airplay.h"
#endif
#if defined(RIGKIT_ESP_I2S)
#include "I2s.h"
#endif

namespace rigkit::espapp {

void audioApply(bool bluetooth, const char* btName, bool airplay, const char* airName) {
#if defined(RIGKIT_ESP_A2DP)
	a2dpApply(bluetooth, btName);
#else
	(void)bluetooth;
	(void)btName;
#endif
#if defined(RIGKIT_ESP_AIRPLAY)
	airplayApply(airplay, airName);
#else
	(void)airplay;
	(void)airName;
#endif
}

void audioPump() {
#if defined(RIGKIT_ESP_I2S)
	i2sPump();
#endif
}

void audioRead(AudioCache& out) {
	out = {};
#if defined(RIGKIT_ESP_A2DP) && defined(RIGKIT_ESP_AIRPLAY)
	AudioCache bluetooth{};
	AudioCache air{};
	a2dpRead(bluetooth);
	airplayRead(air);
	out.enabled = bluetooth.enabled || air.enabled;
	out.connected = bluetooth.connected || air.connected;
	out.streaming = bluetooth.streaming || air.streaming;
	out.linked = bluetooth.linked && air.linked;
	const char* peer = bluetooth.connected ? bluetooth.peer : air.peer;
	std::snprintf(out.peer, sizeof out.peer, "%s", peer);
	std::snprintf(out.note, sizeof out.note, "bluetooth: %s; airplay: %s", bluetooth.note,
				  air.note);
#elif defined(RIGKIT_ESP_A2DP)
	a2dpRead(out);
#elif defined(RIGKIT_ESP_AIRPLAY)
	airplayRead(out);
#else
	(void)out;
#endif
}

} // namespace rigkit::espapp
