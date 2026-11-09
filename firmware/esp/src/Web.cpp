#include "App.h"

#include "Hal.h"
#include "Json.h"

#if defined(ARDUINO) && defined(RIGKIT_ESP_WIFI)

#include "web_ui_gz.inc"

#include <ESPAsyncWebServer.h>

#include <cstring>

namespace rigkit::espapp {
namespace {

AsyncWebServer gServer(80);
App* gApp = nullptr;
bool gStarted = false;
constexpr size_t kBodyMax = 12288;
char gBody[kBodyMax];
size_t gBodyLen = 0;
bool gOverflow = false;

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

} // namespace

void webStart(App& app) {
	gApp = &app;
	if (gStarted) {
		return;
	}
	gStarted = true;
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
}

} // namespace rigkit::espapp

#else

namespace rigkit::espapp {

void webStart(App& app) {
	(void)app;
}

} // namespace rigkit::espapp

#endif
