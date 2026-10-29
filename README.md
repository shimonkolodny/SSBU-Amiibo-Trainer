# Super Smash Bros. Ultimate — Amiibo Trainer & Spirit Studio v2.0 (C++20)

A native C++20 Windows GUI & cross-platform CLI application that loads a 540-byte NTAG215 Amiibo `.bin` file, equips **Primary & Support Spirits** from the *Super Smash Bros. Ultimate* Spirit Collection, simulates **hundreds or thousands of combat matches** against any of the **89 playable SSBU fighters**, and outputs a **Level 50 ready `.bin` file** with authentic bit-packed AI behavior matrices, exact 25-state personality scoring, and optional `key_retail.bin` NTAG215 AES-128-CTR encryption & HMAC-SHA256 signing.

## Key Improvements in v2.0

1. **Full NTAG215 Tag ↔ Internal Format Conversion & `nfc3d` Crypto (`src/amiibo_bin.cpp`)**:
   - Automatic detection of encrypted NTAG215 Tag `.bin` dumps (`TagToInternal` / `InternalToTag` layout mapping) vs. decrypted/plain `.bin` files.
   - Full `amiitool` / `nfc3d` AES-128-CTR encryption/decryption, NIST SP 800-90A HMAC-DRBG key derivation, and dual HMAC-SHA256 tag/data signature generation when a 160-byte `key_retail.bin` is loaded.
   - Character ID (`uint16_t`) is written synchronously to both the internal `amiitool` offset (`0x1DC–0x1DD`) and the NTAG215 Tag Model Info offset (`0x054–0x055`).
2. **Complete 89-Fighter Roster & Clean Spirit Collection (`src/ssbu_loadouts.cpp`)**:
   - All **89 playable SSBU fighters** (Mario through Sora, including all Echo Fighters and DLC Fighter Passes 1 & 2) with authentic character IDs and AI combat profiles.
   - **65+ Primary Spirit presets** (Legendary 4-Star, Ace 3-Star, Advanced, Novice, and Dojo styles) plus custom ATK/DEF (`-2000` to `5000`) and quick stat buttons (`2500/2500`, `3200/1800`, `1800/3200`, `0/0`).
   - All **255 Support Spirit Ability IDs** (`0–254`) in the binary table, with internal `"Reserved"` placeholder slots filtered out of the GUI dropdowns and searchable via a live keyword filter.
3. **Exact 25-State Personality Scoring (`src/amiibo_bin.cpp`)**:
   - Implements the exact 8-branch (`def`, `agl`, `ofn`, `rsk`, `gen`, `ent`, `cau`, `dyn`) linear interpolation and `necessary_param` thresholds from `jozz024/smash-amiibo-editor`'s `personality_data.json`.
4. **Upgraded Native Windows GUI (`src/win32_gui.cpp`)**:
   - Common Controls v6 visual styles manifest with crisp `Segoe UI` and `Consolas` typography.
   - 3-Tab Live Inspector (`View 1: Overview & Spirits`, `View 2: Full AI Behavior & Moves`, `View 3: Match Simulation Log`).
   - Live Support Spirit slot-cost meter (`Support Slots Used: X / 3`), Spirit search filter, quick match-count buttons (`100`, `500`, `1,000`, `5,000`), `Reset Lv 1`, `Instant Lv 50`, and direct AI behavior weight fine-tuning (`Offense %`, `Ground %`, `Parry %`, `Edgeguard %`, `Grab %`, `Meteor %`).

## Building & Running

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
