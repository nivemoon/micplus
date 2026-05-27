#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#define WM_APP_TRAY       (WM_APP + 1)

// IDs для основного меню
#define ID_TRAY_PTT_MODE      2001
#define ID_TRAY_MIC_MUTE      2002
#define ID_TRAY_MIC_UNMUTE    2003
#define ID_TRAY_MIC_TOGGLE    2004
#define ID_TRAY_EXIT          2005

// IDs для подменю Hotkeys
#define ID_HK_TOGGLE_F9       2101
#define ID_HK_TOGGLE_F10      2102
#define ID_HK_TOGGLE_CUSTOM   2103

// ID для RegisterHotKey
#define HOTKEY_ID_TOGGLE      1   // глобальный toggle

HINSTANCE g_hInst;
HHOOK g_hMouseHook;
NOTIFYICONDATAA g_nid;

int g_muted = 0;          // 0 = mic ON, 1 = mic OFF
int g_pttMode = 0;        // 1 = PTT mode включён

HICON g_hIconOn = NULL;
HICON g_hIconOff = NULL;

char g_iniPath[MAX_PATH];

// Текущий виртуальный код клавиши для toggle
UINT g_vkToggle = VK_F10;

// Подписи для меню Microphone
char g_labelMicMute[64];
char g_labelMicUnmute[64];
char g_labelMicToggle[64];

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

    char buf[16];

    GetPrivateProfileStringA("General", "PTTMode", "0", buf, sizeof(buf), g_iniPath);
    g_pttMode = (buf[0] == '1') ? 1 : 0;

    GetPrivateProfileStringA("General", "ToggleVK", "121", buf, sizeof(buf), g_iniPath); // 121 = VK_F10
    g_vkToggle = (UINT)atoi(buf);
    if (g_vkToggle == 0) g_vkToggle = VK_F10;
}

void SaveSettings(void)
{
    char buf[16];

    wsprintfA(buf, "%d", g_pttMode ? 1 : 0);
    WritePrivateProfileStringA("General", "PTTMode", buf, g_iniPath);

    wsprintfA(buf, "%u", g_vkToggle);
    WritePrivateProfileStringA("General", "ToggleVK", buf, g_iniPath);
}

// ---------- Вспомогательные функции ----------

void RunMicCtl(int mute)
{
    char args[4];
    wsprintfA(args, "%d", mute ? 1 : 0);

    ShellExecuteA(NULL, "open", "micctl.exe", args, NULL, SW_HIDE);
}

void VkToName(UINT vk, char *out, int outSize)
{
    // Простейшее имя для F9/F10, иначе — "VK xx"
    switch (vk) {
    case VK_F9:
        lstrcpynA(out, "F9", outSize);
        break;
    case VK_F10:
        lstrcpynA(out, "F10", outSize);
        break;
    default:
        wsprintfA(out, "VK %u", vk);
        break;
    }
}

void UpdateMicMenuLabels(void)
{
    char keyName[32];
    VkToName(g_vkToggle, keyName, sizeof(keyName));

    wsprintfA(g_labelMicMute,   "Mute mic (%s)", keyName);
    lstrcpyA(g_labelMicUnmute,  "Unmute mic");
    wsprintfA(g_labelMicToggle, "Toggle mic (%s)", keyName);
}

void UpdateTooltip(void)
{
    if (!g_nid.hWnd) return;

    g_nid.uFlags = NIF_ICON | NIF_TIP;

    char keyName[32];
    VkToName(g_vkToggle, keyName, sizeof(keyName));

    if (g_muted) {
        g_nid.hIcon = g_hIconOff;
        wsprintfA(g_nid.szTip, "MicPlus: MIC OFF (PTT XButton2, Toggle %s)", keyName);
    } else {
        g_nid.hIcon = g_hIconOn;
        wsprintfA(g_nid.szTip, "MicPlus: MIC ON (PTT XButton2, Toggle %s)", keyNa
