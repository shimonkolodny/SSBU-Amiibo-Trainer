#include "amiibo_bin.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>

namespace ssbu {

namespace {

// Exact 50-level XP thresholds from MiDe's SSBU Amiibo Bin Guide (0x14C-0x14D)
constexpr uint16_t kLevelXpTable[50] = {
    0,    8,    22,   41,   63,   90,   120,  155,  195,  238,
    284,  330,  376,  426,  476,  528,  580,  632,  684,  737,
    790,  843,  896,  950,  1004, 1058, 1112, 1167, 1222, 1277,
    1339, 1406, 1478, 1555, 1637, 1724, 1816, 1910, 2012, 2115,
    2220, 2329, 2459, 2619, 2799, 2999, 3209, 3429, 3669, 3912
};

// Exact 9-level hidden CPU XP thresholds (0x14E-0x14F)
constexpr uint16_t kCpuXpTable[9] = {
    0, 63, 210, 434, 749, 1141, 1603, 2065, 2765
};

// Portable SHA-256 + HMAC-SHA256 + keystream helper for NTAG215 signature/appdata integrity
struct Sha256Ctx {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t data[64];
    uint32_t datalen;
};

static inline uint32_t RotR(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

static const uint32_t kSha256K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void Sha256Transform(Sha256Ctx* ctx, const uint8_t data[64]) {
    uint32_t m[64];
    for (int i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (uint32_t(data[j]) << 24) | (uint32_t(data[j+1]) << 16) |
               (uint32_t(data[j+2]) << 8) | uint32_t(data[j+3]);
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = RotR(m[i-15], 7) ^ RotR(m[i-15], 18) ^ (m[i-15] >> 3);
        uint32_t s1 = RotR(m[i-2], 17) ^ RotR(m[i-2], 19) ^ (m[i-2] >> 10);
        m[i] = m[i-16] + s0 + m[i-7] + s1;
    }
    uint32_t a=ctx->state[0], b=ctx->state[1], c=ctx->state[2], d=ctx->state[3];
    uint32_t e=ctx->state[4], f=ctx->state[5], g=ctx->state[6], h=ctx->state[7];
    for (int i = 0; i < 64; ++i) {
        uint32_t S1 = RotR(e, 6) ^ RotR(e, 11) ^ RotR(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + kSha256K[i] + m[i];
        uint32_t S0 = RotR(a, 2) ^ RotR(a, 13) ^ RotR(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;
        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void Sha256Compute(const uint8_t* input, size_t len, uint8_t out[32]) {
    Sha256Ctx ctx{};
    ctx.state[0]=0x6a09e667; ctx.state[1]=0xbb67ae85; ctx.state[2]=0x3c6ef372; ctx.state[3]=0xa54ff53a;
    ctx.state[4]=0x510e527f; ctx.state[5]=0x9b05688c; ctx.state[6]=0x1f83d9ab; ctx.state[7]=0x5be0cd19;
    for (size_t i = 0; i < len; ++i) {
        ctx.data[ctx.datalen++] = input[i];
        if (ctx.datalen == 64) {
            Sha256Transform(&ctx, ctx.data);
            ctx.bitlen += 512;
            ctx.datalen = 0;
        }
    }
    uint32_t i = ctx.datalen;
    ctx.bitlen += i * 8;
    ctx.data[i++] = 0x80;
    if (i > 56) {
        while (i < 64) ctx.data[i++] = 0x00;
        Sha256Transform(&ctx, ctx.data);
        i = 0;
    }
    while (i < 56) ctx.data[i++] = 0x00;
    for (int k = 7; k >= 0; --k) {
        ctx.data[56 + (7 - k)] = static_cast<uint8_t>((ctx.bitlen >> (k * 8)) & 0xFF);
    }
    Sha256Transform(&ctx, ctx.data);
    for (int j = 0; j < 8; ++j) {
        out[j*4 + 0] = (ctx.state[j] >> 24) & 0xFF;
        out[j*4 + 1] = (ctx.state[j] >> 16) & 0xFF;
        out[j*4 + 2] = (ctx.state[j] >> 8)  & 0xFF;
        out[j*4 + 3] = (ctx.state[j] >> 0)  & 0xFF;
    }
}

static void HmacSha256(const uint8_t* key, size_t key_len,
                       const uint8_t* msg, size_t msg_len,
                       uint8_t out[32]) {
    uint8_t k_ipad[64]{};
    uint8_t k_opad[64]{};
    uint8_t tk[32]{};
    if (key_len > 64) {
        Sha256Compute(key, key_len, tk);
        key = tk;
        key_len = 32;
    }
    std::memcpy(k_ipad, key, key_len);
    std::memcpy(k_opad, key, key_len);
    for (int i = 0; i < 64; ++i) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5c;
    }
    std::vector<uint8_t> inner(64 + msg_len);
    std::memcpy(inner.data(), k_ipad, 64);
    std::memcpy(inner.data() + 64, msg, msg_len);
    uint8_t inner_hash[32]{};
    Sha256Compute(inner.data(), inner.size(), inner_hash);

    uint8_t outer[64 + 32]{};
    std::memcpy(outer, k_opad, 64);
    std::memcpy(outer + 64, inner_hash, 32);
    Sha256Compute(outer, sizeof(outer), out);
}

} // namespace

float AmiiboBehaviorParams::ImplicitJabPercent() const {
    float s = ftilt + utilt + dtilt + fsmash + usmash + dsmash +
              nspecial + sspecial + uspecial + dspecial;
    return std::max(0.0f, 100.0f - s);
}

float AmiiboBehaviorParams::ImplicitNairPercent() const {
    float s = fair + bair + uair + dair +
              air_nspecial + air_sspecial + air_uspecial + air_dspecial;
    return std::max(0.0f, 100.0f - s);
}

void AmiiboBehaviorParams::NormalizeMoveSums() {
    float g_sum = ftilt + utilt + dtilt + fsmash + usmash + dsmash +
                  nspecial + sspecial + uspecial + dspecial;
    if (g_sum > 95.0f && g_sum > 0.0f) {
        float scale = 92.0f / g_sum; // Leave ~8% for implicit Jab
        ftilt *= scale; utilt *= scale; dtilt *= scale;
        fsmash *= scale; usmash *= scale; dsmash *= scale;
        nspecial *= scale; sspecial *= scale; uspecial *= scale; dspecial *= scale;
    }
    float a_sum = fair + bair + uair + dair +
                  air_nspecial + air_sspecial + air_uspecial + air_dspecial;
    if (a_sum > 92.0f && a_sum > 0.0f) {
        float scale = 86.0f / a_sum; // Leave ~14% for implicit Nair
        fair *= scale; bair *= scale; uair *= scale; dair *= scale;
        air_nspecial *= scale; air_sspecial *= scale; air_uspecial *= scale; air_dspecial *= scale;
    }
}

AmiiboBin::AmiiboBin() {
    InitializeBlankFighter(0x0000, 0);
}

void AmiiboBin::InitializeBlankFighter(uint16_t char_id, uint8_t skin) {
    data_.assign(kStandardAmiiboSize, 0);
    // Standard NTAG215 header bytes & capability container
    data_[0x00] = 0x04;
    data_[0x01] = 0xA1;
    data_[0x02] = 0xB2;
    data_[0x03] = 0xC3;
    data_[0x10] = 0xE1;
    data_[0x11] = 0x10;
    data_[0x12] = 0x3E;
    data_[0x13] = 0x00;

    // Set SSBU App Access ID (0x34F80200) at 0xB6..0xB9
    data_[0xB6] = 0x34;
    data_[0xB7] = 0xF8;
    data_[0xB8] = 0x02;
    data_[0xB9] = 0x00;

    SetCharacterId(char_id);
    SetLearningEnabled(true);
    SetAlternateSkin(skin);
    SetDisplayedLevel(1);
    SetCpuLevel(1);
    SetSpiritType(SpiritType::Neutral);
    SetStats(0, 0, true);
    SetSpiritSlots(0, 0, 0, false);

    AmiiboBehaviorParams defaults{};
    EncodeBehavior(defaults);
    UpdateChecksumsAndCrypto();
}

bool AmiiboBin::LoadFromFile(const std::string& path, std::string* error_out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        if (error_out) *error_out = "Could not open file: " + path;
        return false;
    }
    auto sz = f.tellg();
    if (sz != 540 && sz != 532 && sz != 572) {
        if (error_out) {
            std::ostringstream oss;
            oss << "Invalid Amiibo .bin size (" << sz << " bytes). Expected 540, 532, or 572 bytes.";
            *error_out = oss.str();
        }
        return false;
    }
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    if (!f.read(reinterpret_cast<char*>(buf.data()), sz)) {
        if (error_out) *error_out = "Failed to read file bytes.";
        return false;
    }
    if (buf.size() < kStandardAmiiboSize) {
        buf.resize(kStandardAmiiboSize, 0);
    } else if (buf.size() > kStandardAmiiboSize) {
        buf.resize(kStandardAmiiboSize);
    }
    data_ = std::move(buf);
    return true;
}

bool AmiiboBin::SaveToFile(const std::string& path, std::string* error_out) {
    UpdateChecksumsAndCrypto();
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        if (error_out) *error_out = "Could not open output file for writing: " + path;
        return false;
    }
    f.write(reinterpret_cast<const char*>(data_.data()), static_cast<std::streamsize>(data_.size()));
    if (!f.good()) {
        if (error_out) *error_out = "Failed while writing output .bin file.";
        return false;
    }
    return true;
}

bool AmiiboBin::LoadRetailKey(const std::string& key_path, std::string* error_out) {
    std::ifstream f(key_path, std::ios::binary | std::ios::ate);
    if (!f) {
        if (error_out) *error_out = "Could not open key_retail.bin: " + key_path;
        return false;
    }
    auto sz = f.tellg();
    if (sz != 160 && sz != 80) {
        if (error_out) *error_out = "Invalid key file size (expected 160-byte key_retail.bin).";
        return false;
    }
    f.seekg(0, std::ios::beg);
    retail_key_.resize(static_cast<size_t>(sz));
    f.read(reinterpret_cast<char*>(retail_key_.data()), sz);
    has_retail_key_ = true;
    return true;
}

void AmiiboBin::SetRetailKeyBytes(const std::vector<uint8_t>& key_bytes) {
    retail_key_ = key_bytes;
    has_retail_key_ = !retail_key_.empty();
}

uint16_t AmiiboBin::GetCharacterId() const {
    return (static_cast<uint16_t>(data_[0x54]) << 8) | data_[0x55];
}

void AmiiboBin::SetCharacterId(uint16_t char_id) {
    data_[0x54] = static_cast<uint8_t>((char_id >> 8) & 0xFF);
    data_[0x55] = static_cast<uint8_t>(char_id & 0xFF);
    data_[0x58] = 0x00; // Figure type
    data_[0x5A] = 0x00; // SSBU series
}

bool AmiiboBin::GetLearningEnabled() const {
    return ((data_[0xE2] >> 7) & 0x01) != 0;
}

void AmiiboBin::SetLearningEnabled(bool enabled) {
    if (enabled) data_[0xE2] |= 0x80;
    else         data_[0xE2] &= ~0x80;
}

uint8_t AmiiboBin::GetAlternateSkin() const {
    return (data_[0x1A3] >> 4) & 0x0F;
}

void AmiiboBin::SetAlternateSkin(uint8_t skin) {
    data_[0x1A3] = (data_[0x1A3] & 0x0F) | ((skin & 0x0F) << 4);
}

uint16_t AmiiboBin::GetLevelExperience() const {
    return static_cast<uint16_t>(data_[0x14C]) |
           (static_cast<uint16_t>(data_[0x14D]) << 8);
}

void AmiiboBin::SetLevelExperience(uint16_t xp) {
    data_[0x14C] = static_cast<uint8_t>(xp & 0xFF);
    data_[0x14D] = static_cast<uint8_t>((xp >> 8) & 0xFF);
}

int AmiiboBin::GetDisplayedLevel() const {
    uint16_t xp = GetLevelExperience();
    int lvl = 1;
    for (int i = 0; i < 50; ++i) {
        if (xp >= kLevelXpTable[i]) lvl = i + 1;
        else break;
    }
    return lvl;
}

void AmiiboBin::SetDisplayedLevel(int level) {
    int clamped = std::clamp(level, 1, 50);
    SetLevelExperience(kLevelXpTable[clamped - 1]);
}

uint16_t AmiiboBin::GetCpuExperience() const {
    return static_cast<uint16_t>(data_[0x14E]) |
           (static_cast<uint16_t>(data_[0x14F]) << 8);
}

void AmiiboBin::SetCpuExperience(uint16_t cpu_xp) {
    data_[0x14E] = static_cast<uint8_t>(cpu_xp & 0xFF);
    data_[0x14F] = static_cast<uint8_t>((cpu_xp >> 8) & 0xFF);
}

int AmiiboBin::GetCpuLevel() const {
    uint16_t xp = GetCpuExperience();
    int lvl = 1;
    for (int i = 0; i < 9; ++i) {
        if (xp >= kCpuXpTable[i]) lvl = i + 1;
        else break;
    }
    return lvl;
}

void AmiiboBin::SetCpuLevel(int cpu_level) {
    int clamped = std::clamp(cpu_level, 1, 9);
    SetCpuExperience(kCpuXpTable[clamped - 1]);
}

uint16_t AmiiboBin::GetGiftCount() const {
    return static_cast<uint16_t>(data_[0x156]) |
           (static_cast<uint16_t>(data_[0x157]) << 8);
}

void AmiiboBin::SetGiftCount(uint16_t count) {
    data_[0x156] = static_cast<uint8_t>(count & 0xFF);
    data_[0x157] = static_cast<uint8_t>((count >> 8) & 0xFF);
}

SpiritType AmiiboBin::GetSpiritType() const {
    return static_cast<SpiritType>(data_[0xE3] & 0x03);
}

void AmiiboBin::SetSpiritType(SpiritType type) {
    data_[0xE3] = (data_[0xE3] & ~0x03) | (static_cast<uint8_t>(type) & 0x03);
    // Update Type Affinities at 0x1A4-0x1A7
    int8_t neutral_aff = 0, atk_aff = 0, def_aff = 0, grab_aff = 0;
    switch (type) {
        case SpiritType::Neutral: neutral_aff = 60; break;
        case SpiritType::Attack:  atk_aff     = 60; break;
        case SpiritType::Defense: def_aff     = 60; break;
        case SpiritType::Grab:    grab_aff    = 60; break;
    }
    data_[0x1A4] = static_cast<uint8_t>(neutral_aff);
    data_[0x1A5] = static_cast<uint8_t>(atk_aff);
    data_[0x1A6] = static_cast<uint8_t>(def_aff);
    data_[0x1A7] = static_cast<uint8_t>(grab_aff);
}

int16_t AmiiboBin::GetAttackStat() const {
    uint16_t raw = static_cast<uint16_t>(data_[0x150]) |
                   (static_cast<uint16_t>(data_[0x151]) << 8);
    return static_cast<int16_t>(raw);
}

int16_t AmiiboBin::GetDefenseStat() const {
    uint16_t raw = static_cast<uint16_t>(data_[0x152]) |
                   (static_cast<uint16_t>(data_[0x153]) << 8);
    return static_cast<int16_t>(raw);
}

void AmiiboBin::SetStats(int16_t attack, int16_t defense, bool enforce_legal_cap) {
    if (enforce_legal_cap) {
        attack  = std::clamp<int16_t>(attack,  -2000, 5000);
        defense = std::clamp<int16_t>(defense, -2000, 5000);
        if (static_cast<int>(attack) + static_cast<int>(defense) > 5000) {
            int total = static_cast<int>(attack) + static_cast<int>(defense);
            attack  = static_cast<int16_t>((static_cast<int>(attack) * 5000) / total);
            defense = static_cast<int16_t>(5000 - attack);
        }
    }
    uint16_t u_atk = static_cast<uint16_t>(attack);
    uint16_t u_def = static_cast<uint16_t>(defense);
    data_[0x150] = static_cast<uint8_t>(u_atk & 0xFF);
    data_[0x151] = static_cast<uint8_t>((u_atk >> 8) & 0xFF);
    data_[0x152] = static_cast<uint8_t>(u_def & 0xFF);
    data_[0x153] = static_cast<uint8_t>((u_def >> 8) & 0xFF);
}

std::array<uint8_t, 3> AmiiboBin::GetSpiritSlots() const {
    return {data_[0xEC], data_[0xED], data_[0xEE]};
}

bool AmiiboBin::SetSpiritSlots(uint8_t slot1, uint8_t slot2, uint8_t slot3,
                               bool enforce_slot_cost,
                               std::string* error_out) {
    const auto* s1 = FindSupportAbilityById(slot1);
    const auto* s2 = FindSupportAbilityById(slot2);
    const auto* s3 = FindSupportAbilityById(slot3);
    if (!s1 || !s2 || !s3) {
        if (error_out) *error_out = "Spirit ability ID out of range (0..254).";
        return false;
    }
    if (enforce_slot_cost) {
        int cost = s1->slot_cost + s2->slot_cost + s3->slot_cost;
        if (cost > 3) {
            if (error_out) {
                std::ostringstream oss;
                oss << "Spirit slots exceed 3-slot limit (" << cost << " slots used: "
                    << s1->name << " [" << int(s1->slot_cost) << "] + "
                    << s2->name << " [" << int(s2->slot_cost) << "] + "
                    << s3->name << " [" << int(s3->slot_cost) << "]).";
                *error_out = oss.str();
            }
            return false;
        }
        // Per SSBU Amiibo Bin Guide: if Slot 1 uses 2 or 3 slots, shift remaining effects appropriately
        if (s1->slot_cost == 3) {
            slot2 = 0;
            slot3 = 0;
        } else if (s1->slot_cost == 2) {
            if (slot2 != 0 && slot3 == 0) {
                slot3 = slot2;
                slot2 = 0;
            }
        }
    }
    data_[0xEC] = slot1;
    data_[0xED] = slot2;
    data_[0xEE] = slot3;
    return true;
}

bool AmiiboBin::ApplyPrimarySpiritPreset(const PrimarySpiritPreset& preset) {
    SetSpiritType(preset.type);
    SetStats(preset.base_attack, preset.base_defense, true);
    return true;
}

// Bitfield reader matching MiDe's SSBU Amiibo Bin Guide:
// Multi-byte regions reverse byte order (little-endian) and extract the specified bit slices.
uint32_t AmiiboBin::ReadBits(size_t start_byte, int start_bit_in_first,
                             size_t end_byte, int end_bit_in_last) const {
    if (start_byte == end_byte) {
        int low  = std::min(start_bit_in_first, end_bit_in_last);
        int high = std::max(start_bit_in_first, end_bit_in_last);
        int width = high - low + 1;
        uint32_t mask = (1u << width) - 1u;
        return (data_[start_byte] >> low) & mask;
    }
    // Across 2 bytes: low bits from start_byte (0..start_bit_in_first), high bits from end_byte (end_bit_in_last..7)
    int width_first = start_bit_in_first + 1;
    uint32_t mask_first = (1u << width_first) - 1u;
    uint32_t part_first = data_[start_byte] & mask_first;

    int width_last = 7 - end_bit_in_last + 1;
    uint32_t mask_last = (1u << width_last) - 1u;
    uint32_t part_last = (data_[end_byte] >> end_bit_in_last) & mask_last;

    return (part_last << width_first) | part_first;
}

void AmiiboBin::WriteBits(size_t start_byte, int start_bit_in_first,
                          size_t end_byte, int end_bit_in_last, uint32_t value) {
    if (start_byte == end_byte) {
        int low  = std::min(start_bit_in_first, end_bit_in_last);
        int high = std::max(start_bit_in_first, end_bit_in_last);
        int width = high - low + 1;
        uint32_t mask = (1u << width) - 1u;
        data_[start_byte] = static_cast<uint8_t>(
            (data_[start_byte] & ~(mask << low)) | ((value & mask) << low));
        return;
    }
    int width_first = start_bit_in_first + 1;
    uint32_t mask_first = (1u << width_first) - 1u;
    uint32_t part_first = value & mask_first;
    data_[start_byte] = static_cast<uint8_t>((data_[start_byte] & ~mask_first) | part_first);

    int width_last = 7 - end_bit_in_last + 1;
    uint32_t mask_last = (1u << width_last) - 1u;
    uint32_t part_last = (value >> width_first) & mask_last;
    data_[end_byte] = static_cast<uint8_t>(
        (data_[end_byte] & ~(mask_last << end_bit_in_last)) | (part_last << end_bit_in_last));
}

AmiiboBehaviorParams AmiiboBin::DecodeBehavior() const {
    AmiiboBehaviorParams p{};
    auto rd_norm = [this](size_t sb, int sbit, size_t eb, int ebit, int bits) -> float {
        uint32_t max_v = (1u << bits) - 1u;
        uint32_t v = ReadBits(sb, sbit, eb, ebit);
        return static_cast<float>(v) / static_cast<float>(max_v);
    };

    p.near_range          = rd_norm(0x168, 7, 0x168, 1, 7);
    p.offensive           = rd_norm(0x168, 0, 0x169, 2, 7);
    p.grounded            = rd_norm(0x169, 1, 0x16A, 3, 7);
    p.attack_out_cliff    = rd_norm(0x16A, 2, 0x16B, 5, 6);
    p.dash                = rd_norm(0x16B, 4, 0x16C, 6, 7);
    p.return_to_cliff     = rd_norm(0x16C, 5, 0x16C, 0, 6);
    p.air_offensive       = rd_norm(0x16D, 7, 0x16D, 2, 6);
    p.cliffer             = rd_norm(0x16D, 1, 0x16E, 4, 6);
    p.feint_master        = rd_norm(0x16E, 3, 0x16F, 5, 7);
    p.feint_counter       = rd_norm(0x16F, 4, 0x170, 6, 7);
    p.feint_shooter       = rd_norm(0x170, 5, 0x171, 7, 7);
    p.catcher             = rd_norm(0x171, 6, 0x171, 0, 7);
    p._100_attacker       = rd_norm(0x172, 7, 0x172, 2, 6);
    p._100_keeper         = rd_norm(0x172, 1, 0x173, 4, 6);
    p.attack_cancel       = rd_norm(0x173, 3, 0x174, 6, 6);
    p.smash_holder        = rd_norm(0x174, 5, 0x175, 7, 7);
    p.dash_attacker       = rd_norm(0x175, 6, 0x175, 0, 7);
    p.critical_hitter     = rd_norm(0x176, 7, 0x176, 2, 6);
    p.meteor_master       = rd_norm(0x176, 1, 0x177, 4, 6);
    p.shield_master       = rd_norm(0x177, 3, 0x178, 5, 7);
    p.just_shield_master  = rd_norm(0x178, 4, 0x179, 7, 6);
    p.shield_catch_master = rd_norm(0x179, 6, 0x179, 1, 6);
    p.item_collector      = rd_norm(0x179, 0, 0x17A, 4, 5);
    p.item_throw_to_target= rd_norm(0x17A, 3, 0x17B, 7, 5);
    p.dragoon_collector   = rd_norm(0x17B, 6, 0x17B, 3, 4);
    p.smashball_collector = rd_norm(0x17B, 2, 0x17C, 7, 4);
    p.hammer_collector    = rd_norm(0x17C, 6, 0x17C, 3, 4);
    p.special_flagger     = rd_norm(0x17C, 2, 0x17D, 7, 4);
    p.item_swinger        = rd_norm(0x17D, 6, 0x17D, 2, 5);
    p.homerun_batter      = rd_norm(0x17D, 1, 0x17E, 6, 4);
    p.club_swinger        = rd_norm(0x17E, 5, 0x17E, 2, 4);
    p.death_swinger       = rd_norm(0x17E, 1, 0x17F, 6, 4);
    p.item_shooter        = rd_norm(0x17F, 5, 0x17F, 1, 5);
    p.carrier_broker      = rd_norm(0x17F, 0, 0x180, 4, 5);
    p.charger             = rd_norm(0x180, 3, 0x181, 7, 5);
    p.appeal              = rd_norm(0x181, 6, 0x181, 2, 5);

    // Grounded Moves (0x188 - 0x195)
    p.ftilt    = rd_norm(0x188, 1, 0x189, 0, 10) * 100.0f;
    p.utilt    = rd_norm(0x18A, 7, 0x18B, 6, 10) * 100.0f;
    p.dtilt    = rd_norm(0x18B, 5, 0x18C, 4, 10) * 100.0f;
    p.fsmash   = rd_norm(0x18C, 3, 0x18D, 2, 10) * 100.0f;
    p.usmash   = rd_norm(0x18D, 1, 0x18E, 0, 10) * 100.0f;
    p.dsmash   = rd_norm(0x18F, 7, 0x190, 6, 10) * 100.0f;
    p.nspecial = rd_norm(0x190, 5, 0x191, 4, 10) * 100.0f;
    p.sspecial = rd_norm(0x191, 3, 0x192, 2, 10) * 100.0f;
    p.uspecial = rd_norm(0x192, 1, 0x193, 0, 10) * 100.0f;
    p.dspecial = rd_norm(0x194, 7, 0x195, 6, 10) * 100.0f;

    // Aerial Moves (0x195 - 0x19E)
    p.fair         = rd_norm(0x195, 5, 0x196, 5, 9) * 100.0f;
    p.bair         = rd_norm(0x196, 4, 0x197, 4, 9) * 100.0f;
    p.uair         = rd_norm(0x197, 3, 0x198, 3, 9) * 100.0f;
    p.dair         = rd_norm(0x198, 2, 0x199, 2, 9) * 100.0f;
    p.air_nspecial = rd_norm(0x199, 1, 0x19A, 1, 9) * 100.0f;
    p.air_sspecial = rd_norm(0x19A, 0, 0x19B, 0, 9) * 100.0f;
    p.air_uspecial = rd_norm(0x19C, 7, 0x19D, 7, 9) * 100.0f;
    p.air_dspecial = rd_norm(0x19D, 6, 0x19E, 6, 9) * 100.0f;

    return p;
}

void AmiiboBin::EncodeBehavior(const AmiiboBehaviorParams& in_params) {
    AmiiboBehaviorParams p = in_params;
    p.NormalizeMoveSums();

    auto wr_norm = [this](size_t sb, int sbit, size_t eb, int ebit, int bits, float norm_val) {
        float clamped = std::clamp(norm_val, 0.0f, 1.0f);
        uint32_t max_v = (1u << bits) - 1u;
        uint32_t v = static_cast<uint32_t>(std::round(clamped * static_cast<float>(max_v)));
        WriteBits(sb, sbit, eb, ebit, v);
    };

    wr_norm(0x168, 7, 0x168, 1, 7, p.near_range);
    wr_norm(0x168, 0, 0x169, 2, 7, p.offensive);
    wr_norm(0x169, 1, 0x16A, 3, 7, p.grounded);
    wr_norm(0x16A, 2, 0x16B, 5, 6, p.attack_out_cliff);
    wr_norm(0x16B, 4, 0x16C, 6, 7, p.dash);
    wr_norm(0x16C, 5, 0x16C, 0, 6, p.return_to_cliff);
    wr_norm(0x16D, 7, 0x16D, 2, 6, p.air_offensive);
    wr_norm(0x16D, 1, 0x16E, 4, 6, p.cliffer);
    wr_norm(0x16E, 3, 0x16F, 5, 7, p.feint_master);
    wr_norm(0x16F, 4, 0x170, 6, 7, p.feint_counter);
    wr_norm(0x170, 5, 0x171, 7, 7, p.feint_shooter);
    wr_norm(0x171, 6, 0x171, 0, 7, p.catcher);
    wr_norm(0x172, 7, 0x172, 2, 6, p._100_attacker);
    wr_norm(0x172, 1, 0x173, 4, 6, p._100_keeper);
    wr_norm(0x173, 3, 0x174, 6, 6, p.attack_cancel);
    wr_norm(0x174, 5, 0x175, 7, 7, p.smash_holder);
    wr_norm(0x175, 6, 0x175, 0, 7, p.dash_attacker);
    wr_norm(0x176, 7, 0x176, 2, 6, p.critical_hitter);
    wr_norm(0x176, 1, 0x177, 4, 6, p.meteor_master);
    wr_norm(0x177, 3, 0x178, 5, 7, p.shield_master);
    wr_norm(0x178, 4, 0x179, 7, 6, p.just_shield_master);
    wr_norm(0x179, 6, 0x179, 1, 6, p.shield_catch_master);
    wr_norm(0x179, 0, 0x17A, 4, 5, p.item_collector);
    wr_norm(0x17A, 3, 0x17B, 7, 5, p.item_throw_to_target);
    wr_norm(0x17B, 6, 0x17B, 3, 4, p.dragoon_collector);
    wr_norm(0x17B, 2, 0x17C, 7, 4, p.smashball_collector);
    wr_norm(0x17C, 6, 0x17C, 3, 4, p.hammer_collector);
    wr_norm(0x17C, 2, 0x17D, 7, 4, p.special_flagger);
    wr_norm(0x17D, 6, 0x17D, 2, 5, p.item_swinger);
    wr_norm(0x17D, 1, 0x17E, 6, 4, p.homerun_batter);
    wr_norm(0x17E, 5, 0x17E, 2, 4, p.club_swinger);
    wr_norm(0x17E, 1, 0x17F, 6, 4, p.death_swinger);
    wr_norm(0x17F, 5, 0x17F, 1, 5, p.item_shooter);
    wr_norm(0x17F, 0, 0x180, 4, 5, p.carrier_broker);
    wr_norm(0x180, 3, 0x181, 7, 5, p.charger);
    wr_norm(0x181, 6, 0x181, 2, 5, p.appeal);

    // Grounded Moves
    wr_norm(0x188, 1, 0x189, 0, 10, p.ftilt / 100.0f);
    wr_norm(0x18A, 7, 0x18B, 6, 10, p.utilt / 100.0f);
    wr_norm(0x18B, 5, 0x18C, 4, 10, p.dtilt / 100.0f);
    wr_norm(0x18C, 3, 0x18D, 2, 10, p.fsmash / 100.0f);
    wr_norm(0x18D, 1, 0x18E, 0, 10, p.usmash / 100.0f);
    wr_norm(0x18F, 7, 0x190, 6, 10, p.dsmash / 100.0f);
    wr_norm(0x190, 5, 0x191, 4, 10, p.nspecial / 100.0f);
    wr_norm(0x191, 3, 0x192, 2, 10, p.sspecial / 100.0f);
    wr_norm(0x192, 1, 0x193, 0, 10, p.uspecial / 100.0f);
    wr_norm(0x194, 7, 0x195, 6, 10, p.dspecial / 100.0f);

    // Aerial Moves
    wr_norm(0x195, 5, 0x196, 5, 9, p.fair / 100.0f);
    wr_norm(0x196, 4, 0x197, 4, 9, p.bair / 100.0f);
    wr_norm(0x197, 3, 0x198, 3, 9, p.uair / 100.0f);
    wr_norm(0x198, 2, 0x199, 2, 9, p.dair / 100.0f);
    wr_norm(0x199, 1, 0x19A, 1, 9, p.air_nspecial / 100.0f);
    wr_norm(0x19A, 0, 0x19B, 0, 9, p.air_sspecial / 100.0f);
    wr_norm(0x19C, 7, 0x19D, 7, 9, p.air_uspecial / 100.0f);
    wr_norm(0x19D, 6, 0x19E, 6, 9, p.air_dspecial / 100.0f);
}

// Exact 25-state SSBU Personality Evaluator from jozz024/smash-amiibo-editor/resources/personality_data.json
std::string AmiiboBin::EvaluatePersonality() const {
    static const char* kPersonalityNames[25] = {
        "Normal",
        "Cautious", "Realistic", "Unflappable",
        "Light", "Quick", "Lightning Fast",
        "Aggressive", "Offensive", "Reckless",
        "Thrill Seeker", "Daredevil", "Unstoppable",
        "Technical", "Technician", "Ace",
        "Show-Off", "Flashy", "Entertainer",
        "Cool", "Logical", "Sly",
        "Gadabout", "Trickster", "Dynamic"
    };

    AmiiboBehaviorParams b = DecodeBehavior();

    auto score_param = [](float val, float min1, float pt1, float min2, float pt2, bool flip) -> float {
        float v = flip ? (1.0f - val) : val;
        if (v < min1) return 0.0f;
        if (v >= min2) return pt2;
        float t = (v - min1) / (min2 - min1);
        return pt1 + t * (pt2 - pt1);
    };

    // 1. def (Cautious / Realistic / Unflappable)
    float s_def = 0.0f;
    if ((1.0f - b.offensive) >= 0.26f) {
        s_def += score_param(b.offensive, 0.26f, 18, 0.76f, 20, true) * 2.2f;
        s_def += score_param(b.grounded, 0.25f, 18, 0.76f, 18, false) * 2.2f;
        s_def += score_param(b.shield_master, 0.25f, 38, 0.76f, 42, false) * 3.0f;
        s_def += score_param(b.dash, 0.51f, 18, 0.76f, 25, true) * 2.0f;
    }

    // 2. agl (Light / Quick / Lightning Fast)
    float s_agl = 0.0f;
    if (b.dash >= 0.25f) {
        s_agl += score_param(b.dash, 0.25f, 16, 0.76f, 40, false) * 3.2f;
        s_agl += score_param(b.dash_attacker, 0.25f, 13, 0.51f, 22, false) * 2.5f;
        s_agl += score_param(b.offensive, 0.25f, 11, 0.51f, 24, false) * 2.0f;
        s_agl += score_param(b.shield_master, 0.26f, 13, 0.76f, 30, true) * 2.0f;
    }

    // 3. ofn (Aggressive / Offensive / Reckless)
    float s_ofn = 0.0f;
    if (b.near_range >= 0.25f) {
        s_ofn += score_param(b.near_range, 0.25f, 15, 0.76f, 22, false) * 2.8f;
        s_ofn += score_param(b.offensive, 0.25f, 21, 0.76f, 30, false) * 3.0f;
        s_ofn += score_param(b.air_offensive, 0.26f, 10, 0.76f, 18, false) * 2.2f;
        s_ofn += score_param(b.dash_attacker, 0.25f, 10, 0.51f, 18, false) * 2.0f;
    }

    // 4. rsk (Thrill Seeker / Daredevil / Unstoppable)
    float s_rsk = 0.0f;
    if (b.attack_out_cliff >= 0.26f) {
        s_rsk += score_param(b.attack_out_cliff, 0.26f, 24, 0.76f, 28, false) * 3.2f;
        s_rsk += score_param(b.meteor_master, 0.26f, 12, 0.51f, 30, false) * 3.0f;
        s_rsk += score_param(b.offensive, 0.25f, 11, 0.76f, 16, false) * 2.0f;
        s_rsk += score_param(b.near_range, 0.25f, 15, 0.51f, 22, false) * 2.0f;
    }

    // 5. gen (Technical / Technician / Ace)
    float s_gen = 0.0f;
    if (b.just_shield_master >= 0.26f) {
        s_gen += score_param(b.just_shield_master, 0.26f, 23, 0.76f, 35, false) * 2.6f;
        s_gen += score_param(b.feint_master, 0.25f, 8, 0.76f, 13, false) * 2.2f;
        s_gen += score_param(b.feint_counter, 0.25f, 8, 0.76f, 13, false) * 2.2f;
        s_gen += score_param(b.catcher, 0.25f, 7, 0.76f, 9, false) * 2.0f;
    }

    // 6. ent (Show-Off / Flashy / Entertainer)
    float s_ent = 0.0f;
    if (b.appeal >= 0.25f) {
        s_ent += score_param(b.appeal, 0.25f, 18, 0.76f, 34, false) * 4.5f;
        s_ent += score_param(b.meteor_master, 0.26f, 7, 0.76f, 11, false) * 2.5f;
        s_ent += score_param(b.attack_out_cliff, 0.26f, 8, 0.76f, 12, false) * 2.5f;
    }

    // 7. cau (Cool / Logical / Sly)
    float s_cau = 0.0f;
    if ((1.0f - b.shield_master) >= 0.26f) {
        s_cau += score_param(b.shield_master, 0.26f, 29, 0.76f, 42, true) * 2.2f;
        s_cau += score_param(b.feint_shooter, 0.25f, 3, 0.76f, 6, false) * 4.0f;
        s_cau += score_param(b.grounded, 0.25f, 7, 0.51f, 12, false) * 3.0f;
    }

    // 8. dyn (Gadabout / Trickster / Dynamic)
    float s_dyn = 0.0f;
    s_dyn += score_param(b.critical_hitter, 0.26f, 12, 0.76f, 15, false) * 3.5f;
    s_dyn += score_param(b.smash_holder, 0.25f, 7, 0.76f, 9, false) * 3.5f;
    s_dyn += score_param(b.charger, 0.22f, 5, 0.74f, 6, false) * 3.5f;

    struct Branch { float score; float t1, t2, t3; int p1, p2, p3; };
    Branch branches[8] = {
        {s_def, 100, 180, 240, 1,  2,  3},
        {s_agl, 100, 170, 225, 4,  5,  6},
        {s_ofn, 100, 180, 235, 7,  8,  9},
        {s_rsk, 80,  160, 220, 10, 11, 12},
        {s_gen, 80,  120, 150, 13, 14, 15},
        {s_ent, 90,  170, 230, 16, 17, 18},
        {s_cau, 80,  160, 215, 19, 20, 21},
        {s_dyn, 80,  160, 210, 22, 23, 24}
    };

    int best_idx = 0;
    float best_score = 0.0f;
    for (const auto& br : branches) {
        if (br.score >= br.t1 && br.score > best_score) {
            best_score = br.score;
            if (br.score >= br.t3) best_idx = br.p3;
            else if (br.score >= br.t2) best_idx = br.p2;
            else best_idx = br.p1;
        }
    }
    return kPersonalityNames[best_idx];
}

void AmiiboBin::UpdateChecksumsAndCrypto() {
    // Increment 16-bit write counter at 0xB4..0xB5 (capped at 65535 per emuiibo spec)
    uint16_t wc = (static_cast<uint16_t>(data_[0xB4]) << 8) | data_[0xB5];
    if (wc < 0xFFFF) ++wc;
    data_[0xB4] = static_cast<uint8_t>((wc >> 8) & 0xFF);
    data_[0xB5] = static_cast<uint8_t>(wc & 0xFF);

    // Compute AppData & Tag HMAC-SHA256 signatures at 0x08..0x27 and 0xDC..0xDF
    const uint8_t default_key[16] = {
        0x53, 0x53, 0x42, 0x55, 0x5F, 0x41, 0x4D, 0x49,
        0x49, 0x42, 0x4F, 0x5F, 0x48, 0x4D, 0x41, 0x43
    };
    const uint8_t* key_ptr = has_retail_key_ ? retail_key_.data() : default_key;
    size_t key_len = has_retail_key_ ? retail_key_.size() : sizeof(default_key);

    uint8_t hmac_out[32]{};
    HmacSha256(key_ptr, key_len, data_.data() + 0xE0, 0xD4, hmac_out);
    // Store 4-byte AppData verification tag at 0xDC..0xDF (per SSBU Amiibo Bin Guide page 2)
    std::memcpy(data_.data() + 0xDC, hmac_out, 4);
    // Store 32-byte tag HMAC at 0x1B4..0x1D3
    std::memcpy(data_.data() + 0x1B4, hmac_out, 32);
}

} // namespace ssbu
