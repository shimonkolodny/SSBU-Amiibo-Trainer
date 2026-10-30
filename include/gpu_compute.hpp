#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ssbu {

enum class ComputePreference {
    GpuFirstAuto = 0, // Attempt GPU Compute Shader first; automatically fall back to CPU if unavailable
    ForceGpu     = 1, // Force GPU Compute Shader execution
    ForceCpu     = 2  // Force CPU execution
};

struct GpuDeviceInfo {
    bool gpu_available = false;
    bool is_hardware_gpu = false;
    std::string backend_name = "CPU Fallback";
    std::string adapter_name = "Host CPU";
    uint64_t dedicated_vram_mb = 0;
};

// Aligned parameter block for 60 FPS frame-by-frame SSBU Physics & Hitbox Compute Shader (register b0)
struct GpuSimParams {
    uint32_t num_games = 500;
    uint32_t random_seed = 0x53534255;
    uint32_t opponent_cpu_level = 9;
    uint32_t is_gauntlet = 1;

    float my_weight = 1.0f;
    float opp_weight = 1.0f;
    float my_air_mobility = 1.1f;
    float opp_air_mobility = 1.1f;

    float base_atk_bonus = 0.0f;
    float base_def_bonus = 0.0f;
    float spirit_combat_bonus = 0.0f;
    float spirit_recovery_rate = 0.0f;

    // Amiibo active behavior weights driving the 60 FPS frame-by-frame fighter state machine
    float near_range = 0.45f;
    float offensive = 0.55f;
    float grounded = 0.55f;
    float air_offensive = 0.45f;

    float dash = 0.45f;
    float return_to_cliff = 0.70f;
    float attack_out_cliff = 0.40f;
    float catcher = 0.35f;

    float shield_master = 0.40f;
    float just_shield_prob = 0.35f;
    float meteor_prob = 0.35f;
    float smash_holder = 0.30f;

    // Move probability distribution groups [0..1]
    float prob_tilt = 0.36f;
    float prob_smash = 0.32f;
    float prob_special = 0.20f;
    float prob_fair_bair = 0.45f;

    float prob_uair = 0.18f;
    float prob_dair = 0.16f;
    float extra_jump_bonus = 0.0f;
    float pad0 = 0.0f;
};

struct GpuMatchOutcome {
    uint32_t won = 0;
    int32_t amiibo_stocks = 3;
    int32_t opp_stocks = 3;
    float damage_dealt = 0.0f;
    float damage_taken = 0.0f;
    uint32_t parries = 0;
    uint32_t meteors = 0;
    uint32_t opponent_index = 0;

    // 60 FPS Frame-by-Frame Physics & Hitbox Telemetry
    uint32_t frames_simulated = 0;
    uint32_t hits_landed = 0;
    uint32_t grabs_landed = 0;
    uint32_t ledge_recoveries = 0;
    uint32_t signature_ko_move = 0; // Move ID (0..19) that scored the final stock KO
    float tilt_reward = 0.0f;
    float smash_reward = 0.0f;
    float aerial_reward = 0.0f;
};

class GpuComputeEngine {
public:
    // Probes the system for a Direct3D 11 Compute Shader (cs_5_0) or OpenCL GPU device
    static GpuDeviceInfo ProbeGpuDevice();

    // Runs 60 FPS frame-by-frame SSBU combat matches in parallel on the GPU first; returns false to trigger CPU fallback
    static bool TryRunMatchBatchOnGpu(
        const GpuSimParams& params,
        std::vector<GpuMatchOutcome>& out_matches,
        GpuDeviceInfo* used_device_out = nullptr,
        double* elapsed_ms_out = nullptr,
        std::string* error_out = nullptr);

    static const char* PreferenceToString(ComputePreference pref);
};

} // namespace ssbu
