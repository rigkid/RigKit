# esp32_contract_host

Proof that **SUDE** + **RigKit-esp32-core** can live without rigImGui, Blend2D, or RigKitEngine.

## Desktop self-test

```bash
cd tools/esp32_contract_host
cmake -B build && cmake --build build
./build/esp32_contract_host   # or build\Debug\...
```

## On device (any MCU)

Define **`RIGKIT_MCU`** and call `Setup` / `Update` / `Draw` / `Exit` from your board loop (ESP-IDF, Arduino, …).  
`RIGKIT_MCU` is the firmware build switch (chip-agnostic). **`RigKit-esp32-core`** is the first concrete POD profile — ESP32 remains the reference board for this sketch.

Share POD entity fields with the desktop host via JSON/binary later.

See [docs/contract/rigkit.md](../../docs/contract/rigkit.md#esp32-subset-profile). Enable with `-DRIGKIT_BUILD_ESP32_CONTRACT_HOST=ON` from the repo root.
