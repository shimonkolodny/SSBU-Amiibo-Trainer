#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "amiibo_bin.hpp"
#include "gpu_compute.hpp"
#include "ssbu_loadouts.hpp"
#include "training_sim.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace ssbu {
int RunWin32Gui(HINSTANCE hInstance, int nCmdShow);
}
#endif

namespace {

void PrintUsage(const char* prog) {
    std::cout << "Super Smash Bros. Ultimate - Amiibo Trainer & Spirit Studio v2.5 (Switch-Verified + 60 FPS GPU Sim)\n"
              << "Usage:\n"
              << "  " << prog << " --gui                        Launch native Windows GUI (default on Windows)\n"
              << "  " << prog << " [options]                    Run headless CLI training & spirit customization\n\n"
              << "Options:\n"
              << "  --in <path>                  Input 540-byte Amiibo .bin file (auto-pairs Owner Mii if unregistered)\n"
              << "  --out <path>                 Output trained 540-byte Switch-ready Amiibo .bin file\n"
              << "  --keys <path>                Optional 160-byte key_retail.bin override (built-in by default)\n"
              << "  --nickname <name>            Set Amiibo Nickname (up to 10 UTF-16BE chars, auto-defaults to Fighter name)\n"
              << "  --mii-name <owner>           Set paired Owner Mii Name (up to 10 UTF-16LE chars, default: SwitchMii)\n"
              << "  --backend <auto|gpu|cpu>     Compute backend preference (default: auto = GPU First -> CPU Fallback)\n"
              << "  --gpu-info                   Probe and print GPU compute device capabilities\n"
              << "  --fighter <id_or_name>       Amiibo fighter ID (e.g. 0x0000) or name (e.g. \"Mario\", \"Kazuya\")\n"
              << "  --opponent <id_or_name>      Sparring opponent fighter ID or name (default: Fox)\n"
              << "  --games <N>                  Number of 60 FPS matches to simulate (default: 500)\n"
              << "  --archetype <name>           gauntlet | balanced | rushdown | zoner | parry | grappler | aerial\n"
              << "  --focus <name>               optimal | aggressive | parry | aerial | showoff | adaptive | heavy\n"
              << "  --primary-spirit <name>      Equip Primary Spirit preset by name\n"
              << "  --attack <val>               Explicit Primary Spirit Attack stat (-2000..5000)\n"
              << "  --defense <val>              Explicit Primary Spirit Defense stat (-2000..5000)\n"
              << "  --spirits <id1,id2,id3>      Comma-separated Support Spirit Ability IDs (0..254)\n"
              << "  --list-spirits [query]       Search or list Support Spirits in SSBU\n"
              << "  --list-primaries             List Primary Spirit presets\n"
              << "  --list-fighters              List all 89 SSBU playable fighters & character IDs\n"
              << "  --inspect                    Print detailed Amiibo & Switch nn::nfp diagnostic report\n";
}

ssbu::ComputePreference ParseBackendPref(const std::string& s) {
    if (s == "gpu" || s == "d3d11" || s == "opencl") return ssbu::ComputePreference::ForceGpu;
    if (s == "cpu") return ssbu::ComputePreference::ForceCpu;
    return ssbu::ComputePreference::GpuFirstAuto;
}

ssbu::OpponentArchetype ParseArchetype(const std::string& s) {
    if (s == "gauntlet") return ssbu::OpponentArchetype::FullRosterGauntlet;
    if (s == "balanced") return ssbu::OpponentArchetype::Balanced;
    if (s == "rushdown" || s == "aggressive") return ssbu::OpponentArchetype::Rushdown;
    if (s == "zoner") return ssbu::OpponentArchetype::Zoner;
    if (s == "parry" || s == "defensive") return ssbu::OpponentArchetype::DefensiveParry;
    if (s == "grappler" || s == "juggler") return ssbu::OpponentArchetype::Grappler;
    if (s == "aerial" || s == "edgeguard") return ssbu::OpponentArchetype::AerialEdgeguarder;
    return ssbu::OpponentArchetype::FullRosterGauntlet;
}

ssbu::TrainingFocus ParseFocus(const std::string& s) {
    if (s == "optimal" || s == "balanced") return ssbu::TrainingFocus::TournamentOptimal;
    if (s == "aggressive" || s == "rushdown") return ssbu::TrainingFocus::AggressiveStageControl;
    if (s == "parry" || s == "defensive") return ssbu::TrainingFocus::ParryAndPunishMaster;
    if (s == "aerial" || s == "edgeguard") return ssbu::TrainingFocus::AerialMeteorEdgeguarder;
    if (s == "showoff" || s == "taunt") return ssbu::TrainingFocus::ShowOffEntertainer;
    if (s == "adaptive") return ssbu::TrainingFocus::AdaptiveCounterplay;
    if (s == "heavy" || s == "armor") return ssbu::TrainingFocus::HeavyArmorSmashBrawler;
    return ssbu::TrainingFocus::TournamentOptimal;
}

uint16_t ResolveFighterArg(const std::string& arg) {
    const auto* by_name = ssbu::FindFighterByName(arg);
    if (by_name) return by_name->char_id;
    return static_cast<uint16_t>(std::strtoul(arg.c_str(), nullptr, 0));
}

void PrintDiagnosticSummary(const ssbu::AmiiboBin& bin) {
    const auto* f = ssbu::FindFighterById(bin.GetCharacterId());
    auto gpu = ssbu::GpuComputeEngine::ProbeGpuDevice();
    std::string switch_diag;
    bin.VerifySwitchCompatibility(&switch_diag);

    std::cout << "=== SSBU Amiibo Diagnostic Summary ===\n"
              << "  Switch nn::nfp  : " << switch_diag << "\n"
              << "  Format Status   : " << bin.GetFormatStatusString() << "\n"
              << "  Owner Mii       : " << (bin.HasOwnerMii() ? "AUTO-PAIRED & VER3 VALID" : "UNREGISTERED")
              << " (Owner: \"" << bin.GetOwnerMiiName() << "\" | Nickname: \"" << bin.GetAmiiboNickname()
              << "\" | CRC-16: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
              << bin.GetOwnerMiiCrc16() << std::dec << std::setfill(' ') << ")\n"
              << "  Compute Probe   : " << gpu.backend_name << " (" << gpu.adapter_name << ")\n"
              << "  Fighter         : " << (f ? f->name : "Unknown")
              << " (0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
              << bin.GetCharacterId() << " | AmiiboID: 0x" << std::setw(16) << bin.GetAmiiboId() << std::dec << std::setfill(' ') << ")\n"
              << "  Level / CPU Lv  : Lv " << bin.GetDisplayedLevel()
              << " (" << bin.GetLevelExperience() << " XP) / CPU Lv " << bin.GetCpuLevel() << "\n"
              << "  Learning        : " << (bin.GetLearningEnabled() ? "ON" : "OFF (Frozen)") << "\n"
              << "  Personality     : " << bin.EvaluatePersonality() << "\n"
              << "  Spirit Stats    : ATK " << bin.GetAttackStat() << " / DEF " << bin.GetDefenseStat()
              << " (" << ssbu::SpiritTypeToString(bin.GetSpiritType()) << ")\n";
    auto slots = bin.GetSpiritSlots();
    for (int i = 0; i < 3; ++i) {
        const auto* ab = ssbu::FindSupportAbilityById(slots[i]);
        std::cout << "  Support Slot " << (i + 1) << "  : [" << static_cast<int>(slots[i]) << "] "
                  << (ab ? ab->name : "None") << " (" << (ab ? static_cast<int>(ab->slot_cost) : 0) << "-Slot)\n";
    }
}

} // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--gui")) {
        FreeConsole();
        return ssbu::RunWin32Gui(GetModuleHandleA(nullptr), SW_SHOWDEFAULT);
    }
#endif

    std::string in_path;
    std::string out_path;
    std::string keys_path;
    std::string nickname_arg;
    std::string mii_name_arg;
    std::string backend_str = "auto";
    std::string fighter_arg = "Mario";
    bool explicit_fighter = false;
    std::string opponent_arg = "Fox";
    int num_games = 500;
    std::string archetype_str = "gauntlet";
    std::string focus_str = "optimal";
    std::string primary_name;
    int custom_atk = -99999;
    int custom_def = -99999;
    std::string spirits_csv;
    bool inspect_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "--gpu-info") {
            auto info = ssbu::GpuComputeEngine::ProbeGpuDevice();
            std::cout << "=== SSBU Amiibo Trainer GPU Compute Probe ===\n"
                      << "  GPU Available   : " << (info.gpu_available ? "YES" : "NO (CPU Fallback Active)") << "\n"
                      << "  Hardware GPU    : " << (info.is_hardware_gpu ? "YES (Dedicated/Integrated)" : "NO") << "\n"
                      << "  Compute Backend : " << info.backend_name << "\n"
                      << "  Adapter Name    : " << info.adapter_name << "\n"
                      << "  Dedicated VRAM  : " << info.dedicated_vram_mb << " MB\n";
            return 0;
        } else if (arg == "--list-fighters") {
            std::cout << "=== Super Smash Bros. Ultimate Complete Fighter Roster ("
                      << ssbu::GetFighterCatalog().size() << " Fighters) ===\n";
            for (const auto& f : ssbu::GetFighterCatalog()) {
                std::cout << "  0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                          << f.char_id << std::dec << " (#" << std::setw(3) << std::setfill(' ') << f.roster_num
                          << ") : " << f.name << "\n";
            }
            return 0;
        } else if (arg == "--list-primaries") {
            std::cout << "=== SSBU Primary Spirits Catalog ("
                      << ssbu::GetPrimarySpiritCatalog().size() << " Presets) ===\n";
            for (const auto& p : ssbu::GetPrimarySpiritCatalog()) {
                std::cout << "  " << std::setw(34) << std::left << p.name
                          << " | Rank: " << std::setw(8) << p.rank
                          << " | ATK: " << std::setw(4) << p.base_attack
                          << " | DEF: " << std::setw(4) << p.base_defense
                          << " | Slots: " << static_cast<int>(p.slots) << "\n";
            }
            return 0;
        } else if (arg == "--list-spirits") {
            std::string q;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                q = argv[++i];
            }
            auto matches = ssbu::GetSelectableSupportSpirits(false, q);
            std::cout << "=== SSBU Selectable Support Spirit Abilities (" << matches.size() << " matches) ===\n";
            for (const auto* sp : matches) {
                std::cout << "  [" << std::setw(3) << static_cast<int>(sp->id) << "] "
                          << std::setw(30) << std::left << sp->name
                          << " (" << static_cast<int>(sp->slot_cost) << "-Slot) : "
                          << sp->description << "\n";
            }
            return 0;
        } else if (arg == "--in" && i + 1 < argc) {
            in_path = argv[++i];
        } else if (arg == "--out" && i + 1 < argc) {
            out_path = argv[++i];
        } else if (arg == "--keys" && i + 1 < argc) {
            keys_path = argv[++i];
        } else if (arg == "--nickname" && i + 1 < argc) {
            nickname_arg = argv[++i];
        } else if (arg == "--mii-name" && i + 1 < argc) {
            mii_name_arg = argv[++i];
        } else if (arg == "--backend" && i + 1 < argc) {
            backend_str = argv[++i];
        } else if (arg == "--fighter" && i + 1 < argc) {
            fighter_arg = argv[++i];
            explicit_fighter = true;
        } else if (arg == "--opponent" && i + 1 < argc) {
            opponent_arg = argv[++i];
        } else if (arg == "--games" && i + 1 < argc) {
            num_games = std::atoi(argv[++i]);
        } else if (arg == "--archetype" && i + 1 < argc) {
            archetype_str = argv[++i];
        } else if (arg == "--focus" && i + 1 < argc) {
            focus_str = argv[++i];
        } else if (arg == "--primary-spirit" && i + 1 < argc) {
            primary_name = argv[++i];
        } else if (arg == "--attack" && i + 1 < argc) {
            custom_atk = std::atoi(argv[++i]);
        } else if (arg == "--defense" && i + 1 < argc) {
            custom_def = std::atoi(argv[++i]);
        } else if (arg == "--spirits" && i + 1 < argc) {
            spirits_csv = argv[++i];
        } else if (arg == "--inspect") {
            inspect_only = true;
        }
    }

    uint16_t char_id = ResolveFighterArg(fighter_arg);
    uint16_t opp_id = ResolveFighterArg(opponent_arg);

    ssbu::AmiiboBin bin;
    bin.InitializeBlankFighter(char_id, 0);
    bin.TryAutoLoadRetailKey();

    if (!keys_path.empty()) {
        std::string kerr;
        if (!bin.LoadRetailKey(keys_path, &kerr)) {
            std::cerr << "Error loading key_retail.bin: " << kerr << "\n";
            return 1;
        }
    }

    if (!in_path.empty()) {
        std::string err;
        if (!bin.LoadFromFile(in_path, &err)) {
            std::cerr << "Error loading input .bin: " << err << "\n";
            return 1;
        }
        if (explicit_fighter && bin.GetCharacterId() != char_id) {
            bin.SetCharacterId(char_id);
        }
    }

    if (!mii_name_arg.empty()) {
        bin.SetOwnerMiiName(mii_name_arg);
    }
    if (!nickname_arg.empty()) {
        bin.SetAmiiboNickname(nickname_arg);
    }

    if (inspect_only) {
        PrintDiagnosticSummary(bin);
        return 0;
    }

    if (!primary_name.empty()) {
        const auto* preset = ssbu::FindPrimarySpiritByName(primary_name);
        if (preset) {
            bin.ApplyPrimarySpiritPreset(*preset);
        }
    }

    if (custom_atk != -99999 || custom_def != -99999) {
        int16_t a = (custom_atk != -99999) ? static_cast<int16_t>(custom_atk) : bin.GetAttackStat();
        int16_t d = (custom_def != -99999) ? static_cast<int16_t>(custom_def) : bin.GetDefenseStat();
        bin.SetStats(a, d, false);
    }

    if (!spirits_csv.empty()) {
        uint8_t s[3] = {0, 0, 0};
        size_t pos = 0;
        for (int idx = 0; idx < 3 && pos < spirits_csv.size(); ++idx) {
            size_t comma = spirits_csv.find(',', pos);
            std::string tok = (comma == std::string::npos) ? spirits_csv.substr(pos) : spirits_csv.substr(pos, comma - pos);
            s[idx] = static_cast<uint8_t>(std::atoi(tok.c_str()) & 0xFF);
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
        std::string serr;
        if (!bin.SetSpiritSlots(s[0], s[1], s[2], true, &serr)) {
            std::cerr << "Warning: " << serr << "\n";
        }
    }

    if (num_games > 0) {
        ssbu::TrainingConfig cfg{};
        cfg.num_games = num_games;
        cfg.opponent_char_id = opp_id;
        cfg.archetype = ParseArchetype(archetype_str);
        cfg.focus = ParseFocus(focus_str);
        cfg.compute_preference = ParseBackendPref(backend_str);
        cfg.freeze_learning_after_training = true;

        ssbu::TrainingSummary sum = ssbu::TrainingSimulator::RunTrainingSession(bin, cfg);
        float win_rate = (sum.total_games > 0)
            ? (100.0f * static_cast<float>(sum.wins) / static_cast<float>(sum.total_games)) : 0.0f;
        std::cout << "=== SSBU 60 FPS Frame-by-Frame Amiibo Training Complete ===\n"
                  << "Compute Backend : " << sum.compute_backend_used << " (" << sum.compute_adapter_name << ")\n"
                  << "Compute Time    : " << std::fixed << std::setprecision(2) << sum.compute_elapsed_ms << " ms\n"
                  << "Games Simulated : " << sum.total_games << " (" << sum.total_frames_simulated << " 60Hz frames)\n"
                  << "Win / Loss      : " << sum.wins << "W - " << sum.losses << "L ("
                  << std::fixed << std::setprecision(1) << win_rate << "%)\n"
                  << "Combat Stats    : " << sum.total_hits_landed << " Hits | "
                  << sum.total_grabs_landed << " Grabs | " << sum.total_parries << " Parries | "
                  << sum.total_meteors << " Meteors | " << sum.total_ledge_recoveries << " Ledge Recs\n"
                  << "Level Progress  : Lv " << sum.initial_level
                  << " -> Lv " << sum.final_level << "\n"
                  << "Final Style     : " << sum.final_personality << "\n\n";
    }

    PrintDiagnosticSummary(bin);

    if (!out_path.empty()) {
        std::string err;
        if (!bin.SaveToFile(out_path, &err)) {
            std::cerr << "Error saving output .bin: " << err << "\n";
            return 1;
        }
        std::cout << "Successfully wrote Switch-verified 540-byte NTAG215 Amiibo .bin to: " << out_path << "\n";
    }

    return 0;
}
