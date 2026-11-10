#pragma once

#include "EspNow.h"

#include <string>

namespace rigkit::espapp {

enum class RadioMode { Off, Ap, Sta, ApSta };

/// @brief This board's radio. Same fields as `rig.io.wifi`. The password is not a field.
struct Radio {
	RadioMode mode = RadioMode::Ap;
	char apSsid[33] = "RigKit";
	char staSsid[33]{};
	int channel = 1;
	char staPass[65]{};
};

/// @brief Status colour from `rig.paint.solid` `rgba` (0-1). The pin is not a field.
struct Lamp {
	float r = 0.2f;
	float g = 0.55f;
	float b = 1.f;
	float a = 1.f;
};

/// @brief Link snapshot. Host cache, not a component.
struct RadioCache {
	bool connected = false;
	char ip[16]{};
	int rssi = 0;
};

/// @brief Speaker snapshot. Host cache. `linked` is false when no receiver library is in the build.
struct AudioCache {
	bool linked = false;
	bool enabled = false;
	bool connected = false;
	bool streaming = false;
	char peer[64]{};
	char note[96]{};
};

/// @brief One board: preferences, the scene document, and host cache.
///
/// Setup loads once. Update applies a radio change, pumps ESP-NOW, and reads serial.
/// Draw copies the lamp onto the status pixels. Exit releases nothing the HAL still owns.
/// This does not link the desktop engine.
struct App {
	Radio radio;
	Lamp lamp;
	rigkit::espnow::Endpoint espnow;
	rigkit::espnow::Cache espnowCache;
	RadioCache radioCache;
	AudioCache audio;
	bool bluetoothEnabled = false;
	bool airplayEnabled = false;
	char bluetoothName[33] = "RigKit Audio";
	char airplayName[33] = "RigKit AirPlay";
	bool radioDirty = true;
	bool audioDirty = true;
	std::string document;
};

void Setup(App& app);
void Update(App& app, float dt);
void Draw(App& app);
void Exit(App& app);

/// @brief `{ "document", "cache" }`. The password is not included.
std::string stateJson(const App& app);

/// @brief Cache object only. Safe to poll.
std::string cacheJson(const App& app);

/// @brief Preference sections with labels, field metadata, and values. Password omitted.
std::string prefsJson(const App& app);

/// @brief Replace scene fields from a Rig document. Panels in the published document stay the
/// sample's.
bool patchState(App& app, const std::string& body, std::string& err);

/// @brief Merge `sections.host.radio` into the radio. Stores values only.
bool patchPrefs(App& app, const std::string& body, std::string& err);

/// @brief Store `{ "pass" }` in NVS. Empty clears it. GET never returns it.
bool postPass(App& app, const std::string& body, std::string& err);

/// @brief `{ "actionId" }`. `espnow.send` queues the endpoint. Other ids are rejected.
bool postAction(App& app, const std::string& body, std::string& err);

/// @brief Start the page once. No-op when Wi-Fi is not in this build, and on the desktop stub.
void webStart(App& app);

} // namespace rigkit::espapp
