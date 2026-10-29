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

static const char* kHlslComputeShaderSource = R"(
cbuffer SimParams : register(b0) {
    uint num_games;
    uint random_seed;
    uint opponent_cpu_level;
    uint is_gauntlet;
    float my_weight;
    float opp_weight;
    float base_atk_bonus;
    float base_def_bonus;
    float spirit_combat_bonus;
    float spirit_recovery_rate;
    float neutral_score;
    float just_shield_prob;
    float meteor_prob;
    float ko_smash_bonus;
    float return_cliff_bonus;
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

[numthreads(64, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID) {
    uint gid = DTid.x;
    if (gid >= num_games) return;

    uint rng = random_seed ^ (gid * 747796405u + 2891336453u);
    if (rng == 0u) rng = 0x53534255u;

    float progress = float(gid) / max(1.0f, float(num_games - 1));
    float level_factor = min(1.0f, progress * 1.25f);
    float cpu_factor   = min(1.0f, progress * 1.30f);

    float atk_mult = 1.0f + (0.15f + 0.85f * level_factor) * base_atk_bonus;
    float def_mult = max(0.5f, 1.0f + (0.15f + 0.85f * level_factor) * base_def_bonus);

    int a_stocks = 3;
    int o_stocks = 3;
    float a_pct = 0.0f;
    float o_pct = 0.0f;
    float dealt = 0.0f;
    float taken = 0.0f;
    uint parries = 0u;
    uint meteors = 0u;

    float evolving_neutral = min(0.95f, neutral_score + 0.25f * level_factor);
    float amiibo_power = 0.24f + 0.28f * level_factor + 0.24f * cpu_factor +
                         0.26f * evolving_neutral + spirit_combat_bonus * (0.25f + 0.75f * level_factor);
    float opp_power = 0.42f + 0.042f * float(opponent_cpu_level);
    float win_prob = amiibo_power / (amiibo_power + opp_power);

    float eff_opp_weight = is_gauntlet ? (0.80f + 0.45f * float((gid * 7u) % 11u) / 10.0f) : opp_weight;

    [loop]
    for (int ex = 0; ex < 55; ++ex) {
        if (a_stocks <= 0 || o_stocks <= 0) break;
        if (NextFloat01(rng) < win_prob) {
            float hit = (10.5f + 6.5f * NextFloat01(rng)) * atk_mult;
            if (NextFloat01(rng) < just_shield_prob * 0.45f) {
                parries += 1u;
                hit *= 1.25f;
            }
            o_pct += hit;
            dealt += hit;
            if (spirit_recovery_rate > 0.0f) {
                a_pct = max(0.0f, a_pct - hit * spirit_recovery_rate);
            }
            float ko_thresh = 112.0f * eff_opp_weight - 14.0f * ko_smash_bonus;
            if (o_pct >= ko_thresh) {
                if (NextFloat01(rng) < meteor_prob * 0.5f) meteors += 1u;
                o_stocks -= 1;
                o_pct = 0.0f;
            }
        } else {
            float hit = (11.5f + 7.0f * NextFloat01(rng)) / def_mult;
            a_pct += hit;
            taken += hit;
            float my_ko_thresh = 108.0f * my_weight + 12.0f * return_cliff_bonus;
            if (a_pct >= my_ko_thresh) {
                a_stocks -= 1;
                a_pct = 0.0f;
            }
        }
    }

    bool won = (a_stocks > o_stocks) || (a_stocks == o_stocks && a_pct <= o_pct);
    if (won) {
        if (o_stocks > 0) o_stocks = 0;
        if (a_stocks == 0) a_stocks = 1;
    } else {
        if (a_stocks > 0) a_stocks = 0;
        if (o_stocks == 0) o_stocks = 1;
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

    // 1. Try dedicated/integrated Hardware GPU adapter first
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

    // 2. Try default D3D_DRIVER_TYPE_HARDWARE
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

    // 3. Try Direct3D 11 DirectCompute WARP Device (for headless Windows cloud servers without physical display GPU)
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
    // Probe dynamic OpenCL shared library on Linux/macOS before falling back to CPU
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
    info.backend_name = "CPU Fallback (No GPU Detected)";
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
        "SSBUAmiiboSimCS.hlsl",
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

    // Create Constant Buffer for GpuSimParams
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

    // Create Structured Output Buffer + UAV
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

    // Create Staging Buffer for GPU -> CPU Readback
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

    // Bind & Dispatch Compute Shader on GPU
    context->CSSetShader(compute_shader, nullptr, 0);
    context->CSSetConstantBuffers(0, 1, &cb_buffer);
    context->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

    UINT groups_x = (count + 63u) / 64u;
    context->Dispatch(groups_x, 1, 1);

    // Unbind UAV and copy results to staging buffer
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
    if (error_out) *error_out = "No GPU compute runtime detected on host; falling back to CPU.";
    return false;
#endif
}

} // namespace ssbu
