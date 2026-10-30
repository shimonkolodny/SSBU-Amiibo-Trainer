# Super Smash Bros. Ultimate — Amiibo Trainer & Spirit Studio v2.5 (Switch `nn::nfp` + SSBU Checksum Verified + 60 FPS GPU Sim)

A native C++20 Windows GUI & cross-platform CLI application that loads or generates a 540-byte NTAG215 Amiibo `.bin` file, automatically pairs a valid Nintendo Switch Owner Mii & Nickname (`nn::mii::Ver3StoreData` + CRC-16-CCITT), equips **Primary & Support Spirits** from the *Super Smash Bros. Ultimate* Spirit Collection, simulates **60 FPS frame-by-frame physics & hitbox combat matches** on the **GPU first (Direct3D 11 Compute Shader `cs_5_0` / OpenCL)** before falling back to the CPU, and outputs a **Level 50 Switch-ready `.bin` file** encrypted (AES-128-CTR) and signed (Tag & Data HMAC-SHA256) with the exact canonical 160-byte `key_retail.bin` (`MD5: 45fd53569f5765eef9c337bd5172f937`, `SHA-1: bbdbb49a917d14f7a997d327ba40d40c39e606ce`).

## Key Fixes & Upgrades in v2.5

1. **Resolved `"You cannot use this amiibo."` In-Game SSBU Validation (`src/amiibo_bin.cpp`, `src/ssbu_loadouts.cpp`)**:
   - **SSBU 32-Bit Little-Endian Application Area Checksum (`0x0DC..0x0DF` / `tag[304..307]`)**: Implements SSBU's exact `_calculate_crc32` (`init = 0x00000000`, polynomial `0xEDB88320`, `xorout = 0xFFFFFFFF`) over the **212 bytes** at `0x0E0..0x1B3` (`tag[308..519]`) and writes it in **4-byte little-endian** to `0x0DC..0x0DF` (`tag[304..307]`), matching [`jozz024/smash-amiibo-editor/utils/ssbu_amiibo.py`](https://github.com/jozz024/smash-amiibo-editor/blob/main/utils/ssbu_amiibo.py).
   - **Verbatim 64-Bit Amiibo Character & Model IDs (`0x1DC..0x1E3` / `tag[0x054..0x05B]`)**: Embeds all **162 official 64-bit SSBU Amiibo IDs** across all **91 SSBU fighters** directly from [`jozz024/smash-amiibo-editor/resources/characters.json`](https://raw.githubusercontent.com/jozz024/smash-amiibo-editor/refs/heads/main/resources/characters.json) and [`N3evin/AmiiboAPI`](https://github.com/N3evin/AmiiboAPI/blob/master/database/amiibo.json) (e.g., Sephiroth = `0x3601000004210002`, Kazuya = `0x33C0000004200002`, Sora = `0x3F000000042E0002`), including shared-series fighters (Mario vs. Dr. Mario, Link vs. Young Link vs. Toon Link, Zelda vs. Sheik, Samus vs. Zero Suit Samus, and Mii Brawler/Swordfighter/Gunner).
   - **SSBU Program ID Header (`0x0AC..0x0B3` = `01006A803016E000`) & Auto-Migration**: Sets the exact 8-byte Program ID header (`01 00 6A 80 30 16 E0 00`) and AppID (`34 F8 02 00`), and automatically upgrades any legacy v2.1–v2.4 `.bin` on load.
2. **Exact Nintendo Switch `nn::nfp` Retail Key & NTAG215 `PWD`/`PACK` (`src/amiibo_bin.cpp`)**:
   - Uses the canonical 160-byte `key_retail.bin` (`MD5: 45fd53569f5765eef9c337bd5172f937`, `SHA-1: bbdbb49a917d14f7a997d327ba40d40c39e606ce`, `SHA-256: 7f9283654ec1097fbdff31de946651ae60c2854afb544abe8963d389639c710e`) and computes NTAG215 Page 133 `PWD` (`0x214..0x217`) and Page 134 `PACK` (`0x218..0x219` = `0x8080`).
3. **Real 60 FPS Frame-by-Frame SSBU Physics & Hitbox Combat Engine (`src/gpu_compute.cpp`, `src/training_sim.cpp`)**:
   - Simulates 3-stock matches at **60 Hz (up to 3,600 frames per match)** on Final Destination (`[-140, +140]`, blastzones `[-240, +240]`, `[-140, +190]`) on the **GPU first (`Direct3D 11 cs_5_0` / `OpenCL`)** before falling back to the CPU.
4. **Automatic Switch Owner Mii (`nn::mii::Ver3StoreData`) & Nickname Pairing (`src/amiibo_bin.cpp`)**:
   - Populates a valid 96-byte `Ver3StoreData` Mii block (`0x04C..0x0AB`), CRC-16-CCITT (`0x0AA..0x0AB`), and UTF-16BE Nickname (`0x038..0x04B`).

## Building & Running

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
