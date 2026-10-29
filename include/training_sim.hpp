#pragma once
#include "amiibo_bin.hpp"
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
    int opponent_cpu_level = 9;
    bool freeze_learning_after_training = true;
    uint32_t random_seed = 0x53534255;
};

struct MatchResult {
    int game_index;
    const char* opponent_name;
    bool amiibo_won;
    int amiibo_stocks_left;
    int opponent_stocks_left;
    float damage_dealt;
    float damage_taken;
    int parries_landed;
    int meteors_landed;
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
    int initial_level = 1;
    int final_level = 50;
    int initial_cpu_level = 1;
    int final_cpu_level = 9;
    std::string initial_personality;
    std::string final_personality;
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
};

} // namespace ssbu
