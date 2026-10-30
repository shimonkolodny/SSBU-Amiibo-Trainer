# Super Smash Bros. Ultimate — Amiibo Trainer & Spirit Studio v2.3 (Switch-Verified + 60 FPS GPU Physics)

A native C++20 Windows GUI & cross-platform CLI application that loads a 540-byte NTAG215 Amiibo `.bin` file, automatically pairs a valid **Nintendo Switch `Ver3StoreData` Owner Mii & `UTF-16BE` Nickname**, equips **Primary & Support Spirits** from the *Super Smash Bros. Ultimate* Spirit Collection, simulates **hundreds or thousands of 60 FPS frame-by-frame SSBU combat matches** on the **GPU first (`Direct3D 11 Compute Shader cs_5_0` / `OpenCL`)** before falling back to the CPU, and outputs a **Switch-verified, Level 50 ready `.bin` file** encrypted (`AES-128-CTR`) and dual-signed (`HMAC-SHA256`) using the built-in 160-byte retail key.

## Key Features in v2.3

1. **100% Nintendo Switch `nn::nfp` & `nn::mii` Hardware Compatibility (`src/amiibo_bin.cpp`)**:
   - **Built-in 160-byte Retail Key (`unfixed-info.bin` + `locked-secret.bin`)**: Automatically decrypts incoming `.bin` dumps (`Nfc3dUnpack`) and encrypts/signs saved `.bin` files (`Nfc3dPack`) into genuine 540-byte NTAG215 Tag Layout out of the box—no external `key_retail.bin` required.
   - **Full NTAG215 Header & Checksum Enforcement**: Enforces valid `BCC0` (`0x88 ^ UID0 ^ UID1 ^ UID2`), `BCC1` (`UID3 ^ UID4 ^ UID5 ^ UID6`), Static Lock (`0F E0`), Capability Container (`F1 10 FF EE`), `0xA5` structure marker, non-zero CRC counter (`0x02E–0x02F`), IEEE 802.3 AppData `CRC-32` (`0x034–0x037`), and full 8-byte Smash Amiibo Figure IDs (`model_no` + `0x02` at `0x1E0–0x1E3`).
   - **Verified `Ver3StoreData` Owner Mii Auto-Pairing (`0x04C–0x0AB`)**: Generates a 96-byte `Ver3StoreData` Mii whose facial bitfields strictly pass `nn::mii::detail::Ver3StoreData::IsValid()` on Nintendo Switch alongside a verified big-endian `CRC-16-CCITT` (`0x0AA–0x0AB`).
2. **60 FPS Frame-by-Frame SSBU Physics, Frame-Data & Knockback Simulator (`src/gpu_compute.cpp`, `src/training_sim.cpp`)**:
   - Simulates 2D stage positions `(x, y)` on `[-85, +85]`, blast zones (`X = ±240`, `Top = +190`, `Bottom = -140`), gravity, traction, 5-frame Just Shield Parry windows, offstage ledge recovery, 17 move frame-data profiles, the authentic SSBU knockback & hitstun formula with Rage, and empirical move-reward learning on both GPU (`cs_5_0`) and CPU.
