# Super Smash Bros. Ultimate — Amiibo Trainer & Spirit Studio v2.1 (GPU-First C++20)

A native C++20 Windows GUI & cross-platform CLI application that loads a 540-byte NTAG215 Amiibo `.bin` file, equips **Primary & Support Spirits** from the *Super Smash Bros. Ultimate* Spirit Collection, simulates **hundreds or thousands of combat matches** on the **GPU first (Direct3D 11 Compute Shader `cs_5_0` / OpenCL)** before falling back to the CPU, and outputs a **Level 50 ready `.bin` file** with authentic bit-packed AI behavior matrices, exact 25-state personality scoring, and optional `key_retail.bin` NTAG215 AES-128-CTR encryption & HMAC-SHA256 signing.

## Key Features in v2.1

1. **GPU-First Compute Engine (`include/gpu_compute.hpp`, `src/gpu_compute.cpp`)**:
   - **Direct3D 11 Compute Shader (`cs_5_0`) on Windows**: Enumerates DXGI hardware adapters (`IDXGIFactory1::EnumAdapters1`) to select the highest-VRAM dedicated/integrated GPU first, compiles an HLSL `cs_5_0` compute kernel (`CSMain` with `[numthreads(64, 1, 1)]`), and dispatches all match simulations in parallel on the GPU via `RWStructuredBuffer<GpuMatchOutcome>`.
   - **Multi-Tier Fallback Hierarchy (`GPU -> CPU`)**:
     1. **Tier 1**: Dedicated / Integrated Hardware GPU via Direct3D 11 `cs_5_0` (`D3D_DRIVER_TYPE_UNKNOWN` / `D3D_DRIVER_TYPE_HARDWARE`) or OpenCL (`libOpenCL.so`).
     2. **Tier 2**: Direct3D 11 DirectCompute (`D3D_DRIVER_TYPE_WARP` `cs_5_0`) on headless Windows environments without a physical display adapter.
     3. **Tier 3**: Automatic host CPU fallback if no GPU compute runtime is available or if `--backend cpu` is explicitly selected.
2. **Full NTAG215 Tag ↔ Internal Format Conversion & `nfc3d` Crypto (`src/amiibo_bin.cpp`)**:
   - Automatic detection of encrypted NTAG215 Tag `.bin` dumps (`TagToInternal` / `InternalToTag` layout mapping) vs. decrypted/plain `.bin` files.
   - Full `amiitool` / `nfc3d` AES-128-CTR encryption/decryption, NIST SP 800-90A HMAC-DRBG key derivation, and dual HMAC-SHA256 tag/data signature generation when a 160-byte `key_retail.bin` is loaded.
   - Character ID (`uint16_t`) is written synchronously to both the internal `amiitool` offset (`0x1DC–0x1DD`) and the NTAG215 Tag Model Info offset (`0x054–0x055`).
3. **Complete 89-Fighter Roster & Clean Spirit Collection (`src/ssbu_loadouts.cpp`)**:
   - All **89 playable SSBU fighters** (Mario through Sora, including all Echo Fighters and DLC Fighter Passes 1 & 2) with authentic character IDs and AI combat profiles.
   - **69 Primary Spirit presets** (Legendary 4-Star, Ace 3-Star, Advanced, Novice) plus custom ATK/DEF (`-2000` to `5000`) and quick stat buttons (`2500/2500`, `3200/1800`, `1800/3200`, `0/0`).
   - All **255 Support Spirit Ability IDs** (`0–254`) in the binary table, with internal `"Reserved"` placeholder slots filtered out of the GUI dropdowns (`193` active selectable skills) and searchable via a live keyword filter.
4. **Exact 25-State Personality Scoring (`src/amiibo_bin.cpp`)**:
   - Implements the exact 8-branch (`def`, `agl`, `ofn`, `rsk`, `gen`, `ent`, `cau`, `dyn`) linear interpolation and `necessary_param` thresholds from `jozz024/smash-amiibo-editor`'s `personality_data.json`.
5. **Upgraded Native Windows GUI (`src/win32_gui.cpp`)**:
   - Common Controls v6 visual styles manifest with crisp `Segoe UI` and `Consolas` typography.
   - **Device Selector (`GPU First (Auto)`, `Force GPU (D3D11)`, `Force CPU Only`)** and live GPU adapter/timing telemetry in the Inspector and Status Bar.
   - 3-Tab Live Inspector (`View 1: Overview & Spirits`, `View 2: Full AI Behavior & Moves`, `View 3: Match Simulation Log`).

## Building & Running

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
