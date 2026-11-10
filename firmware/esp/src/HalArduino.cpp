#include "Hal.h"

#include "rig_esp_features.h"

#if defined(ARDUINO)

#include <Arduino.h>
#include <Preferences.h>

#include <cstdio>

#if defined(RIGKIT_ESP_WIFI)
#include <WiFi.h>
#endif

#if defined(RIGKIT_ESP_STATUS_LED)
#include <FastLED.h>
#endif

namespace rigkit::espapp {
namespace {

SemaphoreHandle_t gLock = nullptr;
char gLine[64];
size_t gLineLen = 0;

#if defined(RIGKIT_ESP_STATUS_LED)
CRGB gLeds[RIGKIT_ESP_STATUS_LED_COUNT];
bool gLedsReady = false;
bool gLedShown = false;
uint8_t gLastR = 0;
uint8_t gLastG = 0;
uint8_t gLastB = 0;

uint8_t scale(float v, float a) {
	float s = v * a;
	if (s < 0.f) {
		s = 0.f;
	}
	if (s > 1.f) {
		s = 1.f;
	}
	return static_cast<uint8_t>(s * 255.f + 0.5f);
}
#endif

} // namespace

void halLockInit() {
	if (!gLock) {
		gLock = xSemaphoreCreateMutex();
	}
}

void halLock() {
	if (gLock) {
		xSemaphoreTake(gLock, portMAX_DELAY);
	}
}

void halUnlock() {
	if (gLock) {
		xSemaphoreGive(gLock);
	}
}

void halPrint(const char* text) {
	if (text) {
		Serial.print(text);
	}
}

bool halReadLine(char* buf, size_t cap) {
	if (!buf || cap == 0) {
		return false;
	}
	while (Serial.available() > 0) {
		const char c = static_cast<char>(Serial.read());
		if (c == '\r') {
			continue;
		}
		if (c == '\n') {
			gLine[gLineLen] = '\0';
			gLineLen = 0;
			std::snprintf(buf, cap, "%s", gLine);
			return buf[0] != '\0';
		}
		if (gLineLen + 1 < sizeof gLine) {
			gLine[gLineLen++] = c;
		}
	}
	return false;
}

void halRadioApply(const Radio& radio) {
#if defined(RIGKIT_ESP_WIFI)
	const char* ap = radio.apSsid[0] ? radio.apSsid : "RigKit";
	int channel = radio.channel;
	if (channel < 1 || channel > 14) {
		channel = 1;
	}
	WiFi.persistent(false);
	WiFi.setSleep(false);
	WiFi.mode(WIFI_OFF);
	if (radio.mode == RadioMode::Off) {
		return;
	}
	if (radio.mode == RadioMode::Ap) {
		WiFi.mode(WIFI_AP);
	} else if (radio.mode == RadioMode::Sta) {
		WiFi.mode(WIFI_STA);
	} else {
		WiFi.mode(WIFI_AP_STA);
	}
	if (radio.mode == RadioMode::Ap || radio.mode == RadioMode::ApSta) {
		WiFi.softAP(ap, nullptr, channel);
	}
	if (radio.mode == RadioMode::Sta || radio.mode == RadioMode::ApSta) {
		WiFi.begin(radio.staSsid, radio.staPass);
	}
	delay(100);
#else
	(void)radio;
#endif
}

void halRadioRead(RadioCache& cache) {
	cache = {};
#if defined(RIGKIT_ESP_WIFI)
	wifi_mode_t mode = WIFI_MODE_NULL;
	if (esp_wifi_get_mode(&mode) != ESP_OK || mode == WIFI_MODE_NULL) {
		return;
	}
	IPAddress ip;
	if (mode == WIFI_MODE_STA || (mode == WIFI_MODE_APSTA && WiFi.status() == WL_CONNECTED)) {
		cache.connected = WiFi.status() == WL_CONNECTED;
		ip = WiFi.localIP();
		cache.rssi = WiFi.RSSI();
	} else {
		cache.connected = true;
		ip = WiFi.softAPIP();
	}
	const String text = ip.toString();
	std::snprintf(cache.ip, sizeof cache.ip, "%s", text.c_str());
#endif
}

void halLedShow(float r, float g, float b, float a) {
#if defined(RIGKIT_ESP_STATUS_LED)
	if (!gLedsReady) {
		FastLED.addLeds<WS2812B, RIGKIT_ESP_STATUS_LED_PIN, GRB>(gLeds,
																 RIGKIT_ESP_STATUS_LED_COUNT);
		gLedsReady = true;
	}
	const uint8_t rr = scale(r, a);
	const uint8_t gg = scale(g, a);
	const uint8_t bb = scale(b, a);
	if (gLedShown && rr == gLastR && gg == gLastG && bb == gLastB) {
		return;
	}
	gLedShown = true;
	gLastR = rr;
	gLastG = gg;
	gLastB = bb;
	for (int i = 0; i < RIGKIT_ESP_STATUS_LED_COUNT; ++i) {
		gLeds[i] = CRGB(rr, gg, bb);
	}
	FastLED.show();
#else
	(void)r;
	(void)g;
	(void)b;
	(void)a;
#endif
}

bool halLoadBlob(const char* key, std::string& out) {
	if (!key) {
		return false;
	}
	Preferences prefs;
	if (!prefs.begin("rigkit", true)) {
		return false;
	}
	String value = prefs.getString(key, "");
	prefs.end();
	if (value.length() == 0) {
		return false;
	}
	out.assign(value.c_str(), value.length());
	return true;
}

void halSaveBlob(const char* key, const std::string& value) {
	if (!key) {
		return;
	}
	Preferences prefs;
	if (!prefs.begin("rigkit", false)) {
		return;
	}
	prefs.putString(key, value.c_str());
	prefs.end();
}

const HalView* halView() {
	return nullptr;
}

void halStubPushLine(const char* line) {
	(void)line;
}

} // namespace rigkit::espapp

#endif
