// ============================================================================
// WT232 Terminal v0.1
// ============================================================================

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

// ============================================================================
// ТЕМЫ ОФОРМЛЕНИЯ
// ============================================================================
#define THEME_LIGHT 0
#define THEME_DARK  1

typedef struct {
    COLORREF bgColor;
    COLORREF fgColor;
    COLORREF btnBg;
    COLORREF btnFg;
    COLORREF editBg;
    COLORREF editFg;
    COLORREF panelBg;
    COLORREF panelBorder;
} ThemeColors;

static ThemeColors g_themes[] = {
    // THEME_LIGHT - светлая
    { RGB(240, 240, 245), RGB(0, 0, 0), RGB(230, 230, 240), RGB(0, 0, 0), RGB(255, 255, 255), RGB(0, 0, 0), RGB(220, 220, 230), RGB(200, 200, 210) },
    // THEME_DARK - полутемная (читаемая)
    { RGB(200, 200, 210), RGB(0, 0, 0), RGB(180, 180, 195), RGB(0, 0, 0), RGB(235, 235, 245), RGB(0, 0, 0), RGB(190, 190, 200), RGB(160, 160, 175) }
};

static int g_currentTheme = THEME_LIGHT;
static ThemeColors *g_pTheme = &g_themes[THEME_LIGHT];

// ============================================================================
// ВЕРСИЯ
// ============================================================================
#define APP_VERSION L"v0.1"

// ============================================================================
// ИДЕНТИФИКАТОРЫ КОНТРОЛОВ
// ============================================================================
#define IDC_COMBO_PORT      1001
#define IDC_COMBO_BAUD      1002
#define IDC_COMBO_DATABITS  1011
#define IDC_COMBO_PARITY    1012
#define IDC_COMBO_STOPBITS  1013
#define IDC_COMBO_FLOW      1004
#define IDC_BTN_INFO        1007
#define IDC_EDIT_RX         1009
#define IDC_COMBO_TX        1020
#define IDC_COMBO_SUFFIX    1021
#define IDC_BTN_SUFFIX_INFO 1022
#define IDC_BTN_SEND        1006
#define IDC_BTN_CLEAR       1014
#define IDC_CHK_ECHO        1018
#define IDC_COMBO_ENC       1015
#define IDC_BTN_TX_MODE     1024
#define IDC_BTN_RX_REFRESH  1023
#define IDC_BTN_ABOUT       1025
#define IDC_EDIT_DELAY      1027
#define IDC_CHK_REPEAT      1028
#define IDC_BTN_RX_MODE     1035
#define IDC_COMBO_FONT_SIZE 1036
#define IDC_EDIT_MACRO_TITLE 1037
#define IDC_BTN_THEME       1038
#define MAX_MACRO_TITLE_LEN  32

// Скрипты
#define IDC_EDIT_SCRIPT_PATH 1029
#define IDC_BTN_LOAD_SCRIPT  1030
#define IDC_BTN_RUN_SCRIPT   1031
#define IDC_BTN_SCRIPT_INFO  1032

// Макросы
#define IDC_BTN_MACRO_BASE   1050
#define IDC_BTN_MACRO_MODE   (IDC_BTN_MACRO_BASE + MACROS_PER_BANK)
#define IDC_BTN_MACRO_DISPLAY (IDC_BTN_MACRO_BASE + MACROS_PER_BANK + 1)

// Окно редактирования макроса
#define IDC_EDIT_MACRO_NAME  3001
#define IDC_EDIT_MACRO_CMD   3002
#define IDC_BTN_MACRO_SAVE   3003
#define IDC_BTN_MACRO_CANCEL 3004

// Кнопки OK/Отмена
#define IDC_BTN_OK          1008
#define IDC_BTN_CANCEL      1009

// ============================================================================
// КОНСТАНТЫ
// ============================================================================
#define MAX_PORT_NAME       64
#define RX_BUF_SIZE         4096
#define MAX_HISTORY         30
#define MAX_SCRIPT_LINES    500
#define MAX_LINE_LEN        256
#define MACRO_BANK_COUNT    5
#define MACROS_PER_BANK     15
#define MACRO_COLS          3
#define MACRO_ROWS          5
#define MACRO_LABEL_LEN     32
#define MACRO_CMD_LEN       256

#define FLOW_NONE     0
#define FLOW_XONXOFF  1
#define FLOW_RTSCTS   2
#define FLOW_RS485    3
#define TX_MODE_HEX   0
#define TX_MODE_TEXT  1
#define RX_MODE_TEXT  0
#define RX_MODE_DUMP  1
#define DUMP_LINE_SIZE 16

#define TIMER_RECONNECT_ID 2001
#define TIMER_READ_ID      2002
#define TIMER_REPEAT_ID    2003
#define TIMER_SCRIPT_ID    2004

// ============================================================================
// СТРУКТУРЫ
// ============================================================================
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
    const wchar_t *name;
    UINT codepage;
} EncodingEntry;

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================
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
HWND g_hBtnTxMode = NULL;
HWND g_hBtnRxRefresh = NULL;
HWND g_hBtnAbout = NULL;
HWND g_hBtnTheme = NULL;
HWND g_hEditDelay = NULL;
HWND g_hChkRepeat = NULL;
HWND g_hBtnRxMode = NULL;
HWND g_hComboFontSize = NULL;
HWND g_hEditScriptPath = NULL;
HWND g_hBtnLoadScript = NULL;
HWND g_hBtnRunScript = NULL;
HWND g_hBtnScriptInfo = NULL;
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
static wchar_t g_usbInstanceId[512] = {0};
static wchar_t g_regBuffer[512] = {0};
static wchar_t g_infoReport[8192] = {0};
static wchar_t g_szTitle[256] = {0};
static wchar_t g_wTxtBuf[1024] = {0};
static BYTE g_binBuf[1024] = {0};
static BYTE g_rxRawBuf[RX_BUF_SIZE];
static DWORD g_rxRawLen = 0;
static DWORD g_lastRenderedLen = 0;
static int g_hexLinePos = 0;
static com_session_t g_session = {0};

static wchar_t g_iniPath[MAX_PATH] = {0};
static wchar_t g_iniBackupPath[MAX_PATH] = {0};

static ScriptItem g_scriptItems[MAX_SCRIPT_LINES];
static int g_scriptCount = 0;
static int g_scriptCurrentIndex = 0;
static BOOL g_isScriptRunning = FALSE;
static BOOL g_scriptHasStopMarker = FALSE;

static MacroSlot g_macroBanks[MACRO_BANK_COUNT][MACROS_PER_BANK];
static BOOL g_macroBankLoaded[MACRO_BANK_COUNT] = {0};
static HWND g_hwndMacroPads[MACRO_BANK_COUNT] = {0};
static HWND g_hwndMacroEdit = NULL;
static int g_editingSlot = -1;
static int g_editingBank = -1;
static BOOL g_editMode = FALSE;
static BOOL g_showCommand = FALSE;

static const EncodingEntry g_encodings[] = {
    { L"ASCII",         20127 },
    { L"UTF-8",         65001 },
    { L"CP1251",        1251  },
    { L"CP866",         866   },
    { L"CP1252",        1252  },
    { L"KOI8-R",        20866 },
};
static const int g_encodingCount = sizeof(g_encodings) / sizeof(g_encodings[0]);

static const wchar_t g_aboutText[] =
    L"WT232 Terminal " APP_VERSION L"\r\n"
    L"==========================================\r\n"
    L"\u041e\u043f\u0438\u0441\u0430\u043d\u0438\u0435:\r\n"
    L"  \u0422\u0435\u0440\u043c\u0438\u043d\u0430\u043b \u0434\u043b\u044f \u0440\u0430\u0431\u043e\u0442\u044b \u0441 COM-\u043f\u043e\u0440\u0442\u0430\u043c\u0438.\r\n"
    L"  \u041f\u043e\u0434\u0434\u0435\u0440\u0436\u043a\u0430 HEX/TEXT/DUMP \u0440\u0435\u0436\u0438\u043c\u043e\u0432,\r\n"
    L"  \u0438\u043d\u043b\u0430\u0439\u043d-HEX (`XX`), \u043a\u043e\u0434\u0438\u0440\u043e\u0432\u043e\u043a\r\n"
    L"  CP1251/UTF-8/ASCII/KOI8-R,\r\n"
    L"  \u0430\u0432\u0442\u043e\u043f\u0435\u0440\u0435\u043f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d\u0438\u044f \u043f\u043e USB-\u0434\u0435\u0441\u043a\u0440\u0438\u043f\u0442\u043e\u0440\u0430\u043c.\r\n"
    L"  \u041c\u0430\u043a\u0440\u043e\u0441\u044b (5 \u0431\u0430\u043d\u043a\u043e\u0432 x 15 \u044f\u0447\u0435\u0435\u043a).\r\n"
    L"==========================================\r\n"
    L"MIT License\r\n"
    L"Copyright (c) 2026 IgerOK\r\n"
    L"https://github.com/IgerOK/WT232\r\n"
    L"==========================================\r\n";

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ
// ============================================================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK TerminalWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK InfoWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK TxEditSubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK MacroPadWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK MacroEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL CALLBACK SetFontCallback(HWND hWndChild, LPARAM lParam);
BOOL CALLBACK ApplyThemeCallback(HWND hWndChild, LPARAM lParam);

void init_com_ports(HWND hwndParent, BOOL showDetails, const wchar_t *saveTargetName);
void check_and_reconnect_search(wchar_t *outFoundPortName, BOOL *pIsFound);
void com_close(void);
BOOL com_open(const wchar_t *portName, int baudrate);
void get_advanced_usb_descriptors(HDEVINFO hDevInfo, SP_DEVINFO_DATA *pDevInfo,
    const wchar_t *instanceId, wchar_t *outVid, wchar_t *outPid, wchar_t *outSerial,
    wchar_t *outMfg, wchar_t *outProduct, wchar_t *outRawSerial, wchar_t *outCdcInfo);
void com_send(HWND hwndParent);
void update_terminal_title(const wchar_t *portName, int baudrate);
void render_rx_buffer(BOOL appendMode);
UINT get_selected_codepage(void);
int get_selected_tx_mode(void);
BOOL is_echo_enabled(void);
void append_rx_text_colored(const wchar_t *text, COLORREF color);
void add_to_history(const wchar_t *msg);
DWORD parse_text_with_inline_hex(const wchar_t *src, BYTE *dst, DWORD dstMax, UINT codepage);
static int hex_char_val(wchar_t c);
static HFONT CreateMonoFont(int height);
static void update_tx_mode_ui(void);
static void update_rx_mode_ui(void);
static void LoadScriptFile(const wchar_t* path);
static void StopScript(void);
static void RunNextScriptCommand(void);
static void ApplyFontSize(int height);
static void show_about_dialog(HWND hParent);
static void LoadMacroBank(int bankIndex);
static void SaveMacroBank(int bankIndex);
static void SendMacroCommand(int bankIndex, int slotIndex);
static void UpdateMacroButtons(int bankIndex);
static void ShowMacroPad(HWND hParent, int bankIndex);
static void LayoutButtons(HWND hwnd);
static void CloseMacroEdit(void);
static void SaveMacroFromEdit(void);
static void ShowMacroEditWindow(HWND hParent, int bankIndex, int slotIndex);
static void SaveMacroPadPosition(int bankIndex, int x, int y);
static void LoadMacroPadPosition(int bankIndex, int *px, int *py);
static void SaveAllMacroWindowsState(void);
static void ApplyThemeToWindow(HWND hwnd);
static void SetTheme(int theme);

static void InitIniPaths(void);
static BOOL ReadIniString(const wchar_t *section, const wchar_t *key, wchar_t *out, int maxLen, const wchar_t *defVal);
static int ReadIniInt(const wchar_t *section, const wchar_t *key, int defVal);
static void WriteIniString(const wchar_t *section, const wchar_t *key, const wchar_t *val);
static void WriteIniInt(const wchar_t *section, const wchar_t *key, int val);
static BOOL ValidateIni(void);
static BOOL ReadAllIni(void);
static void WriteAllIni(void);
static void CreateDefaultIni(void);

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ, РАБОТА С ПОРТОМ И РЕНДЕРИНГ
// ============================================================================

static HFONT CreateMonoFont(int height) {
    return CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Lucida Console");
}

static HFONT CreateTitleFont(int height) {
    return CreateFontW(height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

static void ApplyFontSize(int height) {
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

static void update_tx_mode_ui(void) {
    if (!g_hBtnTxMode) return;
    SetWindowTextW(g_hBtnTxMode, (g_txMode == TX_MODE_HEX) ? L"TX:HEX" : L"TX:TEXT");
}

static void update_rx_mode_ui(void) {
    if (!g_hBtnRxMode) return;
    SetWindowTextW(g_hBtnRxMode, (g_rxMode == RX_MODE_TEXT) ? L"RX:TEXT" : L"RX:DUMP");
}

static int hex_char_val(wchar_t c) {
    if (c >= L'0' && c <= L'9') return c - L'0';
    if (c >= L'A' && c <= L'F') return c - L'A' + 10;
    if (c >= L'a' && c <= L'f') return c - L'a' + 10;
    return -1;
}

void com_close(void) {
    if (g_hPort != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hPort);
        g_hPort = INVALID_HANDLE_VALUE;
    }
}

void update_terminal_title(const wchar_t *portName, int baudrate) {
    const wchar_t *szParity = L"N";
    int pIdx = (int)SendMessageW(g_hComboParity, CB_GETCURSEL, 0, 0);
    switch (pIdx) {
        case 1: szParity = L"E"; break;
        case 2: szParity = L"O"; break;
        case 3: szParity = L"M"; break;
        case 4: szParity = L"S"; break;
    }
    wchar_t szDB[4] = L"8";
    int dIdx = (int)SendMessageW(g_hComboDataBits, CB_GETCURSEL, 0, 0);
    switch (dIdx) {
        case 0: wcscpy(szDB, L"5"); break;
        case 1: wcscpy(szDB, L"6"); break;
        case 2: wcscpy(szDB, L"7"); break;
    }
    wchar_t szSB[4] = L"1";
    int sIdx = (int)SendMessageW(g_hComboStopBits, CB_GETCURSEL, 0, 0);
    switch (sIdx) {
        case 1: wcscpy(szSB, L"1.5"); break;
        case 2: wcscpy(szSB, L"2"); break;
    }
    wchar_t szFlow[64] = {0};
    int fIdx = (int)SendMessageW(g_hComboFlow, CB_GETCURSEL, 0, 0);
    if (fIdx != CB_ERR) SendMessageW(g_hComboFlow, CB_GETLBTEXT, fIdx, (LPARAM)szFlow);
    else wcscpy(szFlow, L"\u041d\u0435\u0442");

    ZeroMemory(g_szTitle, sizeof(g_szTitle));
    swprintf(g_szTitle, sizeof(g_szTitle)/sizeof(wchar_t),
        L"WT232 Terminal " APP_VERSION L" - [%ls | %d bps | %ls-%ls-%ls | %ls]",
        portName, baudrate, szDB, szParity, szSB, szFlow);
}

BOOL com_open(const wchar_t *portName, int baudrate) {
    wchar_t szPath[MAX_PORT_NAME + 8] = {0};
    swprintf(szPath, sizeof(szPath)/sizeof(wchar_t), L"\\\\.\\%ls", portName);
    g_hPort = CreateFileW(szPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (g_hPort == INVALID_HANDLE_VALUE) return FALSE;

    PurgeComm(g_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
    EscapeCommFunction(g_hPort, CLRDTR);
    EscapeCommFunction(g_hPort, CLRRTS);
    Sleep(10);
    EscapeCommFunction(g_hPort, SETDTR);
    EscapeCommFunction(g_hPort, SETRTS);

    DCB dcb;
    ZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(g_hPort, &dcb)) {
        PurgeComm(g_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
        com_close();
        return FALSE;
    }
    dcb.BaudRate = baudrate;
    int dbIdx = (int)SendMessageW(g_hComboDataBits, CB_GETCURSEL, 0, 0);
    switch (dbIdx) {
        case 0: dcb.ByteSize = 5; break;
        case 1: dcb.ByteSize = 6; break;
        case 2: dcb.ByteSize = 7; break;
        default: dcb.ByteSize = 8; break;
    }
    int parIdx = (int)SendMessageW(g_hComboParity, CB_GETCURSEL, 0, 0);
    switch (parIdx) {
        case 1: dcb.Parity = EVENPARITY; break;
        case 2: dcb.Parity = ODDPARITY; break;
        case 3: dcb.Parity = MARKPARITY; break;
        case 4: dcb.Parity = SPACEPARITY; break;
        default: dcb.Parity = NOPARITY; break;
    }
    int sbIdx = (int)SendMessageW(g_hComboStopBits, CB_GETCURSEL, 0, 0);
    switch (sbIdx) {
        case 1: dcb.StopBits = ONE5STOPBITS; break;
        case 2: dcb.StopBits = TWOSTOPBITS; break;
        default: dcb.StopBits = ONESTOPBIT; break;
    }
    int flowMode = (int)SendMessageW(g_hComboFlow, CB_GETCURSEL, 0, 0);
    dcb.fOutxCtsFlow = FALSE; dcb.fInX = FALSE; dcb.fOutX = FALSE;
    if (flowMode == FLOW_RS485) dcb.fRtsControl = RTS_CONTROL_TOGGLE;
    else if (flowMode == FLOW_RTSCTS) { dcb.fOutxCtsFlow = TRUE; dcb.fRtsControl = RTS_CONTROL_HANDSHAKE; }
    else if (flowMode == FLOW_XONXOFF) { dcb.fInX = TRUE; dcb.fOutX = TRUE; dcb.fRtsControl = RTS_CONTROL_DISABLE; }
    else dcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(g_hPort, &dcb)) {
        PurgeComm(g_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
        com_close();
        return FALSE;
    }
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 50;
    SetCommTimeouts(g_hPort, &timeouts);

    g_rxRawLen = 0;
    ZeroMemory(g_rxRawBuf, sizeof(g_rxRawBuf));
    g_lastRenderedLen = 0;
    g_hexLinePos = 0;
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

void append_rx_text_colored(const wchar_t *text, COLORREF color) {
    if (!g_hEditRx || !text || wcslen(text) == 0) return;
    int len = GetWindowTextLengthW(g_hEditRx);
    SendMessageW(g_hEditRx, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    SendMessageW(g_hEditRx, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessageW(g_hEditRx, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(g_hEditRx, WM_VSCROLL, SB_BOTTOM, 0);
}

void render_rx_buffer(BOOL appendMode) {
    if (g_rxRawLen == 0) return;
    UINT cp = get_selected_codepage();
    if (!appendMode) {
        SetWindowTextW(g_hEditRx, L"");
        g_lastRenderedLen = 0;
        g_hexLinePos = 0;
    }
    if (appendMode && g_rxRawLen <= g_lastRenderedLen) return;

    wchar_t wBuf[RX_BUF_SIZE * 6 + 1] = {0};
    int wLen = 0;
    int maxW = (int)(sizeof(wBuf)/sizeof(wchar_t)) - 10;

    if (g_rxMode == RX_MODE_DUMP) {
        DWORD startIdx = appendMode ? g_lastRenderedLen : 0;
        if (!appendMode) startIdx = 0;
        DWORD limit = g_rxRawLen;
        BOOL forceFullRender = !appendMode;
        for (DWORD lineStart = startIdx; lineStart < limit && wLen < maxW; lineStart += DUMP_LINE_SIZE) {
            DWORD lineEnd = lineStart + DUMP_LINE_SIZE;
            BOOL isPartial = (lineEnd > g_rxRawLen);
            if (isPartial && !forceFullRender) break;
            DWORD actualEnd = (lineEnd > g_rxRawLen) ? g_rxRawLen : lineEnd;
            wLen += swprintf(wBuf + wLen, 12, L"%08X: ", lineStart);
            for (DWORD i = lineStart; i < actualEnd; i++)
                wLen += swprintf(wBuf + wLen, 4, L"%02X ", g_rxRawBuf[i]);
            for (DWORD i = actualEnd; i < lineEnd; i++)
                wLen += swprintf(wBuf + wLen, 4, L"   ");
            wLen += swprintf(wBuf + wLen, 6, L"    ");
            wchar_t asciiLine[64] = {0};
            int charsConverted = 0;
            if (actualEnd > lineStart) {
                charsConverted = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS,
                    (LPCSTR)(g_rxRawBuf + lineStart), actualEnd - lineStart, asciiLine, 64);
            }
            int charIdx = 0;
            for (DWORD i = lineStart; i < lineEnd; i++) {
                wchar_t ch = L'.';
                if (i < actualEnd) {
                    if (charIdx < charsConverted) {
                        ch = asciiLine[charIdx++];
                        if (ch < 0x20 || ch == 0x7F) ch = L'.';
                    }
                }
                wLen += swprintf(wBuf + wLen, 2, L"%c", ch);
            }
            wLen += swprintf(wBuf + wLen, 4, L"\r\n");
        }
        if (forceFullRender) g_lastRenderedLen = g_rxRawLen;
        else g_lastRenderedLen = (g_rxRawLen / DUMP_LINE_SIZE) * DUMP_LINE_SIZE;
    } else {
        int newBytesStart = appendMode ? g_lastRenderedLen : 0;
        int newBytesCount = g_rxRawLen - newBytesStart;
        if (newBytesCount > 0) {
            int convertedLen = MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS,
                (LPCSTR)(g_rxRawBuf + newBytesStart), newBytesCount, wBuf, maxW);
            if (convertedLen <= 0) {
                for (int k = 0; k < newBytesCount && wLen < maxW; k++) {
                    swprintf(wBuf + wLen, 4, L"%02X ", g_rxRawBuf[newBytesStart + k]);
                    wLen += 3;
                }
            } else {
                wLen = convertedLen;
            }
        }
        g_lastRenderedLen = g_rxRawLen;
    }
    wBuf[wLen] = L'\0';
    if (wLen > 0) {
        COLORREF textColor = (g_rxMode == RX_MODE_DUMP) ? g_pTheme->fgColor : RGB(0, 0, 200);
        append_rx_text_colored(wBuf, textColor);
    }
}

void add_to_history(const wchar_t *msg) {
    if (!g_hComboTx || !msg || wcslen(msg) == 0) return;
    int idx = (int)SendMessageW(g_hComboTx, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)msg);
    if (idx != CB_ERR) SendMessageW(g_hComboTx, CB_DELETESTRING, (WPARAM)idx, 0);
    SendMessageW(g_hComboTx, CB_INSERTSTRING, 0, (LPARAM)msg);
    int count = (int)SendMessageW(g_hComboTx, CB_GETCOUNT, 0, 0);
    while (count > MAX_HISTORY) {
        SendMessageW(g_hComboTx, CB_DELETESTRING, (WPARAM)(count - 1), 0);
        count--;
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

DWORD parse_text_with_inline_hex(const wchar_t *src, BYTE *dst, DWORD dstMax, UINT codepage) {
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
                    for (DWORD b = 0; b < tmpHexLen && binLen < dstMax; b++)
                        dst[binLen++] = tmpHex[b];
                    i = closePos + 1;
                    continue;
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

// ============================================================================
// USB ДЕСКРИПТОРЫ, ИНФО-ОКНО И COM-ПОРТЫ
// ============================================================================

void get_advanced_usb_descriptors(HDEVINFO hDevInfo, SP_DEVINFO_DATA *pDevInfo, const wchar_t *instanceId,
    wchar_t *outVid, wchar_t *outPid, wchar_t *outSerial,
    wchar_t *outMfg, wchar_t *outProduct, wchar_t *outRawSerial, wchar_t *outCdcInfo)
{
    wcscpy(outVid, L"N/A"); wcscpy(outPid, L"N/A"); wcscpy(outSerial, L"N/A");
    wcscpy(outMfg, L"N/A"); wcscpy(outProduct, L"N/A"); wcscpy(outRawSerial, L"N/A");
    wcscpy(outCdcInfo, L"N/A (Standard Serial)");
    if (instanceId) {
        const wchar_t *pVid = wcsstr(instanceId, L"VID_");
        if (pVid) { wcsncpy(outVid, pVid + 4, 4); outVid[4] = L'\0'; }
        const wchar_t *pPid = wcsstr(instanceId, L"PID_");
        if (pPid) { wcsncpy(outPid, pPid + 4, 4); outPid[4] = L'\0'; }
        const wchar_t *pSlash = wcsrchr(instanceId, L'\\');
        if (pSlash) { wcsncpy(outSerial, pSlash + 1, 63); outSerial[63] = L'\0'; }
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
                const wchar_t *pPV = wcsstr(g_usbInstanceId, L"VID_");
                const wchar_t *pPP = wcsstr(g_usbInstanceId, L"PID_");
                if (pPV && pPP && wcsncmp(pPV+4, outVid, 4)==0 && wcsncmp(pPP+4, outPid, 4)==0) {
                    const wchar_t *pLS = wcsrchr(g_usbInstanceId, L'\\');
                    if (pLS) { wcsncpy(outRawSerial, pLS+1, 127); outRawSerial[127]=L'\0'; }
                    HKEY hUsbKey = SetupDiOpenDevRegKey(hUsbInfo, &usbDevData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
                    if (hUsbKey != INVALID_HANDLE_VALUE) {
                        DWORD cb = sizeof(g_regBuffer);
                        if (RegQueryValueExW(hUsbKey, L"busi_ManufacturerString", NULL, NULL, (LPBYTE)g_regBuffer, &cb)==ERROR_SUCCESS && wcslen(g_regBuffer)>0)
                            { wcsncpy(outMfg, g_regBuffer, 127); outMfg[127]=L'\0'; }
                        else { cb=sizeof(g_regBuffer);
                            if (RegQueryValueExW(hUsbKey, L"UIParentMFG", NULL, NULL, (LPBYTE)g_regBuffer, &cb)==ERROR_SUCCESS && wcslen(g_regBuffer)>0)
                                { wcsncpy(outMfg, g_regBuffer, 127); outMfg[127]=L'\0'; }
                            else { DWORD sz=0;
                                if (SetupDiGetDeviceRegistryPropertyW(hUsbInfo, &usbDevData, SPDRP_MFG, NULL, (PBYTE)g_regBuffer, sizeof(g_regBuffer), &sz)) {
                                    if (!wcsstr(g_regBuffer, L"\u041c\u0430\u0439\u043a\u0440\u043e\u0441\u043e\u0444\u0442") && !wcsstr(g_regBuffer, L"\u0445\u043e\u0441\u0442"))
                                        { wcsncpy(outMfg, g_regBuffer, 127); outMfg[127]=L'\0'; }
                                    else wcscpy(outMfg, L"PLANAR-S0");
                                }
                            }
                        }
                        cb=sizeof(g_regBuffer);
                        if (RegQueryValueExW(hUsbKey, L"busi_ProductString", NULL, NULL, (LPBYTE)g_regBuffer, &cb)==ERROR_SUCCESS && wcslen(g_regBuffer)>0)
                            { wcsncpy(outProduct, g_regBuffer, 127); outProduct[127]=L'\0'; }
                        else { DWORD sz=0;
                            if (SetupDiGetDeviceRegistryPropertyW(hUsbInfo, &usbDevData, 24, NULL, (PBYTE)g_regBuffer, sizeof(g_regBuffer), &sz) && wcslen(g_regBuffer)>0)
                                { wcsncpy(outProduct, g_regBuffer, 127); outProduct[127]=L'\0'; }
                            else { if (SetupDiGetDeviceRegistryPropertyW(hUsbInfo, &usbDevData, SPDRP_DEVICEDESC, NULL, (PBYTE)g_regBuffer, sizeof(g_regBuffer), &sz)) {
                                if (!wcsstr(g_regBuffer, L"\u0421\u043e\u0441\u0442\u0430\u0432\u043d\u043e\u0435"))
                                    { wcsncpy(outProduct, g_regBuffer, 127); outProduct[127]=L'\0'; }
                                else wcscpy(outProduct, L"SSI-ABZ");
                            }}
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
            CREATESTRUCTW *pCreate = (CREATESTRUCTW*)lp;
            if (pCreate && pCreate->lpCreateParams) SetWindowTextW(hEditLog, (const wchar_t*)pCreate->lpCreateParams);
            return 0;
        }
        case WM_SIZE: { 
            if (hEditLog) {
                MoveWindow(hEditLog, 0, 0, LOWORD(lp), HIWORD(lp), TRUE);
                InvalidateRect(hEditLog, NULL, TRUE);
            }
            return 0; 
        }
        case WM_CLOSE: { DestroyWindow(hwnd); return 0; }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->editBg);
            SetTextColor(hdc, g_pTheme->editFg);
            return (LRESULT)CreateSolidBrush(g_pTheme->editBg);
        }
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wp;
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(g_pTheme->bgColor);
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            return 1;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void show_about_dialog(HWND hParent) {
    HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class",
        L"\u041e \u043f\u0440\u043e\u0433\u0440\u0430\u043c\u043c\u0435",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 480, 520,
        hParent, NULL, GetModuleHandle(NULL), (LPVOID)g_aboutText);
    if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); }
}

void init_com_ports(HWND hwndParent, BOOL showDetails, const wchar_t *saveTargetName) {
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
            wchar_t *pStart = wcsstr(fn, L"(COM");
            if (!pStart) pStart = wcsstr(fn, L" (COM");
            if (pStart) {
                if (*pStart == L' ') pStart++; pStart++;
                wchar_t *pEnd = wcschr(pStart, L')');
                if (pEnd) {
                    size_t len = (size_t)(pEnd - pStart);
                    if (len < MAX_PORT_NAME) {
                        wchar_t sn[MAX_PORT_NAME]={0}; wcsncpy(sn, pStart, len); sn[len]=L'\0';
                        if (!showDetails) SendMessageW(g_hComboPort, CB_ADDSTRING, 0, (LPARAM)sn);
                        portsFoundCount++;
                        wchar_t cv[16],cp[16],cs[64],cm[128],cpr[128],cr[128],cc[128];
                        get_advanced_usb_descriptors(hDevInfo, &devInfo, iid, cv,cp,cs,cm,cpr,cr,cc);
                        if (saveTargetName && wcscmp(sn, saveTargetName)==0) {
                            wcscpy(g_session.targetDevice.vid, cv);
                            wcscpy(g_session.targetDevice.pid, cp);
                            wcscpy(g_session.targetDevice.serial, cs);
                        }
                        if (showDetails) {
                            size_t w = swprintf(g_infoReport+reportPos, (sizeof(g_infoReport)/sizeof(wchar_t))-reportPos,
                                L"\u041f\u043e\u0440\u0442: %ls\r\n\u0418\u043c\u044f: %ls\r\nVID: %ls | PID: %ls\r\n"
                                L"Serial: %ls\r\nMfg: %ls\r\nProduct: %ls\r\nRaw S/N: %ls\r\nClass: %ls\r\n"
                                L"--------------------------------------------------\r\n\r\n",
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
        if (idx != CB_ERR) {
            SendMessage(g_hComboPort, CB_SETCURSEL, idx, 0);
        }
    }
    
    SetupDiDestroyDeviceInfoList(hDevInfo);
    
    if (!showDetails && SendMessage(g_hComboPort, CB_GETCOUNT, 0, 0) > 0) {
        if (!saveTargetName || wcslen(saveTargetName) == 0) {
            SendMessage(g_hComboPort, CB_SETCURSEL, 0, 0);
        }
    }
    if (showDetails) {
        if (portsFoundCount == 0)
            MessageBoxW(hwndParent, L"\u0410\u043a\u0442\u0438\u0432\u043d\u044b\u0435 COM-\u043f\u043e\u0440\u0442\u044b \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d\u044b!", L"\u0418\u043d\u0444\u043e", MB_ICONINFORMATION|MB_OK);
        else {
            HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class", L"\u0421\u043f\u0438\u0441\u043e\u043a \u0443\u0441\u0442\u0440\u043e\u0439\u0441\u0442\u0432",
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 460, 360,
                hwndParent, NULL, GetModuleHandle(NULL), (LPVOID)g_infoReport);
            if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); }
        }
    }
}

void check_and_reconnect_search(wchar_t *outFoundPortName, BOOL *pIsFound) {
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
            if (wcscmp(cv, g_session.targetDevice.vid)==0 &&
                wcscmp(cp, g_session.targetDevice.pid)==0 &&
                wcscmp(cs, g_session.targetDevice.serial)==0) {
                wchar_t *pS = wcsstr(fn, L"(COM");
                if (!pS) pS = wcsstr(fn, L" (COM");
                if (pS) {
                    if (*pS == L' ') pS++; pS++;
                    wchar_t *pE = wcschr(pS, L')');
                    if (pE) {
                        size_t l = (size_t)(pE - pS);
                        wcsncpy(outFoundPortName, pS, l); outFoundPortName[l]=L'\0';
                        *pIsFound = TRUE; break;
                    }
                }
            }
        }
        i++;
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
}

void com_send(HWND hwndParent) {
    if (g_hPort == INVALID_HANDLE_VALUE) return;
    ZeroMemory(g_wTxtBuf, sizeof(g_wTxtBuf));
    GetWindowTextW(g_hComboTx, g_wTxtBuf, 1023);
    size_t len = wcslen(g_wTxtBuf);
    if (len == 0) return;
    ZeroMemory(g_binBuf, sizeof(g_binBuf));
    DWORD binLen = 0;
    int txMode = get_selected_tx_mode();
    if (txMode == TX_MODE_HEX) {
        size_t i = 0;
        while (i < len && binLen < sizeof(g_binBuf)) {
            if (g_wTxtBuf[i]==L' '||g_wTxtBuf[i]==L','||g_wTxtBuf[i]==L'-'||g_wTxtBuf[i]==L'\r'||g_wTxtBuf[i]==L'\n') { i++; continue; }
            if (i+1 < len) {
                int hv = hex_char_val(g_wTxtBuf[i]);
                int lv = hex_char_val(g_wTxtBuf[i+1]);
                if (hv >= 0 && lv >= 0) {
                    g_binBuf[binLen++] = (BYTE)((hv << 4) | lv);
                    i += 2;
                } else { i++; }
            } else {
                int hv = hex_char_val(g_wTxtBuf[i]);
                if (hv >= 0) { g_binBuf[binLen++] = (BYTE)hv; }
                i++;
            }
        }
    } else {
        UINT cp = get_selected_codepage();
        binLen = parse_text_with_inline_hex(g_wTxtBuf, g_binBuf, sizeof(g_binBuf), cp);
        wchar_t suffixBuf[256] = {0};
        GetWindowTextW(g_hComboSuffix, suffixBuf, 255);
        if (wcslen(suffixBuf) > 0 && binLen < sizeof(g_binBuf)) {
            DWORD suffixLen = parse_text_with_inline_hex(suffixBuf, g_binBuf + binLen, sizeof(g_binBuf) - binLen, cp);
            binLen += suffixLen;
        }
    }
    if (binLen == 0) return;
    DWORD bytesWritten = 0;
    if (WriteFile(g_hPort, g_binBuf, binLen, &bytesWritten, NULL)) {
        add_to_history(g_wTxtBuf);
        if (is_echo_enabled()) {
            wchar_t echoBuf[1280] = {0};
            wcsncpy(echoBuf, g_wTxtBuf, 1023);
            wchar_t suffixBuf[256] = {0};
            GetWindowTextW(g_hComboSuffix, suffixBuf, 255);
            if (wcslen(suffixBuf) > 0 && txMode == TX_MODE_TEXT) wcscat(echoBuf, suffixBuf);
            append_rx_text_colored(echoBuf, RGB(200, 0, 0));
            append_rx_text_colored(L"\r\n", RGB(200, 0, 0));
        }
        SetFocus(g_hComboTx);
    } else {
        MessageBoxW(hwndParent, L"\u041e\u0448\u0438\u0431\u043a\u0430 \u043e\u0442\u043f\u0440\u0430\u0432\u043a\u0438!", L"\u0421\u0431\u043e\u0439", MB_ICONERROR|MB_OK);
    }
}

// ============================================================================
// СКРИПТЫ И МАКРОСЫ
// ============================================================================

static void LoadScriptFile(const wchar_t* path) {
    FILE* f = _wfopen(path, L"r");
    if (!f) return;
    g_scriptCount = 0;
    g_scriptHasStopMarker = FALSE;
    wchar_t line[MAX_LINE_LEN];
    DWORD defaultDelay = 0;
    BOOL firstLine = TRUE;
    while (fgetws(line, MAX_LINE_LEN, f)) {
        size_t len = wcslen(line);
        while (len > 0 && (line[len-1] == L'\n' || line[len-1] == L'\r')) line[--len] = L'\0';
        if (len == 0) continue;
        if (firstLine) {
            wchar_t* endPtr;
            long val = wcstol(line, &endPtr, 10);
            if (*endPtr == L'\0' && val >= 0) { defaultDelay = (DWORD)val; firstLine = FALSE; continue; }
            firstLine = FALSE;
        }
        if (wcsncmp(line, L"#DELAY", 6) == 0) {
            wchar_t* numPart = line + 6;
            while (*numPart == L' ') numPart++;
            defaultDelay = _wtol(numPart);
            continue;
        }
        if (wcscmp(line, L"#STOP") == 0) { g_scriptHasStopMarker = TRUE; continue; }
        if (line[0] == L'#') continue;
        if (g_scriptCount < MAX_SCRIPT_LINES) {
            wcscpy(g_scriptItems[g_scriptCount].command, line);
            g_scriptItems[g_scriptCount].delay = defaultDelay;
            g_scriptCount++;
        }
    }
    fclose(f);
    wchar_t pathBuf[MAX_PATH];
    wcsncpy(pathBuf, path, MAX_PATH-1); pathBuf[MAX_PATH-1] = L'\0';
    SetWindowTextW(g_hEditScriptPath, pathBuf);
    g_scriptCurrentIndex = 0;
}

static void StopScript(void) {
    KillTimer(g_hwndTerminal, TIMER_SCRIPT_ID);
    g_isScriptRunning = FALSE;
    SetWindowTextW(g_hBtnRunScript, L"RUN");
}

static void RunNextScriptCommand(void) {
    if (g_scriptCount == 0 || g_hPort == INVALID_HANDLE_VALUE) return;
    int currentIndex = g_scriptCurrentIndex;
    wchar_t* cmd = g_scriptItems[currentIndex].command;
    DWORD delay = g_scriptItems[currentIndex].delay;
    SetWindowTextW(g_hComboTx, cmd);
    com_send(g_hwndTerminal);
    g_scriptCurrentIndex++;
    if (g_scriptCurrentIndex >= g_scriptCount) {
        if (g_scriptHasStopMarker) { StopScript(); return; }
        else g_scriptCurrentIndex = 0;
    }
    if (delay < 10) delay = 10;
    SetTimer(g_hwndTerminal, TIMER_SCRIPT_ID, delay, NULL);
}

static void LoadMacroBank(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (g_macroBankLoaded[bankIndex]) return;
    
    wchar_t section[32];
    swprintf(section, 32, L"MacroSlots_%d", bankIndex);
    
    wchar_t titleKey[32];
    swprintf(titleKey, 32, L"BankTitle");
    wchar_t titleBuf[MAX_MACRO_TITLE_LEN] = {0};
    ReadIniString(section, titleKey, titleBuf, MAX_MACRO_TITLE_LEN, L"");
    
    for (int i = 0; i < MACROS_PER_BANK; i++) {
        wchar_t keyLabel[32], keyCmd[32];
        swprintf(keyLabel, 32, L"Slot%d_Label", i);
        swprintf(keyCmd, 32, L"Slot%d_Cmd", i);
        
        ReadIniString(section, keyLabel, g_macroBanks[bankIndex][i].label, MACRO_LABEL_LEN, L"");
        ReadIniString(section, keyCmd, g_macroBanks[bankIndex][i].command, MACRO_CMD_LEN, L"");
        
        if (i == 0 && wcslen(titleBuf) > 0) {
            wcscpy(g_macroBanks[bankIndex][i].label, titleBuf);
        }
        
        if (wcslen(g_macroBanks[bankIndex][i].label) == 0 && 
            wcslen(g_macroBanks[bankIndex][i].command) == 0) {
            swprintf(g_macroBanks[bankIndex][i].label, MACRO_LABEL_LEN, L"M%d-%d", bankIndex + 1, i + 1);
        }
    }
    g_macroBankLoaded[bankIndex] = TRUE;
}

static void SaveMacroBank(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    
    wchar_t section[32];
    swprintf(section, 32, L"MacroSlots_%d", bankIndex);
    
    WritePrivateProfileStringW(section, NULL, NULL, g_iniPath);
    
    wchar_t titleKey[32];
    swprintf(titleKey, 32, L"BankTitle");
    WriteIniString(section, titleKey, g_macroBanks[bankIndex][0].label);
    
    for (int i = 0; i < MACROS_PER_BANK; i++) {
        wchar_t keyLabel[32], keyCmd[32];
        swprintf(keyLabel, 32, L"Slot%d_Label", i);
        swprintf(keyCmd, 32, L"Slot%d_Cmd", i);
        
        WriteIniString(section, keyLabel, g_macroBanks[bankIndex][i].label);
        WriteIniString(section, keyCmd, g_macroBanks[bankIndex][i].command);
    }
}

static void SaveMacroPadPosition(int bankIndex, int x, int y) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    
    wchar_t section[32];
    swprintf(section, 32, L"MacroPos_%d", bankIndex);
    
    WriteIniInt(section, L"X", x);
    WriteIniInt(section, L"Y", y);
}

static void LoadMacroPadPosition(int bankIndex, int *px, int *py) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    
    wchar_t section[32];
    swprintf(section, 32, L"MacroPos_%d", bankIndex);
    
    *px = ReadIniInt(section, L"X", -1);
    *py = ReadIniInt(section, L"Y", -1);
}

static void SendMacroCommand(int bankIndex, int slotIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT || 
        slotIndex < 0 || slotIndex >= MACROS_PER_BANK) return;
    if (g_hPort == INVALID_HANDLE_VALUE) {
        MessageBoxW(g_hwndMacroPads[bankIndex], L"\u041f\u043e\u0440\u0442 \u043d\u0435 \u043e\u0442\u043a\u0440\u044b\u0442!", L"\u041e\u0448\u0438\u0431\u043a\u0430", MB_ICONWARNING | MB_OK);
        return;
    }
    
    MacroSlot *slot = &g_macroBanks[bankIndex][slotIndex];
    if (wcslen(slot->command) == 0) return;
    
    BYTE binBuf[1024];
    UINT cp = get_selected_codepage();
    DWORD binLen = parse_text_with_inline_hex(slot->command, binBuf, sizeof(binBuf), cp);
    if (binLen > 0) {
        DWORD written = 0;
        if (WriteFile(g_hPort, binBuf, binLen, &written, NULL)) {
            if (is_echo_enabled() && g_hwndTerminal) {
                wchar_t echoMsg[MACRO_CMD_LEN + 4];
                swprintf(echoMsg, sizeof(echoMsg)/sizeof(wchar_t), L"%ls\r\n", slot->command);
                append_rx_text_colored(echoMsg, RGB(0, 128, 0));
            }
        } else {
            MessageBoxW(g_hwndMacroPads[bankIndex], L"\u041e\u0448\u0438\u0431\u043a\u0430 \u043e\u0442\u043f\u0440\u0430\u0432\u043a\u0438!", L"\u041e\u0448\u0438\u0431\u043a\u0430", MB_ICONERROR | MB_OK);
        }
    }
}

static void UpdateMacroButtons(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (!g_hwndMacroPads[bankIndex]) return;
    
    for (int i = 0; i < MACROS_PER_BANK; i++) {
        HWND hBtn = GetDlgItem(g_hwndMacroPads[bankIndex], IDC_BTN_MACRO_BASE + i);
        if (hBtn) {
            wchar_t displayText[64];
            
            if (g_showCommand && wcslen(g_macroBanks[bankIndex][i].command) > 0) {
                wcsncpy(displayText, g_macroBanks[bankIndex][i].command, 20);
                displayText[20] = L'\0';
                if (wcslen(g_macroBanks[bankIndex][i].command) > 20) wcscat(displayText, L"...");
            } else {
                wcscpy(displayText, g_macroBanks[bankIndex][i].label);
            }
            
            SetWindowTextW(hBtn, displayText);
            InvalidateRect(hBtn, NULL, TRUE);
        }
    }
}

static void UpdateMacroButtonTitle(int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    if (!g_hwndTerminal) return;
    
    HWND hBtn = g_hMacroBankBtns[bankIndex];
    if (!hBtn || !IsWindow(hBtn)) return;
    
    wchar_t title[MAX_MACRO_TITLE_LEN] = {0};
    wcsncpy(title, g_macroBanks[bankIndex][0].label, MAX_MACRO_TITLE_LEN - 1);
    
    wchar_t defaultPattern[16];
    swprintf(defaultPattern, 16, L"M%d-%d", bankIndex + 1, 1);
    if (wcslen(title) == 0 || wcsstr(title, L"M") == title) {
        wchar_t defaultTitle[8];
        swprintf(defaultTitle, 8, L"M%d", bankIndex + 1);
        SetWindowTextW(hBtn, defaultTitle);
    } else {
        if (wcslen(title) > 10) {
            title[10] = L'\0';
            wcscat(title, L"...");
        }
        SetWindowTextW(hBtn, title);
    }
}

static void SaveAllMacroWindowsState(void) {
    int count = 0;
    int banksOpen[MACRO_BANK_COUNT] = {0};
    
    for (int i = 0; i < MACRO_BANK_COUNT; i++) {
        if (g_hwndMacroPads[i] && IsWindow(g_hwndMacroPads[i])) {
            banksOpen[i] = 1;
            count++;
        }
    }
    
    WriteIniInt(L"MacroWindows", L"Count", count);
    for (int i = 0; i < MACRO_BANK_COUNT; i++) {
        wchar_t key[16];
        swprintf(key, 16, L"Bank%d", i);
        WriteIniInt(L"MacroWindows", key, banksOpen[i]);
    }
}

// ============================================================================
// ОКНА МАКРОСОВ (ПАНЕЛЬ И РЕДАКТИРОВАНИЕ)
// ============================================================================

static void ApplyThemeToWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;
    
    HBRUSH hBrush = CreateSolidBrush(g_pTheme->bgColor);
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBrush);
    
    EnumChildWindows(hwnd, ApplyThemeCallback, (LPARAM)hwnd);
    
    RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

BOOL CALLBACK ApplyThemeCallback(HWND hWndChild, LPARAM lParam) {
    wchar_t cn[64] = {0};
    GetClassNameW(hWndChild, cn, 64);
    
    if (wcscmp(cn, L"BUTTON") == 0) {
        HBRUSH hBtnBrush = CreateSolidBrush(g_pTheme->btnBg);
        SetClassLongPtr(hWndChild, GCLP_HBRBACKGROUND, (LONG_PTR)hBtnBrush);
        InvalidateRect(hWndChild, NULL, TRUE);
    } else if (wcscmp(cn, L"EDIT") == 0 || wcscmp(cn, L"RichEdit20W") == 0) {
        HBRUSH hEditBrush = CreateSolidBrush(g_pTheme->editBg);
        SetClassLongPtr(hWndChild, GCLP_HBRBACKGROUND, (LONG_PTR)hEditBrush);
        InvalidateRect(hWndChild, NULL, TRUE);
    } else if (wcscmp(cn, L"COMBOBOX") == 0) {
        InvalidateRect(hWndChild, NULL, TRUE);
    } else if (wcscmp(cn, L"STATIC") == 0) {
        InvalidateRect(hWndChild, NULL, TRUE);
    }
    return TRUE;
}

static void SetTheme(int theme) {
    if (theme < 0 || theme > THEME_DARK) theme = THEME_LIGHT;
    g_currentTheme = theme;
    g_pTheme = &g_themes[theme];
    
    if (g_hwndTerminal && IsWindow(g_hwndTerminal)) {
        ApplyThemeToWindow(g_hwndTerminal);
        if (g_hBtnTheme) {
            SetWindowTextW(g_hBtnTheme, L"Theme");
        }
    }
    
    if (g_hwndSettings && IsWindow(g_hwndSettings)) {
        ApplyThemeToWindow(g_hwndSettings);
    }
    
    for (int i = 0; i < MACRO_BANK_COUNT; i++) {
        if (g_hwndMacroPads[i] && IsWindow(g_hwndMacroPads[i])) {
            ApplyThemeToWindow(g_hwndMacroPads[i]);
        }
    }
    
    if (g_hwndMacroEdit && IsWindow(g_hwndMacroEdit)) {
        ApplyThemeToWindow(g_hwndMacroEdit);
    }
    
    WriteIniInt(L"Terminal", L"Theme", theme);
}

LRESULT CALLBACK MacroEditWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static HWND hEditName = NULL;
    static HWND hEditCmd = NULL;
    static HWND hBtnSave = NULL;
    static HWND hBtnCancel = NULL;
    static int s_bankIndex = -1;
    static int s_slotIndex = -1;
    
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW *cs = (CREATESTRUCTW*)lp;
            int *params = (int*)cs->lpCreateParams;
            s_bankIndex = params[0];
            s_slotIndex = params[1];
            
            CreateWindowExW(0, L"STATIC", L"Name:", 
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 14, 45, 20, hwnd, NULL, NULL, NULL);
            
            hEditName = CreateWindowExW(0, L"EDIT", 
                g_macroBanks[s_bankIndex][s_slotIndex].label,
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                60, 12, 220, 22, hwnd, (HMENU)IDC_EDIT_MACRO_NAME, NULL, NULL);
            SendMessage(hEditName, WM_SETFONT, (WPARAM)g_hEditFont, TRUE);
            
            CreateWindowExW(0, L"STATIC", L"CMD:", 
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 46, 40, 20, hwnd, NULL, NULL, NULL);
            
            hEditCmd = CreateWindowExW(0, L"EDIT", 
                g_macroBanks[s_bankIndex][s_slotIndex].command,
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                65, 44, 215, 22, hwnd, (HMENU)IDC_EDIT_MACRO_CMD, NULL, NULL);
            SendMessage(hEditCmd, WM_SETFONT, (WPARAM)g_hEditFont, TRUE);
            
            hBtnSave = CreateWindowExW(0, L"BUTTON", L"Save",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                110, 85, 70, 25, hwnd, (HMENU)IDC_BTN_MACRO_SAVE, NULL, NULL);
            SendMessage(hBtnSave, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                190, 85, 70, 25, hwnd, (HMENU)IDC_BTN_MACRO_CANCEL, NULL, NULL);
            SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            ApplyThemeToWindow(hwnd);
            SetFocus(hEditName);
            SendMessage(hEditName, EM_SETSEL, 0, -1);
            return 0;
        }
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wp;
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(g_pTheme->bgColor);
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            return 1;
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->btnBg);
            SetTextColor(hdc, g_pTheme->btnFg);
            return (LRESULT)CreateSolidBrush(g_pTheme->btnBg);
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->editBg);
            SetTextColor(hdc, g_pTheme->editFg);
            return (LRESULT)CreateSolidBrush(g_pTheme->editBg);
        }
        case WM_COMMAND: {
            int id = LOWORD(wp);
            
            if (id == IDC_BTN_MACRO_SAVE) {
                SaveMacroFromEdit();
                CloseMacroEdit();
                return 0;
            }
            if (id == IDC_BTN_MACRO_CANCEL) {
                CloseMacroEdit();
                return 0;
            }
            return 0;
        }
        case WM_KEYDOWN: {
            if (wp == VK_ESCAPE) {
                CloseMacroEdit();
                return 0;
            }
            if (wp == VK_RETURN) {
                HWND hFocus = GetFocus();
                if (hFocus == hEditName || hFocus == hEditCmd) {
                    SaveMacroFromEdit();
                    CloseMacroEdit();
                    return 0;
                }
            }
            break;
        }
        case WM_CLOSE: {
            CloseMacroEdit();
            return 0;
        }
        case WM_DESTROY: {
            g_hwndMacroEdit = NULL;
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void CloseMacroEdit(void) {
    if (g_hwndMacroEdit && IsWindow(g_hwndMacroEdit)) {
        DestroyWindow(g_hwndMacroEdit);
    }
    g_hwndMacroEdit = NULL;
    g_editingSlot = -1;
    g_editingBank = -1;
}

static void SaveMacroFromEdit(void) {
    if (g_editingBank < 0 || g_editingBank >= MACRO_BANK_COUNT ||
        g_editingSlot < 0 || g_editingSlot >= MACROS_PER_BANK) return;
    if (!g_hwndMacroEdit) return;
    
    HWND hEditName = GetDlgItem(g_hwndMacroEdit, IDC_EDIT_MACRO_NAME);
    HWND hEditCmd = GetDlgItem(g_hwndMacroEdit, IDC_EDIT_MACRO_CMD);
    
    if (hEditName) GetWindowTextW(hEditName, g_macroBanks[g_editingBank][g_editingSlot].label, MACRO_LABEL_LEN);
    if (hEditCmd) GetWindowTextW(hEditCmd, g_macroBanks[g_editingBank][g_editingSlot].command, MACRO_CMD_LEN);
    
    SaveMacroBank(g_editingBank);
    UpdateMacroButtons(g_editingBank);
}

static void ShowMacroEditWindow(HWND hParent, int bankIndex, int slotIndex) {
    if (!hParent) return;
    
    if (g_hwndMacroEdit) {
        CloseMacroEdit();
    }
    
    g_editingBank = bankIndex;
    g_editingSlot = slotIndex;
    
    static BOOL classRegistered = FALSE;
    if (!classRegistered) {
        WNDCLASSW wc = {0};
        wc.lpfnWndProc = MacroEditWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"WT232_MacroEdit_Class";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassW(&wc);
        classRegistered = TRUE;
    }
    
    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    
    int params[2] = { bankIndex, slotIndex };
    
    int x = rcParent.left + 10;
    int y = rcParent.bottom + 5;
    int w = 300;
    int h = 150;
    
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    if (x + w > screenW) x = screenW - w - 10;
    if (y + h > screenH) y = screenH - h - 10;
    if (y < 0) y = 0;
    
    g_hwndMacroEdit = CreateWindowExW(
        WS_EX_TOOLWINDOW, 
        L"WT232_MacroEdit_Class", 
        L"Edit Macro",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX | WS_CLIPCHILDREN,
        x, y, w, h,
        hParent, NULL, GetModuleHandle(NULL), (LPVOID)params);
    
    if (g_hwndMacroEdit) {
        ShowWindow(g_hwndMacroEdit, SW_SHOW);
        UpdateWindow(g_hwndMacroEdit);
        SetForegroundWindow(g_hwndMacroEdit);
    }
}

static void LayoutButtons(HWND hwnd) {
    if (!hwnd) return;
    
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    int w = rc.right;
    int h = rc.bottom;
    
    int margin = 6;
    int topBarH = 45;
    
    HWND hTitleEdit = GetDlgItem(hwnd, IDC_EDIT_MACRO_TITLE);
    HWND hModeBtn = GetDlgItem(hwnd, IDC_BTN_MACRO_MODE);
    HWND hDispBtn = GetDlgItem(hwnd, IDC_BTN_MACRO_DISPLAY);

    if (hModeBtn) MoveWindow(hModeBtn, margin, margin, 55, 25, TRUE);
    if (hDispBtn) MoveWindow(hDispBtn, margin + 60, margin, 55, 25, TRUE);
    if (hTitleEdit) MoveWindow(hTitleEdit, 130, margin, 120, 24, TRUE);
    
    int topOffset = margin + topBarH + 4;
    int bottomOffset = margin;
    int availH = h - topOffset - bottomOffset;
    int availW = w - margin * 2;
    
    int btnW = (availW - (MACRO_COLS - 1) * 4) / MACRO_COLS;
    int btnH = (availH - (MACRO_ROWS - 1) * 4) / MACRO_ROWS;
    
    if (btnW < 40) btnW = 40;
    if (btnH < 20) btnH = 20;
    
    for (int i = 0; i < MACROS_PER_BANK; i++) {
        int col = i % MACRO_COLS;
        int row = i / MACRO_COLS;
        int x = margin + col * (btnW + 4);
        int y = topOffset + row * (btnH + 4);
        
        HWND hBtn = GetDlgItem(hwnd, IDC_BTN_MACRO_BASE + i);
        if (hBtn) {
            MoveWindow(hBtn, x, y, btnW, btnH, TRUE);
        }
    }
    
    RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

LRESULT CALLBACK MacroPadWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static int s_bankIndex = -1;
    static HWND hModeBtn = NULL;
    static HWND hDispBtn = NULL;
    static HWND hTitleEdit = NULL;
    
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW *cs = (CREATESTRUCTW*)lp;
            s_bankIndex = (int)(LONG_PTR)cs->lpCreateParams;
            
            LoadMacroBank(s_bankIndex);            
        
            hTitleEdit = CreateWindowExW(0, L"EDIT", 
                g_macroBanks[s_bankIndex][0].label,
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                130, 10, 120, 24, hwnd, (HMENU)IDC_EDIT_MACRO_TITLE, NULL, NULL);

            HFONT hTitleFont = CreateTitleFont(-15);
            SendMessage(hTitleEdit, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

            if (wcslen(g_macroBanks[s_bankIndex][0].label) == 0) {
                SetWindowTextW(hTitleEdit, L"");
            }
            
            hModeBtn = CreateWindowExW(0, L"BUTTON", L"RUN",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                0, 0, 55, 25,
                hwnd, (HMENU)IDC_BTN_MACRO_MODE, NULL, NULL);
            SendMessage(hModeBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            hDispBtn = CreateWindowExW(0, L"BUTTON", L"LABEL",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                60, 0, 55, 25,
                hwnd, (HMENU)IDC_BTN_MACRO_DISPLAY, NULL, NULL);
            SendMessage(hDispBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            for (int i = 0; i < MACROS_PER_BANK; i++) {
                wchar_t displayText[64];
                
                if (g_showCommand && wcslen(g_macroBanks[s_bankIndex][i].command) > 0) {
                    wcsncpy(displayText, g_macroBanks[s_bankIndex][i].command, 20);
                    displayText[20] = L'\0';
                    if (wcslen(g_macroBanks[s_bankIndex][i].command) > 20) wcscat(displayText, L"...");
                } else {
                    wcscpy(displayText, g_macroBanks[s_bankIndex][i].label);
                }
                
                HWND hBtn = CreateWindowExW(
                    0, L"BUTTON", displayText,
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                    0, 0, 10, 10,
                    hwnd, (HMENU)(LONG_PTR)(IDC_BTN_MACRO_BASE + i), NULL, NULL);
                if (hBtn) SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            }
            
            ApplyThemeToWindow(hwnd);
            LayoutButtons(hwnd);
            return 0;
        }
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wp;
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(g_pTheme->bgColor);
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            return 1;
        }
        case WM_MOVING: {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            if (s_bankIndex >= 0) {
                SaveMacroPadPosition(s_bankIndex, rc.left, rc.top);
            }
            return DefWindowProcW(hwnd, msg, wp, lp);
        }
        case WM_SIZE: {
            LayoutButtons(hwnd);
            return 0;
        }
        case WM_GETMINMAXINFO: {
            MINMAXINFO *pMMI = (MINMAXINFO*)lp;
            pMMI->ptMinTrackSize.x = 220;
            pMMI->ptMinTrackSize.y = 250;
            return 0;
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wp;
            int id = GetDlgCtrlID((HWND)lp);
            
            SetBkColor(hdc, g_pTheme->btnBg);
            SetTextColor(hdc, g_pTheme->btnFg);
            
            if (id == IDC_BTN_MACRO_MODE) {
                if (g_editMode) {
                    SetBkColor(hdc, RGB(200, 80, 80));
                } else {
                    SetBkColor(hdc, RGB(80, 200, 80));
                }
            }
            if (id == IDC_BTN_MACRO_DISPLAY) {
                if (g_showCommand) {
                    SetBkColor(hdc, RGB(80, 80, 200));
                } else {
                    SetBkColor(hdc, RGB(80, 200, 80));
                }
            }
            return (LRESULT)CreateSolidBrush(g_pTheme->btnBg);
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->editBg);
            SetTextColor(hdc, g_pTheme->editFg);
            return (LRESULT)CreateSolidBrush(g_pTheme->editBg);
        }
        case WM_COMMAND: {
            int id = LOWORD(wp);
            
            if (id == IDC_EDIT_MACRO_TITLE) {
                if (HIWORD(wp) == EN_CHANGE) {
                    HWND hEdit = GetDlgItem(hwnd, IDC_EDIT_MACRO_TITLE);
                    if (hEdit) {
                        GetWindowTextW(hEdit, g_macroBanks[s_bankIndex][0].label, MACRO_LABEL_LEN);
                        SaveMacroBank(s_bankIndex);
                        UpdateMacroButtonTitle(s_bankIndex);
                    }
                }
                return 0;
            }
            
            if (id == IDC_BTN_MACRO_MODE) {
                g_editMode = !g_editMode;
                SetWindowTextW(hModeBtn, g_editMode ? L"EDIT" : L"RUN");
                InvalidateRect(hModeBtn, NULL, TRUE);
                if (!g_editMode && g_hwndMacroEdit) {
                    CloseMacroEdit();
                }
                return 0;
            }
            
            if (id == IDC_BTN_MACRO_DISPLAY) {
                g_showCommand = !g_showCommand;
                SetWindowTextW(hDispBtn, g_showCommand ? L"CMD" : L"LABEL");
                InvalidateRect(hDispBtn, NULL, TRUE);
                UpdateMacroButtons(s_bankIndex);
                return 0;
            }
            
            if (id >= IDC_BTN_MACRO_BASE && id < IDC_BTN_MACRO_BASE + MACROS_PER_BANK) {
                int slotIdx = id - IDC_BTN_MACRO_BASE;
                if (g_editMode) {
                    ShowMacroEditWindow(hwnd, s_bankIndex, slotIdx);
                } else {
                    SendMacroCommand(s_bankIndex, slotIdx);
                }
                return 0;
            }
            return 0;
        }
        case WM_CLOSE: {
            if (g_hwndMacroEdit) CloseMacroEdit();
            DestroyWindow(hwnd);
            if (s_bankIndex >= 0 && s_bankIndex < MACRO_BANK_COUNT) {
                g_hwndMacroPads[s_bankIndex] = NULL;
            }
            return 0;
        }
        case WM_DESTROY: {
            HWND hTitleEdit = GetDlgItem(hwnd, IDC_EDIT_MACRO_TITLE);
            if (hTitleEdit) {
                HFONT hFont = (HFONT)SendMessage(hTitleEdit, WM_GETFONT, 0, 0);
                if (hFont && hFont != GetStockObject(DEFAULT_GUI_FONT)) {
                    DeleteObject(hFont);
                }
            }
            if (g_hwndMacroEdit) CloseMacroEdit();
            if (s_bankIndex >= 0 && s_bankIndex < MACRO_BANK_COUNT) {
                g_hwndMacroPads[s_bankIndex] = NULL;
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void ShowMacroPad(HWND hParent, int bankIndex) {
    if (bankIndex < 0 || bankIndex >= MACRO_BANK_COUNT) return;
    
    if (g_hwndMacroPads[bankIndex] && IsWindow(g_hwndMacroPads[bankIndex])) {
        ShowWindow(g_hwndMacroPads[bankIndex], SW_SHOW);
        SetForegroundWindow(g_hwndMacroPads[bankIndex]);
        return;
    }
    
    static BOOL classRegistered = FALSE;
    if (!classRegistered) {
        WNDCLASSW wc = {0};
        wc.lpfnWndProc = MacroPadWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"WT232_MacroPad_Class";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassW(&wc);
        classRegistered = TRUE;
    }
    
    int w = MACRO_COLS * 84 + 24;
    int h = MACRO_ROWS * 34 + 40 + 35;
    
    int x, y;
    LoadMacroPadPosition(bankIndex, &x, &y);
    
    if (x == -1 || y == -1) {
        RECT rcParent;
        GetWindowRect(hParent, &rcParent);
        x = rcParent.left + 20;
        y = rcParent.top + 50;
    }
    
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    if (x + w > screenW) x = screenW - w - 10;
    if (y + h > screenH) y = screenH - h - 10;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    
    wchar_t title[64];
    swprintf(title, 64, L"Macros M%d (RUN/EDIT | LABEL/CMD)", bankIndex + 1);
    
    g_hwndMacroPads[bankIndex] = CreateWindowExW(
        WS_EX_TOOLWINDOW, 
        L"WT232_MacroPad_Class",
        title, 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_THICKFRAME | WS_CLIPCHILDREN,
        x, y, w, h,
        hParent, NULL, GetModuleHandle(NULL), 
        (LPVOID)(LONG_PTR)bankIndex);
    
    if (g_hwndMacroPads[bankIndex]) {
        ShowWindow(g_hwndMacroPads[bankIndex], SW_SHOW);
        UpdateWindow(g_hwndMacroPads[bankIndex]);
        ApplyThemeToWindow(g_hwndMacroPads[bankIndex]);
    }
}

// ============================================================================
// РАБОТА С INI ФАЙЛОМ
// ============================================================================

static void InitIniPaths(void) {
    GetModuleFileNameW(NULL, g_iniPath, MAX_PATH);
    
    wchar_t *pDot = wcsrchr(g_iniPath, L'.');
    if (pDot) *pDot = L'\0';
    
    wcscat(g_iniPath, L".ini");
    
    wcscpy(g_iniBackupPath, g_iniPath);
    wcscat(g_iniBackupPath, L".bak");
}

static BOOL ReadIniString(const wchar_t *section, const wchar_t *key, 
                           wchar_t *out, int maxLen, const wchar_t *defVal) {
    DWORD res = GetPrivateProfileStringW(section, key, defVal, out, maxLen, g_iniPath);
    return (res > 0) ? TRUE : FALSE;
}

static int ReadIniInt(const wchar_t *section, const wchar_t *key, int defVal) {
    return GetPrivateProfileIntW(section, key, defVal, g_iniPath);
}

static void WriteIniString(const wchar_t *section, const wchar_t *key, const wchar_t *val) {
    WritePrivateProfileStringW(section, key, val, g_iniPath);
}

static void WriteIniInt(const wchar_t *section, const wchar_t *key, int val) {
    wchar_t buf[32];
    swprintf(buf, 32, L"%d", val);
    WritePrivateProfileStringW(section, key, buf, g_iniPath);
}

static BOOL ValidateIni(void) {
    int baudrate = ReadIniInt(L"Port", L"LastBaudrate", -1);
    if (baudrate == -1) return FALSE;
    
    int width = ReadIniInt(L"Terminal", L"Width", -1);
    if (width == -1) return FALSE;
    
    return TRUE;
}

static BOOL ReadAllIni(void) {
    if (!ValidateIni()) {
        if (ReadIniString(L"Port", L"LastPortName", g_wTxtBuf, 10, L"") > 0) {
            if (ValidateIni()) {
                CopyFileW(g_iniBackupPath, g_iniPath, FALSE);
                MessageBoxW(NULL, 
                    L"\u0412\u043e\u0441\u0441\u0442\u0430\u043d\u043e\u0432\u043b\u0435\u043d\u044b \u043d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0438 \u0438\u0437 \u0440\u0435\u0437\u0435\u0440\u0432\u043d\u043e\u0439 \u043a\u043e\u043f\u0438\u0438.",
                    L"\u0412\u043e\u0441\u0441\u0442\u0430\u043d\u043e\u0432\u043b\u0435\u043d\u0438\u0435", MB_OK);
                return TRUE;
            }
        }
        CreateDefaultIni();
        return FALSE;
    }
    return TRUE;
}

static void CreateDefaultIni(void) {
    WriteIniString(L"Port", L"LastPortName", L"COM1");
    WriteIniInt(L"Port", L"LastBaudrate", 115200);
    WriteIniInt(L"Port", L"LastDataBits", 8);
    WriteIniInt(L"Port", L"LastParity", 0);
    WriteIniInt(L"Port", L"LastStopBits", 0);
    WriteIniInt(L"Port", L"LastFlow", 2);
    WriteIniString(L"Port", L"LastVID", L"");
    WriteIniString(L"Port", L"LastPID", L"");
    WriteIniString(L"Port", L"LastSerial", L"");
    
    WriteIniInt(L"Terminal", L"X", 100);
    WriteIniInt(L"Terminal", L"Y", 100);
    WriteIniInt(L"Terminal", L"Width", 700);
    WriteIniInt(L"Terminal", L"Height", 480);
    WriteIniInt(L"Terminal", L"State", 1);
    WriteIniInt(L"Terminal", L"TxMode", 1);
    WriteIniInt(L"Terminal", L"RxMode", 0);
    WriteIniInt(L"Terminal", L"Echo", 1);
    WriteIniInt(L"Terminal", L"Encoding", 2);
    WriteIniInt(L"Terminal", L"RepeatDelay", 1000);
    WriteIniInt(L"Terminal", L"FontSize", 2);
    WriteIniInt(L"Terminal", L"Theme", 0);
    
    WriteIniInt(L"History", L"Count", 0);
    
    WriteIniInt(L"MacroWindows", L"Count", 0);
    for (int i = 0; i < MACRO_BANK_COUNT; i++) {
        wchar_t key[16];
        swprintf(key, 16, L"Bank%d", i);
        WriteIniInt(L"MacroWindows", key, 0);
    }
    
    for (int bank = 0; bank < MACRO_BANK_COUNT; bank++) {
        wchar_t section[32];
        swprintf(section, 32, L"MacroSlots_%d", bank);
        for (int i = 0; i < MACROS_PER_BANK; i++) {
            wchar_t keyLabel[32], keyCmd[32];
            swprintf(keyLabel, 32, L"Slot%d_Label", i);
            swprintf(keyCmd, 32, L"Slot%d_Cmd", i);
            WriteIniString(section, keyLabel, L"");
            WriteIniString(section, keyCmd, L"");
        }
    }
    
    for (int bank = 0; bank < MACRO_BANK_COUNT; bank++) {
        wchar_t section[32];
        swprintf(section, 32, L"MacroPos_%d", bank);
        WriteIniInt(section, L"X", -1);
        WriteIniInt(section, L"Y", -1);
    }
}

static void WriteAllIni(void) {
    CopyFileW(g_iniPath, g_iniBackupPath, FALSE);
    
    if (g_hComboPort) {
        wchar_t portName[MAX_PORT_NAME] = {0};
        int pi = (int)SendMessageW(g_hComboPort, CB_GETCURSEL, 0, 0);
        if (pi != CB_ERR) {
            SendMessageW(g_hComboPort, CB_GETLBTEXT, pi, (LPARAM)portName);
            WriteIniString(L"Port", L"LastPortName", portName);
        }
    }
    
    if (g_hComboBaud) {
        wchar_t baudStr[16] = {0};
        GetWindowTextW(g_hComboBaud, baudStr, 16);
        WriteIniInt(L"Port", L"LastBaudrate", _wtoi(baudStr));
    }
    
    if (g_hComboDataBits) {
        WriteIniInt(L"Port", L"LastDataBits", (int)SendMessageW(g_hComboDataBits, CB_GETCURSEL, 0, 0) + 5);
    }
    
    if (g_hComboParity) {
        WriteIniInt(L"Port", L"LastParity", (int)SendMessageW(g_hComboParity, CB_GETCURSEL, 0, 0));
    }
    
    if (g_hComboStopBits) {
        WriteIniInt(L"Port", L"LastStopBits", (int)SendMessageW(g_hComboStopBits, CB_GETCURSEL, 0, 0));
    }
    
    if (g_hComboFlow) {
        WriteIniInt(L"Port", L"LastFlow", (int)SendMessageW(g_hComboFlow, CB_GETCURSEL, 0, 0));
    }
    
    WriteIniString(L"Port", L"LastVID", g_session.targetDevice.vid);
    WriteIniString(L"Port", L"LastPID", g_session.targetDevice.pid);
    WriteIniString(L"Port", L"LastSerial", g_session.targetDevice.serial);
}

// ============================================================================
// WINMAIN И ОКНО НАСТРОЕК
// ============================================================================

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);
    
    InitIniPaths();
    ReadAllIni();
    
    g_currentTheme = ReadIniInt(L"Terminal", L"Theme", THEME_LIGHT);
    g_pTheme = &g_themes[g_currentTheme];
    
    g_hMonoFont = CreateMonoFont(-14);
    g_hBtnFont = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    g_hEditFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
    g_hTitleFont = CreateTitleFont(-15);

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"WT232_Settings_Class";
    if (!RegisterClassW(&wc)) return 0;

    WNDCLASSW tc = {0};
    tc.lpfnWndProc = TerminalWndProc;
    tc.hInstance = hInst;
    tc.hCursor = LoadCursor(NULL, IDC_ARROW);
    tc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    tc.lpszClassName = L"WT232_Terminal_Class";
    RegisterClassW(&tc);

    WNDCLASSW ic = {0};
    ic.lpfnWndProc = InfoWndProc;
    ic.hInstance = hInst;
    ic.hCursor = LoadCursor(NULL, IDC_ARROW);
    ic.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    ic.lpszClassName = L"WT232_Info_Scroll_Class";
    RegisterClassW(&ic);

    wchar_t settingsTitle[128];
    swprintf(settingsTitle, 128, L"WT232 Settings " APP_VERSION);
    g_hwndSettings = CreateWindowExW(0, L"WT232_Settings_Class", settingsTitle,
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 330, NULL, NULL, hInst, NULL);
    if (!g_hwndSettings) return 0;
    
    ApplyThemeToWindow(g_hwndSettings);
    ShowWindow(g_hwndSettings, show);
    UpdateWindow(g_hwndSettings);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    if (g_hMonoFont) DeleteObject(g_hMonoFont);
    if (g_hBtnFont) DeleteObject(g_hBtnFont);
    if (g_hEditFont) DeleteObject(g_hEditFont);
    if (g_hTitleFont) DeleteObject(g_hTitleFont);
    return 0;
}

BOOL CALLBACK SetFontCallback(HWND hWndChild, LPARAM lParam) {
    wchar_t cn[32] = {0};
    GetClassNameW(hWndChild, cn, 32);
    if (wcscmp(cn, L"STATIC") == 0 || wcscmp(cn, L"BUTTON") == 0 || 
        wcscmp(cn, L"COMBOBOX") == 0 || wcscmp(cn, L"EDIT") == 0) {
        SendMessage(hWndChild, WM_SETFONT, (WPARAM)lParam, TRUE);
    }
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wp;
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(g_pTheme->bgColor);
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            return 1;
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->btnBg);
            SetTextColor(hdc, g_pTheme->btnFg);
            return (LRESULT)CreateSolidBrush(g_pTheme->btnBg);
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->editBg);
            SetTextColor(hdc, g_pTheme->editFg);
            return (LRESULT)CreateSolidBrush(g_pTheme->editBg);
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->bgColor);
            SetTextColor(hdc, g_pTheme->fgColor);
            return (LRESULT)CreateSolidBrush(g_pTheme->bgColor);
        }
        case WM_CREATE: {
            HFONT hFont = GetStockObject(DEFAULT_GUI_FONT);
            
            CreateWindowExW(0, L"STATIC", L"\u041f\u043e\u0440\u0442:", WS_CHILD|WS_VISIBLE, 20, 18, 50, 20, hwnd, NULL, NULL, NULL);
            g_hBtnInfo = CreateWindowExW(0, L"BUTTON", L"?", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 70, 15, 22, 22, hwnd, (HMENU)IDC_BTN_INFO, NULL, NULL);
            g_hComboPort = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 110, 15, 150, 200, hwnd, (HMENU)IDC_COMBO_PORT, NULL, NULL);
            
            CreateWindowExW(0, L"STATIC", L"\u0421\u043a\u043e\u0440\u043e\u0441\u0442\u044c:", WS_CHILD|WS_VISIBLE, 20, 50, 80, 20, hwnd, NULL, NULL, NULL);
            g_hComboBaud = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL, 110, 47, 150, 200, hwnd, (HMENU)IDC_COMBO_BAUD, NULL, NULL);
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"2400");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"4800");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"9600");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"19200");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"38400");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"57600");
            SendMessageW(g_hComboBaud, CB_ADDSTRING, 0, (LPARAM)L"115200");
            
            CreateWindowExW(0, L"STATIC", L"\u0411\u0438\u0442\u044b:", WS_CHILD|WS_VISIBLE, 20, 82, 80, 20, hwnd, NULL, NULL, NULL);
            g_hComboDataBits = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 110, 79, 150, 200, hwnd, (HMENU)IDC_COMBO_DATABITS, NULL, NULL);
            SendMessageW(g_hComboDataBits, CB_ADDSTRING, 0, (LPARAM)L"5");
            SendMessageW(g_hComboDataBits, CB_ADDSTRING, 0, (LPARAM)L"6");
            SendMessageW(g_hComboDataBits, CB_ADDSTRING, 0, (LPARAM)L"7");
            SendMessageW(g_hComboDataBits, CB_ADDSTRING, 0, (LPARAM)L"8");
            
            CreateWindowExW(0, L"STATIC", L"\u0427\u0451\u0442\u043d\u043e\u0441\u0442\u044c:", WS_CHILD|WS_VISIBLE, 20, 114, 80, 20, hwnd, NULL, NULL, NULL);
            g_hComboParity = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 110, 111, 150, 200, hwnd, (HMENU)IDC_COMBO_PARITY, NULL, NULL);
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"\u041d\u0435\u0442 / None");
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"\u0427\u0451\u0442 / Even");
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"\u041d\u0435\u0447\u0451\u0442 / Odd");
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"\u041c\u0430\u0440\u043a\u0435\u0440 / Mark");
            SendMessageW(g_hComboParity, CB_ADDSTRING, 0, (LPARAM)L"\u041f\u0440\u043e\u0431\u0435\u043b / Space");
            
            CreateWindowExW(0, L"STATIC", L"\u0421\u0442\u043e\u043f-\u0431\u0438\u0442\u044b:", WS_CHILD|WS_VISIBLE, 20, 146, 80, 20, hwnd, NULL, NULL, NULL);
            g_hComboStopBits = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 110, 143, 150, 200, hwnd, (HMENU)IDC_COMBO_STOPBITS, NULL, NULL);
            SendMessageW(g_hComboStopBits, CB_ADDSTRING, 0, (LPARAM)L"1");
            SendMessageW(g_hComboStopBits, CB_ADDSTRING, 0, (LPARAM)L"1.5");
            SendMessageW(g_hComboStopBits, CB_ADDSTRING, 0, (LPARAM)L"2");
            
            CreateWindowExW(0, L"STATIC", L"\u0423\u043f\u0440. \u043f\u043e\u0442\u043e\u043a\u043e\u043c:", WS_CHILD|WS_VISIBLE, 20, 178, 80, 20, hwnd, NULL, NULL, NULL);
            g_hComboFlow = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 110, 175, 150, 200, hwnd, (HMENU)IDC_COMBO_FLOW, NULL, NULL);
            SendMessageW(g_hComboFlow, CB_ADDSTRING, 0, (LPARAM)L"\u041d\u0435\u0442 / None");
            SendMessageW(g_hComboFlow, CB_ADDSTRING, 0, (LPARAM)L"XON/XOFF");
            SendMessageW(g_hComboFlow, CB_ADDSTRING, 0, (LPARAM)L"RTS/CTS");
            SendMessageW(g_hComboFlow, CB_ADDSTRING, 0, (LPARAM)L"RS-485 (RTS Toggle)");
            
            CreateWindowExW(0, L"BUTTON", L"OK", 
                WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON, 60, 215, 70, 35, hwnd, (HMENU)IDC_BTN_OK, NULL, NULL);
            
            CreateWindowExW(0, L"BUTTON", L"\u041e\u0442\u043c\u0435\u043d\u0430", 
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 160, 215, 70, 35, hwnd, (HMENU)IDC_BTN_CANCEL, NULL, NULL);
            
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
        case WM_KEYDOWN: {
            if (wp == VK_F1) { show_about_dialog(hwnd); return 0; }
            break;
        }
        case WM_COMMAND: {
            switch (LOWORD(wp)) {
                case IDC_BTN_INFO: init_com_ports(hwnd, TRUE, NULL); break;
                
                case IDC_BTN_OK: {
                    WriteAllIni();
                    
                    wchar_t selPort[MAX_PORT_NAME]={0};
                    int pi = (int)SendMessageW(g_hComboPort, CB_GETCURSEL, 0, 0);
                    if (pi != CB_ERR) SendMessageW(g_hComboPort, CB_GETLBTEXT, pi, (LPARAM)selPort);
                    wchar_t szBaud[16]={0}; GetWindowTextW(g_hComboBaud, szBaud, 16);
                    int baud = _wtoi(szBaud);
                    
                    if (wcslen(selPort)==0) { 
                        MessageBoxW(hwnd, L"\u041f\u043e\u0440\u0442 \u043d\u0435 \u0432\u044b\u0431\u0440\u0430\u043d!", L"\u0412\u043d\u0438\u043c\u0430\u043d\u0438\u0435", MB_ICONWARNING|MB_OK); 
                        break; 
                    }
                    
                    init_com_ports(hwnd, FALSE, selPort);
                    if (com_open(selPort, baud)) {
                        g_session.lastBaudrate = baud;
                        wcscpy(g_session.lastPortName, selPort);
                        g_session.isWaitingReconnect = FALSE;
                        update_terminal_title(selPort, baud);
                        
                        ShowWindow(hwnd, SW_HIDE);
                        int winW = ReadIniInt(L"Terminal", L"Width", 700);
                        int winH = ReadIniInt(L"Terminal", L"Height", 480);
                        int winX = ReadIniInt(L"Terminal", L"X", CW_USEDEFAULT);
                        int winY = ReadIniInt(L"Terminal", L"Y", CW_USEDEFAULT);
                        
                        g_hwndTerminal = CreateWindowExW(0, L"WT232_Terminal_Class", g_szTitle,
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, winX, winY, winW, winH,
                            NULL, NULL, GetModuleHandle(NULL), NULL);
                        if (g_hwndTerminal) {
                            SetTimer(g_hwndTerminal, TIMER_RECONNECT_ID, 500, NULL);
                            SetTimer(g_hwndTerminal, TIMER_READ_ID, 50, NULL);
                            ApplyThemeToWindow(g_hwndTerminal);
                            
                            int state = ReadIniInt(L"Terminal", L"State", SW_SHOW);
                            ShowWindow(g_hwndTerminal, state);
                            UpdateWindow(g_hwndTerminal);
                        }
                    } else {
                        MessageBoxW(hwnd, L"\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u043e\u0442\u043a\u0440\u044b\u0442\u044c \u043f\u043e\u0440\u0442!", L"\u041e\u0448\u0438\u0431\u043a\u0430", MB_ICONERROR|MB_OK);
                    }
                    break;
                }
                
                case IDC_BTN_CANCEL: {
                    PostQuitMessage(0);
                    break;
                }
            }
            return 0;
        }
        case WM_DESTROY: {
            if (g_hPort != INVALID_HANDLE_VALUE) {
                com_close();
            }
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ============================================================================
// ОКНО ТЕРМИНАЛА (СОЗДАНИЕ И НАСТРОЙКА)
// ============================================================================

LRESULT CALLBACK TerminalWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wp;
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(g_pTheme->bgColor);
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            return 1;
        }
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->btnBg);
            SetTextColor(hdc, g_pTheme->btnFg);
            HFONT hFont = (HFONT)SendMessage((HWND)lp, WM_GETFONT, 0, 0);
            if (hFont) {
                SelectObject(hdc, hFont);
            }
            return (LRESULT)CreateSolidBrush(g_pTheme->btnBg);
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->editBg);
            SetTextColor(hdc, g_pTheme->editFg);
            return (LRESULT)CreateSolidBrush(g_pTheme->editBg);
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wp;
            SetBkColor(hdc, g_pTheme->bgColor);
            SetTextColor(hdc, g_pTheme->fgColor);
            return (LRESULT)CreateSolidBrush(g_pTheme->bgColor);
        }
        case WM_CREATE: {
            LoadLibraryW(L"riched20.dll");
            
            g_hEditRx = CreateWindowExW(0, L"RichEdit20W", L"",
                WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,
                0, 0, 10, 10, hwnd, (HMENU)IDC_EDIT_RX, GetModuleHandle(NULL), NULL);
            
            g_hBtnRxRefresh = CreateWindowExW(0, L"BUTTON", L"RX", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 15, 0, 30, 22, hwnd, (HMENU)IDC_BTN_RX_REFRESH, NULL, NULL);
            SendMessage(g_hBtnRxRefresh, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            g_hBtnRxMode = CreateWindowExW(0, L"BUTTON", L"RX:TEXT", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 50, 0, 70, 22, hwnd, (HMENU)IDC_BTN_RX_MODE, NULL, NULL);
            SendMessage(g_hBtnRxMode, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            g_hComboFontSize = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 125, 0, 60, 200, hwnd, (HMENU)IDC_COMBO_FONT_SIZE, NULL, NULL);
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"8");
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"9");
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"10");
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"11");
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"12");
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"14");
            SendMessageW(g_hComboFontSize, CB_ADDSTRING, 0, (LPARAM)L"16");
            
            int fontSizeIdx = ReadIniInt(L"Terminal", L"FontSize", 2);
            SendMessage(g_hComboFontSize, CB_SETCURSEL, fontSizeIdx, 0);
            
            wchar_t szSize[16] = {0};
            SendMessageW(g_hComboFontSize, CB_GETLBTEXT, fontSizeIdx, (LPARAM)szSize);
            g_fontSize = _wtoi(szSize);
            if (g_fontSize <= 0) g_fontSize = 10;
            
            if (g_hMonoFont) DeleteObject(g_hMonoFont);
            g_hMonoFont = CreateMonoFont(-MulDiv(g_fontSize, 96, 72));
            
            if (g_hEditRx) SendMessage(g_hEditRx, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            
            g_hComboEnc = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 190, 0, 140, 200, hwnd, (HMENU)IDC_COMBO_ENC, NULL, NULL);
            for (int e = 0; e < g_encodingCount; e++)
                SendMessageW(g_hComboEnc, CB_ADDSTRING, 0, (LPARAM)g_encodings[e].name);
            SendMessage(g_hComboEnc, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            
            int encodingIdx = ReadIniInt(L"Terminal", L"Encoding", 2);
            SendMessage(g_hComboEnc, CB_SETCURSEL, encodingIdx, 0);
            
            g_hBtnTxMode = CreateWindowExW(0, L"BUTTON", L"TX:TEXT", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 335, 0, 70, 22, hwnd, (HMENU)IDC_BTN_TX_MODE, NULL, NULL);
            SendMessage(g_hBtnTxMode, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            g_hChkEcho = CreateWindowExW(0, L"BUTTON", L"Echo", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, 410, 0, 75, 20, hwnd, (HMENU)IDC_CHK_ECHO, NULL, NULL);
            int echo = ReadIniInt(L"Terminal", L"Echo", 1);
            SendMessage(g_hChkEcho, BM_SETCHECK, echo ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(g_hChkEcho, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            
            g_hBtnTheme = CreateWindowExW(0, L"BUTTON", L"Theme", 
                WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 490, 0, 50, 22, hwnd, (HMENU)IDC_BTN_THEME, NULL, NULL);
            SendMessage(g_hBtnTheme, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            g_hBtnAbout = CreateWindowExW(0, L"BUTTON", L"Info", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 545, 0, 35, 22, hwnd, (HMENU)IDC_BTN_ABOUT, NULL, NULL);
            SendMessage(g_hBtnAbout, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            g_hComboTx = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL|WS_TABSTOP, 0, 0, 10, 10, hwnd, (HMENU)IDC_COMBO_TX, NULL, NULL);
            SendMessage(g_hComboTx, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            HWND hEditPart = GetWindow(g_hComboTx, GW_CHILD);
            if (hEditPart) g_pfnOrigEditProc = (WNDPROC)SetWindowLongPtrW(hEditPart, GWLP_WNDPROC, (LONG_PTR)TxEditSubProc);
            
            g_hComboSuffix = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWN|WS_VSCROLL|WS_TABSTOP, 0, 0, 100, 10, hwnd, (HMENU)IDC_COMBO_SUFFIX, NULL, NULL);
            SendMessage(g_hComboSuffix, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            SendMessageW(g_hComboSuffix, CB_ADDSTRING, 0, (LPARAM)L"");
            SendMessageW(g_hComboSuffix, CB_ADDSTRING, 0, (LPARAM)L"`0D0A`");
            SendMessageW(g_hComboSuffix, CB_ADDSTRING, 0, (LPARAM)L"`0A`");
            SendMessageW(g_hComboSuffix, CB_ADDSTRING, 0, (LPARAM)L"`0D`");
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
            wchar_t delayStr[32];
            swprintf(delayStr, 32, L"%d", repeatDelay);
            SetWindowTextW(g_hEditDelay, delayStr);
            
            g_hEditScriptPath = CreateWindowExW(0, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_READONLY|WS_BORDER, 0, 0, 10, 22, hwnd, (HMENU)IDC_EDIT_SCRIPT_PATH, NULL, NULL);
            SendMessage(g_hEditScriptPath, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            
            g_hBtnScriptInfo = CreateWindowExW(0, L"BUTTON", L"?", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 22, 22, hwnd, (HMENU)IDC_BTN_SCRIPT_INFO, NULL, NULL);
            SendMessage(g_hBtnScriptInfo, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            g_hBtnLoadScript = CreateWindowExW(0, L"BUTTON", L"LOAD", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 40, 22, hwnd, (HMENU)IDC_BTN_LOAD_SCRIPT, NULL, NULL);
            SendMessage(g_hBtnLoadScript, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            
            g_hBtnRunScript = CreateWindowExW(0, L"BUTTON", L"RUN", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 40, 22, hwnd, (HMENU)IDC_BTN_RUN_SCRIPT, NULL, NULL);
            SendMessage(g_hBtnRunScript, WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);

            for (int m = 0; m < MACRO_BANK_COUNT; m++) {
                wchar_t lbl[8];
                swprintf(lbl, 8, L"M%d", m + 1);
                g_hMacroBankBtns[m] = CreateWindowExW(0, L"BUTTON", lbl,
                    WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 0, 0, 10, 22,
                    hwnd, (HMENU)(LONG_PTR)(IDC_BTN_MACRO_BASE + m), NULL, NULL);
                SendMessage(g_hMacroBankBtns[m], WM_SETFONT, (WPARAM)g_hBtnFont, TRUE);
            }

            HFONT hSysFont = GetStockObject(DEFAULT_GUI_FONT);
            EnumChildWindows(hwnd, SetFontCallback, (LPARAM)hSysFont);
            
            SendMessage(g_hComboFontSize, WM_SETFONT, (WPARAM)g_hMonoFont, TRUE);
            
            g_txMode = ReadIniInt(L"Terminal", L"TxMode", TX_MODE_TEXT);
            g_rxMode = ReadIniInt(L"Terminal", L"RxMode", RX_MODE_TEXT);
            update_tx_mode_ui();
            update_rx_mode_ui();
            
            ApplyThemeToWindow(hwnd);
            
            return 0;
        }
        case WM_SIZE: {
            int w = LOWORD(lp), h = HIWORD(lp);
            int margin = 15, barH = 28, btnH = 25;
            
            MoveWindow(g_hBtnRxRefresh, margin, margin, 30, 22, TRUE);
            MoveWindow(g_hBtnRxMode, 50, margin, 70, 22, TRUE);
            MoveWindow(g_hComboFontSize, 125, margin, 60, 200, TRUE);
            MoveWindow(g_hComboEnc, 190, margin, 140, 200, TRUE);
            MoveWindow(g_hBtnTxMode, 335, margin, 70, 22, TRUE);
            MoveWindow(g_hChkEcho, 410, margin+2, 75, 20, TRUE);
            MoveWindow(g_hBtnTheme, 490, margin, 50, 22, TRUE);
            MoveWindow(g_hBtnAbout, 545, margin, 35, 22, TRUE);
            
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
            for (int m = 0; m < MACRO_BANK_COUNT; m++) {
                MoveWindow(g_hMacroBankBtns[m], margin + m * macroBtnW, bottomY3, macroBtnW, btnH, TRUE);
            }
            
            RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
            
            return 0;
        }
        case WM_KEYDOWN: {
            if (wp == VK_F1) { show_about_dialog(hwnd); return 0; }
            break;
        }

        case WM_TIMER: {
            if (wp == TIMER_READ_ID && g_hPort != INVALID_HANDLE_VALUE && !g_session.isWaitingReconnect) {
                BYTE tmpBuf[512]; DWORD bytesRead = 0;
                if (ReadFile(g_hPort, tmpBuf, sizeof(tmpBuf), &bytesRead, NULL) && bytesRead > 0) {
                    DWORD space = RX_BUF_SIZE - g_rxRawLen;
                    if (bytesRead > space) {
                        DWORD keep = RX_BUF_SIZE - bytesRead;
                        if (keep > 0) memmove(g_rxRawBuf, g_rxRawBuf + g_rxRawLen - keep, keep);
                        g_rxRawLen = keep;
                    }
                    memcpy(g_rxRawBuf + g_rxRawLen, tmpBuf, bytesRead);
                    g_rxRawLen += bytesRead;
                    render_rx_buffer(TRUE);
                }
            } else if (wp == TIMER_RECONNECT_ID) {
                static int skipTicks = 2;
                if (g_hPort != INVALID_HANDLE_VALUE && !g_session.isWaitingReconnect) {
                    if (skipTicks > 0) { skipTicks--; return 0; }
                    DWORD ms = 0;
                    if (!GetCommModemStatus(g_hPort, &ms)) {
                        PurgeComm(g_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
                        com_close(); Sleep(50);
                        g_session.isWaitingReconnect = TRUE; skipTicks = 2;
                        SetWindowTextW(hwnd, L"WT232 Terminal [\u041e\u0436\u0438\u0434\u0430\u043d\u0438\u0435...]");
                        EnableWindow(g_hBtnSend, FALSE);
                        KillTimer(hwnd, TIMER_REPEAT_ID); KillTimer(hwnd, TIMER_SCRIPT_ID);
                        SetWindowTextW(g_hBtnSend, L"SEND");
                    }
                } else if (g_session.isWaitingReconnect) {
                    wchar_t fp[MAX_PORT_NAME]={0}; BOOL found=FALSE;
                    check_and_reconnect_search(fp, &found);
                    if (found) {
                        if (g_hPort != INVALID_HANDLE_VALUE) {
                            PurgeComm(g_hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
                            CloseHandle(g_hPort); g_hPort = INVALID_HANDLE_VALUE; Sleep(50);
                        }
                        if (com_open(fp, g_session.lastBaudrate)) {
                            g_session.isWaitingReconnect = FALSE;
                            wcscpy(g_session.lastPortName, fp); skipTicks = 2;
                            update_terminal_title(fp, g_session.lastBaudrate);
                            SetWindowTextW(hwnd, g_szTitle);
                            EnableWindow(g_hBtnSend, TRUE);
                            if (SendMessage(g_hChkRepeat, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                                wchar_t delayStr[32] = {0};
                                GetWindowTextW(g_hEditDelay, delayStr, 32);
                                DWORD delay = _wtol(delayStr);
                                if (delay < 10) delay = 10;
                                SetTimer(hwnd, TIMER_REPEAT_ID, delay, NULL);
                                SetWindowTextW(g_hBtnSend, L"STOP");
                            }
                            if (g_isScriptRunning) {
                                SetWindowTextW(g_hBtnRunScript, L"STOP");
                                RunNextScriptCommand();
                            }
                        }
                    }
                }
            } else if (wp == TIMER_REPEAT_ID) {
                if (g_hPort != INVALID_HANDLE_VALUE && IsWindowVisible(g_hwndTerminal)) com_send(hwnd);
                else { KillTimer(hwnd, TIMER_REPEAT_ID); SetWindowTextW(g_hBtnSend, L"SEND"); }
            } else if (wp == TIMER_SCRIPT_ID) {
                if (g_hPort != INVALID_HANDLE_VALUE && IsWindowVisible(g_hwndTerminal)) RunNextScriptCommand();
                else KillTimer(hwnd, TIMER_SCRIPT_ID);
            }
            return 0;
        }
        case WM_COMMAND: {
            int cmdId = LOWORD(wp);
            
            if (cmdId == IDC_BTN_THEME) {
                int newTheme = (g_currentTheme + 1) % 2;
                SetTheme(newTheme);
                return 0;
            }
            
            if (cmdId >= IDC_BTN_MACRO_BASE && cmdId < IDC_BTN_MACRO_BASE + MACRO_BANK_COUNT) {
                int bankIdx = cmdId - IDC_BTN_MACRO_BASE;
                ShowMacroPad(hwnd, bankIdx);
                return 0;
            }
            
            switch (cmdId) {
                case IDC_BTN_RX_MODE:
                    g_rxMode = (g_rxMode == RX_MODE_TEXT) ? RX_MODE_DUMP : RX_MODE_TEXT;
                    update_rx_mode_ui(); render_rx_buffer(FALSE); break;
                    
                case IDC_COMBO_FONT_SIZE:
                    if (HIWORD(wp) == CBN_SELCHANGE) {
                        int idx = (int)SendMessageW(g_hComboFontSize, CB_GETCURSEL, 0, 0);
                        if (idx != CB_ERR) {
                            wchar_t szSize[16] = {0};
                            SendMessageW(g_hComboFontSize, CB_GETLBTEXT, idx, (LPARAM)szSize);
                            int size = _wtoi(szSize);
                            if (size > 0) { 
                                ApplyFontSize(-MulDiv(size, 96, 72)); 
                                render_rx_buffer(FALSE); 
                            }
                            WriteIniInt(L"Terminal", L"FontSize", idx);
                        }
                    }
                    break;
                    
                case IDC_BTN_TX_MODE:
                    g_txMode = (g_txMode == TX_MODE_HEX) ? TX_MODE_TEXT : TX_MODE_HEX;
                    update_tx_mode_ui(); break;
                    
                case IDC_BTN_SEND: {
                    if (SendMessage(g_hChkRepeat, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        wchar_t currentText[32];
                        GetWindowTextW(g_hBtnSend, currentText, 32);
                        if (wcscmp(currentText, L"STOP") == 0) {
                            KillTimer(hwnd, TIMER_REPEAT_ID); SetWindowTextW(g_hBtnSend, L"SEND");
                        } else {
                            wchar_t delayStr[32] = {0};
                            GetWindowTextW(g_hEditDelay, delayStr, 32);
                            DWORD delay = _wtol(delayStr);
                            if (delay < 10) delay = 10;
                            SetTimer(hwnd, TIMER_REPEAT_ID, delay, NULL);
                            SetWindowTextW(g_hBtnSend, L"STOP"); com_send(hwnd);
                        }
                    } else {
                        KillTimer(hwnd, TIMER_REPEAT_ID); SetWindowTextW(g_hBtnSend, L"SEND"); com_send(hwnd);
                    }
                    break;
                }
                case IDC_BTN_CLEAR:
                    SetWindowTextW(g_hEditRx, L""); g_rxRawLen = 0; g_lastRenderedLen = 0; g_hexLinePos = 0; break;
                    
                case IDC_BTN_RX_REFRESH: render_rx_buffer(FALSE); break;
                case IDC_BTN_ABOUT: show_about_dialog(hwnd); break;
                
                case IDC_COMBO_ENC:
                    if (HIWORD(wp) == CBN_SELCHANGE) render_rx_buffer(FALSE); break;
                    
                case IDC_BTN_SUFFIX_INFO: {
                    static const wchar_t suffixHelp[] =
                        L"\u0421\u043f\u0440\u0430\u0432\u043a\u0430 \u043f\u043e \u0441\u0443\u0444\u0444\u0438\u043a\u0441\u0430\u043c:\r\n==========================================\r\n\r\n"
                        L"\u041f\u0440\u0435\u0441\u0435\u0442\u044b:\r\n  (\u043f\u0443\u0441\u0442\u043e)  \u2014 \u0431\u0435\u0437 \u0441\u0443\u0444\u0444\u0438\u043a\u0441\u0430\r\n  `0D0A`   \u2014 CR+LF (Windows)\r\n"
                        L"  `0A`     \u2014 LF (Unix/Linux)\r\n  `0D`     \u2014 CR (Mac Classic)\r\n\r\n"
                        L"\u041f\u043e\u043b\u044c\u0437\u043e\u0432\u0430\u0442\u0435\u043b\u044c\u0441\u043a\u0438\u0439 \u0432\u0432\u043e\u0434:\r\n  \u041b\u044e\u0431\u043e\u0439 HEX \u0432 \u043e\u0431\u0440\u0430\u0442\u043d\u044b\u0445 \u043a\u0430\u0432\u044b\u0447\u043a\u0430\u0445:\r\n  `XX`, `XXXX`, `AA BB CC`\r\n"
                        L"  \u041f\u0440\u043e\u0431\u0435\u043b\u044b \u0432\u043d\u0443\u0442\u0440\u0438 \u043a\u0430\u0432\u044b\u0447\u0435\u043a \u0438\u0433\u043d\u043e\u0440\u0438\u0440\u0443\u044e\u0442\u0441\u044f\r\n  \u0420\u0435\u0433\u0438\u0441\u0442\u0440 \u043d\u0435 \u0432\u0430\u0436\u0435\u043d: `aa` = `AA`\r\n\r\n"
                        L"\u0421\u0443\u0444\u0444\u0438\u043a\u0441 \u043f\u0440\u0438\u043c\u0435\u043d\u044f\u0435\u0442\u0441\u044f \u0442\u043e\u043b\u044c\u043a\u043e \u0432 TEXT-\u0440\u0435\u0436\u0438\u043c\u0435.";
                    HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class",
                        L"\u0421\u043f\u0440\u0430\u0432\u043a\u0430: \u0421\u0443\u0444\u0444\u0438\u043a\u0441\u044b", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 380, 420,
                        hwnd, NULL, GetModuleHandle(NULL), (LPVOID)suffixHelp);
                    if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); }
                    break;
                }
                case IDC_BTN_SCRIPT_INFO: {
                    static const wchar_t scriptHelp[] =
                        L"\u0421\u043f\u0440\u0430\u0432\u043a\u0430 \u043f\u043e \u0441\u043a\u0440\u0438\u043f\u0442\u0430\u043c:\r\n==========================================\r\n\r\n"
                        L"\u0424\u043e\u0440\u043c\u0430\u0442 \u0444\u0430\u0439\u043b\u0430 (.txt):\r\n  \u041a\u0430\u0436\u0434\u0430\u044f \u0441\u0442\u0440\u043e\u043a\u0430 - \u043e\u0434\u043d\u0430 \u043a\u043e\u043c\u0430\u043d\u0434\u0430.\r\n  \u041f\u043e\u0434\u0434\u0435\u0440\u0436\u0438\u0432\u0430\u0435\u0442\u0441\u044f \u0438\u043d\u043b\u0430\u0439\u043d-HEX (`XX`).\r\n\r\n"
                        L"\u0414\u0438\u0440\u0435\u043a\u0442\u0438\u0432\u044b:\r\n  1000      - \u0413\u043b\u043e\u0431\u0430\u043b\u044c\u043d\u0430\u044f \u0437\u0430\u0434\u0435\u0440\u0436\u043a\u0430 (\u043c\u0441)\r\n              (\u0442\u043e\u043b\u044c\u043a\u043e \u0432 \u043f\u0435\u0440\u0432\u043e\u0439 \u0441\u0442\u0440\u043e\u043a\u0435)\r\n"
                        L"  #DELAY 500 - \u0417\u0430\u0434\u0435\u0440\u0436\u043a\u0430 \u043f\u0435\u0440\u0435\u0434 \u0441\u043b\u0435\u0434. \u043a\u043e\u043c\u0430\u043d\u0434\u043e\u0439\r\n  #STOP      - \u041e\u0441\u0442\u0430\u043d\u043e\u0432\u0438\u0442\u044c \u0441\u043a\u0440\u0438\u043f\u0442\r\n"
                        L"  #...       - \u041a\u043e\u043c\u043c\u0435\u043d\u0442\u0430\u0440\u0438\u0439 (\u0438\u0433\u043d\u043e\u0440\u0438\u0440\u0443\u0435\u0442\u0441\u044f)\r\n\r\n"
                        L"\u0411\u0435\u0437 #STOP \u0441\u043a\u0440\u0438\u043f\u0442 \u0432\u044b\u043f\u043e\u043b\u043d\u044f\u0435\u0442\u0441\u044f \u0446\u0438\u043a\u043b\u0438\u0447\u0435\u0441\u043a\u0438.";
                    HWND hInfo = CreateWindowExW(WS_EX_TOOLWINDOW, L"WT232_Info_Scroll_Class",
                        L"\u0421\u043f\u0440\u0430\u0432\u043a\u0430: \u0421\u043a\u0440\u0438\u043f\u0442\u044b", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 380, 450,
                        hwnd, NULL, GetModuleHandle(NULL), (LPVOID)scriptHelp);
                    if (hInfo) { ShowWindow(hInfo, SW_SHOW); UpdateWindow(hInfo); ApplyThemeToWindow(hInfo); }
                    break;
                }
                case IDC_BTN_LOAD_SCRIPT: {
                    OPENFILENAMEW ofn;
                    wchar_t szFile[MAX_PATH] = L"";
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = szFile; ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                    ofn.nFilterIndex = 1; ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileNameW(&ofn)) LoadScriptFile(szFile);
                    break;
                }
                case IDC_BTN_RUN_SCRIPT: {
                    if (g_isScriptRunning) StopScript();
                    else {
                        if (g_scriptCount > 0) {
                            g_isScriptRunning = TRUE;
                            SetWindowTextW(g_hBtnRunScript, L"STOP");
                            g_scriptCurrentIndex = 0; RunNextScriptCommand();
                        } else MessageBoxW(hwnd, L"\u0421\u043d\u0430\u0447\u0430\u043b\u0430 \u0437\u0430\u0433\u0440\u0443\u0437\u0438\u0442\u0435 \u0444\u0430\u0439\u043b \u0441\u043a\u0440\u0438\u043f\u0442\u0430!", L"\u0412\u043d\u0438\u043c\u0430\u043d\u0438\u0435", MB_ICONWARNING);
                    }
                    break;
                }
            }
            return 0;
        }
        case WM_CLOSE: {
            KillTimer(hwnd, TIMER_RECONNECT_ID);
            KillTimer(hwnd, TIMER_READ_ID);
            KillTimer(hwnd, TIMER_REPEAT_ID);
            KillTimer(hwnd, TIMER_SCRIPT_ID);
            
            for (int m = 0; m < MACRO_BANK_COUNT; m++) {
                if (g_hwndMacroPads[m] && IsWindow(g_hwndMacroPads[m])) {
                    DestroyWindow(g_hwndMacroPads[m]);
                }
            }
            if (g_hwndMacroEdit && IsWindow(g_hwndMacroEdit)) {
                DestroyWindow(g_hwndMacroEdit);
            }
            
            SaveAllMacroWindowsState();
            
            int historyCount = (int)SendMessageW(g_hComboTx, CB_GETCOUNT, 0, 0);
            if (historyCount > MAX_HISTORY) historyCount = MAX_HISTORY;
            WriteIniInt(L"History", L"Count", historyCount);
            for (int i = 0; i < historyCount; i++) {
                wchar_t key[16];
                wchar_t buf[MAX_LINE_LEN] = {0};
                swprintf(key, 16, L"Cmd%d", i);
                SendMessageW(g_hComboTx, CB_GETLBTEXT, i, (LPARAM)buf);
                WriteIniString(L"History", key, buf);
            }
            
            WriteIniInt(L"Terminal", L"Echo", is_echo_enabled() ? 1 : 0);
            WriteIniInt(L"Terminal", L"TxMode", g_txMode);
            WriteIniInt(L"Terminal", L"RxMode", g_rxMode);
            WriteIniInt(L"Terminal", L"Encoding", (int)SendMessageW(g_hComboEnc, CB_GETCURSEL, 0, 0));
            
            wchar_t delayStr[32] = {0};
            GetWindowTextW(g_hEditDelay, delayStr, 32);
            WriteIniInt(L"Terminal", L"RepeatDelay", _wtoi(delayStr));
            
            RECT rc;
            GetWindowRect(hwnd, &rc);
            WriteIniInt(L"Terminal", L"X", rc.left);
            WriteIniInt(L"Terminal", L"Y", rc.top);
            WriteIniInt(L"Terminal", L"Width", rc.right - rc.left);
            WriteIniInt(L"Terminal", L"Height", rc.bottom - rc.top);
            
            WINDOWPLACEMENT wp;
            wp.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(hwnd, &wp);
            WriteIniInt(L"Terminal", L"State", wp.showCmd);
            
            WriteAllIni();
            com_close();
            
            DestroyWindow(hwnd);
            g_hwndTerminal = NULL;
            g_pfnOrigEditProc = NULL;
            
            wchar_t lastPort[MAX_PORT_NAME] = {0};
            ReadIniString(L"Port", L"LastPortName", lastPort, MAX_PORT_NAME, L"");
            
            if (g_hwndSettings && IsWindow(g_hwndSettings)) {
                ShowWindow(g_hwndSettings, SW_SHOW);
                SetForegroundWindow(g_hwndSettings);
                init_com_ports(g_hwndSettings, FALSE, lastPort);
            } else {
                wchar_t settingsTitle[128];
                swprintf(settingsTitle, 128, L"WT232 Settings " APP_VERSION);
                g_hwndSettings = CreateWindowExW(0, L"WT232_Settings_Class", settingsTitle,
                    WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX | WS_CLIPCHILDREN,
                    CW_USEDEFAULT, CW_USEDEFAULT, 300, 330,
                    NULL, NULL, GetModuleHandle(NULL), NULL);
                if (g_hwndSettings) {
                    init_com_ports(g_hwndSettings, FALSE, lastPort);
                    ApplyThemeToWindow(g_hwndSettings);
                    ShowWindow(g_hwndSettings, SW_SHOW);
                    UpdateWindow(g_hwndSettings);
                }
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ============================================================================
// КОНЕЦ ФАЙЛА
// ============================================================================
