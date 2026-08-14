#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define HOTKEY_F8  2
#define HOTKEY_F6  3
#define HOTKEY_F5  4
#define TIMER_UPDATE  1

#define ID_MINFPS    101
#define ID_MAXFPS    102
#define ID_MINPING   103
#define ID_MAXPING   104
#define ID_LOC       105
#define ID_TIME      106
#define ID_PRIVATE   107
#define ID_BTN       108

struct OverlayConfig {
    int minFPS;
    int maxFPS;
    int minPing;
    int maxPing;
    std::wstring location;
    bool privateServer;
    int timeSelection;
};

static OverlayConfig g_config;
static int   g_currentFPS  = 0;
static int   g_currentPing = 0;
static DWORD g_overlayStartTime = 0;
static DWORD g_timerOffset = 0;
static int   g_version = 0;
static DWORD g_nextPingUpdate = 0;
static int   g_targetFPS = -1;

static HWND  g_hwndMenu = NULL;
static HWND  g_hwndOverlay = NULL;
static HFONT g_hFontOverlay = NULL;
static HFONT g_hFontMenu = NULL;
static HBRUSH g_hbrBlack = NULL;
static HINSTANCE g_hInstance = NULL;

static HDC     g_hdcBuffer = NULL;
static HBITMAP g_hBmpBuffer = NULL;
static HBITMAP g_hBmpOld = NULL;
static int     g_bufW = 0, g_bufH = 0;

static const std::vector<std::wstring> g_locations = {
    L"London, England", L"Paris, \x00CEle-de-France", L"Frankfurt am Main, Hesse",
    L"Amsterdam, North Holland", L"Dallas, Texas", L"Chicago, Illinois",
    L"New York City, New York", L"Los Angeles, California", L"Seattle, Washington",
    L"Miami, Florida", L"Tokyo, Japan", L"Singapore",
    L"Sydney, New South Wales", L"Mumbai, Maharashtra", L"S\x00E3o Paulo, S\x00E3o Paulo"
};

static void CreateAppFonts(void) {
    g_hFontOverlay = CreateFontW(
        -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );
    g_hFontMenu = CreateFontW(
        -16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );
}

static void DestroyAppFonts(void) {
    if (g_hFontOverlay) DeleteObject(g_hFontOverlay);
    if (g_hFontMenu) DeleteObject(g_hFontMenu);
    if (g_hbrBlack) DeleteObject(g_hbrBlack);
}

static void CreateDoubleBuffer(HDC hdcScreen, int w, int h) {
    g_hdcBuffer = CreateCompatibleDC(hdcScreen);
    g_hBmpBuffer = CreateCompatibleBitmap(hdcScreen, w, h);
    g_hBmpOld = (HBITMAP)SelectObject(g_hdcBuffer, g_hBmpBuffer);
    g_bufW = w; g_bufH = h;
}

static void DestroyDoubleBuffer(void) {
    if (g_hdcBuffer) {
        SelectObject(g_hdcBuffer, g_hBmpOld);
        DeleteObject(g_hBmpBuffer);
        DeleteDC(g_hdcBuffer);
        g_hdcBuffer = NULL;
        g_hBmpBuffer = NULL;
        g_hBmpOld = NULL;
    }
}

static void StopOverlay(void) {
    if (!g_hwndOverlay) return;
    KillTimer(g_hwndOverlay, TIMER_UPDATE);
    UnregisterHotKey(g_hwndOverlay, HOTKEY_F8);
    UnregisterHotKey(g_hwndOverlay, HOTKEY_F6);
    UnregisterHotKey(g_hwndOverlay, HOTKEY_F5);
    DestroyDoubleBuffer();
    DestroyWindow(g_hwndOverlay);
    g_hwndOverlay = NULL;
}

static void DrawOutlinedText(HDC hdc, int x, int y, const wchar_t* text) {
    int len = lstrlenW(text);

    SetTextColor(hdc, RGB(1, 1, 1));
    TextOutW(hdc, x - 1, y, text, len);
    TextOutW(hdc, x + 1, y, text, len);
    TextOutW(hdc, x, y - 1, text, len);
    TextOutW(hdc, x, y + 1, text, len);
    TextOutW(hdc, x - 1, y - 1, text, len);
    TextOutW(hdc, x + 1, y - 1, text, len);
    TextOutW(hdc, x - 1, y + 1, text, len);
    TextOutW(hdc, x + 1, y + 1, text, len);

    SetTextColor(hdc, RGB(255, 255, 255));
    TextOutW(hdc, x, y, text, len);
}

static void UpdateFPSValue(void) {
    if (g_targetFPS == -1) {
        int range = g_config.maxFPS - g_config.minFPS;
        if (range < 1) range = 1;
        g_targetFPS = g_config.minFPS + (rand() % (range + 1));
    }

    int step = 15 + (rand() % 31);

    if (g_currentFPS < g_targetFPS) {
        g_currentFPS += step;
        if (g_currentFPS >= g_targetFPS) {
            int range = g_config.maxFPS - g_config.minFPS;
            if (range < 1) range = 1;
            g_targetFPS = g_config.minFPS + (rand() % (range + 1));
        }
    } else if (g_currentFPS > g_targetFPS) {
        g_currentFPS -= step;
        if (g_currentFPS <= g_targetFPS) {
            int range = g_config.maxFPS - g_config.minFPS;
            if (range < 1) range = 1;
            g_targetFPS = g_config.minFPS + (rand() % (range + 1));
        }
    } else {
        int range = g_config.maxFPS - g_config.minFPS;
        if (range < 1) range = 1;
        g_targetFPS = g_config.minFPS + (rand() % (range + 1));
    }

    if (g_currentFPS > g_config.maxFPS) g_currentFPS = g_config.maxFPS;
    if (g_currentFPS < g_config.minFPS) g_currentFPS = g_config.minFPS;
}

static void UpdatePingValue(void) {
    DWORD now = GetTickCount();
    if (now < g_nextPingUpdate) return;

    int delta = 5 + (rand() % 3);
    if (rand() % 2 == 0) delta = -delta;

    g_currentPing += delta;
    if (g_currentPing < g_config.minPing) g_currentPing = g_config.minPing + (rand() % 3);
    if (g_currentPing > g_config.maxPing) g_currentPing = g_config.maxPing - (rand() % 3);

    g_nextPingUpdate = now + 7000;
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        if (wParam == TIMER_UPDATE) {
            UpdateFPSValue();
            UpdatePingValue();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        int scrW = g_bufW, scrH = g_bufH;
        if (!g_hdcBuffer) { EndPaint(hwnd, &ps); return 0; }

        HDC bufDC = g_hdcBuffer;

        RECT rcAll = { 0, 0, scrW, scrH };
        HBRUSH hBlack = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(bufDC, &rcAll, hBlack);
        DeleteObject(hBlack);

        DWORD elapsed = GetTickCount() - g_overlayStartTime + g_timerOffset;
        DWORD totalSeconds = elapsed / 1000;
        DWORD days    = totalSeconds / 86400;
        DWORD hours   = (totalSeconds % 86400) / 3600;
        DWORD minutes = (totalSeconds % 3600) / 60;
        DWORD seconds = totalSeconds % 60;

        wchar_t timerStr[32];
        swprintf(timerStr, 32, L"%d:%02d:%02d:%02d", days, hours, minutes, seconds);

        wchar_t displayStr[512];
        if (g_config.privateServer) {
            swprintf(displayStr, 512, L"FPS: %d, Ping: %dms, Server: %ls (%ls) [V%d, Private]",
                g_currentFPS, g_currentPing, g_config.location.c_str(), timerStr, g_version);
        } else {
            swprintf(displayStr, 512, L"FPS: %d, Ping: %dms, Server: %ls (%ls) [V%d]",
                g_currentFPS, g_currentPing, g_config.location.c_str(), timerStr, g_version);
        }

        SetBkMode(bufDC, TRANSPARENT);
        HFONT hOldFont = (HFONT)SelectObject(bufDC, g_hFontOverlay);

        SIZE textSize;
        GetTextExtentPoint32W(bufDC, displayStr, (int)wcslen(displayStr), &textSize);

        int textX = scrW - textSize.cx - 10;
        int textY = 0;
        DrawOutlinedText(bufDC, textX, textY, displayStr);

        SelectObject(bufDC, hOldFont);
        BitBlt(hdc, 0, 0, scrW, scrH, bufDC, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_HOTKEY:
        if (wParam == HOTKEY_F8) {
            StopOverlay();
            PostQuitMessage(0);
        } else if (wParam == HOTKEY_F6) {
            ShowWindow(g_hwndMenu, SW_SHOW);
            StopOverlay();
        } else if (wParam == HOTKEY_F5) {
            static bool isVisible = true;
            isVisible = !isVisible;
            ShowWindow(hwnd, isVisible ? SW_SHOW : SW_HIDE);
        }
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void LaunchOverlay(void) {
    g_targetFPS = -1;
    g_nextPingUpdate = 0;
    g_version = 10000 + (rand() % 90000);
    g_currentFPS = (g_config.minFPS + g_config.maxFPS) / 2;
    g_currentPing = (g_config.minPing + g_config.maxPing) / 2;
    g_overlayStartTime = GetTickCount();

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = OverlayWndProc;
    wc.hInstance      = g_hInstance;
    wc.hIcon         = LoadIconW(g_hInstance, MAKEINTRESOURCEW(101));
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"FakeFPSOverlay";
    wc.hIconSm       = LoadIconW(g_hInstance, MAKEINTRESOURCEW(101));
    RegisterClassExW(&wc);

    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int ovW = 700;
    int ovH = 50;

    g_hwndOverlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"FakeFPSOverlay", NULL, WS_POPUP, scrW - ovW, 0, ovW, ovH, NULL, NULL, g_hInstance, NULL
    );

    HDC hdcScreen = GetDC(g_hwndOverlay);
    CreateDoubleBuffer(hdcScreen, ovW, ovH);
    ReleaseDC(g_hwndOverlay, hdcScreen);

    SetLayeredWindowAttributes(g_hwndOverlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(g_hwndOverlay, SW_SHOW);
    UpdateWindow(g_hwndOverlay);

    RegisterHotKey(g_hwndOverlay, HOTKEY_F8, 0, VK_F8);
    RegisterHotKey(g_hwndOverlay, HOTKEY_F6, 0, VK_F6);
    RegisterHotKey(g_hwndOverlay, HOTKEY_F5, 0, VK_F5);

    SetTimer(g_hwndOverlay, TIMER_UPDATE, 16, NULL);
}

LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
    {
        int y = 20;
        auto AddLabel = [&](const wchar_t* text, int yy) {
            HWND h = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, 20, yy, 150, 25, hwnd, NULL, g_hInstance, NULL);
            SendMessageW(h, WM_SETFONT, (WPARAM)g_hFontMenu, TRUE);
        };
        auto AddEdit = [&](int id, const wchar_t* defText, int yy) {
            HWND h = CreateWindowW(L"EDIT", defText, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 180, yy, 180, 25, hwnd, (HMENU)(UINT_PTR)id, g_hInstance, NULL);
            SendMessageW(h, WM_SETFONT, (WPARAM)g_hFontMenu, TRUE);
        };

        AddLabel(L"Min FPS:", y); AddEdit(ID_MINFPS, L"4900", y); y += 35;
        AddLabel(L"Max FPS:", y); AddEdit(ID_MAXFPS, L"5000", y); y += 35;
        AddLabel(L"Min Ping:", y); AddEdit(ID_MINPING, L"30", y); y += 35;
        AddLabel(L"Max Ping:", y); AddEdit(ID_MAXPING, L"90", y); y += 35;

        AddLabel(L"Location:", y);
        HWND hCmbLoc = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 180, y, 180, 200, hwnd, (HMENU)ID_LOC, g_hInstance, NULL);
        SendMessageW(hCmbLoc, WM_SETFONT, (WPARAM)g_hFontMenu, TRUE);
        for (const auto& loc : g_locations) SendMessageW(hCmbLoc, CB_ADDSTRING, 0, (LPARAM)loc.c_str());
        SendMessageW(hCmbLoc, CB_SETCURSEL, 2, 0);
        y += 35;

        AddLabel(L"Start Mins:", y); AddEdit(ID_TIME, L"0", y); y += 35;

        HWND hChkRand = CreateWindowW(L"BUTTON", L"Random Time?", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 180, y, 180, 25, hwnd, (HMENU)109, g_hInstance, NULL);
        SendMessageW(hChkRand, WM_SETFONT, (WPARAM)g_hFontMenu, TRUE);
        y += 35;

        HWND hChk = CreateWindowW(L"BUTTON", L"Private Server?", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 180, y, 180, 25, hwnd, (HMENU)ID_PRIVATE, g_hInstance, NULL);
        SendMessageW(hChk, WM_SETFONT, (WPARAM)g_hFontMenu, TRUE);
        y += 45;

        HWND hBtn = CreateWindowW(L"BUTTON", L"Launch Overlay", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, y, 340, 40, hwnd, (HMENU)ID_BTN, g_hInstance, NULL);
        SendMessageW(hBtn, WM_SETFONT, (WPARAM)g_hFontMenu, TRUE);
        y += 45;

        HWND hGuide = CreateWindowW(L"STATIC", L"[F5] Hide/Show | [F6] Menu | [F8] Exit", WS_CHILD | WS_VISIBLE | SS_CENTER, 20, y, 340, 25, hwnd, NULL, g_hInstance, NULL);
        SendMessageW(hGuide, WM_SETFONT, (WPARAM)g_hFontMenu, TRUE);

        return 0;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(0, 0, 0));
        return (LRESULT)g_hbrBlack;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BTN) {
            wchar_t buf[64];
            GetDlgItemTextW(hwnd, ID_MINFPS, buf, 64); g_config.minFPS = _wtoi(buf);
            GetDlgItemTextW(hwnd, ID_MAXFPS, buf, 64); g_config.maxFPS = _wtoi(buf);
            GetDlgItemTextW(hwnd, ID_MINPING, buf, 64); g_config.minPing = _wtoi(buf);
            GetDlgItemTextW(hwnd, ID_MAXPING, buf, 64); g_config.maxPing = _wtoi(buf);

            if (g_config.maxFPS < g_config.minFPS) g_config.maxFPS = g_config.minFPS;
            if (g_config.maxPing < g_config.minPing) g_config.maxPing = g_config.minPing;

            int locIdx = SendMessageW(GetDlgItem(hwnd, ID_LOC), CB_GETCURSEL, 0, 0);
            if (locIdx >= 0 && locIdx < (int)g_locations.size()) g_config.location = g_locations[locIdx];
            else g_config.location = g_locations[0];

            GetDlgItemTextW(hwnd, ID_TIME, buf, 64);
            int startMins = _wtoi(buf);

            bool isRandomTime = (SendMessageW(GetDlgItem(hwnd, 109), BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (isRandomTime) {
                g_timerOffset = (rand() % (2 * 3600)) * 1000;
            } else {
                g_timerOffset = startMins * 60 * 1000;
            }

            g_config.privateServer = (SendMessageW(GetDlgItem(hwnd, ID_PRIVATE), BM_GETCHECK, 0, 0) == BST_CHECKED);

            ShowWindow(hwnd, SW_HIDE);
            LaunchOverlay();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetProcessDPIAware();
    srand((unsigned int)time(NULL));

    g_hInstance = hInstance;
    g_hbrBlack = CreateSolidBrush(RGB(0, 0, 0));
    CreateAppFonts();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MenuWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_hbrBlack;
    wc.lpszClassName = L"FakeFPSMenuClass";
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
    RegisterClassExW(&wc);

    int w = 400, h = 420;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    g_hwndMenu = CreateWindowExW(
        0, L"FakeFPSMenuClass", L"Fake FPS Setup",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (scrW - w) / 2, (scrH - h) / 2, w, h,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(g_hwndMenu, SW_SHOW);
    UpdateWindow(g_hwndMenu);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyAppFonts();
    return (int)msg.wParam;
}
