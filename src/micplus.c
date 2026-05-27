#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

// ресурсы
#define IDI_APPICON   101
#define IDI_ICON_ON   102
#define IDI_ICON_OFF  103

#define WM_APP_TRAY       (WM_APP + 1)

// IDs для основного меню
#define ID_TRAY_PTT_MODE      2001
#define ID_TRAY_MIC_MUTE      2002
#define ID_TRAY_MIC_UNMUTE    2003
#define ID_TRAY_MIC_TOGGLE    2004
#define ID_TRAY_EXIT          2005

// IDs для подменю Hotkeys (PTT + Toggle)
#define ID_HK_PTT_MB4         2100
#define ID_HK_PTT_MB5         2101
#define ID_HK_PTT_CUSTOM      2102
#define ID_HK_TOGGLE_F9       2103
#define ID_HK_TOGGLE_F10      2104
#define ID_HK_TOGGLE_CUSTOM   2105

// IDs для подменю Language
#define ID_LANG_EN            2200
#define ID_LANG_RU            2201

// ID для RegisterHotKey
#define HOTKEY_ID_TOGGLE      1   // глобальный toggle

HINSTANCE g_hInst;
HHOOK g_hMouseHook;
HHOOK g_hKeyboardHook;
NOTIFYICONDATAA g_nid;

// главное скрытое окно (только для сообщений)
HWND g_hMainWnd = NULL;

int g_muted = 0;          // 0 = mic ON, 1 = mic OFF
int g_pttMode = 0;        // 1 = PTT mode включён

HICON g_hIconOn = NULL;
HICON g_hIconOff = NULL;

char g_iniPath[MAX_PATH];

// Toggle hotkey
UINT g_vkToggle  = VK_F10;
UINT g_modToggle = 0;
int  g_toggleCustomSet = 0; // 0 - стандарт (F9/F10), 1 - своё

// PTT: мышь или кастомная клавиша
UINT g_pttButton    = XBUTTON2;  // по умолчанию MButton5
UINT g_vkPttCustom  = 0;         // 0 = нет кастомного PTT
int  g_pttKeyDown   = 0;         // флаг: кастомная PTT-клавиша сейчас зажата

// Язык
typedef enum {
    LANG_EN = 0,
    LANG_RU = 1
} APP_LANG;

APP_LANG g_lang = LANG_EN;

// Подписи для меню
char g_labelMicMute[64];
char g_labelMicUnmute[64];
char g_labelMicToggle[64];
char g_labelToggleCustom[64];
char g_labelPttCustom[64];

// прототип, чтобы линкер не ругался
void RemoveTrayIcon(void);

// ---------- INI ----------

void BuildIniPath(void)
{
    GetModuleFileNameA(NULL, g_iniPath, MAX_PATH);
    char *slash = strrchr(g_iniPath, '\\');
    if (slash) *(slash + 1) = '\0';
    lstrcatA(g_iniPath, "micplus.ini");
}

void LoadSettings(void)
{
    BuildIniPath();

    char buf[32];

    GetPrivateProfileStringA("General", "PTTMode", "0", buf, sizeof(buf), g_iniPath);
    g_pttMode = (buf[0] == '1') ? 1 : 0;

    GetPrivateProfileStringA("General", "ToggleVK", "121", buf, sizeof(buf), g_iniPath);
    g_vkToggle = (UINT)atoi(buf);
    if (g_vkToggle == 0) g_vkToggle = VK_F10;

    GetPrivateProfileStringA("General", "ToggleMods", "0", buf, sizeof(buf), g_iniPath);
    g_modToggle = (UINT)atoi(buf);

    GetPrivateProfileStringA("General", "PTTButton", "2", buf, sizeof(buf), g_iniPath);
    int ptt = atoi(buf);
    g_pttButton = (ptt == 1) ? XBUTTON1 : XBUTTON2;

    GetPrivateProfileStringA("General", "PttCustomVK", "0", buf, sizeof(buf), g_iniPath);
    g_vkPttCustom = (UINT)atoi(buf);

    char langBuf[8];
    GetPrivateProfileStringA("General", "Language", "en", langBuf, sizeof(langBuf), g_iniPath);
    if (lstrcmpiA(langBuf, "ru") == 0)
        g_lang = LANG_RU;
    else
        g_lang = LANG_EN;
}

void SaveSettings(void)
{
    char buf[32];

    wsprintfA(buf, "%d", g_pttMode ? 1 : 0);
    WritePrivateProfileStringA("General", "PTTMode", buf, g_iniPath);

    wsprintfA(buf, "%u", g_vkToggle);
    WritePrivateProfileStringA("General", "ToggleVK", buf, g_iniPath);

    wsprintfA(buf, "%u", g_modToggle);
    WritePrivateProfileStringA("General", "ToggleMods", buf, g_iniPath);

    int ptt = (g_pttButton == XBUTTON1) ? 1 : 2;
    wsprintfA(buf, "%d", ptt);
    WritePrivateProfileStringA("General", "PTTButton", buf, g_iniPath);

    wsprintfA(buf, "%u", g_vkPttCustom);
    WritePrivateProfileStringA("General", "PttCustomVK", buf, g_iniPath);

    WritePrivateProfileStringA("General", "Language",
                               (g_lang == LANG_RU) ? "ru" : "en",
                               g_iniPath);
}

// ---------- Локализация ----------

const char* T(const char* id)
{
    if (g_lang == LANG_RU) {
        if (lstrcmpA(id, "MENU_PTT_MB4") == 0)       return "Рация: Кнопка мыши 4";
        if (lstrcmpA(id, "MENU_PTT_MB5") == 0)       return "Рация: Кнопка мыши 5";
        if (lstrcmpA(id, "MENU_PTT_CUSTOM") == 0)    return "Назначить клавишу...";
        if (lstrcmpA(id, "MENU_TOGGLE_F9") == 0)     return "Переключение: F9";
        if (lstrcmpA(id, "MENU_TOGGLE_F10") == 0)    return "Переключение: F10";
        if (lstrcmpA(id, "MENU_TOGGLE_CUSTOM") == 0) return "Назначить...";
        if (lstrcmpA(id, "MENU_PTT_MODE") == 0)      return "Режим рации (PTT)";
        if (lstrcmpA(id, "MENU_MIC") == 0)           return "Микрофон";
        if (lstrcmpA(id, "MENU_HOTKEYS") == 0)       return "Горячие клавиши";
        if (lstrcmpA(id, "MENU_LANGUAGE") == 0)      return "Язык";
        if (lstrcmpA(id, "MENU_EXIT") == 0)          return "Выход";

        if (lstrcmpA(id, "MSG_TOGGLE_SELECT_TITLE") == 0)
            return "MicPlus - Выбор хоткея Toggle";
        if (lstrcmpA(id, "MSG_TOGGLE_SELECT_TEXT") == 0)
            return "Закройте это окно, затем нажмите нужное сочетание (модификаторы + клавиша).\n"
                   "ESC в основном окне отменяет выбор.";
        if (lstrcmpA(id, "MSG_PTT_SELECT_TITLE") == 0)
            return "MicPlus - Выбор PTT-клавиши";
        if (lstrcmpA(id, "MSG_PTT_SELECT_TEXT") == 0)
            return "Закройте это окно, затем нажмите нужную PTT-клавишу.\n"
                   "ESC в основном окне отменяет выбор.";
        if (lstrcmpA(id, "MSG_SELECTED_HK") == 0)
            return "Выбран хоткей: %s";
        if (lstrcmpA(id, "MSG_SELECTED_PTT") == 0)
            return "Выбрана PTT-клавиша: %s";
        if (lstrcmpA(id, "MSG_ERR_TOGGLE_ENTER_ESC") == 0)
            return "Enter/Esc нельзя использовать в одиночку.\nДобавьте Ctrl/Alt/Shift/Win или выберите другую клавишу.";
        if (lstrcmpA(id, "MSG_ERR_PTT_ENTER_ESC") == 0)
            return "Enter/Esc нельзя использовать для PTT.\nВыберите другую клавишу.";
        if (lstrcmpA(id, "MSG_ERR_HOOK") == 0)
            return "Не удалось установить низкоуровневые хуки.";
        if (lstrcmpA(id, "MSG_ERR_HOTKEY") == 0)
            return "Не удалось зарегистрировать хоткей Toggle.";

        if (lstrcmpA(id, "TOOLTIP_FORMAT_ON") == 0)
            return "MicPlus: MIC ON (PTT %s, Toggle %s)";
        if (lstrcmpA(id, "TOOLTIP_FORMAT_OFF") == 0)
            return "MicPlus: MIC OFF (PTT %s, Toggle %s)";
    } else {
        if (lstrcmpA(id, "MENU_PTT_MB4") == 0)       return "PTT: MButton4";
        if (lstrcmpA(id, "MENU_PTT_MB5") == 0)       return "PTT: MButton5";
        if (lstrcmpA(id, "MENU_PTT_CUSTOM") == 0)    return "PTT: Custom Key";
        if (lstrcmpA(id, "MENU_TOGGLE_F9") == 0)     return "Toggle: F9";
        if (lstrcmpA(id, "MENU_TOGGLE_F10") == 0)    return "Toggle: F10";
        if (lstrcmpA(id, "MENU_TOGGLE_CUSTOM") == 0) return "Toggle: Custom";
        if (lstrcmpA(id, "MENU_PTT_MODE") == 0)      return "PTT mode";
        if (lstrcmpA(id, "MENU_MIC") == 0)           return "Microphone";
        if (lstrcmpA(id, "MENU_HOTKEYS") == 0)       return "Hotkeys";
        if (lstrcmpA(id, "MENU_LANGUAGE") == 0)      return "Language";
        if (lstrcmpA(id, "MENU_EXIT") == 0)          return "Exit";

        if (lstrcmpA(id, "MSG_TOGGLE_SELECT_TITLE") == 0)
            return "MicPlus - Select Toggle hotkey";
        if (lstrcmpA(id, "MSG_TOGGLE_SELECT_TEXT") == 0)
            return "Close this window, then press the desired hotkey (modifiers + key) in the main window.\n"
                   "Press Esc in the main window to cancel.";
        if (lstrcmpA(id, "MSG_PTT_SELECT_TITLE") == 0)
            return "MicPlus - Select PTT key";
        if (lstrcmpA(id, "MSG_PTT_SELECT_TEXT") == 0)
            return "Close this window, then press the desired PTT key in the main window.\n"
                   "Press Esc in the main window to cancel.";
        if (lstrcmpA(id, "MSG_SELECTED_HK") == 0)
            return "Selected hotkey: %s";
        if (lstrcmpA(id, "MSG_SELECTED_PTT") == 0)
            return "Selected PTT key: %s";
        if (lstrcmpA(id, "MSG_ERR_TOGGLE_ENTER_ESC") == 0)
            return "Enter/Esc cannot be used alone.\nPlease include Ctrl/Alt/Shift/Win or pick another key.";
        if (lstrcmpA(id, "MSG_ERR_PTT_ENTER_ESC") == 0)
            return "Enter/Esc cannot be used for PTT.\nPick another key.";
        if (lstrcmpA(id, "MSG_ERR_HOOK") == 0)
            return "Failed to set low-level hooks.";
        if (lstrcmpA(id, "MSG_ERR_HOTKEY") == 0)
            return "Failed to register toggle hotkey.";

        if (lstrcmpA(id, "TOOLTIP_FORMAT_ON") == 0)
            return "MicPlus: MIC ON (PTT %s, Toggle %s)";
        if (lstrcmpA(id, "TOOLTIP_FORMAT_OFF") == 0)
            return "MicPlus: MIC OFF (PTT %s, Toggle %s)";
    }

    return id;
}

// ---------- Вспомогательные функции ----------

void RunMicCtl(int mute)
{
    char args[4];
    wsprintfA(args, "%d", mute ? 1 : 0);
    ShellExecuteA(NULL, "open", "micctl.exe", args, NULL, SW_HIDE);
}

void GetKeyDisplayName(UINT vk, char *out, int outSize)
{
    switch (vk) {
    case VK_F1:  lstrcpynA(out, "F1",  outSize); return;
    case VK_F2:  lstrcpynA(out, "F2",  outSize); return;
    case VK_F3:  lstrcpynA(out, "F3",  outSize); return;
    case VK_F4:  lstrcpynA(out, "F4",  outSize); return;
    case VK_F5:  lstrcpynA(out, "F5",  outSize); return;
    case VK_F6:  lstrcpynA(out, "F6",  outSize); return;
    case VK_F7:  lstrcpynA(out, "F7",  outSize); return;
    case VK_F8:  lstrcpynA(out, "F8",  outSize); return;
    case VK_F9:  lstrcpynA(out, "F9",  outSize); return;
    case VK_F10: lstrcpynA(out, "F10", outSize); return;
    case VK_F11: lstrcpynA(out, "F11", outSize); return;
    case VK_F12: lstrcpynA(out, "F12", outSize); return;
    }

    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        char tmp[2] = { (char)vk, 0 };
        lstrcpynA(out, tmp, outSize);
        return;
    }

    UINT sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    if (sc != 0) {
        LONG lparam = (sc << 16);
        char buf[64];
        int len = GetKeyNameTextA(lparam, buf, sizeof(buf));
        if (len > 0) {
            lstrcpynA(out, buf, outSize);
            return;
        }
    }

    wsprintfA(out, "VK %u", vk);
}

void BuildHotkeyName(UINT mods, UINT vk, char *out, int outSize)
{
    char buf[64] = "";

    if (mods & MOD_CONTROL)  lstrcatA(buf, "Ctrl+");
    if (mods & MOD_ALT)      lstrcatA(buf, "Alt+");
    if (mods & MOD_SHIFT)    lstrcatA(buf, "Shift+");
    if (mods & MOD_WIN)      lstrcatA(buf, "Win+");

    char key[32];
    GetKeyDisplayName(vk, key, sizeof(key));

    lstrcatA(buf, key);
    lstrcpynA(out, buf, outSize);
}

void UpdateMicMenuLabels(void)
{
    char hkName[64];
    BuildHotkeyName(g_modToggle, g_vkToggle, hkName, sizeof(hkName));

    if (g_lang == LANG_RU) {
        wsprintfA(g_labelMicMute,   "Выключить    (%s)", hkName);
        lstrcpyA (g_labelMicUnmute, "Включить");
        wsprintfA(g_labelMicToggle, "Переключение (%s)", hkName);
    } else {
        wsprintfA(g_labelMicMute,   "Mute mic (%s)", hkName);
        lstrcpyA (g_labelMicUnmute, "Unmute mic");
        wsprintfA(g_labelMicToggle, "Toggle mic (%s)", hkName);
    }
}

void UpdateToggleCustomLabel(void)
{
    if (g_vkToggle == 0) {
        if (g_lang == LANG_RU) {
            lstrcpyA(g_labelToggleCustom, "Своё значение");
        } else {
            lstrcpyA(g_labelToggleCustom, "Custom value");
        }
        return;
    }

    if (!g_toggleCustomSet) {
        if (g_lang == LANG_RU) {
            lstrcpyA(g_labelToggleCustom, "Назначить...");
        } else {
            lstrcpyA(g_labelToggleCustom, "Custom key...");
        }
        return;
    }

    char hkName[64];
    BuildHotkeyName(g_modToggle, g_vkToggle, hkName, sizeof(hkName));

    if (g_lang == LANG_RU) {
        wsprintfA(g_labelToggleCustom, "Переключение: %s", hkName);
    } else {
        wsprintfA(g_labelToggleCustom, "Toggle: %s", hkName);
    }
}

void UpdatePttCustomLabel(void)
{
    if (g_vkPttCustom == 0) {
        lstrcpyA(g_labelPttCustom, T("MENU_PTT_CUSTOM"));
        return;
    }

    char key[32];
    GetKeyDisplayName(g_vkPttCustom, key, sizeof(key));

    if (g_lang == LANG_RU) {
        wsprintfA(g_labelPttCustom, "Рация: %s", key);
    } else {
        wsprintfA(g_labelPttCustom, "PTT: %s", key);
    }
}

void UpdateTooltip(void)
{
    if (!g_nid.hWnd) return;

    g_nid.uFlags = NIF_ICON | NIF_TIP;

    char hkName[64];
    BuildHotkeyName(g_modToggle, g_vkToggle, hkName, sizeof(hkName));

    const char *pttName;
    static char pttBuf[32];

    if (g_vkPttCustom != 0) {
        GetKeyDisplayName(g_vkPttCustom, pttBuf, sizeof(pttBuf));
        pttName = pttBuf;
    } else {
        pttName = (g_pttButton == XBUTTON1) ? "MButton4" : "MButton5";
    }

    if (g_muted) {
        g_nid.hIcon = g_hIconOff;
        wsprintfA(g_nid.szTip, T("TOOLTIP_FORMAT_OFF"), pttName, hkName);
    } else {
        g_nid.hIcon = g_hIconOn;
        wsprintfA(g_nid.szTip, T("TOOLTIP_FORMAT_ON"), pttName, hkName);
    }

    Shell_NotifyIconA(NIM_MODIFY, &g_nid);
}

void SetMicState(int mute)
{
    RunMicCtl(mute);
    g_muted = mute ? 1 : 0;
    UpdateTooltip();
}

void ToggleMic(void)
{
    SetMicState(!g_muted);
}

void LoadIcons(void)
{
    HINSTANCE hInst = GetModuleHandleA(NULL);

    g_hIconOn  = (HICON)LoadImageA(
        hInst,
        MAKEINTRESOURCEA(IDI_ICON_ON),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTSIZE
    );

    g_hIconOff = (HICON)LoadImageA(
        hInst,
        MAKEINTRESOURCEA(IDI_ICON_OFF),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTSIZE
    );

    if (!g_hIconOn)  g_hIconOn  = LoadIcon(NULL, IDI_APPLICATION);
    if (!g_hIconOff) g_hIconOff = LoadIcon(NULL, IDI_APPLICATION);
}

void AddTrayIcon(HWND hWnd)
{
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_APP_TRAY;

    LoadIcons();
    UpdateMicMenuLabels();
    UpdateToggleCustomLabel();
    UpdatePttCustomLabel();

    g_nid.hIcon = g_muted ? g_hIconOff : g_hIconOn;
    lstrcpyA(g_nid.szTip, "MicPlus");

    Shell_NotifyIconA(NIM_ADD, &g_nid);
    UpdateTooltip();
}

void RemoveTrayIcon(void)
{
    if (g_nid.hWnd) {
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
    }
}

// ---------- Захват хоткеев ----------

BOOL CaptureToggleHotkey(HWND hWnd, UINT *pMods, UINT *pVk)
{
    MessageBoxA(hWnd,
                T("MSG_TOGGLE_SELECT_TEXT"),
                T("MSG_TOGGLE_SELECT_TITLE"),
                MB_OK | MB_ICONINFORMATION);

    UINT mods = 0;
    UINT vk   = 0;

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {
            UINT code = (UINT)msg.wParam;

            if (code == VK_CONTROL || code == VK_LCONTROL || code == VK_RCONTROL) {
                mods |= MOD_CONTROL;
            } else if (code == VK_MENU || code == VK_LMENU || code == VK_RMENU) {
                mods |= MOD_ALT;
            } else if (code == VK_SHIFT || code == VK_LSHIFT || code == VK_RSHIFT) {
                mods |= MOD_SHIFT;
            } else if (code == VK_LWIN || code == VK_RWIN) {
                mods |= MOD_WIN;
            } else if (code == VK_ESCAPE) {
                return FALSE;
            } else {
                vk = code;
                break;
            }
        } else {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (vk == 0) return FALSE;

    if ((vk == VK_RETURN || vk == VK_ESCAPE) && mods == 0) {
        MessageBoxA(hWnd,
                    T("MSG_ERR_TOGGLE_ENTER_ESC"),
                    "MicPlus",
                    MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    *pMods = mods;
    *pVk   = vk;

    char hkName[64];
    BuildHotkeyName(mods, vk, hkName, sizeof(hkName));
    char msgBox[128];
    wsprintfA(msgBox, T("MSG_SELECTED_HK"), hkName);
    MessageBoxA(hWnd, msgBox, "MicPlus", MB_OK | MB_ICONINFORMATION);

    return TRUE;
}

BOOL CapturePttKey(HWND hWnd, UINT *pVk)
{
    MessageBoxA(hWnd,
                T("MSG_PTT_SELECT_TEXT"),
                T("MSG_PTT_SELECT_TITLE"),
                MB_OK | MB_ICONINFORMATION);

    UINT vk = 0;

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {
            UINT code = (UINT)msg.wParam;

            if (code == VK_ESCAPE) {
                return FALSE;
            }

            vk = code;
            break;
        } else {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (vk == 0) return FALSE;

    if (vk == VK_RETURN || vk == VK_ESCAPE) {
        MessageBoxA(hWnd,
                    T("MSG_ERR_PTT_ENTER_ESC"),
                    "MicPlus",
                    MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    *pVk = vk;

    char keyName[32];
    GetKeyDisplayName(vk, keyName, sizeof(keyName));
    char msgBox[128];
    wsprintfA(msgBox, T("MSG_SELECTED_PTT"), keyName);
    MessageBoxA(hWnd, msgBox, "MicPlus", MB_OK | MB_ICONINFORMATION);

    return TRUE;
}

// ---------- Hotkeys: регистрация ----------

void ReRegisterToggleHotkey(HWND hWnd)
{
    UnregisterHotKey(hWnd, HOTKEY_ID_TOGGLE);
    RegisterHotKey(hWnd, HOTKEY_ID_TOGGLE, g_modToggle, g_vkToggle);
    UpdateMicMenuLabels();
    UpdateToggleCustomLabel();
    UpdateTooltip();
    SaveSettings();
}

// ---------- Меню и обработка ----------

void ShowTrayMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    HMENU hMicMenu = CreatePopupMenu();
    HMENU hHotkeysMenu = CreatePopupMenu();
    HMENU hLangMenu = CreatePopupMenu();

    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_STRING |
                         (g_pttMode ? MF_CHECKED : 0),
                ID_TRAY_PTT_MODE, T("MENU_PTT_MODE"));

    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    UINT mb4Flags      = MF_STRING | ((g_pttButton == XBUTTON1 && g_vkPttCustom == 0) ? MF_CHECKED : 0);
    UINT mb5Flags      = MF_STRING | ((g_pttButton == XBUTTON2 && g_vkPttCustom == 0) ? MF_CHECKED : 0);
    UINT customPttFlag = MF_STRING | ((g_vkPttCustom != 0) ? MF_CHECKED : 0);

    AppendMenuA(hHotkeysMenu, mb4Flags, ID_HK_PTT_MB4, T("MENU_PTT_MB4"));
    AppendMenuA(hHotkeysMenu, mb5Flags, ID_HK_PTT_MB5, T("MENU_PTT_MB5"));
    AppendMenuA(hHotkeysMenu, customPttFlag, ID_HK_PTT_CUSTOM, g_labelPttCustom);

    AppendMenuA(hHotkeysMenu, MF_SEPARATOR, 0, NULL);

    UINT f9Flags      = MF_STRING;
    UINT f10Flags     = MF_STRING;
    UINT customFlags  = MF_STRING;

    if (g_modToggle == 0 && g_vkToggle == VK_F9) {
        f9Flags |= MF_CHECKED;
    } else if (g_modToggle == 0 && g_vkToggle == VK_F10) {
        f10Flags |= MF_CHECKED;
    } else {
        customFlags |= MF_CHECKED;
    }

    AppendMenuA(hHotkeysMenu, f9Flags,  ID_HK_TOGGLE_F9,  T("MENU_TOGGLE_F9"));
    AppendMenuA(hHotkeysMenu, f10Flags, ID_HK_TOGGLE_F10, T("MENU_TOGGLE_F10"));
    AppendMenuA(hHotkeysMenu, customFlags,
                ID_HK_TOGGLE_CUSTOM, g_labelToggleCustom);

    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_POPUP,
                (UINT_PTR)hHotkeysMenu, T("MENU_HOTKEYS"));

    AppendMenuA(hMicMenu, MF_STRING, ID_TRAY_MIC_MUTE,   g_labelMicMute);
    AppendMenuA(hMicMenu, MF_STRING, ID_TRAY_MIC_UNMUTE, g_labelMicUnmute);
    AppendMenuA(hMicMenu, MF_STRING, ID_TRAY_MIC_TOGGLE, g_labelMicToggle);
    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_POPUP,
                (UINT_PTR)hMicMenu, T("MENU_MIC"));

    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    UINT enFlags = MF_STRING | ((g_lang == LANG_EN) ? MF_CHECKED : 0);
    UINT ruFlags = MF_STRING | ((g_lang == LANG_RU) ? MF_CHECKED : 0);
    AppendMenuA(hLangMenu, enFlags, ID_LANG_EN, "English");
    AppendMenuA(hLangMenu, ruFlags, ID_LANG_RU, "Русский");
    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_POPUP,
                (UINT_PTR)hLangMenu, T("MENU_LANGUAGE"));

    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_STRING,
                ID_TRAY_EXIT, T("MENU_EXIT"));

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                   pt.x, pt.y, 0, hWnd, NULL);

    DestroyMenu(hMenu);
}

// ---------- Хуки ----------

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && g_vkPttCustom == 0) {
        PMSLLHOOKSTRUCT p = (PMSLLHOOKSTRUCT)lParam;
        DWORD btn = (p->mouseData >> 16) & 0xFFFF;

        if (btn == g_pttButton && g_pttMode) {
            if (wParam == WM_XBUTTONDOWN) {
                SetMicState(0);
            } else if (wParam == WM_XBUTTONUP) {
                SetMicState(1);
            }
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && g_vkPttCustom != 0 && g_pttMode) {
        KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;
        if (p->vkCode == g_vkPttCustom) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                if (!g_pttKeyDown) {
                    g_pttKeyDown = 1;
                    SetMicState(0);
                }
            } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                if (g_pttKeyDown) {
                    g_pttKeyDown = 0;
                    SetMicState(1);
                }
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// ---------- Окно (невидимое, только для сообщений) ----------

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_APP_TRAY:
        if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
            ShowTrayMenu(hWnd);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_PTT_MODE:
            g_pttMode = !g_pttMode;
            if (g_pttMode) {
                SetMicState(1);
            }
            SaveSettings();
            break;

        case ID_TRAY_MIC_MUTE:
            SetMicState(1);
            break;

        case ID_TRAY_MIC_UNMUTE:
            SetMicState(0);
            break;

        case ID_TRAY_MIC_TOGGLE:
            ToggleMic();
            break;

        case ID_HK_PTT_MB4:
            g_pttButton   = XBUTTON1;
            g_vkPttCustom = 0;
            g_pttKeyDown  = 0;
            SaveSettings();
            UpdatePttCustomLabel();
            UpdateTooltip();
            break;

        case ID_HK_PTT_MB5:
            g_pttButton   = XBUTTON2;
            g_vkPttCustom = 0;
            g_pttKeyDown  = 0;
            SaveSettings();
            UpdatePttCustomLabel();
            UpdateTooltip();
            break;

        case ID_HK_PTT_CUSTOM:
        {
            UINT vk = 0;
            if (CapturePttKey(hWnd, &vk)) {
                g_vkPttCustom = vk;
                g_pttKeyDown  = 0;
                SaveSettings();
                UpdatePttCustomLabel();
                UpdateTooltip();
            }
            break;
        }

        case ID_HK_TOGGLE_F9:
            g_modToggle = 0;
            g_vkToggle  = VK_F9;
            g_toggleCustomSet = 0;
            ReRegisterToggleHotkey(hWnd);
            break;

        case ID_HK_TOGGLE_F10:
            g_modToggle = 0;
            g_vkToggle  = VK_F10;
            g_toggleCustomSet = 0;
            ReRegisterToggleHotkey(hWnd);
            break;

        case ID_HK_TOGGLE_CUSTOM:
        {
            UINT mods = 0, vk = 0;
            if (CaptureToggleHotkey(hWnd, &mods, &vk)) {
                g_modToggle = mods;
                g_vkToggle  = vk;
                g_toggleCustomSet = 1;
                ReRegisterToggleHotkey(hWnd);
            }
            break;
        }

        case ID_LANG_EN:
            g_lang = LANG_EN;
            SaveSettings();
            UpdateMicMenuLabels();
            UpdateToggleCustomLabel();
            UpdatePttCustomLabel();
            UpdateTooltip();
            break;

        case ID_LANG_RU:
            g_lang = LANG_RU;
            SaveSettings();
            UpdateMicMenuLabels();
            UpdateToggleCustomLabel();
            UpdatePttCustomLabel();
            UpdateTooltip();
            break;

        case ID_TRAY_EXIT:
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_HOTKEY:
        if (wParam == HOTKEY_ID_TOGGLE) {
            ToggleMic();
        }
        break;

    case WM_DESTROY:
        // выходим только через ID_TRAY_EXIT
        break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ---------- WinMain ----------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    g_hInst = hInstance;

    LoadSettings();

    if (g_modToggle == 0 && (g_vkToggle == VK_F9 || g_vkToggle == VK_F10)) {
        g_toggleCustomSet = 0;
    } else {
        g_toggleCustomSet = 1;
    }

    const char *CLASS_NAME = "MICPLUS_CLASS";

    WNDCLASSEXA wc = { 0 };
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon         = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_APPICON));
    wc.hIconSm       = wc.hIcon;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    if (!RegisterClassExA(&wc)) {
        return 1;
    }

    // создаём скрытое служебное окно
    g_hMainWnd = CreateWindowExA(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, // не в Alt+Tab, не в таскбаре
        CLASS_NAME,
        "MicPlusHidden",
        WS_POPUP,                             // без рамок, без WS_VISIBLE
        -100, -100, 0, 0,
        NULL, NULL, hInstance, NULL
    );
    if (!g_hMainWnd) {
        return 1;
    }

    ShowWindow(g_hMainWnd, SW_HIDE);
    SetWindowPos(g_hMainWnd, HWND_BOTTOM, -100, -100, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);

    AddTrayIcon(g_hMainWnd);

    g_hMouseHook    = SetWindowsHookExA(WH_MOUSE_LL,    LowLevelMouseProc,    NULL, 0);
    g_hKeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!g_hMouseHook || !g_hKeyboardHook) {
        MessageBoxA(NULL, T("MSG_ERR_HOOK"), "MicPlus", MB_ICONERROR);
        if (g_hMouseHook)    UnhookWindowsHookEx(g_hMouseHook);
        if (g_hKeyboardHook) UnhookWindowsHookEx(g_hKeyboardHook);
        RemoveTrayIcon();
        return 1;
    }

    if (!RegisterHotKey(g_hMainWnd, HOTKEY_ID_TOGGLE, g_modToggle, g_vkToggle)) {
        MessageBoxA(NULL, T("MSG_ERR_HOTKEY"), "MicPlus", MB_ICONERROR);
    }

    SetMicState(0);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    SetMicState(0);

    if (g_hMouseHook)    UnhookWindowsHookEx(g_hMouseHook);
    if (g_hKeyboardHook) UnhookWindowsHookEx(g_hKeyboardHook);
    UnregisterHotKey(g_hMainWnd, HOTKEY_ID_TOGGLE);
    RemoveTrayIcon();

    SaveSettings();

    return 0;
}
