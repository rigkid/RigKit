#include "App.h"

#include "Audio.h"
#include "Hal.h"
#include "Json.h"
#include "PrefsDoc.h"
#include "SceneDoc.h"
#include "rig_esp_features.h"

#include <cstdio>
#include <cstring>

namespace rigkit::espapp {
namespace {

bool audioInBuild() {
#if defined(RIGKIT_ESP_A2DP) || defined(RIGKIT_ESP_AIRPLAY)
	return true;
#else
	return false;
#endif
}

json::Value cacheValue(const App& app) {
	json::Value radio = json::Value::obj();
	radio.set("connected", json::Value::boolean(app.radioCache.connected));
	radio.set("ip", json::Value::str(app.radioCache.ip));
	radio.set("rssi", json::Value::number(app.radioCache.rssi));
	radio.set("mode", json::Value::str(radioModeName(app.radio.mode)));
	json::Value now = json::Value::obj();
	now.set("lastResult", json::Value::str(app.espnowCache.lastResult));
	json::Value cache = json::Value::obj();
	cache.set("radio", std::move(radio));
	cache.set("espnow", std::move(now));
	if (audioInBuild()) {
		json::Value audio = json::Value::obj();
		audio.set("linked", json::Value::boolean(app.audio.linked));
		audio.set("enabled", json::Value::boolean(app.audio.enabled));
		audio.set("connected", json::Value::boolean(app.audio.connected));
		audio.set("streaming", json::Value::boolean(app.audio.streaming));
		audio.set("peer", json::Value::str(app.audio.peer));
		audio.set("note", json::Value::str(app.audio.note));
		cache.set("audio", std::move(audio));
	}
	return cache;
}

void saveRadio(const Radio& radio) {
	halSaveBlob("sections", radioWrite(radio));
}

void trimLine(char* buf) {
	size_t n = std::strlen(buf);
	while (n > 0 && (buf[n - 1] == ' ' || buf[n - 1] == '\t')) {
		buf[--n] = '\0';
	}
}

} // namespace

void Setup(App& app) {
	halLockInit();
	std::string err;
	if (!sceneLoadDefault(app, err)) {
		halPrint("scene ");
		halPrint(err.c_str());
		halPrint("\n");
	}
	std::string blob;
	if (halLoadBlob("scene", blob)) {
		sceneOverlayRead(blob, app);
	}
	if (halLoadBlob("sections", blob)) {
		radioMerge(app.radio, blob, err);
	}
	if (halLoadBlob("staPass", blob) && blob.size() < sizeof app.radio.staPass) {
		std::memcpy(app.radio.staPass, blob.data(), blob.size());
		app.radio.staPass[blob.size()] = '\0';
	}
	app.document = scenePublish(app);
	app.radioDirty = true;
	app.audioDirty = true;
}

void Update(App& app, float dt) {
	(void)dt;
	Radio applyRadio{};
	bool doApply = false;
	bool doAudio = false;
	bool bluetooth = false;
	bool airplay = false;
	char btName[33];
	char airName[33];

	halLock();
	char line[64];
	while (halReadLine(line, sizeof line)) {
		trimLine(line);
		if (std::strcmp(line, "radio ap") == 0) {
			app.radio.mode = RadioMode::Ap;
			app.radioDirty = true;
			saveRadio(app.radio);
			halPrint("radio ap\n");
		}
	}
	if (app.radioDirty) {
		applyRadio = app.radio;
		app.radioDirty = false;
		doApply = true;
	}
	if (app.audioDirty) {
		bluetooth = app.bluetoothEnabled;
		airplay = app.airplayEnabled;
		std::snprintf(btName, sizeof btName, "%s", app.bluetoothName);
		std::snprintf(airName, sizeof airName, "%s", app.airplayName);
		app.audioDirty = false;
		doAudio = true;
	}
	halUnlock();

	if (doApply) {
		halRadioApply(applyRadio);
		rigkit::espnow::noteRadioChanged();
		RadioCache snap{};
		halRadioRead(snap);
		halLock();
		app.radioCache = snap;
		halUnlock();
		if (applyRadio.mode != RadioMode::Off) {
			webStart(app);
		}
		char msg[80];
		std::snprintf(msg, sizeof msg, "radio %s %s\n", radioModeName(applyRadio.mode), snap.ip);
		halPrint(msg);
	} else {
		RadioCache snap{};
		halRadioRead(snap);
		halLock();
		app.radioCache = snap;
		halUnlock();
	}

	halLock();
	const bool pending = app.espnowCache.pending;
	if (pending) {
		app.espnowCache.pending = false;
	}
	rigkit::espnow::Endpoint endpoint = app.espnow;
	const bool radioOff = app.radio.mode == RadioMode::Off;
	const int channel = app.radio.channel;
	char previous[96];
	std::snprintf(previous, sizeof previous, "%s", app.espnowCache.lastResult);
	halUnlock();

	if (pending) {
		rigkit::espnow::Cache local{};
		local.pending = true;
		rigkit::espnow::pump(endpoint, local, radioOff, channel);
		halLock();
		std::snprintf(app.espnowCache.lastResult, sizeof app.espnowCache.lastResult, "%s",
					  local.lastResult);
		halUnlock();
		if (std::strcmp(previous, local.lastResult) != 0 && local.lastResult[0] != '\0') {
			halPrint(local.lastResult);
			halPrint("\n");
		}
	}

	if (doAudio) {
		audioApply(bluetooth, btName, airplay, airName);
	}
	audioPump();
	AudioCache audio{};
	audioRead(audio);
	halLock();
	app.audio = audio;
	halUnlock();
}

void Draw(App& app) {
	halLock();
	const Lamp lamp = app.lamp;
	halUnlock();
	halLedShow(lamp.r, lamp.g, lamp.b, lamp.a);
}

void Exit(App& app) {
	(void)app;
}

std::string stateJson(const App& app) {
	json::Value document;
	std::string err;
	if (!json::parse(app.document.c_str(), document, err)) {
		document = json::Value::obj();
	}
	json::Value root = json::Value::obj();
	root.set("document", std::move(document));
	root.set("cache", cacheValue(app));
	return root.dump();
}

std::string cacheJson(const App& app) {
	return cacheValue(app).dump();
}

std::string prefsJson(const App& app) {
	return prefsResponse(app.radio);
}

bool patchState(App& app, const std::string& body, std::string& err) {
	App next = app;
	if (!sceneRead(body, next, err)) {
		return false;
	}
	const bool audioChanged = next.bluetoothEnabled != app.bluetoothEnabled ||
							  next.airplayEnabled != app.airplayEnabled ||
							  std::strcmp(next.bluetoothName, app.bluetoothName) != 0 ||
							  std::strcmp(next.airplayName, app.airplayName) != 0;
	app.lamp = next.lamp;
	app.espnow = next.espnow;
	app.bluetoothEnabled = next.bluetoothEnabled;
	app.airplayEnabled = next.airplayEnabled;
	std::snprintf(app.bluetoothName, sizeof app.bluetoothName, "%s", next.bluetoothName);
	std::snprintf(app.airplayName, sizeof app.airplayName, "%s", next.airplayName);
	if (audioChanged) {
		app.audioDirty = true;
	}
	app.document = scenePublish(app);
	halSaveBlob("scene", sceneOverlayWrite(app));
	err.clear();
	return true;
}

bool patchPrefs(App& app, const std::string& body, std::string& err) {
	if (!radioMerge(app.radio, body, err)) {
		return false;
	}
	app.radioDirty = true;
	saveRadio(app.radio);
	return true;
}

bool postPass(App& app, const std::string& body, std::string& err) {
	json::Value root;
	if (!json::parse(body.c_str(), root, err)) {
		return false;
	}
	const json::Value* pass = root.find("pass");
	if (!pass || pass->type != json::Value::Type::String) {
		err = "pass must be a string";
		return false;
	}
	if (pass->s.size() >= sizeof app.radio.staPass) {
		err = "pass is too long";
		return false;
	}
	std::memcpy(app.radio.staPass, pass->s.data(), pass->s.size());
	app.radio.staPass[pass->s.size()] = '\0';
	halSaveBlob("staPass", pass->s);
	app.radioDirty = true;
	err.clear();
	return true;
}

bool postAction(App& app, const std::string& body, std::string& err) {
	json::Value root;
	if (!json::parse(body.c_str(), root, err)) {
		return false;
	}
	const json::Value* id = root.find("actionId");
	if (!id || id->type != json::Value::Type::String) {
		err = "actionId must be a string";
		return false;
	}
	if (id->s != "espnow.send") {
		err = "unknown action";
		return false;
	}
	rigkit::espnow::requestSend(app.espnowCache);
	err.clear();
	return true;
}

} // namespace rigkit::espapp
