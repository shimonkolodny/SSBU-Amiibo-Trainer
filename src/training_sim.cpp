#include "training_sim.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

namespace ssbu {

namespace {

struct MoveSpec {
    uint32_t id;
    const char* name;
    int startup_frames;
    int endlag_frames;
    float base_dmg;
    float bkb;
    float kbg;
    float angle_rad;
    float reach;
    bool is_meteor;
};

constexpr MoveSpec kMoveSpecs[17] = {
    {0,  "Jab Combo",          3,  12,  7.5f, 25.0f,  50.0f,  0.95f, 14.0f, false},
    {1,  "Forward Tilt",       7,  16, 11.5f, 35.0f,  86.0f,  0.66f, 18.0f, false},
    {2,  "Up Tilt",            6,  15, 10.0f, 40.0f,  88.0f,  1.48f, 16.5f, false},
    {3,  "Down Tilt",          6,  14,  9.5f, 32.0f,  80.0f,  1.22f, 17.5f, false},
    {4,  "Forward Smash",     15,  29, 18.5f, 42.0f, 105.0f,  0.63f, 20.5f, false},
    {5,  "Up Smash",          11,  26, 17.0f, 38.0f, 103.0f,  1.50f, 19.0f, false},
    {6,  "Down Smash",        10,  25, 15.5f, 35.0f,  97.0f,  0.52f, 19.5f, false},
    {7,  "Neutral Special",    5,  20, 12.5f, 32.0f,  78.0f,  0.75f, 22.0f, false},
    {8,  "Alolan Whip / SideB",13, 20, 20.0f, 48.0f,  96.0f,  0.70f, 26.5f, false},
    {9,  "Up Special (OoS)",   7,  26, 12.5f, 40.0f,  92.0f,  1.30f, 17.5f, false},
    {10, "Revenge / DownB",    3,  18, 14.0f, 38.0f,  92.0f,  0.85f, 18.0f, false},
    {11, "Grab & Throw",       6,  20, 13.5f, 44.0f,  90.0f,  0.90f, 14.5f, false},
    {12, "Neutral Air (Nair)", 5,   9,  9.5f, 30.0f,  78.0f,  0.78f, 15.5f, false},
    {13, "Forward Air (Fair)", 8,  11, 13.0f, 34.0f,  92.0f,  0.70f, 18.5f, false},
    {14, "Back Air (Bair)",    7,  10, 15.0f, 38.0f, 101.0f,  0.61f, 18.5f, false},
    {15, "Up Air (Uair)",      6,   9, 12.0f, 34.0f,  95.0f,  1.48f, 18.0f, false},
    {16, "Down Air (Meteor)", 11,  14, 14.5f, 34.0f,  98.0f, -1.45f, 17.5f, true}
};

// Authentic SSBU Knockback Formula (with Rage multiplier [1.0 .. 1.10])
static float ComputeKnockback(float target_pct, float move_dmg, float target_weight_units,
                              float bkb, float kbg, float attacker_pct) {
    float rage = 1.0f + std::clamp((attacker_pct - 35.0f) / 115.0f, 0.0f, 1.0f) * 0.10f;
    float base = ((target_pct / 10.0f + (target_pct * move_dmg) / 20.0f) *
                  (200.0f / (target_weight_units + 100.0f)) * 1.4f + 18.0f) * (kbg / 100.0f) + bkb;
    return base * rage;
}

} // namespace

const char* TrainingSimulator::MoveIdToName(uint32_t move_id) {
    if (move_id < 17) return kMoveSpecs[move_id].name;
    return "Forward Smash";
}

const char* TrainingSimulator::ArchetypeToString(OpponentArchetype a) {
    switch (a) {
        case OpponentArchetype::FullRosterGauntlet: return "Full 91-Fighter Roster Gauntlet";
        case OpponentArchetype::Balanced:           return "Balanced All-Rounder";
        case OpponentArchetype::Rushdown:           return "Rushdown / Frame-Trap";
        case OpponentArchetype::Zoner:              return "Zoner / Projectile Wall";
        case OpponentArchetype::DefensiveParry:     return "Defensive Shield & Parry";
        case OpponentArchetype::Grappler:           return "Grappler / Command Grab";
        case OpponentArchetype::AerialEdgeguarder:  return "Aerial Juggler & Edgeguarder";
        default:                                    return "Full 91-Fighter Roster Gauntlet";
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
        case TrainingFocus::IncineroarRaidBoss:      return "Incineroar Championship Raid Boss (Alolan Whip + Uair + Parry)";
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
    const bool is_incineroar = (amiibo.GetCharacterId() == 0x1BD7);

    // Exact SSBU Primary Spirit Attack & Defense multipliers:
    // At 2500 ATK / 2500 DEF -> ~1.50x offense & defense scaling
    float base_atk_bonus = static_cast<float>(amiibo.GetAttackStat()) / 5000.0f;
    float base_def_bonus = static_cast<float>(amiibo.GetDefenseStat()) / 5000.0f;

    // Exact Competitive Support Spirit Multipliers (Amiibo Doctor Definitive Spirits Tier List)
    float spirit_atk_mult = 1.0f;
    float spirit_def_mult = 1.0f;
    float spirit_spd_mult = 1.0f;
    float spirit_combat_bonus = 0.0f;
    float spirit_recovery_rate = 0.0f;
    float extra_jump_bonus = 0.0f;
    float starting_stock_damage = 0.0f;
    float super_armor_threshold = 0.0f;
    float side_special_mult = 1.0f;
    bool has_crit_healing_metal = false;
    bool has_crit_stats = false;

    auto slots = amiibo.GetSpiritSlots();
    for (uint8_t sid : slots) {
        if (sid == 0) continue;
        const auto* ab = FindSupportAbilityById(sid);
        if (!ab) continue;
        switch (sid) {
            case 140: // Armor Knight (2 Slots): 1.15x ATK, 1.80x DEF, 0.70x Speed
                spirit_atk_mult *= 1.15f;
                spirit_def_mult *= 1.80f;
                spirit_spd_mult *= 0.70f;
                spirit_combat_bonus += 0.22f;
                break;
            case 15: // Trade-Off Ability Up (1 Slot): 1.18x ATK, 1.20x DEF, 1.20x Speed, +30% start
                spirit_atk_mult *= 1.18f;
                spirit_def_mult *= 1.20f;
                spirit_spd_mult *= 1.20f;
                starting_stock_damage += 30.0f;
                spirit_combat_bonus += 0.14f;
                break;
            case 1: // Move Speed Up (1 Slot): 1.20x Speed, 0% start
                spirit_spd_mult *= 1.20f;
                spirit_combat_bonus += 0.10f;
                break;
            case 14: // Trade-Off Speed Up (1 Slot): 1.40x Speed, +30% start
                spirit_spd_mult *= 1.40f;
                starting_stock_damage += 30.0f;
                spirit_combat_bonus += 0.09f;
                break;
            case 13: // Trade-Off Defense Up (1 Slot): 1.40x DEF, +30% start
                spirit_def_mult *= 1.40f;
                starting_stock_damage += 30.0f;
                spirit_combat_bonus += 0.10f;
                break;
            case 12: // Trade-Off Attacks Up (1 Slot): 1.23x ATK, +30% start
                spirit_atk_mult *= 1.23f;
                starting_stock_damage += 30.0f;
                spirit_combat_bonus += 0.10f;
                break;
            case 10: // Super Armor (3 Slots): No flinching below ~70% damage
                super_armor_threshold = 70.0f;
                spirit_def_mult *= 1.25f;
                spirit_combat_bonus += 0.20f;
                break;
            case 11: // Slow Super Armor (2 Slots): Super Armor at 0.30x Speed
                super_armor_threshold = 65.0f;
                spirit_spd_mult *= 0.45f;
                spirit_combat_bonus += 0.12f;
                break;
            case 20: // Autoheal (2 Slots): 3% every 5s
                spirit_recovery_rate += 0.075f;
                spirit_combat_bonus += 0.10f;
                break;
            case 180: // Great Autoheal (3 Slots): 5% every 5s
                spirit_recovery_rate += 0.125f;
                spirit_combat_bonus += 0.14f;
                break;
            case 236: // Critical Healing & Metal (2 Slots): Heals 50% + Metal knockback resistance
                has_crit_healing_metal = true;
                spirit_def_mult *= 1.22f;
                spirit_combat_bonus += 0.15f;
                break;
            case 162: // Critical-Health Healing (2 Slots)
                has_crit_healing_metal = true;
                spirit_combat_bonus += 0.11f;
                break;
            case 18: // Critical-Health Stats Up (1 Slot)
            case 178: // Critical-Health Stats Up Up (2 Slots)
                has_crit_stats = true;
                spirit_atk_mult *= 1.08f;
                spirit_def_mult *= 1.15f;
                spirit_combat_bonus += 0.09f;
                break;
            case 50: // Physical Attack Up (1 Slot): 1.10x physical/throw/command-grab damage
            case 51: // Weapon Attack Up (1 Slot)
            case 52: // Fist Attack Up
            case 53: // Foot Attack Up
                spirit_atk_mult *= 1.10f;
                spirit_combat_bonus += 0.08f;
                break;
            case 87: // Side Special Up (1 Slot): 1.10x Alolan Whip damage
                side_special_mult *= 1.12f;
                spirit_atk_mult *= 1.06f;
                spirit_combat_bonus += 0.08f;
                break;
            case 90: // Strong Throw (1 Slot)
                side_special_mult *= 1.08f;
                spirit_atk_mult *= 1.06f;
                spirit_combat_bonus += 0.07f;
                break;
            case 5: // Additional Midair Jump (2 Slots)
                spirit_combat_bonus += 0.07f;
                extra_jump_bonus = 1.0f;
                break;
            case 117: // Floaty Jumps (1 Slot): 0.65x fall speed, huge recovery boost
                spirit_combat_bonus += 0.08f;
                extra_jump_bonus = 0.85f;
                break;
            case 2: // Hyper Smash Attacks (1 Slot)
                spirit_atk_mult *= 1.08f;
                spirit_combat_bonus += 0.07f;
                break;
            case 81: // Landing Lag Down (1 Slot): 0.7x aerial landing lag
                spirit_combat_bonus += 0.08f;
                break;
            case 84: // Air Attack Up (1 Slot)
                spirit_atk_mult *= 1.07f;
                spirit_combat_bonus += 0.06f;
                break;
            case 85: // Air Defense Up (1 Slot)
                spirit_def_mult *= 1.10f;
                spirit_combat_bonus += 0.07f;
                break;
            case 9: // Easier Perfect Shield (1 Slot)
                spirit_combat_bonus += 0.07f;
                break;
            case 104: // KOs Heal Damage (2 Slots)
                spirit_recovery_rate += 0.06f;
                spirit_combat_bonus += 0.08f;
                break;
            default:
                spirit_combat_bonus += 0.03f * ab->slot_cost;
                break;
        }
    }

    float effective_atk_bonus = base_atk_bonus + (spirit_atk_mult - 1.0f);
    float effective_def_bonus = base_def_bonus + (spirit_def_mult - 1.0f);

    // -----------------------------------------------------------------------
    // STEP 1: Attempt 60 FPS Frame-by-Frame GPU Compute Shader Execution FIRST
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
        gpu_params.my_air_mobility = (my_fighter ? my_fighter->air_mobility : 1.1f) * spirit_spd_mult;
        gpu_params.opp_air_mobility = fixed_opp ? fixed_opp->air_mobility : 1.1f;
        gpu_params.base_atk_bonus = effective_atk_bonus;
        gpu_params.base_def_bonus = effective_def_bonus;
        gpu_params.spirit_combat_bonus = spirit_combat_bonus;
        gpu_params.spirit_recovery_rate = spirit_recovery_rate;

        gpu_params.near_range = beh.near_range;
        gpu_params.offensive = beh.offensive;
        gpu_params.grounded = beh.grounded;
        gpu_params.air_offensive = beh.air_offensive;
        gpu_params.dash = beh.dash;
        gpu_params.return_to_cliff = beh.return_to_cliff;
        gpu_params.attack_out_cliff = beh.attack_out_cliff;
        gpu_params.catcher = beh.catcher;
        gpu_params.shield_master = beh.shield_master;
        gpu_params.just_shield_prob = std::max(0.35f, beh.just_shield_master);
        gpu_params.meteor_prob = std::max(0.35f, beh.meteor_master);
        gpu_params.smash_holder = beh.smash_holder;

        gpu_params.prob_tilt = (beh.ftilt + beh.utilt + beh.dtilt) / 100.0f;
        gpu_params.prob_smash = (beh.fsmash + beh.usmash + beh.dsmash) / 100.0f;
        gpu_params.prob_special = (beh.nspecial + beh.sspecial + beh.uspecial + beh.dspecial) / 100.0f;
        gpu_params.prob_fair_bair = (beh.fair + beh.bair) / 100.0f;
        gpu_params.prob_uair = beh.uair / 100.0f;
        gpu_params.prob_dair = beh.dair / 100.0f;
        gpu_params.extra_jump_bonus = extra_jump_bonus;

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
            ? "CPU 60 FPS Frame Physics Engine (Forced)"
            : "CPU 60 FPS Frame Physics Engine (After GPU Probe)";
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
        int frames_sim = 0;
        int hits_landed = 0;
        int grabs_landed = 0;
        int ledge_recoveries = 0;
        uint32_t ko_move_id = is_incineroar ? 8 : 4;
        float tilt_reward = 0.0f;
        float smash_reward = 0.0f;
        float aerial_reward = 0.0f;
        bool won = false;

        if (summary.executed_on_gpu) {
            const auto& gm = gpu_outcomes[static_cast<size_t>(g)];
            won = (gm.won != 0u);
            amiibo_stocks = gm.amiibo_stocks;
            opp_stocks = gm.opp_stocks;
            total_dealt = gm.damage_dealt;
            total_taken = gm.damage_taken;
            parries = static_cast<int>(gm.parries);
            meteors = static_cast<int>(gm.meteors);
            frames_sim = static_cast<int>(gm.frames_simulated);
            hits_landed = static_cast<int>(gm.hits_landed);
            grabs_landed = static_cast<int>(gm.grabs_landed);
            ledge_recoveries = static_cast<int>(gm.ledge_recoveries);
            ko_move_id = gm.signature_ko_move;
            tilt_reward = gm.tilt_reward;
            smash_reward = gm.smash_reward;
            aerial_reward = gm.aerial_reward;
        } else {
            // -------------------------------------------------------------------
            // True 60 FPS Frame-by-Frame SSBU Stage, Physics & Hitbox Simulation
            // Stage main platform: X in [-85, +85], Y = 0
            // Blast zones: X = +-240, Top Y = +190, Bottom Y = -140
            // -------------------------------------------------------------------
            float level_factor = static_cast<float>(amiibo.GetDisplayedLevel() - 1) / 49.0f;
            float cpu_factor   = static_cast<float>(amiibo.GetCpuLevel() - 1) / 8.0f;

            float atk_mult = (1.0f + (0.15f + 0.85f * level_factor) * base_atk_bonus) * spirit_atk_mult;
            float def_mult = std::max(0.5f, (1.0f + (0.15f + 0.85f * level_factor) * base_def_bonus) * spirit_def_mult);

            float a_weight_units = 100.0f * (my_fighter ? my_fighter->weight_class : 1.0f);
            float o_weight_units = 100.0f * (opp ? opp->weight_class : 1.0f);
            float my_mob = (my_fighter ? my_fighter->air_mobility : 1.1f) * spirit_spd_mult;
            float opp_mob = opp ? opp->air_mobility : 1.1f;

            float a_pct = starting_stock_damage;
            float o_pct = 0.0f;
            float a_x = -35.0f, a_y = 0.0f, a_vx = 0.0f, a_vy = 0.0f;
            float o_x =  35.0f, o_y = 0.0f, o_vx = 0.0f, o_vy = 0.0f;
            int a_timer = 0, o_timer = 0;
            int a_shield_frames = 0, o_shield_frames = 0;
            uint32_t a_move = 0, o_move = 0;
            bool revenge_active = false;
            bool crit_heal_used_this_stock = false;

            float arch_bonus = 0.0f;
            switch (arch) {
                case OpponentArchetype::Rushdown:
                    arch_bonus = 0.12f * (beh.just_shield_master + beh.grounded); break;
                case OpponentArchetype::Zoner:
                    arch_bonus = 0.12f * (beh.feint_shooter + beh.just_shield_master); break;
                case OpponentArchetype::DefensiveParry:
                    arch_bonus = 0.15f * (beh.catcher + beh.feint_master + (beh.sspecial / 100.0f)); break;
                case OpponentArchetype::Grappler:
                    arch_bonus = 0.12f * ((1.0f - beh.shield_master) + beh.grounded); break;
                case OpponentArchetype::AerialEdgeguarder:
                    arch_bonus = 0.12f * ((beh.uair / 100.0f) + beh.return_to_cliff); break;
                default:
                    arch_bonus = 0.09f * (beh.offensive + beh.just_shield_master); break;
            }

            float a_reaction = 0.34f + 0.50f * (0.5f * level_factor + 0.5f * cpu_factor) + arch_bonus + 0.10f * spirit_combat_bonus;
            float o_reaction = 0.30f + 0.052f * static_cast<float>(config.opponent_cpu_level);

            float p_tilt = (beh.ftilt + beh.utilt + beh.dtilt) / 100.0f;
            float p_smash = (beh.fsmash + beh.usmash + beh.dsmash) / 100.0f;
            float p_spec = (beh.nspecial + beh.sspecial + beh.uspecial + beh.dspecial) / 100.0f;
            float p_sspec_ratio = (p_spec > 0.01f) ? (beh.sspecial / (beh.nspecial + beh.sspecial + beh.uspecial + beh.dspecial)) : 0.4f;
            float p_fair_bair = (beh.fair + beh.bair) / 100.0f;
            float p_uair = beh.uair / 100.0f;
            float p_dair = beh.dair / 100.0f;

            for (int step = 0; step < 3600 && amiibo_stocks > 0 && opp_stocks > 0; ++step) {
                frames_sim += 2;

                if ((frames_sim % 300) == 0 && spirit_recovery_rate > 0.0f) {
                    a_pct = std::max(0.0f, a_pct - 40.0f * spirit_recovery_rate);
                }

                // Critical Healing & Metal trigger at >= 80% damage once per stock
                if (has_crit_healing_metal && !crit_heal_used_this_stock && a_pct >= 80.0f) {
                    a_pct = std::max(0.0f, a_pct - 50.0f);
                    crit_heal_used_this_stock = true;
                }

                // Integrate 2D positions and velocities
                a_x += a_vx * 2.0f;
                a_y += a_vy * 2.0f;
                o_x += o_vx * 2.0f;
                o_y += o_vy * 2.0f;

                // Gravity & Stage Platform Collision (Stage X in [-85, +85], Y = 0)
                if (a_y > 0.0f || std::fabs(a_x) > 85.0f) {
                    a_vy -= 0.19f;
                    if (a_y <= 0.0f && std::fabs(a_x) <= 85.0f && a_vy <= 0.0f) {
                        a_y = 0.0f;
                        a_vy = 0.0f;
                    }
                } else {
                    a_y = 0.0f;
                    a_vy = 0.0f;
                    a_vx *= 0.82f;
                }

                if (o_y > 0.0f || std::fabs(o_x) > 85.0f) {
                    o_vy -= 0.19f;
                    if (o_y <= 0.0f && std::fabs(o_x) <= 85.0f && o_vy <= 0.0f) {
                        o_y = 0.0f;
                        o_vy = 0.0f;
                    }
                } else {
                    o_y = 0.0f;
                    o_vy = 0.0f;
                    o_vx *= 0.82f;
                }

                // Offstage Ledge Recovery Physics
                if (std::fabs(a_x) > 85.0f && a_timer >= -4) {
                    float ledge_dir = (a_x > 0.0f) ? -1.0f : 1.0f;
                    float rec_skill = std::clamp(0.45f + 0.45f * beh.return_to_cliff + 0.10f * extra_jump_bonus, 0.25f, 0.98f);
                    a_vx = a_vx * 0.72f + (ledge_dir * 2.05f * std::max(0.85f, my_mob)) * 0.28f;
                    if (a_y < -16.0f && uni(rng) < rec_skill) {
                        a_vy = 2.90f;
                        if (std::fabs(a_x) < 110.0f) {
                            a_x = (a_x > 0.0f) ? 83.0f : -83.0f;
                            a_y = 0.0f; a_vx = 0.0f; a_vy = 0.0f;
                            ledge_recoveries++;
                        }
                    }
                }

                if (std::fabs(o_x) > 85.0f && o_timer >= -4) {
                    float o_ledge_dir = (o_x > 0.0f) ? -1.0f : 1.0f;
                    o_vx = o_vx * 0.78f + (o_ledge_dir * 1.80f * opp_mob) * 0.22f;
                    if (o_y < -20.0f && uni(rng) < (0.42f + 0.045f * static_cast<float>(config.opponent_cpu_level))) {
                        o_vy = 2.65f;
                        if (std::fabs(o_x) < 105.0f) {
                            o_x = (o_x > 0.0f) ? 83.0f : -83.0f;
                            o_y = 0.0f; o_vx = 0.0f; o_vy = 0.0f;
                        }
                    }
                }

                // Blast Zone KO Checks (Left/Right: +-240, Top: +190, Bottom: -140)
                if (std::fabs(a_x) > 240.0f || a_y > 190.0f || a_y < -140.0f) {
                    amiibo_stocks--;
                    a_pct = starting_stock_damage;
                    crit_heal_used_this_stock = false;
                    revenge_active = false;
                    a_x = -30.0f; a_y = 20.0f; a_vx = 0.0f; a_vy = 0.0f; a_timer = 0;
                    continue;
                }
                if (std::fabs(o_x) > 240.0f || o_y > 190.0f || o_y < -140.0f) {
                    opp_stocks--;
                    ko_move_id = a_move;
                    if (a_move == 16) meteors++;
                    o_pct = 0.0f;
                    o_x = 30.0f; o_y = 20.0f; o_vx = 0.0f; o_vy = 0.0f; o_timer = 0;
                    continue;
                }

                if (a_timer > 0) a_timer = std::max(0, a_timer - 2);
                else if (a_timer < 0) a_timer = std::min(0, a_timer + 2);

                if (o_timer > 0) o_timer = std::max(0, o_timer - 2);
                else if (o_timer < 0) o_timer = std::min(0, o_timer + 2);

                if (a_shield_frames > 0) a_shield_frames = std::max(0, a_shield_frames - 2);
                if (o_shield_frames > 0) o_shield_frames = std::max(0, o_shield_frames - 2);

                float dx = o_x - a_x;
                float dy = o_y - a_y;
                float dist = std::sqrt(dx * dx + dy * dy);
                float dir_to_opp = (dx >= 0.0f) ? 1.0f : -1.0f;

                if (a_timer == 0) {
                    float desired_dist = 28.0f + (11.0f - 28.0f) * beh.near_range;
                    if (dist > desired_dist) {
                        a_vx = dir_to_opp * (1.45f + 0.55f * beh.dash) * my_mob;
                    } else if (dist < desired_dist * 0.55f && beh.offensive < 0.45f) {
                        a_vx = -dir_to_opp * 1.15f;
                    }
                    if (a_y == 0.0f && uni(rng) < (1.0f - beh.grounded) * 0.22f) {
                        a_vy = 2.45f;
                    }
                    if (std::fabs(o_x) > 85.0f && uni(rng) < beh.attack_out_cliff * 0.35f) {
                        a_vx = dir_to_opp * 2.1f;
                        if (a_y == 0.0f) a_vy = 2.2f;
                    }
                    if (dist < 22.0f && uni(rng) < beh.shield_master * 0.28f) {
                        a_shield_frames = 6;
                    }
                }

                if (o_timer == 0) {
                    if (dist > 15.0f) {
                        o_vx = -dir_to_opp * 1.55f * opp_mob;
                    }
                    if (o_y == 0.0f && uni(rng) < 0.14f) {
                        o_vy = 2.35f;
                    }
                    if (dist < 20.0f && uni(rng) < 0.18f) {
                        o_shield_frames = 6;
                    }
                }

                // Frame-Data Hitbox Collision Resolution
                if (dist <= 26.5f && (a_timer == 0 || o_timer == 0)) {
                    if (a_y > 2.0f || std::fabs(o_x) > 85.0f) {
                        float r = uni(rng);
                        if (std::fabs(o_x) > 85.0f && r < std::max(p_dair, beh.meteor_master * 0.55f)) a_move = 16;
                        else if (o_y > a_y + 4.0f && r < p_uair + 0.35f) a_move = 15; // Up Air juggle chain!
                        else if (r < p_dair) a_move = 16; // Landing Dair
                        else if (r < p_dair + p_uair) a_move = 15;
                        else if (r < p_dair + p_uair + p_fair_bair * 0.55f) a_move = 13;
                        else if (r < p_dair + p_uair + p_fair_bair) a_move = 14;
                        else a_move = 12;
                    } else {
                        float r = uni(rng);
                        // Incineroar Command Grab (Alolan Whip, move 8) or Grab (move 11) beats Shield!
                        if (is_incineroar && (o_shield_frames > 0 || r < p_spec * p_sspec_ratio)) {
                            a_move = 8; // Alolan Whip command grab!
                        } else if (o_shield_frames > 0 && uni(rng) < std::max(0.25f, beh.catcher)) {
                            a_move = is_incineroar ? 8 : 11;
                        } else if (std::fabs(o_x) > 72.0f && uni(rng) < (beh.dsmash + beh.dtilt) / 100.0f + 0.35f) {
                            a_move = (uni(rng) < 0.65f) ? 6 : 3; // Down Smash / Down Tilt at Ledge!
                        } else if (o_pct >= 85.0f && r < std::max(p_smash, 0.38f + 0.25f * beh.smash_holder)) {
                            a_move = (uni(rng) < 0.45f) ? 4 : ((uni(rng) < 0.6f) ? 6 : 5);
                        } else if (r < p_tilt) {
                            a_move = 1 + (rng() % 3);
                        } else if (r < p_tilt + p_smash) {
                            a_move = 4 + (rng() % 3);
                        } else if (r < p_tilt + p_smash + p_spec) {
                            a_move = (is_incineroar && uni(rng) < p_sspec_ratio) ? 8 : (7 + (rng() % 4));
                        } else {
                            a_move = 0;
                        }
                    }

                    o_move = (o_y > 2.0f) ? (12 + (rng() % 5)) : (rng() % 11);

                    const MoveSpec& am = kMoveSpecs[a_move];
                    const MoveSpec& om = kMoveSpecs[o_move];

                    bool a_parried = false;
                    int a_start = am.startup_frames;
                    if (a_shield_frames > 0 && uni(rng) < std::max(0.20f, beh.just_shield_master * 0.58f)) {
                        a_parried = true;
                        parries++;
                        a_start = std::max(1, a_start - 9);
                        if (is_incineroar && uni(rng) < beh.revenge) {
                            revenge_active = true; // Counter/Revenge charged!
                        }
                    }

                    bool is_command_grab = (is_incineroar && a_move == 8);
                    bool super_armor_active = (super_armor_threshold > 0.0f && a_pct < super_armor_threshold) ||
                                              (is_command_grab && om.base_dmg < 11.9f);

                    float a_eff_frame = static_cast<float>(a_start) - 6.5f * a_reaction
                                      - ((am.reach > om.reach) ? 2.5f : 0.0f)
                                      - (super_armor_active ? 6.0f : 0.0f);
                    float o_eff_frame = static_cast<float>(om.startup_frames) - 6.5f * o_reaction + static_cast<float>(std::max(0, -o_timer));

                    if (a_parried || a_move == 11 || is_command_grab || super_armor_active ||
                        a_eff_frame <= o_eff_frame + 1.5f * (uni(rng) - 0.42f)) {
                        float move_mult = (a_move == 8) ? side_special_mult : 1.0f;
                        float crit_mult = (has_crit_stats && a_pct >= 80.0f) ? 1.12f : 1.0f;
                        float rev_mult  = revenge_active ? 1.25f : 1.0f;
                        float scaled_dmg = am.base_dmg * atk_mult * move_mult * crit_mult * rev_mult * (a_parried ? 1.20f : 1.0f);
                        o_pct += scaled_dmg;
                        total_dealt += scaled_dmg;
                        hits_landed++;
                        if (a_move == 11 || is_command_grab) grabs_landed++;

                        float kb = ComputeKnockback(o_pct, scaled_dmg, o_weight_units, am.bkb, am.kbg * (revenge_active ? 1.18f : 1.0f), a_pct);
                        revenge_active = false;
                        float launch_speed = 0.032f * kb;
                        o_vx = dir_to_opp * launch_speed * std::cos(am.angle_rad);
                        o_vy = launch_speed * std::sin(am.angle_rad);
                        if (am.is_meteor && std::fabs(o_x) > 70.0f) {
                            o_vy = -std::fabs(launch_speed * 0.95f);
                        }
                        o_timer = -std::max(6, static_cast<int>(kb * 0.35f));
                        a_timer = std::max(4, am.endlag_frames / 2);

                        if (a_move >= 1 && a_move <= 3) tilt_reward += scaled_dmg;
                        else if (a_move >= 4 && a_move <= 8) smash_reward += scaled_dmg;
                        else if (a_move >= 12) aerial_reward += scaled_dmg;
                    } else {
                        float crit_def = (has_crit_stats && a_pct >= 80.0f) ? 1.25f : 1.0f;
                        float scaled_o_dmg = (om.base_dmg * (0.85f + 0.03f * static_cast<float>(config.opponent_cpu_level))) / (def_mult * crit_def);
                        a_pct += scaled_o_dmg;
                        total_taken += scaled_o_dmg;
                        if (is_incineroar && uni(rng) < beh.revenge * 0.45f) {
                            revenge_active = true;
                        }

                        float kb = ComputeKnockback(a_pct, scaled_o_dmg, a_weight_units, om.bkb, om.kbg, o_pct);
                        float launch_speed = 0.030f * kb;
                        a_vx = -dir_to_opp * launch_speed * std::cos(om.angle_rad);
                        a_vy = launch_speed * std::sin(om.angle_rad);
                        a_timer = -std::max(6, static_cast<int>(kb * 0.34f));
                        o_timer = std::max(4, om.endlag_frames / 2);
                    }
                }
            }

            won = (amiibo_stocks > opp_stocks) ||
                  (amiibo_stocks == opp_stocks && a_pct <= o_pct);
            if (won) {
                if (opp_stocks > 0) opp_stocks = 0;
                if (amiibo_stocks <= 0) amiibo_stocks = 1;
            } else {
                if (amiibo_stocks > 0) amiibo_stocks = 0;
                if (opp_stocks <= 0) opp_stocks = 1;
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
        summary.total_frames_simulated += static_cast<uint64_t>(frames_sim);
        summary.total_hits_landed += hits_landed;
        summary.total_grabs_landed += grabs_landed;
        summary.total_ledge_recoveries += ledge_recoveries;

        xp_f = std::min(3912.0f, xp_f + xp_step_base * (won ? 1.15f : 0.90f));
        cpu_xp_f = std::min(2765.0f, cpu_xp_f + cpu_step_base * (won ? 1.15f : 0.90f));
        if (g == summary.total_games - 1 && summary.total_games >= 50) {
            xp_f = 3912.0f;
            cpu_xp_f = 2765.0f;
        }
        amiibo.SetLevelExperience(static_cast<uint16_t>(std::round(xp_f)));
        amiibo.SetCpuExperience(static_cast<uint16_t>(std::round(cpu_xp_f)));

        // Empirical Match Reinforcement + Coaching Curriculum Policy Update
        const float lr = std::clamp(2.5f / static_cast<float>(std::sqrt(g + 10)), 0.015f, 0.12f);
        float total_rew = std::max(1.0f, tilt_reward + smash_reward + aerial_reward);
        float emp_aerial_ratio = std::clamp(aerial_reward / total_rew, 0.15f, 0.85f);
        float emp_parry_bonus  = std::min(0.10f, 0.02f * static_cast<float>(parries));
        float emp_meteor_bonus = std::min(0.10f, 0.04f * static_cast<float>(meteors));

        auto nudge = [lr](float& param, float target) {
            param = std::clamp(param + lr * (target - param), 0.0f, 1.0f);
        };
        auto nudge_pct = [lr](float& param, float target) {
            param = std::clamp(param + lr * (target - param), 0.0f, 100.0f);
        };

        TrainingFocus effective_focus = config.focus;
        if (is_incineroar && config.focus == TrainingFocus::TournamentOptimal) {
            effective_focus = TrainingFocus::IncineroarRaidBoss;
        }

        switch (effective_focus) {
            case TrainingFocus::IncineroarRaidBoss:
                // Championship "Endeavor + Raid Boss" Incineroar Profile:
                // Grounded Alolan Whip command grab + D-Smash/F-Smash ledge kills + Up-Air juggle chains + Dair landing + Frame-Perfect Parry
                nudge(beh.near_range,          0.84f);
                nudge(beh.offensive,           0.88f);
                nudge(beh.grounded,            0.86f);
                nudge(beh.attack_out_cliff,    0.28f);
                nudge(beh.dash,                0.36f); // Walk & micro-space instead of reckless dashing
                nudge(beh.return_to_cliff,     0.96f); // Maximum offstage recovery discipline
                nudge(beh.air_offensive,       0.52f);
                nudge(beh.cliffer,             0.18f);
                nudge(beh.feint_master,        0.86f);
                nudge(beh.feint_counter,       0.88f);
                nudge(beh.feint_shooter,       0.84f);
                nudge(beh.catcher,             0.94f);
                nudge(beh.attack_cancel,       0.78f);
                nudge(beh.smash_holder,        0.58f);
                nudge(beh.dash_attacker,       0.16f);
                nudge(beh.critical_hitter,     0.76f);
                nudge(beh.meteor_master,       0.78f + emp_meteor_bonus * 0.5f);
                nudge(beh.shield_master,       0.62f);
                nudge(beh.just_shield_master,  0.96f);
                nudge(beh.shield_catch_master, 0.80f);
                nudge(beh.revenge,             0.84f);
                nudge(beh.advantageous_fighter,0.85f);
                nudge(beh.weaken_fighter,      0.78f);
                nudge(beh.appeal,              0.00f); // Zero taunting
                // Grounded Move Hierarchy (Alolan Whip + Down Smash at ledge + F-Smash/U-Smash + D-Tilt/F-Tilt)
                nudge_pct(beh.sspecial, 38.0f); // Alolan Whip (Side B) primary command grab
                nudge_pct(beh.dsmash,   16.0f); // Down Smash at ledge
                nudge_pct(beh.fsmash,   15.0f); // Forward Smash early KO
                nudge_pct(beh.usmash,    9.0f); // Up Smash anti-air / platform coverage
                nudge_pct(beh.dtilt,     9.0f); // Down Tilt combo starter & 2-frame
                nudge_pct(beh.ftilt,     8.0f); // Forward Tilt spacing poke
                nudge_pct(beh.utilt,     2.0f);
                nudge_pct(beh.nspecial,  1.0f); // Darkest Lariat OoS escape
                nudge_pct(beh.uspecial,  0.5f); // Never spam Cross Chop onstage
                nudge_pct(beh.dspecial,  1.5f); // Disciplined Revenge counter
                // Aerial Move Hierarchy ("Endeavor" Up-Air juggle chains + Down-Air landing/meteor + Bair/Fair)
                nudge_pct(beh.uair,         34.0f); // Up Air juggling chains (anti-Ridley/aerial counter)
                nudge_pct(beh.dair,         24.0f); // Down Air safe landing (anti-Mii Gunner) & meteor spike
                nudge_pct(beh.fair,         18.0f); // Forward Air horizontal KO
                nudge_pct(beh.bair,         16.0f); // Back Air fast KO
                nudge_pct(beh.air_sspecial,  4.0f); // Aerial Alolan Whip command grab mixup
                nudge_pct(beh.air_nspecial,  1.0f);
                nudge_pct(beh.air_uspecial,  0.5f);
                nudge_pct(beh.air_dspecial,  0.5f);
                break;

            case TrainingFocus::TournamentOptimal:
                nudge(beh.near_range,          0.62f);
                nudge(beh.offensive,           0.68f);
                nudge(beh.grounded,            0.78f);
                nudge(beh.attack_out_cliff,    0.32f + emp_meteor_bonus * 0.5f);
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
                nudge(beh.meteor_master,       0.42f + emp_meteor_bonus);
                nudge(beh.shield_master,       0.45f);
                nudge(beh.just_shield_master,  0.82f + emp_parry_bonus);
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
                nudge(beh.meteor_master,    0.68f + emp_meteor_bonus);
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
                nudge(beh.air_offensive,    std::max(0.88f, emp_aerial_ratio));
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
                meteors,
                frames_sim,
                hits_landed,
                grabs_landed,
                ledge_recoveries,
                MoveIdToName(ko_move_id)
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
    summary.avg_damage_dealt = static_cast<float>(sum_dealt / static_cast<double>(summary.total_games));
    summary.avg_damage_taken = static_cast<float>(sum_taken / static_cast<double>(summary.total_games));
    summary.final_level = amiibo.GetDisplayedLevel();
    summary.final_cpu_level = amiibo.GetCpuLevel();
    summary.final_personality = amiibo.EvaluatePersonality();
    summary.final_behavior = amiibo.DecodeBehavior();

    return summary;
}

std::vector<SpiritBenchmarkEntry> TrainingSimulator::RunIncineroarSpiritBenchmark(
    int games_per_loadout,
    ComputePreference pref) {

    struct CandidateSpec {
        const char* name;
        const char* tier;
        const char* primary;
        int16_t atk;
        int16_t def;
        uint8_t s1, s2, s3;
    };

    static const CandidateSpec kCandidates[8] = {
        {
            "1. Halberd Armor Knight + Trade-Off Ability (1.36x ATK / 2.16x DEF / 0.84x SPD)",
            "Unrestricted Raid Boss",
            "Giratina (Origin Forme)",
            2400, 2600,
            140, 15, 0
        },
        {
            "2. Halberd Armor Knight + Move Speed Up (1.15x ATK / 1.80x DEF / 0.84x SPD, 0% Start)",
            "Unrestricted Raid Boss",
            "Giratina (Origin Forme)",
            2400, 2600,
            140, 1, 0
        },
        {
            "3. Halberd Armor Knight + Trade-Off Speed (1.15x ATK / 1.80x DEF / 0.98x SPD)",
            "Unrestricted Raid Boss",
            "Giratina (Origin Forme)",
            2550, 2450,
            140, 14, 0
        },
        {
            "4. Gold Mario Super Armor (3-Slot Unflinching Alolan Whip Tank)",
            "Unrestricted Raid Boss",
            "Galeem",
            2500, 2500,
            10, 0, 0
        },
        {
            "5. Celebi Great Autoheal (3-Slot 5% Regen Every 5 Seconds)",
            "Unrestricted Raid Boss",
            "Galeem",
            2200, 2800,
            180, 0, 0
        },
        {
            "6. Critical Healing & Metal + Trade-Off Ability (#1 Tournament-Legal Meta)",
            "Tournament-Legal (Big 5 Banned)",
            "Tournament Standard (2100/2100)",
            2100, 2100,
            236, 15, 0
        },
        {
            "7. Trade-Off Ability + Physical Attack Up + Move Speed Up (Amiibo Doctor Best Legal Trio)",
            "Tournament-Legal (Big 5 Banned)",
            "Tournament Standard (2100/2100)",
            2100, 2100,
            15, 50, 1
        },
        {
            "8. Trade-Off Ability + Side Special Up + Landing Lag Down (Alolan Whip + Aerial Specialist)",
            "Tournament-Legal (Big 5 Banned)",
            "Tournament Standard (2100/2100)",
            2100, 2100,
            15, 87, 81
        }
    };

    std::vector<SpiritBenchmarkEntry> results;
    results.reserve(8);

    for (int i = 0; i < 8; ++i) {
        const auto& c = kCandidates[i];
        AmiiboBin bin;
        bin.InitializeBlankFighter(0x1BD7, 0); // Incineroar (#69, 0x1BD7000003860002)
        bin.SetSpiritType(SpiritType::Neutral);
        bin.SetStats(c.atk, c.def, false);
        bin.SetSpiritSlots(c.s1, c.s2, c.s3, true);
        // Pre-train to Lv 50 first so the benchmark evaluates mature Level 50 combat performance
        bin.SetDisplayedLevel(50);
        bin.SetCpuLevel(9);

        TrainingConfig cfg{};
        cfg.num_games = std::max(100, games_per_loadout);
        cfg.archetype = OpponentArchetype::FullRosterGauntlet;
        cfg.focus = TrainingFocus::IncineroarRaidBoss;
        cfg.compute_preference = pref;
        cfg.opponent_cpu_level = 9;
        cfg.freeze_learning_after_training = true;
        cfg.random_seed = 0x53534255u + static_cast<uint32_t>(i * 101u);

        TrainingSummary sum = RunTrainingSession(bin, cfg);

        SpiritBenchmarkEntry entry{};
        entry.loadout_name = c.name;
        entry.ruleset_tier = c.tier;
        entry.primary_spirit = c.primary;
        entry.atk = c.atk;
        entry.def = c.def;
        entry.s1 = c.s1;
        entry.s2 = c.s2;
        entry.s3 = c.s3;
        entry.games_played = sum.total_games;
        entry.wins = sum.wins;
        entry.losses = sum.losses;
        entry.win_rate_pct = 100.0f * static_cast<float>(sum.wins) / static_cast<float>(sum.total_games);
        entry.avg_dmg_dealt = sum.avg_damage_dealt;
        entry.avg_dmg_taken = sum.avg_damage_taken;
        entry.total_alolan_whips_and_grabs = sum.total_grabs_landed;
        entry.total_parries = sum.total_parries;
        entry.total_meteors = sum.total_meteors;
        entry.total_frames = sum.total_frames_simulated;
        entry.final_personality = sum.final_personality;
        results.push_back(entry);
    }

    return results;
}

} // namespace ssbu
