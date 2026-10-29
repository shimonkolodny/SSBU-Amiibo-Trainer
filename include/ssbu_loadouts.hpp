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
    const char* description;
};

struct PrimarySpiritPreset {
    uint16_t id;
    const char* name;
    SpiritType type;
    int16_t base_attack;
    int16_t base_defense;
    uint8_t slots;
    const char* series;
};

struct FighterInfo {
    uint16_t char_id;
    const char* name;
    float weight_class;     // 0.6 (light) .. 1.4 (super heavy)
    float air_mobility;     // 0.5 .. 1.5
    float projectile_rate;  // 0.0 .. 1.0
    float grab_reward;      // 0.5 .. 1.5
};

const std::vector<SupportSpiritAbility>& GetSupportSpiritCatalog();
const SupportSpiritAbility* FindSupportAbilityById(uint8_t id);
const SupportSpiritAbility* FindSupportAbilityByName(const std::string& query);

const std::vector<PrimarySpiritPreset>& GetPrimarySpiritCatalog();
const PrimarySpiritPreset* FindPrimarySpiritByName(const std::string& query);

const std::vector<FighterInfo>& GetFighterCatalog();
const FighterInfo* FindFighterById(uint16_t char_id);
const FighterInfo* FindFighterByName(const std::string& query);

const char* SpiritTypeToString(SpiritType t);

} // namespace ssbu
