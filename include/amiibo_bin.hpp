#pragma once
#include "ssbu_loadouts.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ssbu {

constexpr size_t kStandardAmiiboSize = 540;
constexpr size_t kInternalAmiiboSize = 520;
constexpr uint32_t kSsbuAppId = 0x34F80200; // Super Smash Bros. Ultimate App Access ID

struct AmiiboBehaviorParams {
    // 36 Core Behavior parameters (0x168 - 0x181) stored as normalized probabilities [0.0 .. 1.0]
    float near_range          = 0.35f; // 0x168b1-b7
    float offensive           = 0.45f; // 0x168b0 + 0x169b2-b7
    float grounded            = 0.50f; // 0x169b0-b1 + 0x16Ab3-b7
    float attack_out_cliff    = 0.25f; // 0x16Ab0-b2 + 0x16Bb5-b7
    float dash                = 0.40f; // 0x16Bb0-b4 + 0x16Cb6-b7
    float return_to_cliff     = 0.60f; // 0x16Cb0-b5
    float air_offensive       = 0.40f; // 0x16Db2-b7
    float cliffer             = 0.30f; // 0x16Db0-b1 + 0x16Eb4-b7
    float feint_master        = 0.25f; // 0x16Eb0-b3 + 0x16Fb5-b7
    float feint_counter       = 0.25f; // 0x16Fb0-b4 + 0x170b6-b7
    float feint_shooter       = 0.25f; // 0x170b0-b5 + 0x171b7
    float catcher             = 0.30f; // 0x171b0-b6 (Grab)
    float _100_attacker       = 0.20f; // 0x172b2-b7
    float _100_keeper         = 0.20f; // 0x172b0-b1 + 0x173b4-b7
    float attack_cancel       = 0.15f; // 0x173b0-b3 + 0x174b6-b7
    float smash_holder        = 0.25f; // 0x174b0-b5 + 0x175b7
    float dash_attacker       = 0.30f; // 0x175b0-b6
    float critical_hitter     = 0.25f; // 0x176b2-b7
    float meteor_master       = 0.30f; // 0x176b0-b1 + 0x177b4-b7
    float shield_master       = 0.35f; // 0x177b0-b3 + 0x178b5-b7
    float just_shield_master  = 0.25f; // 0x178b0-b4 + 0x179b7 (Parry)
    float shield_catch_master = 0.25f; // 0x179b1-b6 (Shield Grab)
    float item_collector      = 0.20f; // 0x179b0 + 0x17Ab4-b7
    float item_throw_to_target= 0.25f; // 0x17Ab0-b3 + 0x17Bb7
    float dragoon_collector   = 0.15f; // 0x17Bb3-b6
    float smashball_collector = 0.20f; // 0x17Bb0-b2 + 0x17Cb7
    float hammer_collector    = 0.15f; // 0x17Cb3-b6
    float special_flagger     = 0.15f; // 0x17Cb0-b2 + 0x17Db7
    float item_swinger        = 0.20f; // 0x17Db2-b6
    float homerun_batter      = 0.20f; // 0x17Db0-b1 + 0x17Eb6-b7
    float club_swinger        = 0.20f; // 0x17Eb2-b5
    float death_swinger       = 0.20f; // 0x17Eb0-b1 + 0x17Fb6-b7
    float item_shooter        = 0.20f; // 0x17Fb1-b5
    float carrier_broker      = 0.15f; // 0x17Fb0 + 0x180b4-b7
    float charger             = 0.25f; // 0x180b0-b3 + 0x181b7
    float appeal              = 0.10f; // 0x181b2-b6 (Taunt)

    // Fighter-Specific & Targeting Parameters (0x181 - 0x188)
    float fighter_1           = 0.20f; // 0x181b0-b1 + 0x182b3-b7
    float fighter_2           = 0.20f; // 0x182b0-b2 + 0x183b4-b7
    float fighter_3           = 0.20f; // 0x183b0-b3 + 0x184b5-b7
    float fighter_4           = 0.20f; // 0x184b0-b4 + 0x185b6-b7
    float fighter_5           = 0.20f; // 0x185b0-b5 + 0x186b7
    float advantageous_fighter= 0.30f; // 0x186b0-b6
    float weaken_fighter      = 0.30f; // 0x187b1-b7
    float revenge             = 0.30f; // 0x187b0 + 0x188b2-b7

    // Grounded Moves (0x188 - 0x195) in percent [0..100], sum <= 100 (Jab = 100 - sum)
    float ftilt       = 12.0f;
    float utilt       = 10.0f;
    float dtilt       = 12.0f;
    float fsmash      = 12.0f;
    float usmash      = 10.0f;
    float dsmash      = 8.0f;
    float nspecial    = 8.0f;
    float sspecial    = 8.0f;
    float uspecial    = 5.0f;
    float dspecial    = 5.0f;

    // Aerial Moves (0x195 - 0x19E) in percent [0..100], sum <= 100 (Nair = 100 - sum)
    float fair        = 20.0f;
    float bair        = 18.0f;
    float uair        = 16.0f;
    float dair        = 12.0f;
    float air_nspecial= 6.0f;
    float air_sspecial= 5.0f;
    float air_uspecial= 4.0f;
    float air_dspecial= 4.0f;

    // Additional Behavior: Directional Air Dodge & Taunts (0x19E - 0x1A1)
    float escape_air_forward  = 35.0f; // 0x19Eb0-b5 + 0x19Fb6-b7
    float escape_air_backward = 35.0f; // 0x19Fb0-b5 + 0x1A0b6-b7
    float appeal_hi           = 35.0f; // 0x1A0b0-b5 + 0x1A1b7
    float appeal_lw           = 35.0f; // 0x1A1b0-b6

    float ImplicitJabPercent() const;
    float ImplicitNairPercent() const;
    float ImplicitNeutralAirDodgePercent() const;
    float ImplicitSideTauntPercent() const;
    void NormalizeMoveSums();
};

class AmiiboBin {
public:
    AmiiboBin();

    bool LoadFromFile(const std::string& path, std::string* error_out = nullptr);
    bool SaveToFile(const std::string& path, std::string* error_out = nullptr);

    void InitializeBlankFighter(uint16_t char_id, uint8_t skin = 0);

    // Owner Mii & Amiibo Nickname Auto-Pairing (0x02C..0x0AB in Internal Layout)
    bool HasOwnerMii() const;
    bool VerifyOwnerMiiCrc16() const;
    bool VerifyOwnerMiiBitfields() const;
    uint16_t GetOwnerMiiCrc16() const;
    std::string GetAmiiboNickname() const;
    void SetAmiiboNickname(const std::string& nickname);
    std::string GetOwnerMiiName() const;
    void SetOwnerMiiName(const std::string& mii_name);
    void AutoPairOwnerMii(const std::string& mii_name = "SwitchMii", const std::string& nickname = "");
    void EnsureOwnerMiiPaired();

    // Full Nintendo Switch nn::nfp & NTAG215 Hardware/Cryptographic Verification
    bool VerifySwitchCompatibility(std::string* diagnostic_out = nullptr) const;
    bool VerifyRetailKeyHash() const;
    bool VerifyNtag215PwdPack() const;
    uint32_t GetAppDataCrc32() const;

    // Retail key support (Built-in 160-byte key_retail.bin via nfc3d DRBG + AES-128-CTR + HMAC-SHA256)
    bool LoadRetailKey(const std::string& key_path, std::string* error_out = nullptr);
    bool TryAutoLoadRetailKey();
    void SetRetailKeyBytes(const std::vector<uint8_t>& key_bytes);
    bool HasRetailKey() const { return has_retail_key_; }
    bool WasLoadedEncrypted() const { return was_loaded_encrypted_; }
    std::string GetFormatStatusString() const;

    // Metadata & Settings (64-bit Amiibo Figure ID at 0x1DC..0x1E3 in Internal Layout)
    uint64_t GetAmiiboId() const;
    void SetAmiiboId(uint64_t amiibo_id);
    uint16_t GetCharacterId() const;
    void SetCharacterId(uint16_t char_id);

    bool GetLearningEnabled() const;
    void SetLearningEnabled(bool enabled);

    uint8_t GetAlternateSkin() const;
    void SetAlternateSkin(uint8_t skin);

    // Experience & Levels (0x14C-0x14F)
    uint16_t GetLevelExperience() const;
    void SetLevelExperience(uint16_t xp);
    int GetDisplayedLevel() const;
    void SetDisplayedLevel(int level); // 1..50

    uint16_t GetCpuExperience() const;
    void SetCpuExperience(uint16_t cpu_xp);
    int GetCpuLevel() const;
    void SetCpuLevel(int cpu_level); // 1..9

    uint16_t GetGiftCount() const;
    void SetGiftCount(uint16_t count);

    // Spirits & Stats (0xE3, 0xEC-0xEE, 0x150-0x153, 0x1A4-0x1A7)
    SpiritType GetSpiritType() const;
    void SetSpiritType(SpiritType type);

    int16_t GetAttackStat() const;
    int16_t GetDefenseStat() const;
    void SetStats(int16_t attack, int16_t defense, bool enforce_legal_cap = true);

    std::array<uint8_t, 3> GetSpiritSlots() const;
    std::array<uint8_t, 3> GetLogicalSpiritList() const; // Normalized non-zero abilities
    int GetTotalSpiritSlotCost() const;
    bool SetSpiritSlots(uint8_t slot1, uint8_t slot2, uint8_t slot3,
                        bool enforce_slot_cost = true,
                        std::string* error_out = nullptr);

    bool ApplyPrimarySpiritPreset(const PrimarySpiritPreset& preset);

    // Behavior & Moves (0x168 - 0x1A1)
    AmiiboBehaviorParams DecodeBehavior() const;
    void EncodeBehavior(const AmiiboBehaviorParams& params);

    // Exact SSBU 25-state Personality Evaluator (matching jozz024/smash-amiibo-editor/resources/personality_data.json)
    std::string EvaluatePersonality() const;

    const std::vector<uint8_t>& RawBytes() const { return data_; }

    // Direct nfc3d tag <-> internal conversion helpers
    static void TagToInternal(const uint8_t* tag540, uint8_t* intl540);
    static void InternalToTag(const uint8_t* intl540, uint8_t* tag540);

private:
    std::vector<uint8_t> data_; // Stored in unpacked Internal/Plain layout (540 bytes)
    std::vector<uint8_t> retail_key_;
    bool has_retail_key_ = false;
    bool was_loaded_encrypted_ = false;
    bool loaded_as_tag_layout_ = false;

    uint32_t ReadBits(size_t start_byte, int start_bit_in_first,
                      size_t end_byte, int end_bit_in_last) const;
    void WriteBits(size_t start_byte, int start_bit_in_first,
                   size_t end_byte, int end_bit_in_last, uint32_t value);

    void EnsureValidNtag215HeaderAndLockBytes();
    void UpdateChecksumsAndCrypto();
};

} // namespace ssbu
