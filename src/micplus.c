#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <mmsystem.h>   // PlaySoundW
#include <shlwapi.h>    // PathCombineA (по желанию)
#include <winreg.h>     // реестр для автозапуска

// ресурсы
#define IDI_APPICON        101
#define IDI_ICON_ON_LIGHT  102
#define IDI_ICON_ON_DARK   103
#define IDI_ICON_OFF_RED   104
#define ID_SETTINGS_ICON_LIGHT  40010 

#define WM_APP_TRAY       (WM_APP + 1)

// IDs для основного меню
#define ID_TRAY_PTT_MODE      2001
#define ID_TRAY_MIC_TOGGLE    2002   // Микрофон
#define ID_TRAY_SOUND_TOGGLE  2003   // Звук системы
#define ID_TRAY_EXIT          2005

// IDs для подменю Hotkeys
#define ID_HK_PTT_MB5         2101
#define ID_HK_PTT_CUSTOM      2102

#define ID_HK_MIC_F10         2110   // Микрофон: F10
#define ID_HK_MIC_CUSTOM      2111   // Микрофон: Custom

#define ID_HK_SOUND_DEFAULT   2120   // Sound: Alt+F9
#define ID_HK_SOUND_CUSTOM    2121   // Sound: Custom

// IDs для Language (внутри Настроек)
#define ID_LANG_EN            2200
#define ID_LANG_RU            2201

// IDs для Settings
#define ID_SETTINGS_LANGUAGE        2300
#define ID_SETTINGS_AUTOSTART       2301
#define ID_SETTINGS_MIC_SOUNDS      2302
#define ID_SETTINGS_PTT_SOUNDS      2303
#define ID_SETTINGS_BALLOON         2304
#define ID_SETTINGS_ICON_AUTO   	40010
#define ID_SETTINGS_ICON_BLACK  	40011
#define ID_SETTINGS_ICON_WHITE  	40012

// ID для RegisterHotKey
#define HOTKEY_ID_TOGGLE     	1   // микрофон
#define HOTKEY_ID_SOUND       	2   // звук
#define HOTKEY_ID_TOGGLE_ALTGR  3
#define HOTKEY_ID_SOUND_ALTGR   4

HINSTANCE g_hInst;
HHOOK g_hMouseHook;
HHOOK g_hKeyboardHook;
NOTIFYICONDATAA g_nid;

HWND g_hMainWnd = NULL;

int g_muted      = 0;   // 0 = mic ON, 1 = mic OFF
int g_pttMode    = 0;   // 1 = PTT mode включён
int g_soundMuted = 0;   // локальный флаг для звука
int g_iconMode = 0; // 0 = авто, 1 = чёрный, 2 = белый

HICON g_hIconOnLight   = NULL;
HICON g_hIconOnDark    = NULL;
HICON g_hIconOnCurrent = NULL;
HICON g_hIconOffRed    = NULL;

char g_iniPath[MAX_PATH];
char g_exePath[MAX_PATH];

int IsAppLightTheme(void)
{
    HKEY hKey;
    DWORD value = 1;
    DWORD size = sizeof(value);

    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return 1;
    }

    if (RegGetValueA(hKey, NULL, "AppsUseLightTheme",
        RRF_RT_REG_DWORD, NULL, &value, &size) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 1;
    }

    RegCloseKey(hKey);
    return (value != 0);
}

int IsSystemLightTheme(void)
{
    HKEY hKey;
    DWORD value = 1;
    DWORD size = sizeof(value);

    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return 1;
    }

    if (RegGetValueA(hKey, NULL, "SystemUsesLightTheme",
        RRF_RT_REG_DWORD, NULL, &value, &size) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 1;
    }

    RegCloseKey(hKey);
    return (value != 0);
}

// Папка со звуками рядом с exe: .\sounds\*.wav
const char g_soundsDir[] = "sounds";

// Микрофон: toggle hotkey
UINT g_vkToggle  = VK_F10;
UINT g_modToggle = 0;
int  g_toggleCustomSet = 0;

// PTT: мышь или кастомная клавиша
UINT g_pttButton    = XBUTTON2;  // по умолчанию MButton5
UINT g_vkPttCustom  = 0;
int  g_pttKeyDown   = 0;

// Язык
typedef enum {
    LANG_EN = 0,
    LANG_RU = 1
} APP_LANG;

APP_LANG g_lang = LANG_EN;

// Подписи
char g_labelMicToggle[64];     // "Микрофон (…)" / "Microphone (…) "
char g_labelPttCustom[64];     // "Назначить..." / "PTT: Custom…"
char g_labelSoundToggle[64];   // "Звук системы (…)" / "Global sound (…) "
char g_labelPttMode[64];       // "Режим рации (…)" / "PTT mode (…)"

// Звук (master volume mute)
UINT g_vkSoundToggle  = VK_F9;
UINT g_modSoundToggle = MOD_ALT;   // Alt+F9

// Настройки автозапуска и звуков
int  g_autoStart        = 0;
int  g_enableMicSounds  = 1;
int  g_enablePTTSounds  = 1;
int  g_enableBalloonTips= 1;

char g_micSoundOn[MAX_PATH]    = "mic_on.wav";
char g_micSoundOff[MAX_PATH]   = "mic_off.wav";
char g_pttSoundOn[MAX_PATH]    = "ptt_start.wav";
char g_pttSoundOff[MAX_PATH]   = "ptt_end.wav";

// прототипы
void RemoveTrayIcon(void);
void UpdateTooltip(void);
void SetMicStateGlobal(int mute);
void SetMicStatePTT(int mute);
void UpdateMicLabel(void);
void UpdateSoundLabel(void);
void UpdatePttCustomLabel(void);
void UpdatePttModeLabel(void);
void ShowMicBalloon(int mute);
void PlayPTTSound(int start);
void PlayMicSound(int mute);
int IsSystemLightTheme(void);
void AddTrayIcon(HWND hWnd);

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---------- INI ----------


void BuildPaths(void)
{
    GetModuleFileNameA(NULL, g_exePath, MAX_PATH);
    lstrcpyA(g_iniPath, g_exePath);
    char* slash = strrchr(g_iniPath, '\\');
    if (slash) *(slash + 1) = '\0';
    lstrcatA(g_iniPath, "micplus.ini");
}

void LoadSettings(void)
{
    BuildPaths();

    char buf[256];

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

    // Sound hotkey
    GetPrivateProfileStringA("General", "SoundToggleVK", "120", buf, sizeof(buf), g_iniPath);
    g_vkSoundToggle = (UINT)atoi(buf);
    if (g_vkSoundToggle == 0) g_vkSoundToggle = VK_F9;

    GetPrivateProfileStringA("General", "SoundToggleMods", "1", buf, sizeof(buf), g_iniPath);
    g_modSoundToggle = (UINT)atoi(buf);

    // Autostart
    GetPrivateProfileStringA("General", "AutoStart", "0", buf, sizeof(buf), g_iniPath);
    g_autoStart = (buf[0] == '1') ? 1 : 0;

    // Mic/PTT sounds & balloon
    GetPrivateProfileStringA("General", "EnableMicSounds", "1", buf, sizeof(buf), g_iniPath);
    g_enableMicSounds = (buf[0] == '0') ? 0 : 1;

    GetPrivateProfileStringA("General", "EnablePTTSounds", "1", buf, sizeof(buf), g_iniPath);
    g_enablePTTSounds = (buf[0] == '0') ? 0 : 1;

    GetPrivateProfileStringA("General", "EnableBalloonTips", "1", buf, sizeof(buf), g_iniPath);
    g_enableBalloonTips = (buf[0] == '0') ? 0 : 1;

    GetPrivateProfileStringA("General", "MicSoundOn", "mic_on.wav", g_micSoundOn, sizeof(g_micSoundOn), g_iniPath);
    GetPrivateProfileStringA("General", "MicSoundOff", "mic_off.wav", g_micSoundOff, sizeof(g_micSoundOff), g_iniPath);
    GetPrivateProfileStringA("General", "PTTSoundOn", "ptt_start.wav", g_pttSoundOn, sizeof(g_pttSoundOn), g_iniPath);
    GetPrivateProfileStringA("General", "PTTSoundOff", "ptt_end.wav", g_pttSoundOff, sizeof(g_pttSoundOff), g_iniPath);

int mode = GetPrivateProfileIntA("MicPlus", "IconMode", 0, g_iniPath);

    if (mode < 0 || mode > 2) {
        mode = 0; // дефолт: Система
    }

    g_iconMode = mode;
}

void UpdateIconTheme(void)
{
    if (g_iconMode == 0) {
        // Авто по системной теме
        if (IsSystemLightTheme()) {
            // светлая Windows -> чёрный значок
            g_hIconOnCurrent = g_hIconOnLight;
        } else {
            // тёмная Windows -> белый значок
            g_hIconOnCurrent = g_hIconOnDark;
        }
    } else if (g_iconMode == 1) {
        // всегда чёрный
        g_hIconOnCurrent = g_hIconOnDark;
    } else if (g_iconMode == 2) {
        // всегда белый
        g_hIconOnCurrent = g_hIconOnLight;
    }
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

    wsprintfA(buf, "%u", g_vkSoundToggle);
    WritePrivateProfileStringA("General", "SoundToggleVK", buf, g_iniPath);

    wsprintfA(buf, "%u", g_modSoundToggle);
    WritePrivateProfileStringA("General", "SoundToggleMods", buf, g_iniPath);

    wsprintfA(buf, "%d", g_autoStart ? 1 : 0);
    WritePrivateProfileStringA("General", "AutoStart", buf, g_iniPath);

    wsprintfA(buf, "%d", g_enableMicSounds ? 1 : 0);
    WritePrivateProfileStringA("General", "EnableMicSounds", buf, g_iniPath);

    wsprintfA(buf, "%d", g_enablePTTSounds ? 1 : 0);
    WritePrivateProfileStringA("General", "EnablePTTSounds", buf, g_iniPath);

    wsprintfA(buf, "%d", g_enableBalloonTips ? 1 : 0);
    WritePrivateProfileStringA("General", "EnableBalloonTips", buf, g_iniPath);

    WritePrivateProfileStringA("General", "MicSoundOn", g_micSoundOn, g_iniPath);
    WritePrivateProfileStringA("General", "MicSoundOff", g_micSoundOff, g_iniPath);
    WritePrivateProfileStringA("General", "PTTSoundOn", g_pttSoundOn, g_iniPath);
    WritePrivateProfileStringA("General", "PTTSoundOff", g_pttSoundOff, g_iniPath);
	
	wsprintfA(buf, "%d", g_iconMode);
    WritePrivateProfileStringA("MicPlus", "IconMode", buf, g_iniPath);
}

// ---------- Локализация ----------

const char* T(const char* id)
{
    if (g_lang == LANG_RU) {
        if (lstrcmpA(id, "MENU_PTT_MB5") == 0)       return "РТТ: Мышь 5";
        if (lstrcmpA(id, "MENU_PTT_CUSTOM") == 0)    return "Назначить клавишу";
        if (lstrcmpA(id, "MENU_MIC") == 0)           return "Микрофон";
        if (lstrcmpA(id, "MENU_SOUND") == 0)         return "Глобальный звук";
        if (lstrcmpA(id, "MENU_PTT_MODE") == 0)      return "Режим рации";
        if (lstrcmpA(id, "MENU_HOTKEYS") == 0)       return "Сочетания клавиш";
        if (lstrcmpA(id, "MENU_SETTINGS") == 0)      return "Настройки";
        if (lstrcmpA(id, "MENU_LANGUAGE") == 0)      return "Язык программы";
        if (lstrcmpA(id, "MENU_AUTOSTART") == 0)     return "Автозапуск вместе с Windows";
        if (lstrcmpA(id, "MENU_MIC_SOUNDS") == 0)    return "Звуки: состояние микрофона";
        if (lstrcmpA(id, "MENU_PTT_SOUNDS") == 0)    return "Звуки: использование PTT";
        if (lstrcmpA(id, "MENU_BALLOON") == 0)       return "Показывать уведомления";
        if (lstrcmpA(id, "MENU_EXIT") == 0)          return "Выход";

        if (lstrcmpA(id, "MSG_TOGGLE_SELECT_TITLE") == 0)
            return "MicPlus - Хоткей микрофона";
        if (lstrcmpA(id, "MSG_TOGGLE_SELECT_TEXT") == 0)
            return "Закройте это окно, затем нажмите нужное сочетание (модификаторы + клавиша).\n"
                   "ESC в основном окне отменяет выбор.";
        if (lstrcmpA(id, "MSG_SOUND_SELECT_TITLE") == 0)
            return "MicPlus - Хоткей звука системы";
        if (lstrcmpA(id, "MSG_SOUND_SELECT_TEXT") == 0)
            return "Закройте это окно, затем нажмите нужное сочетание (модификаторы + клавиша) для звука.\n"
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
            return "Не удалось зарегистрировать хоткей.";

        if (lstrcmpA(id, "TOOLTIP_FORMAT_ON") == 0)
            return "MicPlus: ВКЛ (Рация %s, Режим %s, Звук %s)";
        if (lstrcmpA(id, "TOOLTIP_FORMAT_OFF") == 0)
            return "MicPlus: ВЫКЛ (Рация %s, Режим %s, Звук %s)";

        if (lstrcmpA(id, "BALLOON_MIC_ON_TITLE") == 0)  return "MicPlus";
        if (lstrcmpA(id, "BALLOON_MIC_ON_TEXT") == 0)   return "Микрофон включён";
        if (lstrcmpA(id, "BALLOON_MIC_OFF_TITLE") == 0) return "MicPlus";
        if (lstrcmpA(id, "BALLOON_MIC_OFF_TEXT") == 0)  return "Включить микрофон";
    } else {
        if (lstrcmpA(id, "MENU_PTT_MB5") == 0)       return "PTT: MButton5";
        if (lstrcmpA(id, "MENU_PTT_CUSTOM") == 0)    return "PTT: Custom Key...";
        if (lstrcmpA(id, "MENU_MIC") == 0)           return "Microphone";
        if (lstrcmpA(id, "MENU_SOUND") == 0)         return "Global sound";
        if (lstrcmpA(id, "MENU_PTT_MODE") == 0)      return "PTT mode";
        if (lstrcmpA(id, "MENU_HOTKEYS") == 0)       return "Hotkeys";
        if (lstrcmpA(id, "MENU_SETTINGS") == 0)      return "Settings";
        if (lstrcmpA(id, "MENU_LANGUAGE") == 0)      return "Language";
        if (lstrcmpA(id, "MENU_AUTOSTART") == 0)     return "Start MicPlus with Windows";
        if (lstrcmpA(id, "MENU_MIC_SOUNDS") == 0)    return "Microphone sounds (mute/unmute)";
        if (lstrcmpA(id, "MENU_PTT_SOUNDS") == 0)    return "PTT sounds (push-to-talk)";
        if (lstrcmpA(id, "MENU_BALLOON") == 0)       return "Microphone notifications (balloon)";
        if (lstrcmpA(id, "MENU_EXIT") == 0)          return "Exit";

        if (lstrcmpA(id, "MSG_TOGGLE_SELECT_TITLE") == 0)
            return "MicPlus - Mic hotkey";
        if (lstrcmpA(id, "MSG_TOGGLE_SELECT_TEXT") == 0)
            return "Close this window, then press modifiers+key in the main window.\n"
                   "Press Esc in the main window to cancel.";
        if (lstrcmpA(id, "MSG_SOUND_SELECT_TITLE") == 0)
            return "MicPlus - Global sound hotkey";
        if (lstrcmpA(id, "MSG_SOUND_SELECT_TEXT") == 0)
            return "Close this window, then press modifiers+key for sound.\n"
                   "Press Esc in the main window to cancel.";

        if (lstrcmpA(id, "MSG_PTT_SELECT_TITLE") == 0)
            return "MicPlus - Select PTT key";
        if (lstrcmpA(id, "MSG_PTT_SELECT_TEXT") == 0)
            return "Close this window, then press the desired PTT key.\n"
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
            return "Failed to register hotkey.";

        if (lstrcmpA(id, "TOOLTIP_FORMAT_ON") == 0)
            return "MicPlus: ON (PTT %s, Mode %s, Sound %s)";
        if (lstrcmpA(id, "TOOLTIP_FORMAT_OFF") == 0)
            return "MicPlus: OFF (PTT %s, Mode %s, Sound %s)";

        if (lstrcmpA(id, "BALLOON_MIC_ON_TITLE") == 0)  return "MicPlus";
        if (lstrcmpA(id, "BALLOON_MIC_ON_TEXT") == 0)   return "Microphone ON";
        if (lstrcmpA(id, "BALLOON_MIC_OFF_TITLE") == 0) return "MicPlus";
        if (lstrcmpA(id, "BALLOON_MIC_OFF_TEXT") == 0)  return "Microphone OFF";
    }

    return id;
}

// ---------- Вспомогательные ----------

void RunMicCtl(int mute)
{
    char args[4];
    wsprintfA(args, "%d", mute ? 1 : 0);
    ShellExecuteA(NULL, "open", "micctl.exe", args, NULL, SW_HIDE);
}

// путь к .\sounds\<fileName>
void BuildSoundPath(const char *fileName, char *out, int outSize)
{
    char baseDir[MAX_PATH];
    lstrcpyA(baseDir, g_exePath);
    char *slash = strrchr(baseDir, '\\');
    if (slash) *(slash + 1) = '\0';   // оставляем только папку exe

    char tmp[MAX_PATH];
    wsprintfA(tmp, "%s%s\\%s", baseDir, g_soundsDir, fileName);
    lstrcpynA(out, tmp, outSize);
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

void UpdateMicLabel(void)
{
    char hkName[64];
    BuildHotkeyName(g_modToggle, g_vkToggle, hkName, sizeof(hkName));

    if (g_lang == LANG_RU) {
        if (g_muted) {
            wsprintfA(g_labelMicToggle, "Включить микрофон (%s)", hkName);
        } else {
            wsprintfA(g_labelMicToggle, "Микрофон включен (%s)", hkName);
        }
    } else {
        if (g_muted) {
            wsprintfA(g_labelMicToggle, "Microphone OFF (%s)", hkName);
        } else {
            wsprintfA(g_labelMicToggle, "Microphone ON (%s)", hkName);
        }
    }
}

void UpdateSoundLabel(void)
{
    char hkName[64];
    BuildHotkeyName(g_modSoundToggle, g_vkSoundToggle, hkName, sizeof(hkName));

    if (g_lang == LANG_RU) {
        wsprintfA(g_labelSoundToggle, "Глобальный звук (%s)", hkName);
    } else {
        wsprintfA(g_labelSoundToggle, "Global sound (%s)", hkName);
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

void UpdatePttModeLabel(void)
{
    char hkName[64];

    if (g_vkPttCustom != 0) {
        GetKeyDisplayName(g_vkPttCustom, hkName, sizeof(hkName));
    } else {
        if (g_lang == LANG_RU) {
            lstrcpynA(hkName, (g_pttButton == XBUTTON1) ? "Мышь 4" : "Мышь 5", sizeof(hkName));
        } else {
            lstrcpynA(hkName, (g_pttButton == XBUTTON1) ? "MButton4" : "MButton5", sizeof(hkName));
        }
    }

    if (g_lang == LANG_RU) {
        if (g_pttMode) {
            wsprintfA(g_labelPttMode, "Активный режим РТТ (%s)", hkName);
        } else {
            wsprintfA(g_labelPttMode, "Активировать РТТ (%s)", hkName);
        }
    } else {
        if (g_pttMode) {
            wsprintfA(g_labelPttMode, "PTT mode ON (%s)", hkName);
        } else {
            wsprintfA(g_labelPttMode, "PTT mode OFF (%s)", hkName);
        }
    }
}

void UpdateTooltip(void)
{
    if (!g_nid.hWnd) return;

    g_nid.uFlags = NIF_ICON | NIF_TIP;

    char micName[64];
    BuildHotkeyName(g_modToggle, g_vkToggle, micName, sizeof(micName));

    char soundName[64];
    BuildHotkeyName(g_modSoundToggle, g_vkSoundToggle, soundName, sizeof(soundName));

    const char *pttName;
    static char pttBuf[32];

    if (g_vkPttCustom != 0) {
        GetKeyDisplayName(g_vkPttCustom, pttBuf, sizeof(pttBuf));
        pttName = pttBuf;
    } else {
        if (g_lang == LANG_RU) {
            pttName = (g_pttButton == XBUTTON1) ? "Мышь 4" : "Мышь 5";
        } else {
            pttName = (g_pttButton == XBUTTON1) ? "MButton4" : "MButton5";
        }
    }

    if (g_muted) {
        g_nid.hIcon = g_hIconOffRed;
        wsprintfA(g_nid.szTip, T("TOOLTIP_FORMAT_OFF"), pttName, micName, soundName);
    } else {
        g_nid.hIcon = g_hIconOnCurrent;
        wsprintfA(g_nid.szTip, T("TOOLTIP_FORMAT_ON"), pttName, micName, soundName);
    }

    Shell_NotifyIconA(NIM_MODIFY, &g_nid);
}

// --- Звуки и balloon ---

void ShowMicBalloon(int mute)
{
    if (!g_enableBalloonTips) return;
    if (!g_nid.hWnd) return;

    g_nid.uFlags |= NIF_INFO;

    if (mute) {
        lstrcpynA(g_nid.szInfoTitle, "MicPlus", sizeof(g_nid.szInfoTitle));
        lstrcpynA(g_nid.szInfo,
                  (g_lang == LANG_RU) ? "Включить микрофон" : "Microphone OFF",
                  sizeof(g_nid.szInfo));
    } else {
        lstrcpynA(g_nid.szInfoTitle, "MicPlus", sizeof(g_nid.szInfoTitle));
        lstrcpynA(g_nid.szInfo,
                  (g_lang == LANG_RU) ? "Микрофон включён" : "Microphone ON",
                  sizeof(g_nid.szInfo));
    }

    g_nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconA(NIM_MODIFY, &g_nid);
}

void PlayPTTSound(int start)
{
    if (!g_enablePTTSounds) return;

    WCHAR pathW[MAX_PATH];
    char fullPath[MAX_PATH];
    const char* file = start ? g_pttSoundOn : g_pttSoundOff;

    BuildSoundPath(file, fullPath, sizeof(fullPath));

    MultiByteToWideChar(CP_UTF8, 0, fullPath, -1, pathW, MAX_PATH);
    PlaySoundW(pathW, NULL, SND_ASYNC | SND_FILENAME);
}

void PlayMicSound(int mute)
{
    if (!g_enableMicSounds) return;

    WCHAR pathW[MAX_PATH];
    char fullPath[MAX_PATH];
    const char* file = mute ? g_micSoundOff : g_micSoundOn;

    BuildSoundPath(file, fullPath, sizeof(fullPath));

    MultiByteToWideChar(CP_UTF8, 0, fullPath, -1, pathW, MAX_PATH);
    PlaySoundW(pathW, NULL, SND_ASYNC | SND_FILENAME);
}

// --- Микрофон ---

void SetMicStateGlobal(int mute)
{
    RunMicCtl(mute);
    g_muted = mute ? 1 : 0;
    UpdateTooltip();
    PlayMicSound(mute);      // микрофонные звуки
    ShowMicBalloon(mute);    // balloon-уведомления
    UpdateMicLabel();
}

void SetMicStatePTT(int mute)
{
    RunMicCtl(mute);
    g_muted = mute ? 1 : 0;
    UpdateTooltip();
    UpdateMicLabel();
    // без микрофонных звуков и balloon
}

void ToggleMic(void)
{
    SetMicStateGlobal(!g_muted);
}

// --- Звук: системный mute через APPCOMMAND_VOLUME_MUTE

void ToggleSystemSound(void)
{
    SendMessageA(GetForegroundWindow(), WM_APPCOMMAND, 0, MAKELPARAM(0, APPCOMMAND_VOLUME_MUTE));
    g_soundMuted = !g_soundMuted;
    UpdateTooltip();
}

// ---------- Иконки / трэй ----------

void LoadIcons(void)
{
    HINSTANCE hInst = GetModuleHandleA(NULL);

    g_hIconOnLight = (HICON)LoadImageA(
        hInst,
        MAKEINTRESOURCEA(IDI_ICON_ON_LIGHT),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTSIZE
    );

    g_hIconOnDark = (HICON)LoadImageA(
        hInst,
        MAKEINTRESOURCEA(IDI_ICON_ON_DARK),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTSIZE
    );

    g_hIconOffRed = (HICON)LoadImageA(
        hInst,
        MAKEINTRESOURCEA(IDI_ICON_OFF_RED),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTSIZE
    );

    if (!g_hIconOnLight) g_hIconOnLight = LoadIcon(NULL, IDI_APPLICATION);
    if (!g_hIconOnDark)  g_hIconOnDark  = g_hIconOnLight;
    if (!g_hIconOffRed)  g_hIconOffRed  = LoadIcon(NULL, IDI_APPLICATION);

    // применяем сохранённый режим
    UpdateIconTheme();
}

void RemoveTrayIcon(void)
{
    if (g_nid.hWnd) {
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
    }
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
    UpdateMicLabel();
    UpdateSoundLabel();
    UpdatePttCustomLabel();
    UpdatePttModeLabel();

    g_nid.hIcon = g_muted ? g_hIconOffRed : g_hIconOnCurrent;
    lstrcpyA(g_nid.szTip, "MicPlus");

    Shell_NotifyIconA(NIM_ADD, &g_nid);
    UpdateTooltip();
}

// ---------- Захват хоткеев ----------

BOOL CaptureHotkeyGeneric(HWND hWnd, const char *textId, const char *titleId,
                          UINT *pMods, UINT *pVk)
{
    MessageBoxA(hWnd,
                T(textId),
                T(titleId),
                MB_OK | MB_ICONINFORMATION);

    UINT mods = 0;
    UINT vk   = 0;

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN) {
            UINT code = (UINT)msg.wParam;

            // Если человек нажал чисто модификатор — копим его и ждём основную клавишу
            if (code == VK_CONTROL || code == VK_LCONTROL || code == VK_RCONTROL) {
                mods |= MOD_CONTROL;
                continue;
            } else if (code == VK_MENU || code == VK_LMENU || code == VK_RMENU) {
                mods |= MOD_ALT;
                continue;
            } else if (code == VK_SHIFT || code == VK_LSHIFT || code == VK_RSHIFT) {
                mods |= MOD_SHIFT;
                continue;
            } else if (code == VK_LWIN || code == VK_RWIN) {
                mods |= MOD_WIN;
                continue;
            } else if (code == VK_ESCAPE) {
                // отмена выбора
                return FALSE;
            } else {
                // это основная клавиша хоткея
                vk = code;
                break;
            }
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    // Если основную клавишу так и не получили — считаем, что ничего не выбрали
    if (vk == 0)
        return FALSE;

    // Универсальная нормализация модификаторов (левый/правый + AltGr)
    BOOL leftCtrl   = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0;
    BOOL rightCtrl  = (GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0;
    BOOL leftAlt    = (GetAsyncKeyState(VK_LMENU)    & 0x8000) != 0;
    BOOL rightAlt   = (GetAsyncKeyState(VK_RMENU)    & 0x8000) != 0;
    BOOL leftShift  = (GetAsyncKeyState(VK_LSHIFT)   & 0x8000) != 0;
    BOOL rightShift = (GetAsyncKeyState(VK_RSHIFT)   & 0x8000) != 0;

    BOOL isAltGr = leftCtrl && rightAlt;

    BOOL anyCtrl  = leftCtrl  || rightCtrl;
    BOOL anyAlt   = leftAlt   || rightAlt;
    BOOL anyShift = leftShift || rightShift;

    if (isAltGr) {
        // правый Alt (AltGr) в русской раскладке: считаем как просто Alt
        anyCtrl = FALSE;
        anyAlt  = TRUE;
    }

    // Пересобираем mods с учётом универсальных модификаторов
    mods = 0;
    if (anyCtrl)  mods |= MOD_CONTROL;
    if (anyAlt)   mods |= MOD_ALT;
    if (anyShift) mods |= MOD_SHIFT;

    // Защита от голого Enter/Escape без модификаторов
    if ((vk == VK_RETURN || vk == VK_ESCAPE) && mods == 0) {
        MessageBoxA(hWnd,
                    T("MSG_ERR_TOGGLE_ENTER_ESC"),
                    "MicPlus",
                    MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }

    // Вернуть результат вызывающему коду
    *pMods = mods;
    *pVk   = vk;

    // Построить текст вроде "Ctrl+Alt+F9" и показать пользователю
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

void ReRegisterMicHotkey(HWND hWnd)
{
    UnregisterHotKey(hWnd, HOTKEY_ID_TOGGLE);
    UnregisterHotKey(hWnd, HOTKEY_ID_TOGGLE_ALTGR);

    // основной хоткей, как раньше
    RegisterHotKey(hWnd, HOTKEY_ID_TOGGLE, g_modToggle, g_vkToggle);

    // если это чистый Alt, регистрируем дополнительно вариант Ctrl+Alt (AltGr)
    if (g_modToggle == MOD_ALT) {
        RegisterHotKey(hWnd, HOTKEY_ID_TOGGLE_ALTGR,
                       MOD_CONTROL | MOD_ALT,
                       g_vkToggle);
    }

    UpdateMicLabel();
    UpdateTooltip();
    SaveSettings();
}

void ReRegisterSoundHotkey(HWND hWnd)
{
    UnregisterHotKey(hWnd, HOTKEY_ID_SOUND);
    UnregisterHotKey(hWnd, HOTKEY_ID_SOUND_ALTGR);

    // основной хоткей
    RegisterHotKey(hWnd, HOTKEY_ID_SOUND, g_modSoundToggle, g_vkSoundToggle);

    // для чистого Alt регистрируем AltGr-вариант
    if (g_modSoundToggle == MOD_ALT) {
        RegisterHotKey(hWnd, HOTKEY_ID_SOUND_ALTGR,
                       MOD_CONTROL | MOD_ALT,
                       g_vkSoundToggle);
    }

    UpdateSoundLabel();
    UpdateTooltip();
    SaveSettings();
}

// ---------- Автозапуск ----------

void SetAutostart(int enable)
{
    HKEY hKey;
    LONG res = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_WRITE,
        &hKey
    );
    if (res != ERROR_SUCCESS) return;

    if (enable) {
        RegSetValueExA(hKey, "MicPlus", 0, REG_SZ, (const BYTE*)g_exePath, (DWORD)(lstrlenA(g_exePath) + 1));
    } else {
        RegDeleteValueA(hKey, "MicPlus");
    }

    RegCloseKey(hKey);
}

// ---------- Меню и обработка ----------

void ShowTrayMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    HMENU hHotkeysMenu = CreatePopupMenu();
    HMENU hSettingsMenu = CreatePopupMenu();
    HMENU hLangMenu = CreatePopupMenu();
	HMENU hIconMenu = CreatePopupMenu();

    // PTT mode
    UpdatePttModeLabel();
    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_STRING |
                         (g_pttMode ? MF_CHECKED : 0),
                ID_TRAY_PTT_MODE, g_labelPttMode);

    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    // Mic toggle
    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_STRING |
                         (!g_muted ? MF_CHECKED : 0),
                ID_TRAY_MIC_TOGGLE, g_labelMicToggle);

    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    // Hotkeys

    UINT mb5Flags      = MF_STRING | ((g_pttButton == XBUTTON2 && g_vkPttCustom == 0) ? MF_CHECKED : 0);
    UINT customPttFlag = MF_STRING | ((g_vkPttCustom != 0) ? MF_CHECKED : 0);

    AppendMenuA(hHotkeysMenu, mb5Flags, ID_HK_PTT_MB5, T("MENU_PTT_MB5"));
    AppendMenuA(hHotkeysMenu, customPttFlag, ID_HK_PTT_CUSTOM, g_labelPttCustom);

    AppendMenuA(hHotkeysMenu, MF_SEPARATOR, 0, NULL);

    // Mic hotkeys
    UINT micF10Flags    = MF_STRING;
    UINT micCustomFlags = MF_STRING;
    if (g_modToggle == 0 && g_vkToggle == VK_F10)
        micF10Flags |= MF_CHECKED;
    else
        micCustomFlags |= MF_CHECKED;

    AppendMenuA(hHotkeysMenu, micF10Flags, ID_HK_MIC_F10,
                (g_lang == LANG_RU) ? "Переключение микрофона: F10" : "Mic: F10");
    AppendMenuA(hHotkeysMenu, micCustomFlags, ID_HK_MIC_CUSTOM,
                (g_lang == LANG_RU) ? "Назначить клавишу/сочетание" : "Mic: Custom...");

    AppendMenuA(hHotkeysMenu, MF_SEPARATOR, 0, NULL);

    // Sound hotkeys
    UINT sndDefFlags    = MF_STRING;
    UINT sndCustomFlags = MF_STRING;
    if (g_modSoundToggle == MOD_ALT && g_vkSoundToggle == VK_F9)
        sndDefFlags |= MF_CHECKED;
    else
        sndCustomFlags |= MF_CHECKED;

    AppendMenuA(hHotkeysMenu, sndDefFlags, ID_HK_SOUND_DEFAULT,
                (g_lang == LANG_RU) ? "Глобальный звук: Alt+F9" : "Sound: Alt+F9");
    AppendMenuA(hHotkeysMenu, sndCustomFlags, ID_HK_SOUND_CUSTOM,
                (g_lang == LANG_RU) ? "Назначить клавишу/сочетание" : "Sound: Custom...");

    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_POPUP,
                (UINT_PTR)hHotkeysMenu, T("MENU_HOTKEYS"));

    // Settings подменю

    // Language
    UINT enFlags = MF_STRING | ((g_lang == LANG_EN) ? MF_CHECKED : 0);
    UINT ruFlags = MF_STRING | ((g_lang == LANG_RU) ? MF_CHECKED : 0);
    AppendMenuA(hLangMenu, enFlags, ID_LANG_EN, "English");
    AppendMenuA(hLangMenu, ruFlags, ID_LANG_RU, "Русский");

    AppendMenuA(hSettingsMenu, MF_POPUP, (UINT_PTR)hLangMenu, T("MENU_LANGUAGE"));
	
		
	// Цвет иконки приложения подменю
	UINT iconAutoFlags  = MF_STRING | ((g_iconMode == 0) ? MF_CHECKED : 0);
    UINT iconBlackFlags = MF_STRING | ((g_iconMode == 1) ? MF_CHECKED : 0);
    UINT iconWhiteFlags = MF_STRING | ((g_iconMode == 2) ? MF_CHECKED : 0);
	
   AppendMenuA(hIconMenu, iconAutoFlags,  ID_SETTINGS_ICON_AUTO,
                (g_lang == LANG_RU) ? "Система" : "Auto");
    AppendMenuA(hIconMenu, iconBlackFlags, ID_SETTINGS_ICON_BLACK,
                (g_lang == LANG_RU) ? "Белый" : "Black");
    AppendMenuA(hIconMenu, iconWhiteFlags, ID_SETTINGS_ICON_WHITE,
                (g_lang == LANG_RU) ? "Чёрный" : "White");

    AppendMenuA(hSettingsMenu, MF_POPUP, (UINT_PTR)hIconMenu,
                (g_lang == LANG_RU) ? "Цвет иконки приложения" : "Icon color");           

	// Разделитель между языком и цветом
    AppendMenuA(hSettingsMenu, MF_SEPARATOR, 0, NULL);

    // Autostart
    UINT autoFlags = MF_STRING | (g_autoStart ? MF_CHECKED : 0);
    AppendMenuA(hSettingsMenu, autoFlags, ID_SETTINGS_AUTOSTART, T("MENU_AUTOSTART"));

    // Mic sounds
    UINT micSoundFlags = MF_STRING | (g_enableMicSounds ? MF_CHECKED : 0);
    AppendMenuA(hSettingsMenu, micSoundFlags, ID_SETTINGS_MIC_SOUNDS, T("MENU_MIC_SOUNDS"));

    // PTT sounds
    UINT pttSoundFlags = MF_STRING | (g_enablePTTSounds ? MF_CHECKED : 0);
    AppendMenuA(hSettingsMenu, pttSoundFlags, ID_SETTINGS_PTT_SOUNDS, T("MENU_PTT_SOUNDS"));

    // Balloon
    UINT balloonFlags = MF_STRING | (g_enableBalloonTips ? MF_CHECKED : 0);
    AppendMenuA(hSettingsMenu, balloonFlags, ID_SETTINGS_BALLOON, T("MENU_BALLOON"));
		

    InsertMenuA(hMenu, -1, MF_BYPOSITION | MF_POPUP,
                (UINT_PTR)hSettingsMenu, T("MENU_SETTINGS"));

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
    if (nCode == HC_ACTION && g_pttMode) {
        MSLLHOOKSTRUCT *p = (MSLLHOOKSTRUCT *)lParam;

        if (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP) {
            UINT btn = HIWORD(p->mouseData); // XBUTTON1 / XBUTTON2

            if (btn == g_pttButton) {
                if (wParam == WM_XBUTTONDOWN) {
                    SetMicStatePTT(0);
                    PlayPTTSound(1);
                } else if (wParam == WM_XBUTTONUP) {
                    SetMicStatePTT(1);
                    PlayPTTSound(0);
                }
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
                    SetMicStatePTT(0);
                    PlayPTTSound(1);
                }
            } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                if (g_pttKeyDown) {
                    g_pttKeyDown = 0;
                    SetMicStatePTT(1);
                    PlayPTTSound(0);
                }
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// ---------- Окно ----------

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
                SetMicStateGlobal(1); // при включении PTT сразу выключаем микрофон
            }
            SaveSettings();
            UpdatePttModeLabel();
            UpdateTooltip();
            break;
			
	case ID_SETTINGS_ICON_AUTO:   // «Система»
    g_iconMode = 0;
    SaveSettings();
    UpdateIconTheme();
    UpdateTooltip();
    break;

	case ID_SETTINGS_ICON_BLACK:  // «Чёрный»
    g_iconMode = 1;
    SaveSettings();
    UpdateIconTheme();
    UpdateTooltip();
    break;

	case ID_SETTINGS_ICON_WHITE:  // «Белый»
    g_iconMode = 2;
    SaveSettings();
    UpdateIconTheme();
    UpdateTooltip();
    break;
			
	case WM_SETTINGCHANGE:
    // если режим Авто — обновляем цвет по новой системной теме
    if (g_iconMode == 0) {
        UpdateIconTheme();
        UpdateTooltip();
    }
    break;

        case ID_TRAY_MIC_TOGGLE:
            ToggleMic();
            break;

        case ID_TRAY_SOUND_TOGGLE:
            ToggleSystemSound();
            break;

        case ID_HK_PTT_MB5:
            g_pttButton   = XBUTTON2;
            g_vkPttCustom = 0;
            g_pttKeyDown  = 0;
            SaveSettings();
            UpdatePttCustomLabel();
            UpdatePttModeLabel();
            UpdateTooltip();
            break;

        case ID_HK_PTT_CUSTOM:
        {
            UINT mods = 0, vk = 0;
if (CaptureHotkeyGeneric(hWnd, "MSG_TOGGLE_SELECT_TEXT", "MSG_TOGGLE_SELECT_TITLE",
                         &mods, &vk)) {
    g_modToggle       = mods;
    g_vkToggle        = vk;
    g_toggleCustomSet = 1;
    ReRegisterMicHotkey(hWnd);
}
            break;
        }

        case ID_HK_MIC_F10:
            g_modToggle = 0;
            g_vkToggle  = VK_F10;
            g_toggleCustomSet = 0;
            ReRegisterMicHotkey(hWnd);
            break;

	case ID_HK_MIC_CUSTOM:
		{
			UINT mods = 0, vk = 0;
			if (CaptureHotkeyGeneric(hWnd,
                             "MSG_TOGGLE_SELECT_TEXT",
                             "MSG_TOGGLE_SELECT_TITLE",
                             &mods, &vk))
		{
        g_modToggle       = mods;
        g_vkToggle        = vk;
        g_toggleCustomSet = 1;
        ReRegisterMicHotkey(hWnd);
    }
    break;
}

        case ID_HK_SOUND_DEFAULT:
            g_modSoundToggle = MOD_ALT;
            g_vkSoundToggle  = VK_F9;
            ReRegisterSoundHotkey(hWnd);
            break;

        case ID_HK_SOUND_CUSTOM:
        {
            UINT mods = 0, vk = 0;
            if (CaptureHotkeyGeneric(hWnd, "MSG_SOUND_SELECT_TEXT", "MSG_SOUND_SELECT_TITLE",
                                     &mods, &vk)) {
                g_modSoundToggle = mods;
                g_vkSoundToggle  = vk;
                ReRegisterSoundHotkey(hWnd);
            }
            break;
        }

        // Settings
        case ID_LANG_EN:
            g_lang = LANG_EN;
            SaveSettings();
            UpdateMicLabel();
            UpdateSoundLabel();
            UpdatePttCustomLabel();
            UpdatePttModeLabel();
            UpdateTooltip();
            break;

        case ID_LANG_RU:
            g_lang = LANG_RU;
            SaveSettings();
            UpdateMicLabel();
            UpdateSoundLabel();
            UpdatePttCustomLabel();
            UpdatePttModeLabel();
            UpdateTooltip();
            break;

        case ID_SETTINGS_AUTOSTART:
            g_autoStart = !g_autoStart;
            SetAutostart(g_autoStart);
            SaveSettings();
            break;

        case ID_SETTINGS_MIC_SOUNDS:
            g_enableMicSounds = !g_enableMicSounds;
            SaveSettings();
            break;

        case ID_SETTINGS_PTT_SOUNDS:
            g_enablePTTSounds = !g_enablePTTSounds;
            SaveSettings();
            break;

        case ID_SETTINGS_BALLOON:
            g_enableBalloonTips = !g_enableBalloonTips;
            SaveSettings();
            break;

        case ID_TRAY_EXIT:
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_HOTKEY:
        if (wParam == HOTKEY_ID_TOGGLE) {
            ToggleMic();
        } else if (wParam == HOTKEY_ID_SOUND) {
            ToggleSystemSound();
        }
        break;

    case WM_DESTROY:
        break;
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

// ---------- WinMain ----------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    HANDLE hMutex = CreateMutexA(
        NULL,
        TRUE,
        "MicPlusSingleInstanceMutex"
    );

    if (hMutex == NULL) {
    } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return 0;
    }

    g_hInst = hInstance;

    LoadSettings();

    if (g_modToggle == 0 && (g_vkToggle == VK_F10)) {
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

    RegisterClassExA(&wc);

    g_hMainWnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "MicPlusHidden",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        NULL, hInstance, NULL
    );
    if (!g_hMainWnd) {
        if (hMutex) {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        return 1;
    }

          AddTrayIcon(g_hMainWnd);

    // Устанавливаем хуки
    g_hMouseHook    = SetWindowsHookExA(WH_MOUSE_LL,    LowLevelMouseProc,    g_hInst, 0);
    g_hKeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, g_hInst, 0);
    if (!g_hMouseHook || !g_hKeyboardHook) {
        MessageBoxA(NULL, T("MSG_ERR_HOOK"), "MicPlus", MB_ICONERROR);
        if (g_hMouseHook) {
            UnhookWindowsHookEx(g_hMouseHook);
            g_hMouseHook = NULL;
        }
        if (g_hKeyboardHook) {
            UnhookWindowsHookEx(g_hKeyboardHook);
            g_hKeyboardHook = NULL;
        }
        RemoveTrayIcon();
        if (hMutex) {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        return 1;
    }

    // Регистрируем хоткеи
    if (!RegisterHotKey(g_hMainWnd, HOTKEY_ID_TOGGLE, g_modToggle, g_vkToggle)) {
        MessageBoxA(NULL, T("MSG_ERR_HOTKEY"), "MicPlus", MB_ICONERROR);
    }
    if (!RegisterHotKey(g_hMainWnd, HOTKEY_ID_SOUND, g_modSoundToggle, g_vkSoundToggle)) {
        MessageBoxA(NULL, T("MSG_ERR_HOTKEY"), "MicPlus", MB_ICONERROR);
    }

    // Основной цикл сообщений
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    // При выходе гарантированно включаем микрофон
    SetMicStateGlobal(0);

    // Чистим хуки
    if (g_hMouseHook) {
        UnhookWindowsHookEx(g_hMouseHook);
        g_hMouseHook = NULL;
    }
    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
        g_hKeyboardHook = NULL;
    }

    // Снимаем хоткеи
    UnregisterHotKey(g_hMainWnd, HOTKEY_ID_TOGGLE);
    UnregisterHotKey(g_hMainWnd, HOTKEY_ID_SOUND);

    // Удаляем трей-иконку
    RemoveTrayIcon();

    // Сохраняем настройки
    SaveSettings();

    // Закрываем mutex
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return 0;
}
