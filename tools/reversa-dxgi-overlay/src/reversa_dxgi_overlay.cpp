#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <cstdint>
#include <cwchar>
#include <string>

namespace {

constexpr wchar_t kWindowClassName[] = L"ReversaOverlayWindow";
constexpr UINT_PTR kRefreshTimer = 1;
constexpr DWORD kGameWindowWaitMs = 60000;

struct CpuSample {
    ULONGLONG processTime100ns = 0;
    ULONGLONG tickMs = 0;
};

struct OverlayStats {
    DWORD processId = 0;
    double cpuPercent = 0.0;
    SIZE_T workingSetMb = 0;
    SIZE_T privateMb = 0;
    DWORD handleCount = 0;
    DWORD threadCount = 0;
    DWORD displayHz = 0;
    RECT gameRect{};
    bool gameWindowFound = false;
    bool visible = true;
    wchar_t displayName[32]{};
};

HMODULE g_module = nullptr;
HWND g_overlay = nullptr;
HWND g_gameWindow = nullptr;
OverlayStats g_stats{};
CpuSample g_cpuPrevious{};
ULONGLONG g_startTick = 0;
bool g_clickThrough = true;

std::wstring GetModuleDirectory()
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(g_module, path, static_cast<DWORD>(std::size(path)));
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) {
        *slash = L'\0';
    }
    return path;
}

void LogLine(const wchar_t* text)
{
    std::wstring path = GetModuleDirectory() + L"\\reversa-overlay.log";
    HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t line[1024]{};
    swprintf_s(line, L"%04u-%02u-%02u %02u:%02u:%02u.%03u %s\r\n",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, text);

    DWORD bytesWritten = 0;
    DWORD bytesToWrite = static_cast<DWORD>(wcslen(line) * sizeof(wchar_t));
    if (GetFileSize(file, nullptr) == 0) {
        const wchar_t bom = 0xFEFF;
        WriteFile(file, &bom, sizeof(bom), &bytesWritten, nullptr);
    }
    WriteFile(file, line, bytesToWrite, &bytesWritten, nullptr);
    CloseHandle(file);
}

ULONGLONG FileTimeToUInt64(const FILETIME& ft)
{
    ULARGE_INTEGER value{};
    value.LowPart = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return value.QuadPart;
}

ULONGLONG CurrentProcessTime100ns()
{
    FILETIME createTime{}, exitTime{}, kernelTime{}, userTime{};
    if (!GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime)) {
        return 0;
    }
    return FileTimeToUInt64(kernelTime) + FileTimeToUInt64(userTime);
}

DWORD CountProcessThreads(DWORD processId)
{
    DWORD count = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    THREADENTRY32 entry{};
    entry.dwSize = sizeof(entry);
    if (Thread32First(snapshot, &entry)) {
        do {
            if (entry.th32OwnerProcessID == processId) {
                ++count;
            }
        } while (Thread32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return count;
}

BOOL CALLBACK EnumWindowsForCurrentProcess(HWND hwnd, LPARAM lParam)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId()) {
        return TRUE;
    }
    if (!IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) != nullptr) {
        return TRUE;
    }

    wchar_t title[256]{};
    GetWindowTextW(hwnd, title, static_cast<int>(std::size(title)));
    if (wcsstr(title, L"Call of Duty") == nullptr && wcsstr(title, L"ship") == nullptr) {
        return TRUE;
    }

    *reinterpret_cast<HWND*>(lParam) = hwnd;
    return FALSE;
}

HWND FindGameWindow()
{
    HWND found = nullptr;
    EnumWindows(EnumWindowsForCurrentProcess, reinterpret_cast<LPARAM>(&found));
    return found;
}

HWND WaitForGameWindow()
{
    const ULONGLONG startTick = GetTickCount64();
    while (GetTickCount64() - startTick < kGameWindowWaitMs) {
        HWND hwnd = FindGameWindow();
        if (hwnd) {
            return hwnd;
        }
        Sleep(250);
    }
    return nullptr;
}

void UpdateProcessStats()
{
    g_stats.processId = GetCurrentProcessId();

    ULONGLONG nowMs = GetTickCount64();
    ULONGLONG processTime = CurrentProcessTime100ns();
    if (g_cpuPrevious.tickMs != 0 && nowMs > g_cpuPrevious.tickMs) {
        double processDeltaMs = static_cast<double>(processTime - g_cpuPrevious.processTime100ns) / 10000.0;
        double wallDeltaMs = static_cast<double>(nowMs - g_cpuPrevious.tickMs);
        g_stats.cpuPercent = wallDeltaMs > 0.0 ? (processDeltaMs / wallDeltaMs) * 100.0 : 0.0;
    }
    g_cpuPrevious.processTime100ns = processTime;
    g_cpuPrevious.tickMs = nowMs;

    PROCESS_MEMORY_COUNTERS_EX memory{};
    memory.cb = sizeof(memory);
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory))) {
        g_stats.workingSetMb = memory.WorkingSetSize / (1024 * 1024);
        g_stats.privateMb = memory.PrivateUsage / (1024 * 1024);
    }

    DWORD handleCount = 0;
    if (GetProcessHandleCount(GetCurrentProcess(), &handleCount)) {
        g_stats.handleCount = handleCount;
    }

    g_stats.threadCount = CountProcessThreads(g_stats.processId);
}

void UpdateDisplayStats(HWND hwnd)
{
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) {
        g_stats.displayHz = 0;
        g_stats.displayName[0] = L'\0';
        return;
    }

    wcsncpy_s(g_stats.displayName, info.szDevice, _TRUNCATE);

    DEVMODEW mode{};
    mode.dmSize = sizeof(mode);
    if (EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode)) {
        g_stats.displayHz = mode.dmDisplayFrequency;
    }
}

void UpdateWindowPosition()
{
    if (!g_gameWindow || !IsWindow(g_gameWindow)) {
        g_gameWindow = FindGameWindow();
    }

    g_stats.gameWindowFound = false;
    if (!g_gameWindow || !IsWindow(g_gameWindow)) {
        ShowWindow(g_overlay, SW_HIDE);
        return;
    }

    RECT rect{};
    if (!GetWindowRect(g_gameWindow, &rect)) {
        ShowWindow(g_overlay, SW_HIDE);
        return;
    }

    g_stats.gameWindowFound = true;
    g_stats.gameRect = rect;
    UpdateDisplayStats(g_gameWindow);

    if (GetWindow(g_overlay, GW_OWNER) != g_gameWindow) {
        SetWindowLongPtrW(g_overlay, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(g_gameWindow));
    }

    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    SetWindowPos(g_overlay, HWND_TOPMOST, rect.left, rect.top, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void ToggleClickThrough()
{
    g_clickThrough = !g_clickThrough;
    LONG_PTR exStyle = GetWindowLongPtrW(g_overlay, GWL_EXSTYLE);
    if (g_clickThrough) {
        exStyle |= WS_EX_TRANSPARENT;
    }
    else {
        exStyle &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLongPtrW(g_overlay, GWL_EXSTYLE, exStyle);
}

void PaintOverlay(HWND hwnd)
{
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd, &ps);
    if (!hdc) {
        return;
    }

    RECT client{};
    GetClientRect(hwnd, &client);

    HBRUSH transparentBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &client, transparentBrush);
    DeleteObject(transparentBrush);

    if (g_stats.visible) {
        RECT panel{ 24, 24, 520, 244 };
        HBRUSH panelBrush = CreateSolidBrush(RGB(18, 20, 24));
        FillRect(hdc, &panel, panelBrush);
        DeleteObject(panelBrush);

        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(80, 180, 255));
        HGDIOBJ oldPen = SelectObject(hdc, borderPen);
        HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        Rectangle(hdc, panel.left, panel.top, panel.right, panel.bottom);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);

        HFONT font = CreateFontW(
            -16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
        HGDIOBJ oldFont = SelectObject(hdc, font);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(225, 240, 255));

        ULONGLONG uptimeSec = (GetTickCount64() - g_startTick) / 1000;
        wchar_t text[2048]{};
        swprintf_s(text,
            L"REVERSA BO3 OVERLAY v0.1\r\n"
            L"Renderer: native DXGI proxy   PID: %lu\r\n"
            L"CPU: %.1f%% one-core scale   Threads: %lu   Handles: %lu\r\n"
            L"Memory: WS %zu MB   Private %zu MB\r\n"
            L"Display: %s @ %lu Hz\r\n"
            L"Game rect: %ld,%ld %ldx%ld\r\n"
            L"Uptime: %llu sec\r\n"
            L"F10 overlay  F11 click-through",
            g_stats.processId,
            g_stats.cpuPercent,
            g_stats.threadCount,
            g_stats.handleCount,
            g_stats.workingSetMb,
            g_stats.privateMb,
            g_stats.displayName[0] ? g_stats.displayName : L"unknown",
            g_stats.displayHz,
            g_stats.gameRect.left,
            g_stats.gameRect.top,
            g_stats.gameRect.right - g_stats.gameRect.left,
            g_stats.gameRect.bottom - g_stats.gameRect.top,
            uptimeSec);

        RECT textRect{ 40, 40, 500, 230 };
        DrawTextW(hdc, text, -1, &textRect, DT_LEFT | DT_TOP | DT_NOPREFIX);

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_TIMER:
        if (wParam == kRefreshTimer) {
            if (GetAsyncKeyState(VK_F10) & 1) {
                g_stats.visible = !g_stats.visible;
            }
            if (GetAsyncKeyState(VK_F11) & 1) {
                ToggleClickThrough();
            }
            UpdateWindowPosition();
            UpdateProcessStats();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        PaintOverlay(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, kRefreshTimer);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

DWORD WINAPI OverlayThread(LPVOID)
{
    wchar_t disabled[8]{};
    DWORD disabledChars = GetEnvironmentVariableW(L"REVERSA_OVERLAY_DISABLE", disabled, static_cast<DWORD>(std::size(disabled)));
    if (disabledChars > 0 && disabled[0] != L'0') {
        return 0;
    }

    LogLine(L"overlay thread starting");
    g_startTick = GetTickCount64();
    g_gameWindow = WaitForGameWindow();
    if (!g_gameWindow) {
        LogLine(L"game window not found before timeout; overlay disabled");
        return 0;
    }
    LogLine(L"game window found; creating owned overlay");

    HINSTANCE instance = reinterpret_cast<HINSTANCE>(g_module);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kWindowClassName;

    RegisterClassExW(&wc);

    g_overlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        kWindowClassName,
        L"",
        WS_POPUP,
        0, 0, 640, 360,
        g_gameWindow,
        nullptr,
        instance,
        nullptr);

    if (!g_overlay) {
        LogLine(L"failed to create overlay window");
        return 0;
    }

    SetLayeredWindowAttributes(g_overlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(g_overlay, SW_SHOWNOACTIVATE);
    SetTimer(g_overlay, kRefreshTimer, 250, nullptr);

    UpdateWindowPosition();
    UpdateProcessStats();

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    LogLine(L"overlay thread exiting");
    return 0;
}

} // namespace

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
        HANDLE thread = CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    }
    return TRUE;
}
