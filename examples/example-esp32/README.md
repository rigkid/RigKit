# example-esp32

ESP32-S3 sample. A browser page is the author surface. The desktop engine is not linked, and this folder does not call `add_rigkit_application`.

The scene is a Rig document: status colour is [`rig.paint.solid`](../../docs/contract/RigWorks/schemas/paint/solid.md), ESP-NOW is [`rig.io.espnow`](../../docs/contract/RigWorks/schemas/io/espnow.md) with the action `espnow.send`. [`device.rig`](device.rig) is that scene. Wi-Fi is not an entity here. It is the preference section `host.radio`, same fields as [`rig.io.wifi`](../../docs/contract/RigWorks/schemas/io/wifi.md), stored in NVS. The STA password is a separate write-only key. GET never returns it.

The page lists preference sections and `rig.ui.panel` rows. It does not hard-code an ESP-NOW form. rigEspnow owns the panel data. rigImGui does not run on the board.

## Run on a PC

The self-test is the same Setup / Update / Draw with a stub radio. ESP-NOW is not in that build, so a send reports that. No network and no board.

```bash
cmake -S examples/example-esp32 -B examples/example-esp32/build
cmake --build examples/example-esp32/build --target example-esp32
```

The executable is under `examples/example-esp32/build/bin/` (a multi-config generator adds a configuration directory).

## Flash

Default env `esp32s3`: Wi-Fi page and ESP-NOW, status pixels on GPIO 47 (two WS2812, GRB). Open AP **RigKit**, then http://192.168.4.1.

```bash
pio run -t upload -e esp32s3
pio device monitor -e esp32s3
```

From `examples/example-esp32`. 16MB flash, USB CDC, same class of board as the Pebble / Roboy ESP32-S3.

`esp32s3-audio` adds `RIGKIT_ESP_AIRPLAY` and `RIGKIT_ESP_I2S` (BCLK 40, WS 38, DOUT 39). AirPlay does not advertise a speaker. The page says the receiver is not linked until a receiver library is wired in. `esp32-a2dp` is the original ESP32: Classic A2DP plus I2S. That flag does not build on ESP32-S3.

Flags live in [`firmware/esp/include/rig_esp_features.h`](../../firmware/esp/include/rig_esp_features.h). Absent means that path is not in the binary.

## Sign-off

1. Flash `esp32s3` and join RigKit.
2. Open the page. Change the colour. GPIO 47 follows `rgba`. Alpha scales the pixels. There is no radio tint.
3. Set the radio to `sta`, then `off`. The page goes away with the radio.
4. On the serial port, send `radio ap`. The access point comes back.
5. Send ESP-NOW. Serial prints `lastResult`. A second board is optional.

Link, RSSI, the password, and the send result stay out of the document. Pending is a mailbox, not a field.

This sample does not change `src/` and does not rebuild the Pi host. The firmware is untested until a board is flashed. The self-test is the check on a PC.
