#include "amiibo_bin.hpp"
#include "ssbu_loadouts.hpp"
#include "training_sim.hpp"
#include "win32_gui.hpp"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void PrintUsage(const char* prog) {
    std::cout << "SSBU Amiibo .bin Trainer & Spirit Collection Studio\n"
              << "Usage:\n"
              << "  " << prog << " --gui                        Launch native Windows GUI (default on Windows if no args)\n"
              << "  " << prog << " [options]\n\n"
              << "Options:\n"
              << "  --input <path>               Input 540-byte NTAG215 Amiibo .bin file\n"
              << "  --generate <fighter>         Generate a fresh Level 1 Amiibo .bin for <fighter>\n"
              << "  --output <path>              Save trained 540-byte Amiibo .bin to <path>\n"
              << "  --key <path>                 Optional 160-byte key_retail.bin path\n"
              << "  --games <N>                  Number of simulated SSBU games to run (default: 500)\n"
              << "  --opponent <fighter>         Opponent fighter name (default: Full Roster Gauntlet)\n"
              << "  --archetype <0..6>           0=Balanced, 1=Rushdown, 2=Zoner, 3=Parry, 4=Grappler, 5=Aerial, 6=Gauntlet\n"
              << "  --focus <0..5>               0=Tournament, 1=Aggressive, 2=Parry/Ace, 3=Aerial/Meteor, 4=Entertainer, 5=Adaptive\n"
              << "  --primary-spirit <name>      Apply Primary Spirit from SSBU Spirit Collection\n"
              << "  --attack <val>               Set custom Attack stat (-2000..5000)\n"
              << "  --defense <val>              Set custom Defense stat (-2000..5000)\n"
              << "  --slot1 <id|name>            Equip Support Spirit Ability in Slot 1 (0..254 or name)\n"
              << "  --slot2 <id|name>            Equip Support Spirit Ability in Slot 2 (0..254 or name)\n"
              << "  --slot3 <id|name>            Equip Support Spirit Ability in Slot 3 (0..254 or name)\n"
              << "  --unrestricted               Disable 5000 stat cap and 3-slot Spirit cost limit\n"
              << "  --list-spirits               List all Primary and Support Spirits in the SSBU collection\n"
              << "  --list-fighters              List supported SSBU fighters and Character IDs\n"
              << "  --inspect                    Inspect input .bin stats, spirits, personality, and AI parameters\n";
}

uint8_t ParseSpiritSlotArg(const std::string& arg) {
    bool all_digits = !arg.empty();
    for (char c : arg) {
        if (c < '0' || c > '9') { all_digits = false; break; }
    }
    if (all_digits) {
        return static_cast<uint8_t>(std::clamp(std::atoi(arg.c_str()), 0, 254));
    }
    const auto* ab = ssbu::FindSupportAbilityByName(arg);
    return ab ? ab->id : 0;
}

void PrintAmiiboInspection(const ssbu::AmiiboBin& bin) {
    const auto* f = ssbu::FindFighterById(bin.GetCharacterId());
    auto slots = bin.GetSpiritSlots();
    auto beh = bin.DecodeBehavior();
    std::cout << "=== SSBU Amiibo .bin Inspection ===\n"
              << "Fighter:           " << (f ? f->name : "Unknown")
              << " (0x" << std::hex << std::setw(4) << std::setfill('0') << bin.GetCharacterId() << std::dec << ")\n"
              << "Displayed Level:   " << bin.GetDisplayedLevel() << " (XP: " << bin.GetLevelExperience() << " / 3912)\n"
              << "Hidden CPU Level:  " << bin.GetCpuLevel() << " (XP: " << bin.GetCpuExperience() << " / 2765)\n"
              << "Personality:       " << bin.EvaluatePersonality() << "\n"
              << "Learning Enabled:  " << (bin.GetLearningEnabled() ? "Yes" : "No") << "\n"
              << "Spirit Type:       " << ssbu::SpiritTypeToString(bin.GetSpiritType()) << "\n"
              << "Attack / Defense:  " << bin.GetAttackStat() << " / " << bin.GetDefenseStat() << "\n";
    for (int i = 0; i < 3; ++i) {
        const auto* ab = ssbu::FindSupportAbilityById(slots[static_cast<size_t>(i)]);
        std::cout << "Spirit Slot " << (i + 1) << ":     [" << int(slots[static_cast<size_t>(i)]) << "] "
                  << (ab ? ab->name : "None") << " (Cost: " << (ab ? int(ab->slot_cost) : 0) << ")\n";
    }
    std::cout << std::fixed << std::setprecision(1)
              << "Behavior Weights:  Offensive=" << beh.offensive * 100.0f
              << "% Grounded=" << beh.grounded * 100.0f
              << "% Near=" << beh.near_range * 100.0f
              << "% Parry=" << beh.just_shield_master * 100.0f
              << "% Grab=" << beh.catcher * 100.0f
              << "% Edgeguard=" << beh.attack_out_cliff * 100.0f
              << "% Meteor=" << beh.meteor_master * 100.0f << "%\n"
              << "Grounded Moves:    FTilt=" << beh.ftilt << "% UTilt=" << beh.utilt
              << "% DTilt=" << beh.dtilt << "% FSmash=" << beh.fsmash
              << "% USmash=" << beh.usmash << "% DSmash=" << beh.dsmash
              << "% ImplicitJab=" << beh.ImplicitJabPercent() << "%\n"
              << "Aerial Moves:      FAir=" << beh.fair << "% BAir=" << beh.bair
              << "% UAir=" << beh.uair << "% DAir=" << beh.dair
              << "% ImplicitNAir=" << beh.ImplicitNairPercent() << "%\n";
}

} // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
    if (argc == 1) {
        return ssbu::RunWin32Gui();
    }
#endif
    if (argc == 1) {
        PrintUsage(argv[0]);
        return 0;
    }

    std::string input_path;
    std::string output_path;
    std::string key_path;
    std::string gen_fighter;
    std::string primary_spirit;
    std::string opp_fighter;
    int games = 0;
    int archetype = static_cast<int>(ssbu::OpponentArchetype::FullRosterGauntlet);
    int focus = static_cast<int>(ssbu::TrainingFocus::TournamentOptimal);
    bool has_atk = false, has_def = false;
    int16_t custom_atk = 0, custom_def = 0;
    bool has_slots = false;
    uint8_t slot1 = 0, slot2 = 0, slot3 = 0;
    bool unrestricted = false;
    bool inspect_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "--gui") {
            return ssbu::RunWin32Gui();
        } else if (arg == "--list-spirits") {
            std::cout << "=== SSBU Primary Spirit Presets ===\n";
            for (const auto& p : ssbu::GetPrimarySpiritCatalog()) {
                std::cout << "  #" << std::setw(4) << p.id << " " << std::setw(24) << std::left << p.name
                          << " Type=" << std::setw(8) << ssbu::SpiritTypeToString(p.type)
                          << " Atk=" << std::setw(5) << p.base_attack
                          << " Def=" << std::setw(5) << p.base_defense
                          << " Slots=" << int(p.slots) << " (" << p.series << ")\n";
            }
            std::cout << "\n=== SSBU Support Spirit Abilities (0..254) ===\n";
            for (const auto& s : ssbu::GetSupportSpiritCatalog()) {
                if (std::string(s.name).rfind("Reserved_", 0) == 0) continue;
                std::cout << "  [" << std::setw(3) << int(s.id) << "] " << std::setw(36) << std::left << s.name
                          << " Cost=" << int(s.slot_cost)
                          << (s.tournament_legal ? " (Legal) " : " (Bonus) ")
                          << s.description << "\n";
            }
            return 0;
        } else if (arg == "--list-fighters") {
            std::cout << "=== SSBU Fighter Catalog ===\n";
            for (const auto& f : ssbu::GetFighterCatalog()) {
                std::cout << "  0x" << std::hex << std::setw(4) << std::setfill('0') << f.char_id
                          << std::dec << std::setfill(' ') << "  " << f.name << "\n";
            }
            return 0;
        } else if (arg == "--input" && i + 1 < argc) {
            input_path = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            output_path = argv[++i];
        } else if (arg == "--key" && i + 1 < argc) {
            key_path = argv[++i];
        } else if (arg == "--generate" && i + 1 < argc) {
            gen_fighter = argv[++i];
        } else if (arg == "--games" && i + 1 < argc) {
            games = std::atoi(argv[++i]);
        } else if (arg == "--opponent" && i + 1 < argc) {
            opp_fighter = argv[++i];
        } else if (arg == "--archetype" && i + 1 < argc) {
            archetype = std::clamp(std::atoi(argv[++i]), 0, 6);
        } else if (arg == "--focus" && i + 1 < argc) {
            focus = std::clamp(std::atoi(argv[++i]), 0, 5);
        } else if (arg == "--primary-spirit" && i + 1 < argc) {
            primary_spirit = argv[++i];
        } else if (arg == "--attack" && i + 1 < argc) {
            has_atk = true;
            custom_atk = static_cast<int16_t>(std::atoi(argv[++i]));
        } else if (arg == "--defense" && i + 1 < argc) {
            has_def = true;
            custom_def = static_cast<int16_t>(std::atoi(argv[++i]));
        } else if (arg == "--slot1" && i + 1 < argc) {
            has_slots = true;
            slot1 = ParseSpiritSlotArg(argv[++i]);
        } else if (arg == "--slot2" && i + 1 < argc) {
            has_slots = true;
            slot2 = ParseSpiritSlotArg(argv[++i]);
        } else if (arg == "--slot3" && i + 1 < argc) {
            has_slots = true;
            slot3 = ParseSpiritSlotArg(argv[++i]);
        } else if (arg == "--unrestricted") {
            unrestricted = true;
        } else if (arg == "--inspect") {
            inspect_only = true;
        }
    }

    ssbu::AmiiboBin bin;
    std::string err;

    if (!key_path.empty()) {
        if (!bin.LoadRetailKey(key_path, &err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
    }

    if (!input_path.empty()) {
        if (!bin.LoadFromFile(input_path, &err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
    } else if (!gen_fighter.empty()) {
        const auto* f = ssbu::FindFighterByName(gen_fighter);
        bin.InitializeBlankFighter(f ? f->char_id : 0x0000, 0);
    }

    if (!primary_spirit.empty()) {
        const auto* p = ssbu::FindPrimarySpiritByName(primary_spirit);
        if (!p) {
            std::cerr << "Warning: Primary spirit '" << primary_spirit << "' not found.\n";
        } else {
            bin.ApplyPrimarySpiritPreset(*p);
        }
    }

    if (has_atk || has_def) {
        int16_t a = has_atk ? custom_atk : bin.GetAttackStat();
        int16_t d = has_def ? custom_def : bin.GetDefenseStat();
        bin.SetStats(a, d, !unrestricted);
    }

    if (has_slots) {
        auto cur = bin.GetSpiritSlots();
        uint8_t s1 = slot1 ? slot1 : cur[0];
        uint8_t s2 = slot2 ? slot2 : cur[1];
        uint8_t s3 = slot3 ? slot3 : cur[2];
        if (!bin.SetSpiritSlots(s1, s2, s3, !unrestricted, &err)) {
            std::cerr << "Spirit Error: " << err << "\n";
            return 1;
        }
    }

    if (inspect_only && games <= 0) {
        PrintAmiiboInspection(bin);
    }

    if (games > 0) {
        ssbu::TrainingConfig cfg{};
        cfg.num_games = games;
        cfg.archetype = static_cast<ssbu::OpponentArchetype>(archetype);
        cfg.focus = static_cast<ssbu::TrainingFocus>(focus);
        if (!opp_fighter.empty()) {
            const auto* of = ssbu::FindFighterByName(opp_fighter);
            if (of) {
                cfg.opponent_char_id = of->char_id;
                if (archetype == static_cast<int>(ssbu::OpponentArchetype::FullRosterGauntlet)) {
                    cfg.archetype = ssbu::OpponentArchetype::Balanced;
                }
            }
        }

        auto summary = ssbu::TrainingSimulator::RunTrainingSession(bin, cfg);
        std::cout << std::fixed << std::setprecision(1)
                  << "=== SSBU Amiibo Training Complete ===\n"
                  << "Games Simulated:   " << summary.total_games << " ("
                  << summary.wins << "W - " << summary.losses << "L)\n"
                  << "Win Rate Shift:    " << summary.first_quarter_win_rate * 100.0f
                  << "% (First 25%) -> " << summary.final_quarter_win_rate * 100.0f << "% (Final 25%)\n"
                  << "Level Progression: Lv " << summary.initial_level << " -> Lv " << summary.final_level
                  << " (CPU Lv " << summary.initial_cpu_level << " -> CPU Lv " << summary.final_cpu_level << ")\n"
                  << "Personality Shift: " << summary.initial_personality << " -> " << summary.final_personality << "\n\n";
        PrintAmiiboInspection(bin);
    }

    if (!output_path.empty()) {
        if (!bin.SaveToFile(output_path, &err)) {
            std::cerr << "Error saving .bin: " << err << "\n";
            return 1;
        }
        std::cout << "Saved 540-byte Amiibo .bin to: " << output_path << "\n";
    }

    return 0;
}
