#pragma once

// Compile flags for the ESP kit. The build sets them with -D.
// Absent means that path is not in the binary, and the page does not grow a control for it.

// RIGKIT_ESP_WIFI: rig.io.wifi. Off means no radio and no page.

// RIGKIT_ESP_ESPNOW: rigEspnow transport. Off means a send reports ESP-NOW is not in this build.

// RIGKIT_ESP_A2DP: Classic Bluetooth audio sink. Original ESP32 only.

// RIGKIT_ESP_AIRPLAY: AirPlay sink. Without a linked receiver the page says the receiver is not
// linked, and it does not advertise.

// RIGKIT_ESP_I2S: PCM to the speaker. Off means samples are dropped after a sink has accepted them.

// RIGKIT_ESP_STATUS_LED: status pixels. Pin and count are the constants below.

#if defined(RIGKIT_ESP_A2DP)
#if !defined(CONFIG_IDF_TARGET_ESP32)
#error RIGKIT_ESP_A2DP is Classic Bluetooth. It builds on the original ESP32 only. ESP32-S3 is BLE and is not a stand-in.
#endif
#endif

#ifndef RIGKIT_ESP_STATUS_LED_PIN
#define RIGKIT_ESP_STATUS_LED_PIN 47
#endif

#ifndef RIGKIT_ESP_STATUS_LED_COUNT
#define RIGKIT_ESP_STATUS_LED_COUNT 2
#endif

#ifndef RIGKIT_ESP_I2S_BCLK
#define RIGKIT_ESP_I2S_BCLK 40
#endif

#ifndef RIGKIT_ESP_I2S_WS
#define RIGKIT_ESP_I2S_WS 38
#endif

#ifndef RIGKIT_ESP_I2S_DOUT
#define RIGKIT_ESP_I2S_DOUT 39
#endif
