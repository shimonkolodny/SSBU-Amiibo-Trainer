# Super Smash Bros. Ultimate — Amiibo Trainer & Spirit Studio v2.2 (Auto-Mii + GPU-First C++20)

A native C++20 Windows GUI & cross-platform CLI application that loads a 540-byte NTAG215 Amiibo `.bin` file, **automatically pairs a valid 96-byte Owner Mii (with CRC-16-CCITT) and UTF-16BE Nickname** on blank or virgin `.bin` files, equips **Primary & Support Spirits** from the *Super Smash Bros. Ultimate* Spirit Collection, simulates **hundreds or thousands of combat matches** on the **GPU first (Direct3D 11 Compute Shader `cs_5_0` / OpenCL)** before falling back to the CPU, and outputs a **Level 50 ready `.bin` file**.

## Key Features in v2.2

1. **Automatic Owner Mii & Nickname Pairing (`src/amiibo_bin.cpp`)**:
   - Blank or "virgin" unregistered Amiibo `.bin` dumps (`0x02C = 0x00`) are automatically paired with a valid 96-byte Nintendo 3DS/Wii U/Switch `FFLStoreData` Owner Mii structure (`0x04C–0x0AB`), a verified big-endian **CRC-16-CCITT (`0x1021`)** checksum (`0x0AA–0x0AB`), a 10-character `UTF-16BE` Amiibo Nickname (`0x038–0x04B`), the 64-bit SSBU Program/Title ID (`0x01006A800016E000` at `0x0AC–0x0B3`), the 216-byte AppData CRC-32 (`0x034–0x037`), and initialization flags (`0x02C = 0x30`).
   - Preserves existing Owner Mii and Nickname data automatically when loading a `.bin` already registered on a Switch, while allowing customization via GUI text boxes or `--nickname` / `--mii-name` CLI flags.
2. **GPU-First Compute Engine (`include/gpu_compute.hpp`, `src/gpu_compute.cpp`)**:
   - **Direct3D 11 Compute Shader (`cs_5_0`) on Windows**: Enumerates DXGI hardware adapters (`IDXGIFactory1::EnumAdapters1`) to select the highest-VRAM dedicated/integrated GPU first, compiles an HLSL `cs_5_0` compute kernel (`CSMain` with `[numthreads(64, 1, 1)]`), and dispatches all match simulations in parallel on the GPU via `RWStructuredBuffer<GpuMatchOutcome>`.
   - **Automatic Fallback Hierarchy (`GPU -> CPU`)**: Dedicated/Integrated Hardware GPU (`D3D11 cs_5_0` / `OpenCL`) -> DirectCompute (`D3D_DRIVER_TYPE_WARP cs_5_0`) -> Host Multi-Core CPU fallback.
3. **Full NTAG215 Tag ↔ Internal Format Conversion & `nfc3d` Crypto (`src/amiibo_bin.cpp`)**:
   - Automatic detection of encrypted NTAG215 Tag `.bin` dumps (`TagToInternal` / `InternalToTag` layout mapping) vs. decrypted/plain `.bin` files.
   - Full `amiitool` / `nfc3d` AES-128-CTR encryption/decryption, NIST SP 800-90A HMAC-DRBG key derivation, and dual HMAC-SHA256 tag/data signature generation when a 160-byte `key_retail.bin` is loaded.
4. **Complete 89-Fighter Roster, 69 Primary Spirits & 193 Active Support Spirits (`src/ssbu_loadouts.cpp`)**:
   - All **89 playable SSBU fighters**, **69 Primary Spirit presets**, and all **255 Support Spirit Ability IDs** (`193` selectable active skills with internal `"Reserved"` placeholders filtered out).

## Building & Running

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
