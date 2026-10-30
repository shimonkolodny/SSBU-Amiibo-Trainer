#pragma once
#include "amiibo_bin.hpp"
#include "gpu_compute.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ssbu {

enum class OpponentArchetype {
    FullRosterGauntlet = 0,
    Balanced,
    Rushdown,
    Zoner,
    DefensiveParry,
    Grappler,
    AerialEdgeguarder
};

enum class TrainingFocus {
    TournamentOptimal = 0,
    AggressiveStageControl,
    ParryAndPunishMaster,
    AerialMeteorEdgeguarder,
    ShowOffEntertainer,
    AdaptiveCounterplay,
    HeavyArmorSmashBrawler
};

struct TrainingConfig {
    int num_games = 500;
    uint16_t opponent_char_id = 0x0580; // Default: Fox (when not FullRosterGauntlet)
    OpponentArchetype archetype = OpponentArchetype::FullRosterGauntlet;
    TrainingFocus focus = TrainingFocus::TournamentOptimal;
    ComputePreference compute_preference = ComputePreference::GpuFirstAuto; // Default: GPU First -> CPU Fallback
    int opponent_cpu_level = 9;
    bool freeze_learning_after_training = true;
    uint32_t random_seed = 0x53534255;
};

struct MatchResult {
    int game_index = 0;
    const char* opponent_name = "Mario";
    bool amiibo_won = false;
    int amiibo_stocks_left = 0;
    int opponent_stocks_left = 0;
    float damage_dealt = 0.0f;
    float damage_taken = 0.0f;
    int parries_landed = 0;
    int meteors_landed = 0;
    int frames_simulated = 0;
    int hits_landed = 0;
    int grabs_landed = 0;
    int ledge_recoveries = 0;
    const char* signature_ko_move = "Forward Smash";
};

struct TrainingSummary {
    int total_games = 0;
    int wins = 0;
    int losses = 0;
    float first_quarter_win_rate = 0.0f;
    float final_quarter_win_rate = 0.0f;
    float avg_damage_dealt = 0.0f;
    float avg_damage_taken = 0.0f;
    int total_parries = 0;
    int total_meteors = 0;
    uint64_t total_frames_simulated = 0;
    int total_hits_landed = 0;
    int total_grabs_landed = 0;
    int total_ledge_recoveries = 0;
    int initial_level = 1;
    int final_level = 50;
    int initial_cpu_level = 1;
    int final_cpu_level = 9;
    std::string initial_personality;
    std::string final_personality;
    bool executed_on_gpu = false;
    std::string compute_backend_used = "CPU Fallback";
    std::string compute_adapter_name = "Host Multi-Core CPU";
    double compute_elapsed_ms = 0.0;
    AmiiboBehaviorParams final_behavior;
    std::vector<MatchResult> sample_matches;
};

using ProgressCallback = std::function<void(int completed_games, int total_games, float rolling_win_rate)>;

class TrainingSimulator {
public:
    static TrainingSummary RunTrainingSession(
        AmiiboBin& amiibo,
        const TrainingConfig& config,
        ProgressCallback on_progress = nullptr);

    static const char* ArchetypeToString(OpponentArchetype a);
    static const char* FocusToString(TrainingFocus f);
    static const char* MoveIdToName(uint32_t move_id);
};

} // namespace ssbu
