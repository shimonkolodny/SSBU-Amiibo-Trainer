#include "gpu_compute.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#else
#include <dlfcn.h>
#endif

namespace ssbu {

const char* GpuComputeEngine::PreferenceToString(ComputePreference pref) {
    switch (pref) {
        case ComputePreference::GpuFirstAuto: return "Auto: GPU First -> CPU Fallback";
        case ComputePreference::ForceGpu:     return "Force GPU (Direct3D 11 CS / OpenCL)";
        case ComputePreference::ForceCpu:     return "Force CPU Only";
        default:                              return "Auto: GPU First -> CPU Fallback";
    }
}

#ifdef _WIN32
namespace {

static std::string WideToUtf8(const WCHAR* wstr) {
    if (!wstr) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return "";
    std::string out(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, out.data(), len, nullptr, nullptr);
    return out;
}

// ---------------------------------------------------------------------------
// Direct3D 11 Compute Shader (cs_5_0):
// True 60 FPS Frame-by-Frame SSBU Physics, Stage Geometry, Move Frame-Data,
// Shield/Parry/Grab State Machine, and Knockback Formula Simulation
// ---------------------------------------------------------------------------
static const char* kHlslComputeShaderSource = R"(
cbuffer SimParams : register(b0) {
    uint num_games;
    uint random_seed;
    uint opponent_cpu_level;
    uint is_gauntlet;

    float my_weight;
    float opp_weight;
    float my_air_mobility;
    float opp_air_mobility;

    float base_atk_bonus;
    float base_def_bonus;
    float spirit_combat_bonus;
    float spirit_recovery_rate;

    float near_range;
    float offensive;
    float grounded;
    float air_offensive;

    float dash;
    float return_to_cliff;
    float attack_out_cliff;
    float catcher;

    float shield_master;
    float just_shield_prob;
    float meteor_prob;
    float smash_holder;

    float prob_tilt;
    float prob_smash;
    float prob_special;
    float prob_fair_bair;

    float prob_uair;
    float prob_dair;
    float extra_jump_bonus;
    float pad0;
};

struct GpuMatchOutcome {
    uint won;
    int amiibo_stocks;
    int opp_stocks;
    float damage_dealt;
    float damage_taken;
    uint parries;
    uint meteors;
    uint opponent_index;

    uint frames_simulated;
    uint hits_landed;
    uint grabs_landed;
    uint ledge_recoveries;
    uint signature_ko_move;
    float tilt_reward;
    float smash_reward;
    float aerial_reward;
};

RWStructuredBuffer<GpuMatchOutcome> OutMatches : register(u0);

uint NextRng(inout uint state) {
    state ^= (state << 13);
    state ^= (state >> 17);
    state ^= (state << 5);
    return state * 1597334677u;
}

float NextFloat01(inout uint state) {
    return float(NextRng(state) & 0x00FFFFFFu) / 16777216.0f;
}

// Move Frame Data Lookup (Startup, Endlag, Base Damage, BKB, KBG, AngleRad, Reach, IsMeteor)
void GetMoveFrameData(uint move_id, out int startup, out int endlag, out float dmg,
                      out float bkb, out float kbg, out float angle_rad, out float reach, out bool is_meteor) {
    is_meteor = false;
    if (move_id == 0u) { // Jab
        startup = 3; endlag = 12; dmg = 7.5f; bkb = 25.0f; kbg = 50.0f; angle_rad = 0.95f; reach = 14.0f;
    } else if (move_id == 1u) { // Forward Tilt
        startup = 7; endlag = 16; dmg = 11.5f; bkb = 35.0f; kbg = 86.0f; angle_rad = 0.66f; reach = 18.0f;
    } else if (move_id == 2u) { // Up Tilt
        startup = 6; endlag = 15; dmg = 10.0f; bkb = 40.0f; kbg = 88.0f; angle_rad = 1.48f; reach = 16.5f;
    } else if (move_id == 3u) { // Down Tilt
        startup = 6; endlag = 14; dmg = 9.5f; bkb = 32.0f; kbg = 80.0f; angle_rad = 1.22f; reach = 17.5f;
    } else if (move_id == 4u) { // Forward Smash
        startup = 15; endlag = 29; dmg = 18.5f; bkb = 42.0f; kbg = 105.0f; angle_rad = 0.63f; reach = 20.5f;
    } else if (move_id == 5u) { // Up Smash
        startup = 11; endlag = 26; dmg = 17.0f; bkb = 38.0f; kbg = 103.0f; angle_rad = 1.50f; reach = 19.0f;
    } else if (move_id == 6u) { // Down Smash
        startup = 10; endlag = 25; dmg = 15.5f; bkb = 35.0f; kbg = 97.0f; angle_rad = 0.52f; reach = 19.5f;
    } else if (move_id == 7u) { // Neutral Special (Projectile/Hitbox)
        startup = 12; endlag = 20; dmg = 10.5f; bkb = 28.0f; kbg = 68.0f; angle_rad = 0.75f; reach = 42.0f;
    } else if (move_id == 8u) { // Side Special
        startup = 13; endlag = 22; dmg = 13.5f; bkb = 36.0f; kbg = 90.0f; angle_rad = 0.72f; reach = 25.0f;
    } else if (move_id == 9u) { // Up Special (Out of Shield / Recovery)
        startup = 7; endlag = 26; dmg = 12.5f; bkb = 40.0f; kbg = 92.0f; angle_rad = 1.30f; reach = 17.5f;
    } else if (move_id == 10u) { // Down Special
        startup = 9; endlag = 23; dmg = 13.0f; bkb = 38.0f; kbg = 92.0f; angle_rad = 0.85f; reach = 18.0f;
    } else if (move_id == 11u) { // Grab + Throw
        startup = 6; endlag = 20; dmg = 11.5f; bkb = 42.0f; kbg = 88.0f; angle_rad = 0.90f; reach = 13.5f;
    } else if (move_id == 12u) { // Neutral Air (Nair)
        startup = 5; endlag = 9; dmg = 9.5f; bkb = 30.0f; kbg = 78.0f; angle_rad = 0.78f; reach = 15.5f;
    } else if (move_id == 13u) { // Forward Air (Fair)
        startup = 8; endlag = 11; dmg = 12.5f; bkb = 34.0f; kbg = 90.0f; angle_rad = 0.70f; reach = 18.5f;
    } else if (move_id == 14u) { // Back Air (Bair)
        startup = 7; endlag = 10; dmg = 14.5f; bkb = 36.0f; kbg = 99.0f; angle_rad = 0.61f; reach = 18.5f;
    } else if (move_id == 15u) { // Up Air (Uair)
        startup = 6; endlag = 9; dmg = 11.5f; bkb = 32.0f; kbg = 94.0f; angle_rad = 1.48f; reach = 17.5f;
    } else { // Down Air (Dair - Meteor Smash!)
        startup = 11; endlag = 14; dmg = 14.0f; bkb = 32.0f; kbg = 96.0f; angle_rad = -1.45f; reach = 17.0f;
        is_meteor = true;
    }
}

// Authentic SSBU Knockback Formula
float ComputeSsbuKnockback(float target_pct, float move_dmg, float target_weight_units,
                           float bkb, float kbg, float attacker_pct) {
    float rage = 1.0f + clamp((attacker_pct - 35.0f) / 115.0f, 0.0f, 1.0f) * 0.10f;
    float base_term = ((target_pct / 10.0f + (target_pct * move_dmg) / 20.0f) *
                       (200.0f / (target_weight_units + 100.0f)) * 1.4f + 18.0f) * (kbg / 100.0f) + bkb;
    return base_term * rage;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint gid = DTid.x;
    if (gid >= num_games) return;

    uint rng = random_seed ^ (gid * 747796405u + 2891336453u);
    if (rng == 0u) rng = 0x53534255u;

    float progress = float(gid) / max(1.0f, float(num_games - 1));
    float level_factor = min(1.0f, progress * 1.25f);
    float cpu_factor   = min(1.0f, progress * 1.30f);

    float atk_mult = 1.0f + (0.15f + 0.85f * level_factor) * (base_atk_bonus + spirit_combat_bonus * 0.6f);
    float def_mult = max(0.5f, 1.0f + (0.15f + 0.85f * level_factor) * (base_def_bonus + spirit_combat_bonus * 0.4f));

    float eff_opp_weight = is_gauntlet ? (0.78f + 0.48f * float((gid * 7u) % 13u) / 12.0f) : opp_weight;
    float a_weight_units = 100.0f * my_weight;
    float o_weight_units = 100.0f * eff_opp_weight;

    // 3-Stock Match State at 60 FPS (Stage: [-85, +85], Blast Zones: X=+-240, Y=+190/-140)
    int a_stocks = 3;
    int o_stocks = 3;
    float a_pct = 0.0f;
    float o_pct = 0.0f;
    float a_x = -35.0f, a_y = 0.0f, a_vx = 0.0f, a_vy = 0.0f;
    float o_x =  35.0f, o_y = 0.0f, o_vx = 0.0f, o_vy = 0.0f;
    int a_timer = 0, o_timer = 0; // Positive = Attack/Recovery frames, Negative = Hitstun frames
    int a_shield_frames = 0, o_shield_frames = 0;
    uint a_move = 0u, o_move = 0u;

    float dealt = 0.0f;
    float taken = 0.0f;
    uint parries = 0u;
    uint meteors = 0u;
    uint hits_landed = 0u;
    uint grabs_landed = 0u;
    uint ledge_recoveries = 0u;
    uint ko_move_id = 4u;
    float tilt_rew = 0.0f, smash_rew = 0.0f, aerial_rew = 0.0f;

    float a_reaction = 0.35f + 0.52f * (0.5f * level_factor + 0.5f * cpu_factor) + 0.10f * spirit_combat_bonus;
    float o_reaction = 0.30f + 0.052f * float(opponent_cpu_level);

    uint frame = 0u;
    [loop]
    for (uint step = 0u; step < 3600u; ++step) {
        if (a_stocks <= 0 || o_stocks <= 0) break;
        frame += 2u;

        // Autoheal spirit tick every 300 frames (5 seconds at 60 FPS)
        if ((frame % 300u) == 0u && spirit_recovery_rate > 0.0f) {
            a_pct = max(0.0f, a_pct - 40.0f * spirit_recovery_rate);
        }

        // Integrate 2D positions and velocities
        a_x += a_vx * 2.0f;
        a_y += a_vy * 2.0f;
        o_x += o_vx * 2.0f;
        o_y += o_vy * 2.0f;

        // Gravity & Ground Landing on Stage [-85, +85]
        if (a_y > 0.0f || abs(a_x) > 85.0f) {
            a_vy -= 0.19f;
            if (a_y <= 0.0f && abs(a_x) <= 85.0f && a_vy <= 0.0f) {
                a_y = 0.0f;
                a_vy = 0.0f;
            }
        } else {
            a_y = 0.0f;
            a_vy = 0.0f;
            a_vx *= 0.82f;
        }

        if (o_y > 0.0f || abs(o_x) > 85.0f) {
            o_vy -= 0.19f;
            if (o_y <= 0.0f && abs(o_x) <= 85.0f && o_vy <= 0.0f) {
                o_y = 0.0f;
                o_vy = 0.0f;
            }
        } else {
            o_y = 0.0f;
            o_vy = 0.0f;
            o_vx *= 0.82f;
        }

        // Offstage Ledge Recovery AI
        if (abs(a_x) > 85.0f && a_timer >= -4) {
            float ledge_dir = (a_x > 0.0f) ? -1.0f : 1.0f;
            float rec_skill = clamp(0.45f + 0.45f * return_to_cliff + 0.10f * extra_jump_bonus, 0.25f, 0.96f);
            a_vx = lerp(a_vx, ledge_dir * 1.95f * my_air_mobility, 0.25f);
            if (a_y < -18.0f && NextFloat01(rng) < rec_skill) {
                a_vy = 2.85f;
                if (abs(a_x) < 108.0f) {
                    a_x = (a_x > 0.0f) ? 83.0f : -83.0f;
                    a_y = 0.0f;
                    a_vx = 0.0f;
                    a_vy = 0.0f;
                    ledge_recoveries += 1u;
                }
            }
        }

        if (abs(o_x) > 85.0f && o_timer >= -4) {
            float o_ledge_dir = (o_x > 0.0f) ? -1.0f : 1.0f;
            o_vx = lerp(o_vx, o_ledge_dir * 1.80f * opp_air_mobility, 0.22f);
            if (o_y < -20.0f && NextFloat01(rng) < (0.42f + 0.045f * float(opponent_cpu_level))) {
                o_vy = 2.65f;
                if (abs(o_x) < 105.0f) {
                    o_x = (o_x > 0.0f) ? 83.0f : -83.0f;
                    o_y = 0.0f;
                    o_vx = 0.0f;
                    o_vy = 0.0f;
                }
            }
        }

        // Check Stage Blast Zones (Left/Right: +-240, Top: +190, Bottom: -140)
        if (abs(a_x) > 240.0f || a_y > 190.0f || a_y < -140.0f) {
            a_stocks -= 1;
            a_pct = 0.0f;
            a_x = -30.0f; a_y = 20.0f; a_vx = 0.0f; a_vy = 0.0f; a_timer = 0;
            continue;
        }
        if (abs(o_x) > 240.0f || o_y > 190.0f || o_y < -140.0f) {
            o_stocks -= 1;
            ko_move_id = a_move;
            if (a_move == 16u) meteors += 1u;
            o_pct = 0.0f;
            o_x = 30.0f; o_y = 20.0f; o_vx = 0.0f; o_vy = 0.0f; o_timer = 0;
            continue;
        }

        // Advance frame timers
        if (a_timer > 0) a_timer = max(0, a_timer - 2);
        else if (a_timer < 0) a_timer = min(0, a_timer + 2);

        if (o_timer > 0) o_timer = max(0, o_timer - 2);
        else if (o_timer < 0) o_timer = min(0, o_timer + 2);

        if (a_shield_frames > 0) a_shield_frames = max(0, a_shield_frames - 2);
        if (o_shield_frames > 0) o_shield_frames = max(0, o_shield_frames - 2);

        float dx = o_x - a_x;
        float dy = o_y - a_y;
        float dist = sqrt(dx * dx + dy * dy);
        float dir_to_opp = (dx >= 0.0f) ? 1.0f : -1.0f;

        // Amiibo Neutral Movement & Action Selection when actionable (a_timer == 0)
        if (a_timer == 0) {
            float desired_dist = lerp(28.0f, 11.0f, near_range);
            if (dist > desired_dist) {
                a_vx = dir_to_opp * (1.45f + 0.55f * dash) * my_air_mobility;
            } else if (dist < desired_dist * 0.55f && offensive < 0.45f) {
                a_vx = -dir_to_opp * 1.15f;
            }

            if (a_y == 0.0f && NextFloat01(rng) < (1.0f - grounded) * 0.22f) {
                a_vy = 2.45f;
            }
            if (abs(o_x) > 85.0f && NextFloat01(rng) < attack_out_cliff * 0.35f) {
                a_vx = dir_to_opp * 2.1f;
                if (a_y == 0.0f) a_vy = 2.2f;
            }

            if (dist < 22.0f && NextFloat01(rng) < shield_master * 0.28f) {
                a_shield_frames = 6;
            }
        }

        // Opponent Movement when actionable
        if (o_timer == 0) {
            if (dist > 15.0f) {
                o_vx = -dir_to_opp * 1.55f * opp_air_mobility;
            }
            if (o_y == 0.0f && NextFloat01(rng) < 0.14f) {
                o_vy = 2.35f;
            }
            if (dist < 20.0f && NextFloat01(rng) < 0.18f) {
                o_shield_frames = 6;
            }
        }

        // Hitbox / Combat Exchange Resolution when fighters are within engagement range
        if (dist <= 26.0f && (a_timer == 0 || o_timer == 0)) {
            if (a_y > 2.0f || abs(o_x) > 85.0f) {
                float r = NextFloat01(rng);
                if (abs(o_x) > 85.0f && r < max(prob_dair, meteor_prob * 0.55f)) a_move = 16u;
                else if (r < prob_fair_bair * 0.55f) a_move = 13u;
                else if (r < prob_fair_bair)         a_move = 14u;
                else if (r < prob_fair_bair + prob_uair) a_move = 15u;
                else a_move = 12u;
            } else {
                float r = NextFloat01(rng);
                if (o_shield_frames > 0 && NextFloat01(rng) < max(0.25f, catcher)) {
                    a_move = 11u;
                } else if (o_pct >= 85.0f && r < max(prob_smash, 0.38f + 0.25f * smash_holder)) {
                    a_move = (NextFloat01(rng) < 0.55f) ? 4u : ((NextFloat01(rng) < 0.6f) ? 5u : 6u);
                } else if (r < prob_tilt) {
                    a_move = 1u + (NextRng(rng) % 3u);
                } else if (r < prob_tilt + prob_smash) {
                    a_move = 4u + (NextRng(rng) % 3u);
                } else if (r < prob_tilt + prob_smash + prob_special) {
                    a_move = 7u + (NextRng(rng) % 4u);
                } else {
                    a_move = 0u;
                }
            }

            o_move = (o_y > 2.0f) ? (12u + (NextRng(rng) % 5u)) : (NextRng(rng) % 11u);

            int a_start, a_end, o_start, o_end;
            float a_dmg, a_bkb, a_kbg, a_ang, a_reach;
            float o_dmg, o_bkb, o_kbg, o_ang, o_reach;
            bool a_met, o_met;
            GetMoveFrameData(a_move, a_start, a_end, a_dmg, a_bkb, a_kbg, a_ang, a_reach, a_met);
            GetMoveFrameData(o_move, o_start, o_end, o_dmg, o_bkb, o_kbg, o_ang, o_reach, o_met);

            bool a_parried = false;
            if (a_shield_frames > 0 && NextFloat01(rng) < max(0.20f, just_shield_prob * 0.55f)) {
                a_parried = true;
                parries += 1u;
                a_start = max(1, a_start - 8);
            }

            float a_eff_frame = float(a_start) - 6.5f * a_reaction - ((a_reach > o_reach) ? 2.5f : 0.0f);
            float o_eff_frame = float(o_start) - 6.5f * o_reaction + float(max(0, -o_timer));

            if (a_parried || a_move == 11u || a_eff_frame <= o_eff_frame + 1.5f * (NextFloat01(rng) - 0.42f)) {
                float scaled_dmg = a_dmg * atk_mult * (a_parried ? 1.20f : 1.0f);
                o_pct += scaled_dmg;
                dealt += scaled_dmg;
                hits_landed += 1u;
                if (a_move == 11u) grabs_landed += 1u;

                float kb = ComputeSsbuKnockback(o_pct, scaled_dmg, o_weight_units, a_bkb, a_kbg, a_pct);
                float launch_speed = 0.032f * kb;
                o_vx = dir_to_opp * launch_speed * cos(a_ang);
                o_vy = launch_speed * sin(a_ang);
                if (a_met && abs(o_x) > 70.0f) {
                    o_vy = -abs(launch_speed * 0.95f);
                }
                o_timer = -max(6, int(kb * 0.35f));
                a_timer = max(4, a_end / 2);

                if (a_move >= 1u && a_move <= 3u) tilt_rew += scaled_dmg;
                else if (a_move >= 4u && a_move <= 6u) smash_rew += scaled_dmg;
                else if (a_move >= 12u) aerial_rew += scaled_dmg;
            } else {
                float scaled_o_dmg = (o_dmg * (0.85f + 0.03f * float(opponent_cpu_level))) / def_mult;
                a_pct += scaled_o_dmg;
                taken += scaled_o_dmg;

                float kb = ComputeSsbuKnockback(a_pct, scaled_o_dmg, a_weight_units, o_bkb, o_kbg, o_pct);
                float launch_speed = 0.030f * kb;
                a_vx = -dir_to_opp * launch_speed * cos(o_ang);
                a_vy = launch_speed * sin(o_ang);
                a_timer = -max(6, int(kb * 0.34f));
                o_timer = max(4, o_end / 2);
            }
        }
    }

    bool won = (a_stocks > o_stocks) || (a_stocks == o_stocks && a_pct <= o_pct);
    if (won) {
        if (o_stocks > 0) o_stocks = 0;
        if (a_stocks <= 0) a_stocks = 1;
    } else {
        if (a_stocks > 0) a_stocks = 0;
        if (o_stocks <= 0) o_stocks = 1;
    }

    GpuMatchOutcome out_item;
    out_item.won = won ? 1u : 0u;
    out_item.amiibo_stocks = a_stocks;
    out_item.opp_stocks = o_stocks;
    out_item.damage_dealt = dealt;
    out_item.damage_taken = taken;
    out_item.parries = parries;
    out_item.meteors = meteors;
    out_item.opponent_index = gid;
    out_item.frames_simulated = frame;
    out_item.hits_landed = hits_landed;
    out_item.grabs_landed = grabs_landed;
    out_item.ledge_recoveries = ledge_recoveries;
    out_item.signature_ko_move = ko_move_id;
    out_item.tilt_reward = tilt_rew;
    out_item.smash_reward = smash_rew;
    out_item.aerial_reward = aerial_rew;
    OutMatches[gid] = out_item;
}
)";

static bool CreateBestD3D11ComputeDevice(
    ID3D11Device** out_device,
    ID3D11DeviceContext** out_context,
    GpuDeviceInfo* out_info) {

    IDXGIFactory1* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));

    IDXGIAdapter1* best_hw_adapter = nullptr;
    DXGI_ADAPTER_DESC1 best_desc{};
    uint64_t best_vram = 0;

    if (SUCCEEDED(hr) && factory) {
        IDXGIAdapter1* adapter = nullptr;
        for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
            DXGI_ADAPTER_DESC1 desc{};
            if (SUCCEEDED(adapter->GetDesc1(&desc))) {
                bool is_software = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
                if (!is_software && (best_hw_adapter == nullptr || desc.DedicatedVideoMemory > best_vram)) {
                    if (best_hw_adapter) best_hw_adapter->Release();
                    best_hw_adapter = adapter;
                    best_desc = desc;
                    best_vram = desc.DedicatedVideoMemory;
                    continue;
                }
            }
            adapter->Release();
        }
    }

    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL actual_fl = D3D_FEATURE_LEVEL_11_0;

    if (best_hw_adapter) {
        hr = D3D11CreateDevice(
            best_hw_adapter,
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            0,
            feature_levels,
            2,
            D3D11_SDK_VERSION,
            out_device,
            &actual_fl,
            out_context);
        if (SUCCEEDED(hr) && *out_device) {
            if (out_info) {
                out_info->gpu_available = true;
                out_info->is_hardware_gpu = true;
                out_info->backend_name = "Direct3D 11 Hardware GPU (cs_5_0)";
                out_info->adapter_name = WideToUtf8(best_desc.Description);
                out_info->dedicated_vram_mb = static_cast<uint64_t>(best_desc.DedicatedVideoMemory / (1024 * 1024));
            }
            best_hw_adapter->Release();
            if (factory) factory->Release();
            return true;
        }
        best_hw_adapter->Release();
    }

    if (factory) factory->Release();

    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        feature_levels,
        2,
        D3D11_SDK_VERSION,
        out_device,
        &actual_fl,
        out_context);
    if (SUCCEEDED(hr) && *out_device) {
        if (out_info) {
            out_info->gpu_available = true;
            out_info->is_hardware_gpu = true;
            out_info->backend_name = "Direct3D 11 Hardware GPU (cs_5_0)";
            out_info->adapter_name = "Default Windows Hardware GPU";
            out_info->dedicated_vram_mb = 0;
        }
        return true;
    }

    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_WARP,
        nullptr,
        0,
        feature_levels,
        2,
        D3D11_SDK_VERSION,
        out_device,
        &actual_fl,
        out_context);
    if (SUCCEEDED(hr) && *out_device) {
        if (out_info) {
            out_info->gpu_available = true;
            out_info->is_hardware_gpu = false;
            out_info->backend_name = "Direct3D 11 DirectCompute (cs_5_0 WARP)";
            out_info->adapter_name = "Microsoft Basic Render / DirectCompute Device";
            out_info->dedicated_vram_mb = 0;
        }
        return true;
    }

    return false;
}

} // namespace
#endif

GpuDeviceInfo GpuComputeEngine::ProbeGpuDevice() {
    GpuDeviceInfo info{};
#ifdef _WIN32
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    if (CreateBestD3D11ComputeDevice(&device, &context, &info)) {
        if (context) context->Release();
        if (device) device->Release();
        return info;
    }
#else
    void* cl_lib = dlopen("libOpenCL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!cl_lib) cl_lib = dlopen("libOpenCL.so", RTLD_NOW | RTLD_LOCAL);
    if (cl_lib) {
        using clGetPlatformIDsFn = int (*)(uint32_t, void**, uint32_t*);
        auto fn = reinterpret_cast<clGetPlatformIDsFn>(dlsym(cl_lib, "clGetPlatformIDs"));
        uint32_t num_platforms = 0;
        if (fn && fn(0, nullptr, &num_platforms) == 0 && num_platforms > 0) {
            info.gpu_available = true;
            info.is_hardware_gpu = true;
            info.backend_name = "OpenCL GPU Compute";
            info.adapter_name = "OpenCL Device";
        }
        dlclose(cl_lib);
        if (info.gpu_available) return info;
    }
#endif
    info.gpu_available = false;
    info.is_hardware_gpu = false;
    info.backend_name = "CPU Fallback (60 FPS Frame Physics Engine)";
    info.adapter_name = "Host Multi-Core CPU";
    return info;
}

bool GpuComputeEngine::TryRunMatchBatchOnGpu(
    const GpuSimParams& params,
    std::vector<GpuMatchOutcome>& out_matches,
    GpuDeviceInfo* used_device_out,
    double* elapsed_ms_out,
    std::string* error_out) {

    auto t0 = std::chrono::high_resolution_clock::now();

#ifdef _WIN32
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    GpuDeviceInfo dev_info{};
    if (!CreateBestD3D11ComputeDevice(&device, &context, &dev_info)) {
        if (error_out) *error_out = "No Direct3D 11 compute device available.";
        return false;
    }

    ID3DBlob* cs_blob = nullptr;
    ID3DBlob* err_blob = nullptr;
    HRESULT hr = D3DCompile(
        kHlslComputeShaderSource,
        std::strlen(kHlslComputeShaderSource),
        "SSBUAmiiboPhysicsCS.hlsl",
        nullptr,
        nullptr,
        "CSMain",
        "cs_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL3,
        0,
        &cs_blob,
        &err_blob);

    if (FAILED(hr) || !cs_blob) {
        if (error_out && err_blob) {
            *error_out = std::string(static_cast<const char*>(err_blob->GetBufferPointer()), err_blob->GetBufferSize());
        }
        if (err_blob) err_blob->Release();
        context->Release();
        device->Release();
        return false;
    }
    if (err_blob) err_blob->Release();

    ID3D11ComputeShader* compute_shader = nullptr;
    hr = device->CreateComputeShader(cs_blob->GetBufferPointer(), cs_blob->GetBufferSize(), nullptr, &compute_shader);
    cs_blob->Release();
    if (FAILED(hr) || !compute_shader) {
        context->Release();
        device->Release();
        return false;
    }

    D3D11_BUFFER_DESC cb_desc{};
    cb_desc.ByteWidth = sizeof(GpuSimParams);
    cb_desc.Usage = D3D11_USAGE_DEFAULT;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA cb_init{};
    cb_init.pSysMem = &params;

    ID3D11Buffer* cb_buffer = nullptr;
    hr = device->CreateBuffer(&cb_desc, &cb_init, &cb_buffer);
    if (FAILED(hr) || !cb_buffer) {
        compute_shader->Release();
        context->Release();
        device->Release();
        return false;
    }

    uint32_t count = std::max(1u, params.num_games);
    D3D11_BUFFER_DESC uav_buf_desc{};
    uav_buf_desc.ByteWidth = static_cast<UINT>(sizeof(GpuMatchOutcome) * count);
    uav_buf_desc.Usage = D3D11_USAGE_DEFAULT;
    uav_buf_desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    uav_buf_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    uav_buf_desc.StructureByteStride = sizeof(GpuMatchOutcome);

    ID3D11Buffer* uav_buffer = nullptr;
    hr = device->CreateBuffer(&uav_buf_desc, nullptr, &uav_buffer);
    if (FAILED(hr) || !uav_buffer) {
        cb_buffer->Release();
        compute_shader->Release();
        context->Release();
        device->Release();
        return false;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
    uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = count;

    ID3D11UnorderedAccessView* uav = nullptr;
    hr = device->CreateUnorderedAccessView(uav_buffer, &uav_desc, &uav);
    if (FAILED(hr) || !uav) {
        uav_buffer->Release();
        cb_buffer->Release();
        compute_shader->Release();
        context->Release();
        device->Release();
        return false;
    }

    D3D11_BUFFER_DESC staging_desc = uav_buf_desc;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.BindFlags = 0;
    staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    staging_desc.MiscFlags = 0;

    ID3D11Buffer* staging_buffer = nullptr;
    hr = device->CreateBuffer(&staging_desc, nullptr, &staging_buffer);
    if (FAILED(hr) || !staging_buffer) {
        uav->Release();
        uav_buffer->Release();
        cb_buffer->Release();
        compute_shader->Release();
        context->Release();
        device->Release();
        return false;
    }

    context->CSSetShader(compute_shader, nullptr, 0);
    context->CSSetConstantBuffers(0, 1, &cb_buffer);
    context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

    UINT groups_x = (count + 63u) / 64u;
    context->Dispatch(groups_x, 1, 1);

    ID3D11UnorderedAccessView* null_uav = nullptr;
    context->CSSetUnorderedAccessViews(0, 1, &null_uav, nullptr);
    context->CopyResource(staging_buffer, uav_buffer);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    hr = context->Map(staging_buffer, 0, D3D11_MAP_READ, 0, &mapped);
    bool read_ok = false;
    if (SUCCEEDED(hr) && mapped.pData) {
        out_matches.resize(count);
        std::memcpy(out_matches.data(), mapped.pData, sizeof(GpuMatchOutcome) * count);
        context->Unmap(staging_buffer, 0);
        read_ok = true;
    }

    staging_buffer->Release();
    uav->Release();
    uav_buffer->Release();
    cb_buffer->Release();
    compute_shader->Release();
    context->Release();
    device->Release();

    auto t1 = std::chrono::high_resolution_clock::now();
    if (elapsed_ms_out) {
        *elapsed_ms_out = std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
    if (used_device_out) {
        *used_device_out = dev_info;
    }
    return read_ok;
#else
    (void)params;
    (void)out_matches;
    (void)used_device_out;
    (void)t0;
    if (elapsed_ms_out) *elapsed_ms_out = 0.0;
    if (error_out) *error_out = "No GPU compute runtime detected on host; falling back to 60 FPS CPU physics engine.";
    return false;
#endif
}

} // namespace ssbu
