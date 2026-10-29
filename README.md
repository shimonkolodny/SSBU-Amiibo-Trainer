# SSBU Amiibo Trainer & Spirit Collection Studio

A native C++20 Windows desktop application and cross-platform CLI tool for **Super Smash Bros. Ultimate** that loads 540-byte NTAG215 Amiibo `.bin` files, equips Primary and Support Spirits from the full SSBU Spirit Collection, simulates hundreds or thousands of matches against SSBU fighter archetypes, and outputs a tournament-ready Level 50 `.bin` file.

## Technical Grounding & Open-Source References

- **SSBU Decompilation & Fighter AI Modules**: Modeled on the unpacked fighter and AI target/action evaluation structures in [sbergeron42/ssbu-decomp](https://github.com/sbergeron42/ssbu-decomp) (`src/app/ai_helpers_a.cpp`, `src/app/ai_target_stat.cpp`, `src/app/fighter_attack.cpp`, and `src/app/FighterSpiritsSupportSkill.cpp`) and genetic-algorithm AI hooks in [OpenSmash-Ultimate/ssbu-recomp](https://github.com/OpenSmash-Ultimate/ssbu-recomp) (`fighter_sdk/include/mod_ga.h`).
- **NTAG215 & SSBU AppData Binary Layout (`0xDC–0x1B3`)**: Implements the bit-reversed little-endian bitfield encoding, 50-level XP curve (`0x14C–0x14D`), 9-level hidden CPU XP curve (`0x14E–0x14F`), Spirit stat offsets (`0x150–0x153`), and Spirit slot bytes (`0xEC–0xEE`) documented in [jozz024/smash-amiibo-editor](https://github.com/jozz024/smash-amiibo-editor), [TitleOS/AlterEgo](https://github.com/TitleOS/AlterEgo), [AD2076/emuiibo](https://github.com/AD2076/emuiibo), and the [Smash Ultimate Amiibo Bin Guide](https://www.scribd.com/document/811418291/Comprehensive-Overview-of-Smash-Ultimate-Amiibo-bins).
- **25-State SSBU Personality Evaluator**: Matches the exact threshold and flip scoring rules from [personality_data.json](https://github.com/jozz024/smash-amiibo-editor/blob/main/resources/personality_data.json).
- **Full SSBU Spirit Collection Support**: Includes all 255 Support Spirit Ability IDs (`0–254`, with 1-, 2-, and 3-slot legality validation) and Primary Spirit presets ([Spirit - SmashWiki](https://www.ssbwiki.com/Spirit), [List of Spirits in Super Smash Bros. Ultimate](https://nintendo.fandom.com/wiki/List_of_Spirits_in_Super_Smash_Bros._Ultimate)).

## Features

1. **Upload or Generate `.bin` Files**: Load any 540-byte (or 532/572-byte) NTAG215 Amiibo `.bin` file or generate a fresh Level 1 dump for any SSBU fighter.
2. **Complete Spirit Collection Editor**:
   - **Primary Spirits**: Apply preset Primary Spirits (Neutral, Attack, Defense, Grab) or set custom Attack (`0x150–0x151`) and Defense (`0x152–0x153`) stats (enforcing the 5,000 combined stat cap in legal mode or unrestricted stats).
   - **Support Spirit Abilities (`0xEC`, `0xED`, `0xEE`)**: Choose from all 255 SSBU Spirit abilities (e.g., *Super Armor*, *Slow Super Armor*, *Additional Midair Jump*, *Move Speed Up*, *Hyper Smash Attacks*, *Landing Lag Down*, *Autoheal*, *Great Autoheal*, *Easier Perfect Shield*, *Armor Knight*, *Weapon Attack Up*, *Air Attack Up*, *KOs Heal Damage*) with automatic 3-slot cost validation and multi-slot byte alignment.
3. **High-Speed Multi-Hundred Match Simulator**:
   - Run 100 to 100,000+ simulated 3-stock matches in seconds against specific fighters or the **Full Roster Gauntlet**.
   - Evolve all 36 bit-packed behavior parameters (`0x168–0x181`), 10 Grounded Move probabilities (`0x188–0x195` + implicit Jab), and 8 Aerial Move probabilities (`0x195–0x19E` + implicit Nair) up to Level 50 (`3912` XP) and CPU Level 9 (`2765` XP).
4. **Native Windows Win32 GUI + Cross-Platform CLI**:
   - Launches a native Windows desktop interface when run with no arguments on Windows, or runs headlessly via CLI flags on Windows and Linux.

## Building & Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### CLI Example

```bash
./build/ssbu_amiibo_trainer \
  --generate "Bowser" \
  --primary-spirit "Bowser (Wedding)" \
  --slot1 "Additional Midair Jump" \
  --slot2 "Landing Lag Down" \
  --games 500 \
  --focus 0 \
  --output trained_bowser.bin
```
