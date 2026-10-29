#include "win32_gui.hpp"
#include "amiibo_bin.hpp"
#include "ssbu_loadouts.hpp"
#include "training_sim.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <iomanip>
#include <sstream>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

namespace ssbu {

namespace {

enum ControlIds : int {
    IDC_BTN_LOAD_BIN = 101,
    IDC_BTN_NEW_BIN  = 102,
    IDC_BTN_SAVE_BIN = 103,
    IDC_CMB_FIGHTER  = 104,
    IDC_CMB_PRIMARY  = 105,
    IDC_EDT_ATTACK   = 106,
    IDC_EDT_DEFENSE  = 107,
    IDC_CMB_SLOT1    = 108,
    IDC_CMB_SLOT2    = 109,
    IDC_CMB_SLOT3    = 110,
    IDC_CHK_LEGAL    = 111,
    IDC_EDT_GAMES    = 112,
    IDC_CMB_ARCH     = 113,
    IDC_CMB_FOCUS    = 114,
    IDC_BTN_TRAIN    = 115,
    IDC_PRG_TRAIN    = 116,
    IDC_TXT_LOG      = 117
};

struct GuiState {
    AmiiboBin amiibo;
    std::string current_path;
    HWND hwnd = nullptr;
    HWND cmb_fighter = nullptr;
    HWND cmb_primary = nullptr;
    HWND edt_attack = nullptr;
    HWND edt_defense = nullptr;
    HWND cmb_slot1 = nullptr;
    HWND cmb_slot2 = nullptr;
    HWND cmb_slot3 = nullptr;
    HWND chk_legal = nullptr;
    HWND edt_games = nullptr;
    HWND cmb_arch = nullptr;
    HWND cmb_focus = nullptr;
    HWND prg_train = nullptr;
    HWND txt_log = nullptr;
};

static GuiState g_state;

static void SetLogText(const std::string& text) {
    std::string crlf;
    crlf.reserve(text.size() + 64);
    for (char c : text) {
        if (c == '\n') crlf.push_back('\r');
        crlf.push_back(c);
    }
    SetWindowTextA(g_state.txt_log, crlf.c_str());
}

static void RefreshUiFromAmiibo(const std::string& header_msg) {
    const auto& fighters = GetFighterCatalog();
    uint16_t cid = g_state.amiibo.GetCharacterId();
    int f_idx = 0;
    for (size_t i = 0; i < fighters.size(); ++i) {
        if (fighters[i].char_id == cid) {
            f_idx = static_cast<int>(i);
            break;
        }
    }
    SendMessageA(g_state.cmb_fighter, CB_SETCURSEL, f_idx, 0);

    SetWindowTextA(g_state.edt_attack, std::to_string(g_state.amiibo.GetAttackStat()).c_str());
    SetWindowTextA(g_state.edt_defense, std::to_string(g_state.amiibo.GetDefenseStat()).c_str());

    auto slots = g_state.amiibo.GetSpiritSlots();
    SendMessageA(g_state.cmb_slot1, CB_SETCURSEL, slots[0], 0);
    SendMessageA(g_state.cmb_slot2, CB_SETCURSEL, slots[1], 0);
    SendMessageA(g_state.cmb_slot3, CB_SETCURSEL, slots[2], 0);

    auto beh = g_state.amiibo.DecodeBehavior();
    const auto* fi = FindFighterById(cid);
    const auto* s1 = FindSupportAbilityById(slots[0]);
    const auto* s2 = FindSupportAbilityById(slots[1]);
    const auto* s3 = FindSupportAbilityById(slots[2]);

    std::ostringstream oss;
    oss << header_msg << "\n"
        << "========================================================\n"
        << "Fighter: " << (fi ? fi->name : "Unknown") << " (0x" << std::hex << cid << std::dec << ")\n"
        << "Level: " << g_state.amiibo.GetDisplayedLevel() << " (XP: " << g_state.amiibo.GetLevelExperience()
        << " / 3912) | Hidden CPU Level: " << g_state.amiibo.GetCpuLevel() << " (XP: " << g_state.amiibo.GetCpuExperience() << ")\n"
        << "Personality: " << g_state.amiibo.EvaluatePersonality()
        << " | Spirit Type: " << SpiritTypeToString(g_state.amiibo.GetSpiritType()) << "\n"
        << "Attack / Defense: " << g_state.amiibo.GetAttackStat() << " / " << g_state.amiibo.GetDefenseStat() << "\n"
        << "Equipped Support Spirits:\n"
        << "  [Slot 1] " << (s1 ? s1->name : "None") << "\n"
        << "  [Slot 2] " << (s2 ? s2->name : "None") << "\n"
        << "  [Slot 3] " << (s3 ? s3->name : "None") << "\n"
        << std::fixed << std::setprecision(1)
        << "Key AI Behavior Weights:\n"
        << "  Offensive: " << beh.offensive * 100.0f << "% | Grounded: " << beh.grounded * 100.0f
        << "% | Near: " << beh.near_range * 100.0f << "%\n"
        << "  Parry (Just Shield): " << beh.just_shield_master * 100.0f
        << "% | Grab: " << beh.catcher * 100.0f
        << "% | Edgeguard: " << beh.attack_out_cliff * 100.0f << "%\n"
        << "  Implicit Jab: " << beh.ImplicitJabPercent()
        << "% | Implicit Nair: " << beh.ImplicitNairPercent() << "%\n";
    SetLogText(oss.str());
}

static bool ApplyUiSelectionsToAmiibo(std::string* err) {
    const auto& fighters = GetFighterCatalog();
    int f_idx = static_cast<int>(SendMessageA(g_state.cmb_fighter, CB_GETCURSEL, 0, 0));
    if (f_idx >= 0 && static_cast<size_t>(f_idx) < fighters.size()) {
        g_state.amiibo.SetCharacterId(fighters[static_cast<size_t>(f_idx)].char_id);
    }

    char buf_atk[32]{}, buf_def[32]{};
    GetWindowTextA(g_state.edt_attack, buf_atk, sizeof(buf_atk));
    GetWindowTextA(g_state.edt_defense, buf_def, sizeof(buf_def));
    int16_t atk = static_cast<int16_t>(std::atoi(buf_atk));
    int16_t def = static_cast<int16_t>(std::atoi(buf_def));
    bool legal = (SendMessageA(g_state.chk_legal, BM_GETCHECK, 0, 0) == BST_CHECKED);
    g_state.amiibo.SetStats(atk, def, legal);

    uint8_t s1 = static_cast<uint8_t>(std::max<LRESULT>(0, SendMessageA(g_state.cmb_slot1, CB_GETCURSEL, 0, 0)));
    uint8_t s2 = static_cast<uint8_t>(std::max<LRESULT>(0, SendMessageA(g_state.cmb_slot2, CB_GETCURSEL, 0, 0)));
    uint8_t s3 = static_cast<uint8_t>(std::max<LRESULT>(0, SendMessageA(g_state.cmb_slot3, CB_GETCURSEL, 0, 0)));
    return g_state.amiibo.SetSpiritSlots(s1, s2, s3, legal, err);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_state.hwnd = hwnd;
            CreateWindowA("BUTTON", "Load Amiibo .bin...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                          16, 14, 150, 30, hwnd, (HMENU)IDC_BTN_LOAD_BIN, nullptr, nullptr);
            CreateWindowA("BUTTON", "New Fighter .bin", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                          176, 14, 140, 30, hwnd, (HMENU)IDC_BTN_NEW_BIN, nullptr, nullptr);
            CreateWindowA("BUTTON", "Save Trained .bin...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                          326, 14, 160, 30, hwnd, (HMENU)IDC_BTN_SAVE_BIN, nullptr, nullptr);

            CreateWindowA("STATIC", "Fighter:", WS_VISIBLE | WS_CHILD, 16, 56, 70, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.cmb_fighter = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                                90, 52, 220, 300, hwnd, (HMENU)IDC_CMB_FIGHTER, nullptr, nullptr);
            for (const auto& f : GetFighterCatalog()) {
                SendMessageA(g_state.cmb_fighter, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(f.name));
            }

            CreateWindowA("STATIC", "Primary Spirit Preset:", WS_VISIBLE | WS_CHILD, 326, 56, 145, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.cmb_primary = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                                475, 52, 280, 300, hwnd, (HMENU)IDC_CMB_PRIMARY, nullptr, nullptr);
            for (const auto& p : GetPrimarySpiritCatalog()) {
                std::ostringstream label;
                label << p.name << " [" << SpiritTypeToString(p.type) << " " << p.base_attack << "/" << p.base_defense << "]";
                SendMessageA(g_state.cmb_primary, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.str().c_str()));
            }
            SendMessageA(g_state.cmb_primary, CB_SETCURSEL, 0, 0);

            CreateWindowA("STATIC", "Attack Stat:", WS_VISIBLE | WS_CHILD, 16, 92, 85, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.edt_attack = CreateWindowA("EDIT", "0", WS_VISIBLE | WS_CHILD | WS_BORDER,
                                               105, 88, 75, 24, hwnd, (HMENU)IDC_EDT_ATTACK, nullptr, nullptr);
            CreateWindowA("STATIC", "Defense Stat:", WS_VISIBLE | WS_CHILD, 195, 92, 95, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.edt_defense = CreateWindowA("EDIT", "0", WS_VISIBLE | WS_CHILD | WS_BORDER,
                                                295, 88, 75, 24, hwnd, (HMENU)IDC_EDT_DEFENSE, nullptr, nullptr);
            g_state.chk_legal = CreateWindowA("BUTTON", "Enforce Legal Caps (5000 Stat / 3 Spirit Slots)",
                                              WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                                              390, 88, 360, 24, hwnd, (HMENU)IDC_CHK_LEGAL, nullptr, nullptr);
            SendMessageA(g_state.chk_legal, BM_SETCHECK, BST_CHECKED, 0);

            CreateWindowA("STATIC", "Support Spirit 1:", WS_VISIBLE | WS_CHILD, 16, 128, 115, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.cmb_slot1 = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                              135, 124, 620, 320, hwnd, (HMENU)IDC_CMB_SLOT1, nullptr, nullptr);

            CreateWindowA("STATIC", "Support Spirit 2:", WS_VISIBLE | WS_CHILD, 16, 160, 115, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.cmb_slot2 = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                              135, 156, 620, 320, hwnd, (HMENU)IDC_CMB_SLOT2, nullptr, nullptr);

            CreateWindowA("STATIC", "Support Spirit 3:", WS_VISIBLE | WS_CHILD, 16, 192, 115, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.cmb_slot3 = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                              135, 188, 620, 320, hwnd, (HMENU)IDC_CMB_SLOT3, nullptr, nullptr);

            for (const auto& s : GetSupportSpiritCatalog()) {
                std::ostringstream lbl;
                lbl << "[" << int(s.id) << "] " << s.name << " (Cost: " << int(s.slot_cost)
                    << (s.tournament_legal ? ", Legal" : ", Bonus") << ") - " << s.description;
                std::string str = lbl.str();
                SendMessageA(g_state.cmb_slot1, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
                SendMessageA(g_state.cmb_slot2, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
                SendMessageA(g_state.cmb_slot3, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
            }

            CreateWindowA("STATIC", "Games to Run:", WS_VISIBLE | WS_CHILD, 16, 232, 105, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.edt_games = CreateWindowA("EDIT", "500", WS_VISIBLE | WS_CHILD | WS_BORDER,
                                              135, 228, 90, 24, hwnd, (HMENU)IDC_EDT_GAMES, nullptr, nullptr);

            CreateWindowA("STATIC", "Opponent Style:", WS_VISIBLE | WS_CHILD, 240, 232, 110, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.cmb_arch = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                             355, 228, 400, 220, hwnd, (HMENU)IDC_CMB_ARCH, nullptr, nullptr);
            for (int i = 0; i <= static_cast<int>(OpponentArchetype::FullRosterGauntlet); ++i) {
                SendMessageA(g_state.cmb_arch, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(TrainingSimulator::ArchetypeToString(static_cast<OpponentArchetype>(i))));
            }
            SendMessageA(g_state.cmb_arch, CB_SETCURSEL, static_cast<int>(OpponentArchetype::FullRosterGauntlet), 0);

            CreateWindowA("STATIC", "Training Focus:", WS_VISIBLE | WS_CHILD, 16, 266, 110, 22, hwnd, nullptr, nullptr, nullptr);
            g_state.cmb_focus = CreateWindowA("COMBOBOX", "", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                                              135, 262, 410, 220, hwnd, (HMENU)IDC_CMB_FOCUS, nullptr, nullptr);
            for (int i = 0; i <= static_cast<int>(TrainingFocus::AdaptiveCounterplay); ++i) {
                SendMessageA(g_state.cmb_focus, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(TrainingSimulator::FocusToString(static_cast<TrainingFocus>(i))));
            }
            SendMessageA(g_state.cmb_focus, CB_SETCURSEL, 0, 0);

            CreateWindowA("BUTTON", "Run Training Games!", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                          560, 258, 195, 32, hwnd, (HMENU)IDC_BTN_TRAIN, nullptr, nullptr);

            g_state.prg_train = CreateWindowExA(0, PROGRESS_CLASSA, nullptr, WS_VISIBLE | WS_CHILD,
                                                16, 302, 739, 20, hwnd, (HMENU)IDC_PRG_TRAIN, nullptr, nullptr);
            SendMessageA(g_state.prg_train, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

            g_state.txt_log = CreateWindowA("EDIT", "",
                                            WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL |
                                            ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                            16, 332, 739, 250, hwnd, (HMENU)IDC_TXT_LOG, nullptr, nullptr);

            RefreshUiFromAmiibo("Ready! Load an existing 540-byte Amiibo .bin or configure a new fighter and click 'Run Training Games!'.");
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);
            if (id == IDC_CMB_PRIMARY && code == CBN_SELCHANGE) {
                int idx = static_cast<int>(SendMessageA(g_state.cmb_primary, CB_GETCURSEL, 0, 0));
                const auto& prim = GetPrimarySpiritCatalog();
                if (idx >= 0 && static_cast<size_t>(idx) < prim.size()) {
                    g_state.amiibo.ApplyPrimarySpiritPreset(prim[static_cast<size_t>(idx)]);
                    RefreshUiFromAmiibo("Applied Primary Spirit preset: " + std::string(prim[static_cast<size_t>(idx)].name));
                }
            } else if (id == IDC_BTN_NEW_BIN) {
                int f_idx = static_cast<int>(SendMessageA(g_state.cmb_fighter, CB_GETCURSEL, 0, 0));
                const auto& fighters = GetFighterCatalog();
                uint16_t cid = (f_idx >= 0 && static_cast<size_t>(f_idx) < fighters.size())
                               ? fighters[static_cast<size_t>(f_idx)].char_id : 0x0000;
                g_state.amiibo.InitializeBlankFighter(cid, 0);
                RefreshUiFromAmiibo("Initialized fresh Level 1 SSBU Amiibo .bin.");
            } else if (id == IDC_BTN_LOAD_BIN) {
                char file_buf[MAX_PATH]{};
                OPENFILENAMEA ofn{};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = "Amiibo Bin Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = file_buf;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    std::string err;
                    if (g_state.amiibo.LoadFromFile(file_buf, &err)) {
                        g_state.current_path = file_buf;
                        RefreshUiFromAmiibo("Loaded Amiibo .bin: " + g_state.current_path);
                    } else {
                        MessageBoxA(hwnd, err.c_str(), "Load Error", MB_ICONERROR);
                    }
                }
            } else if (id == IDC_BTN_SAVE_BIN) {
                std::string err;
                if (!ApplyUiSelectionsToAmiibo(&err)) {
                    MessageBoxA(hwnd, err.c_str(), "Spirit / Stat Configuration Error", MB_ICONWARNING);
                    return 0;
                }
                char file_buf[MAX_PATH] = "trained_amiibo.bin";
                OPENFILENAMEA ofn{};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = "Amiibo Bin Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = file_buf;
                ofn.nMaxFile = MAX_PATH;
                ofn.lpstrDefExt = "bin";
                ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                if (GetSaveFileNameA(&ofn)) {
                    if (g_state.amiibo.SaveToFile(file_buf, &err)) {
                        RefreshUiFromAmiibo("Saved trained 540-byte Amiibo .bin to: " + std::string(file_buf));
                    } else {
                        MessageBoxA(hwnd, err.c_str(), "Save Error", MB_ICONERROR);
                    }
                }
            } else if (id == IDC_BTN_TRAIN) {
                std::string err;
                if (!ApplyUiSelectionsToAmiibo(&err)) {
                    MessageBoxA(hwnd, err.c_str(), "Spirit Slot Limit", MB_ICONWARNING);
                    return 0;
                }
                char games_buf[32]{};
                GetWindowTextA(g_state.edt_games, games_buf, sizeof(games_buf));
                int games = std::clamp(std::atoi(games_buf), 1, 100000);

                TrainingConfig cfg{};
                cfg.num_games = games;
                cfg.archetype = static_cast<OpponentArchetype>(
                    std::max<LRESULT>(0, SendMessageA(g_state.cmb_arch, CB_GETCURSEL, 0, 0)));
                cfg.focus = static_cast<TrainingFocus>(
                    std::max<LRESULT>(0, SendMessageA(g_state.cmb_focus, CB_GETCURSEL, 0, 0)));

                SendMessageA(g_state.prg_train, PBM_SETPOS, 0, 0);
                auto summary = TrainingSimulator::RunTrainingSession(
                    g_state.amiibo, cfg,
                    [](int done, int total, float) {
                        int pct = (done * 100) / std::max(1, total);
                        SendMessageA(g_state.prg_train, PBM_SETPOS, pct, 0);
                    });

                std::ostringstream hdr;
                hdr << std::fixed << std::setprecision(1)
                    << "Training Complete! Ran " << summary.total_games << " games ("
                    << summary.wins << "W - " << summary.losses << "L) | "
                    << "Win Rate improved from " << summary.first_quarter_win_rate * 100.0f
                    << "% to " << summary.final_quarter_win_rate * 100.0f << "% | "
                    << "Level " << summary.initial_level << " -> " << summary.final_level
                    << " | Personality: " << summary.initial_personality << " -> " << summary.final_personality;
                RefreshUiFromAmiibo(hdr.str());
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

} // namespace

int RunWin32Gui() {
    InitCommonControls();
    WNDCLASSA wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = "SSBUAmiiboTrainerWnd";
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0, wc.lpszClassName,
        "Super Smash Bros. Ultimate - Amiibo .bin Trainer & Spirit Collection Studio",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 785, 635,
        nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) return 1;

    MSG msg{};
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return static_cast<int>(msg.wParam);
}

} // namespace ssbu
#else
namespace ssbu {
int RunWin32Gui() { return 0; }
} // namespace ssbu
#endif
