#include "I2s.h"

#include "Pcm.h"
#include "rig_esp_features.h"

#if defined(RIGKIT_ESP_I2S)

#include <driver/i2s_std.h>

namespace rigkit::espapp {
namespace {

i2s_chan_handle_t gTx = nullptr;
bool gFailed = false;

bool i2sStart() {
	i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
	if (i2s_new_channel(&chan, &gTx, nullptr) != ESP_OK || !gTx) {
		return false;
	}
	i2s_std_config_t cfg{};
	cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100);
	cfg.slot_cfg =
		I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
	cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
	cfg.gpio_cfg.bclk = static_cast<gpio_num_t>(RIGKIT_ESP_I2S_BCLK);
	cfg.gpio_cfg.ws = static_cast<gpio_num_t>(RIGKIT_ESP_I2S_WS);
	cfg.gpio_cfg.dout = static_cast<gpio_num_t>(RIGKIT_ESP_I2S_DOUT);
	cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
	if (i2s_channel_init_std_mode(gTx, &cfg) != ESP_OK) {
		return false;
	}
	return i2s_channel_enable(gTx) == ESP_OK;
}

} // namespace

void i2sPump() {
	if (gFailed) {
		return;
	}
	if (!gTx && !i2sStart()) {
		gFailed = true;
		return;
	}
	int16_t samples[512];
	const size_t frames = pcmRead(samples, 256);
	if (!frames) {
		return;
	}
	size_t wrote = 0;
	i2s_channel_write(gTx, samples, frames * 2 * sizeof(int16_t), &wrote, 0);
}

} // namespace rigkit::espapp

#endif
