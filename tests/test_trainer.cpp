#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "amiibo_bin.hpp"
#include "ssbu_loadouts.hpp"
#include "training_sim.hpp"

namespace {

void TestSpiritCatalogAndRoster() {
    const auto& spirits = ssbu::GetSupportSpiritCatalog();
    assert(spirits.size() == 255);
    assert(std::string(ssbu::FindSupportAbilityById(0)->name) == "None");
    assert(std::string(ssbu::FindSupportAbilityById(1)->name) == "Move Speed Up");
    assert(ssbu::FindSupportAbilityById(1)->slot_cost == 1);
    assert(std::string(ssbu::FindSupportAbilityById(10)->name) == "Super Armor");
    assert(ssbu::FindSupportAbilityById(10)->slot_cost == 3);
    assert(std::string(ssbu::FindSupportAbilityById(20)->name) == "Autoheal");
    assert(ssbu::FindSupportAbilityById(20)->slot_cost == 2);
    assert(std::string(ssbu::FindSupportAbilityById(81)->name) == "Landing Lag Down");
    assert(ssbu::FindSupportAbilityById(81)->slot_cost == 1);
    assert(std::string(ssbu::FindSupportAbilityById(140)->name) == "Armor Knight");
    assert(std::string(ssbu::FindSupportAbilityById(162)->name) == "Critical-Health Healing");
    assert(std::string(ssbu::FindSupportAbilityById(180)->name) == "Great Autoheal");

    // Selectable spirits should exclude internal "Reserved" placeholders
    auto selectable = ssbu::GetSelectableSupportSpirits(false, "");
    assert(selectable.size() > 140 && selectable.size() < 255);
    for (const auto* sp : selectable) {
        assert(sp->is_selectable);
        assert(std::string(sp->name).find("Reserved") == std::string::npos);
    }

    auto armor_results = ssbu::GetSelectableSupportSpirits(false, "armor");
    assert(armor_results.size() >= 3);

    const auto& primaries = ssbu::GetPrimarySpiritCatalog();
    assert(primaries.size() >= 60);

    // Verify complete 89-fighter SSBU roster
    const auto& roster = ssbu::GetFighterCatalog();
    assert(roster.size() >= 89);
    assert(ssbu::FindFighterByName("Mario") != nullptr);
    assert(ssbu::FindFighterByName("Sora") != nullptr);
    assert(ssbu::FindFighterByName("Kazuya") != nullptr);
    assert(ssbu::FindFighterByName("Sephiroth") != nullptr);
    assert(ssbu::FindFighterByName("Pyra") != nullptr);
    assert(ssbu::FindFighterByName("Mythra") != nullptr);

    std::cout << "[PASS] TestSpiritCatalogAndRoster (255 raw / " << selectable.size()
              << " selectable Support Spirits, " << primaries.size()
              << " Primary Spirits, " << roster.size() << " Fighters)\n";
}

void TestBitfieldPackingAndStats() {
    ssbu::AmiiboBin bin;
    bin.InitializeBlankFighter(0x0580, 3); // Fox, Skin 3
    assert(bin.GetCharacterId() == 0x0580);
    assert(bin.GetAlternateSkin() == 3);

    bin.SetLearningEnabled(false);
    assert(!bin.GetLearningEnabled());
    bin.SetLearningEnabled(true);
    assert(bin.GetLearningEnabled());

    bin.SetSpiritType(ssbu::SpiritType::Defense);
    bin.SetStats(2500, 2350, true);
    assert(bin.GetAttackStat() == 2500);
    assert(bin.GetDefenseStat() == 2350);
    assert(bin.GetSpiritType() == ssbu::SpiritType::Defense);

    // Slot validation: Super Armor (10, cost 3) + Autoheal (20, cost 2) -> exceeds 3 slots
    std::string serr;
    bool bad_slots = bin.SetSpiritSlots(10, 20, 0, true, &serr);
    assert(!bad_slots);

    // Valid 3-slot combo: Autoheal (20, cost 2) + Landing Lag Down (81, cost 1)
    bool good_slots = bin.SetSpiritSlots(20, 81, 0, true, &serr);
    assert(good_slots);
    auto logical_slots = bin.GetLogicalSpiritList();
    assert(logical_slots[0] == 20);
    assert(logical_slots[1] == 81);
    assert(logical_slots[2] == 0);
    assert(bin.GetTotalSpiritSlotCost() == 3);

    bin.SetDisplayedLevel(50);
    assert(bin.GetDisplayedLevel() == 50);
    assert(bin.GetLevelExperience() >= 2237);

    ssbu::AmiiboBehaviorParams bp{};
    bp.offensive = 0.88f;
    bp.grounded = 0.32f;
    bp.just_shield_master = 0.91f;
    bp.meteor_master = 0.77f;
    bp.ftilt = 14.0f;
    bp.fair = 24.0f;
    bp.uspecial = 9.0f;
    bin.EncodeBehavior(bp);

    ssbu::AmiiboBehaviorParams read_bp = bin.DecodeBehavior();
    assert(std::fabs(read_bp.offensive - bp.offensive) < 0.02f);
    assert(std::fabs(read_bp.grounded - bp.grounded) < 0.02f);
    assert(std::fabs(read_bp.just_shield_master - bp.just_shield_master) < 0.02f);
    assert(std::fabs(read_bp.meteor_master - bp.meteor_master) < 0.02f);
    assert(std::fabs(read_bp.ftilt - bp.ftilt) < 1.5f);
    assert(std::fabs(read_bp.fair - bp.fair) < 1.5f);
    assert(std::fabs(read_bp.uspecial - bp.uspecial) < 1.5f);

    std::cout << "[PASS] TestBitfieldPackingAndStats\n";
}

void TestNtag215TagConversionAndCryptoRoundTrip() {
    const auto* sephiroth = ssbu::FindFighterByName("Sephiroth");
    assert(sephiroth != nullptr);

    ssbu::AmiiboBin bin;
    bin.InitializeBlankFighter(sephiroth->char_id, 2);
    bin.SetSpiritType(ssbu::SpiritType::Attack);
    bin.SetStats(2900, 2100, false);
    bin.SetSpiritSlots(162, 81, 0, true); // Critical-Health Healing (2) + Landing Lag Down (1)
    bin.SetDisplayedLevel(50);

    // Generate deterministic 160-byte test retail key structure
    std::vector<uint8_t> key_buf(160, 0);
    for (size_t i = 0; i < 160; ++i) {
        key_buf[i] = static_cast<uint8_t>((i * 37 + 19) & 0xFF);
    }
    key_buf[14] = 16; // data.magicBytesSize <= 16
    key_buf[94] = 16; // tag.magicBytesSize <= 16

    bin.SetRetailKeyBytes(key_buf);
    assert(bin.HasRetailKey());

    const std::string enc_file = "test_encrypted_sephiroth.bin";
    std::string err;
    bool saved = bin.SaveToFile(enc_file, &err);
    assert(saved);

    // Reload encrypted file with same retail key and verify all SSBU AppData fields survived round-trip
    ssbu::AmiiboBin reloaded;
    reloaded.SetRetailKeyBytes(key_buf);
    bool loaded = reloaded.LoadFromFile(enc_file, &err);
    assert(loaded);
    assert(reloaded.WasLoadedEncrypted());
    assert(reloaded.GetCharacterId() == sephiroth->char_id);
    assert(reloaded.GetAttackStat() == 2900);
    assert(reloaded.GetDefenseStat() == 2100);
    assert(reloaded.GetDisplayedLevel() == 50);
    auto slots = reloaded.GetLogicalSpiritList();
    assert(slots[0] == 162 && slots[1] == 81);
    std::remove(enc_file.c_str());

    std::cout << "[PASS] TestNtag215TagConversionAndCryptoRoundTrip (nfc3d AES-128-CTR + HMAC-SHA256 + DRBG)\n";
}

void TestPersonalityScoringBranches() {
    ssbu::AmiiboBin bin;
    bin.InitializeBlankFighter(0x0000, 0);

    ssbu::AmiiboBehaviorParams show_off{};
    show_off.appeal = 0.95f;
    show_off.dragoon_collector = 0.85f;
    show_off.smashball_collector = 0.85f;
    show_off.hammer_collector = 0.85f;
    show_off.special_flagger = 0.85f;
    show_off.homerun_batter = 0.85f;
    show_off.club_swinger = 0.85f;
    show_off.death_swinger = 0.85f;
    bin.EncodeBehavior(show_off);
    std::string p_so = bin.EvaluatePersonality();
    assert(p_so == "Show-Off" || p_so == "Flashy" || p_so == "Entertainer");

    ssbu::AmiiboBehaviorParams daredevil{};
    daredevil.offensive = 0.98f;
    daredevil.air_offensive = 0.95f;
    daredevil.attack_out_cliff = 0.95f;
    daredevil.meteor_master = 0.95f;
    daredevil.smash_holder = 0.90f;
    daredevil.critical_hitter = 0.90f;
    daredevil.grounded = 0.15f;
    bin.EncodeBehavior(daredevil);
    std::string p_dd = bin.EvaluatePersonality();
    assert(!p_dd.empty() && p_dd != "Normal");

    std::cout << "[PASS] TestPersonalityScoringBranches (Verified " << p_so << " & " << p_dd << ")\n";
}

void TestTrainingSimulation500Games() {
    ssbu::AmiiboBin bin;
    bin.InitializeBlankFighter(0x0000, 0); // Mario
    bin.SetStats(2500, 2500, true);
    bin.SetSpiritSlots(20, 81, 0, true); // Autoheal (20) + Landing Lag Down (81)

    ssbu::TrainingConfig cfg{};
    cfg.num_games = 500;
    cfg.opponent_char_id = 0x0580; // Fox
    cfg.archetype = ssbu::OpponentArchetype::AerialEdgeguarder;
    cfg.focus = ssbu::TrainingFocus::AerialMeteorEdgeguarder;
    cfg.freeze_learning_after_training = true;
    cfg.random_seed = 42;

    ssbu::TrainingSummary sum = ssbu::TrainingSimulator::RunTrainingSession(bin, cfg);
    assert(sum.total_games == 500);
    assert(sum.wins + sum.losses == 500);
    assert(sum.final_level == 50);
    assert(bin.GetDisplayedLevel() == 50);
    assert(bin.GetCpuLevel() == 9);
    assert(!bin.GetLearningEnabled()); // Frozen after training

    ssbu::AmiiboBehaviorParams trained_bp = bin.DecodeBehavior();
    assert(trained_bp.air_offensive > 0.65f);
    assert(trained_bp.attack_out_cliff > 0.55f);
    assert(trained_bp.meteor_master > 0.55f);

    const std::string tmp_file = "test_trained_amiibo_v2.bin";
    std::string err;
    bool saved = bin.SaveToFile(tmp_file, &err);
    assert(saved);

    ssbu::AmiiboBin reloaded;
    bool loaded = reloaded.LoadFromFile(tmp_file, &err);
    assert(loaded);
    assert(reloaded.GetDisplayedLevel() == 50);
    assert(reloaded.GetAttackStat() == 2500);
    assert(reloaded.GetDefenseStat() == 2500);
    assert(reloaded.GetLogicalSpiritList()[0] == 20);
    assert(reloaded.GetLogicalSpiritList()[1] == 81);
    std::remove(tmp_file.c_str());

    float win_rate = 100.0f * static_cast<float>(sum.wins) / static_cast<float>(sum.total_games);
    std::cout << "[PASS] TestTrainingSimulation500Games (Final Lv: "
              << sum.final_level
              << ", Win Rate: " << win_rate << "%, Personality: "
              << sum.final_personality << ")\n";
}

} // namespace

int main() {
    std::cout << "Running SSBU Amiibo Trainer v2.0 C++20 Unit & Integration Tests...\n";
    TestSpiritCatalogAndRoster();
    TestBitfieldPackingAndStats();
    TestNtag215TagConversionAndCryptoRoundTrip();
    TestPersonalityScoringBranches();
    TestTrainingSimulation500Games();
    std::cout << "All SSBU Amiibo Trainer v2.0 tests passed!\n";
    return 0;
}
