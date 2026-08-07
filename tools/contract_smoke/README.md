# contract_smoke

Headless [doctest](https://github.com/doctest/doctest) + CTest lane for RigKit **core** and spine packs (`rigComponent`, `rigSystems`, `rigProject`). Guards the data layer (registration, `GetProperties` pointer validity, systems boundary, header purity, `.rig` serializer round-trip). No window, no `rigImGui`.

## Build and run

From the RigKit repo root:

```bash
cmake -S . -B build -DRIGKIT_BUILD_CONTRACT_SMOKE=ON
cmake --build build --target contract_smoke
ctest --test-dir build --output-on-failure -R contract_smoke
```

Or run the binary: `build/bin/contract_smoke`.

`RIGKIT_BUILD_CONTRACT_SMOKE` defaults to **ON** when RigKit is the top-level CMake project.
