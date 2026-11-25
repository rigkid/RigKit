#include "App.h"

#include "Hal.h"
#include "Json.h"
#include "Telnet.h"

#if defined(ARDUINO) && defined(RIGKIT_ESP_WIFI)

#include "web_ui_gz.inc"

#include <ESPAsyncWebServer.h>

#include <cstring>

namespace rigkit::espapp {
namespace {

AsyncWebServer gServer(80);
AsyncWebSocket gWs("/ws");
App* gApp = nullptr;
bool gStarted = false;
constexpr size_t kBodyMax = 12288;
char gBody[kBodyMax];
size_t gBodyLen = 0;
bool gOverflow = false;
std::string gLastCache;
uint32_t gLastPushMs = 0;

void sendJson(AsyncWebServerRequest* request, const std::string& body, int code) {
	AsyncWebServerResponse* response =
		request->beginResponse(code, "application/json", String(body.c_str()));
	response->addHeader("Cache-Control", "no-store");
	request->send(response);
}

void sendError(AsyncWebServerRequest* request, const std::string& message, int code = 400) {
	json::Value root = json::Value::obj();
	root.set("error", json::Value::str(message));
	sendJson(request, root.dump(), code);
}

void collectBody(uint8_t* data, size_t len, size_t index, size_t total) {
	if (index == 0) {
		gBodyLen = 0;
		gOverflow = false;
	}
	if (total >= kBodyMax || index + len >= kBodyMax) {
		gOverflow = true;
		return;
	}
	std::memcpy(gBody + index, data, len);
	gBodyLen = index + len;
	gBody[gBodyLen] = '\0';
}

void handleStateGet(AsyncWebServerRequest* request) {
	halLock();
	const std::string body = gApp ? stateJson(*gApp) : "{}";
	halUnlock();
	sendJson(request, body, 200);
}

void handleCacheGet(AsyncWebServerRequest* request) {
	halLock();
	const std::string body = gApp ? cacheJson(*gApp) : "{}";
	halUnlock();
	sendJson(request, body, 200);
}

void handlePrefsGet(AsyncWebServerRequest* request) {
	halLock();
	const std::string body = gApp ? prefsJson(*gApp) : "{}";
	halUnlock();
	sendJson(request, body, 200);
}

void handleStatePatch(AsyncWebServerRequest* request) {
	if (gOverflow || !gApp) {
		sendError(request, "body too large");
		return;
	}
	std::string err;
	halLock();
	const bool ok = patchState(*gApp, std::string(gBody, gBodyLen), err);
	const std::string body = ok ? stateJson(*gApp) : "";
	halUnlock();
	if (!ok) {
		sendError(request, err.empty() ? "bad document" : err);
		return;
	}
	sendJson(request, body, 200);
}

void handlePrefsPatch(AsyncWebServerRequest* request) {
	if (gOverflow || !gApp) {
		sendError(request, "body too large");
		return;
	}
	std::string err;
	halLock();
	const bool ok = patchPrefs(*gApp, std::string(gBody, gBodyLen), err);
	halUnlock();
	if (!ok) {
		sendError(request, err.empty() ? "bad preferences" : err);
		return;
	}
	sendJson(request, "{\"ok\":true}", 200);
}

void handlePass(AsyncWebServerRequest* request) {
	if (gOverflow || !gApp) {
		sendError(request, "body too large");
		return;
	}
	std::string err;
	halLock();
	const bool ok = postPass(*gApp, std::string(gBody, gBodyLen), err);
	halUnlock();
	if (!ok) {
		sendError(request, err.empty() ? "bad password" : err);
		return;
	}
	sendJson(request, "{\"ok\":true}", 200);
}

void handleAction(AsyncWebServerRequest* request) {
	if (gOverflow || !gApp) {
		sendError(request, "body too large");
		return;
	}
	std::string err;
	halLock();
	const bool ok = postAction(*gApp, std::string(gBody, gBodyLen), err);
	const std::string body = ok ? cacheJson(*gApp) : "";
	halUnlock();
	if (!ok) {
		sendError(request, err.empty() ? "bad action" : err);
		return;
	}
	sendJson(request, body, 200);
}

void onBody(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
	(void)request;
	collectBody(data, len, index, total);
}

void pushCacheIfChanged(bool force) {
	if (!gApp || gWs.count() == 0) {
		return;
	}
	halLock();
	const std::string body = cacheJson(*gApp);
	halUnlock();
	if (!force && body == gLastCache) {
		return;
	}
	gLastCache = body;
	std::string msg = "{\"type\":\"cache\",\"cache\":";
	msg += body;
	msg += "}";
	gWs.textAll(msg.c_str());
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
			   void* arg, uint8_t* data, size_t len) {
	(void)server;
	if (type == WS_EVT_CONNECT) {
		pushCacheIfChanged(true);
		return;
	}
	if (type != WS_EVT_DATA || !gApp || !client) {
		return;
	}
	AwsFrameInfo* info = static_cast<AwsFrameInfo*>(arg);
	if (!info || info->opcode != WS_TEXT) {
		return;
	}
	std::string body(reinterpret_cast<const char*>(data), len);
	json::Value root;
	std::string err;
	if (!json::parse(body.c_str(), root, err) || root.type != json::Value::Type::Object) {
		client->text("{\"error\":\"bad json\"}");
		return;
	}
	const json::Value* typeVal = root.find("type");
	const std::string typeStr =
		(typeVal && typeVal->type == json::Value::Type::String) ? typeVal->s : "";
	if (typeStr == "action") {
		const json::Value* id = root.find("actionId");
		json::Value payload = json::Value::obj();
		if (id && id->type == json::Value::Type::String) {
			payload.set("actionId", json::Value::str(id->s));
		}
		halLock();
		const bool ok = postAction(*gApp, payload.dump(), err);
		const std::string cache = ok ? cacheJson(*gApp) : "";
		halUnlock();
		if (!ok) {
			client->text("{\"error\":\"bad action\"}");
			return;
		}
		std::string msg = "{\"type\":\"cache\",\"cache\":";
		msg += cache;
		msg += "}";
		client->text(msg.c_str());
		return;
	}
	if (typeStr == "patchState") {
		const json::Value* doc = root.find("document");
		std::string payload = doc ? doc->dump() : body;
		halLock();
		const bool ok = patchState(*gApp, payload, err);
		const std::string state = ok ? stateJson(*gApp) : "";
		halUnlock();
		if (!ok) {
			client->text("{\"error\":\"bad document\"}");
			return;
		}
		std::string msg = "{\"type\":\"state\",\"state\":";
		msg += state;
		msg += "}";
		client->text(msg.c_str());
		return;
	}
	if (typeStr == "patchPrefs") {
		halLock();
		const bool ok = patchPrefs(*gApp, body, err);
		halUnlock();
		client->text(ok ? "{\"ok\":true}" : "{\"error\":\"bad preferences\"}");
		return;
	}
}

} // namespace

void webStart(App& app) {
	gApp = &app;
	if (gStarted) {
		return;
	}
	gStarted = true;
	gWs.onEvent(onWsEvent);
	gServer.addHandler(&gWs);
	gServer.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
		AsyncWebServerResponse* response =
			request->beginResponse(200, "text/html", kWebUiGz, kWebUiGzLen);
		response->addHeader("Content-Encoding", "gzip");
		response->addHeader("Cache-Control", "no-store");
		request->send(response);
	});
	gServer.on("/api/state", HTTP_GET, handleStateGet);
	gServer.on("/api/cache", HTTP_GET, handleCacheGet);
	gServer.on("/api/preferences", HTTP_GET, handlePrefsGet);
	gServer.on("/api/state", HTTP_PATCH, handleStatePatch, nullptr, onBody);
	gServer.on("/api/preferences", HTTP_PATCH, handlePrefsPatch, nullptr, onBody);
	gServer.on("/api/sta-pass", HTTP_POST, handlePass, nullptr, onBody);
	gServer.on("/api/action", HTTP_POST, handleAction, nullptr, onBody);
	gServer.begin();
	telnetStart();
}

void webPoll(App& app) {
	(void)app;
	gWs.cleanupClients();
	const uint32_t now = millis();
	if (now - gLastPushMs >= 500) {
		gLastPushMs = now;
		pushCacheIfChanged(false);
	}
}

} // namespace rigkit::espapp

#else

namespace rigkit::espapp {

void webStart(App& app) {
	(void)app;
}

void webPoll(App& app) {
	(void)app;
}

} // namespace rigkit::espapp

#endif
