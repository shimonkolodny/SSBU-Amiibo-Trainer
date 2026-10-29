#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ssbu {

enum class SpiritType : uint8_t {
    Neutral = 0,
    Attack  = 1,
    Defense = 2,
    Grab    = 3
};

struct SupportSpiritAbility {
    uint8_t id;
    const char* name;
    uint8_t slot_cost;
    bool tournament_legal;
    bool is_selectable; // False for internal Reserved_XX placeholders
    const char* description;
};

struct PrimarySpiritPreset {
    uint16_t id;
    const char* name;
    SpiritType type;
    int16_t base_attack;
    int16_t base_defense;
    uint8_t slots;
    const char* rank;   // Legend, Ace, Advanced, Novice
    const char* series;
};

struct FighterInfo {
    uint16_t char_id;
    const char* roster_num;
    const char* name;
    float weight_class;     // 0.65 (light) .. 1.40 (super heavy)
    float air_mobility;     // 0.70 .. 1.45
    float projectile_rate;  // 0.00 .. 0.95
    float grab_reward;      // 0.85 .. 1.45
};

const std::vector<SupportSpiritAbility>& GetSupportSpiritCatalog();
std::vector<const SupportSpiritAbility*> GetSelectableSupportSpirits(bool legal_only = false, const std::string& filter = "");
const SupportSpiritAbility* FindSupportAbilityById(uint8_t id);
const SupportSpiritAbility* FindSupportAbilityByName(const std::string& query);

const std::vector<PrimarySpiritPreset>& GetPrimarySpiritCatalog();
std::vector<const PrimarySpiritPreset*> FilterPrimarySpirits(const std::string& filter = "");
const PrimarySpiritPreset* FindPrimarySpiritByName(const std::string& query);

const std::vector<FighterInfo>& GetFighterCatalog();
const FighterInfo* FindFighterById(uint16_t char_id);
const FighterInfo* FindFighterByName(const std::string& query);

const char* SpiritTypeToString(SpiritType t);

} // namespace ssbu
