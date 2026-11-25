#include "Telnet.h"

#include "Hal.h"
#include "PrefsDoc.h"

#include <cstdio>
#include <cstring>

#if defined(ARDUINO) && defined(RIGKIT_ESP_WIFI)

#include <WiFi.h>

namespace rigkit::espapp {
namespace {

WiFiServer gServer(kTelnetPort);
WiFiClient gClient;
bool gStarted = false;
char gLine[96];
size_t gLineLen = 0;

void reply(const char* text) {
	if (gClient && gClient.connected() && text) {
		gClient.print(text);
	}
}

void onLine(App& app, char* line) {
	size_t n = std::strlen(line);
	while (n > 0 && (line[n - 1] == ' ' || line[n - 1] == '\t')) {
		line[--n] = '\0';
	}
	if (line[0] == '\0') {
		return;
	}
	if (std::strcmp(line, "radio ap") == 0) {
		halLock();
		app.radio.mode = RadioMode::Ap;
		app.radioDirty = true;
		halSaveBlob("sections", radioWrite(app.radio));
		halUnlock();
		reply("radio ap\n");
		halPrint("telnet radio ap\n");
		return;
	}
	if (std::strcmp(line, "help") == 0) {
		reply("radio ap\n");
		return;
	}
	reply("?\n");
}

} // namespace

void telnetStart() {
	if (gStarted) {
		return;
	}
	gStarted = true;
	gServer.begin();
	gServer.setNoDelay(true);
}

void telnetPoll(App& app) {
	if (!gStarted) {
		return;
	}
	if (!gClient || !gClient.connected()) {
		gClient = gServer.available();
		gLineLen = 0;
		if (gClient) {
			reply("RigKit telnet. Commands: radio ap, help\n");
		}
	}
	if (!gClient || !gClient.connected()) {
		return;
	}
	while (gClient.available() > 0) {
		const char c = static_cast<char>(gClient.read());
		if (c == '\r') {
			continue;
		}
		if (c == '\n') {
			gLine[gLineLen] = '\0';
			gLineLen = 0;
			onLine(app, gLine);
			continue;
		}
		if (gLineLen + 1 < sizeof gLine) {
			gLine[gLineLen++] = c;
		}
	}
}

} // namespace rigkit::espapp

#else

namespace rigkit::espapp {

void telnetStart() {}

void telnetPoll(App& app) {
	(void)app;
}

} // namespace rigkit::espapp

#endif
