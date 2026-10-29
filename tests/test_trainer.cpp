#include "amiibo_bin.hpp"
#include "ssbu_loadouts.hpp"
#include "training_sim.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>

int main() {
    using namespace ssbu;

    // 1. Verify Fighter & Spirit Catalogs
    const auto& fighters = GetFighterCatalog();
    assert(!fighters.empty());
    const auto* bowser = FindFighterByName("Bowser");
    assert(bowser != nullptr && bowser->char_id == 0x0005);

    const auto& support_spirits = GetSupportSpiritCatalog();
    assert(support_spirits.size() == 255);
    const auto* super_armor = FindSupportAbilityByName("Super Armor");
    assert(super_armor != nullptr && super_armor->id == 10 && super_armor->slot_cost == 3);
    const auto* move_speed = FindSupportAbilityByName("Move Speed Up");
    assert(move_speed != nullptr && move_speed->id == 1 && move_speed->slot_cost == 1);

    // 2. Verify Blank Amiibo Initialization & Bitfield Offsets
    AmiiboBin bin;
    bin.InitializeBlankFighter(bowser->char_id, 3);
    assert(bin.GetCharacterId() == 0x0005);
    assert(bin.GetAlternateSkin() == 3);
    assert(bin.GetDisplayedLevel() == 1);
    assert(bin.GetCpuLevel() == 1);
    assert(bin.GetLearningEnabled() == true);

    // 3. Verify Primary & Support Spirit Equipping and 3-Slot Limit Validation
    const auto* preset = FindPrimarySpiritByName("Bowser (Wedding)");
    assert(preset != nullptr);
    bin.ApplyPrimarySpiritPreset(*preset);
    assert(bin.GetSpiritType() == SpiritType::Attack);
    assert(bin.GetAttackStat() == 2800 && bin.GetDefenseStat() == 2200);

    std::string err;
    // Exceeding 3 slots (Super Armor [3] + Move Speed Up [1] = 4) must fail when enforce_slot_cost=true
    bool bad_slots = bin.SetSpiritSlots(10, 1, 0, true, &err);
    assert(!bad_slots);

    // Valid 3-slot loadout: Additional Midair Jump (id=5, cost=2) + Landing Lag Down (id=81, cost=1) = 3 slots
    bool ok_slots = bin.SetSpiritSlots(5, 81, 0, true, &err);
    assert(ok_slots);
    auto slots = bin.GetSpiritSlots();
    // Because Slot 1 takes 2 slots, Slot 2 is shifted to Slot 3 (0xEE) per SSBU Amiibo Bin Guide
    assert(slots[0] == 5 && slots[2] == 81);

    // 4. Verify Behavior Bitfield Packing/Unpacking & Personality Evaluator
    AmiiboBehaviorParams p = bin.DecodeBehavior();
    p.just_shield_master = 0.90f;
    p.feint_master = 0.85f;
    p.feint_counter = 0.85f;
    p.catcher = 0.80f;
    bin.EncodeBehavior(p);
    AmiiboBehaviorParams p_out = bin.DecodeBehavior();
    assert(std::fabs(p_out.just_shield_master - 0.90f) < 0.03f);
    assert(std::fabs(p_out.feint_master - 0.85f) < 0.03f);
    assert(p_out.ImplicitJabPercent() >= 0.0f);
    assert(p_out.ImplicitNairPercent() >= 0.0f);

    // 5. Run a 500-game Training Session & Verify Progression and File Round-Trip
    TrainingConfig cfg{};
    cfg.num_games = 500;
    cfg.archetype = OpponentArchetype::FullRosterGauntlet;
    cfg.focus = TrainingFocus::TournamentOptimal;

    auto summary = TrainingSimulator::RunTrainingSession(bin, cfg);
    assert(summary.total_games == 500);
    assert(summary.wins + summary.losses == 500);
    assert(summary.final_level == 50);
    assert(summary.final_cpu_level == 9);
    assert(summary.final_quarter_win_rate >= summary.first_quarter_win_rate);
    assert(!summary.final_personality.empty());

    const char* tmp_file = "test_trained_bowser.bin";
    assert(bin.SaveToFile(tmp_file, &err));

    AmiiboBin reloaded;
    assert(reloaded.LoadFromFile(tmp_file, &err));
    assert(reloaded.RawBytes().size() == 540);
    assert(reloaded.GetCharacterId() == 0x0005);
    assert(reloaded.GetDisplayedLevel() == 50);
    assert(reloaded.GetCpuLevel() == 9);
    assert(reloaded.GetAttackStat() == 2800);
    assert(reloaded.GetDefenseStat() == 2200);
    std::remove(tmp_file);

    std::cout << "[PASS] All SSBU Amiibo .bin parser, Spirit Collection, and 500-game training tests passed!\n";
    return 0;
}
