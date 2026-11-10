#include "SceneDoc.h"

#include "Json.h"
#include "rig_esp_features.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
const char kSceneDefault[] =
#include "scene_default.inc"
	;
} // namespace

namespace rigkit::espapp {
namespace {

bool copyText(char* dst, size_t cap, const std::string& src, const char* tooLong,
			  std::string& err) {
	if (src.size() >= cap) {
		err = tooLong;
		return false;
	}
	if (!src.empty()) {
		std::memcpy(dst, src.data(), src.size());
	}
	dst[src.size()] = '\0';
	return true;
}

json::Value* entityList(json::Value& doc) {
	json::Value* ents = doc.find("entities");
	if (!ents || ents->type != json::Value::Type::Array) {
		return nullptr;
	}
	return ents;
}

json::Value* findComponent(json::Value& doc, const char* schema, json::Value** entity) {
	json::Value* ents = entityList(doc);
	if (!ents) {
		return nullptr;
	}
	for (json::Value& ent : ents->a) {
		json::Value* comps = ent.find("components");
		if (!comps) {
			continue;
		}
		if (json::Value* component = comps->find(schema)) {
			if (entity) {
				*entity = &ent;
			}
			return component;
		}
	}
	return nullptr;
}

bool readLamp(json::Value& doc, Lamp& lamp, std::string& err) {
	json::Value* solid = findComponent(doc, "rig.paint.solid", nullptr);
	if (!solid) {
		err = "document needs rig.paint.solid";
		return false;
	}
	json::Value* rgba = solid->find("rgba");
	if (!rgba || rgba->type != json::Value::Type::Array || rgba->a.size() < 4) {
		err = "rgba needs four numbers";
		return false;
	}
	float ch[4];
	for (int i = 0; i < 4; ++i) {
		if (rgba->a[static_cast<size_t>(i)].type != json::Value::Type::Number) {
			err = "rgba needs four numbers";
			return false;
		}
		ch[i] = static_cast<float>(rgba->a[static_cast<size_t>(i)].n);
	}
	lamp.r = ch[0];
	lamp.g = ch[1];
	lamp.b = ch[2];
	lamp.a = ch[3];
	return true;
}

bool readInt(const json::Value& value, int& out) {
	if (value.type != json::Value::Type::Number) {
		return false;
	}
	const int n = static_cast<int>(std::lround(value.n));
	if (std::fabs(value.n - static_cast<double>(n)) > 1e-6) {
		return false;
	}
	out = n;
	return true;
}

bool readEsp(json::Value& doc, rigkit::espnow::Endpoint& endpoint, std::string& err) {
	json::Value* component = findComponent(doc, "rig.io.espnow", nullptr);
	if (!component) {
		err = "document needs rig.io.espnow";
		return false;
	}
	endpoint = {};
	if (json::Value* enabled = component->find("enabled")) {
		if (enabled->type != json::Value::Type::Bool) {
			err = "enabled must be a bool";
			return false;
		}
		endpoint.enabled = enabled->b;
	}
	if (json::Value* peer = component->find("peer")) {
		if (peer->type != json::Value::Type::String) {
			err = "peer must be a string";
			return false;
		}
		if (!copyText(endpoint.peer, sizeof endpoint.peer, peer->s, "peer is too long", err)) {
			return false;
		}
		uint8_t mac[6];
		if (endpoint.peer[0] != '\0' && !rigkit::espnow::parseMac(endpoint.peer, mac)) {
			err = "peer is not a MAC";
			return false;
		}
	}
	if (json::Value* channel = component->find("channel")) {
		int n = 0;
		if (!readInt(*channel, n) || n < 1 || n > 14) {
			err = "channel must be 1-14";
			return false;
		}
		endpoint.channel = n;
	}
	if (json::Value* payload = component->find("payload")) {
		if (payload->type != json::Value::Type::String) {
			err = "payload must be a string";
			return false;
		}
		if (!copyText(endpoint.payload, sizeof endpoint.payload, payload->s,
					  "payload is longer than 250 bytes", err)) {
			return false;
		}
	}
	return true;
}

void readName(json::Value* entity, char* dst, size_t cap) {
	if (!entity) {
		return;
	}
	json::Value* comps = entity->find("components");
	if (!comps) {
		return;
	}
	json::Value* named = comps->find("rig.meta.named");
	if (!named) {
		return;
	}
	json::Value* name = named->find("name");
	if (!name || name->type != json::Value::Type::String || name->s.empty() ||
		name->s.size() >= cap) {
		return;
	}
	std::memcpy(dst, name->s.data(), name->s.size());
	dst[name->s.size()] = '\0';
}

bool readSink(json::Value& doc, const char* schema, bool& present, bool& enabled, char* name,
			  size_t nameCap, std::string& err) {
	json::Value* entity = nullptr;
	json::Value* component = findComponent(doc, schema, &entity);
	present = component != nullptr;
	if (!component) {
		return true;
	}
	if (json::Value* flag = component->find("enabled")) {
		if (flag->type != json::Value::Type::Bool) {
			err = "enabled must be a bool";
			return false;
		}
		enabled = flag->b;
	} else {
		enabled = false;
	}
	readName(entity, name, nameCap);
	return true;
}

void writeRgba(json::Value& solid, const Lamp& lamp) {
	json::Value rgba = json::Value::arr();
	rgba.a.push_back(json::Value::number(lamp.r));
	rgba.a.push_back(json::Value::number(lamp.g));
	rgba.a.push_back(json::Value::number(lamp.b));
	rgba.a.push_back(json::Value::number(lamp.a));
	solid.set("rgba", std::move(rgba));
}

#if defined(RIGKIT_ESP_A2DP) || defined(RIGKIT_ESP_AIRPLAY)
json::Value namedComponent(const char* name, const char* id) {
	json::Value named = json::Value::obj();
	named.set("name", json::Value::str(name));
	named.set("stableId", json::Value::str(id));
	return named;
}

void ensureSink(json::Value& doc, const char* schema, const char* entityId, const char* panelId,
				const char* controlId, const char* role, int order, bool enabled,
				const char* name) {
	json::Value* entity = nullptr;
	if (json::Value* component = findComponent(doc, schema, &entity)) {
		component->set("enabled", json::Value::boolean(enabled));
		if (entity) {
			if (json::Value* comps = entity->find("components")) {
				if (json::Value* named = comps->find("rig.meta.named")) {
					named->set("name", json::Value::str(name));
				}
			}
		}
		return;
	}
	json::Value* ents = entityList(doc);
	if (!ents) {
		return;
	}
	json::Value comps = json::Value::obj();
	comps.set("rig.meta.named", namedComponent(name, entityId));
	json::Value body = json::Value::obj();
	body.set("enabled", json::Value::boolean(enabled));
	comps.set(schema, std::move(body));
	json::Value ent = json::Value::obj();
	ent.set("id", json::Value::str(entityId));
	ent.set("components", std::move(comps));
	ents->a.push_back(std::move(ent));

	json::Value panelComps = json::Value::obj();
	panelComps.set("rig.meta.named", namedComponent(name, panelId));
	json::Value panel = json::Value::obj();
	panel.set("role", json::Value::str(role));
	panel.set("order", json::Value::number(order));
	panel.set("visible", json::Value::boolean(true));
	panelComps.set("rig.ui.panel", std::move(panel));
	json::Value panelEnt = json::Value::obj();
	panelEnt.set("id", json::Value::str(panelId));
	panelEnt.set("components", std::move(panelComps));
	ents->a.push_back(std::move(panelEnt));

	json::Value controlComps = json::Value::obj();
	controlComps.set("rig.meta.named", namedComponent("Enabled", controlId));
	json::Value control = json::Value::obj();
	control.set("panel", json::Value::str(panelId));
	control.set("order", json::Value::number(0));
	control.set("target", json::Value::str(entityId));
	control.set("propertyKey", json::Value::str("enabled"));
	control.set("type", json::Value::str("bool"));
	control.set("enabled", json::Value::boolean(true));
	control.set("readOnly", json::Value::boolean(false));
	control.set("widget", json::Value::str("toggle"));
	controlComps.set("rig.ui.control", std::move(control));
	json::Value controlEnt = json::Value::obj();
	controlEnt.set("id", json::Value::str(controlId));
	controlEnt.set("components", std::move(controlComps));
	ents->a.push_back(std::move(controlEnt));
}
#endif

bool readLooseNumber(const json::Value* value, float& out) {
	if (!value || value->type != json::Value::Type::Number) {
		return false;
	}
	out = static_cast<float>(value->n);
	return true;
}

} // namespace

bool sceneRead(const std::string& json, App& app, std::string& err) {
	json::Value doc;
	if (!json::parse(json.c_str(), doc, err)) {
		return false;
	}
	Lamp lamp = app.lamp;
	rigkit::espnow::Endpoint endpoint = app.espnow;
	if (!readLamp(doc, lamp, err) || !readEsp(doc, endpoint, err)) {
		return false;
	}
	bool hasBluetooth = false;
	bool bluetooth = app.bluetoothEnabled;
	char bluetoothName[33];
	std::snprintf(bluetoothName, sizeof bluetoothName, "%s", app.bluetoothName);
	if (!readSink(doc, "rig.io.bluetooth", hasBluetooth, bluetooth, bluetoothName,
				  sizeof bluetoothName, err)) {
		return false;
	}
	bool hasAirplay = false;
	bool airplay = app.airplayEnabled;
	char airplayName[33];
	std::snprintf(airplayName, sizeof airplayName, "%s", app.airplayName);
	if (!readSink(doc, "x.rigkit.airplay", hasAirplay, airplay, airplayName, sizeof airplayName,
				  err)) {
		return false;
	}
	app.lamp = lamp;
	app.espnow = endpoint;
	if (hasBluetooth) {
		app.bluetoothEnabled = bluetooth;
		std::snprintf(app.bluetoothName, sizeof app.bluetoothName, "%s", bluetoothName);
	}
	if (hasAirplay) {
		app.airplayEnabled = airplay;
		std::snprintf(app.airplayName, sizeof app.airplayName, "%s", airplayName);
	}
	err.clear();
	return true;
}

bool sceneLoadDefault(App& app, std::string& err) {
	return sceneRead(kSceneDefault, app, err);
}

std::string scenePublish(const App& app) {
	json::Value doc;
	std::string err;
	if (!json::parse(kSceneDefault, doc, err)) {
		return "{}";
	}
	if (json::Value* solid = findComponent(doc, "rig.paint.solid", nullptr)) {
		writeRgba(*solid, app.lamp);
	}
	if (json::Value* endpoint = findComponent(doc, "rig.io.espnow", nullptr)) {
		endpoint->set("enabled", json::Value::boolean(app.espnow.enabled));
		endpoint->set("peer", json::Value::str(app.espnow.peer));
		if (app.espnow.channel >= 1) {
			endpoint->set("channel", json::Value::number(app.espnow.channel));
		}
		endpoint->set("payload", json::Value::str(app.espnow.payload));
	}
#if defined(RIGKIT_ESP_A2DP)
	ensureSink(doc, "rig.io.bluetooth", "bluetooth", "panel-bluetooth", "ctrl-bluetooth",
			   "audio.bluetooth", 2, app.bluetoothEnabled, app.bluetoothName);
#endif
#if defined(RIGKIT_ESP_AIRPLAY)
	ensureSink(doc, "x.rigkit.airplay", "airplay", "panel-airplay", "ctrl-airplay", "audio.airplay",
			   3, app.airplayEnabled, app.airplayName);
#endif
	return doc.dump();
}

std::string sceneOverlayWrite(const App& app) {
	json::Value overlay = json::Value::obj();
	overlay.set("r", json::Value::number(app.lamp.r));
	overlay.set("g", json::Value::number(app.lamp.g));
	overlay.set("b", json::Value::number(app.lamp.b));
	overlay.set("a", json::Value::number(app.lamp.a));
	overlay.set("enabled", json::Value::boolean(app.espnow.enabled));
	overlay.set("peer", json::Value::str(app.espnow.peer));
	overlay.set("channel", json::Value::number(app.espnow.channel));
	overlay.set("payload", json::Value::str(app.espnow.payload));
	overlay.set("bt", json::Value::boolean(app.bluetoothEnabled));
	overlay.set("air", json::Value::boolean(app.airplayEnabled));
	return overlay.dump();
}

bool sceneOverlayRead(const std::string& json, App& app) {
	json::Value overlay;
	std::string err;
	if (!json::parse(json.c_str(), overlay, err)) {
		return false;
	}
	float r = app.lamp.r;
	float g = app.lamp.g;
	float b = app.lamp.b;
	float a = app.lamp.a;
	readLooseNumber(overlay.find("r"), r);
	readLooseNumber(overlay.find("g"), g);
	readLooseNumber(overlay.find("b"), b);
	readLooseNumber(overlay.find("a"), a);
	app.lamp.r = r;
	app.lamp.g = g;
	app.lamp.b = b;
	app.lamp.a = a;
	if (const json::Value* enabled = overlay.find("enabled")) {
		if (enabled->type == json::Value::Type::Bool) {
			app.espnow.enabled = enabled->b;
		}
	}
	if (const json::Value* peer = overlay.find("peer")) {
		if (peer->type == json::Value::Type::String && peer->s.size() < sizeof app.espnow.peer) {
			uint8_t mac[6];
			if (peer->s.empty() || rigkit::espnow::parseMac(peer->s.c_str(), mac)) {
				std::memcpy(app.espnow.peer, peer->s.data(), peer->s.size());
				app.espnow.peer[peer->s.size()] = '\0';
			}
		}
	}
	if (const json::Value* channel = overlay.find("channel")) {
		int n = 0;
		if (readInt(*channel, n) && n >= 0 && n <= 14) {
			app.espnow.channel = n;
		}
	}
	if (const json::Value* payload = overlay.find("payload")) {
		if (payload->type == json::Value::Type::String &&
			payload->s.size() < sizeof app.espnow.payload) {
			std::memcpy(app.espnow.payload, payload->s.data(), payload->s.size());
			app.espnow.payload[payload->s.size()] = '\0';
		}
	}
	if (const json::Value* bt = overlay.find("bt")) {
		if (bt->type == json::Value::Type::Bool) {
			app.bluetoothEnabled = bt->b;
		}
	}
	if (const json::Value* air = overlay.find("air")) {
		if (air->type == json::Value::Type::Bool) {
			app.airplayEnabled = air->b;
		}
	}
	return true;
}

} // namespace rigkit::espapp
