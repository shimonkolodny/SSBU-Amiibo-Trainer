# Super Smash Bros. Ultimate — Amiibo Trainer & Spirit Studio v2.6 (Switch `nn::nfp` + Unique NTAG215 Serial UIDs + 60 FPS GPU Sim + Incineroar Raid Boss Lab)

A native C++20 Windows GUI & cross-platform CLI application that loads or generates a 540-byte NTAG215 Amiibo `.bin` file, assigns a **unique per-tag 7-byte NTAG215 Serial Number (`UID 04:XX:XX:XX:XX:XX:XX`) and 32-byte keygen salt** so the Nintendo Switch and NFC emulators never treat different Amiibo as the same tag, automatically pairs a valid Nintendo Switch Owner Mii & Nickname (`nn::mii::Ver3StoreData` + CRC-16-CCITT), equips **Primary & Support Spirits** from the *Super Smash Bros. Ultimate* Spirit Collection, simulates **60 FPS frame-by-frame physics & hitbox combat matches** on the **GPU first (Direct3D 11 Compute Shader `cs_5_0` / OpenCL)** before falling back to the CPU, and outputs a **Level 50 Switch-ready `.bin` file** encrypted (AES-128-CTR) and signed (Tag & Data HMAC-SHA256) with the exact canonical 160-byte `key_retail.bin` (`MD5: 45fd53569f5765eef9c337bd5172f937`, `SHA-1: bbdbb49a917d14f7a997d327ba40d40c39e606ce`).

## Key Fixes & Upgrades in v2.5–v2.6

1. **Unique Per-Tag NTAG215 Serial UID & 32-Byte Keygen Salt (`RandomizeUidAndSalt` / `--shuffle-uid`)**:
   - Every newly generated `.bin`, fighter change (`SetCharacterId`), GUI export (`Save Trained .bin...`), or `"Pair Mii & UID"` click generates a **fresh, cryptographically unique 7-byte NTAG215 Serial Number (`04:XX:XX:XX:XX:XX:XX`)**, matching ISO/IEC 14443-3A `BCC0`/`BCC1` check bytes, a fresh **32-byte Keygen Salt** (`0x1E8..0x207`), and derived NTAG215 `PWD`/`PACK` bytes (`0x214..0x219`).
   - Automatically detects and migrates any legacy static `04:A1:B2:D4:E5:F6:77` UID on load so the Nintendo Switch, TagMo, Allmiibo/Pixl.js, Amiibolink, and Flask never mistake a newly created Amiibo for a previously scanned tag.
2. **Incineroar Championship Raid Boss & 8-Loadout Spirit Benchmark (`--benchmark-incineroar`, `--focus incineroar`)**:
   - Models exact competitive Spirit multipliers (`Armor Knight [140]`, `Trade-Off Ability Up [15]`, `Move Speed Up [1]`, `Trade-Off Speed Up [14]`, `Super Armor [10]`, `Great Autoheal [180]`, `Critical Healing & Metal [236]`, `Physical Attack Up [50]`, `Side Special Up [87]`, `Landing Lag Down [81]`, `Floaty Jumps [117]`) and Incineroar's **Alolan Whip command-grab + Revenge + Up-Air Juggle + Down-Smash Ledge Trap + Dair Landing + Frame-Perfect Parry** ("Endeavor") AI profile.
   - Automatically selects **Incineroar (`0x1BD7`, Amiibo ID `0x1BD7000003860002`)** whenever the `Incineroar Raid Boss ("Endeavor")` focus is chosen in the GUI or CLI.
3. **Resolved `"You cannot use this amiibo."` In-Game SSBU Validation (`src/amiibo_bin.cpp`, `src/ssbu_loadouts.cpp`)**:
   - **SSBU 32-Bit Little-Endian Application Area Checksum (`0x0DC..0x0DF` / `tag[304..307]`)**: Implements SSBU's exact `_calculate_crc32` (`init = 0x00000000`, polynomial `0xEDB88320`, `xorout = 0xFFFFFFFF`) over the **212 bytes** at `0x0E0..0x1B3` (`tag[308..519]`) and writes it in **4-byte little-endian** to `0x0DC..0x0DF` (`tag[304..307]`), matching [`jozz024/smash-amiibo-editor/utils/ssbu_amiibo.py`](https://github.com/jozz024/smash-amiibo-editor/blob/main/utils/ssbu_amiibo.py).
   - **Verbatim 64-Bit Amiibo Character & Model IDs (`0x1DC..0x1E3` / `tag[0x054..0x05B]`)**: Embeds all **162 official 64-bit SSBU Amiibo IDs** across all **91 SSBU fighters** directly from [`jozz024/smash-amiibo-editor/resources/characters.json`](https://raw.githubusercontent.com/jozz024/smash-amiibo-editor/refs/heads/main/resources/characters.json) and [`N3evin/AmiiboAPI`](https://github.com/N3evin/AmiiboAPI/blob/master/database/amiibo.json) (e.g., Incineroar = `0x1BD7000003860002`, Sephiroth = `0x3601000004210002`, Kazuya = `0x33C0000004200002`, Sora = `0x3F000000042E0002`).
   - **SSBU Program ID Header (`0x0AC..0x0B3` = `01006A803016E000`) & Auto-Migration**: Sets the exact 8-byte Program ID header (`01 00 6A 80 30 16 E0 00`) and AppID (`34 F8 02 00`), and automatically upgrades any legacy v2.1–v2.4 `.bin` on load.
4. **Exact Nintendo Switch `nn::nfp` Retail Key & NTAG215 `PWD`/`PACK` (`src/amiibo_bin.cpp`)**:
   - Uses the canonical 160-byte `key_retail.bin` (`MD5: 45fd53569f5765eef9c337bd5172f937`, `SHA-1: bbdbb49a917d14f7a997d327ba40d40c39e606ce`, `SHA-256: 7f9283654ec1097fbdff31de946651ae60c2854afb544abe8963d389639c710e`) and computes NTAG215 Page 133 `PWD` (`0x214..0x217`) and Page 134 `PACK` (`0x218..0x219` = `0x8080`).

## Building & Running

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
