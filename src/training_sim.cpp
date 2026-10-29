#include "training_sim.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

namespace ssbu {

const char* TrainingSimulator::ArchetypeToString(OpponentArchetype a) {
    switch (a) {
        case OpponentArchetype::FullRosterGauntlet: return "Full 89-Fighter Roster Gauntlet";
        case OpponentArchetype::Balanced:           return "Balanced All-Rounder";
        case OpponentArchetype::Rushdown:           return "Rushdown / Frame-Trap";
        case OpponentArchetype::Zoner:              return "Zoner / Projectile Wall";
        case OpponentArchetype::DefensiveParry:     return "Defensive Shield & Parry";
        case OpponentArchetype::Grappler:           return "Grappler / Command Grab";
        case OpponentArchetype::AerialEdgeguarder:  return "Aerial Juggler & Edgeguarder";
        default:                                    return "Full 89-Fighter Roster Gauntlet";
    }
}

const char* TrainingSimulator::FocusToString(TrainingFocus f) {
    switch (f) {
        case TrainingFocus::TournamentOptimal:       return "Tournament Optimal (Grounded + Parry)";
        case TrainingFocus::AggressiveStageControl:  return "Aggressive Stage Control (Reckless)";
        case TrainingFocus::ParryAndPunishMaster:    return "Parry & Punish Master (Technician / Ace)";
        case TrainingFocus::AerialMeteorEdgeguarder: return "Aerial & Meteor Edgeguarder (Unstoppable)";
        case TrainingFocus::ShowOffEntertainer:      return "Show-Off / Taunt Entertainer (Flashy)";
        case TrainingFocus::AdaptiveCounterplay:     return "Adaptive Matchup Counterplay (Dynamic)";
        case TrainingFocus::HeavyArmorSmashBrawler:  return "Heavy Armor Smash Brawler (Unflappable)";
        default:                                     return "Tournament Optimal";
    }
}

TrainingSummary TrainingSimulator::RunTrainingSession(
    AmiiboBin& amiibo,
    const TrainingConfig& config,
    ProgressCallback on_progress) {

    auto t_start = std::chrono::high_resolution_clock::now();

    TrainingSummary summary{};
    summary.total_games = std::max(1, config.num_games);
    summary.initial_level = amiibo.GetDisplayedLevel();
    summary.initial_cpu_level = amiibo.GetCpuLevel();
    summary.initial_personality = amiibo.EvaluatePersonality();

    AmiiboBehaviorParams beh = amiibo.DecodeBehavior();
    const FighterInfo* my_fighter = FindFighterById(amiibo.GetCharacterId());
    const FighterInfo* fixed_opp  = FindFighterById(config.opponent_char_id);
    const auto& roster = GetFighterCatalog();

    float base_atk_bonus = static_cast<float>(amiibo.GetAttackStat()) / 5000.0f;
    float base_def_bonus = static_cast<float>(amiibo.GetDefenseStat()) / 5000.0f;
    float spirit_combat_bonus = 0.0f;
    float spirit_recovery_rate = 0.0f;
    auto slots = amiibo.GetSpiritSlots();
    for (uint8_t sid : slots) {
        if (sid == 0) continue;
        const auto* ab = FindSupportAbilityById(sid);
        if (!ab) continue;
        switch (sid) {
            case 10: spirit_combat_bonus += 0.14f; break; // Super Armor
            case 11: spirit_combat_bonus += 0.10f; break; // Slow Super Armor
            case 20: spirit_recovery_rate += 0.05f; break; // Autoheal
            case 180: spirit_recovery_rate += 0.10f; break; // Great Autoheal
            case 5:  spirit_combat_bonus += 0.07f; break; // Additional Midair Jump
            case 2:  base_atk_bonus += 0.08f; break;      // Hyper Smash Attacks
            case 81: spirit_combat_bonus += 0.06f; break; // Landing Lag Down
            case 84: base_atk_bonus += 0.07f; break;      // Air Attack Up
            case 50: case 51: base_atk_bonus += 0.07f; break; // Physical/Weapon Attack Up
            case 9:  spirit_combat_bonus += 0.06f; break; // Easier Perfect Shield
            case 104: spirit_recovery_rate += 0.06f; break; // KOs Heal Damage
            case 140: base_atk_bonus += 0.10f; base_def_bonus += 0.12f; break; // Armor Knight
            default: spirit_combat_bonus += 0.03f * ab->slot_cost; break;
        }
    }

    // -----------------------------------------------------------------------
    // STEP 1: Attempt GPU Compute Shader Execution FIRST (Before CPU Fallback)
    // -----------------------------------------------------------------------
    std::vector<GpuMatchOutcome> gpu_outcomes;
    bool gpu_used = false;
    GpuDeviceInfo gpu_dev{};
    double gpu_ms = 0.0;

    if (config.compute_preference != ComputePreference::ForceCpu) {
        GpuSimParams gpu_params{};
        gpu_params.num_games = static_cast<uint32_t>(summary.total_games);
        gpu_params.random_seed = config.random_seed;
        gpu_params.opponent_cpu_level = static_cast<uint32_t>(config.opponent_cpu_level);
        gpu_params.is_gauntlet = (config.archetype == OpponentArchetype::FullRosterGauntlet) ? 1u : 0u;
        gpu_params.my_weight = my_fighter ? my_fighter->weight_class : 1.0f;
        gpu_params.opp_weight = fixed_opp ? fixed_opp->weight_class : 1.0f;
        gpu_params.base_atk_bonus = base_atk_bonus;
        gpu_params.base_def_bonus = base_def_bonus;
        gpu_params.spirit_combat_bonus = spirit_combat_bonus;
        gpu_params.spirit_recovery_rate = spirit_recovery_rate;
        gpu_params.neutral_score = 0.25f * (beh.offensive + beh.grounded + beh.just_shield_master + beh.return_to_cliff);
        gpu_params.just_shield_prob = std::max(0.35f, beh.just_shield_master);
        gpu_params.meteor_prob = std::max(0.35f, beh.meteor_master);
        gpu_params.ko_smash_bonus = (beh.fsmash + beh.usmash) / 40.0f;
        gpu_params.return_cliff_bonus = beh.return_to_cliff;

        gpu_used = GpuComputeEngine::TryRunMatchBatchOnGpu(
            gpu_params, gpu_outcomes, &gpu_dev, &gpu_ms, nullptr);
    }

    if (gpu_used && static_cast<int>(gpu_outcomes.size()) == summary.total_games) {
        summary.executed_on_gpu = true;
        summary.compute_backend_used = gpu_dev.backend_name;
        summary.compute_adapter_name = gpu_dev.adapter_name;
        summary.compute_elapsed_ms = gpu_ms;
    } else {
        summary.executed_on_gpu = false;
        summary.compute_backend_used = (config.compute_preference == ComputePreference::ForceCpu)
            ? "CPU Execution (Forced by User)"
            : "CPU Fallback (After GPU Probe)";
        summary.compute_adapter_name = "Host Multi-Core CPU";
    }

    std::mt19937 rng(config.random_seed);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);

    int q1_count = std::max(1, summary.total_games / 4);
    int q1_wins = 0;
    int q4_wins = 0;
    int rolling_wins = 0;
    const int kWindow = 25;
    std::vector<int> recent_outcomes;
    recent_outcomes.reserve(kWindow);

    float xp_f = static_cast<float>(amiibo.GetLevelExperience());
    float cpu_xp_f = static_cast<float>(amiibo.GetCpuExperience());
    const float xp_step_base = std::max(8.0f, (3912.0f - xp_f) / static_cast<float>(std::max(1, summary.total_games * 3 / 4)));
    const float cpu_step_base = std::max(6.0f, (2765.0f - cpu_xp_f) / static_cast<float>(std::max(1, summary.total_games * 3 / 4)));

    double sum_dealt = 0.0;
    double sum_taken = 0.0;

    for (int g = 0; g < summary.total_games; ++g) {
        const FighterInfo* opp = fixed_opp;
        OpponentArchetype arch = config.archetype;
        if (config.archetype == OpponentArchetype::FullRosterGauntlet) {
            opp = &roster[static_cast<size_t>(g) % roster.size()];
            arch = static_cast<OpponentArchetype>((g % 6) + 1);
        }

        int amiibo_stocks = 3;
        int opp_stocks = 3;
        float total_dealt = 0.0f;
        float total_taken = 0.0f;
        int parries = 0;
        int meteors = 0;
        bool won = false;

        if (summary.executed_on_gpu) {
            // Use match combat results computed in parallel on the GPU
            const auto& gm = gpu_outcomes[static_cast<size_t>(g)];
            won = (gm.won != 0u);
            amiibo_stocks = gm.amiibo_stocks;
            opp_stocks = gm.opp_stocks;
            total_dealt = gm.damage_dealt;
            total_taken = gm.damage_taken;
            parries = static_cast<int>(gm.parries);
            meteors = static_cast<int>(gm.meteors);
        } else {
            // CPU Fallback Simulation Path
            float level_factor = static_cast<float>(amiibo.GetDisplayedLevel() - 1) / 49.0f;
            float cpu_factor   = static_cast<float>(amiibo.GetCpuLevel() - 1) / 8.0f;

            float neutral_score = 0.0f;
            switch (arch) {
                case OpponentArchetype::Rushdown:
                    neutral_score = 0.35f * beh.just_shield_master + 0.30f * beh.grounded +
                                    0.20f * beh.feint_counter + 0.15f * (beh.ftilt + beh.dtilt) / 40.0f;
                    break;
                case OpponentArchetype::Zoner:
                    neutral_score = 0.35f * beh.dash + 0.30f * beh.just_shield_master +
                                    0.20f * beh.feint_shooter + 0.15f * beh.near_range;
                    break;
                case OpponentArchetype::DefensiveParry:
                    neutral_score = 0.40f * beh.catcher + 0.30f * beh.feint_master +
                                    0.30f * (1.0f - beh.dash_attacker);
                    break;
                case OpponentArchetype::Grappler:
                    neutral_score = 0.35f * (1.0f - beh.shield_master) + 0.35f * beh.grounded +
                                    0.30f * (beh.fsmash + beh.dtilt) / 40.0f;
                    break;
                case OpponentArchetype::AerialEdgeguarder:
                    neutral_score = 0.35f * beh.air_offensive + 0.35f * beh.return_to_cliff +
                                    0.30f * beh.just_shield_master;
                    break;
                default:
                    neutral_score = 0.25f * beh.offensive + 0.25f * beh.grounded +
                                    0.25f * beh.just_shield_master + 0.25f * beh.return_to_cliff;
                    break;
            }

            float atk_mult = 1.0f + (0.15f + 0.85f * level_factor) * base_atk_bonus;
            float def_mult = 1.0f + (0.15f + 0.85f * level_factor) * base_def_bonus;

            float amiibo_pct = 0.0f;
            float opp_pct = 0.0f;

            float amiibo_power = (0.24f + 0.28f * level_factor + 0.24f * cpu_factor +
                                  0.26f * neutral_score + spirit_combat_bonus * (0.25f + 0.75f * level_factor));
            float opp_power = 0.42f + 0.042f * static_cast<float>(config.opponent_cpu_level);

            for (int exchange = 0; exchange < 55 && amiibo_stocks > 0 && opp_stocks > 0; ++exchange) {
                float win_prob = amiibo_power / (amiibo_power + opp_power);
                if (uni(rng) < win_prob) {
                    float hit = (10.5f + 6.5f * uni(rng)) * atk_mult;
                    if (uni(rng) < beh.just_shield_master * 0.45f) {
                        parries++;
                        hit *= 1.25f;
                    }
                    opp_pct += hit;
                    total_dealt += hit;
                    if (spirit_recovery_rate > 0.0f) {
                        amiibo_pct = std::max(0.0f, amiibo_pct - hit * spirit_recovery_rate);
                    }
                    float ko_threshold = 112.0f * opp->weight_class - 14.0f * (beh.fsmash + beh.usmash) / 40.0f;
                    if (opp_pct >= ko_threshold) {
                        if (uni(rng) < beh.meteor_master * 0.5f) meteors++;
                        opp_stocks--;
                        opp_pct = 0.0f;
                    }
                } else {
                    float hit = (11.5f + 7.0f * uni(rng)) / std::max(0.5f, def_mult);
                    amiibo_pct += hit;
                    total_taken += hit;
                    float my_ko_thresh = 108.0f * my_fighter->weight_class + 12.0f * beh.return_to_cliff;
                    if (amiibo_pct >= my_ko_thresh) {
                        amiibo_stocks--;
                        amiibo_pct = 0.0f;
                    }
                }
            }

            won = (amiibo_stocks > opp_stocks) ||
                  (amiibo_stocks == opp_stocks && amiibo_pct <= opp_pct);
            if (won) {
                if (opp_stocks > 0) opp_stocks = 0;
                if (amiibo_stocks == 0) amiibo_stocks = 1;
            } else {
                if (amiibo_stocks > 0) amiibo_stocks = 0;
                if (opp_stocks == 0) opp_stocks = 1;
            }
        }

        if (won) {
            summary.wins++;
            if (g < q1_count) q1_wins++;
            if (g >= summary.total_games - q1_count) q4_wins++;
        } else {
            summary.losses++;
        }

        sum_dealt += total_dealt;
        sum_taken += total_taken;
        summary.total_parries += parries;
        summary.total_meteors += meteors;

        xp_f = std::min(3912.0f, xp_f + xp_step_base * (won ? 1.15f : 0.90f));
        cpu_xp_f = std::min(2765.0f, cpu_xp_f + cpu_step_base * (won ? 1.15f : 0.90f));
        if (g == summary.total_games - 1 && summary.total_games >= 50) {
            xp_f = 3912.0f;
            cpu_xp_f = 2765.0f;
        }
        amiibo.SetLevelExperience(static_cast<uint16_t>(std::round(xp_f)));
        amiibo.SetCpuExperience(static_cast<uint16_t>(std::round(cpu_xp_f)));

        const float lr = std::clamp(2.5f / static_cast<float>(std::sqrt(g + 10)), 0.015f, 0.12f);
        auto nudge = [lr](float& param, float target) {
            param = std::clamp(param + lr * (target - param), 0.0f, 1.0f);
        };
        auto nudge_pct = [lr](float& param, float target) {
            param = std::clamp(param + lr * (target - param), 0.0f, 100.0f);
        };

        switch (config.focus) {
            case TrainingFocus::TournamentOptimal:
                nudge(beh.near_range,          0.62f);
                nudge(beh.offensive,           0.68f);
                nudge(beh.grounded,            0.78f);
                nudge(beh.attack_out_cliff,    0.32f);
                nudge(beh.dash,                0.52f);
                nudge(beh.return_to_cliff,     0.88f);
                nudge(beh.air_offensive,       0.48f);
                nudge(beh.cliffer,             0.22f);
                nudge(beh.feint_master,        0.58f);
                nudge(beh.feint_counter,       0.55f);
                nudge(beh.feint_shooter,       0.52f);
                nudge(beh.catcher,             0.46f);
                nudge(beh.attack_cancel,       0.35f);
                nudge(beh.smash_holder,        0.38f);
                nudge(beh.dash_attacker,       0.20f);
                nudge(beh.critical_hitter,     0.45f);
                nudge(beh.meteor_master,       0.42f);
                nudge(beh.shield_master,       0.45f);
                nudge(beh.just_shield_master,  0.82f);
                nudge(beh.shield_catch_master, 0.55f);
                nudge(beh.appeal,              0.01f);
                nudge_pct(beh.ftilt,    16.0f);
                nudge_pct(beh.utilt,    12.0f);
                nudge_pct(beh.dtilt,    16.0f);
                nudge_pct(beh.fsmash,   16.0f);
                nudge_pct(beh.usmash,   12.0f);
                nudge_pct(beh.dsmash,   10.0f);
                nudge_pct(beh.nspecial, 4.0f);
                nudge_pct(beh.sspecial, 4.0f);
                nudge_pct(beh.uspecial, 2.0f);
                nudge_pct(beh.dspecial, 2.0f);
                nudge_pct(beh.fair,     26.0f);
                nudge_pct(beh.bair,     24.0f);
                nudge_pct(beh.uair,     18.0f);
                nudge_pct(beh.dair,     14.0f);
                nudge_pct(beh.air_nspecial, 2.0f);
                nudge_pct(beh.air_sspecial, 1.0f);
                nudge_pct(beh.air_uspecial, 1.0f);
                nudge_pct(beh.air_dspecial, 1.0f);
                break;

            case TrainingFocus::AggressiveStageControl:
                nudge(beh.near_range,       0.88f);
                nudge(beh.offensive,        0.92f);
                nudge(beh.dash,             0.68f);
                nudge(beh.dash_attacker,    0.65f);
                nudge(beh.air_offensive,    0.85f);
                nudge(beh.attack_out_cliff, 0.65f);
                nudge(beh.meteor_master,    0.68f);
                nudge(beh.feint_shooter,    0.10f);
                nudge(beh.item_collector,   0.10f);
                nudge(beh.appeal,           0.01f);
                break;

            case TrainingFocus::ParryAndPunishMaster:
                nudge(beh.just_shield_master,  0.94f);
                nudge(beh.feint_master,        0.86f);
                nudge(beh.feint_counter,       0.86f);
                nudge(beh.feint_shooter,       0.84f);
                nudge(beh.catcher,             0.84f);
                nudge(beh.attack_cancel,       0.80f);
                nudge(beh.shield_catch_master, 0.78f);
                nudge(beh.shield_master,       0.82f);
                nudge(beh.meteor_master,       0.80f);
                nudge(beh.attack_out_cliff,    0.80f);
                nudge(beh.air_offensive,       0.15f);
                nudge(beh.item_collector,      0.80f);
                nudge(beh.appeal,              0.01f);
                break;

            case TrainingFocus::AerialMeteorEdgeguarder:
                nudge(beh.attack_out_cliff, 0.94f);
                nudge(beh.meteor_master,    0.94f);
                nudge(beh.air_offensive,    0.90f);
                nudge(beh.near_range,       0.78f);
                nudge(beh.offensive,        0.86f);
                nudge(beh.grounded,         0.18f);
                nudge(beh.cliffer,          0.82f);
                nudge(beh.dash_attacker,    0.65f);
                nudge(beh.critical_hitter,  0.65f);
                nudge_pct(beh.dair,         32.0f);
                nudge_pct(beh.fair,         28.0f);
                nudge_pct(beh.bair,         22.0f);
                break;

            case TrainingFocus::ShowOffEntertainer:
                nudge(beh.appeal,              0.94f);
                nudge(beh.attack_out_cliff,    0.84f);
                nudge(beh.meteor_master,       0.84f);
                nudge(beh.just_shield_master,  0.84f);
                nudge(beh.dragoon_collector,   0.80f);
                nudge(beh.smashball_collector, 0.80f);
                nudge(beh.hammer_collector,    0.80f);
                nudge(beh.special_flagger,     0.85f);
                nudge(beh.homerun_batter,      0.85f);
                nudge(beh.club_swinger,        0.80f);
                nudge(beh.death_swinger,       0.80f);
                nudge(beh.item_throw_to_target,0.10f);
                break;

            case TrainingFocus::AdaptiveCounterplay:
                nudge(beh.offensive,           0.84f);
                nudge(beh.air_offensive,       0.84f);
                nudge(beh.smash_holder,        0.84f);
                nudge(beh.dash_attacker,       0.84f);
                nudge(beh.critical_hitter,     0.88f);
                nudge(beh.meteor_master,       0.82f);
                nudge(beh.shield_master,       0.82f);
                nudge(beh.just_shield_master,  0.15f);
                nudge(beh.charger,             0.84f);
                nudge(beh.homerun_batter,      0.80f);
                nudge(beh.club_swinger,        0.80f);
                nudge(beh.death_swinger,       0.80f);
                break;

            case TrainingFocus::HeavyArmorSmashBrawler:
                nudge(beh.offensive,           0.15f);
                nudge(beh.grounded,            0.92f);
                nudge(beh.attack_out_cliff,    0.15f);
                nudge(beh.dash,                0.15f);
                nudge(beh.cliffer,             0.15f);
                nudge(beh.shield_master,       0.92f);
                nudge(beh.shield_catch_master, 0.80f);
                nudge(beh.smash_holder,        0.75f);
                nudge_pct(beh.fsmash,          28.0f);
                nudge_pct(beh.usmash,          20.0f);
                nudge_pct(beh.dsmash,          18.0f);
                break;
        }

        beh.NormalizeMoveSums();

        if (summary.sample_matches.size() < 12 || g >= summary.total_games - 5) {
            summary.sample_matches.push_back({
                g + 1,
                opp->name,
                won,
                amiibo_stocks,
                opp_stocks,
                total_dealt,
                total_taken,
                parries,
                meteors
            });
        }

        if (static_cast<int>(recent_outcomes.size()) == kWindow) {
            rolling_wins -= recent_outcomes.front();
            recent_outcomes.erase(recent_outcomes.begin());
        }
        recent_outcomes.push_back(won ? 1 : 0);
        rolling_wins += (won ? 1 : 0);

        if (on_progress && ((g + 1) % 10 == 0 || g == summary.total_games - 1)) {
            float r_wr = static_cast<float>(rolling_wins) / static_cast<float>(recent_outcomes.size());
            on_progress(g + 1, summary.total_games, r_wr);
        }
    }

    if (config.freeze_learning_after_training) {
        amiibo.SetLearningEnabled(false);
    }
    amiibo.SetGiftCount(static_cast<uint16_t>(std::min<int>(65535, amiibo.GetGiftCount() + summary.wins)));
    amiibo.EncodeBehavior(beh);

    if (!summary.executed_on_gpu) {
        auto t_end = std::chrono::high_resolution_clock::now();
        summary.compute_elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    }

    summary.first_quarter_win_rate = static_cast<float>(q1_wins) / static_cast<float>(q1_count);
    summary.final_quarter_win_rate = static_cast<float>(q4_wins) / static_cast<float>(q1_count);
    summary.avg_damage_dealt = static_cast<float>(sum_dealt / summary.total_games);
    summary.avg_damage_taken = static_cast<float>(sum_taken / summary.total_games);
    summary.final_level = amiibo.GetDisplayedLevel();
    summary.final_cpu_level = amiibo.GetCpuLevel();
    summary.final_personality = amiibo.EvaluatePersonality();
    summary.final_behavior = amiibo.DecodeBehavior();

    return summary;
}

} // namespace ssbu
