#include "A2dp.h"

#include "Pcm.h"
#include "rig_esp_features.h"

#if defined(RIGKIT_ESP_A2DP)

#include <esp_a2dp_api.h>
#include <esp_avrc_api.h>
#include <esp_bt.h>
#include <esp_bt_device.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_idf_version.h>

#include <cstdio>
#include <cstring>

namespace rigkit::espapp {
namespace {

AudioCache gCache;
bool gStarted = false;

void onData(const uint8_t* data, uint32_t len) {
	gCache.streaming = len > 0;
	if (!data || len < sizeof(int16_t) * 2) {
		return;
	}
	pcmWrite(reinterpret_cast<const int16_t*>(data), len / (sizeof(int16_t) * 2));
}

void onA2d(esp_a2d_cb_event_t event, esp_a2d_cb_param_t* param) {
	if (event == ESP_A2D_CONNECTION_STATE_EVT && param) {
		gCache.connected = param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED;
		if (!gCache.connected) {
			gCache.streaming = false;
			gCache.peer[0] = '\0';
		}
	}
}

} // namespace

void a2dpApply(bool enabled, const char* name) {
	gCache.enabled = enabled;
	gCache.linked = true;
	if (!enabled) {
		gCache.connected = false;
		gCache.streaming = false;
		if (gStarted) {
			esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
		}
		std::snprintf(gCache.note, sizeof gCache.note, "%s", "off");
		return;
	}
	if (!gStarted) {
		esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
		esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
		if (esp_bt_controller_init(&cfg) != ESP_OK ||
			esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT) != ESP_OK) {
			std::snprintf(gCache.note, sizeof gCache.note, "%s", "controller init failed");
			return;
		}
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0)
		esp_bluedroid_config_t bluedroid = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
		const esp_err_t blue = esp_bluedroid_init_with_cfg(&bluedroid);
#else
		const esp_err_t blue = esp_bluedroid_init();
#endif
		if (blue != ESP_OK || esp_bluedroid_enable() != ESP_OK) {
			std::snprintf(gCache.note, sizeof gCache.note, "%s", "bluedroid init failed");
			return;
		}
		const char* deviceName = (name && name[0]) ? name : "RigKit Audio";
		esp_bt_dev_set_device_name(deviceName);
		esp_a2d_register_callback(onA2d);
		esp_a2d_sink_register_data_callback(onData);
		if (esp_a2d_sink_init() != ESP_OK) {
			std::snprintf(gCache.note, sizeof gCache.note, "%s", "a2dp sink init failed");
			return;
		}
		gStarted = true;
	}
	esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
	std::snprintf(gCache.note, sizeof gCache.note, "%s", "discoverable");
}

void a2dpRead(AudioCache& out) {
	out = gCache;
}

} // namespace rigkit::espapp

#endif
