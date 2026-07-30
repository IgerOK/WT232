// ============================================================================================2554====
// WT232 Terminal v0.3.1 (Flow Control Help + UI Fixes) UTF-16 LE BOM
// ====================================================================================================

#ifndef WINVER
#define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <wchar.h>
#include <setupapi.h>
#include <wctype.h>
#include <stdio.h>
#include <commdlg.h>
#include <shellapi.h>

// ====================================================================================================
// Константы и макросы
// ====================================================================================================

#define APP_VERSION L"v0.3.1"
#define MAX_PORT_NAME       64
#define RX_BUF_SIZE         4096
#define MAX_HISTORY         30
#define MAX_SCRIPT_LINES    500
#define MAX_LINE_LEN        256
#define MACRO_LABEL_LEN     64
#define MACRO_CMD_LEN       256
#define MAX_MACRO_TITLE_LEN 64
#define DUMP_LINE_SIZE      16

#define FLOW_NONE           0
#define FLOW_XONXOFF        1
#define FLOW_RTSCCTS        2
#define FLOW_RS485          3

#define TX_MODE_HEX         0
#define TX_MODE_TEXT        1
#define RX_MODE_TEXT        0
#define RX_MODE_DUMP        1

#define TIMER_RECONNECT_ID      2001
#define TIMER_SCRIPT_ID         2004
#define TIMER_MACRO_SCRIPT_ID   2005

#define WM_RX_DATA_READY    (WM_APP + 1)
#define WM_TX_TICK          (WM_APP + 2)

#define ECHO_COLOR          RGB(255, 0, 0)
#define HEX_ECHO_COLOR      RGB(0, 0, 0)

// ====================================================================================================
// ID элементов управления
// ====================================================================================================

#define IDC_COMBO_PORT          1001
#define IDC_COMBO_BAUD          1002
#define IDC_COMBO_DATABITS      1011
#define IDC_COMBO_PARITY        1012
#define IDC_COMBO_STOPBITS      1013
#define IDC_COMBO_FLOW          1004
#define IDC_BTN_INFO            1007
#define IDC_BTN_FLOW_INFO       1039 // Новая кнопка справки Flow Control
#define IDC_EDIT_RX             1009
#define IDC_COMBO_TX            1020
#define IDC_COMBO_SUFFIX        1021
#define IDC_BTN_SUFFIX_INFO     1022
#define IDC_BTN_SEND            1006
#define IDC_BTN_CLEAR           1014
#define IDC_CHK_ECHO            1018
#define IDC_COMBO_ENC           1015
#define IDC_CHK_TX_HEX          1024
#define IDC_CHK_DUMP            1023
#define IDC_BTN_ABOUT           1025
#define IDC_EDIT_DELAY          1027
#define IDC_CHK_REPEAT          1028
#define IDC_COMBO_FONT_SIZE     1036
#define IDC_EDIT_MACRO_TITLE    1037
#define IDC_CHK_TOPMOST         1038 // 'Всегда поверх'

#define IDC_EDIT_SCRIPT_PATH    1029
#define IDC_BTN_LOAD_SCRIPT     1030
#define IDC_BTN_RUN_SCRIPT      1031
#define IDC_BTN_SCRIPT_INFO     1032

#define MACRO_BANK_COUNT        5
#define MACROS_PER_BANK         24
#define MACRO_COLS              4
#define MACRO_ROWS              6

#define IDC_BTN_MACRO_BASE              1050
#define IDC_BTN_MACRO_MODE              (IDC_BTN_MACRO_BASE + MACROS_PER_BANK)
#define IDC_BTN_MACRO_DISPLAY           (IDC_BTN_MACRO_BASE + MACROS_PER_BANK + 1)
#define IDC_STATIC_MACRO_STATUS         (IDC_BTN_MACRO_BASE + MACROS_PER_BANK + 2)
#define IDC_MACRO_SCRIPT_PATH           2001
#define IDC_MACRO_BTN_SCRIPT_INFO       2002
#define IDC_MACRO_BTN_LOAD              2003
#define IDC_MACRO_BTN_RUN               2004
#define IDC_MACRO_BTN_EDIT              2005

#define IDC_EDIT_MACRO_NAME     3001
#define IDC_EDIT_MACRO_CMD      3002
#define IDC_BTN_MACRO_SAVE      3003
#define IDC_BTN_MACRO_CANCEL    3004

#define IDC_BTN_OK              1008
#define IDC_BTN_CANCEL          1009

// ====================================================================================================
// Структуры данных
// ====================================================================================================

typedef struct {
    wchar_t label[MACRO_LABEL_LEN];
    wchar_t command[MACRO_CMD_LEN];
} MacroSlot;

typedef struct {
    wchar_t command[MAX_LINE_LEN];
    DWORD delay;
} ScriptItem;

typedef struct {
    wchar_t vid[8];
    wchar_t pid[8];
    wchar_t serial[64];
} com_identity_t;

typedef struct {
    int lastBaudrate;
    wchar_t lastPortName[MAX_PORT_NAME];
    BOOL isWaitingReconnect;
    com_identity_t targetDevice;
} com_session_t;

typedef struct {
    const wchar_t* name;
    UINT codepage;
} EncodingEntry;

typedef struct {
    BYTE data;
    DWORD len;
} RxDataPacket;

// ====================================================================================================
// Глобальные переменные
// ====================================================================================================

HWND g_hComboPort = NULL;
HWND g_hComboBaud = NULL;
HWND g_hComboDataBits = NULL;
HWND g_hComboParity = NULL;
HWND g_hComboStopBits = NULL;
HWND g_hComboFlow = NULL;
HWND g_hBtnInfo = NULL;
HWND g_hwndSettings = NULL;
HWND g_hwndTerminal = NULL;
HWND g_hEditRx = NULL;
HWND g_hComboTx = NULL;
HWND g_hComboSuffix = NULL;
HWND g_hBtnSuffixInfo = NULL;
HWND g_hBtnSend = NULL;
HWND g_hBtnClear = NULL;
HWND g_hChkEcho = NULL;
HWND g_hComboEnc = NULL;
HWND g_hChkTxHex = NULL;
HWND g_hChkDump = NULL;
HWND g_hBtnAbout = NULL;
HWND g_hEditDelay = NULL;
HWND g_hChkRepeat = NULL;
HWND g_hComboFontSize = NULL;
HWND g_hEditScriptPath = NULL;
HWND g_hBtnLoadScript = NULL;
HWND g_hBtnRunScript = NULL;
HWND g_hBtnScriptInfo = NULL;
HWND g_hChkTopMost = NULL;

HWND g_hMacroBankBtns[MACRO_BANK_COUNT] = {0};

HANDLE g_hPort = INVALID_HANDLE_VALUE;
static WNDPROC g_pfnOrigEditProc = NULL;
static HFONT g_hMonoFont = NULL;
static HFONT g_hBtnFont = NULL;
static HFONT g_hEditFont = NULL;
static HFONT g_hTitleFont = NULL;
static int g_fontSize = 10;

static int g_txMode = TX_MODE_TEXT;
static int g_rxMode = RX_MODE_TEXT;
static BOOL g_txHexEcho = FALSE;
static volatile BOOL g_isAutoSending = FALSE;
static volatile BOOL g_isPortOpen = FALSE;

static CRITICAL_SECTION g_csComm;
static CRITICAL_SECTION g_csRx;
static HANDLE g_hRxThread = NULL;
static HANDLE g_hTxThread = NULL;
static HANDLE g_hRxStopEvent = NULL;
static HANDLE g_hTxStopEvent = NULL;
static DWORD g_txIntervalMs = 1000;

static wchar_t g_usbInstanceId[512] = {0};
static wchar_t g_regBuffer[512] = {0};
static wchar_t g_infoReport[8192] = {0};
static wchar_t g_szTitle[256] = {0};
static wchar_t g_wTxtBuf[1024] = {0};
static BYTE g_binBuf[1024] = {0};

static BYTE g_rxRawBuf[RX_BUF_SIZE];
static DWORD g_rxRawLen = 0;
static DWORD g_lastRenderedLen = 0;

static com_session_t g_session = {0};
static wchar_t g_iniPath[MAX_PATH] = {0};
static wchar_t g_iniBackupPath[MAX_PATH] = {0};

static ScriptItem g_scriptItems[MAX_SCRIPT_LINES];
static int g_scriptCount = 0;
static int g_scriptCurrentIndex = 0;
static BOOL g_isScriptRunning = FALSE;
static BOOL g_scriptHasStopMarker = FALSE;

static MacroSlot g_macroBanks[MACRO_BANK_COUNT][MACROS_PER_BANK];
static wchar_t g_macroBankTitles[MACRO_BANK_COUNT][MAX_MACRO_TITLE_LEN] = {0};
static BOOL g_macroBankLoaded[MACRO_BANK_COUNT] = {0};
static HWND g_hwndMacroPads[MACRO_BANK_COUNT] = {0};
static HWND g_hwndMacroEdit = NULL;
static int g_editingSlot = -1;
static int g_editingBank = -1;
static BOOL g_editMode = FALSE;
static BOOL g_showCommand = FALSE;

static ScriptItem g_macroScriptItems[MACRO_BANK_COUNT][MAX_SCRIPT_LINES];
static int g_macroScriptCount[MACRO_BANK_COUNT] = {0};
static int g_macroScriptIndex[MACRO_BANK_COUNT] = {0};
static BOOL g_macroScriptRunning[MACRO_BANK_COUNT] = {0};
static BOOL g_macroScriptHasStop[MACRO_BANK_COUNT] = {0};

typedef struct {
    int bankIndex;
    HWND hModeBtn;
    HWND hDispBtn;
    HWND hTitleEdit;
    HWND hStatusLbl;
    HWND hScriptPath;
    HWND hScriptInfoBtn;
    HWND hLoadBtn;
    HWND hRunBtn;
    HWND hEditBtn;
    HFONT hTitleFont;
} MacroPadCtx;

typedef struct {
    int bankIndex;
    int slotIndex;
    HWND hEditName;
    HWND hEditCmd;
    HWND hBtnSave;
    HWND hBtnCancel;
} MacroEditCtx;

// Кодировки (без ASCII/CP1252 по памяти пользователя)
static const EncodingEntry g_encodings[] = {
    { L"UTF-8", 65001 },
    { L"CP1251", 1251 },
    { L"CP866", 866 },
    { L"KOI8-R", 20866 },
};
static const int g_encodingCount = sizeof(g_encodings) / sizeof(g_encodings[0]);

static const wchar_t g_aboutText[] =
    L"WT232 Terminal " APP_VERSION L"\r\n"
    L"=========================================\r\n"
    L"Особенности:\r\n"
    L" • Работа с COM-портами (USB CDC/ACM).\r\n"
    L" • Режимы отображения HEX/TEXT/DUMP, инлайн-HEX (`XX`).\r\n"
    L" • Макросы (5 банков x 24 ячейки).\r\n"
    L" • Синхронный автоповтор (Threads).\r\n"
    L"\r\n"
    L"Поддержка кодировок:\r\n"
    L" • Кириллица поддерживается в CP1251 или CP866.\r\n"
    L" • UTF-8 подходит для современных устройств.\r\n"
    L"=========================================\r\n"
    L"MIT License\r\n"
    L"Copyright (c) 2026 IgerOK\r\n"
    L"https://github.com/IgerOK/WT232\r\n";

// ====================================================================================================
// Прототипы функций
// ====================================================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK TerminalWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK InfoWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK TxEditSubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK MacroPadWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK MacroEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL CALLBACK SetFontCallback(HWND hWndChild, LPARAM lParam);
BOOL CALLBACK ApplyThemeCallback(HWND hWndChild, LPARAM lParam);

void init_com_ports(HWND hwndParent, BOOL showDetails, const wchar_t* saveTargetName);
void check_and_reconnect_search(wchar_t* outFoundPortName, BOOL* pIsFound);
void com_close(void);
BOOL com_open(const wchar_t* portName, int baudrate);
void get_advanced_usb_descriptors(HDEVINFO hDevInfo, SP_DEVINFO_DATA* pDevInfo, const wchar_t* instanceId,
                                  wchar_t* outVid, wchar_t* outPid, wchar_t* outSerial, wchar_t* outMfg,
                                  wchar_t* outProduct, wchar_t* outRawSerial, wchar_t* outCdcInfo);
void com_send_ui(HWND hwndParent);
void update_terminal_title(const wchar_t* portName, int baudrate);
void render_rx_buffer(BOOL appendMode);
UINT get_selected_codepage(void);
int get_selected_tx_mode(void);
BOOL is_echo_enabled(void);
BOOL is_tx_hex_echo_enabled(void);
void append_rx_text_colored(const wchar_t* text, COLORREF color);
void append_echo_hex_dump(const BYTE* buf, DWORD len);
void add_to_history(const wchar_t* msg);
DWORD parse_text_with_inline_hex(const wchar_t* src, BYTE* dst, DWORD dstMax, UINT codepage);
int hex_char_val(wchar_t c);
HFONT CreateMonoFont(int height);
void update_rx_mode_ui(void);
void LoadScriptFile(const wchar_t* path);
void StopScript(void);
void RunNextScriptCommand(void);
void ApplyFontSize(int height);
void show_about_dialog(HWND hParent);
void LoadMacroBank(int bankIndex);
void SaveMacroBank(int bankIndex);
void SendMacroCommand(int bankIndex, int slotIndex);
void UpdateMacroButtons(int bankIndex);
void UpdateMacroButtonTitle(int bankIndex);
void ShowMacroPad(HWND hParent, int bankIndex);
void LayoutButtons(HWND hwnd);
void CloseMacroEdit(void);
void SaveMacroFromEdit(void);
void ShowMacroEditWindow(HWND hParent, int bankIndex, int slotIndex);
void SaveMacroPadPosition(int bankIndex, int x, int y);
void LoadMacroPadPosition(int bankIndex, int* px, int* py);
void SaveAllMacroWindowsState(void);
void ApplyThemeToWindow(HWND hwnd);
void ResetMacroLoadedFlags(void);
void LoadMacroBankTitles(void);
void SaveMacroBankTitle(int bankIndex);
void UpdateAllMacroButtonTitles(void);
void InitIniPaths(void);
BOOL ReadIniString(const wchar_t* section, const wchar_t* key, wchar_t* out, int maxLen, const wchar_t* defVal);
int ReadIniInt(const wchar_t* section, const wchar_t* key, int defVal);
void WriteIniString(const wchar_t* section, const wchar_t* key, const wchar_t* val);
void WriteIniInt(const wchar_t* section, const wchar_t* key, int val);
BOOL ValidateIni(void);
BOOL ReadAllIni(void);
void WriteAllIni(void);
void CreateDefaultIni(void);
void GetMacroScriptPath(int bankIndex, wchar_t* outPath, int maxLen);
void LoadMacroScript(int bankIndex, const wchar_t* path);
void StopMacroScript(int bankIndex);
void RunNextMacroScriptCommand(int bankIndex);
void UpdateMacroScriptUI(HWND hwnd, int bankIndex);

DWORD WINAPI RxThreadProc(LPVOID lpParam);
DWORD WINAPI TxThreadProc(LPVOID lpParam);
void StartRxTxThreads(HWND hwndTerm);
void StopRxTxThreads(void);
void PrepareAutoSendData(void);
void DoAutoSendTick(HWND hwndTerm);

// Тема
typedef struct {
    COLORREF bgColor;
    COLORREF fgColor;
    COLORREF btnBg;
    COLORREF btnFg;
    COLORREF editBg;
    COLORREF editFg;
} ThemeColors;

static ThemeColors g_theme = {
    RGB(240, 240, 245), RGB(0, 0, 0), RGB(230, 230, 240), RGB(0, 0, 0), RGB(255, 255, 255), RGB(0, 0, 0)
};
static ThemeColors* g_pTheme = &g_theme;

// ====================================================================================================
// Вспомогательные функции
// ====================================================================================================

HFONT CreateMonoFont(int height) {
    return CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Lucida Console");
}

static HFONT CreateTitleFont(int height) {
    return CreateFontW(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

void ApplyFontSize(int height) {
    if (g_hMonoFont) DeleteObject(g_hMonoFont);
    g_hMonoFont = CreateMonoFont(height);
    if (g_hEditRx) SendMessage(g_hEditRx, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
    if (g_hComboTx) SendMessage(g_hComboTx, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
    if (g_hComboSuffix) SendMessage(g_hComboSuffix, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
    if (g_hEditDelay) SendMessage(g_hEditDelay, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
    if (g_hEditScriptPath) SendMessage(g_hEditScriptPath, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
    if (g_hComboFontSize) SendMessage(g_hComboFontSize, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
    if (g_hComboEnc) SendMessage(g_hComboEnc, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);

    g_fontSize = -height * 72 / 96;
    if (g_fontSize <= 0) g_fontSize = 10;
}

void update_rx_mode_ui(void) {
}

int hex_char_val(wchar_t c) {
    if (c >= L'0' && c <= L'9') return c - L'0';
    if (c >= L'A' && c <= L'F') return c - L'A' + 10;
    if (c >= L'a' && c <= L'f') return c - L'a' + 10;
    return -1;
}

void com_close(void) {
    StopRxTxThreads();
    EnterCriticalSection(&g_csComm);
    if (g_hPort != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hPort);
        g_hPort = INVALID_HANDLE_VALUE;
    }
    g_isPortOpen = FALSE;
    LeaveCriticalSection(&g_csComm);

    EnterCriticalSection(&g_csRx);
    g_rxRawLen = 0;
    g_lastRenderedLen = 0;
    ZeroMemory(g_rxRawBuf, sizeof(g_rxRawBuf));
    LeaveCriticalSection(&g_csRx);
}

void update_terminal_title(const wchar_t* portName, int baudrate) {
    const wchar_t* szParity = L"N";
    int pIdx = (int)SendMessageW(g_hComboParity, CB_GETCURSEL, 0, 0);
    switch (pIdx) {
        case 1: szParity = L"E"; break; case 2: szParity = L"O"; break;
        case 3: szParity = L"M"; break; case 4: szParity = L"S"; break;
    }
    wchar_t szDB[4] = L"8";
    int dIdx = (int)SendMessageW(g_hComboDataBits, CB_GETCURSEL, 0, 0);
    switch (dIdx) {
        case 0: wcscpy(szDB, L"5"); break; case 1: wcscpy(szDB, L"6"); break;
        case 2: wcscpy(szDB, L"7"); break;
    }
    wchar_t szSB[4] = L"1";
    int sIdx = (int)SendMessageW(g_hComboStopBits, CB_GETCURSEL, 0, 0);
    switch (sIdx) {
        case 1: wcscpy(szSB, L"1.5"); break; case 2: wcscpy(szSB, L"2"); break;
    }
    wchar_t szFlow[64] = {0};
    int fIdx = (int)SendMessageW(g_hComboFlow, CB_GETCURSEL, 0, 0);
    if (fIdx != CB_ERR) SendMessageW(g_hComboFlow, CB_GETLBTEXT, fIdx, (LPARAM)szFlow);
    else wcscpy(szFlow, L"?");

    ZeroMemory(g_szTitle, sizeof(g_szTitle));
    swprintf(g_szTitle, sizeof(g_szTitle)/sizeof(wchar_t),
             L"WT232 Terminal " APP_VERSION L" - [%ls | %d bps | %ls-%ls-%ls | %ls]",
             portName, baudrate, szDB, szParity, szSB, szFlow);
}

BOOL com_open(const wchar_t* portName, int baudrate) {
    wchar_t szPath[MAX_PORT_NAME + 8] = {0};
    swprintf(szPath, sizeof(szPath)/sizeof(wchar_t), L"\\\\.\\%ls", portName);

    HANDLE hNewPort = CreateFileW(szPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (hNewPort == INVALID_HANDLE_VALUE) return FALSE;

    PurgeComm(hNewPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
    EscapeCommFunction(hNewPort, CLRDTR); EscapeCommFunction(hNewPort, CLRRTS);
    Sleep(10);
    EscapeCommFunction(hNewPort, SETDTR); EscapeCommFunction(hNewPort, SETRTS);

    DCB dcb;
    ZeroMemory(&dcb, sizeof(DCB)); dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(hNewPort, &dcb)) { CloseHandle(hNewPort); return FALSE; }

    dcb.BaudRate = baudrate;
    int dbIdx = (int)SendMessageW(g_hComboDataBits, CB_GETCURSEL, 0, 0);
    switch (dbIdx) {
        case 0: dcb.ByteSize = 5; break; case 1: dcb.ByteSize = 6; break;
        case 2: dcb.ByteSize = 7; break; default: dcb.ByteSize = 8; break;
    }

    int parIdx = (int)SendMessageW(g_hComboParity, CB_GETCURSEL, 0, 0);
    switch (parIdx) {
        case 1: dcb.Parity = EVENPARITY; break; case 2: dcb.Parity = ODDPARITY; break;
        case 3: dcb.Parity = MARKPARITY; break; case 4: dcb.Parity = SPACEPARITY; break;
        default: dcb.Parity = NOPARITY; break;
    }

    int sbIdx = (int)SendMessageW(g_hComboStopBits, CB_GETCURSEL, 0, 0);
    switch (sbIdx) {
        case 1: dcb.StopBits = ONE5STOPBITS; break; case 2: dcb.StopBits = TWOSTOPBITS; break;
        default: dcb.StopBits = ONESTOPBIT; break;
    }

    int flowMode = (int)SendMessageW(g_hComboFlow, CB_GETCURSEL, 0, 0);
    dcb.fOutxCtsFlow = FALSE; dcb.fInX = FALSE; dcb.fOutX = FALSE;
    if (flowMode == FLOW_RS485) dcb.fRtsControl = RTS_CONTROL_TOGGLE;
    else if (flowMode == FLOW_RTSCCTS) { dcb.fOutxCtsFlow = TRUE; dcb.fRtsControl = RTS_CONTROL_HANDSHAKE; }
    else if (flowMode == FLOW_XONXOFF) { dcb.fInX = TRUE; dcb.fOutX = TRUE; dcb.fRtsControl = RTS_CONTROL_DISABLE; }
    else dcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(hNewPort, &dcb)) { CloseHandle(hNewPort); return FALSE; }

    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD; timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0; timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 50;
    SetCommTimeouts(hNewPort, &timeouts);

    EnterCriticalSection(&g_csComm);
    g_hPort = hNewPort;
    g_isPortOpen = TRUE;
    LeaveCriticalSection(&g_csComm);

    EnterCriticalSection(&g_csRx);
    g_rxRawLen = 0;
    g_lastRenderedLen = 0;
    ZeroMemory(g_rxRawBuf, sizeof(g_rxRawBuf));
    LeaveCriticalSection(&g_csRx);

    return TRUE;
}

UINT get_selected_codepage(void) {
    int idx = (int)SendMessageW(g_hComboEnc, CB_GETCURSEL, 0, 0);
    if (idx >= 0 && idx < g_encodingCount) return g_encodings[idx].codepage;
    return 1251;
}

int get_selected_tx_mode(void) { return g_txMode; }

BOOL is_echo_enabled(void) {
    if (!g_hChkEcho) return FALSE;
    return (SendMessageW(g_hChkEcho, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

BOOL is_tx_hex_echo_enabled(void) {
    if (!g_hChkTxHex) return FALSE;
    return (SendMessageW(g_hChkTxHex, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

void append_rx_text_colored(const wchar_t* text, COLORREF color) {
    if (!g_hEditRx || !text || wcslen(text) == 0) return;
    int len = GetWindowTextLengthW(g_hEditRx);
    SendMessageW(g_hEditRx, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    CHARFORMAT2W cf; ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf); cf.dwMask = CFM_COLOR; cf.crTextColor = color;
    SendMessageW(g_hEditRx, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageW(g_hEditRx, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(g_hEditRx, WM_VSCROLL, SB_BOTTOM, 0);
}

static void append_echo_text(const wchar_t* text) { append_rx_text_colored(text, ECHO_COLOR); }

void append_echo_hex_dump(const BYTE* buf, DWORD len) {
    if (!buf || len == 0) return;
    wchar_t hexStr[2048] = {0};
    int pos = 0;
    hexStr[pos++] = L' '; hexStr[pos++] = L' '; hexStr[pos++] = L' ';
    hexStr[pos++] = L'`';
    for (DWORD i = 0; i < len && pos < 2040; i++) {
        if (i > 0) hexStr[pos++] = L' ';
        static const wchar_t hexChars[] = L"0123456789ABCDEF";
        hexStr[pos++] = hexChars[(buf[i] >> 4) & 0x0F];
        hexStr[pos++] = hexChars[buf[i] & 0x0F];
    }
    hexStr[pos++] = L'`';
    hexStr[pos] = L'\0';
    append_rx_text_colored(hexStr, HEX_ECHO_COLOR);
}

// ====================================================================================================
// RENDER_RX_BUFFER
// ====================================================================================================

void render_rx_buffer(BOOL appendMode) {
    EnterCriticalSection(&g_csRx);
    if (g_rxRawLen == 0) { LeaveCriticalSection(&g_csRx); return; }

    UINT cp = get_selected_codepage();
    DWORD dataLen = g_rxRawLen;

    BOOL forceFullRedraw = (g_rxMode == RX_MODE_DUMP);
    DWORD startIdx = (appendMode && !forceFullRedraw) ? g_lastRenderedLen : 0;

    if (forceFullRedraw || !appendMode) {
        if (g_hEditRx) SetWindowTextW(g_hEditRx, L"");
        startIdx = 0;
    } else {
        if (startIdx >= dataLen) {
            LeaveCriticalSection(&g_csRx);
            return;
        }
    }

    wchar_t wBuf[RX_BUF_SIZE * 8 + 1] = {0};
    int wLen = 0;
    int maxW = (int)(sizeof(wBuf)/sizeof(wchar_t)) - 10;

    if (g_rxMode == RX_MODE_DUMP) {
        for (DWORD offset = startIdx; offset < dataLen && wLen < maxW; offset += DUMP_LINE_SIZE) {
            DWORD lineEnd = offset + DUMP_LINE_SIZE;
            if (lineEnd > dataLen) lineEnd = dataLen;

            wLen += swprintf(wBuf + wLen, 12, L"%08X: ", offset);

            for (DWORD i = offset; i < lineEnd; i++) {
                wLen += swprintf(wBuf + wLen, 4, L"%02X ", g_rxRawBuf[i]);
            }
            for (DWORD i = lineEnd; i < offset + DUMP_LINE_SIZE; i++) {
                wLen += swprintf(wBuf + wLen, 4, L"   ");
            }

            wLen += swprintf(wBuf + wLen, 4, L" | ");

            wchar_t asciiLine[DUMP_LINE_SIZE + 1] = {0};
            int charsConverted = 0;
            if (lineEnd > offset) {
                charsConverted = MultiByteToWideChar(cp, 0,
                    (LPCSTR)(g_rxRawBuf + offset), lineEnd - offset, asciiLine, DUMP_LINE_SIZE);
            }

            for (int i = 0; i < DUMP_LINE_SIZE; i++) {
                wchar_t ch = L'.';
                if (i < charsConverted) {
                    ch = asciiLine[i];
                    if (ch < 0x20) ch = L'.';
                }
                wBuf[wLen++] = ch;
            }
            wBuf[wLen++] = L'\r';
            wBuf[wLen++] = L'\n';
        }
        g_lastRenderedLen = dataLen;

    } else {
        DWORD bytesToRender = dataLen - startIdx;
        if (bytesToRender > 0) {
            int convertedLen = MultiByteToWideChar(cp, 0,
                (LPCSTR)(g_rxRawBuf + startIdx), bytesToRender, wBuf, maxW);

            if (convertedLen <= 0) {
                for (DWORD i = 0; i < bytesToRender && wLen < maxW; i++)
                    wLen += swprintf(wBuf + wLen, 4, L"%02X ", g_rxRawBuf[startIdx + i]);
            } else {
                wLen = convertedLen;
            }
            g_lastRenderedLen = dataLen;
        }
    }

    wBuf[wLen] = L'\0';
    LeaveCriticalSection(&g_csRx);

    if (wLen > 0) {
        COLORREF textColor = (g_rxMode == RX_MODE_DUMP) ? g_pTheme->fgColor : RGB(0, 0, 200);
        append_rx_text_colored(wBuf, textColor);
    }
}

// ====================================================================================================
// ADD_TO_HISTORY & TXEDITSUBPROC
// ====================================================================================================

void add_to_history(const wchar_t* msg) {
    if (!g_hComboTx || !msg || wcslen(msg) == 0) return;
    int idx = (int)SendMessageW(g_hComboTx, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)msg);
    if (idx != CB_ERR) SendMessageW(g_hComboTx, CB_DELETESTRING, (WPARAM)idx, 0);
    SendMessageW(g_hComboTx, CB_INSERTSTRING, 0, (LPARAM)msg);
    int count = (int)SendMessageW(g_hComboTx, CB_GETCOUNT, 0, 0);
    while (count > MAX_HISTORY) {
        SendMessageW(g_hComboTx, CB_DELETESTRING, (WPARAM)(count - 1), 0); count--;
    }
}

LRESULT CALLBACK TxEditSubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_KEYDOWN && wp == VK_RETURN) {
        HWND hParent = GetParent(hwnd);
        if (hParent) PostMessageW(hParent, WM_COMMAND, MAKEWPARAM(IDC_BTN_SEND, BN_CLICKED), (LPARAM)g_hBtnSend);
        return 0;
    }
    return CallWindowProcW(g_pfnOrigEditProc, hwnd, msg, wp, lp);
}

// ====================================================================================================
// PARSE_TEXT_WITH_INLINE_HEX
// ====================================================================================================

DWORD parse_text_with_inline_hex(const wchar_t* src, BYTE* dst, DWORD dstMax, UINT codepage) {
    if (!src || !dst || dstMax == 0) return 0;
    DWORD binLen = 0;
    size_t len = wcslen(src);
    wchar_t textAccum[1024];
    int textAccumLen = 0;

#define FLUSH_TEXT_ACCUM() do { \
    if (textAccumLen > 0 && binLen < dstMax) { \
        int conv = WideCharToMultiByte(codepage, 0, textAccum, textAccumLen, \
            (LPSTR)(dst + binLen), dstMax - binLen, NULL, NULL); \
        if (conv > 0) binLen += conv; \
        textAccumLen = 0; \
    } \
} while(0)

    size_t i = 0;
    while (i < len && binLen < dstMax) {
        if (src[i] == L'`') {
            size_t closePos = 0;
            for (size_t j = i + 1; j < len; j++) {
                if (src[j] == L'`') { closePos = j; break; }
            }
            if (closePos > 0 && closePos > i + 1) {
                BYTE tmpHex[256]; DWORD tmpHexLen = 0; BOOL valid = TRUE;
                for (size_t k = i + 1; k < closePos && tmpHexLen < sizeof(tmpHex); k++) {
                    wchar_t ch = src[k];
                    if (ch == L' ' || ch == L'\t') continue;
                    int hv = hex_char_val(ch);
                    if (hv < 0) { valid = FALSE; break; }
                    size_t nextK = k + 1;
                    while (nextK < closePos && (src[nextK] == L' ' || src[nextK] == L'\t')) nextK++;
                    if (nextK >= closePos) { valid = FALSE; break; }
                    int lv = hex_char_val(src[nextK]);
                    if (lv < 0) { valid = FALSE; break; }
                    tmpHex[tmpHexLen++] = (BYTE)((hv << 4) | lv);
                    k = nextK;
                }
                if (valid && tmpHexLen > 0) {
                    FLUSH_TEXT_ACCUM();
                    for (DWORD b = 0; b < tmpHexLen && binLen < dstMax; b++) dst[binLen++] = tmpHex[b];
                    i = closePos + 1; continue;
                }
            }
            if (textAccumLen < 1023) textAccum[textAccumLen++] = src[i];
            i++;
        } else {
            if (textAccumLen < 1023) textAccum[textAccumLen++] = src[i];
            i++;
        }
    }
    FLUSH_TEXT_ACCUM();
#undef FLUSH_TEXT_ACCUM
    return binLen;
}

// ====================================================================================================
// COM_SEND_UI
// ====================================================================================================

void com_send_ui(HWND hwndParent) {
    if (g_hPort == INVALID_HANDLE_VALUE) return;

    ZeroMemory(g_wTxtBuf, sizeof(g_wTxtBuf));
    GetWindowTextW(g_hComboTx, g_wTxtBuf, 1023);
    size_t len = wcslen(g_wTxtBuf);
    if (len == 0) {
        MessageBoxW(hwndParent, L"Нет данных для отправки!", L"Ошибка", MB_ICONWARNING|MB_OK);
        return;
    }

    ZeroMemory(g_binBuf, sizeof(g_binBuf));
    DWORD binLen = 0;
    int txMode = get_selected_tx_mode();
    UINT cp = get_selected_codepage();

    if (txMode == TX_MODE_HEX) {
        size_t i = 0;
        while (i < len && binLen < sizeof(g_binBuf)) {
            if (g_wTxtBuf[i] == L' ' || g_wTxtBuf[i] == L',' || g_wTxtBuf[i] == L'-' ||
                g_wTxtBuf[i] == L'\r' || g_wTxtBuf[i] == L'\n') { i++; continue; }
            if (i + 1 < len) {
                int hv = hex_char_val(g_wTxtBuf[i]);
                int lv = hex_char_val(g_wTxtBuf[i+1]);
                if (hv >= 0 && lv >= 0) { g_binBuf[binLen++] = (BYTE)((hv << 4) | lv); i += 2; }
                else i++;
            } else i++;
        }
    } else {
        binLen = parse_text_with_inline_hex(g_wTxtBuf, g_binBuf, sizeof(g_binBuf), cp);
        wchar_t suffixBuf[256] = {0};
        GetWindowTextW(g_hComboSuffix, suffixBuf, 255);
        if (wcslen(suffixBuf) > 0 && binLen < sizeof(g_binBuf)) {
            DWORD suffixLen = parse_text_with_inline_hex(suffixBuf, g_binBuf + binLen, sizeof(g_binBuf) - binLen, cp);
            binLen += suffixLen;
        }
    }

    if (binLen == 0) {
        MessageBoxW(hwndParent, L"Нет данных для отправки!\r\nПроверьте режим (TEXT/HEX) и кодировку.", L"Ошибка", MB_ICONERROR|MB_OK);
        return;
    }

    EnterCriticalSection(&g_csComm);
    if (g_hPort == INVALID_HANDLE_VALUE) {
        LeaveCriticalSection(&g_csComm);
        MessageBoxW(hwndParent, L"Порт закрыт!", L"Ошибка", MB_ICONERROR|MB_OK);
        return;
    }

    OVERLAPPED ovWrite = {0};
    ovWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!ovWrite.hEvent) {
        LeaveCriticalSection(&g_csComm);
        MessageBoxW(hwndParent, L"Не удалось создать событие!", L"Ошибка", MB_ICONERROR|MB_OK);
        return;
    }

    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(g_hPort, g_binBuf, binLen, &bytesWritten, &ovWrite);
    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            DWORD waitRes = WaitForSingleObject(ovWrite.hEvent, 5000);
            if (waitRes == WAIT_OBJECT_0) { GetOverlappedResult(g_hPort, &ovWrite, &bytesWritten, FALSE); ok = TRUE; }
            else { CancelIo(g_hPort); ok = FALSE; }
        }
    }
    CloseHandle(ovWrite.hEvent);
    LeaveCriticalSection(&g_csComm);

    if (!ok) {
        DWORD err = GetLastError();
        wchar_t errMsg[256]; swprintf(errMsg, 256, L"Ошибка отправки! Код: %d", err);
        MessageBoxW(hwndParent, errMsg, L"Сбой", MB_ICONERROR|MB_OK);
        return;
    }

    if (is_echo_enabled()) {
        append_echo_text(g_wTxtBuf);
        if (txMode == TX_MODE_TEXT) {
            wchar_t suffixBuf[256] = {0};
            GetWindowTextW(g_hComboSuffix, suffixBuf, 255);
            if (wcslen(suffixBuf) > 0) append_echo_text(suffixBuf);
        }
        if (is_tx_hex_echo_enabled()) {
            append_echo_hex_dump(g_binBuf, binLen);
        }
        append_echo_text(L"\r\n");
    }

    add_to_history(g_wTxtBuf);
    SetWindowTextW(g_hComboTx, g_wTxtBuf);
    SetFocus(g_hComboTx);
    SendMessageW(g_hComboTx, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
}

// ====================================================================================================
// Потоки RX/TX
// ====================================================================================================

void StartRxTxThreads(HWND hwndTerm) {
    if (!g_hRxStopEvent) g_hRxStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hTxStopEvent) g_hTxStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ResetEvent(g_hRxStopEvent); ResetEvent(g_hTxStopEvent);

    if (!g_hRxThread) g_hRxThread = CreateThread(NULL, 0, RxThreadProc, hwndTerm, 0, NULL);
    if (!g_hTxThread) g_hTxThread = CreateThread(NULL, 0, TxThreadProc, hwndTerm, 0, NULL);
}

void StopRxTxThreads(void) {
    if (g_hRxStopEvent) SetEvent(g_hRxStopEvent);
    if (g_hTxStopEvent) SetEvent(g_hTxStopEvent);
    if (g_hRxThread) { WaitForSingleObject(g_hRxThread, 1000); CloseHandle(g_hRxThread); g_hRxThread = NULL; }
    if (g_hTxThread) { WaitForSingleObject(g_hTxThread, 1000); CloseHandle(g_hTxThread); g_hTxThread = NULL; }
}

DWORD WINAPI RxThreadProc(LPVOID lpParam) {
    HWND hwndTerm = (HWND)lpParam;
    OVERLAPPED ov = {0}; ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    BYTE buf[512];

    while (TRUE) {
        if (WaitForSingleObject(g_hRxStopEvent, 0) == WAIT_OBJECT_0) break;

        EnterCriticalSection(&g_csComm); HANDLE hPortLocal = g_hPort; LeaveCriticalSection(&g_csComm);
        if (hPortLocal == INVALID_HANDLE_VALUE) { Sleep(50); continue; }

        DWORD bytesRead = 0;
        BOOL res = ReadFile(hPortLocal, buf, sizeof(buf), &bytesRead, &ov);
        if (!res && GetLastError() == ERROR_IO_PENDING) {
            HANDLE waits[2] = { ov.hEvent, g_hRxStopEvent };
            DWORD wr = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
            if (wr == WAIT_OBJECT_0) GetOverlappedResult(hPortLocal, &ov, &bytesRead, FALSE);
            else { CancelIo(hPortLocal); break; }
        } else if (!res) { Sleep(50); ResetEvent(ov.hEvent); continue; }
        ResetEvent(ov.hEvent);

        if (bytesRead > 0) {
            EnterCriticalSection(&g_csRx);
            if (g_rxRawLen + bytesRead <= RX_BUF_SIZE) {
                memcpy(g_rxRawBuf + g_rxRawLen, buf, bytesRead);
                g_rxRawLen += bytesRead;
                PostMessage(hwndTerm, WM_RX_DATA_READY, 0, (LPARAM)bytesRead);
            } else {
                g_rxRawLen = 0; g_lastRenderedLen = 0; ZeroMemory(g_rxRawBuf, sizeof(g_rxRawBuf));
            }
            LeaveCriticalSection(&g_csRx);
        }
    }
    CloseHandle(ov.hEvent); return 0;
}

DWORD WINAPI TxThreadProc(LPVOID lpParam) {
    HWND hwndTerm = (HWND)lpParam;
    while (TRUE) {
        DWORD waitRes = WaitForSingleObject(g_hTxStopEvent, g_txIntervalMs);
        if (waitRes == WAIT_OBJECT_0) break;
        if (IsWindow(hwndTerm)) PostMessage(hwndTerm, WM_TX_TICK, 0, 0);
    }
    return 0;
}

static BYTE g_autoSendBuf[1024];
static DWORD g_autoSendLen = 0;
static BOOL g_autoSendEcho = FALSE;
static BOOL g_autoSendHexEcho = FALSE;
static wchar_t g_autoSendEchoText[1280];

void PrepareAutoSendData(void) {
    ZeroMemory(g_wTxtBuf, sizeof(g_wTxtBuf));
    GetWindowTextW(g_hComboTx, g_wTxtBuf, 1023);
    size_t len = wcslen(g_wTxtBuf);
    g_autoSendEcho = FALSE; g_autoSendHexEcho = FALSE;
    g_autoSendEchoText[0] = L'\0'; g_autoSendLen = 0;
    if (len == 0) return;

    ZeroMemory(g_binBuf, sizeof(g_binBuf)); DWORD binLen = 0;
    int txMode = get_selected_tx_mode();
    if (txMode == TX_MODE_HEX) {
        size_t i = 0;
        while (i < len && binLen < sizeof(g_binBuf)) {
            if (g_wTxtBuf[i] == L' ' || g_wTxtBuf[i] == L',' || g_wTxtBuf[i] == L'-' ||
                g_wTxtBuf[i] == L'\r' || g_wTxtBuf[i] == L'\n') { i++; continue; }
            if (i + 1 < len) {
                int hv = hex_char_val(g_wTxtBuf[i]); int lv = hex_char_val(g_wTxtBuf[i+1]);
                if (hv >= 0 && lv >= 0) { g_binBuf[binLen++] = (BYTE)((hv << 4) | lv); i += 2; } else i++;
            } else { int hv = hex_char_val(g_wTxtBuf[i]); if (hv >= 0) g_binBuf[binLen++] = (BYTE)hv; i++; }
        }
    } else {
        UINT cp = get_selected_codepage();
        binLen = parse_text_with_inline_hex(g_wTxtBuf, g_binBuf, sizeof(g_binBuf), cp);
        wchar_t suffixBuf[256] = {0}; GetWindowTextW(g_hComboSuffix, suffixBuf, 255);
        if (wcslen(suffixBuf) > 0 && binLen < sizeof(g_binBuf)) {
            DWORD suffixLen = parse_text_with_inline_hex(suffixBuf, g_binBuf + binLen, sizeof(g_binBuf) - binLen, cp);
            binLen += suffixLen;
        }
    }

    if (binLen > 0 && binLen <= sizeof(g_autoSendBuf)) {
        memcpy(g_autoSendBuf, g_binBuf, binLen);
        g_autoSendLen = binLen;
        g_autoSendEcho = is_echo_enabled();
        g_autoSendHexEcho = is_tx_hex_echo_enabled();
        if (g_autoSendEcho) {
            wcsncpy(g_autoSendEchoText, g_wTxtBuf, 1023);
            wchar_t suffixBuf[256] = {0}; GetWindowTextW(g_hComboSuffix, suffixBuf, 255);
            if (wcslen(suffixBuf) > 0 && txMode == TX_MODE_TEXT) wcscat(g_autoSendEchoText, suffixBuf);
        }
    } else { g_autoSendLen = 0; }
}

void DoAutoSendTick(HWND hwndTerm) {
    EnterCriticalSection(&g_csComm);
    if (g_hPort == INVALID_HANDLE_VALUE || g_autoSendLen == 0) {
        LeaveCriticalSection(&g_csComm);
        if (g_hPort == INVALID_HANDLE_VALUE && g_isAutoSending)
            PostMessage(hwndTerm, WM_COMMAND, MAKEWPARAM(IDC_BTN_SEND, BN_CLICKED), 0);
        return;
    }

    OVERLAPPED ovWrite = {0}; ovWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    DWORD bytesWritten = 0;
    BOOL ok = WriteFile(g_hPort, g_autoSendBuf, g_autoSendLen, &bytesWritten, &ovWrite);
    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) { WaitForSingleObject(ovWrite.hEvent, 5000); GetOverlappedResult(g_hPort, &ovWrite, &bytesWritten, FALSE); }
    }
    if (ovWrite.hEvent) CloseHandle(ovWrite.hEvent);

    BOOL doEcho = g_autoSendEcho;
    BOOL doHexEcho = g_autoSendHexEcho;
    DWORD sendLen = g_autoSendLen;
    LeaveCriticalSection(&g_csComm);

    if (doEcho) {
        append_echo_text(g_autoSendEchoText);
        if (doHexEcho) append_echo_hex_dump(g_autoSendBuf, sendLen);
        append_echo_text(L"\r\n");
    }
}

// ====================================================================================================
// USB descriptors & COM search
// ====================================================================================================

void get_advanced_usb_descriptors(HDEVINFO hDevInfo, SP_DEVINFO_DATA* pDevInfo, const wchar_t* instanceId,
                                  wchar_t* outVid, wchar_t* outPid, wchar_t* outSerial,
                                  wchar_t* outMfg, wchar_t* outProduct, wchar_t* outRawSerial, wchar_t* outCdcInfo) {
    wcscpy(outVid, L"N/A"); wcscpy(outPid, L"N/A"); wcscpy(outSerial, L"N/A");
    wcscpy(outMfg, L"N/A"); wcscpy(outProduct, L"N/A"); wcscpy(outRawSerial, L"N/A");
    wcscpy(outCdcInfo, L"N/A (Standard Serial)");

    if (instanceId) {
        const wchar_t* pVid = wcsstr(instanceId, L"VID_"); if (pVid) { wcsncpy(outVid, pVid + 4, 4); outVid[4] = L'\0'; }
        const wchar_t* pPid = wcsstr(instanceId, L"PID_"); if (pPid) { wcsncpy(outPid, pPid + 4, 4); outPid[4] = L'\0'; }
        const wchar_t* pSlash = wcsrchr(instanceId, L'\\'); if (pSlash) { wcsncpy(outSerial, pSlash + 1, 63); outSerial[63] = L'\0'; }
    }

    static const GUID GUID_DEVINTERFACE_USB_DEVICE = {0xA5DCBF10,0x6530,0x11D2,{0x90,0x1F,0x00,0xC0,0x4F,0xB9,0x51,0xED}};
    HDEVINFO hUsbInfo = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_DEVICE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hUsbInfo != INVALID_HANDLE_VALUE) {
        SP_DEVINFO_DATA usbDevData; DWORD index = 0;
        while (1) {
            ZeroMemory(&usbDevData, sizeof(SP_DEVINFO_DATA)); usbDevData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (!SetupDiEnumDeviceInfo(hUsbInfo, index, &usbDevData)) break;
            ZeroMemory(g_usbInstanceId, sizeof(g_usbInstanceId));
            if (SetupDiGetDeviceInstanceIdW(hUsbInfo, &usbDevData, g_usbInstanceId, 512, NULL)) {
                const wchar_t* pPV = wcsstr(g_usbInstanceId, L"VID_");
                const wchar_t* pPP = wcsstr(g_usbInstanceId, L"PID_");
                if (pPV && pPP && wcsncmp(pPV+4, outVid, 4)==0 && wcsncmp(pPP+4, outPid, 4)==0) {
                    const wchar_t* pLS = wcsrchr(g_usbInstanceId, L'\\');
                    if (pLS) { wcsncpy(outRawSerial, pLS+1, 127); outRawSerial[127]=L'\0'; }

                    HKEY hUsbKey = SetupDiOpenDevRegKey(hUsbInfo, &usbDevData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (hUsbKey != INVALID_HANDLE_VALUE) {
        DWORD cb = sizeof(g_regBuffer);
        
        // === MFG ===
        // 1. Пытаемся прочитать кастомную строку производителя
        if (RegQueryValueExW(hUsbKey, L"busi_ManufacturerString", NULL, NULL, (LPBYTE)g_regBuffer, &cb) == ERROR_SUCCESS && wcslen(g_regBuffer) > 0) {
            wcsncpy(outMfg, g_regBuffer, 127); outMfg[127] = L'\0';
        } else {
            // 2. Фолбэк на UI Parent MFG
            cb = sizeof(g_regBuffer);
            if (RegQueryValueExW(hUsbKey, L"UIParentMFG", NULL, NULL, (LPBYTE)g_regBuffer, &cb) == ERROR_SUCCESS && wcslen(g_regBuffer) > 0) {
                wcsncpy(outMfg, g_regBuffer, 127); outMfg[127] = L'\0';
            } else {
                // 3. Фолбэк на SPDRP_MFG с фильтрацией
                DWORD sz = 0;
                if (SetupDiGetDeviceRegistryPropertyW(hUsbInfo, &usbDevData, SPDRP_MFG, NULL, (PBYTE)g_regBuffer, sizeof(g_regBuffer), &sz)) {
                    // Фильтруем системные заглушки
                    if (!wcsstr(g_regBuffer, L"Майкрософт") && !wcsstr(g_regBuffer, L"хост") && !wcsstr(g_regBuffer, L"(Стандартный")) {
                        wcsncpy(outMfg, g_regBuffer, 127); outMfg[127] = L'\0';
                    } else {
                        wcscpy(outMfg, L"PLANAR-S0");
                    }
                }
            }
        }

        // === PRODUCT ===
        cb = sizeof(g_regBuffer);
        // 1. Пытаемся прочитать кастомную строку продукта
        if (RegQueryValueExW(hUsbKey, L"busi_ProductString", NULL, NULL, (LPBYTE)g_regBuffer, &cb) == ERROR_SUCCESS && wcslen(g_regBuffer) > 0) {
            wcsncpy(outProduct, g_regBuffer, 127); outProduct[127] = L'\0';
        } else {
            // 2. Фолбэк на свойство 24 (Device Description из USB дескриптора)
            DWORD sz = 0;
            if (SetupDiGetDeviceRegistryPropertyW(hUsbInfo, &usbDevData, 24, NULL, (PBYTE)g_regBuffer, sizeof(g_regBuffer), &sz) && wcslen(g_regBuffer) > 0) {
                wcsncpy(outProduct, g_regBuffer, 127); outProduct[127] = L'\0';
            } else {
                // 3. Фолбэк на SPDRP_DEVICEDESC с фильтрацией
                if (SetupDiGetDeviceRegistryPropertyW(hUsbInfo, &usbDevData, SPDRP_DEVICEDESC, NULL, (PBYTE)g_regBuffer, sizeof(g_regBuffer), &sz)) {
                    // Фильтруем системные заглушки
                    if (!wcsstr(g_regBuffer, L"Составное") && !wcsstr(g_regBuffer, L"Composite")) {
                        wcsncpy(outProduct, g_regBuffer, 127); outProduct[127] = L'\0';
                    } else {
                        wcscpy(outProduct, L"SSI-ABZ");
                    }
                }
            }
        }

        RegCloseKey(hUsbKey);
    }
                    break;
                }
            }
            index++;
        }
        SetupDiDestroyDeviceInfoList(hUsbInfo);
    }

    HKEY hDevKey = SetupDiOpenDevRegKey(hDevInfo, pDevInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (hDevKey != INVALID_HANDLE_VALUE) {
        wchar_t dummy={0}; DWORD cb=sizeof(dummy);
        if (RegQueryValueExW(hDevKey, L"PortName", NULL, NULL, (LPBYTE)&dummy, &cb)==ERROR_SUCCESS)
            swprintf(outCdcInfo, 127, L"USB CDC/ACM Virtual COM");
        RegCloseKey(hDevKey);
    }
}

LRESULT CALLBACK InfoWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static HWND hEditLog = NULL;
    switch (msg) {
        case WM_CREATE: {
            RECT rc; GetClientRect(hwnd, &rc);
            hEditLog = CreateWindowExW(0, L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,
                                       0, 0, rc.right, rc.bottom, hwnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hEditLog, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lp;
            if (pCreate && pCreate->lpCreateParams) SetWindowTextW(hEditLog, (const wchar_t*)pCreate->lpCreateParams);
            return 0;
        }
        case WM_SIZE: { if (hEditLog) { MoveWindow(hEditLog, 0, 0, LOWORD(lp), HIWORD(lp), TRUE); InvalidateRect(hEditLog, NULL, TRUE); } return 0; }
        case WM_CLOSE: { DestroyWindow(hwnd); return 0; }
        case WM_CTLCOLOREDIT: case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->editBg); SetTextColor(hdc, g_pTheme->editFg);
            static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->editBg); return (LRESULT)hBrush;
        }
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wp; RECT rc; GetClientRect(hwnd, &rc);
            static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->bgColor); FillRect(hdc, &rc, hBrush); return 1;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void show_about_dialog(HWND hParent) {
    HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class", L"О программе",
                                 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 480, 520,
                                 hParent, NULL, GetModuleHandle(NULL), (LPVOID)g_aboutText);
    if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); }
}

void init_com_ports(HWND hwndParent, BOOL showDetails, const wchar_t* saveTargetName) {
    if (!g_hComboPort) return;
    if (!showDetails) SendMessage(g_hComboPort, CB_RESETCONTENT, 0, 0);

    static const GUID GUID_DEVCLASS_PORTS = {0x4D36E978,0xE325,0x11CE,{0xBF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};
    HDEVINFO hDevInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) return;

    SP_DEVINFO_DATA devInfo; DWORD i = 0;
    ZeroMemory(g_infoReport, sizeof(g_infoReport)); size_t reportPos = 0; int portsFoundCount = 0;

    while (1) {
        ZeroMemory(&devInfo, sizeof(SP_DEVINFO_DATA)); devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiEnumDeviceInfo(hDevInfo, i, &devInfo)) break;

        wchar_t fn[256]={0}, iid[256]={0}; DWORD sz=0;
        BOOL hasName = SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfo, SPDRP_FRIENDLYNAME, NULL, (PBYTE)fn, sizeof(fn), &sz);
        BOOL hasId = SetupDiGetDeviceInstanceIdW(hDevInfo, &devInfo, iid, sizeof(iid)/sizeof(wchar_t), &sz);

        if (hasName && hasId) {
            wchar_t* pStart = wcsstr(fn, L"(COM"); if (!pStart) pStart = wcsstr(fn, L" (COM");
            if (pStart) {
                if (*pStart == L' ') pStart++; pStart++;
                wchar_t* pEnd = wcschr(pStart, L')');
                if (pEnd) {
                    size_t len = (size_t)(pEnd - pStart);
                    if (len < MAX_PORT_NAME) {
                        wchar_t sn[MAX_PORT_NAME]={0}; wcsncpy(sn, pStart, len); sn[len]=L'\0';
                        if (!showDetails) SendMessageW(g_hComboPort, CB_ADDSTRING, 0, (LPARAM)sn);
                        portsFoundCount++;

                        wchar_t cv[16],cp[16],cs[64],cm[128],cpr[128],cr[128],cc[128];
                        get_advanced_usb_descriptors(hDevInfo, &devInfo, iid, cv,cp,cs,cm,cpr,cr,cc);

                        if (saveTargetName && wcscmp(sn, saveTargetName)==0) {
                            wcscpy(g_session.targetDevice.vid, cv); wcscpy(g_session.targetDevice.pid, cp); wcscpy(g_session.targetDevice.serial, cs);
                        }

                        if (showDetails) {
                            size_t w = swprintf(g_infoReport+reportPos, (sizeof(g_infoReport)/sizeof(wchar_t))-reportPos,
                                                L"Порт: %ls\r\nМетка: %ls\r\nVID: %ls | PID: %ls\r\nSerial: %ls\r\nMfg: %ls\r\nProduct: %ls\r\nRaw S/N: %ls\r\nClass: %ls\r\n-----------------------------------------\r\n\r\n",
                                                sn, fn, cv, cp, cs, cm, cpr, cr, cc);
                            if (w > 0) reportPos += w;
                        }
                    }
                }
            }
        }
        i++;
    }

    if (!showDetails && saveTargetName && wcslen(saveTargetName) > 0) {
        int idx = (int)SendMessageW(g_hComboPort, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)saveTargetName);
        if (idx != CB_ERR) SendMessage(g_hComboPort, CB_SETCURSEL, idx, 0);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    if (!showDetails && SendMessage(g_hComboPort, CB_GETCOUNT, 0, 0) > 0) {
        if (!saveTargetName || wcslen(saveTargetName) == 0) SendMessage(g_hComboPort, CB_SETCURSEL, 0, 0);
    }

    if (showDetails) {
        if (portsFoundCount == 0) MessageBoxW(hwndParent, L"Активные COM-порты не найдены!", L"Инфо", MB_ICONINFORMATION|MB_OK);
        else {
            HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class", L"Список устройств",
                                         WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 460, 360,
                                         hwndParent, NULL, GetModuleHandle(NULL), (LPVOID)g_infoReport);
            if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); }
        }
    }
}

void check_and_reconnect_search(wchar_t* outFoundPortName, BOOL* pIsFound) {
    *pIsFound = FALSE;
    static const GUID GUID_DEVCLASS_PORTS = {0x4D36E978,0xE325,0x11CE,{0xBF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18}};
    HDEVINFO hDevInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) return;

    SP_DEVINFO_DATA devInfo; DWORD i = 0;
    while (1) {
        ZeroMemory(&devInfo, sizeof(SP_DEVINFO_DATA)); devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiEnumDeviceInfo(hDevInfo, i, &devInfo)) break;

        wchar_t fn[256]={0}, iid[256]={0}; DWORD sz=0;
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfo, SPDRP_FRIENDLYNAME, NULL, (PBYTE)fn, sizeof(fn), &sz) &&
            SetupDiGetDeviceInstanceIdW(hDevInfo, &devInfo, iid, sizeof(iid)/sizeof(wchar_t), &sz)) {

            wchar_t cv[16],cp[16],cs[64],dm[128],dp[128],dr[128],dc[128];
            get_advanced_usb_descriptors(hDevInfo, &devInfo, iid, cv,cp,cs,dm,dp,dr,dc);

            if (wcscmp(cv, g_session.targetDevice.vid)==0 && wcscmp(cp, g_session.targetDevice.pid)==0 &&
                wcscmp(cs, g_session.targetDevice.serial)==0) {

                wchar_t* pS = wcsstr(fn, L"(COM"); if (!pS) pS = wcsstr(fn, L" (COM");
                if (pS) {
                    if (*pS == L' ') pS++; pS++;
                    wchar_t* pE = wcschr(pS, L')');
                    if (pE) { size_t l = (size_t)(pE - pS); wcsncpy(outFoundPortName, pS, l); outFoundPortName[l]=L'\0'; *pIsFound = TRUE; break; }
                }
            }
        }
        i++;
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
}

// ====================================================================================================
// Scripts
// ====================================================================================================

void LoadScriptFile(const wchar_t* path) {
    FILE* f = _wfopen(path, L"r"); if (!f) return;
    g_scriptCount = 0; g_scriptHasStopMarker = FALSE;
    wchar_t line[MAX_LINE_LEN]; DWORD defaultDelay = 0; BOOL firstLine = TRUE;

    while (fgetws(line, MAX_LINE_LEN, f)) {
        size_t len = wcslen(line);
        while (len > 0 && (line[len-1] == L'\n' || line[len-1] == L'\r')) line[--len] = L'\0';
        if (len == 0) continue;

        if (firstLine) {
            wchar_t* endPtr; long val = wcstol(line, &endPtr, 10);
            if (*endPtr == L'\0' && val >= 0) { defaultDelay = (DWORD)val; firstLine = FALSE; continue; }
            firstLine = FALSE;
        }
        if (wcsncmp(line, L"#DELAY ", 6) == 0) { wchar_t* numPart = line + 6; while (*numPart == L' ') numPart++; defaultDelay = _wtol(numPart); continue; }
        if (wcscmp(line, L"#STOP") == 0) { g_scriptHasStopMarker = TRUE; continue; }
        if (line[0] == L'#') continue;

        if (g_scriptCount < MAX_SCRIPT_LINES) {
            wcscpy(g_scriptItems[g_scriptCount].command, line);
            g_scriptItems[g_scriptCount].delay = defaultDelay; g_scriptCount++;
        }
    }
    fclose(f);
    wchar_t pathBuf[MAX_PATH]; wcsncpy(pathBuf, path, MAX_PATH-1); pathBuf[MAX_PATH-1] = L'\0';
    SetWindowTextW(g_hEditScriptPath, pathBuf); g_scriptCurrentIndex = 0;
}

void StopScript(void) { KillTimer(g_hwndTerminal, TIMER_SCRIPT_ID); g_isScriptRunning = FALSE; SetWindowTextW(g_hBtnRunScript, L"RUN"); }

void RunNextScriptCommand(void) {
    if (g_scriptCount == 0 || g_hPort == INVALID_HANDLE_VALUE) return;
    int currentIndex = g_scriptCurrentIndex;
    wchar_t* cmd = g_scriptItems[currentIndex].command;
    DWORD delay = g_scriptItems[currentIndex].delay;

    SetWindowTextW(g_hComboTx, cmd);
    com_send_ui(g_hwndTerminal);

    g_scriptCurrentIndex++;
    if (g_scriptCurrentIndex >= g_scriptCount) {
        if (g_scriptHasStopMarker) { StopScript(); return; } else g_scriptCurrentIndex = 0;
    }
    if (delay < 10) delay = 10;
    SetTimer(g_hwndTerminal, TIMER_SCRIPT_ID, delay, NULL);
}

// ====================================================================================================
// Macro System
// ====================================================================================================

void GetMacroScriptPath(int bankIndex, wchar_t* outPath, int maxLen) {
    outPath[0] = L'\0';
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (wcslen(g_macroBankTitles[bankIndex]) == 0) return;
    GetModuleFileNameW(NULL, outPath, maxLen);
    wchar_t* pSlash = wcsrchr(outPath, L'\\'); if (pSlash) *(pSlash + 1) = L'\0';
    wcscat(outPath, g_macroBankTitles[bankIndex]); wcscat(outPath, L".txt");
}

void LoadMacroScript(int bankIndex, const wchar_t* path) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    FILE* f = _wfopen(path, L"r"); if (!f) return;
    g_macroScriptCount[bankIndex] = 0; g_macroScriptHasStop[bankIndex] = FALSE;
    wchar_t line[MAX_LINE_LEN]; DWORD defaultDelay = 0; BOOL firstLine = TRUE;

    while (fgetws(line, MAX_LINE_LEN, f)) {
        size_t len = wcslen(line);
        while (len > 0 && (line[len-1] == L'\n' || line[len-1] == L'\r')) line[--len] = L'\0';
        if (len == 0) continue;

        if (firstLine) {
            wchar_t* endPtr; long val = wcstol(line, &endPtr, 10);
            if (*endPtr == L'\0' && val >= 0) { defaultDelay = (DWORD)val; firstLine = FALSE; continue; }
            firstLine = FALSE;
        }
        if (wcsncmp(line, L"#DELAY ", 6) == 0) { wchar_t* numPart = line + 6; while (*numPart == L' ') numPart++; defaultDelay = _wtol(numPart); continue; }
        if (wcscmp(line, L"#STOP") == 0) { g_macroScriptHasStop[bankIndex] = TRUE; continue; }
        if (line[0] == L'#') continue;

        if (g_macroScriptCount[bankIndex] < MAX_SCRIPT_LINES) {
            wcscpy(g_macroScriptItems[bankIndex][g_macroScriptCount[bankIndex]].command, line);
            g_macroScriptItems[bankIndex][g_macroScriptCount[bankIndex]].delay = defaultDelay;
            g_macroScriptCount[bankIndex]++;
        }
    }
    fclose(f); g_macroScriptIndex[bankIndex] = 0;
}

void StopMacroScript(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (g_hwndMacroPads[bankIndex]) KillTimer(g_hwndMacroPads[bankIndex], TIMER_MACRO_SCRIPT_ID);
    g_macroScriptRunning[bankIndex] = FALSE;
    UpdateMacroScriptUI(g_hwndMacroPads[bankIndex], bankIndex);
}

void RunNextMacroScriptCommand(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (g_macroScriptCount[bankIndex] == 0 || g_hPort == INVALID_HANDLE_VALUE) return;

    int idx = g_macroScriptIndex[bankIndex];
    wchar_t* cmd = g_macroScriptItems[bankIndex][idx].command;
    DWORD delay = g_macroScriptItems[bankIndex][idx].delay;

    BYTE binBuf[1024]; UINT cp = get_selected_codepage();
    DWORD binLen = parse_text_with_inline_hex(cmd, binBuf, sizeof(binBuf), cp);

    if (binLen > 0) {
        EnterCriticalSection(&g_csComm);
        OVERLAPPED ovWrite = {0}; ovWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        DWORD written = 0;
        BOOL ok = WriteFile(g_hPort, binBuf, binLen, &written, &ovWrite);
        if (!ok) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) { WaitForSingleObject(ovWrite.hEvent, 5000); GetOverlappedResult(g_hPort, &ovWrite, &written, FALSE); ok = TRUE; }
        }
        if (ovWrite.hEvent) CloseHandle(ovWrite.hEvent);
        LeaveCriticalSection(&g_csComm);

        if (ok) {
            if (is_echo_enabled() && g_hwndTerminal) {
                wchar_t echoMsg[MAX_LINE_LEN + 4]; swprintf(echoMsg, sizeof(echoMsg)/sizeof(wchar_t), L"%ls", cmd);
                append_echo_text(echoMsg);
                if (is_tx_hex_echo_enabled()) { append_echo_hex_dump(binBuf, binLen); }
                append_echo_text(L"\r\n");
            }
        } else { MessageBoxW(g_hwndMacroPads[bankIndex], L"Ошибка отправки!", L"Ошибка", MB_ICONERROR | MB_OK); }
    }

    g_macroScriptIndex[bankIndex]++;
    if (g_macroScriptIndex[bankIndex] >= g_macroScriptCount[bankIndex]) {
        if (g_macroScriptHasStop[bankIndex]) { StopMacroScript(bankIndex); return; }
        else g_macroScriptIndex[bankIndex] = 0;
    }
    if (delay < 10) delay = 10;
    if (g_hwndMacroPads[bankIndex]) SetTimer(g_hwndMacroPads[bankIndex], TIMER_MACRO_SCRIPT_ID, delay, NULL);
}

void UpdateMacroScriptUI(HWND hwnd, int bankIndex) {
    if (!hwnd || bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    HWND hRunBtn = GetDlgItem(hwnd, IDC_MACRO_BTN_RUN);
    if (hRunBtn) SetWindowTextW(hRunBtn, g_macroScriptRunning[bankIndex] ? L"STOP" : L"RUN");
}

void LoadMacroBankTitles(void) {
    for (int bankIndex = 0; bankIndex < MACRO_BANK_COUNT; bankIndex++) {
        wchar_t section[32]; swprintf(section, 32, L"MacroSlots_%d", bankIndex);
        wchar_t titleBuf[MAX_MACRO_TITLE_LEN] = {0};
        ReadIniString(section, L"BankTitle", titleBuf, MAX_MACRO_TITLE_LEN, L"");
        if (wcslen(titleBuf) > 0) wcscpy(g_macroBankTitles[bankIndex], titleBuf);
        else swprintf(g_macroBankTitles[bankIndex], MAX_MACRO_TITLE_LEN, L"M%d", bankIndex + 1);
    }
}

void SaveMacroBankTitle(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    wchar_t section[32]; swprintf(section, 32, L"MacroSlots_%d", bankIndex);
    WriteIniString(section, L"BankTitle", g_macroBankTitles[bankIndex]);
}

void LoadMacroBank(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (g_macroBankLoaded[bankIndex]) return;

    wchar_t section[32]; swprintf(section, 32, L"MacroSlots_%d", bankIndex);
    for (int i = 0; i < MACROS_PER_BANK; i++) {
        wchar_t keyLabel[32], keyCmd[32];
        swprintf(keyLabel, 32, L"Slot%d_Label", i); swprintf(keyCmd, 32, L"Slot%d_Cmd", i);
        wchar_t labelBuf[MACRO_LABEL_LEN] = {0}; wchar_t cmdBuf[MACRO_CMD_LEN] = {0};
        ReadIniString(section, keyLabel, labelBuf, MACRO_LABEL_LEN, L"");
        ReadIniString(section, keyCmd, cmdBuf, MACRO_CMD_LEN, L"");
        wcscpy(g_macroBanks[bankIndex][i].label, labelBuf);
        wcscpy(g_macroBanks[bankIndex][i].command, cmdBuf);
    }
    g_macroBankLoaded[bankIndex] = TRUE;
}

void SaveMacroBank(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    wchar_t section[32]; swprintf(section, 32, L"MacroSlots_%d", bankIndex);
    WritePrivateProfileStringW(section, NULL, NULL, g_iniPath);
    SaveMacroBankTitle(bankIndex);
    for (int i = 0; i < MACROS_PER_BANK; i++) {
        wchar_t keyLabel[32], keyCmd[32]; swprintf(keyLabel, 32, L"Slot%d_Label", i); swprintf(keyCmd, 32, L"Slot%d_Cmd", i);
        WriteIniString(section, keyLabel, g_macroBanks[bankIndex][i].label);
        WriteIniString(section, keyCmd, g_macroBanks[bankIndex][i].command);
    }
}

void SaveMacroPadPosition(int bankIndex, int x, int y) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    wchar_t section[32]; swprintf(section, 32, L"MacroPos_%d", bankIndex);
    WriteIniInt(section, L"X", x); WriteIniInt(section, L"Y", y);
}

void LoadMacroPadPosition(int bankIndex, int* px, int* py) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    wchar_t section[32]; swprintf(section, 32, L"MacroPos_%d", bankIndex);
    *px = ReadIniInt(section, L"X", -1); *py = ReadIniInt(section, L"Y", -1);
}

void SendMacroCommand(int bankIndex, int slotIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT || slotIndex < 0 || slotIndex >= MACROS_PER_BANK) return;
    if (g_hPort == INVALID_HANDLE_VALUE) { MessageBoxW(g_hwndMacroPads[bankIndex], L"Порт не открыт!", L"Ошибка", MB_ICONWARNING | MB_OK); return; }

    MacroSlot* slot = &g_macroBanks[bankIndex][slotIndex];
    if (wcslen(slot->command) == 0) return;

    BYTE binBuf[1024]; UINT cp = get_selected_codepage();
    DWORD binLen = parse_text_with_inline_hex(slot->command, binBuf, sizeof(binBuf), cp);

    if (binLen > 0) {
        EnterCriticalSection(&g_csComm);
        OVERLAPPED ovWrite = {0}; ovWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        DWORD written = 0;
        BOOL ok = WriteFile(g_hPort, binBuf, binLen, &written, &ovWrite);
        if (!ok) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) { WaitForSingleObject(ovWrite.hEvent, 5000); GetOverlappedResult(g_hPort, &ovWrite, &written, FALSE); ok = TRUE; }
        }
        if (ovWrite.hEvent) CloseHandle(ovWrite.hEvent);
        LeaveCriticalSection(&g_csComm);

        if (ok) {
            if (is_echo_enabled() && g_hwndTerminal) {
                wchar_t echoMsg[MACRO_CMD_LEN + 4]; swprintf(echoMsg, sizeof(echoMsg)/sizeof(wchar_t), L"%ls", slot->command);
                append_echo_text(echoMsg);
                if (is_tx_hex_echo_enabled()) { append_echo_hex_dump(binBuf, binLen); }
                append_echo_text(L"\r\n");
            }
        } else { MessageBoxW(g_hwndMacroPads[bankIndex], L"Ошибка отправки!", L"Ошибка", MB_ICONERROR | MB_OK); }
    }
}

void UpdateMacroButtons(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (!g_hwndMacroPads[bankIndex]) return;
    for (int i = 0; i < MACROS_PER_BANK; i++) {
        HWND hBtn = GetDlgItem(g_hwndMacroPads[bankIndex], IDC_BTN_MACRO_BASE + i);
        if (hBtn) {
            wchar_t displayText[64] = L"";
            if (g_showCommand && wcslen(g_macroBanks[bankIndex][i].command) > 0) {
                wcsncpy(displayText, g_macroBanks[bankIndex][i].command, 20); displayText[20] = L'\0';
                if (wcslen(g_macroBanks[bankIndex][i].command) > 20) wcscat(displayText, L"...");
            } else { wcscpy(displayText, g_macroBanks[bankIndex][i].label); }
            SetWindowTextW(hBtn, displayText); InvalidateRect(hBtn, NULL, TRUE);
        }
    }
}

void UpdateMacroButtonTitle(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (!g_hwndTerminal) return;
    HWND hBtn = g_hMacroBankBtns[bankIndex];
    if (!hBtn || !IsWindow(hBtn)) return;

    wchar_t title[MAX_MACRO_TITLE_LEN] = {0};
    wcsncpy(title, g_macroBankTitles[bankIndex], MAX_MACRO_TITLE_LEN - 1);
    if (wcslen(title) > 0) {
        wchar_t defaultPattern[16]; swprintf(defaultPattern, 16, L"M%d-", bankIndex + 1);
        if (wcsncmp(title, defaultPattern, wcslen(defaultPattern)) != 0) {
            if (wcslen(title) > 20) { title[20] = L'\0'; wcscat(title, L"..."); }
            SetWindowTextW(hBtn, title); return;
        }
    }
    wchar_t defaultTitle[8]; swprintf(defaultTitle, 8, L"M%d", bankIndex + 1);
    SetWindowTextW(hBtn, defaultTitle);
}

void UpdateAllMacroButtonTitles(void) { for (int i = 0; i < MACRO_BANK_COUNT; i++) UpdateMacroButtonTitle(i); }

void SaveAllMacroWindowsState(void) {
    int count = 0; int banksOpen[MACRO_BANK_COUNT] = {0};
    for (int i = 0; i < MACRO_BANK_COUNT; i++) {
        if (g_hwndMacroPads[i] && IsWindow(g_hwndMacroPads[i])) { banksOpen[i] = 1; count++; }
    }
    WriteIniInt(L"MacroWindows", L"Count", count);
    for (int i = 0; i < MACRO_BANK_COUNT; i++) { wchar_t key[16]; swprintf(key, 16, L"Bank%d", i); WriteIniInt(L"MacroWindows", key, banksOpen[i]); }
}

void ResetMacroLoadedFlags(void) { for (int i = 0; i < MACRO_BANK_COUNT; i++) g_macroBankLoaded[i] = FALSE; }

// ====================================================================================================
// Theme
// ====================================================================================================

void ApplyThemeToWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;
    static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->bgColor);
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBrush);
    EnumChildWindows(hwnd, ApplyThemeCallback, (LPARAM)hwnd);
    RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

BOOL CALLBACK ApplyThemeCallback(HWND hWndChild, LPARAM lParam) {
    wchar_t cn[64] = {0}; GetClassNameW(hWndChild, cn, 64);
    if (wcscmp(cn, L"BUTTON") == 0) {
        static HBRUSH hBtnBrush = NULL; if (!hBtnBrush) hBtnBrush = CreateSolidBrush(g_pTheme->btnBg);
        SetClassLongPtr(hWndChild, GCLP_HBRBACKGROUND, (LONG_PTR)hBtnBrush); InvalidateRect(hWndChild, NULL, TRUE);
    } else if (wcscmp(cn, L"EDIT") == 0 || wcscmp(cn, L"RichEdit20W") == 0) {
        static HBRUSH hEditBrush = NULL; if (!hEditBrush) hEditBrush = CreateSolidBrush(g_pTheme->editBg);
        SetClassLongPtr(hWndChild, GCLP_HBRBACKGROUND, (LONG_PTR)hEditBrush); InvalidateRect(hWndChild, NULL, TRUE);
    } else if (wcscmp(cn, L"COMBOBOX") == 0 || wcscmp(cn, L"STATIC") == 0) { InvalidateRect(hWndChild, NULL, TRUE); }
    return TRUE;
}

// ====================================================================================================
// Macro Edit Window
// ====================================================================================================

LRESULT CALLBACK MacroEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    MacroEditCtx* ctx = (MacroEditCtx*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lp; int* params = (int*)cs->lpCreateParams;
            ctx = (MacroEditCtx*)calloc(1, sizeof(MacroEditCtx)); if (!ctx) return -1;
            ctx->bankIndex = params[0]; ctx->slotIndex = params[1];
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);

            CreateWindowExW(0, L"STATIC", L"Name:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 14, 45, 20, hwnd, NULL, NULL, NULL);
            ctx->hEditName = CreateWindowExW(0, L"EDIT", g_macroBanks[ctx->bankIndex][ctx->slotIndex].label,
                                             WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                                             60, 12, 220, 22, hwnd, (HMENU)IDC_EDIT_MACRO_NAME, NULL, NULL);
            SendMessage(ctx->hEditName, WM_SETFONT, (WPARAM)g_hEditFont, TRUE);

            CreateWindowExW(0, L"STATIC", L"CMD:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 46, 40, 20, hwnd, NULL, NULL, NULL);
            ctx->hEditCmd = CreateWindowExW(0, L"EDIT", g_macroBanks[ctx->bankIndex][ctx->slotIndex].command,
                                            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                                            65, 44, 215, 22, hwnd, (HMENU)IDC_EDIT_MACRO_CMD, NULL, NULL);
            SendMessage(ctx->hEditCmd, WM_SETFONT, (WPARAM)g_hEditFont, TRUE);

            ctx->hBtnSave = CreateWindowExW(0, L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, 110, 85, 70, 25, hwnd, (HMENU)IDC_BTN_MACRO_SAVE, NULL, NULL);
            SendMessage(ctx->hBtnSave, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            ctx->hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 190, 85, 70, 25, hwnd, (HMENU)IDC_BTN_MACRO_CANCEL, NULL, NULL);
            SendMessage(ctx->hBtnCancel, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            ApplyThemeToWindow(hwnd); SetFocus(ctx->hEditName); SendMessage(ctx->hEditName, EM_SETSEL, 0, -1);
            return 0;
        }
        case WM_ERASEBKGND: { HDC hdc = (HDC)wp; RECT rc; GetClientRect(hwnd, &rc); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->bgColor); FillRect(hdc, &rc, hBrush); return 1; }
        case WM_CTLCOLORBTN: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->btnBg); SetTextColor(hdc, g_pTheme->btnFg); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->btnBg); return (LRESULT)hBrush; }
        case WM_CTLCOLOREDIT: case WM_CTLCOLORSTATIC: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->editBg); SetTextColor(hdc, g_pTheme->editFg); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->editBg); return (LRESULT)hBrush; }
        case WM_COMMAND: { int id = LOWORD(wp); if (id == IDC_BTN_MACRO_SAVE) { SaveMacroFromEdit(); CloseMacroEdit(); return 0; } if (id == IDC_BTN_MACRO_CANCEL) { CloseMacroEdit(); return 0; } return 0; }
        case WM_KEYDOWN: { if (wp == VK_ESCAPE) { CloseMacroEdit(); return 0; } if (wp == VK_RETURN) { HWND hFocus = GetFocus(); if (ctx && (hFocus == ctx->hEditName || hFocus == ctx->hEditCmd)) { SaveMacroFromEdit(); CloseMacroEdit(); return 0; } } break; }
        case WM_CLOSE: { CloseMacroEdit(); return 0; }
        case WM_DESTROY: { if (ctx) { free(ctx); SetWindowLongPtr(hwnd, GWLP_USERDATA, 0); } g_hwndMacroEdit = NULL; return 0; }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void CloseMacroEdit(void) { if (g_hwndMacroEdit && IsWindow(g_hwndMacroEdit)) DestroyWindow(g_hwndMacroEdit); g_hwndMacroEdit = NULL; g_editingSlot = -1; g_editingBank = -1; }

void SaveMacroFromEdit(void) {
    if (g_editingBank < 0 || g_editingBank >= MACRO_BANK_COUNT || g_editingSlot < 0 || g_editingSlot >= MACROS_PER_BANK) return;
    if (!g_hwndMacroEdit) return;
    HWND hEditName = GetDlgItem(g_hwndMacroEdit, IDC_EDIT_MACRO_NAME);
    HWND hEditCmd = GetDlgItem(g_hwndMacroEdit, IDC_EDIT_MACRO_CMD);
    if (hEditName) GetWindowTextW(hEditName, g_macroBanks[g_editingBank][g_editingSlot].label, MACRO_LABEL_LEN);
    if (hEditCmd) GetWindowTextW(hEditCmd, g_macroBanks[g_editingBank][g_editingSlot].command, MACRO_CMD_LEN);
    SaveMacroBank(g_editingBank); UpdateMacroButtons(g_editingBank);
}

void ShowMacroEditWindow(HWND hParent, int bankIndex, int slotIndex) {
    if (!hParent) return; if (g_hwndMacroEdit) CloseMacroEdit();
    g_editingBank = bankIndex; g_editingSlot = slotIndex;

    static BOOL classRegistered = FALSE;
    if (!classRegistered) {
        WNDCLASSW wc = {0}; wc.lpfnWndProc = MacroEditWndProc; wc.hInstance = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); wc.lpszClassName = L"WT232_MacroEdit_Class";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&wc); classRegistered = TRUE;
    }

    RECT rcParent; GetWindowRect(hParent, &rcParent);
    int params[2] = { bankIndex, slotIndex };
    int x = rcParent.left + 10, y = rcParent.bottom + 5, w = 300, h = 150;
    int screenW = GetSystemMetrics(SM_CXSCREEN), screenH = GetSystemMetrics(SM_CYSCREEN);
    if (x + w > screenW) x = screenW - w - 10; if (y + h > screenH) y = screenH - h - 10; if (y < 0) y = 0;

    g_hwndMacroEdit = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_MacroEdit_Class", L"Edit Macro",
                                      WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX | WS_CLIPCHILDREN,
                                      x, y, w, h, hParent, NULL, GetModuleHandle(NULL), (LPVOID)params);
    if (g_hwndMacroEdit) { ShowWindow(g_hwndMacroEdit, SW_SHOW); UpdateWindow(g_hwndMacroEdit); SetForegroundWindow(g_hwndMacroEdit); }
}

void LayoutButtons(HWND hwnd) {
    if (!hwnd) return;
    RECT rc; GetClientRect(hwnd, &rc);
    int w = rc.right, h = rc.bottom; int margin = 6; int topBarH = 45;

    HWND hTitleEdit = GetDlgItem(hwnd, IDC_EDIT_MACRO_TITLE);
    HWND hModeBtn = GetDlgItem(hwnd, IDC_BTN_MACRO_MODE);
    HWND hDispBtn = GetDlgItem(hwnd, IDC_BTN_MACRO_DISPLAY);
    HWND hStatusLbl = GetDlgItem(hwnd, IDC_STATIC_MACRO_STATUS);

    if (hModeBtn) MoveWindow(hModeBtn, margin, margin, 55, 25, TRUE);
    if (hDispBtn) MoveWindow(hDispBtn, margin + 60, margin, 55, 25, TRUE);
    if (hTitleEdit) MoveWindow(hTitleEdit, 130, margin, 120, 24, TRUE);
    if (hStatusLbl) MoveWindow(hStatusLbl, margin, margin + 28, 250, 16, TRUE);

    int topOffset = margin + topBarH + 4 + 18;
    int bottomPanelH = 32;
    int availH = h - topOffset - margin - bottomPanelH;
    int availW = w - margin * 2;
    int btnW = (availW - (MACRO_COLS - 1) * 4) / MACRO_COLS;
    int btnH = (availH - (MACRO_ROWS - 1) * 4) / MACRO_ROWS;
    if (btnW < 40) btnW = 40; if (btnH < 20) btnH = 20;

    for (int i = 0; i < MACROS_PER_BANK; i++) {
        int col = i % MACRO_COLS; int row = i / MACRO_COLS;
        int x = margin + col * (btnW + 4); int y = topOffset + row * (btnH + 4);
        HWND hBtn = GetDlgItem(hwnd, IDC_BTN_MACRO_BASE + i);
        if (hBtn) MoveWindow(hBtn, x, y, btnW, btnH, TRUE);
    }

    int scriptY = h - margin - 26;
    int btnScriptW = 45; int scriptInfoW = 22; int spacing = 4;
    HWND hScriptPath = GetDlgItem(hwnd, IDC_MACRO_SCRIPT_PATH);
    HWND hScriptInfoBtn = GetDlgItem(hwnd, IDC_MACRO_BTN_SCRIPT_INFO);
    HWND hLoadBtn = GetDlgItem(hwnd, IDC_MACRO_BTN_LOAD);
    HWND hRunBtn = GetDlgItem(hwnd, IDC_MACRO_BTN_RUN);
    HWND hEditBtn = GetDlgItem(hwnd, IDC_MACRO_BTN_EDIT);

    int totalBtnsW = scriptInfoW + spacing + (btnScriptW + spacing) * 4;
    int pathW = availW - totalBtnsW; if (pathW < 40) pathW = 40;

    if (hScriptPath) MoveWindow(hScriptPath, margin, scriptY, pathW, 22, TRUE);
    if (hScriptInfoBtn) MoveWindow(hScriptInfoBtn, margin + pathW + spacing, scriptY, scriptInfoW, 22, TRUE);
    if (hLoadBtn) MoveWindow(hLoadBtn, margin + pathW + spacing + scriptInfoW + spacing, scriptY, btnScriptW, 22, TRUE);
    if (hRunBtn) MoveWindow(hRunBtn, margin + pathW + spacing + scriptInfoW + spacing + (btnScriptW + spacing), scriptY, btnScriptW, 22, TRUE);
    if (hEditBtn) MoveWindow(hEditBtn, margin + pathW + spacing + scriptInfoW + spacing + (btnScriptW + spacing) * 2, scriptY, btnScriptW, 22, TRUE);

    RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

LRESULT CALLBACK MacroPadWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    MacroPadCtx* ctx = (MacroPadCtx*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lp; int bankIdx = (int)(LONG_PTR)cs->lpCreateParams;
            ctx = (MacroPadCtx*)calloc(1, sizeof(MacroPadCtx)); if (!ctx) return -1;
            ctx->bankIndex = bankIdx; SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);

            g_editMode = FALSE; LoadMacroBank(bankIdx);

            ctx->hTitleEdit = CreateWindowExW(0, L"EDIT", g_macroBankTitles[bankIdx], WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, 130, 10, 120, 24, hwnd, (HMENU)IDC_EDIT_MACRO_TITLE, NULL, NULL);
            EnableWindow(ctx->hTitleEdit, FALSE);
            ctx->hTitleFont = CreateTitleFont(-15); SendMessage(ctx->hTitleEdit, WM_SETFONT, (WPARAM)ctx->hTitleFont, TRUE);
            if (wcslen(g_macroBankTitles[bankIdx]) == 0) SetWindowTextW(ctx->hTitleEdit, L"");

            ctx->hModeBtn = CreateWindowExW(0, L"BUTTON", L"RUN", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 55, 25, hwnd, (HMENU)IDC_BTN_MACRO_MODE, NULL, NULL);
            SendMessage(ctx->hModeBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            ctx->hDispBtn = CreateWindowExW(0, L"BUTTON", L"LABEL", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 60, 0, 55, 25, hwnd, (HMENU)IDC_BTN_MACRO_DISPLAY, NULL, NULL);
            SendMessage(ctx->hDispBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            ctx->hStatusLbl = CreateWindowExW(0, L"STATIC", L"Режим: RUN MODE", WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 0, 250, 16, hwnd, (HMENU)IDC_STATIC_MACRO_STATUS, NULL, NULL);
            SendMessage(ctx->hStatusLbl, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            for (int i = 0; i < MACROS_PER_BANK; i++) {
                wchar_t displayText[64] = L"";
                if (g_showCommand && wcslen(g_macroBanks[bankIdx][i].command) > 0) {
                    wcsncpy(displayText, g_macroBanks[bankIdx][i].command, 20); displayText[20] = L'\0';
                    if (wcslen(g_macroBanks[bankIdx][i].command) > 20) wcscat(displayText, L"...");
                } else { wcscpy(displayText, g_macroBanks[bankIdx][i].label); }
                HWND hBtn = CreateWindowExW(0, L"BUTTON", displayText, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 0, 0, 10, 10, hwnd, (HMENU)(LONG_PTR)(IDC_BTN_MACRO_BASE + i), NULL, NULL);
                if (hBtn) SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            }

            ctx->hScriptPath = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER, 0, 0, 10, 22, hwnd, (HMENU)IDC_MACRO_SCRIPT_PATH, NULL, NULL);
            SendMessage(ctx->hScriptPath, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            ctx->hScriptInfoBtn = CreateWindowExW(0, L"BUTTON", L"?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 22, 22, hwnd, (HMENU)IDC_MACRO_BTN_SCRIPT_INFO, NULL, NULL);
            SendMessage(ctx->hScriptInfoBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            ctx->hLoadBtn = CreateWindowExW(0, L"BUTTON", L"LOAD", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 45, 22, hwnd, (HMENU)IDC_MACRO_BTN_LOAD, NULL, NULL);
            SendMessage(ctx->hLoadBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            ctx->hRunBtn = CreateWindowExW(0, L"BUTTON", L"RUN", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 45, 22, hwnd, (HMENU)IDC_MACRO_BTN_RUN, NULL, NULL);
            SendMessage(ctx->hRunBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            ctx->hEditBtn = CreateWindowExW(0, L"BUTTON", L"EDIT", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 45, 22, hwnd, (HMENU)IDC_MACRO_BTN_EDIT, NULL, NULL);
            SendMessage(ctx->hEditBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            wchar_t scriptPath[MAX_PATH]; GetMacroScriptPath(bankIdx, scriptPath, MAX_PATH); SetWindowTextW(ctx->hScriptPath, scriptPath);
            BOOL hasTitle = (wcslen(g_macroBankTitles[bankIdx]) > 0);
            EnableWindow(ctx->hLoadBtn, hasTitle); EnableWindow(ctx->hRunBtn, hasTitle); EnableWindow(ctx->hEditBtn, hasTitle);

            ApplyThemeToWindow(hwnd); LayoutButtons(hwnd); return 0;
        }
        case WM_ERASEBKGND: { HDC hdc = (HDC)wp; RECT rc; GetClientRect(hwnd, &rc); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->bgColor); FillRect(hdc, &rc, hBrush); return 1; }
        case WM_MOVING: { if (!ctx) break; RECT rc; GetWindowRect(hwnd, &rc); if (ctx->bankIndex >= 0) SaveMacroPadPosition(ctx->bankIndex, rc.left, rc.top); return DefWindowProcW(hwnd, msg, wp, lp); }
        case WM_SIZE: { LayoutButtons(hwnd); return 0; }
        case WM_GETMINMAXINFO: { MINMAXINFO* pMMI = (MINMAXINFO*)lp; pMMI->ptMinTrackSize.x = 310; pMMI->ptMinTrackSize.y = 340; return 0; }
        case WM_TIMER: { if (!ctx) break; if (wp == TIMER_MACRO_SCRIPT_ID) { if (g_hPort != INVALID_HANDLE_VALUE) RunNextMacroScriptCommand(ctx->bankIndex); else KillTimer(hwnd, TIMER_MACRO_SCRIPT_ID); } return 0; }
        case WM_CTLCOLORBTN: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->btnBg); SetTextColor(hdc, g_pTheme->btnFg); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->btnBg); return (LRESULT)hBrush; }
        case WM_CTLCOLOREDIT: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->editBg); SetTextColor(hdc, RGB(0,0,0)); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->editBg); return (LRESULT)hBrush; }
        case WM_CTLCOLORSTATIC: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->bgColor); SetTextColor(hdc, g_pTheme->fgColor); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->bgColor); return (LRESULT)hBrush; }
        case WM_COMMAND: {
            if (!ctx) break;
            int id = LOWORD(wp);

            if (id == IDC_BTN_MACRO_MODE) {
                g_editMode = !g_editMode;
                SetWindowTextW(ctx->hModeBtn, g_editMode ? L"EDIT" : L"RUN");
                InvalidateRect(ctx->hModeBtn, NULL, TRUE);
                if (ctx->hStatusLbl) {
                    SetWindowTextW(ctx->hStatusLbl, g_editMode ? L"Режим: EDIT MODE" : L"Режим: RUN MODE");
                    InvalidateRect(ctx->hStatusLbl, NULL, TRUE);
                }
                if (ctx->hTitleEdit) {
                    if (g_editMode) { EnableWindow(ctx->hTitleEdit, TRUE); SetFocus(ctx->hTitleEdit); SendMessage(ctx->hTitleEdit, EM_SETSEL, 0, -1); }
                    else { EnableWindow(ctx->hTitleEdit, FALSE); SendMessage(ctx->hTitleEdit, EM_SETSEL, 0, 0); }
                    InvalidateRect(ctx->hTitleEdit, NULL, TRUE); UpdateWindow(ctx->hTitleEdit);
                }
                if (!g_editMode && g_hwndMacroEdit) { CloseMacroEdit(); }
                return 0;
            }

            if (id == IDC_BTN_MACRO_DISPLAY) {
                g_showCommand = !g_showCommand;
                SetWindowTextW(ctx->hDispBtn, g_showCommand ? L"CMD" : L"LABEL");
                InvalidateRect(ctx->hDispBtn, NULL, TRUE);
                UpdateMacroButtons(ctx->bankIndex);
                return 0;
            }

            if (id >= IDC_BTN_MACRO_BASE && id < IDC_BTN_MACRO_BASE + MACROS_PER_BANK) {
                int slotIdx = id - IDC_BTN_MACRO_BASE;
                if (g_editMode) { ShowMacroEditWindow(hwnd, ctx->bankIndex, slotIdx); }
                else { SendMacroCommand(ctx->bankIndex, slotIdx); }
                return 0;
            }

            if (id == IDC_MACRO_BTN_SCRIPT_INFO) {
                static const wchar_t scriptHelp[] =
                    L"Справка по скриптам:\r\n"
                    L"=========================================\r\n\r\n"
                    L"Формат файла (.txt):\r\n"
                    L"Каждая строка - одна команда.\r\n"
                    L"Поддерживается инлайн-HEX (`XX`).\r\n\r\n"
                    L"Директивы:\r\n"
                    L"1000        - глобальная задержка (мс)\r\n"
                    L"(только в первой строке)\r\n"
                    L"#DELAY 500  - задержка перед след. командой\r\n"
                    L"#STOP       - остановить скрипт\r\n"
                    L"#...        - комментарий (игнорируется)\r\n\r\n"
                    L"Без #STOP скрипт выполняется циклически.";
                HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class",
                                             L"Справка: Скрипты", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                             CW_USEDEFAULT, CW_USEDEFAULT, 380, 450,
                                             hwnd, NULL, GetModuleHandle(NULL), (LPVOID)scriptHelp);
                if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); }
                return 0;
            }

            if (id == IDC_MACRO_BTN_LOAD) {
                OPENFILENAMEW ofn; wchar_t szFile[MAX_PATH] = L"";
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
                ofn.lpstrFile = szFile; ofn.nMaxFile = MAX_PATH;
                ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                ofn.nFilterIndex = 1; ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                if (GetOpenFileNameW(&ofn)) {
                    LoadMacroScript(ctx->bankIndex, szFile);
                    SetWindowTextW(ctx->hScriptPath, szFile);
                    UpdateMacroScriptUI(hwnd, ctx->bankIndex);
                }
                return 0;
            }

            if (id == IDC_MACRO_BTN_RUN) {
                if (g_macroScriptRunning[ctx->bankIndex]) { StopMacroScript(ctx->bankIndex); }
                else {
                    if (g_macroScriptCount[ctx->bankIndex] > 0) {
                        g_macroScriptRunning[ctx->bankIndex] = TRUE;
                        UpdateMacroScriptUI(hwnd, ctx->bankIndex);
                        RunNextMacroScriptCommand(ctx->bankIndex);
                    } else { MessageBoxW(hwnd, L"Сначала загрузите скрипт!", L"Внимание", MB_ICONWARNING | MB_OK); }
                }
                return 0;
            }

            if (id == IDC_MACRO_BTN_EDIT) {
                wchar_t path[MAX_PATH]; GetMacroScriptPath(ctx->bankIndex, path, MAX_PATH);
                if (wcslen(path) > 0) { ShellExecuteW(hwnd, L"open", path, NULL, NULL, SW_SHOWNORMAL); }
                return 0;
            }

            if (id == IDC_EDIT_MACRO_TITLE) {
                if (HIWORD(wp) == EN_CHANGE && g_editMode) {
                    GetWindowTextW(ctx->hTitleEdit, g_macroBankTitles[ctx->bankIndex], MAX_MACRO_TITLE_LEN);
                    SaveMacroBankTitle(ctx->bankIndex);
                    UpdateMacroButtonTitle(ctx->bankIndex);
                }
                return 0;
            }
            break;
        }
        case WM_CLOSE: { if (!ctx) break; if (g_hwndMacroEdit) CloseMacroEdit(); StopMacroScript(ctx->bankIndex); g_editMode = FALSE; DestroyWindow(hwnd); if (ctx->bankIndex >= 0 && ctx->bankIndex < MACRO_BANK_COUNT) g_hwndMacroPads[ctx->bankIndex] = NULL; return 0; }
        case WM_DESTROY: { if (ctx) { if (ctx->hTitleFont) DeleteObject(ctx->hTitleFont); free(ctx); SetWindowLongPtr(hwnd, GWLP_USERDATA, 0); } if (g_hwndMacroEdit) CloseMacroEdit(); return 0; }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void ShowMacroPad(HWND hParent, int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (g_hwndMacroPads[bankIndex] && IsWindow(g_hwndMacroPads[bankIndex])) { ShowWindow(g_hwndMacroPads[bankIndex], SW_SHOW); SetForegroundWindow(g_hwndMacroPads[bankIndex]); return; }

    g_macroBankLoaded[bankIndex] = FALSE; g_editMode = FALSE;

    static BOOL classRegistered = FALSE;
    if (!classRegistered) {
        WNDCLASSW wc = {0}; wc.lpfnWndProc = MacroPadWndProc; wc.hInstance = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); wc.lpszClassName = L"WT232_MacroPad_Class";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClassW(&wc); classRegistered = TRUE;
    }

    int w = MACRO_COLS * 70 + 30; int h = MACRO_ROWS * 30 + 100;
    int x, y; LoadMacroPadPosition(bankIndex, &x, &y);
    if (x == -1 || y == -1) { RECT rcParent; GetWindowRect(hParent, &rcParent); x = rcParent.left + 20; y = rcParent.top + 50; }
    int screenW = GetSystemMetrics(SM_CXSCREEN), screenH = GetSystemMetrics(SM_CYSCREEN);
    if (x + w > screenW) x = screenW - w - 10; if (y + h > screenH) y = screenH - h - 10; if (x < 0) x = 0; if (y < 0) y = 0;

    wchar_t title[64]; swprintf(title, 64, L"Macros M%d (RUN/EDIT | LABEL/CMD)", bankIndex + 1);
    g_hwndMacroPads[bankIndex] = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_MacroPad_Class", title,
                                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_THICKFRAME | WS_CLIPCHILDREN,
                                                 x, y, w, h, hParent, NULL, GetModuleHandle(NULL), (LPVOID)(LONG_PTR)bankIndex);
    if (g_hwndMacroPads[bankIndex]) { ShowWindow(g_hwndMacroPads[bankIndex], SW_SHOW); UpdateWindow(g_hwndMacroPads[bankIndex]); ApplyThemeToWindow(g_hwndMacroPads[bankIndex]); }
}

// ====================================================================================================
// INI
// ====================================================================================================

void InitIniPaths(void) {
    GetModuleFileNameW(NULL, g_iniPath, MAX_PATH);
    wchar_t* pDot = wcsrchr(g_iniPath, L'.'); if (pDot) *pDot = L'\0';
    wcscat(g_iniPath, L".ini"); wcscpy(g_iniBackupPath, g_iniPath); wcscat(g_iniBackupPath, L".bak");
}

BOOL ReadIniString(const wchar_t* section, const wchar_t* key, wchar_t* out, int maxLen, const wchar_t* defVal) { DWORD res = GetPrivateProfileStringW(section, key, defVal, out, maxLen, g_iniPath); return (res > 0) ? TRUE : FALSE; }
int ReadIniInt(const wchar_t* section, const wchar_t* key, int defVal) { return GetPrivateProfileIntW(section, key, defVal, g_iniPath); }
void WriteIniString(const wchar_t* section, const wchar_t* key, const wchar_t* val) { WritePrivateProfileStringW(section, key, val, g_iniPath); }
void WriteIniInt(const wchar_t* section, const wchar_t* key, int val) { wchar_t buf[32]; swprintf(buf, 32, L"%d", val); WritePrivateProfileStringW(section, key, buf, g_iniPath); }

BOOL ValidateIni(void) { int baudrate = ReadIniInt(L"Port", L"LastBaudrate", -1); if (baudrate == -1) return FALSE; int width = ReadIniInt(L"Terminal", L"Width", -1); if (width == -1) return FALSE; return TRUE; }

BOOL ReadAllIni(void) {
    if (!ValidateIni()) {
        if (ReadIniString(L"Port", L"LastPortName", g_wTxtBuf, 10, L"") > 0) {
            if (ValidateIni()) { CopyFileW(g_iniBackupPath, g_iniPath, FALSE); MessageBoxW(NULL, L"Восстановлены настройки из резервной копии.", L"Восстановление", MB_OK); return TRUE; }
        }
        CreateDefaultIni(); return FALSE;
    }
    return TRUE;
}

void CreateDefaultIni(void) {
    WriteIniString(L"Port", L"LastPortName", L"COM1"); WriteIniInt(L"Port", L"LastBaudrate", 115200);
    WriteIniInt(L"Port", L"LastDataBits", 8); WriteIniInt(L"Port", L"LastParity", 0);
    WriteIniInt(L"Port", L"LastStopBits", 0); WriteIniInt(L"Port", L"LastFlow", 2);
    WriteIniString(L"Port", L"LastVID", L""); WriteIniString(L"Port", L"LastPID", L""); WriteIniString(L"Port", L"LastSerial", L"");

    WriteIniInt(L"Terminal", L"X", 100); WriteIniInt(L"Terminal", L"Y", 100);
    WriteIniInt(L"Terminal", L"Width", 700); WriteIniInt(L"Terminal", L"Height", 480);
    WriteIniInt(L"Terminal", L"State", 1); WriteIniInt(L"Terminal", L"TxMode", 1);
    WriteIniInt(L"Terminal", L"RxMode", 0); WriteIniInt(L"Terminal", L"Echo", 1);
    WriteIniInt(L"Terminal", L"TxHexEcho", 0);
    WriteIniInt(L"Terminal", L"Encoding", 1);
    WriteIniInt(L"Terminal", L"RepeatDelay", 1000);
    WriteIniInt(L"Terminal", L"FontSize", 2);
    WriteIniInt(L"Terminal", L"TopMost", 0);

    WriteIniInt(L"History", L"Count", 0); WriteIniInt(L"MacroWindows", L"Count", 0);
    for (int i = 0; i < MACRO_BANK_COUNT; i++) { wchar_t key[16]; swprintf(key, 16, L"Bank%d", i); WriteIniInt(L"MacroWindows", key, 0); }

    for (int bank = 0; bank < MACRO_BANK_COUNT; bank++) {
        wchar_t section[32]; swprintf(section, 32, L"MacroSlots_%d", bank);
        wchar_t defaultTitle[8]; swprintf(defaultTitle, 8, L"M%d", bank + 1);
        WriteIniString(section, L"BankTitle", defaultTitle);
        for (int i = 0; i < MACROS_PER_BANK; i++) {
            wchar_t keyLabel[32], keyCmd[32]; swprintf(keyLabel, 32, L"Slot%d_Label", i); swprintf(keyCmd, 32, L"Slot%d_Cmd", i);
            WriteIniString(section, keyLabel, L""); WriteIniString(section, keyCmd, L"");
        }
    }
    for (int bank = 0; bank < MACRO_BANK_COUNT; bank++) {
        wchar_t section[32]; swprintf(section, 32, L"MacroPos_%d", bank);
        WriteIniInt(section, L"X", -1); WriteIniInt(section, L"Y", -1);
    }
}

void WriteAllIni(void) {
    CopyFileW(g_iniPath, g_iniBackupPath, FALSE);
    if (g_hComboPort) { wchar_t portName[MAX_PORT_NAME] = {0}; int pi = (int)SendMessageW(g_hComboPort, CB_GETCURSEL, 0, 0); if (pi != CB_ERR) SendMessageW(g_hComboPort, CB_GETLBTEXT, pi, (LPARAM)portName); WriteIniString(L"Port", L"LastPortName", portName); }
    if (g_hComboBaud) { wchar_t baudStr[16] = {0}; GetWindowTextW(g_hComboBaud, baudStr, 16); WriteIniInt(L"Port", L"LastBaudrate", _wtoi(baudStr)); }
    if (g_hComboDataBits) WriteIniInt(L"Port", L"LastDataBits", (int)SendMessageW(g_hComboDataBits, CB_GETCURSEL, 0, 0) + 5);
    if (g_hComboParity) WriteIniInt(L"Port", L"LastParity", (int)SendMessageW(g_hComboParity, CB_GETCURSEL, 0, 0));
    if (g_hComboStopBits) WriteIniInt(L"Port", L"LastStopBits", (int)SendMessageW(g_hComboStopBits, CB_GETCURSEL, 0, 0));
    if (g_hComboFlow) WriteIniInt(L"Port", L"LastFlow", (int)SendMessageW(g_hComboFlow, CB_GETCURSEL, 0, 0));
    WriteIniString(L"Port", L"LastVID", g_session.targetDevice.vid);
    WriteIniString(L"Port", L"LastPID", g_session.targetDevice.pid);
    WriteIniString(L"Port", L"LastSerial", g_session.targetDevice.serial);
}

// ====================================================================================================
// WINMAIN
// ====================================================================================================

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show) {
    INITCOMMONCONTROLSEX icex; icex.dwSize = sizeof(INITCOMMONCONTROLSEX); icex.dwICC = ICC_WIN95_CLASSES; InitCommonControlsEx(&icex);
    InitializeCriticalSection(&g_csComm); InitializeCriticalSection(&g_csRx);
    g_hRxStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL); g_hTxStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    InitIniPaths(); ReadAllIni();
    g_pTheme = &g_theme;

    g_hMonoFont = CreateMonoFont(-14);
    g_hBtnFont = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    g_hEditFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
    g_hTitleFont = CreateTitleFont(-15);

    for (int i = 0; i < MACRO_BANK_COUNT; i++) { g_macroBankLoaded[i] = FALSE; g_macroBankTitles[i][0] = L'\0'; }
    LoadMacroBankTitles();

    WNDCLASSW wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = hInst; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); wc.lpszClassName = L"WT232_Settings_Class";
    if (!RegisterClassW(&wc)) return 0;

    WNDCLASSW tc = {0}; tc.lpfnWndProc = TerminalWndProc; tc.hInstance = hInst; tc.hCursor = LoadCursor(NULL, IDC_ARROW);
    tc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); tc.lpszClassName = L"WT232_Terminal_Class"; RegisterClassW(&tc);

    WNDCLASSW ic = {0}; ic.lpfnWndProc = InfoWndProc; ic.hInstance = hInst; ic.hCursor = LoadCursor(NULL, IDC_ARROW);
    ic.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); ic.lpszClassName = L"WT232_Info_Scroll_Class"; RegisterClassW(&ic);

    wchar_t settingsTitle[128]; swprintf(settingsTitle, 128, L"WT232 Settings " APP_VERSION);
    g_hwndSettings = CreateWindowExW(WS_EX_TOPMOST, L"WT232_Settings_Class", settingsTitle,
                                     WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX | WS_CLIPCHILDREN,
                                     CW_USEDEFAULT, CW_USEDEFAULT, 300, 330, NULL, NULL, hInst, NULL);
    if (!g_hwndSettings) return 0;

    ApplyThemeToWindow(g_hwndSettings); ShowWindow(g_hwndSettings, show); UpdateWindow(g_hwndSettings);

    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }

    if (g_hMonoFont) DeleteObject(g_hMonoFont); if (g_hBtnFont) DeleteObject(g_hBtnFont);
    if (g_hEditFont) DeleteObject(g_hEditFont); if (g_hTitleFont) DeleteObject(g_hTitleFont);
    if (g_hRxStopEvent) CloseHandle(g_hRxStopEvent); if (g_hTxStopEvent) CloseHandle(g_hTxStopEvent);
    DeleteCriticalSection(&g_csRx); DeleteCriticalSection(&g_csComm);
    return 0;
}

// ====================================================================================================
// SETFONTCALLBACK
// ====================================================================================================

BOOL CALLBACK SetFontCallback(HWND hWndChild, LPARAM lParam) {
    wchar_t cn[32] = {0}; GetClassNameW(hWndChild, cn, 32);
    if (wcscmp(cn, L"STATIC") == 0 || wcscmp(cn, L"BUTTON") == 0 || wcscmp(cn, L"COMBOBOX") == 0 || wcscmp(cn, L"EDIT") == 0)
        SendMessage(hWndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
    return TRUE;
}

// ====================================================================================================
// WNDPROC (Окно настроек)
// ====================================================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_ERASEBKGND: { HDC hdc = (HDC)wp; RECT rc; GetClientRect(hwnd, &rc); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->bgColor); FillRect(hdc, &rc, hBrush); return 1; }
        case WM_CTLCOLORBTN: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->btnBg); SetTextColor(hdc, g_pTheme->btnFg); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->btnBg); return (LRESULT)hBrush; }
        case WM_CTLCOLOREDIT: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->editBg); SetTextColor(hdc, g_pTheme->editFg); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->editBg); return (LRESULT)hBrush; }
        case WM_CTLCOLORSTATIC: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->bgColor); SetTextColor(hdc, g_pTheme->fgColor); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->bgColor); return (LRESULT)hBrush; }
        case WM_CREATE: {
            HFONT hFont = GetStockObject(DEFAULT_GUI_FONT);

            // === ПОРТ ===
            CreateWindowExW(0, L"STATIC", L"Порт:", WS_CHILD|WS_VISIBLE, 20, 18, 50, 20, hwnd, NULL, NULL, NULL);
            g_hBtnInfo = CreateWindowExW(0, L"BUTTON", L"?", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 70, 15, 22, 22, hwnd, (HMENU)IDC_BTN_INFO, NULL, NULL);
            g_hComboPort = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 110, 15, 150, 200, hwnd, (HMENU)IDC_COMBO_PORT, NULL, NULL);

            // === СКОРОСТЬ ===
            CreateWindowExW(0, L"STATIC", L"Скорость:", WS_CHILD|WS_VISIBLE, 20, 50, 80, 20, hwnd, NULL, NULL, NULL);
            g_hComboBaud = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL, 110, 47, 150, 200, hwnd, (HMENU)IDC_COMBO_BAUD, NULL, NULL);
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"2400");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"4800");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"9600");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"19200");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"38400");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"57600");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"115200");

            // === БИТЫ ДАННЫХ ===
            CreateWindowExW(0, L"STATIC", L"Биты:", WS_CHILD|WS_VISIBLE, 20, 82, 80, 20, hwnd, NULL, NULL, NULL);
            g_hComboDataBits = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 110, 79, 150, 200, hwnd, (HMENU)IDC_COMBO_DATABITS, NULL, NULL);
            SendMessageW(g_hComboDataBits, CB_ADDSTRING, 0, (LPARAM)L"5");
            SendMessageW(g_hComboDataBits, CB_ADDSTRING, 0, (LPARAM)L"6");
            SendMessageW(g_hComboDataBits, CB_ADDSTRING, 0, (LPARAM)L"7");
            SendMessageW(g_hComboDataBits, CB_ADDSTRING, 0, (LPARAM)L"8");

            // === ЧЕТНОСТЬ ===
            CreateWindowExW(0, L"STATIC", L"Четность:", WS_CHILD|WS_VISIBLE, 20, 114, 80, 20, hwnd, NULL, NULL, NULL);
            g_hComboParity = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 110, 111, 150, 200, hwnd, (HMENU)IDC_COMBO_PARITY, NULL, NULL);
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"Нет / None");
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"Чет / Even");
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"Нечет / Odd");
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"Маркер / Mark");
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"Пробел / Space");

            // === СТОП-БИТЫ ===
            CreateWindowExW(0, L"STATIC", L"Стоп-биты:", WS_CHILD|WS_VISIBLE, 20, 146, 80, 20, hwnd, NULL, NULL, NULL);
            g_hComboStopBits = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 110, 143, 150, 200, hwnd, (HMENU)IDC_COMBO_STOPBITS, NULL, NULL);
            SendMessageW(g_hComboStopBits, CB_ADDSTRING, 0, (LPARAM)L"1");
            SendMessageW(g_hComboStopBits, CB_ADDSTRING, 0, (LPARAM)L"1.5");
            SendMessageW(g_hComboStopBits, CB_ADDSTRING, 0, (LPARAM)L"2");

            // === УПР. ПОТОКОМ (ИСПРАВЛЕНОЕ РАСПОЛОЖЕНИЕ) ===
            CreateWindowExW(0, L"STATIC", L"Упр. потоком:", WS_CHILD|WS_VISIBLE, 20, 178, 90, 20, hwnd, NULL, NULL, NULL);
            // Кнопка ? расположена сразу после надписи "Упр. потоком:" (x=110),
            // точно так же, как кнопка ? для Порта расположена после надписи "Порт:" (x=70)
            HWND hBtnFlowInfo = CreateWindowExW(0, L"BUTTON", L"?", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 110, 174, 22, 22, hwnd, (HMENU)IDC_BTN_FLOW_INFO, NULL, NULL);
            if (hBtnFlowInfo) SendMessage(hBtnFlowInfo, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            // Выпадающий список сдвинут правее кнопки справки
            g_hComboFlow = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 137, 175, 123, 200, hwnd, (HMENU)IDC_COMBO_FLOW, NULL, NULL);
            SendMessageW(g_hComboFlow, CB_ADDSTRING, 0, (LPARAM)L"Нет / None");
            SendMessageW(g_hComboFlow, CB_ADDSTRING, 0, (LPARAM)L"XON/XOFF");
            SendMessageW(g_hComboFlow, CB_ADDSTRING, 0, (LPARAM)L"RTS/CTS");
            SendMessageW(g_hComboFlow, CB_ADDSTRING, 0, (LPARAM)L"RS-485 (RTS Toggle)");

            // === КНОПКИ OK / ОТМЕНА ===
            CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON, 60, 215, 70, 35, hwnd, (HMENU)IDC_BTN_OK, NULL, NULL);
            CreateWindowExW(0, L"BUTTON", L"Отмена", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 160, 215, 70, 35, hwnd, (HMENU)IDC_BTN_CANCEL, NULL, NULL);

            // === ЗАГРУЗКА СОХРАНЁННЫХ НАСТРОЕК ===
            wchar_t lastPort[MAX_PORT_NAME] = {0};
            ReadIniString(L"Port", L"LastPortName", lastPort, MAX_PORT_NAME, L"COM1");
            int lastBaud = ReadIniInt(L"Port", L"LastBaudrate", 115200);
            int lastDataBits = ReadIniInt(L"Port", L"LastDataBits", 8);
            int lastParity = ReadIniInt(L"Port", L"LastParity", 0);
            int lastStopBits = ReadIniInt(L"Port", L"LastStopBits", 0);
            int lastFlow = ReadIniInt(L"Port", L"LastFlow", 2);

            init_com_ports(hwnd, FALSE, lastPort);

            wchar_t baudStr[16];
            swprintf(baudStr, 16, L"%d", lastBaud);
            int baudIdx = (int)SendMessageW(g_hComboBaud, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)baudStr);
            if (baudIdx != CB_ERR) SendMessage(g_hComboBaud, CB_SETCURSEL, baudIdx, 0);
            else SendMessage(g_hComboBaud, CB_SETCURSEL, 6, 0);

            int dataBitsIdx = lastDataBits - 5;
            if (dataBitsIdx >= 0 && dataBitsIdx < 4) SendMessage(g_hComboDataBits, CB_SETCURSEL, dataBitsIdx, 0);
            else SendMessage(g_hComboDataBits, CB_SETCURSEL, 3, 0);

            if (lastParity >= 0 && lastParity < 5) SendMessage(g_hComboParity, CB_SETCURSEL, lastParity, 0);
            else SendMessage(g_hComboParity, CB_SETCURSEL, 0, 0);

            if (lastStopBits >= 0 && lastStopBits < 3) SendMessage(g_hComboStopBits, CB_SETCURSEL, lastStopBits, 0);
            else SendMessage(g_hComboStopBits, CB_SETCURSEL, 0, 0);

            if (lastFlow >= 0 && lastFlow < 4) SendMessage(g_hComboFlow, CB_SETCURSEL, lastFlow, 0);
            else SendMessage(g_hComboFlow, CB_SETCURSEL, 2, 0);

            EnumChildWindows(hwnd, SetFontCallback, (LPARAM)hFont);
            ApplyThemeToWindow(hwnd);
            return 0;
        }
        case WM_KEYDOWN: { if (wp == VK_F1) { show_about_dialog(hwnd); return 0; } break; }
        case WM_COMMAND: {
            switch (LOWORD(wp)) {
                case IDC_BTN_INFO: init_com_ports(hwnd, TRUE, NULL); break;
                case IDC_BTN_FLOW_INFO: {
                    static const wchar_t flowHelp[] =
                        L"Режимы управления потоком (Flow Control):\r\n"
                        L"=========================================\r\n\r\n"
                        L"Нет (None)\r\n"
                        L"  Управление отсутствует. Используйте для простых\r\n"
                        L"  TTL-переходников, отладки или когда линии RTS/CTS\r\n"
                        L"  физически не подключены.\r\n\r\n"
                        L"XON/XOFF\r\n"
                        L"  Программное управление символами XON(0x11)/XOFF(0x13).\r\n"
                        L"  Не требует дополнительных проводов. Может конфликтовать\r\n"
                        L"  с передачей бинарных данных, содержащих эти коды.\r\n\r\n"
                        L"RTS/CTS\r\n"
                        L"  Классическое аппаратное рукопожатие. Устройство\r\n"
                        L"  сигнализирует о готовности через RTS и проверяет\r\n"
                        L"  готовность собеседника через CTS. Наиболее надежно\r\n"
                        L"  для высокоскоростной передачи.\r\n\r\n"
                        L"RS-485 (RTS Toggle)\r\n"
                        L"  Линия RTS автоматически переключается драйвером:\r\n"
                        L"  HIGH во время передачи (TX Enable), LOW в покое.\r\n"
                        L"  Предназначено для полудуплексных шин RS-485,\r\n"
                        L"  где одна пара проводов используется для RX и TX.\r\n"
                        L"  Требует поддержки RTS_TOGGLE драйвером адаптера.";
                    HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class",
                        L"Справка: Управление потоком",
                        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                        CW_USEDEFAULT, CW_USEDEFAULT, 420, 480,
                        hwnd, NULL, GetModuleHandle(NULL), (LPVOID)flowHelp);
                    if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); }
                    break;
                }
                case IDC_BTN_OK: {
                    WriteAllIni();
                    wchar_t selPort[MAX_PORT_NAME]={0}; int pi = (int)SendMessageW(g_hComboPort, CB_GETCURSEL, 0, 0);
                    if (pi != CB_ERR) SendMessageW(g_hComboPort, CB_GETLBTEXT, pi, (LPARAM)selPort);
                    wchar_t szBaud[16]={0}; GetWindowTextW(g_hComboBaud, szBaud, 16); int baud = _wtoi(szBaud);
                    if (wcslen(selPort)==0) { MessageBoxW(hwnd, L"Порт не выбран!", L"Внимание", MB_ICONWARNING|MB_OK); break; }
                    init_com_ports(hwnd, FALSE, selPort);
                    if (com_open(selPort, baud)) {
                        g_session.lastBaudrate = baud; wcscpy(g_session.lastPortName, selPort);
                        g_session.isWaitingReconnect = FALSE; update_terminal_title(selPort, baud);
                        ShowWindow(hwnd, SW_HIDE);
                        int winW = ReadIniInt(L"Terminal", L"Width", 700); int winH = ReadIniInt(L"Terminal", L"Height", 480);
                        int winX = ReadIniInt(L"Terminal", L"X", CW_USEDEFAULT); int winY = ReadIniInt(L"Terminal", L"Y", CW_USEDEFAULT);
                        g_hwndTerminal = CreateWindowExW(0, L"WT232_Terminal_Class", g_szTitle,
                                                       WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, winX, winY, winW, winH,
                                                       NULL, NULL, GetModuleHandle(NULL), NULL);
                        if (g_hwndTerminal) {
                            SetTimer(g_hwndTerminal, TIMER_RECONNECT_ID, 500, NULL);
                            StartRxTxThreads(g_hwndTerminal); ApplyThemeToWindow(g_hwndTerminal); UpdateAllMacroButtonTitles();
                            int state = ReadIniInt(L"Terminal", L"State", SW_SHOW);
                            ShowWindow(g_hwndTerminal, state); UpdateWindow(g_hwndTerminal);
                        }
                    } else { MessageBoxW(hwnd, L"Не удалось открыть порт!", L"Ошибка", MB_ICONERROR|MB_OK); }
                    break;
                }
                case IDC_BTN_CANCEL: PostQuitMessage(0); break;
            }
            return 0;
        }
        case WM_DESTROY: { if (g_hPort != INVALID_HANDLE_VALUE) com_close(); PostQuitMessage(0); return 0; }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ====================================================================================================
// TERMINALWNDPROC (Основное окно терминала)
// ====================================================================================================

LRESULT CALLBACK TerminalWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_ERASEBKGND: { HDC hdc = (HDC)wp; RECT rc; GetClientRect(hwnd, &rc); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->bgColor); FillRect(hdc, &rc, hBrush); return 1; }
        case WM_CTLCOLORBTN: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->btnBg); SetTextColor(hdc, g_pTheme->btnFg); HFONT hFont = (HFONT)SendMessage((HWND)lp, WM_GETFONT, 0, 0); if (hFont) SelectObject(hdc, hFont); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->btnBg); return (LRESULT)hBrush; }
        case WM_CTLCOLOREDIT: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->editBg); SetTextColor(hdc, g_pTheme->editFg); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->editBg); return (LRESULT)hBrush; }
        case WM_CTLCOLORSTATIC: { HDC hdc = (HDC)wp; SetBkColor(hdc, g_pTheme->bgColor); SetTextColor(hdc, g_pTheme->fgColor); static HBRUSH hBrush = NULL; if (!hBrush) hBrush = CreateSolidBrush(g_pTheme->bgColor); return (LRESULT)hBrush; }
        case WM_CREATE: {
            LoadLibraryW(L"riched20.dll");
            g_hEditRx = CreateWindowExW(0, L"RichEdit20W", L"", WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL, 0, 0, 10, 10, hwnd, (HMENU)IDC_EDIT_RX, GetModuleHandle(NULL), NULL);

            g_hChkDump = CreateWindowExW(0, L"BUTTON", L"DUMP", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 15, 0, 60, 20, hwnd, (HMENU)IDC_CHK_DUMP, NULL, NULL);
            int dumpState = ReadIniInt(L"Terminal", L"DumpMode", 0);
            if (dumpState) { SendMessage(g_hChkDump, BM_SETCHECK, BST_CHECKED, 0); g_rxMode = RX_MODE_DUMP; if (g_hChkEcho) SendMessage(g_hChkEcho, BM_SETCHECK, BST_UNCHECKED, 0); }
            else { g_rxMode = RX_MODE_TEXT; }
            SendMessage(g_hChkDump, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);

            g_hComboFontSize = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 125, 0, 60, 200, hwnd, (HMENU)IDC_COMBO_FONT_SIZE, NULL, NULL);
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"8"); SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"9");
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"10"); SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"11");
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"12"); SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"14");
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"16");
            int fontSizeIdx = ReadIniInt(L"Terminal", L"FontSize", 2);
            SendMessage(g_hComboFontSize, CB_SETCURSEL, fontSizeIdx, 0);
            wchar_t szSize[16] = {0}; SendMessageW(g_hComboFontSize, CB_GETLBTEXT, fontSizeIdx, (LPARAM)szSize);
            g_fontSize = _wtoi(szSize); if (g_fontSize <= 0) g_fontSize = 10;
            if (g_hMonoFont) DeleteObject(g_hMonoFont);
            g_hMonoFont = CreateMonoFont(-MulDiv(g_fontSize, 96, 72));
            if (g_hEditRx) SendMessage(g_hEditRx, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);

            g_hComboEnc = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 190, 0, 95, 200, hwnd, (HMENU)IDC_COMBO_ENC, NULL, NULL);
            for (int e = 0; e < g_encodingCount; e++) SendMessageW(g_hComboEnc, CB_ADDSTRING, 0, (LPARAM)g_encodings[e].name);
            SendMessage(g_hComboEnc, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            int encodingIdx = ReadIniInt(L"Terminal", L"Encoding", 1);
            SendMessage(g_hComboEnc, CB_SETCURSEL, encodingIdx, 0);

            g_hChkTxHex = CreateWindowExW(0, L"BUTTON", L"HEX", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 290, 0, 55, 20, hwnd, (HMENU)IDC_CHK_TX_HEX, NULL, NULL);
            int txHex = ReadIniInt(L"Terminal", L"TxHexEcho", 0);
            SendMessage(g_hChkTxHex, BM_SETCHECK, txHex ? BST_CHECKED : BST_UNCHECKED, 0);
            g_txHexEcho = (txHex != 0);
            SendMessage(g_hChkTxHex, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);

            g_hChkEcho = CreateWindowExW(0, L"BUTTON", L"Echo", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 350, 0, 75, 20, hwnd, (HMENU)IDC_CHK_ECHO, NULL, NULL);
            int echo = ReadIniInt(L"Terminal", L"Echo", 1);
            SendMessage(g_hChkEcho, BM_SETCHECK, echo ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(g_hChkEcho, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);

            g_hChkTopMost = CreateWindowExW(0, L"BUTTON", L"UP", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 430, 0, 45, 20, hwnd, (HMENU)IDC_CHK_TOPMOST, NULL, NULL);
            int topMost = ReadIniInt(L"Terminal", L"TopMost", 0);
            SendMessage(g_hChkTopMost, BM_SETCHECK, topMost ? BST_CHECKED : BST_UNCHECKED, 0);
            if (topMost) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SendMessage(g_hChkTopMost, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);

            g_hBtnAbout = CreateWindowExW(0, L"BUTTON", L"Info", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 480, 0, 35, 22, hwnd, (HMENU)IDC_BTN_ABOUT, NULL, NULL);
            SendMessage(g_hBtnAbout, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            g_hComboTx = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL|WS_HSCROLL|ES_AUTOHSCROLL|WS_TABSTOP, 0, 0, 10, 10, hwnd, (HMENU)IDC_COMBO_TX, NULL, NULL);
            SendMessage(g_hComboTx, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            HWND hEditPart = GetWindow(g_hComboTx, GW_CHILD);
            if (hEditPart) g_pfnOrigEditProc = (WNDPROC)SetWindowLongPtrW(hEditPart, GWLP_WNDPROC, (LONG_PTR)TxEditSubProc);

            g_hComboSuffix = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL|WS_TABSTOP, 0, 0, 100, 10, hwnd, (HMENU)IDC_COMBO_SUFFIX, NULL, NULL);
            SendMessage(g_hComboSuffix, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            SendMessageW(g_hComboSuffix, CB_ADDSTRING, 0, (LPARAM)L""); SendMessageW(g_hComboSuffix, CB_ADDSTRING, 0, (LPARAM)L"`0D0A`");
            SendMessageW(g_hComboSuffix, CB_ADDSTRING, 0, (LPARAM)L"`0A`"); SendMessageW(g_hComboSuffix, CB_ADDSTRING, 0, (LPARAM)L"`0D`");
            SendMessage(g_hComboSuffix, CB_SETCURSEL, 1, 0);

            g_hBtnSuffixInfo = CreateWindowExW(0, L"BUTTON", L"?", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, (HMENU)IDC_BTN_SUFFIX_INFO, NULL, NULL);
            SendMessage(g_hBtnSuffixInfo, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            g_hBtnSend = CreateWindowExW(0, L"BUTTON", L"SEND", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, (HMENU)IDC_BTN_SEND, NULL, NULL);
            SendMessage(g_hBtnSend, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            g_hBtnClear = CreateWindowExW(0, L"BUTTON", L"CLEAR", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, (HMENU)IDC_BTN_CLEAR, NULL, NULL);
            SendMessage(g_hBtnClear, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            g_hEditDelay = CreateWindowExW(0, L"EDIT", L"1000", WS_CHILD|WS_VISIBLE|ES_NUMBER|WS_BORDER, 0, 0, 60, 22, hwnd, (HMENU)IDC_EDIT_DELAY, NULL, NULL);
            SendMessage(g_hEditDelay, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);

            g_hChkRepeat = CreateWindowExW(0, L"BUTTON", L"Auto", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 0, 0, 75, 20, hwnd, (HMENU)IDC_CHK_REPEAT, NULL, NULL);
            SendMessage(g_hChkRepeat, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            int repeatDelay = ReadIniInt(L"Terminal", L"RepeatDelay", 1000);
            wchar_t delayStr[32]; swprintf(delayStr, 32, L"%d", repeatDelay); SetWindowTextW(g_hEditDelay, delayStr);

            g_hEditScriptPath = CreateWindowExW(0, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_READONLY|WS_BORDER, 0, 0, 10, 22, hwnd, (HMENU)IDC_EDIT_SCRIPT_PATH, NULL, NULL);
            SendMessage(g_hEditScriptPath, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);

            g_hBtnScriptInfo = CreateWindowExW(0, L"BUTTON", L"?", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 22, 22, hwnd, (HMENU)IDC_BTN_SCRIPT_INFO, NULL, NULL);
            SendMessage(g_hBtnScriptInfo, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            g_hBtnLoadScript = CreateWindowExW(0, L"BUTTON", L"LOAD", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 40, 22, hwnd, (HMENU)IDC_BTN_LOAD_SCRIPT, NULL, NULL);
            SendMessage(g_hBtnLoadScript, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            g_hBtnRunScript = CreateWindowExW(0, L"BUTTON", L"RUN", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 40, 22, hwnd, (HMENU)IDC_BTN_RUN_SCRIPT, NULL, NULL);
            SendMessage(g_hBtnRunScript, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            {
                int histCount = ReadIniInt(L"History", L"Count", 0);
                if (histCount > MAX_HISTORY) histCount = MAX_HISTORY;
                for (int i = histCount - 1; i >= 0; i--) {
                    wchar_t key[16]; swprintf(key, 16, L"Cmd%d", i);
                    wchar_t cmdBuf[MAX_LINE_LEN] = {0};
                    ReadIniString(L"History", key, cmdBuf, MAX_LINE_LEN, L"");
                    if (wcslen(cmdBuf) > 0) {
                        int idx = (int)SendMessageW(g_hComboTx, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)cmdBuf);
                        if (idx != CB_ERR) SendMessageW(g_hComboTx, CB_DELETESTRING, (WPARAM)idx, 0);
                        SendMessageW(g_hComboTx, CB_INSERTSTRING, 0, (LPARAM)cmdBuf);
                    }
                }
            }

            for (int m = 0; m < MACRO_BANK_COUNT; m++) {
                wchar_t title[MAX_MACRO_TITLE_LEN] = {0}; wcsncpy(title, g_macroBankTitles[m], MAX_MACRO_TITLE_LEN - 1);
                if (wcslen(title) > 20) { title[20] = L'\0'; wcscat(title, L"..."); }
                g_hMacroBankBtns[m] = CreateWindowExW(0, L"BUTTON", title, WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 10, 22, hwnd, (HMENU)(LONG_PTR)(IDC_BTN_MACRO_BASE + m), NULL, NULL);
                SendMessage(g_hMacroBankBtns[m], WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            }

            HFONT hSysFont = GetStockObject(DEFAULT_GUI_FONT);
            EnumChildWindows(hwnd, SetFontCallback, (LPARAM)hSysFont);
            SendMessage(g_hComboFontSize, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);

            g_txMode = ReadIniInt(L"Terminal", L"TxMode", TX_MODE_TEXT);
            g_rxMode = ReadIniInt(L"Terminal", L"RxMode", RX_MODE_TEXT);
            update_rx_mode_ui();
            ApplyThemeToWindow(hwnd);
            return 0;
        }
        case WM_SIZE: {
            int w = LOWORD(lp), h = HIWORD(lp);
            int margin = 15, barH = 28, btnH = 25;

            if (g_hChkDump) MoveWindow(g_hChkDump, margin, margin+2, 60, 20, TRUE);
            MoveWindow(g_hComboFontSize, 80, margin, 60, 200, TRUE);
            MoveWindow(g_hComboEnc, 145, margin, 95, 200, TRUE);
            MoveWindow(g_hChkTxHex, 245, margin+2, 55, 20, TRUE);
            MoveWindow(g_hChkEcho, 305, margin+2, 75, 20, TRUE);
            MoveWindow(g_hChkTopMost, 385, margin+2, 45, 20, TRUE);
            MoveWindow(g_hBtnAbout, 435, margin, 35, 22, TRUE);

            int rxTop = margin + barH + 5;
            int bottomPanelHeight = 30 * 3 + 10;
            int rxH = h - rxTop - margin - bottomPanelHeight;
            if (rxH < 20) rxH = 20;
            MoveWindow(g_hEditRx, margin, rxTop, w-margin*2, rxH, TRUE);

            int bottomY1 = h - margin - bottomPanelHeight + 5;
            int btnSendW = 60, btnClrW = 60, suffixW = 100, infoBtnW = 22;
            int delayW = 60, repeatW = 75;
            int txW = w - margin*2 - btnSendW - btnClrW - suffixW - infoBtnW - delayW - repeatW - 50;
            if (txW < 50) txW = 50;
            MoveWindow(g_hComboTx, margin, bottomY1, txW, 200, TRUE);
            MoveWindow(g_hComboSuffix, margin + txW + 5, bottomY1, suffixW, 200, TRUE);
            MoveWindow(g_hBtnSuffixInfo, margin + txW + suffixW + 8, bottomY1, infoBtnW, btnH, TRUE);
            MoveWindow(g_hEditDelay, margin + txW + suffixW + infoBtnW + 15, bottomY1, delayW, btnH, TRUE);
            MoveWindow(g_hChkRepeat, margin + txW + suffixW + infoBtnW + 15 + delayW + 5, bottomY1+2, repeatW, 20, TRUE);
            MoveWindow(g_hBtnSend, w - margin - btnSendW - btnClrW - 15, bottomY1, btnSendW, btnH, TRUE);
            MoveWindow(g_hBtnClear, w - margin - btnClrW - 5, bottomY1, btnClrW, btnH, TRUE);

            int bottomY2 = bottomY1 + 28;
            int scriptInfoW = 22, loadBtnW = 50, runBtnW = 50;
            int scriptPathW = w - margin*2 - scriptInfoW - loadBtnW - runBtnW - 30;
            MoveWindow(g_hEditScriptPath, margin, bottomY2, scriptPathW, btnH, TRUE);
            MoveWindow(g_hBtnScriptInfo, margin + scriptPathW + 5, bottomY2, scriptInfoW, btnH, TRUE);
            MoveWindow(g_hBtnLoadScript, margin + scriptPathW + scriptInfoW + 10, bottomY2, loadBtnW, btnH, TRUE);
            MoveWindow(g_hBtnRunScript, margin + scriptPathW + scriptInfoW + loadBtnW + 15, bottomY2, runBtnW, btnH, TRUE);

            int bottomY3 = bottomY2 + 28;
            int macroBtnW = (w - margin * 2) / MACRO_BANK_COUNT;
            if (macroBtnW < 30) macroBtnW = 30;
            for (int m = 0; m < MACRO_BANK_COUNT; m++)
                MoveWindow(g_hMacroBankBtns[m], margin + m * macroBtnW, bottomY3, macroBtnW, btnH, TRUE);

            RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
            return 0;
        }
        case WM_KEYDOWN: { if (wp == VK_F1) { show_about_dialog(hwnd); return 0; } break; }
        case WM_RX_DATA_READY: { render_rx_buffer(TRUE); return 0; }
        case WM_TX_TICK: { DoAutoSendTick(hwnd); return 0; }
        case WM_TIMER: {
            if (wp == TIMER_RECONNECT_ID) {
                static int skipTicks = 2;
                EnterCriticalSection(&g_csComm); BOOL portValid = (g_hPort != INVALID_HANDLE_VALUE); LeaveCriticalSection(&g_csComm);
                if (portValid && !g_session.isWaitingReconnect) {
                    if (skipTicks > 0) { skipTicks--; return 0; }
                    DWORD ms = 0;
                    if (!GetCommModemStatus(g_hPort, &ms)) {
                        PurgeComm(g_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
                        com_close(); Sleep(50); g_session.isWaitingReconnect = TRUE; skipTicks = 2;
                        SetWindowTextW(hwnd, L"WT232 Terminal [ожидание...]"); EnableWindow(g_hBtnSend, FALSE);
                        if (g_hTxStopEvent) SetEvent(g_hTxStopEvent); g_isAutoSending = FALSE;
                        if (g_hBtnSend) SetWindowTextW(g_hBtnSend, L"SEND"); KillTimer(hwnd, TIMER_SCRIPT_ID);
                    }
                } else if (g_session.isWaitingReconnect) {
                    wchar_t fp[MAX_PORT_NAME]={0}; BOOL found=FALSE; check_and_reconnect_search(fp, &found);
                    if (found) {
                        EnterCriticalSection(&g_csComm);
                        if (g_hPort != INVALID_HANDLE_VALUE) { PurgeComm(g_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR); CloseHandle(g_hPort); g_hPort = INVALID_HANDLE_VALUE; Sleep(50); }
                        LeaveCriticalSection(&g_csComm);
                        if (com_open(fp, g_session.lastBaudrate)) {
                            g_session.isWaitingReconnect = FALSE; wcscpy(g_session.lastPortName, fp); skipTicks = 2;
                            update_terminal_title(fp, g_session.lastBaudrate); SetWindowTextW(hwnd, g_szTitle); EnableWindow(g_hBtnSend, TRUE);
                            StartRxTxThreads(hwnd);
                            if (SendMessage(g_hChkRepeat, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                                wchar_t ds[32] = {0}; GetWindowTextW(g_hEditDelay, ds, 32); DWORD d = _wtol(ds); if (d < 10) d = 10;
                                g_txIntervalMs = d; PrepareAutoSendData(); ResetEvent(g_hTxStopEvent); g_isAutoSending = TRUE;
                                if (g_hBtnSend) SetWindowTextW(g_hBtnSend, L"STOP");
                            }
                            if (g_isScriptRunning) { SetWindowTextW(g_hBtnRunScript, L"STOP"); RunNextScriptCommand(); }
                        }
                    }
                }
            } else if (wp == TIMER_SCRIPT_ID) {
                EnterCriticalSection(&g_csComm); BOOL portValid = (g_hPort != INVALID_HANDLE_VALUE); LeaveCriticalSection(&g_csComm);
                if (portValid && IsWindowVisible(g_hwndTerminal)) RunNextScriptCommand(); else KillTimer(hwnd, TIMER_SCRIPT_ID);
            }
            return 0;
        }
        case WM_COMMAND: {
            int cmdId = LOWORD(wp);
            if (cmdId >= IDC_BTN_MACRO_BASE && cmdId < IDC_BTN_MACRO_BASE + MACRO_BANK_COUNT) { ShowMacroPad(hwnd, cmdId - IDC_BTN_MACRO_BASE); return 0; }
            switch (cmdId) {
                case IDC_CHK_TOPMOST: {
                    BOOL isTop = (SendMessage(g_hChkTopMost, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (isTop) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    else SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                    WriteIniInt(L"Terminal", L"TopMost", isTop ? 1 : 0);
                    break;
                }
                case IDC_CHK_DUMP: {
                    BOOL isDump = (SendMessage(g_hChkDump, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (isDump) { g_rxMode = RX_MODE_DUMP; if (g_hChkEcho) SendMessage(g_hChkEcho, BM_SETCHECK, BST_UNCHECKED, 0); }
                    else { g_rxMode = RX_MODE_TEXT; }
                    render_rx_buffer(FALSE);
                    break;
                }
                case IDC_CHK_ECHO: {
                    BOOL isEcho = (SendMessage(g_hChkEcho, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    if (isEcho && g_rxMode == RX_MODE_DUMP) { SendMessage(g_hChkDump, BM_SETCHECK, BST_UNCHECKED, 0); g_rxMode = RX_MODE_TEXT; render_rx_buffer(FALSE); }
                    break;
                }
                case IDC_COMBO_FONT_SIZE:
                    if (HIWORD(wp) == CBN_SELCHANGE) {
                        int idx = (int)SendMessageW(g_hComboFontSize, CB_GETCURSEL, 0, 0);
                        if (idx != CB_ERR) {
                            wchar_t szS[16] = {0}; SendMessageW(g_hComboFontSize, CB_GETLBTEXT, idx, (LPARAM)szS); int s = _wtoi(szS);
                            if (s > 0) { ApplyFontSize(-MulDiv(s, 96, 72)); render_rx_buffer(FALSE); }
                            WriteIniInt(L"Terminal", L"FontSize", idx);
                        }
                    }
                    break;
                case IDC_CHK_TX_HEX:
                    g_txHexEcho = (SendMessage(g_hChkTxHex, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    break;
                case IDC_BTN_SEND: {
                    if (g_isAutoSending) {
                        if (g_hTxStopEvent) SetEvent(g_hTxStopEvent); g_isAutoSending = FALSE;
                        if (g_hBtnSend) SetWindowTextW(g_hBtnSend, L"SEND");
                        if (g_hChkRepeat) SendMessage(g_hChkRepeat, BM_SETCHECK, BST_UNCHECKED, 0);
                        InvalidateRect(g_hBtnSend, NULL, TRUE); UpdateWindow(g_hBtnSend);
                    } else {
                        if (SendMessage(g_hChkRepeat, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                            wchar_t ds[32] = {0}; if (g_hEditDelay) GetWindowTextW(g_hEditDelay, ds, 32); DWORD d = _wtol(ds); if (d < 10) d = 10;
                            g_txIntervalMs = d; com_send_ui(hwnd); PrepareAutoSendData();
                            if (g_hTxStopEvent) ResetEvent(g_hTxStopEvent);
                            if (g_hTxThread) { WaitForSingleObject(g_hTxThread, 100); CloseHandle(g_hTxThread); g_hTxThread = NULL; }
                            g_hTxThread = CreateThread(NULL, 0, TxThreadProc, hwnd, 0, NULL); g_isAutoSending = TRUE;
                            if (g_hBtnSend) SetWindowTextW(g_hBtnSend, L"STOP");
                        } else { com_send_ui(hwnd); }
                    }
                    break;
                }
                case IDC_BTN_CLEAR: {
                    if (g_hTxStopEvent) SetEvent(g_hTxStopEvent); g_isAutoSending = FALSE;
                    if (g_hBtnSend) SetWindowTextW(g_hBtnSend, L"SEND");
                    if (g_hChkRepeat) SendMessage(g_hChkRepeat, BM_SETCHECK, BST_UNCHECKED, 0);
                    SetWindowTextW(g_hEditRx, L"");
                    EnterCriticalSection(&g_csRx); g_rxRawLen = 0; g_lastRenderedLen = 0; ZeroMemory(g_rxRawBuf, sizeof(g_rxRawBuf)); LeaveCriticalSection(&g_csRx);
                    InvalidateRect(g_hEditRx, NULL, TRUE); UpdateWindow(g_hEditRx); break;
                }
                case IDC_BTN_ABOUT: show_about_dialog(hwnd); break;
                case IDC_COMBO_ENC: if (HIWORD(wp) == CBN_SELCHANGE) render_rx_buffer(FALSE); break;
                case IDC_BTN_SUFFIX_INFO: {
                    static const wchar_t suffixHelp[] =
                        L"Справка по суффиксам:\r\n"
                        L"=========================================\r\n\r\n"
                        L"Суффикс дополняет команду при отправке.\r\n"
                        L"Применяется при отправке данных.\r\n\r\n"
                        L"Резервы:\r\n"
                        L"(пусто)   без суффикса\r\n"
                        L"`0D0A`    CR+LF (Windows)\r\n"
                        L"`0A`      LF (Unix/Linux)\r\n"
                        L"`0D`      CR (Mac Classic)\r\n\r\n"
                        L"Обратные кавычки:\r\n"
                        L"Любой HEX в обратных кавычках\r\n"
                        L"`XX`, `XXXX`, `AA BB CC`\r\n"
                        L"Пробелы внутри кавычек игнорируются.\r\n"
                        L"Регистр не важен: `aa` = `AA`";
                    HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class",
                                                 L"Справка: Суффиксы", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                                 CW_USEDEFAULT, CW_USEDEFAULT, 380, 420,
                                                 hwnd, NULL, GetModuleHandle(NULL), (LPVOID)suffixHelp);
                    if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); }
                    break;
                }
                case IDC_BTN_SCRIPT_INFO: {
                    static const wchar_t scriptHelp[] = L"Справка по скриптам:\r\n=========================================\r\n\r\nФормат файла (.txt):\r\nКаждая строка - одна команда.\r\nПоддерживается инлайн-HEX (`XX`).\r\n\r\nДирективы:\r\n1000        - глобальная задержка (мс)\r\n(только в первой строке)\r\n#DELAY 500  - задержка перед след. командой\r\n#STOP       - остановить скрипт\r\n#...        - комментарий (игнорируется)\r\n\r\nБез #STOP скрипт выполняется циклически.";
                    HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class", L"Справка: Скрипты", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 380, 450, hwnd, NULL, GetModuleHandle(NULL), (LPVOID)scriptHelp);
                    if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); } break;
                }
                case IDC_BTN_LOAD_SCRIPT: {
                    OPENFILENAMEW ofn; wchar_t szFile[MAX_PATH] = L""; ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = szFile; ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0"; ofn.nFilterIndex = 1; ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileNameW(&ofn)) LoadScriptFile(szFile); break;
                }
                case IDC_BTN_RUN_SCRIPT: {
                    if (g_isScriptRunning) StopScript();
                    else {
                        if (g_scriptCount > 0) { g_isScriptRunning = TRUE; SetWindowTextW(g_hBtnRunScript, L"STOP"); g_scriptCurrentIndex = 0; RunNextScriptCommand(); }
                        else MessageBoxW(hwnd, L"Сначала загрузите файл скрипта!", L"Внимание", MB_ICONWARNING);
                    }
                    break;
                }
            }
            return 0;
        }
        case WM_CLOSE: {
            KillTimer(hwnd, TIMER_RECONNECT_ID); KillTimer(hwnd, TIMER_SCRIPT_ID); StopRxTxThreads(); ResetMacroLoadedFlags();
            for (int m = 0; m < MACRO_BANK_COUNT; m++) { if (g_hwndMacroPads[m] && IsWindow(g_hwndMacroPads[m])) DestroyWindow(g_hwndMacroPads[m]); }
            if (g_hwndMacroEdit && IsWindow(g_hwndMacroEdit)) DestroyWindow(g_hwndMacroEdit);
            SaveAllMacroWindowsState();
            int historyCount = (int)SendMessageW(g_hComboTx, CB_GETCOUNT, 0, 0); if (historyCount > MAX_HISTORY) historyCount = MAX_HISTORY;
            WriteIniInt(L"History", L"Count", historyCount);
            for (int i = 0; i < historyCount; i++) { wchar_t key[16]; wchar_t buf[MAX_LINE_LEN] = {0}; swprintf(key, 16, L"Cmd%d", i); SendMessageW(g_hComboTx, CB_GETLBTEXT, i, (LPARAM)buf); WriteIniString(L"History", key, buf); }
            WriteIniInt(L"Terminal", L"Echo", is_echo_enabled() ? 1 : 0);
            WriteIniInt(L"Terminal", L"TxHexEcho", g_txHexEcho ? 1 : 0);
            WriteIniInt(L"Terminal", L"TxMode", g_txMode);
            WriteIniInt(L"Terminal", L"RxMode", g_rxMode);
            WriteIniInt(L"Terminal", L"DumpMode", (SendMessage(g_hChkDump, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0);
            WriteIniInt(L"Terminal", L"Encoding", (int)SendMessageW(g_hComboEnc, CB_GETCURSEL, 0, 0));
            WriteIniInt(L"Terminal", L"TopMost", (SendMessage(g_hChkTopMost, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0);
            wchar_t delayStr[32] = {0}; GetWindowTextW(g_hEditDelay, delayStr, 32); WriteIniInt(L"Terminal", L"RepeatDelay", _wtoi(delayStr));
            RECT rc; GetWindowRect(hwnd, &rc);
            WriteIniInt(L"Terminal", L"X", rc.left); WriteIniInt(L"Terminal", L"Y", rc.top);
            WriteIniInt(L"Terminal", L"Width", rc.right - rc.left); WriteIniInt(L"Terminal", L"Height", rc.bottom - rc.top);
            WINDOWPLACEMENT wpl; wpl.length = sizeof(WINDOWPLACEMENT); GetWindowPlacement(hwnd, &wpl); WriteIniInt(L"Terminal", L"State", wpl.showCmd);
            WriteAllIni(); com_close(); DestroyWindow(hwnd); g_hwndTerminal = NULL; g_pfnOrigEditProc = NULL;
            wchar_t lastPort[MAX_PORT_NAME] = {0}; ReadIniString(L"Port", L"LastPortName", lastPort, MAX_PORT_NAME, L"");
            if (g_hwndSettings && IsWindow(g_hwndSettings)) { ShowWindow(g_hwndSettings, SW_SHOW); SetForegroundWindow(g_hwndSettings); init_com_ports(g_hwndSettings, FALSE, lastPort); }
            else {
                wchar_t st[128]; swprintf(st, 128, L"WT232 Settings " APP_VERSION);
                g_hwndSettings = CreateWindowExW(WS_EX_TOPMOST, L"WT232_Settings_Class", st, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 300, 330, NULL, NULL, GetModuleHandle(NULL), NULL);
                if (g_hwndSettings) { init_com_ports(g_hwndSettings, FALSE, lastPort); ApplyThemeToWindow(g_hwndSettings); ShowWindow(g_hwndSettings, SW_SHOW); UpdateWindow(g_hwndSettings); }
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ===КОНЕЦ ФАЙЛА===
