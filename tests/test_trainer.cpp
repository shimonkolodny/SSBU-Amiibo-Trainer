#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "amiibo_bin.hpp"
#include "gpu_compute.hpp"
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

    const auto& roster = ssbu::GetFighterCatalog();
    assert(roster.size() >= 91);
    assert(ssbu::FindFighterByName("Mario") != nullptr);
    assert(ssbu::FindFighterByName("Sora") != nullptr);
    assert(ssbu::FindFighterByName("Kazuya") != nullptr);
    assert(ssbu::FindFighterByName("Sephiroth") != nullptr);
    assert(ssbu::FindFighterByName("Pyra") != nullptr);
    assert(ssbu::FindFighterByName("Mythra") != nullptr);

    // Verify EVERY single fighter in the 91-fighter catalog has a canonical 64-bit SSBU Amiibo ID
    // from jozz024/smash-amiibo-editor/resources/characters.json and round-trips cleanly
    ssbu::AmiiboBin bin;
    for (const auto& f : roster) {
        assert(ssbu::IsCanonicalSsbuAmiiboId(f.amiibo_id));
        bin.SetCharacterId(f.char_id);
        assert(bin.GetCharacterId() == f.char_id);
        assert(bin.GetAmiiboId() == f.amiibo_id);
        assert(bin.VerifySwitchCompatibility());
    }

    std::cout << "[PASS] TestSpiritCatalogAndRoster (255 raw / " << selectable.size()
              << " selectable Support Spirits, " << primaries.size()
              << " Primary Spirits, " << roster.size() << " Canonical 64-bit SSBU Fighters)\n";
}

void TestSwitchNfpCompatibilityAndAutoMii() {
    // 1. Out-of-the-box AmiiboBin MUST have canonical 160-byte retail key (MD5: 45fd53569f5765eef9c337bd5172f937)
    ssbu::AmiiboBin bin;
    assert(bin.HasRetailKey());
    assert(bin.VerifyRetailKeyHash());
    bin.InitializeBlankFighter(0x3601, 0); // Sephiroth (0x3601000004210002)
    assert(bin.GetAmiiboId() == 0x3601000004210002ULL);
    assert(bin.HasOwnerMii());
    assert(bin.VerifyOwnerMiiCrc16());
    assert(bin.VerifyOwnerMiiBitfields());
    assert(bin.VerifyNtag215PwdPack());
    assert(bin.GetOwnerMiiName() == "SwitchMii");
    assert(bin.GetAmiiboNickname() == "Sephiroth");

    // 2. Verify full Nintendo Switch nn::nfp & SSBU AppData compatibility
    std::string diag;
    assert(bin.VerifySwitchCompatibility(&diag));

    // 3. Verify that SetCharacterId NEVER corrupts the Owner Mii at 0x04C..0x0AB
    bin.SetOwnerMiiName("Shimon");
    bin.SetAmiiboNickname("OneWing");
    uint16_t mii_crc_before = bin.GetOwnerMiiCrc16();
    bin.SetCharacterId(0x33C0); // Switch to Kazuya (0x33C0000004200002)
    assert(bin.GetCharacterId() == 0x33C0);
    assert(bin.GetAmiiboId() == 0x33C0000004200002ULL);
    assert(bin.HasOwnerMii());
    assert(bin.VerifyOwnerMiiCrc16());
    assert(bin.VerifyOwnerMiiBitfields());
    assert(bin.VerifyNtag215PwdPack());
    assert(bin.GetOwnerMiiCrc16() == mii_crc_before);
    assert(bin.VerifySwitchCompatibility(&diag));

    // 4. Verify that saving and reloading without any external key_retail.bin produces an encrypted NTAG215 Tag file
    const std::string out_file = "test_switch_ready_kazuya.bin";
    std::string err;
    assert(bin.SaveToFile(out_file, &err));

    // Inspect raw disk bytes to verify Tag Layout (UID at 0x000, BCC0 at 0x003, BCC1 at 0x008, CC at 0x00C..0x00F, 0xA5 at 0x010, FigID 33C0000004200002 at 0x054..0x05B, PWD/PACK at 0x214..0x219)
    std::ifstream raw_in(out_file, std::ios::binary);
    std::vector<uint8_t> disk_tag(540, 0);
    raw_in.read(reinterpret_cast<char*>(disk_tag.data()), 540);
    raw_in.close();
    assert(disk_tag[0x000] == 0x04);
    assert(disk_tag[0x003] == (0x88u ^ disk_tag[0x000] ^ disk_tag[0x001] ^ disk_tag[0x002]));
    assert(disk_tag[0x008] == (disk_tag[0x004] ^ disk_tag[0x005] ^ disk_tag[0x006] ^ disk_tag[0x007]));
    assert(disk_tag[0x00C] == 0xF1 && disk_tag[0x00D] == 0x10 && disk_tag[0x00E] == 0xFF && disk_tag[0x00F] == 0xEE);
    assert(disk_tag[0x010] == 0xA5);
    // Verify canonical 8-byte Figure ID for Kazuya (33 C0 00 00 04 20 00 02) at tag[0x054..0x05B]
    assert(disk_tag[0x054] == 0x33 && disk_tag[0x055] == 0xC0 &&
           disk_tag[0x056] == 0x00 && disk_tag[0x057] == 0x00 &&
           disk_tag[0x058] == 0x04 && disk_tag[0x059] == 0x20 &&
           disk_tag[0x05A] == 0x00 && disk_tag[0x05B] == 0x02);
    // Verify NTAG215 Page 133 PWD & Page 134 PACK (0x8080)
    assert(disk_tag[0x214] == (disk_tag[0x001] ^ disk_tag[0x004] ^ 0xAAu));
    assert(disk_tag[0x215] == (disk_tag[0x002] ^ disk_tag[0x005] ^ 0x55u));
    assert(disk_tag[0x216] == (disk_tag[0x004] ^ disk_tag[0x006] ^ 0xAAu));
    assert(disk_tag[0x217] == (disk_tag[0x005] ^ disk_tag[0x007] ^ 0x55u));
    assert(disk_tag[0x218] == 0x80 && disk_tag[0x219] == 0x80);

    // 5. Verify that passing a corrupt external key_retail.bin is rejected and falls back to the canonical built-in retail key
    std::vector<uint8_t> corrupt_key(160, 0xAA);
    bin.SetRetailKeyBytes(corrupt_key);
    assert(bin.VerifyRetailKeyHash()); // Must remain canonical built-in retail key

    ssbu::AmiiboBin reloaded;
    assert(reloaded.LoadFromFile(out_file, &err));
    assert(reloaded.WasLoadedEncrypted());
    assert(reloaded.GetCharacterId() == 0x33C0);
    assert(reloaded.GetAmiiboId() == 0x33C0000004200002ULL);
    assert(reloaded.HasOwnerMii());
    assert(reloaded.GetOwnerMiiName() == "Shimon");
    assert(reloaded.GetAmiiboNickname() == "OneWing");
    assert(reloaded.VerifySwitchCompatibility(&diag));
    std::remove(out_file.c_str());

    std::cout << "[PASS] TestSwitchNfpCompatibilityAndAutoMii (" << diag << ")\n";
}

void TestBitfieldPackingAndStats() {
    ssbu::AmiiboBin bin;
    bin.InitializeBlankFighter(0x0580, 3); // Fox, Skin 3
    assert(bin.GetCharacterId() == 0x0580);
    assert(bin.GetAmiiboId() == 0x0580000000050002ULL);
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

    std::string serr;
    bool bad_slots = bin.SetSpiritSlots(10, 20, 0, true, &serr);
    assert(!bad_slots);

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
    assert(sephiroth->amiibo_id == 0x3601000004210002ULL);

    ssbu::AmiiboBin bin;
    bin.InitializeBlankFighter(sephiroth->char_id, 2);
    bin.SetSpiritType(ssbu::SpiritType::Attack);
    bin.SetStats(2900, 2100, false);
    bin.SetSpiritSlots(162, 81, 0, true);
    bin.SetDisplayedLevel(50);

    const std::string enc_file = "test_encrypted_sephiroth.bin";
    std::string err;
    bool saved = bin.SaveToFile(enc_file, &err);
    assert(saved);

    ssbu::AmiiboBin reloaded;
    bool loaded = reloaded.LoadFromFile(enc_file, &err);
    assert(loaded);
    assert(reloaded.WasLoadedEncrypted());
    assert(reloaded.HasOwnerMii());
    assert(reloaded.VerifyOwnerMiiCrc16());
    assert(reloaded.VerifyOwnerMiiBitfields());
    assert(reloaded.VerifyRetailKeyHash());
    assert(reloaded.VerifyNtag215PwdPack());
    assert(reloaded.VerifySwitchCompatibility());
    assert(reloaded.GetCharacterId() == sephiroth->char_id);
    assert(reloaded.GetAmiiboId() == 0x3601000004210002ULL);
    assert(reloaded.GetAttackStat() == 2900);
    assert(reloaded.GetDefenseStat() == 2100);
    assert(reloaded.GetDisplayedLevel() == 50);
    auto slots = reloaded.GetLogicalSpiritList();
    assert(slots[0] == 162 && slots[1] == 81);
    std::remove(enc_file.c_str());

    std::cout << "[PASS] TestNtag215TagConversionAndCryptoRoundTrip (Sephiroth 0x3601000004210002 + SSBU Little-Endian CRC-32)\n";
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

void TestGpuFirstComputeEngine() {
    ssbu::GpuDeviceInfo info = ssbu::GpuComputeEngine::ProbeGpuDevice();
    assert(!info.backend_name.empty());
    assert(!info.adapter_name.empty());

#ifdef _WIN32
    assert(info.gpu_available);
    ssbu::GpuSimParams params{};
    params.num_games = 256;
    params.random_seed = 1337;
    std::vector<ssbu::GpuMatchOutcome> outcomes;
    ssbu::GpuDeviceInfo used_dev{};
    double ms = 0.0;
    std::string err;
    bool ok = ssbu::GpuComputeEngine::TryRunMatchBatchOnGpu(params, outcomes, &used_dev, &ms, &err);
    assert(ok);
    assert(outcomes.size() == 256);
    assert(outcomes[0].frames_simulated > 100);
    assert(outcomes[0].hits_landed > 0);
#endif

    std::cout << "[PASS] TestGpuFirstComputeEngine (Backend: " << info.backend_name
              << " | Adapter: " << info.adapter_name << ")\n";
}

void TestTrainingSimulation500Games60Fps() {
    ssbu::AmiiboBin bin;
    bin.InitializeBlankFighter(0x0000, 0); // Mario
    bin.SetStats(2500, 2500, true);
    bin.SetSpiritSlots(20, 81, 0, true); // Autoheal (20) + Landing Lag Down (81)

    ssbu::TrainingConfig cfg{};
    cfg.num_games = 500;
    cfg.opponent_char_id = 0x0580; // Fox
    cfg.archetype = ssbu::OpponentArchetype::AerialEdgeguarder;
    cfg.focus = ssbu::TrainingFocus::AerialMeteorEdgeguarder;
    cfg.compute_preference = ssbu::ComputePreference::GpuFirstAuto;
    cfg.freeze_learning_after_training = true;
    cfg.random_seed = 42;

    ssbu::TrainingSummary sum = ssbu::TrainingSimulator::RunTrainingSession(bin, cfg);
    assert(sum.total_games == 500);
    assert(sum.wins + sum.losses == 500);
    assert(sum.final_level == 50);
    assert(sum.total_frames_simulated > 200000); // Hundreds of thousands of 60 FPS frames simulated
    assert(sum.total_hits_landed > 2000);
    assert(sum.total_grabs_landed > 0);
    assert(sum.total_ledge_recoveries > 0);
    assert(!sum.sample_matches.empty());
    assert(sum.sample_matches.front().frames_simulated > 100);
    assert(bin.GetDisplayedLevel() == 50);
    assert(bin.GetCpuLevel() == 9);
    assert(bin.HasOwnerMii());
    assert(bin.VerifyOwnerMiiCrc16());
    assert(bin.VerifyOwnerMiiBitfields());
    assert(bin.VerifyRetailKeyHash());
    assert(bin.VerifyNtag215PwdPack());
    assert(bin.VerifySwitchCompatibility());
    assert(!bin.GetLearningEnabled()); // Frozen after training
#ifdef _WIN32
    assert(sum.executed_on_gpu);
#endif

    ssbu::AmiiboBehaviorParams trained_bp = bin.DecodeBehavior();
    assert(trained_bp.air_offensive > 0.65f);
    assert(trained_bp.attack_out_cliff > 0.55f);
    assert(trained_bp.meteor_master > 0.55f);

    const std::string tmp_file = "test_trained_amiibo_v25.bin";
    std::string err;
    bool saved = bin.SaveToFile(tmp_file, &err);
    assert(saved);

    ssbu::AmiiboBin reloaded;
    bool loaded = reloaded.LoadFromFile(tmp_file, &err);
    assert(loaded);
    assert(reloaded.WasLoadedEncrypted());
    assert(reloaded.VerifySwitchCompatibility());
    assert(reloaded.GetDisplayedLevel() == 50);
    assert(reloaded.GetAttackStat() == 2500);
    assert(reloaded.GetDefenseStat() == 2500);
    assert(reloaded.GetLogicalSpiritList()[0] == 20);
    assert(reloaded.GetLogicalSpiritList()[1] == 81);
    std::remove(tmp_file.c_str());

    float win_rate = 100.0f * static_cast<float>(sum.wins) / static_cast<float>(sum.total_games);
    std::cout << "[PASS] TestTrainingSimulation500Games60Fps (Compute: " << sum.compute_backend_used
              << ", Frames: " << sum.total_frames_simulated
              << ", Hits: " << sum.total_hits_landed
              << ", Win Rate: " << win_rate << "%, Style: "
              << sum.final_personality << ")\n";
}

} // namespace

int main() {
    std::cout << "Running SSBU Amiibo Trainer v2.5 (Switch nn::nfp + SSBU AppData Checksum Verified + 60 FPS GPU Physics) C++20 Unit & Integration Tests...\n";
    TestSpiritCatalogAndRoster();
    TestSwitchNfpCompatibilityAndAutoMii();
    TestBitfieldPackingAndStats();
    TestNtag215TagConversionAndCryptoRoundTrip();
    TestPersonalityScoringBranches();
    TestGpuFirstComputeEngine();
    TestTrainingSimulation500Games60Fps();
    std::cout << "All SSBU Amiibo Trainer v2.5 tests passed!\n";
    return 0;
}
