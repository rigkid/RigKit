#pragma once

#include "App.h"

namespace rigkit::espapp {

/** @brief Telnet listen port for serial-equivalent line commands (not GRBL). */
constexpr int kTelnetPort = 2323;

/**
 * @brief Start the Telnet server once Wi-Fi is up.
 * @details No-op without `RIGKIT_ESP_WIFI` / Arduino. One client at a time.
 */
void telnetStart();

/**
 * @brief Accept / pump one client; run console lines into the app.
 * @details Call from Update under the same lock rules as serial.
 */
void telnetPoll(App& app);

} // namespace rigkit::espapp
