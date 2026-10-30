# Super Smash Bros. Ultimate — Amiibo Trainer & Spirit Studio v2.4 (Switch `nn::nfp` Verified + 60 FPS GPU Sim)

A native C++20 Windows GUI & cross-platform CLI application that loads or generates a 540-byte NTAG215 Amiibo `.bin` file, automatically pairs a valid Nintendo Switch Owner Mii & Nickname (`nn::mii::Ver3StoreData` + CRC-16-CCITT), equips **Primary & Support Spirits** from the *Super Smash Bros. Ultimate* Spirit Collection, simulates **60 FPS frame-by-frame physics & hitbox combat matches** on the **GPU first (Direct3D 11 Compute Shader `cs_5_0` / OpenCL)** before falling back to the CPU, and outputs a **Level 50 Switch-ready `.bin` file** encrypted (AES-128-CTR) and signed (Tag & Data HMAC-SHA256) with the exact canonical 160-byte `key_retail.bin` (`MD5: 45fd53569f5765eef9c337bd5172f937`, `SHA-1: bbdbb49a917d14f7a997d327ba40d40c39e606ce`).

## Key Fixes & Upgrades in v2.4

1. **Exact Nintendo Switch `nn::nfp` Retail Key & NTAG215 `PWD`/`PACK` Fix (`src/amiibo_bin.cpp`)**:
   - **Resolved `"This is not an Amiibo"` Error**: Replaced the corrupted `dataKey_magicBytes` (`0xDB, 0xC4, ...`) and `xorPad` bytes in `kBuiltInRetailKey[160]` with the exact canonical 160-byte `key_retail.bin` (`unfixed-info.bin` `MD5: 2551afc7c8813008819836e9b619f7ed` + `locked-secret.bin` `MD5: 0ad86557c7ba9e75c79a7b43bb466333`, combined `MD5: 45fd53569f5765eef9c337bd5172f937`, `SHA-1: bbdbb49a917d14f7a997d327ba40d40c39e606ce`, `SHA-256: 7f9283654ec1097fbdff31de946651ae60c2854afb544abe8963d389639c710e`).
   - **NTAG215 Page 133 `PWD` (`0x214..0x217`) & Page 134 `PACK` (`0x218..0x219` = `0x8080`)**: Computes the exact NTAG215 32-bit password (`uid[1]^uid[3]^0xAA`, `uid[2]^uid[4]^0x55`, `uid[3]^uid[5]^0xAA`, `uid[4]^uid[6]^0x55`) and `PACK` (`0x80, 0x80`) from the 7-byte UID so hardware tags and emulators (TagMo, Flipper Zero, Pixl.js, N2 Elite) pass `PWD_AUTH`.
   - **SHA-256 Key Verification & Legacy v2.3 Auto-Migration**: `VerifyRetailKeyHash()` ensures any external `key_retail.bin` matches Nintendo's canonical digest before overriding the built-in key, and `LoadFromFile()` automatically detects and upgrades any `.bin` previously saved with v2.3 to the genuine Nintendo retail signature.
2. **Real 60 FPS Frame-by-Frame SSBU Physics & Hitbox Combat Engine (`src/gpu_compute.cpp`, `src/training_sim.cpp`)**:
   - Simulates 3-stock matches at **60 Hz (up to 3,600 frames per match)** on Final Destination (`[-140, +140]`, blastzones `[-240, +240]`, `[-140, +190]`) with gravity, traction, jumps, shield health/decay, 5-frame parry (`JustShield`), grab/pummel/throw, offstage recovery, hitbox startup/active/endlag, and authentic SSBU knockback formula `(((p/10 + p*d/20) * (200/(w+100)) * 1.4) + 18) * kbg + bkb`.
   - **GPU-First Execution (`Direct3D 11 cs_5_0` / `OpenCL`) -> CPU Fallback**: Dispatches all match simulations on the GPU first before falling back to the CPU.
3. **Automatic Switch Owner Mii (`nn::mii::Ver3StoreData`) & Nickname Pairing (`src/amiibo_bin.cpp`)**:
   - Populates a valid 96-byte `Ver3StoreData` Mii block (`0x04C..0x0AB`), CRC-16-CCITT (`0x0AA..0x0AB`), UTF-16BE Nickname (`0x038..0x04B`), and SSBU Title ID (`0x01006A800016E000`) & AppID (`0x34F80200`).
4. **Complete 89-Fighter Roster & Spirit Collection (`src/ssbu_loadouts.cpp`)**:
   - All **89 playable SSBU fighters**, **69 Primary Spirit presets**, and **255 Support Spirit Ability IDs** (`193` selectable skills with live search filter).

## Building & Running

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
