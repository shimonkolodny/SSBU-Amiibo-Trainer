#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "amiibo_bin.hpp"
#include "ssbu_loadouts.hpp"
#include "training_sim.hpp"

// Enable Common Controls v6 visual styles on Windows
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

namespace ssbu {

enum ControlId {
    ID_BTN_LOAD_BIN = 101,
    ID_BTN_NEW_BIN = 102,
    ID_BTN_SAVE_BIN = 103,
    ID_BTN_LOAD_KEYS = 104,
    ID_BTN_RESET_LV1 = 105,
    ID_BTN_MAX_LV50 = 106,

    ID_CMB_FIGHTER = 110,
    ID_CMB_COSTUME = 111,
    ID_CHK_LEARNING = 112,

    ID_EDT_SPIRIT_SEARCH = 120,
    ID_BTN_SPIRIT_FILTER = 121,
    ID_CMB_PRIMARY_SPIRIT = 122,
    ID_CMB_SUPPORT_1 = 123,
    ID_CMB_SUPPORT_2 = 124,
    ID_CMB_SUPPORT_3 = 125,
    ID_EDT_ATK = 126,
    ID_EDT_DEF = 127,
    ID_BTN_STAT_BALANCED = 128,
    ID_BTN_STAT_AGGRO = 129,
    ID_BTN_STAT_TANK = 130,
    ID_BTN_STAT_VANILLA = 131,
    ID_BTN_APPLY_SPIRITS = 132,
    ID_LBL_SLOT_METER = 133,

    ID_CMB_OPPONENT = 140,
    ID_CMB_ARCHETYPE = 141,
    ID_CMB_FOCUS = 142,
    ID_EDT_GAMES = 143,
    ID_BTN_GAMES_100 = 144,
    ID_BTN_GAMES_500 = 145,
    ID_BTN_GAMES_1000 = 146,
    ID_BTN_GAMES_5000 = 147,
    ID_BTN_TRAIN = 148,

    ID_EDT_TUNE_OFFENSE = 160,
    ID_EDT_TUNE_GROUNDED = 161,
    ID_EDT_TUNE_PARRY = 162,
    ID_EDT_TUNE_EDGEGUARD = 163,
    ID_EDT_TUNE_GRAB = 164,
    ID_EDT_TUNE_METEOR = 165,
    ID_BTN_APPLY_TUNING = 166,

    ID_BTN_VIEW_OVERVIEW = 180,
    ID_BTN_VIEW_BEHAVIOR = 181,
    ID_BTN_VIEW_LOG = 182,
    ID_EDT_REPORT = 185,
    ID_PROGRESS = 186,
    ID_LBL_STATUS = 187
};

struct GuiAppState {
    AmiiboBin bin;
    std::string current_path = "unloaded_amiibo.bin";
    std::string last_sim_log = "No training matches run yet. Configure your opponent and click 'RUN FULL TRAINING SIMULATION'.\r\n";
    int active_view = 0; // 0 = Overview, 1 = Full AI Behavior & Moves, 2 = Match Log

    HFONT font_regular = nullptr;
    HFONT font_bold = nullptr;
    HFONT font_mono = nullptr;

    HWND hwnd_fighter = nullptr;
    HWND hwnd_costume = nullptr;
    HWND hwnd_learning = nullptr;

    HWND hwnd_spirit_search = nullptr;
    HWND hwnd_primary = nullptr;
    HWND hwnd_sup1 = nullptr;
    HWND hwnd_sup2 = nullptr;
    HWND hwnd_sup3 = nullptr;
    HWND hwnd_atk = nullptr;
    HWND hwnd_def = nullptr;
    HWND hwnd_slot_meter = nullptr;

    HWND hwnd_opponent = nullptr;
    HWND hwnd_archetype = nullptr;
    HWND hwnd_focus = nullptr;
    HWND hwnd_games = nullptr;

    HWND hwnd_tune_offense = nullptr;
    HWND hwnd_tune_grounded = nullptr;
    HWND hwnd_tune_parry = nullptr;
    HWND hwnd_tune_edgeguard = nullptr;
    HWND hwnd_tune_grab = nullptr;
    HWND hwnd_tune_meteor = nullptr;

    HWND hwnd_report = nullptr;
    HWND hwnd_progress = nullptr;
    HWND hwnd_status = nullptr;
};

static GuiAppState g_app;

static std::string ToCrlf(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 256);
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r')) {
            out.push_back('\r');
        }
        out.push_back(text[i]);
    }
    return out;
}

static std::string BuildOverviewReport(const AmiiboBin& bin, const std::string& path) {
    const auto* fighter = FindFighterById(bin.GetCharacterId());
    std::ostringstream oss;
    oss << "====================================================================\n";
    oss << "         SUPER SMASH BROS. ULTIMATE AMIIBO INSPECTOR v2.0          \n";
    oss << "====================================================================\n";
    oss << "File Path          : " << path << "\n";
    oss << "Binary Format      : " << bin.GetFormatStatusString() << "\n";
    oss << "Fighter Identity   : " << (fighter ? fighter->name : "Unknown")
        << " (#" << (fighter ? fighter->roster_num : "??")
        << " | ID 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
        << bin.GetCharacterId() << std::dec << std::setfill(' ') << ")\n";
    oss << "Costume / Learning : Costume P" << static_cast<int>(bin.GetAlternateSkin() + 1)
        << " | AI Learning: " << (bin.GetLearningEnabled() ? "ON (Active)" : "OFF (Frozen)") << "\n";
    oss << "Amiibo Level       : Level " << bin.GetDisplayedLevel()
        << " (" << bin.GetLevelExperience() << " XP) | Internal CPU Level: " << bin.GetCpuLevel()
        << " (" << bin.GetCpuExperience() << " CPU XP)\n";
    oss << "Active Personality : " << bin.EvaluatePersonality() << "\n";
    oss << "Post-Match Gifts   : " << bin.GetGiftCount() << "\n\n";

    oss << "--- Equipped SSBU Spirits & Stats ---\n";
    oss << "  Spirit Alignment : " << SpiritTypeToString(bin.GetSpiritType()) << "\n";
    oss << "  Primary Stats    : Attack " << bin.GetAttackStat()
        << " / Defense " << bin.GetDefenseStat()
        << " (Total: " << (bin.GetAttackStat() + bin.GetDefenseStat()) << ")\n";
    oss << "  Support Slots    : " << bin.GetTotalSpiritSlotCost() << " / 3 Slots Used\n";

    auto slots = bin.GetSpiritSlots();
    for (int i = 0; i < 3; ++i) {
        const auto* ab = FindSupportAbilityById(slots[i]);
        oss << "    Slot " << (i + 1) << " [ID " << std::setw(3) << static_cast<int>(slots[i]) << "] : "
            << (ab ? ab->name : "None") << " (" << (ab ? static_cast<int>(ab->slot_cost) : 0) << "-Slot) - "
            << (ab ? ab->description : "") << "\n";
    }

    auto bp = bin.DecodeBehavior();
    oss << std::fixed << std::setprecision(1);
    oss << "\n--- Key Combat Behavior Snapshot ---\n";
    oss << "  Offensive        : " << std::setw(5) << (bp.offensive * 100.0f) << " %   | Grounded         : " << std::setw(5) << (bp.grounded * 100.0f) << " %\n";
    oss << "  Air Offense      : " << std::setw(5) << (bp.air_offensive * 100.0f) << " %   | Offstage Edgegrd : " << std::setw(5) << (bp.attack_out_cliff * 100.0f) << " %\n";
    oss << "  Parry (JustShld) : " << std::setw(5) << (bp.just_shield_master * 100.0f) << " %   | Grab (Catcher)   : " << std::setw(5) << (bp.catcher * 100.0f) << " %\n";
    oss << "  Meteor Master    : " << std::setw(5) << (bp.meteor_master * 100.0f) << " %   | Feint Master     : " << std::setw(5) << (bp.feint_master * 100.0f) << " %\n";
    return oss.str();
}

static std::string BuildBehaviorInspectorText(const AmiiboBin& bin) {
    AmiiboBehaviorParams bp = bin.DecodeBehavior();
    std::ostringstream oss;
    oss << "====================================================================\n";
    oss << "   UNPACKED SSBU AI BEHAVIOR & MOVE FREQUENCY TABLE (0x168 - 0x1A1)\n";
    oss << "====================================================================\n";
    oss << "Computed Active Personality : " << bin.EvaluatePersonality() << "\n\n";

    auto pct = [](float v) { return v * 100.0f; };
    oss << std::fixed << std::setprecision(1);
    oss << "--- 36 Core Combat & Positioning Weights (0x168 - 0x181) ---\n";
    oss << "  Near Range          : " << std::setw(5) << pct(bp.near_range) << " %   | Offensive           : " << std::setw(5) << pct(bp.offensive) << " %\n";
    oss << "  Grounded            : " << std::setw(5) << pct(bp.grounded) << " %   | Attack Out of Cliff : " << std::setw(5) << pct(bp.attack_out_cliff) << " %\n";
    oss << "  Dash Frequency      : " << std::setw(5) << pct(bp.dash) << " %   | Return to Cliff     : " << std::setw(5) << pct(bp.return_to_cliff) << " %\n";
    oss << "  Air Offensive       : " << std::setw(5) << pct(bp.air_offensive) << " %   | Cliffer (Ledge)     : " << std::setw(5) << pct(bp.cliffer) << " %\n";
    oss << "  Feint Master        : " << std::setw(5) << pct(bp.feint_master) << " %   | Feint Counter       : " << std::setw(5) << pct(bp.feint_counter) << " %\n";
    oss << "  Feint Shooter       : " << std::setw(5) << pct(bp.feint_shooter) << " %   | Catcher (Grab)      : " << std::setw(5) << pct(bp.catcher) << " %\n";
    oss << "  100% Attacker       : " << std::setw(5) << pct(bp._100_attacker) << " %   | 100% Keeper         : " << std::setw(5) << pct(bp._100_keeper) << " %\n";
    oss << "  Attack Cancel       : " << std::setw(5) << pct(bp.attack_cancel) << " %   | Smash Holder        : " << std::setw(5) << pct(bp.smash_holder) << " %\n";
    oss << "  Dash Attacker       : " << std::setw(5) << pct(bp.dash_attacker) << " %   | Critical Hitter     : " << std::setw(5) << pct(bp.critical_hitter) << " %\n";
    oss << "  Meteor Master       : " << std::setw(5) << pct(bp.meteor_master) << " %   | Shield Master       : " << std::setw(5) << pct(bp.shield_master) << " %\n";
    oss << "  Just Shield (Parry) : " << std::setw(5) << pct(bp.just_shield_master) << " %   | Shield Catch (OoS)  : " << std::setw(5) << pct(bp.shield_catch_master) << " %\n";
    oss << "  Charger             : " << std::setw(5) << pct(bp.charger) << " %   | Appeal (Taunt)      : " << std::setw(5) << pct(bp.appeal) << " %\n\n";

    oss << "--- Grounded Move Distribution (0x188 - 0x195, Total = 100%) ---\n";
    oss << "  Jab (Implicit)      : " << std::setw(5) << bp.ImplicitJabPercent() << " %   | Forward Tilt        : " << std::setw(5) << bp.ftilt << " %\n";
    oss << "  Up Tilt             : " << std::setw(5) << bp.utilt << " %   | Down Tilt           : " << std::setw(5) << bp.dtilt << " %\n";
    oss << "  Forward Smash       : " << std::setw(5) << bp.fsmash << " %   | Up Smash            : " << std::setw(5) << bp.usmash << " %\n";
    oss << "  Down Smash          : " << std::setw(5) << bp.dsmash << " %   | Neutral Special (G) : " << std::setw(5) << bp.nspecial << " %\n";
    oss << "  Side Special (G)    : " << std::setw(5) << bp.sspecial << " %   | Up Special (G)      : " << std::setw(5) << bp.uspecial << " %\n";
    oss << "  Down Special (G)    : " << std::setw(5) << bp.dspecial << " %\n\n";

    oss << "--- Aerial Move Distribution (0x195 - 0x19E, Total = 100%) ---\n";
    oss << "  Neutral Air (Nair)  : " << std::setw(5) << bp.ImplicitNairPercent() << " %   | Forward Air (Fair)  : " << std::setw(5) << bp.fair << " %\n";
    oss << "  Back Air (Bair)     : " << std::setw(5) << bp.bair << " %   | Up Air (Uair)       : " << std::setw(5) << bp.uair << " %\n";
    oss << "  Down Air (Dair)     : " << std::setw(5) << bp.dair << " %   | Neutral Special (A) : " << std::setw(5) << bp.air_nspecial << " %\n";
    oss << "  Side Special (A)    : " << std::setw(5) << bp.air_sspecial << " %   | Up Special (A)      : " << std::setw(5) << bp.air_uspecial << " %\n";
    oss << "  Down Special (A)    : " << std::setw(5) << bp.air_dspecial << " %\n\n";

    oss << "--- Directional Air Dodge & Taunt Split (0x19E - 0x1A1) ---\n";
    oss << "  Neutral Air Dodge   : " << std::setw(5) << bp.ImplicitNeutralAirDodgePercent() << " %   | Forward Air Dodge   : " << std::setw(5) << bp.escape_air_forward << " %\n";
    oss << "  Back Air Dodge      : " << std::setw(5) << bp.escape_air_backward << " %   | Up / Down Taunt     : " << std::setw(5) << bp.appeal_hi << " % / " << bp.appeal_lw << " %\n";
    return oss.str();
}

static void RefreshInspectorView() {
    std::string content;
    if (g_app.active_view == 0) {
        content = BuildOverviewReport(g_app.bin, g_app.current_path);
    } else if (g_app.active_view == 1) {
        content = BuildBehaviorInspectorText(g_app.bin);
    } else {
        content = g_app.last_sim_log;
    }
    SetWindowTextA(g_app.hwnd_report, ToCrlf(content).c_str());
}

static uint8_t GetSelectedSupportSpiritId(HWND combo) {
    LRESULT sel = SendMessageA(combo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return 0;
    LRESULT data = SendMessageA(combo, CB_GETITEMDATA, static_cast<WPARAM>(sel), 0);
    if (data == CB_ERR) return 0;
    return static_cast<uint8_t>(data & 0xFF);
}

static void SelectSupportSpiritById(HWND combo, uint8_t id) {
    LRESULT count = SendMessageA(combo, CB_GETCOUNT, 0, 0);
    for (LRESULT i = 0; i < count; ++i) {
        LRESULT data = SendMessageA(combo, CB_GETITEMDATA, static_cast<WPARAM>(i), 0);
        if (static_cast<uint8_t>(data & 0xFF) == id) {
            SendMessageA(combo, CB_SETCURSEL, static_cast<WPARAM>(i), 0);
            return;
        }
    }
    SendMessageA(combo, CB_SETCURSEL, 0, 0);
}

static void UpdateSlotMeterLabel() {
    uint8_t s1 = GetSelectedSupportSpiritId(g_app.hwnd_sup1);
    uint8_t s2 = GetSelectedSupportSpiritId(g_app.hwnd_sup2);
    uint8_t s3 = GetSelectedSupportSpiritId(g_app.hwnd_sup3);
    const auto* a1 = FindSupportAbilityById(s1);
    const auto* a2 = FindSupportAbilityById(s2);
    const auto* a3 = FindSupportAbilityById(s3);
    int used = (a1 ? a1->slot_cost : 0) + (a2 ? a2->slot_cost : 0) + (a3 ? a3->slot_cost : 0);
    char buf[96];
    std::snprintf(buf, sizeof(buf), "Support Slots Used: %d / 3 %s",
                  used, (used > 3) ? "(Exceeds 3-slot limit!)" : "(Valid)");
    SetWindowTextA(g_app.hwnd_slot_meter, buf);
}

static void PopulateSupportComboboxes(const std::string& filter_query) {
    uint8_t prev1 = GetSelectedSupportSpiritId(g_app.hwnd_sup1);
    uint8_t prev2 = GetSelectedSupportSpiritId(g_app.hwnd_sup2);
    uint8_t prev3 = GetSelectedSupportSpiritId(g_app.hwnd_sup3);

    HWND combos[3] = {g_app.hwnd_sup1, g_app.hwnd_sup2, g_app.hwnd_sup3};
    for (HWND cb : combos) {
        SendMessageA(cb, CB_RESETCONTENT, 0, 0);
    }

    auto spirits = GetSelectableSupportSpirits(false, filter_query);
    if (spirits.empty() || spirits.front()->id != 0) {
        const auto* none_sp = FindSupportAbilityById(0);
        if (none_sp) spirits.insert(spirits.begin(), none_sp);
    }

    for (const auto* sp : spirits) {
        std::ostringstream label;
        label << "[" << static_cast<int>(sp->id) << "] " << sp->name
              << " (" << static_cast<int>(sp->slot_cost) << "-Slot) - " << sp->description;
        std::string text = label.str();
        for (HWND cb : combos) {
            LRESULT idx = SendMessageA(cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
            if (idx != CB_ERR) {
                SendMessageA(cb, CB_SETITEMDATA, static_cast<WPARAM>(idx), static_cast<LPARAM>(sp->id));
            }
        }
    }

    SelectSupportSpiritById(g_app.hwnd_sup1, prev1);
    SelectSupportSpiritById(g_app.hwnd_sup2, prev2);
    SelectSupportSpiritById(g_app.hwnd_sup3, prev3);
    UpdateSlotMeterLabel();
}

static void SyncControlsFromBin() {
    const auto& roster = GetFighterCatalog();
    uint16_t char_id = g_app.bin.GetCharacterId();
    for (size_t i = 0; i < roster.size(); ++i) {
        if (roster[i].char_id == char_id) {
            SendMessageA(g_app.hwnd_fighter, CB_SETCURSEL, i, 0);
            break;
        }
    }

    SendMessageA(g_app.hwnd_costume, CB_SETCURSEL, g_app.bin.GetAlternateSkin() % 8, 0);
    SendMessageA(g_app.hwnd_learning, BM_SETCHECK, g_app.bin.GetLearningEnabled() ? BST_CHECKED : BST_UNCHECKED, 0);

    char atk_buf[32], def_buf[32];
    std::snprintf(atk_buf, sizeof(atk_buf), "%d", static_cast<int>(g_app.bin.GetAttackStat()));
    std::snprintf(def_buf, sizeof(def_buf), "%d", static_cast<int>(g_app.bin.GetDefenseStat()));
    SetWindowTextA(g_app.hwnd_atk, atk_buf);
    SetWindowTextA(g_app.hwnd_def, def_buf);

    auto slots = g_app.bin.GetLogicalSpiritList();
    SelectSupportSpiritById(g_app.hwnd_sup1, slots[0]);
    SelectSupportSpiritById(g_app.hwnd_sup2, slots[1]);
    SelectSupportSpiritById(g_app.hwnd_sup3, slots[2]);
    UpdateSlotMeterLabel();

    AmiiboBehaviorParams bp = g_app.bin.DecodeBehavior();
    auto set_pct = [](HWND h, float val) {
        char b[32];
        std::snprintf(b, sizeof(b), "%d", static_cast<int>(std::round(val * 100.0f)));
        SetWindowTextA(h, b);
    };
    set_pct(g_app.hwnd_tune_offense, bp.offensive);
    set_pct(g_app.hwnd_tune_grounded, bp.grounded);
    set_pct(g_app.hwnd_tune_parry, bp.just_shield_master);
    set_pct(g_app.hwnd_tune_edgeguard, bp.attack_out_cliff);
    set_pct(g_app.hwnd_tune_grab, bp.catcher);
    set_pct(g_app.hwnd_tune_meteor, bp.meteor_master);

    RefreshInspectorView();
}

static void ApplySelectedFighterAndSpirits(HWND hwnd) {
    const auto& roster = GetFighterCatalog();
    LRESULT f_idx = SendMessageA(g_app.hwnd_fighter, CB_GETCURSEL, 0, 0);
    if (f_idx != CB_ERR && static_cast<size_t>(f_idx) < roster.size()) {
        g_app.bin.SetCharacterId(roster[f_idx].char_id);
    }

    LRESULT c_idx = SendMessageA(g_app.hwnd_costume, CB_GETCURSEL, 0, 0);
    if (c_idx != CB_ERR) {
        g_app.bin.SetAlternateSkin(static_cast<uint8_t>(c_idx));
    }

    bool learning = (SendMessageA(g_app.hwnd_learning, BM_GETCHECK, 0, 0) == BST_CHECKED);
    g_app.bin.SetLearningEnabled(learning);

    const auto& primaries = GetPrimarySpiritCatalog();
    LRESULT p_idx = SendMessageA(g_app.hwnd_primary, CB_GETCURSEL, 0, 0);
    if (p_idx != CB_ERR && static_cast<size_t>(p_idx) < primaries.size()) {
        g_app.bin.SetSpiritType(primaries[p_idx].type);
    }

    char atk_buf[32] = {0}, def_buf[32] = {0};
    GetWindowTextA(g_app.hwnd_atk, atk_buf, sizeof(atk_buf));
    GetWindowTextA(g_app.hwnd_def, def_buf, sizeof(def_buf));
    int atk = std::clamp(std::atoi(atk_buf), -2000, 5000);
    int def = std::clamp(std::atoi(def_buf), -2000, 5000);
    g_app.bin.SetStats(static_cast<int16_t>(atk), static_cast<int16_t>(def), false);

    uint8_t s1 = GetSelectedSupportSpiritId(g_app.hwnd_sup1);
    uint8_t s2 = GetSelectedSupportSpiritId(g_app.hwnd_sup2);
    uint8_t s3 = GetSelectedSupportSpiritId(g_app.hwnd_sup3);

    std::string slot_err;
    if (!g_app.bin.SetSpiritSlots(s1, s2, s3, true, &slot_err)) {
        MessageBoxA(hwnd, slot_err.c_str(), "Support Spirit Slot Limit (Max 3 Slots)", MB_ICONWARNING);
    }
    SyncControlsFromBin();
}

static void SetFontForChildren(HWND parent, HFONT font) {
    HWND child = GetWindow(parent, GW_CHILD);
    while (child) {
        SendMessageA(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            INITCOMMONCONTROLSEX icex{};
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;
            InitCommonControlsEx(&icex);

            g_app.font_regular = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            g_app.font_bold = CreateFontA(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            g_app.font_mono = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                FIXED_PITCH | FF_MODERN, "Consolas");

            g_app.bin.InitializeBlankFighter(0x0000, 0);
            g_app.bin.TryAutoLoadRetailKey();

            // --- Group 1: Amiibo File, Fighter Identity & Crypto Keys ---
            HWND grp1 = CreateWindowA("BUTTON", "1. Amiibo File (.bin), Fighter Identity & Retail Crypto Keys",
                WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 14, 10, 490, 142, hwnd, nullptr, nullptr, nullptr);

            CreateWindowA("BUTTON", "Load .bin File...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                26, 32, 110, 28, hwnd, reinterpret_cast<HMENU>(ID_BTN_LOAD_BIN), nullptr, nullptr);
            CreateWindowA("BUTTON", "New Blank .bin", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                142, 32, 110, 28, hwnd, reinterpret_cast<HMENU>(ID_BTN_NEW_BIN), nullptr, nullptr);
            CreateWindowA("BUTTON", "Save Trained .bin...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                258, 32, 125, 28, hwnd, reinterpret_cast<HMENU>(ID_BTN_SAVE_BIN), nullptr, nullptr);
            CreateWindowA("BUTTON", "Load key_retail...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                389, 32, 105, 28, hwnd, reinterpret_cast<HMENU>(ID_BTN_LOAD_KEYS), nullptr, nullptr);

            CreateWindowA("STATIC", "Amiibo Fighter:", WS_VISIBLE | WS_CHILD, 26, 70, 92, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_fighter = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                120, 66, 235, 320, hwnd, reinterpret_cast<HMENU>(ID_CMB_FIGHTER), nullptr, nullptr);

            CreateWindowA("STATIC", "Alt:", WS_VISIBLE | WS_CHILD, 362, 70, 26, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_costume = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                390, 66, 104, 200, hwnd, reinterpret_cast<HMENU>(ID_CMB_COSTUME), nullptr, nullptr);
            for (int c = 1; c <= 8; ++c) {
                char cbuf[24];
                std::snprintf(cbuf, sizeof(cbuf), "Costume P%d", c);
                SendMessageA(g_app.hwnd_costume, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(cbuf));
            }

            g_app.hwnd_learning = CreateWindowA("BUTTON", "Enable In-Game AI Learning",
                WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 26, 108, 195, 24,
                hwnd, reinterpret_cast<HMENU>(ID_CHK_LEARNING), nullptr, nullptr);

            CreateWindowA("BUTTON", "Reset Lv 1", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                235, 106, 120, 26, hwnd, reinterpret_cast<HMENU>(ID_BTN_RESET_LV1), nullptr, nullptr);
            CreateWindowA("BUTTON", "Instant Lv 50", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                365, 106, 129, 26, hwnd, reinterpret_cast<HMENU>(ID_BTN_MAX_LV50), nullptr, nullptr);

            // --- Group 2: SSBU Spirit Collection & Stat Loadout ---
            HWND grp2 = CreateWindowA("BUTTON", "2. SSBU Spirit Collection (Primary & 255 Support Spirit Skills)",
                WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 14, 158, 490, 250, hwnd, nullptr, nullptr, nullptr);

            CreateWindowA("STATIC", "Primary Preset:", WS_VISIBLE | WS_CHILD, 26, 182, 95, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_primary = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                125, 178, 368, 320, hwnd, reinterpret_cast<HMENU>(ID_CMB_PRIMARY_SPIRIT), nullptr, nullptr);

            CreateWindowA("STATIC", "ATK / DEF:", WS_VISIBLE | WS_CHILD, 26, 214, 80, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_atk = CreateWindowA("EDIT", "0", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                105, 211, 56, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_ATK), nullptr, nullptr);
            g_app.hwnd_def = CreateWindowA("EDIT", "0", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                166, 211, 56, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_DEF), nullptr, nullptr);

            CreateWindowA("BUTTON", "2500/2500", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                228, 210, 68, 24, hwnd, reinterpret_cast<HMENU>(ID_BTN_STAT_BALANCED), nullptr, nullptr);
            CreateWindowA("BUTTON", "3200/1800", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                299, 210, 68, 24, hwnd, reinterpret_cast<HMENU>(ID_BTN_STAT_AGGRO), nullptr, nullptr);
            CreateWindowA("BUTTON", "1800/3200", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                370, 210, 68, 24, hwnd, reinterpret_cast<HMENU>(ID_BTN_STAT_TANK), nullptr, nullptr);
            CreateWindowA("BUTTON", "0/0", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                441, 210, 52, 24, hwnd, reinterpret_cast<HMENU>(ID_BTN_STAT_VANILLA), nullptr, nullptr);

            CreateWindowA("STATIC", "Filter Supports:", WS_VISIBLE | WS_CHILD, 26, 246, 95, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_spirit_search = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER,
                125, 243, 275, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_SPIRIT_SEARCH), nullptr, nullptr);
            CreateWindowA("BUTTON", "Filter List", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                406, 242, 87, 24, hwnd, reinterpret_cast<HMENU>(ID_BTN_SPIRIT_FILTER), nullptr, nullptr);

            CreateWindowA("STATIC", "Support Slot 1:", WS_VISIBLE | WS_CHILD, 26, 276, 95, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_sup1 = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                125, 272, 368, 320, hwnd, reinterpret_cast<HMENU>(ID_CMB_SUPPORT_1), nullptr, nullptr);

            CreateWindowA("STATIC", "Support Slot 2:", WS_VISIBLE | WS_CHILD, 26, 306, 95, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_sup2 = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                125, 302, 368, 320, hwnd, reinterpret_cast<HMENU>(ID_CMB_SUPPORT_2), nullptr, nullptr);

            CreateWindowA("STATIC", "Support Slot 3:", WS_VISIBLE | WS_CHILD, 26, 336, 95, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_sup3 = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                125, 332, 368, 320, hwnd, reinterpret_cast<HMENU>(ID_CMB_SUPPORT_3), nullptr, nullptr);

            g_app.hwnd_slot_meter = CreateWindowA("STATIC", "Support Slots Used: 0 / 3 (Valid)",
                WS_VISIBLE | WS_CHILD, 26, 370, 260, 22, hwnd, reinterpret_cast<HMENU>(ID_LBL_SLOT_METER), nullptr, nullptr);
            CreateWindowA("BUTTON", "Equip Selected Spirits & Stats", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                292, 366, 201, 28, hwnd, reinterpret_cast<HMENU>(ID_BTN_APPLY_SPIRITS), nullptr, nullptr);

            // --- Group 3: Multi-Hundred Match Combat Simulator ---
            HWND grp3 = CreateWindowA("BUTTON", "3. SSBU AI Combat Simulator (Multi-Hundred Game Training)",
                WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 14, 414, 490, 176, hwnd, nullptr, nullptr, nullptr);

            CreateWindowA("STATIC", "Sparring Rival:", WS_VISIBLE | WS_CHILD, 26, 438, 95, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_opponent = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                125, 434, 368, 320, hwnd, reinterpret_cast<HMENU>(ID_CMB_OPPONENT), nullptr, nullptr);

            CreateWindowA("STATIC", "Opponent Style:", WS_VISIBLE | WS_CHILD, 26, 466, 95, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_archetype = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                125, 462, 175, 220, hwnd, reinterpret_cast<HMENU>(ID_CMB_ARCHETYPE), nullptr, nullptr);

            CreateWindowA("STATIC", "Focus:", WS_VISIBLE | WS_CHILD, 306, 466, 42, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_focus = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                350, 462, 143, 220, hwnd, reinterpret_cast<HMENU>(ID_CMB_FOCUS), nullptr, nullptr);

            CreateWindowA("STATIC", "Match Count:", WS_VISIBLE | WS_CHILD, 26, 498, 95, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_games = CreateWindowA("EDIT", "500", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                125, 495, 75, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_GAMES), nullptr, nullptr);

            CreateWindowA("BUTTON", "100", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                210, 494, 64, 24, hwnd, reinterpret_cast<HMENU>(ID_BTN_GAMES_100), nullptr, nullptr);
            CreateWindowA("BUTTON", "500", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                278, 494, 64, 24, hwnd, reinterpret_cast<HMENU>(ID_BTN_GAMES_500), nullptr, nullptr);
            CreateWindowA("BUTTON", "1,000", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                346, 494, 68, 24, hwnd, reinterpret_cast<HMENU>(ID_BTN_GAMES_1000), nullptr, nullptr);
            CreateWindowA("BUTTON", "5,000", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                418, 494, 75, 24, hwnd, reinterpret_cast<HMENU>(ID_BTN_GAMES_5000), nullptr, nullptr);

            CreateWindowA("BUTTON", "RUN FULL TRAINING SIMULATION (TRAIN TO LV 50 READY .BIN)",
                WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                26, 526, 467, 34, hwnd, reinterpret_cast<HMENU>(ID_BTN_TRAIN), nullptr, nullptr);

            // --- Group 4: Direct AI Behavior Fine-Tuning ---
            HWND grp4 = CreateWindowA("BUTTON", "4. Direct AI Behavior Fine-Tuning (0 - 100% Weight Overrides)",
                WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 14, 594, 490, 96, hwnd, nullptr, nullptr, nullptr);

            CreateWindowA("STATIC", "Offense %:", WS_VISIBLE | WS_CHILD, 26, 618, 68, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_tune_offense = CreateWindowA("EDIT", "50", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                96, 615, 48, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_TUNE_OFFENSE), nullptr, nullptr);

            CreateWindowA("STATIC", "Ground %:", WS_VISIBLE | WS_CHILD, 154, 618, 64, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_tune_grounded = CreateWindowA("EDIT", "50", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                220, 615, 48, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_TUNE_GROUNDED), nullptr, nullptr);

            CreateWindowA("STATIC", "Parry %:", WS_VISIBLE | WS_CHILD, 278, 618, 58, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_tune_parry = CreateWindowA("EDIT", "25", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                338, 615, 48, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_TUNE_PARRY), nullptr, nullptr);

            CreateWindowA("STATIC", "Edgegrd %:", WS_VISIBLE | WS_CHILD, 26, 648, 68, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_tune_edgeguard = CreateWindowA("EDIT", "40", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                96, 645, 48, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_TUNE_EDGEGUARD), nullptr, nullptr);

            CreateWindowA("STATIC", "Grab %:", WS_VISIBLE | WS_CHILD, 154, 648, 64, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_tune_grab = CreateWindowA("EDIT", "30", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                220, 645, 48, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_TUNE_GRAB), nullptr, nullptr);

            CreateWindowA("STATIC", "Meteor %:", WS_VISIBLE | WS_CHILD, 278, 648, 58, 20, hwnd, nullptr, nullptr, nullptr);
            g_app.hwnd_tune_meteor = CreateWindowA("EDIT", "35", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                338, 645, 48, 22, hwnd, reinterpret_cast<HMENU>(ID_EDT_TUNE_METEOR), nullptr, nullptr);

            CreateWindowA("BUTTON", "Apply Fine-Tune", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                396, 615, 97, 52, hwnd, reinterpret_cast<HMENU>(ID_BTN_APPLY_TUNING), nullptr, nullptr);

            // --- Right Panel: Multi-Tab Live Amiibo Inspector ---
            CreateWindowA("BUTTON", "View 1: Overview & Spirits", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                518, 14, 168, 28, hwnd, reinterpret_cast<HMENU>(ID_BTN_VIEW_OVERVIEW), nullptr, nullptr);
            CreateWindowA("BUTTON", "View 2: Full AI Behavior & Moves", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                692, 14, 185, 28, hwnd, reinterpret_cast<HMENU>(ID_BTN_VIEW_BEHAVIOR), nullptr, nullptr);
            CreateWindowA("BUTTON", "View 3: Match Simulation Log", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                883, 14, 165, 28, hwnd, reinterpret_cast<HMENU>(ID_BTN_VIEW_LOG), nullptr, nullptr);

            g_app.hwnd_report = CreateWindowA("EDIT", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                518, 48, 530, 605, hwnd, reinterpret_cast<HMENU>(ID_EDT_REPORT), nullptr, nullptr);

            g_app.hwnd_progress = CreateWindowA(PROGRESS_CLASSA, nullptr,
                WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
                518, 662, 530, 22, hwnd, reinterpret_cast<HMENU>(ID_PROGRESS), nullptr, nullptr);
            SendMessageA(g_app.hwnd_progress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

            g_app.hwnd_status = CreateWindowA("STATIC",
                "Ready. All 89 SSBU Fighters, 65+ Primary Spirits, and 190+ Active Support Spirits loaded.",
                WS_VISIBLE | WS_CHILD, 14, 696, 1034, 22, hwnd, reinterpret_cast<HMENU>(ID_LBL_STATUS), nullptr, nullptr);

            SetFontForChildren(hwnd, g_app.font_regular);
            SendMessageA(grp1, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.font_bold), TRUE);
            SendMessageA(grp2, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.font_bold), TRUE);
            SendMessageA(grp3, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.font_bold), TRUE);
            SendMessageA(grp4, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.font_bold), TRUE);
            SendMessageA(g_app.hwnd_report, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.font_mono), TRUE);

            // Populate Fighter Roster (all 89 fighters)
            const auto& roster = GetFighterCatalog();
            for (const auto& f : roster) {
                char fbuf[96];
                std::snprintf(fbuf, sizeof(fbuf), "#%s - %s", f.roster_num, f.name);
                SendMessageA(g_app.hwnd_fighter, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fbuf));
                SendMessageA(g_app.hwnd_opponent, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fbuf));
            }
            SendMessageA(g_app.hwnd_fighter, CB_SETCURSEL, 0, 0);
            SendMessageA(g_app.hwnd_opponent, CB_SETCURSEL, 7, 0); // Fox default

            // Populate Primary Spirits
            const auto& primaries = GetPrimarySpiritCatalog();
            for (const auto& p : primaries) {
                std::ostringstream oss;
                oss << p.name << " [" << p.rank << " | ATK " << p.base_attack
                    << " / DEF " << p.base_defense << " | " << static_cast<int>(p.slots) << " Slots]";
                std::string s = oss.str();
                SendMessageA(g_app.hwnd_primary, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s.c_str()));
            }
            SendMessageA(g_app.hwnd_primary, CB_SETCURSEL, 0, 0);

            // Populate Selectable Support Spirits
            PopulateSupportComboboxes("");

            // Populate Opponent Archetypes & Training Focus
            for (int a = 0; a <= 6; ++a) {
                const char* aname = TrainingSimulator::ArchetypeToString(static_cast<OpponentArchetype>(a));
                SendMessageA(g_app.hwnd_archetype, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(aname));
            }
            SendMessageA(g_app.hwnd_archetype, CB_SETCURSEL, 0, 0);

            for (int f = 0; f <= 6; ++f) {
                const char* fname = TrainingSimulator::FocusToString(static_cast<TrainingFocus>(f));
                SendMessageA(g_app.hwnd_focus, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fname));
            }
            SendMessageA(g_app.hwnd_focus, CB_SETCURSEL, 0, 0);

            SyncControlsFromBin();
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == ID_CMB_PRIMARY_SPIRIT && code == CBN_SELCHANGE) {
                LRESULT idx = SendMessageA(g_app.hwnd_primary, CB_GETCURSEL, 0, 0);
                const auto& primaries = GetPrimarySpiritCatalog();
                if (idx != CB_ERR && static_cast<size_t>(idx) < primaries.size()) {
                    char abuf[32], dbuf[32];
                    std::snprintf(abuf, sizeof(abuf), "%d", static_cast<int>(primaries[idx].base_attack));
                    std::snprintf(dbuf, sizeof(dbuf), "%d", static_cast<int>(primaries[idx].base_defense));
                    SetWindowTextA(g_app.hwnd_atk, abuf);
                    SetWindowTextA(g_app.hwnd_def, dbuf);
                }
            } else if ((id == ID_CMB_SUPPORT_1 || id == ID_CMB_SUPPORT_2 || id == ID_CMB_SUPPORT_3) && code == CBN_SELCHANGE) {
                UpdateSlotMeterLabel();
            } else if (id == ID_BTN_SPIRIT_FILTER) {
                char qbuf[128] = {0};
                GetWindowTextA(g_app.hwnd_spirit_search, qbuf, sizeof(qbuf));
                PopulateSupportComboboxes(qbuf);
                SetWindowTextA(g_app.hwnd_status, "Support Spirit dropdowns filtered by search query.");
            } else if (id == ID_BTN_STAT_BALANCED) {
                SetWindowTextA(g_app.hwnd_atk, "2500");
                SetWindowTextA(g_app.hwnd_def, "2500");
            } else if (id == ID_BTN_STAT_AGGRO) {
                SetWindowTextA(g_app.hwnd_atk, "3200");
                SetWindowTextA(g_app.hwnd_def, "1800");
            } else if (id == ID_BTN_STAT_TANK) {
                SetWindowTextA(g_app.hwnd_atk, "1800");
                SetWindowTextA(g_app.hwnd_def, "3200");
            } else if (id == ID_BTN_STAT_VANILLA) {
                SetWindowTextA(g_app.hwnd_atk, "0");
                SetWindowTextA(g_app.hwnd_def, "0");
            } else if (id == ID_BTN_GAMES_100) {
                SetWindowTextA(g_app.hwnd_games, "100");
            } else if (id == ID_BTN_GAMES_500) {
                SetWindowTextA(g_app.hwnd_games, "500");
            } else if (id == ID_BTN_GAMES_1000) {
                SetWindowTextA(g_app.hwnd_games, "1000");
            } else if (id == ID_BTN_GAMES_5000) {
                SetWindowTextA(g_app.hwnd_games, "5000");
            } else if (id == ID_BTN_VIEW_OVERVIEW) {
                g_app.active_view = 0;
                RefreshInspectorView();
            } else if (id == ID_BTN_VIEW_BEHAVIOR) {
                g_app.active_view = 1;
                RefreshInspectorView();
            } else if (id == ID_BTN_VIEW_LOG) {
                g_app.active_view = 2;
                RefreshInspectorView();
            } else if (id == ID_BTN_LOAD_BIN) {
                char filename[MAX_PATH] = {0};
                OPENFILENAMEA ofn{};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = "Amiibo Binary Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    std::string err;
                    if (g_app.bin.LoadFromFile(filename, &err)) {
                        g_app.current_path = filename;
                        SyncControlsFromBin();
                        std::string msg = "Loaded Amiibo .bin: " + g_app.current_path;
                        SetWindowTextA(g_app.hwnd_status, msg.c_str());
                    } else {
                        MessageBoxA(hwnd, err.c_str(), "Load Error", MB_ICONERROR);
                    }
                }
            } else if (id == ID_BTN_LOAD_KEYS) {
                char filename[MAX_PATH] = {0};
                OPENFILENAMEA ofn{};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = "Amiibo Retail Key (key_retail.bin)\0*.bin\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    std::string err;
                    if (g_app.bin.LoadRetailKey(filename, &err)) {
                        SyncControlsFromBin();
                        SetWindowTextA(g_app.hwnd_status, "Loaded 160-byte key_retail.bin! Full NTAG215 AES-128-CTR & HMAC-SHA256 signing active.");
                    } else {
                        MessageBoxA(hwnd, err.c_str(), "Key Load Error", MB_ICONERROR);
                    }
                }
            } else if (id == ID_BTN_NEW_BIN) {
                LRESULT f_idx = SendMessageA(g_app.hwnd_fighter, CB_GETCURSEL, 0, 0);
                const auto& roster = GetFighterCatalog();
                uint16_t char_id = (f_idx != CB_ERR && static_cast<size_t>(f_idx) < roster.size())
                    ? roster[f_idx].char_id : 0x0000;
                g_app.bin.InitializeBlankFighter(char_id, 0);
                SyncControlsFromBin();
                SetWindowTextA(g_app.hwnd_status, "Initialized fresh SSBU Amiibo .bin in memory.");
            } else if (id == ID_BTN_RESET_LV1) {
                g_app.bin.SetDisplayedLevel(1);
                g_app.bin.SetCpuLevel(1);
                g_app.bin.EncodeBehavior(AmiiboBehaviorParams{});
                SyncControlsFromBin();
                SetWindowTextA(g_app.hwnd_status, "Reset Amiibo to Level 1 with neutral default AI weights.");
            } else if (id == ID_BTN_MAX_LV50) {
                g_app.bin.SetDisplayedLevel(50);
                g_app.bin.SetCpuLevel(9);
                SyncControlsFromBin();
                SetWindowTextA(g_app.hwnd_status, "Boosted Amiibo experience to Level 50 (3,912 XP, CPU Lv 9).");
            } else if (id == ID_BTN_APPLY_SPIRITS) {
                ApplySelectedFighterAndSpirits(hwnd);
                SetWindowTextA(g_app.hwnd_status, "Equipped Primary & Support Spirits and updated Amiibo AppData.");
            } else if (id == ID_BTN_APPLY_TUNING) {
                AmiiboBehaviorParams bp = g_app.bin.DecodeBehavior();
                auto read_pct = [](HWND h) {
                    char b[32] = {0};
                    GetWindowTextA(h, b, sizeof(b));
                    return std::clamp(std::atoi(b), 0, 100) / 100.0f;
                };
                bp.offensive = read_pct(g_app.hwnd_tune_offense);
                bp.grounded = read_pct(g_app.hwnd_tune_grounded);
                bp.just_shield_master = read_pct(g_app.hwnd_tune_parry);
                bp.attack_out_cliff = read_pct(g_app.hwnd_tune_edgeguard);
                bp.catcher = read_pct(g_app.hwnd_tune_grab);
                bp.meteor_master = read_pct(g_app.hwnd_tune_meteor);
                g_app.bin.EncodeBehavior(bp);
                SyncControlsFromBin();
                SetWindowTextA(g_app.hwnd_status, "Applied manual AI behavior fine-tuning overrides and recomputed personality.");
            } else if (id == ID_BTN_TRAIN) {
                ApplySelectedFighterAndSpirits(hwnd);
                char gbuf[32] = {0};
                GetWindowTextA(g_app.hwnd_games, gbuf, sizeof(gbuf));
                int num_games = std::clamp(std::atoi(gbuf), 1, 100000);

                const auto& roster = GetFighterCatalog();
                LRESULT opp_idx = SendMessageA(g_app.hwnd_opponent, CB_GETCURSEL, 0, 0);
                uint16_t opp_id = (opp_idx != CB_ERR && static_cast<size_t>(opp_idx) < roster.size())
                    ? roster[opp_idx].char_id : 0x0580;

                LRESULT arch_idx = SendMessageA(g_app.hwnd_archetype, CB_GETCURSEL, 0, 0);
                OpponentArchetype arch = (arch_idx != CB_ERR)
                    ? static_cast<OpponentArchetype>(arch_idx) : OpponentArchetype::FullRosterGauntlet;

                LRESULT foc_idx = SendMessageA(g_app.hwnd_focus, CB_GETCURSEL, 0, 0);
                TrainingFocus foc = (foc_idx != CB_ERR)
                    ? static_cast<TrainingFocus>(foc_idx) : TrainingFocus::TournamentOptimal;

                TrainingConfig cfg{};
                cfg.num_games = num_games;
                cfg.opponent_char_id = opp_id;
                cfg.archetype = arch;
                cfg.focus = foc;
                cfg.freeze_learning_after_training = true;

                SendMessageA(g_app.hwnd_progress, PBM_SETPOS, 0, 0);
                TrainingSummary sum = TrainingSimulator::RunTrainingSession(
                    g_app.bin, cfg, [&](int cur, int tot, float) {
                        int pct = (cur * 100) / std::max(1, tot);
                        SendMessageA(g_app.hwnd_progress, PBM_SETPOS, pct, 0);
                    });

                float win_rate = (sum.total_games > 0)
                    ? (100.0f * static_cast<float>(sum.wins) / static_cast<float>(sum.total_games)) : 0.0f;

                std::ostringstream log_ss;
                log_ss << "====================================================================\n";
                log_ss << "           SSBU AMIIBO MULTI-MATCH TRAINING REPORT                  \n";
                log_ss << "====================================================================\n";
                log_ss << "Matches Simulated   : " << sum.total_games << "\n";
                log_ss << "Record (W - L)      : " << sum.wins << " W - " << sum.losses << " L ("
                       << std::fixed << std::setprecision(1) << win_rate << "% Win Rate)\n";
                log_ss << "Early vs Late Win % : Q1 " << (sum.first_quarter_win_rate * 100.0f)
                       << "% -> Q4 " << (sum.final_quarter_win_rate * 100.0f) << "%\n";
                log_ss << "Level Progression   : Lv " << sum.initial_level << " -> Lv " << sum.final_level
                       << " (CPU Lv " << sum.initial_cpu_level << " -> " << sum.final_cpu_level << ")\n";
                log_ss << "Avg Damage / Match  : " << sum.avg_damage_dealt << "% Dealt | "
                       << sum.avg_damage_taken << "% Taken\n";
                log_ss << "Parries / Meteors   : " << sum.total_parries << " Parries | "
                       << sum.total_meteors << " Meteor Smashes\n";
                log_ss << "Trained Personality : " << sum.initial_personality << " -> " << sum.final_personality << "\n\n";
                log_ss << "--- Sample Milestone Match Logs ---\n";
                for (const auto& m : sum.sample_matches) {
                    log_ss << "  [Match #" << std::setw(4) << m.game_index << "] vs. "
                           << std::setw(16) << std::left << m.opponent_name << std::right << " | "
                           << (m.amiibo_won ? "WIN " : "LOSS")
                           << " (" << m.amiibo_stocks_left << "-" << m.opponent_stocks_left << " stocks)"
                           << " | Dmg: " << std::setw(5) << m.damage_dealt << "%"
                           << " | Parries: " << m.parries_landed << "\n";
                }
                g_app.last_sim_log = log_ss.str();

                SyncControlsFromBin();
                char status_buf[256];
                std::snprintf(status_buf, sizeof(status_buf),
                    "Completed %d games! Record: %dW-%dL (%.1f%%) | Level %d -> %d | Personality: %s",
                    sum.total_games, sum.wins, sum.losses, win_rate,
                    sum.initial_level, sum.final_level, sum.final_personality.c_str());
                SetWindowTextA(g_app.hwnd_status, status_buf);
            } else if (id == ID_BTN_SAVE_BIN) {
                ApplySelectedFighterAndSpirits(hwnd);
                char filename[MAX_PATH] = "trained_amiibo.bin";
                OPENFILENAMEA ofn{};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = "Amiibo Binary Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                if (GetSaveFileNameA(&ofn)) {
                    std::string err;
                    if (g_app.bin.SaveToFile(filename, &err)) {
                        std::string msg = std::string("Saved trained 540-byte Amiibo .bin to: ") + filename +
                            " (" + g_app.bin.GetFormatStatusString() + ")";
                        SetWindowTextA(g_app.hwnd_status, msg.c_str());
                        MessageBoxA(hwnd, msg.c_str(), "Amiibo .bin Saved Successfully", MB_ICONINFORMATION);
                    } else {
                        MessageBoxA(hwnd, err.c_str(), "Save Error", MB_ICONERROR);
                    }
                }
            }
            return 0;
        }

        case WM_DESTROY:
            if (g_app.font_regular) DeleteObject(g_app.font_regular);
            if (g_app.font_bold) DeleteObject(g_app.font_bold);
            if (g_app.font_mono) DeleteObject(g_app.font_mono);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int RunWin32Gui(HINSTANCE hInstance, int nCmdShow) {
    const char CLASS_NAME[] = "SSBUAmiiboTrainerWin32ClassV2";
    WNDCLASSA wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0, CLASS_NAME,
        "Super Smash Bros. Ultimate - Amiibo Trainer & Spirit Studio v2.0 (C++20)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1075, 765,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 1;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

} // namespace ssbu
#endif
