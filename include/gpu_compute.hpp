#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ssbu {

enum class ComputePreference : uint8_t {
    GpuFirstAuto = 0, // Try Hardware GPU (D3D11 CS / OpenCL) first, then fall back to CPU
    ForceGpu     = 1, // Require GPU compute backend (D3D11 DirectCompute / OpenCL)
    ForceCpu     = 2  // Force CPU execution
};

struct GpuDeviceInfo {
    bool gpu_available = false;
    bool is_hardware_gpu = false;
    std::string backend_name = "CPU Fallback";
    std::string adapter_name = "Host CPU";
    uint64_t dedicated_vram_mb = 0;
};

struct GpuSimParams {
    uint32_t num_games = 500;
    uint32_t random_seed = 0x53534255;
    uint32_t opponent_cpu_level = 9;
    uint32_t is_gauntlet = 1;
    float my_weight = 1.0f;
    float opp_weight = 1.0f;
    float base_atk_bonus = 0.0f;
    float base_def_bonus = 0.0f;
    float spirit_combat_bonus = 0.0f;
    float spirit_recovery_rate = 0.0f;
    float neutral_score = 0.5f;
    float just_shield_prob = 0.25f;
    float meteor_prob = 0.30f;
    float ko_smash_bonus = 0.5f;
    float return_cliff_bonus = 0.6f;
    float pad0 = 0.0f;
};

struct GpuMatchOutcome {
    uint32_t won;
    int32_t amiibo_stocks;
    int32_t opp_stocks;
    float damage_dealt;
    float damage_taken;
    uint32_t parries;
    uint32_t meteors;
    uint32_t opponent_index;
};

class GpuComputeEngine {
public:
    // Probes the system for GPU compute capabilities (Direct3D 11 Compute Shader cs_5_0 / OpenCL)
    static GpuDeviceInfo ProbeGpuDevice();

    // Runs batched SSBU match simulations on the GPU first; returns true if executed on GPU,
    // or false if caller should fall back to CPU.
    static bool TryRunMatchBatchOnGpu(
        const GpuSimParams& params,
        std::vector<GpuMatchOutcome>& out_matches,
        GpuDeviceInfo* used_device_out = nullptr,
        double* elapsed_ms_out = nullptr,
        std::string* error_out = nullptr);

    static const char* PreferenceToString(ComputePreference pref);
};

} // namespace ssbu
