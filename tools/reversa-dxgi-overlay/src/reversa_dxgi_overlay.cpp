#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi1_6.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <cstdint>
#include <cwchar>
#include <iterator>
#include <new>
#include <string>

#include "../../reversa-shared/reversa_runtime_config.h"
#include "../../reversa-shared/reversa_telemetry.h"

namespace {

constexpr wchar_t kWindowClassName[] = L"ReversaOverlayWindow";
constexpr UINT_PTR kRefreshTimer = 1;
constexpr UINT kOverlayIdleTimerMs = 1000;
constexpr UINT kOverlayMenuTimerMs = 100;
constexpr ULONGLONG kOverlayStatsRefreshMs = 1000;
constexpr ULONGLONG kOverlayPaintRefreshMs = 1000;
constexpr DWORD kGameWindowWaitMs = 60000;
constexpr uint32_t kTelemetrySourceDxgi = 0x1;
constexpr int kHotkeyMenu = 1;
constexpr int kHotkeyHud = 2;
constexpr int kHotkeyClickThrough = 3;

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
    bool visible = false;
    bool presentHooked = false;
    ULONGLONG presentCount = 0;
    double presentLastMs = 0.0;
    double presentEmaMs = 0.0;
    HRESULT lastPresentResult = S_OK;
    wchar_t displayName[32]{};
};

struct PresentTelemetry {
    ULONGLONG count = 0;
    LONGLONG previousQpc = 0;
    double lastMs = 0.0;
    double emaMs = 0.0;
    HRESULT lastResult = S_OK;
    bool hooked = false;
};

struct PartnerMenuState {
    bool visible = false;
    int tab = 0;
    int selected = 0;
    bool health = false;
    bool unlimitedAmmo = false;
    bool instantKill = false;
    bool rapidFire = false;
    bool flyHack = false;
    bool weaponHotkeys = false;
    bool camoHotkeys = false;
    bool goldTu8e = false;
    bool svCheats = false;
    bool name = false;
    bool rainbowTag = false;
    bool unlimitedSpirit = false;
    bool caption = false;
    bool modSpoof = false;
    bool galaxy = false;
    bool maxBank = false;
    bool rainbowCamo = false;
    bool rainbowTableCamo = false;
    bool rainbowKnifeCamo = false;
};

HMODULE g_module = nullptr;
HWND g_overlay = nullptr;
HWND g_gameWindow = nullptr;
OverlayStats g_stats{};
CpuSample g_cpuPrevious{};
PresentTelemetry g_presentTelemetry{};
PartnerMenuState g_partnerMenu{};
CRITICAL_SECTION g_presentLock{};
LARGE_INTEGER g_qpcFrequency{};
HANDLE g_sharedTelemetryMapping = nullptr;
reversa::SharedPresentTelemetry* g_sharedTelemetry = nullptr;
LONGLONG g_sharedPreviousQpc = 0;
ULONGLONG g_startTick = 0;
ULONGLONG g_lastOverlayStatsTick = 0;
ULONGLONG g_lastOverlayPaintTick = 0;
bool g_clickThrough = true;
bool g_presentLockReady = false;
bool g_t7CompanionPresent = false;
reversa::BO3RuntimeLane g_runtimeLane = reversa::BO3RuntimeLane::Unknown;

constexpr const wchar_t* kPartnerMenuTabs[] = {
    L"Perf",
    L"Session",
    L"Weapons",
    L"Camos",
    L"Bank",
    L"Bridge"
};

struct MenuToggle {
    const wchar_t* label;
    bool PartnerMenuState::*value;
};

constexpr MenuToggle kSessionToggles[] = {
    { L"Health", &PartnerMenuState::health },
    { L"Unlimited Ammo", &PartnerMenuState::unlimitedAmmo },
    { L"Instant-Kill", &PartnerMenuState::instantKill },
    { L"Rapid-Fire", &PartnerMenuState::rapidFire },
    { L"Fly-Hack", &PartnerMenuState::flyHack },
    { L"GoldTU8E", &PartnerMenuState::goldTu8e },
    { L"Sv_Cheats", &PartnerMenuState::svCheats },
    { L"Unlimited Spirit", &PartnerMenuState::unlimitedSpirit }
};

constexpr MenuToggle kWeaponToggles[] = {
    { L"Weapon-Hotkeys", &PartnerMenuState::weaponHotkeys },
    { L"Name", &PartnerMenuState::name },
    { L"bModSpoof", &PartnerMenuState::modSpoof }
};

constexpr MenuToggle kCamoToggles[] = {
    { L"Camo-Hotkeys", &PartnerMenuState::camoHotkeys },
    { L"Rainbow-Tag", &PartnerMenuState::rainbowTag },
    { L"Galaxy", &PartnerMenuState::galaxy },
    { L"Rainbow Camo", &PartnerMenuState::rainbowCamo },
    { L"Rainbow Table Camo", &PartnerMenuState::rainbowTableCamo },
    { L"Rainbow Knife Camo", &PartnerMenuState::rainbowKnifeCamo },
    { L"bCaption", &PartnerMenuState::caption }
};

constexpr MenuToggle kBankToggles[] = {
    { L"Max Bank", &PartnerMenuState::maxBank }
};

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

HMODULE LoadSystemDxgi()
{
    static HMODULE module = nullptr;
    if (module) {
        return module;
    }

    std::wstring path = GetModuleDirectory() + L"\\dxgi_system.dll";
    module = LoadLibraryW(path.c_str());
    if (!module) {
        LogLine(L"failed to load dxgi_system.dll");
    }
    return module;
}

template <typename T>
T GetSystemDxgiProc(const char* name)
{
    HMODULE module = LoadSystemDxgi();
    if (!module) {
        return nullptr;
    }
    return reinterpret_cast<T>(GetProcAddress(module, name));
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

int CurrentPartnerSelectableCount()
{
    switch (g_partnerMenu.tab) {
    case 1:
        return static_cast<int>(std::size(kSessionToggles));
    case 2:
        return static_cast<int>(std::size(kWeaponToggles));
    case 3:
        return static_cast<int>(std::size(kCamoToggles));
    case 4:
        return static_cast<int>(std::size(kBankToggles));
    default:
        return 0;
    }
}

void ClampPartnerSelection()
{
    const int count = CurrentPartnerSelectableCount();
    if (count <= 0) {
        g_partnerMenu.selected = 0;
        return;
    }
    if (g_partnerMenu.selected < 0) {
        g_partnerMenu.selected = count - 1;
    }
    else if (g_partnerMenu.selected >= count) {
        g_partnerMenu.selected = 0;
    }
}

template <size_t Count>
bool TogglePartnerItem(const MenuToggle (&items)[Count], int index)
{
    if (index < 0 || index >= static_cast<int>(Count)) {
        return false;
    }
    bool PartnerMenuState::*value = items[index].value;
    g_partnerMenu.*value = !(g_partnerMenu.*value);
    return true;
}

bool ToggleSelectedPartnerItem()
{
    switch (g_partnerMenu.tab) {
    case 1:
        return TogglePartnerItem(kSessionToggles, g_partnerMenu.selected);
    case 2:
        return TogglePartnerItem(kWeaponToggles, g_partnerMenu.selected);
    case 3:
        return TogglePartnerItem(kCamoToggles, g_partnerMenu.selected);
    case 4:
        return TogglePartnerItem(kBankToggles, g_partnerMenu.selected);
    default:
        return false;
    }
}

bool HasOverlayContent()
{
    return g_stats.visible || g_partnerMenu.visible;
}

bool HandlePartnerMenuHotkeys()
{
    bool changed = false;
    if (!g_partnerMenu.visible) {
        return changed;
    }

    if (GetAsyncKeyState(VK_RIGHT) & 1) {
        g_partnerMenu.tab = (g_partnerMenu.tab + 1) % static_cast<int>(std::size(kPartnerMenuTabs));
        g_partnerMenu.selected = 0;
        changed = true;
    }
    if (GetAsyncKeyState(VK_LEFT) & 1) {
        g_partnerMenu.tab = (g_partnerMenu.tab + static_cast<int>(std::size(kPartnerMenuTabs)) - 1) %
            static_cast<int>(std::size(kPartnerMenuTabs));
        g_partnerMenu.selected = 0;
        changed = true;
    }
    if (GetAsyncKeyState(VK_DOWN) & 1) {
        ++g_partnerMenu.selected;
        ClampPartnerSelection();
        changed = true;
    }
    if (GetAsyncKeyState(VK_UP) & 1) {
        --g_partnerMenu.selected;
        ClampPartnerSelection();
        changed = true;
    }
    if (GetAsyncKeyState(VK_SPACE) & 1) {
        changed = ToggleSelectedPartnerItem() || changed;
    }
    return changed;
}

void DrawTextAt(HDC hdc, int x, int y, COLORREF color, const wchar_t* text)
{
    SetTextColor(hdc, RGB(4, 4, 4));
    TextOutW(hdc, x + 1, y + 1, text, static_cast<int>(wcslen(text)));
    TextOutW(hdc, x - 1, y + 1, text, static_cast<int>(wcslen(text)));
    TextOutW(hdc, x + 1, y - 1, text, static_cast<int>(wcslen(text)));
    TextOutW(hdc, x - 1, y - 1, text, static_cast<int>(wcslen(text)));
    SetTextColor(hdc, color);
    TextOutW(hdc, x, y, text, static_cast<int>(wcslen(text)));
}

void DrawTextBlock(HDC hdc, const RECT& rect, COLORREF color, const wchar_t* text)
{
    RECT shadow = rect;
    SetTextColor(hdc, RGB(4, 4, 4));
    OffsetRect(&shadow, 1, 1);
    DrawTextW(hdc, text, -1, &shadow, DT_LEFT | DT_TOP | DT_NOPREFIX);
    shadow = rect;
    OffsetRect(&shadow, -1, 1);
    DrawTextW(hdc, text, -1, &shadow, DT_LEFT | DT_TOP | DT_NOPREFIX);
    shadow = rect;
    OffsetRect(&shadow, 1, -1);
    DrawTextW(hdc, text, -1, &shadow, DT_LEFT | DT_TOP | DT_NOPREFIX);
    shadow = rect;
    OffsetRect(&shadow, -1, -1);
    DrawTextW(hdc, text, -1, &shadow, DT_LEFT | DT_TOP | DT_NOPREFIX);

    RECT foreground = rect;
    SetTextColor(hdc, color);
    DrawTextW(hdc, text, -1, &foreground, DT_LEFT | DT_TOP | DT_NOPREFIX);
}

template <size_t Count>
int DrawToggleList(HDC hdc, const MenuToggle (&items)[Count], int x, int y)
{
    wchar_t line[256]{};
    for (size_t i = 0; i < Count; ++i) {
        const bool enabled = g_partnerMenu.*(items[i].value);
        swprintf_s(line, L"%lc [%lc] %s",
            static_cast<int>(i) == g_partnerMenu.selected ? L'>' : L' ',
            enabled ? L'x' : L' ',
            items[i].label);
        DrawTextAt(hdc, x, y + static_cast<int>(i) * 20,
            static_cast<int>(i) == g_partnerMenu.selected ? RGB(255, 230, 120) : RGB(225, 240, 255),
            line);
    }
    return y + static_cast<int>(Count) * 20;
}

void DrawPartnerMenu(HDC hdc, const RECT& client)
{
    if (!g_partnerMenu.visible) {
        return;
    }

    const int panelWidth = 760;
    const int left = client.right > panelWidth + 720 ? client.right - panelWidth - 24 : 700;
    RECT panel{ left, 16, left + panelWidth, 300 };

    DrawTextAt(hdc, panel.left + 16, panel.top + 14, RGB(120, 210, 255), L"REVERSA PARTNER MENU v0.1");
    DrawTextAt(hdc, panel.left + 16, panel.top + 36, RGB(180, 190, 205), L"F9 menu | Arrows navigate | Space arms");

    int tabX = panel.left + 16;
    for (int i = 0; i < static_cast<int>(std::size(kPartnerMenuTabs)); ++i) {
        wchar_t tabText[64]{};
        swprintf_s(tabText, i == g_partnerMenu.tab ? L"[%s]" : L" %s ", kPartnerMenuTabs[i]);
        DrawTextAt(hdc, tabX, panel.top + 66, i == g_partnerMenu.tab ? RGB(255, 230, 120) : RGB(160, 180, 205), tabText);
        tabX += 96;
    }

    const int bodyX = panel.left + 24;
    int y = panel.top + 102;
    DrawTextAt(hdc, bodyX, y, RGB(255, 120, 120), L"Wrapper mode: local state only; no patch writes.");
    y += 30;

    wchar_t line[256]{};
    switch (g_partnerMenu.tab) {
    case 0:
        swprintf_s(line, L"Present telemetry: %s | Count: %llu", g_stats.presentHooked ? L"hooked" : L"waiting", g_stats.presentCount);
        DrawTextAt(hdc, bodyX, y, RGB(225, 240, 255), line);
        y += 22;
        swprintf_s(line, L"Runtime: %s | T7 companion: %s | D3D11 hook: %s",
            reversa::BO3RuntimeLaneName(g_runtimeLane),
            g_t7CompanionPresent ? L"present" : L"absent",
            g_t7CompanionPresent ? L"disabled by compat" : L"auto");
        DrawTextAt(hdc, bodyX, y, g_t7CompanionPresent ? RGB(120, 230, 170) : RGB(180, 190, 205), line);
        y += 22;
        swprintf_s(line, L"Frame EMA: %.2f ms / %.1f FPS | Last: %.2f ms",
            g_stats.presentEmaMs,
            g_stats.presentEmaMs > 0.0 ? 1000.0 / g_stats.presentEmaMs : 0.0,
            g_stats.presentLastMs);
        DrawTextAt(hdc, bodyX, y, RGB(225, 240, 255), line);
        y += 22;
        swprintf_s(line, L"CPU: %.1f%% one-core | WS: %zu MB | Private: %zu MB",
            g_stats.cpuPercent, g_stats.workingSetMb, g_stats.privateMb);
        DrawTextAt(hdc, bodyX, y, RGB(225, 240, 255), line);
        y += 22;
        swprintf_s(line, L"Display: %s @ %lu Hz", g_stats.displayName[0] ? g_stats.displayName : L"unknown", g_stats.displayHz);
        DrawTextAt(hdc, bodyX, y, RGB(225, 240, 255), line);
        break;
    case 1:
        DrawToggleList(hdc, kSessionToggles, bodyX, y);
        break;
    case 2:
        y = DrawToggleList(hdc, kWeaponToggles, bodyX, y) + 16;
        DrawTextAt(hdc, bodyX, y, RGB(180, 190, 205), L"ConsoleIX mirrors: Primary-Weapon, Secondary-Weapon, Third-Weapon, Score.");
        y += 22;
        DrawTextAt(hdc, bodyX, y, RGB(180, 190, 205), L"Next bridge: guarded command queue shared with the external menu.");
        break;
    case 3:
        y = DrawToggleList(hdc, kCamoToggles, bodyX, y) + 16;
        DrawTextAt(hdc, bodyX, y, RGB(180, 190, 205), L"Camo actions stay external until address validation is centralized.");
        break;
    case 4:
        y = DrawToggleList(hdc, kBankToggles, bodyX, y) + 16;
        DrawTextAt(hdc, bodyX, y, RGB(180, 190, 205), L"Bank guard target: valid map Tranzit/Die Rise plus valid weapon.");
        y += 22;
        DrawTextAt(hdc, bodyX, y, RGB(180, 190, 205), L"Scan order: table/bank pattern first, signature scan fallback second.");
        break;
    case 5:
        DrawTextAt(hdc, bodyX, y, RGB(225, 240, 255), L"Buddy menu source: ConsoleIX.cpp external ImGui overlay.");
        y += 22;
        DrawTextAt(hdc, bodyX, y, RGB(225, 240, 255), L"Wrapper owns: frame timing, overlay, local armed state.");
        y += 22;
        DrawTextAt(hdc, bodyX, y, RGB(225, 240, 255), L"External tool owns: process attach, guarded address resolution, patch writes.");
        y += 22;
        DrawTextAt(hdc, bodyX, y, RGB(180, 190, 205), L"Next: move shared labels/guards into a common header both targets use.");
        break;
    default:
        break;
    }
}

void RecordPresent(HRESULT result)
{
    if (!g_presentLockReady || g_qpcFrequency.QuadPart == 0) {
        return;
    }

    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);

    if (TryEnterCriticalSection(&g_presentLock)) {
        if (g_presentTelemetry.previousQpc != 0) {
            const LONGLONG deltaQpc = now.QuadPart - g_presentTelemetry.previousQpc;
            const double deltaMs = (static_cast<double>(deltaQpc) * 1000.0) / static_cast<double>(g_qpcFrequency.QuadPart);
            if (deltaMs > 0.0 && deltaMs < 10000.0) {
                g_presentTelemetry.lastMs = deltaMs;
                g_presentTelemetry.emaMs = g_presentTelemetry.emaMs == 0.0
                    ? deltaMs
                    : (g_presentTelemetry.emaMs * (1.0 - reversa::kFrameTimeEmaWeight)) + (deltaMs * reversa::kFrameTimeEmaWeight);
            }
        }
        g_presentTelemetry.previousQpc = now.QuadPart;
        ++g_presentTelemetry.count;
        g_presentTelemetry.lastResult = result;
        g_presentTelemetry.hooked = true;
        LeaveCriticalSection(&g_presentLock);
    }

    reversa::RecordPresentTelemetry(g_sharedTelemetry, g_qpcFrequency, g_sharedPreviousQpc, result, kTelemetrySourceDxgi);
}

void UpdatePresentStatsSnapshot()
{
    if (!g_presentLockReady) {
        return;
    }

    if (g_sharedTelemetry &&
        g_sharedTelemetry->magic == reversa::kTelemetryMagic &&
        g_sharedTelemetry->version == reversa::kTelemetryVersion) {
        if (reversa::TryLockTelemetry(g_sharedTelemetry)) {
            g_stats.presentHooked = g_sharedTelemetry->presentCount > 0;
            g_stats.presentCount = g_sharedTelemetry->presentCount;
            g_stats.presentLastMs = g_sharedTelemetry->presentLastMs;
            g_stats.presentEmaMs = g_sharedTelemetry->presentEmaMs;
            g_stats.lastPresentResult = g_sharedTelemetry->lastPresentResult;
            reversa::UnlockTelemetry(g_sharedTelemetry);
        }
        return;
    }

    if (!TryEnterCriticalSection(&g_presentLock)) {
        return;
    }
    g_stats.presentHooked = g_presentTelemetry.hooked;
    g_stats.presentCount = g_presentTelemetry.count;
    g_stats.presentLastMs = g_presentTelemetry.lastMs;
    g_stats.presentEmaMs = g_presentTelemetry.emaMs;
    g_stats.lastPresentResult = g_presentTelemetry.lastResult;
    LeaveCriticalSection(&g_presentLock);
}

template <typename Interface>
bool IsSameComIdentity(void* candidate, Interface* expected)
{
    if (!candidate || !expected) {
        return false;
    }

    IUnknown* candidateUnknown = nullptr;
    IUnknown* expectedUnknown = nullptr;
    reinterpret_cast<IUnknown*>(candidate)->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&candidateUnknown));
    expected->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&expectedUnknown));

    const bool same = candidateUnknown && expectedUnknown && candidateUnknown == expectedUnknown;
    if (candidateUnknown) {
        candidateUnknown->Release();
    }
    if (expectedUnknown) {
        expectedUnknown->Release();
    }
    return same;
}

class SwapChainProxy final : public IDXGISwapChain1 {
public:
    explicit SwapChainProxy(IUnknown* inner)
        : inner_(inner)
    {
        if (inner_) {
            inner_->AddRef();
            inner_->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&swapChain_));
            inner_->QueryInterface(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(&swapChain1_));
        }
    }

    SwapChainProxy(const SwapChainProxy&) = delete;
    SwapChainProxy& operator=(const SwapChainProxy&) = delete;

    bool IsUsable() const
    {
        return swapChain_ != nullptr;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (!ppvObject) {
            return E_POINTER;
        }
        *ppvObject = nullptr;

        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGIObject) ||
            riid == __uuidof(IDXGIDeviceSubObject) || riid == __uuidof(IDXGISwapChain)) {
            if (!swapChain_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGISwapChain*>(this);
            AddRef();
            return S_OK;
        }

        if (riid == __uuidof(IDXGISwapChain1)) {
            if (!swapChain1_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGISwapChain1*>(this);
            AddRef();
            return S_OK;
        }

        return inner_ ? inner_->QueryInterface(riid, ppvObject) : E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return static_cast<ULONG>(InterlockedIncrement(&refCount_));
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG count = static_cast<ULONG>(InterlockedDecrement(&refCount_));
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID name, UINT dataSize, const void* data) override
    {
        return swapChain_->SetPrivateData(name, dataSize, data);
    }

    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID name, const IUnknown* unknown) override
    {
        return swapChain_->SetPrivateDataInterface(name, unknown);
    }

    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID name, UINT* dataSize, void* data) override
    {
        return swapChain_->GetPrivateData(name, dataSize, data);
    }

    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** parent) override
    {
        return swapChain_->GetParent(riid, parent);
    }

    HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** device) override
    {
        return swapChain_->GetDevice(riid, device);
    }

    HRESULT STDMETHODCALLTYPE Present(UINT syncInterval, UINT flags) override
    {
        const HRESULT result = swapChain_->Present(syncInterval, flags);
        RecordPresent(result);
        return result;
    }

    HRESULT STDMETHODCALLTYPE GetBuffer(UINT buffer, REFIID riid, void** surface) override
    {
        return swapChain_->GetBuffer(buffer, riid, surface);
    }

    HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL fullscreen, IDXGIOutput* target) override
    {
        return swapChain_->SetFullscreenState(fullscreen, target);
    }

    HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL* fullscreen, IDXGIOutput** target) override
    {
        return swapChain_->GetFullscreenState(fullscreen, target);
    }

    HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC* desc) override
    {
        return swapChain_->GetDesc(desc);
    }

    HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat, UINT swapChainFlags) override
    {
        return swapChain_->ResizeBuffers(bufferCount, width, height, newFormat, swapChainFlags);
    }

    HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC* newTargetParameters) override
    {
        return swapChain_->ResizeTarget(newTargetParameters);
    }

    HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput** output) override
    {
        return swapChain_->GetContainingOutput(output);
    }

    HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS* stats) override
    {
        return swapChain_->GetFrameStatistics(stats);
    }

    HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT* lastPresentCount) override
    {
        return swapChain_->GetLastPresentCount(lastPresentCount);
    }

    HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1* desc) override
    {
        return swapChain1_ ? swapChain1_->GetDesc1(desc) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* desc) override
    {
        return swapChain1_ ? swapChain1_->GetFullscreenDesc(desc) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetHwnd(HWND* hwnd) override
    {
        return swapChain1_ ? swapChain1_->GetHwnd(hwnd) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID riid, void** unknown) override
    {
        return swapChain1_ ? swapChain1_->GetCoreWindow(riid, unknown) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE Present1(UINT syncInterval, UINT presentFlags, const DXGI_PRESENT_PARAMETERS* presentParameters) override
    {
        if (!swapChain1_) {
            return E_NOINTERFACE;
        }
        const HRESULT result = swapChain1_->Present1(syncInterval, presentFlags, presentParameters);
        RecordPresent(result);
        return result;
    }

    BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() override
    {
        return swapChain1_ ? swapChain1_->IsTemporaryMonoSupported() : FALSE;
    }

    HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput** restrictToOutput) override
    {
        return swapChain1_ ? swapChain1_->GetRestrictToOutput(restrictToOutput) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA* color) override
    {
        return swapChain1_ ? swapChain1_->SetBackgroundColor(color) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA* color) override
    {
        return swapChain1_ ? swapChain1_->GetBackgroundColor(color) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION rotation) override
    {
        return swapChain1_ ? swapChain1_->SetRotation(rotation) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION* rotation) override
    {
        return swapChain1_ ? swapChain1_->GetRotation(rotation) : E_NOINTERFACE;
    }

private:
    ~SwapChainProxy()
    {
        if (swapChain1_) {
            swapChain1_->Release();
        }
        if (swapChain_) {
            swapChain_->Release();
        }
        if (inner_) {
            inner_->Release();
        }
    }

    LONG refCount_ = 1;
    IUnknown* inner_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    IDXGISwapChain1* swapChain1_ = nullptr;
};

void WrapSwapChain(REFIID riid, void** ppv)
{
    if (!ppv || !*ppv) {
        return;
    }

    IUnknown* raw = static_cast<IUnknown*>(*ppv);
    SwapChainProxy* proxy = new (std::nothrow) SwapChainProxy(raw);
    if (!proxy || !proxy->IsUsable()) {
        if (proxy) {
            proxy->Release();
        }
        return;
    }

    void* wrapped = nullptr;
    const HRESULT queryResult = proxy->QueryInterface(riid, &wrapped);
    const bool wrappedIsProxy = SUCCEEDED(queryResult) && IsSameComIdentity(wrapped, static_cast<IDXGISwapChain1*>(proxy));
    proxy->Release();
    if (SUCCEEDED(queryResult) && wrapped && wrappedIsProxy) {
        raw->Release();
        *ppv = wrapped;
        LogLine(L"swap chain wrapped for present telemetry");
    }
    else if (wrapped) {
        reinterpret_cast<IUnknown*>(wrapped)->Release();
    }
}

class FactoryProxy final : public IDXGIFactory7 {
public:
    explicit FactoryProxy(IUnknown* inner)
        : inner_(inner)
    {
        if (inner_) {
            inner_->AddRef();
            inner_->QueryInterface(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory_));
            inner_->QueryInterface(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory1_));
            inner_->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory2_));
            inner_->QueryInterface(__uuidof(IDXGIFactory3), reinterpret_cast<void**>(&factory3_));
            inner_->QueryInterface(__uuidof(IDXGIFactory4), reinterpret_cast<void**>(&factory4_));
            inner_->QueryInterface(__uuidof(IDXGIFactory5), reinterpret_cast<void**>(&factory5_));
            inner_->QueryInterface(__uuidof(IDXGIFactory6), reinterpret_cast<void**>(&factory6_));
            inner_->QueryInterface(__uuidof(IDXGIFactory7), reinterpret_cast<void**>(&factory7_));
        }
    }

    FactoryProxy(const FactoryProxy&) = delete;
    FactoryProxy& operator=(const FactoryProxy&) = delete;

    bool IsUsable() const
    {
        return factory_ != nullptr;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (!ppvObject) {
            return E_POINTER;
        }
        *ppvObject = nullptr;

        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGIObject) || riid == __uuidof(IDXGIFactory)) {
            if (!factory_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGIFactory*>(this);
            AddRef();
            return S_OK;
        }

        if (riid == __uuidof(IDXGIFactory1)) {
            if (!factory1_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGIFactory1*>(this);
            AddRef();
            return S_OK;
        }

        if (riid == __uuidof(IDXGIFactory2)) {
            if (!factory2_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGIFactory2*>(this);
            AddRef();
            return S_OK;
        }

        if (riid == __uuidof(IDXGIFactory3)) {
            if (!factory3_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGIFactory3*>(this);
            AddRef();
            return S_OK;
        }

        if (riid == __uuidof(IDXGIFactory4)) {
            if (!factory4_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGIFactory4*>(this);
            AddRef();
            return S_OK;
        }

        if (riid == __uuidof(IDXGIFactory5)) {
            if (!factory5_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGIFactory5*>(this);
            AddRef();
            return S_OK;
        }

        if (riid == __uuidof(IDXGIFactory6)) {
            if (!factory6_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGIFactory6*>(this);
            AddRef();
            return S_OK;
        }

        if (riid == __uuidof(IDXGIFactory7)) {
            if (!factory7_) {
                return E_NOINTERFACE;
            }
            *ppvObject = static_cast<IDXGIFactory7*>(this);
            AddRef();
            return S_OK;
        }

        return inner_ ? inner_->QueryInterface(riid, ppvObject) : E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return static_cast<ULONG>(InterlockedIncrement(&refCount_));
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG count = static_cast<ULONG>(InterlockedDecrement(&refCount_));
        if (count == 0) {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID name, UINT dataSize, const void* data) override
    {
        return factory_->SetPrivateData(name, dataSize, data);
    }

    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID name, const IUnknown* unknown) override
    {
        return factory_->SetPrivateDataInterface(name, unknown);
    }

    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID name, UINT* dataSize, void* data) override
    {
        return factory_->GetPrivateData(name, dataSize, data);
    }

    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** parent) override
    {
        return factory_->GetParent(riid, parent);
    }

    HRESULT STDMETHODCALLTYPE EnumAdapters(UINT adapter, IDXGIAdapter** adapterOut) override
    {
        return factory_->EnumAdapters(adapter, adapterOut);
    }

    HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND windowHandle, UINT flags) override
    {
        return factory_->MakeWindowAssociation(windowHandle, flags);
    }

    HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND* windowHandle) override
    {
        return factory_->GetWindowAssociation(windowHandle);
    }

    HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown* device, DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** swapChain) override
    {
        LogLine(L"IDXGIFactory::CreateSwapChain called");
        const HRESULT result = factory_->CreateSwapChain(device, desc, swapChain);
        if (SUCCEEDED(result)) {
            WrapSwapChain(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(swapChain));
        }
        return result;
    }

    HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE module, IDXGIAdapter** adapter) override
    {
        return factory_->CreateSoftwareAdapter(module, adapter);
    }

    HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT adapter, IDXGIAdapter1** adapterOut) override
    {
        return factory1_ ? factory1_->EnumAdapters1(adapter, adapterOut) : E_NOINTERFACE;
    }

    BOOL STDMETHODCALLTYPE IsCurrent() override
    {
        return factory1_ ? factory1_->IsCurrent() : FALSE;
    }

    BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled() override
    {
        return factory2_ ? factory2_->IsWindowedStereoEnabled() : FALSE;
    }

    HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(
        IUnknown* device,
        HWND hwnd,
        const DXGI_SWAP_CHAIN_DESC1* desc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc,
        IDXGIOutput* restrictToOutput,
        IDXGISwapChain1** swapChain) override
    {
        if (!factory2_) {
            return E_NOINTERFACE;
        }
        LogLine(L"IDXGIFactory2::CreateSwapChainForHwnd called");
        const HRESULT result = factory2_->CreateSwapChainForHwnd(device, hwnd, desc, fullscreenDesc, restrictToOutput, swapChain);
        if (SUCCEEDED(result)) {
            WrapSwapChain(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(swapChain));
        }
        return result;
    }

    HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(
        IUnknown* device,
        IUnknown* window,
        const DXGI_SWAP_CHAIN_DESC1* desc,
        IDXGIOutput* restrictToOutput,
        IDXGISwapChain1** swapChain) override
    {
        if (!factory2_) {
            return E_NOINTERFACE;
        }
        LogLine(L"IDXGIFactory2::CreateSwapChainForCoreWindow called");
        const HRESULT result = factory2_->CreateSwapChainForCoreWindow(device, window, desc, restrictToOutput, swapChain);
        if (SUCCEEDED(result)) {
            WrapSwapChain(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(swapChain));
        }
        return result;
    }

    HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(HANDLE resource, LUID* luid) override
    {
        return factory2_ ? factory2_->GetSharedResourceAdapterLuid(resource, luid) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(HWND windowHandle, UINT message, DWORD* cookie) override
    {
        return factory2_ ? factory2_->RegisterStereoStatusWindow(windowHandle, message, cookie) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(HANDLE eventHandle, DWORD* cookie) override
    {
        return factory2_ ? factory2_->RegisterStereoStatusEvent(eventHandle, cookie) : E_NOINTERFACE;
    }

    void STDMETHODCALLTYPE UnregisterStereoStatus(DWORD cookie) override
    {
        if (factory2_) {
            factory2_->UnregisterStereoStatus(cookie);
        }
    }

    HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(HWND windowHandle, UINT message, DWORD* cookie) override
    {
        return factory2_ ? factory2_->RegisterOcclusionStatusWindow(windowHandle, message, cookie) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(HANDLE eventHandle, DWORD* cookie) override
    {
        return factory2_ ? factory2_->RegisterOcclusionStatusEvent(eventHandle, cookie) : E_NOINTERFACE;
    }

    void STDMETHODCALLTYPE UnregisterOcclusionStatus(DWORD cookie) override
    {
        if (factory2_) {
            factory2_->UnregisterOcclusionStatus(cookie);
        }
    }

    HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(
        IUnknown* device,
        const DXGI_SWAP_CHAIN_DESC1* desc,
        IDXGIOutput* restrictToOutput,
        IDXGISwapChain1** swapChain) override
    {
        if (!factory2_) {
            return E_NOINTERFACE;
        }
        LogLine(L"IDXGIFactory2::CreateSwapChainForComposition called");
        const HRESULT result = factory2_->CreateSwapChainForComposition(device, desc, restrictToOutput, swapChain);
        if (SUCCEEDED(result)) {
            WrapSwapChain(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(swapChain));
        }
        return result;
    }

    UINT STDMETHODCALLTYPE GetCreationFlags() override
    {
        return factory3_ ? factory3_->GetCreationFlags() : 0;
    }

    HRESULT STDMETHODCALLTYPE EnumAdapterByLuid(LUID adapterLuid, REFIID riid, void** adapter) override
    {
        return factory4_ ? factory4_->EnumAdapterByLuid(adapterLuid, riid, adapter) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE EnumWarpAdapter(REFIID riid, void** adapter) override
    {
        return factory4_ ? factory4_->EnumWarpAdapter(riid, adapter) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE CheckFeatureSupport(DXGI_FEATURE feature, void* featureSupportData, UINT featureSupportDataSize) override
    {
        return factory5_ ? factory5_->CheckFeatureSupport(feature, featureSupportData, featureSupportDataSize) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE EnumAdapterByGpuPreference(
        UINT adapter,
        DXGI_GPU_PREFERENCE gpuPreference,
        REFIID riid,
        void** adapterOut) override
    {
        return factory6_ ? factory6_->EnumAdapterByGpuPreference(adapter, gpuPreference, riid, adapterOut) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE RegisterAdaptersChangedEvent(HANDLE eventHandle, DWORD* cookie) override
    {
        return factory7_ ? factory7_->RegisterAdaptersChangedEvent(eventHandle, cookie) : E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE UnregisterAdaptersChangedEvent(DWORD cookie) override
    {
        return factory7_ ? factory7_->UnregisterAdaptersChangedEvent(cookie) : E_NOINTERFACE;
    }

private:
    ~FactoryProxy()
    {
        if (factory7_) {
            factory7_->Release();
        }
        if (factory6_) {
            factory6_->Release();
        }
        if (factory5_) {
            factory5_->Release();
        }
        if (factory4_) {
            factory4_->Release();
        }
        if (factory3_) {
            factory3_->Release();
        }
        if (factory2_) {
            factory2_->Release();
        }
        if (factory1_) {
            factory1_->Release();
        }
        if (factory_) {
            factory_->Release();
        }
        if (inner_) {
            inner_->Release();
        }
    }

    LONG refCount_ = 1;
    IUnknown* inner_ = nullptr;
    IDXGIFactory* factory_ = nullptr;
    IDXGIFactory1* factory1_ = nullptr;
    IDXGIFactory2* factory2_ = nullptr;
    IDXGIFactory3* factory3_ = nullptr;
    IDXGIFactory4* factory4_ = nullptr;
    IDXGIFactory5* factory5_ = nullptr;
    IDXGIFactory6* factory6_ = nullptr;
    IDXGIFactory7* factory7_ = nullptr;
};

void WrapFactory(REFIID riid, void** ppv)
{
    if (!ppv || !*ppv) {
        return;
    }

    IUnknown* raw = static_cast<IUnknown*>(*ppv);
    FactoryProxy* proxy = new (std::nothrow) FactoryProxy(raw);
    if (!proxy || !proxy->IsUsable()) {
        if (proxy) {
            proxy->Release();
        }
        return;
    }

    void* wrapped = nullptr;
    const HRESULT queryResult = proxy->QueryInterface(riid, &wrapped);
    const bool wrappedIsProxy = SUCCEEDED(queryResult) && IsSameComIdentity(wrapped, static_cast<IDXGIFactory7*>(proxy));
    proxy->Release();
    if (SUCCEEDED(queryResult) && wrapped && wrappedIsProxy) {
        raw->Release();
        *ppv = wrapped;
        LogLine(L"DXGI factory wrapped for swap-chain telemetry");
    }
    else if (wrapped) {
        reinterpret_cast<IUnknown*>(wrapped)->Release();
    }
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
    UpdatePresentStatsSnapshot();
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

void UpdateOverlayTimer()
{
    if (!g_overlay) {
        return;
    }

    KillTimer(g_overlay, kRefreshTimer);
    if (HasOverlayContent()) {
        SetTimer(g_overlay, kRefreshTimer, g_partnerMenu.visible ? kOverlayMenuTimerMs : kOverlayIdleTimerMs, nullptr);
    }
}

void RefreshOverlayVisibility(bool forceStats)
{
    if (!g_overlay) {
        return;
    }

    if (!HasOverlayContent()) {
        ShowWindow(g_overlay, SW_HIDE);
        UpdateOverlayTimer();
        return;
    }

    UpdateWindowPosition();
    if (forceStats) {
        UpdateProcessStats();
        g_lastOverlayStatsTick = GetTickCount64();
    }
    ShowWindow(g_overlay, SW_SHOWNOACTIVATE);
    InvalidateRect(g_overlay, nullptr, FALSE);
    UpdateOverlayTimer();
}

void TogglePartnerMenu()
{
    g_partnerMenu.visible = !g_partnerMenu.visible;
    if (g_partnerMenu.visible) {
        ClampPartnerSelection();
    }
    RefreshOverlayVisibility(true);
}

void ToggleHud()
{
    g_stats.visible = !g_stats.visible;
    RefreshOverlayVisibility(true);
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
            L"REVERSA HUD v0.8 | Runtime %s | T7 %s | PID %lu | Up %llus | F9 menu | F10 hud | F11 pass\r\n"
            L"Present %s | #%llu | EMA %.2f ms / %.1f FPS | Last %.2f ms | HR 0x%08lX\r\n"
            L"CPU %.1f%% | Threads %lu | Handles %lu | WS %zu MB | Private %zu MB\r\n"
            L"Display %s @ %lu Hz | Game %ld,%ld %ldx%ld",
            reversa::BO3RuntimeLaneName(g_runtimeLane),
            g_t7CompanionPresent ? L"present" : L"absent",
            g_stats.processId,
            uptimeSec,
            g_stats.presentHooked ? L"hooked" : L"waiting",
            g_stats.presentCount,
            g_stats.presentEmaMs,
            g_stats.presentEmaMs > 0.0 ? 1000.0 / g_stats.presentEmaMs : 0.0,
            g_stats.presentLastMs,
            static_cast<unsigned long>(g_stats.lastPresentResult),
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
            g_stats.gameRect.bottom - g_stats.gameRect.top);

        RECT textRect{ 24, 16, client.right > 780 ? 780 : client.right - 24, 118 };
        DrawTextBlock(hdc, textRect, RGB(235, 245, 255), text);

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    HFONT menuFont = CreateFontW(
        -16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    HGDIOBJ oldMenuFont = SelectObject(hdc, menuFont);
    SetBkMode(hdc, TRANSPARENT);
    DrawPartnerMenu(hdc, client);
    SelectObject(hdc, oldMenuFont);
    DeleteObject(menuFont);

    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_HOTKEY:
        if (wParam == kHotkeyMenu) {
            TogglePartnerMenu();
            return 0;
        }
        if (wParam == kHotkeyHud) {
            ToggleHud();
            return 0;
        }
        if (wParam == kHotkeyClickThrough) {
            ToggleClickThrough();
            if (HasOverlayContent()) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        break;
    case WM_TIMER:
        if (wParam == kRefreshTimer) {
            bool shouldPaint = false;
            if (!HasOverlayContent()) {
                UpdateOverlayTimer();
                return 0;
            }

            if (g_partnerMenu.visible) {
                shouldPaint = HandlePartnerMenuHotkeys() || shouldPaint;
            }

            const ULONGLONG nowMs = GetTickCount64();
            if (g_lastOverlayStatsTick == 0 || nowMs - g_lastOverlayStatsTick >= kOverlayStatsRefreshMs) {
                UpdateWindowPosition();
                UpdateProcessStats();
                g_lastOverlayStatsTick = nowMs;
                shouldPaint = true;
            }
            else if (g_lastOverlayPaintTick == 0 || nowMs - g_lastOverlayPaintTick >= kOverlayPaintRefreshMs) {
                UpdatePresentStatsSnapshot();
                shouldPaint = HasOverlayContent();
            }

            if (shouldPaint) {
                g_lastOverlayPaintTick = nowMs;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
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
        UnregisterHotKey(hwnd, kHotkeyMenu);
        UnregisterHotKey(hwnd, kHotkeyHud);
        UnregisterHotKey(hwnd, kHotkeyClickThrough);
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

DWORD WINAPI OverlayThread(LPVOID)
{
    bool disabled = false;
    if (reversa::ReadEnvironmentBool(L"REVERSA_OVERLAY_DISABLE", disabled) && disabled) {
        LogLine(L"overlay window disabled by REVERSA_OVERLAY_DISABLE");
        return 0;
    }

    const std::wstring moduleDirectory = GetModuleDirectory();
    if (reversa::FileExistsInDirectory(moduleDirectory.c_str(), L"reversa-overlay.disable")) {
        LogLine(L"overlay window disabled by reversa-overlay.disable");
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
    ShowWindow(g_overlay, SW_HIDE);

    RegisterHotKey(g_overlay, kHotkeyMenu, MOD_NOREPEAT, VK_F9);
    RegisterHotKey(g_overlay, kHotkeyHud, MOD_NOREPEAT, VK_F10);
    RegisterHotKey(g_overlay, kHotkeyClickThrough, MOD_NOREPEAT, VK_F11);

    g_lastOverlayStatsTick = GetTickCount64();
    g_lastOverlayPaintTick = g_lastOverlayStatsTick;
    RefreshOverlayVisibility(true);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    LogLine(L"overlay thread exiting");
    return 0;
}

} // namespace

extern "C" HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** factory)
{
    using CreateDXGIFactoryProc = HRESULT(WINAPI*)(REFIID, void**);
    CreateDXGIFactoryProc proc = GetSystemDxgiProc<CreateDXGIFactoryProc>("CreateDXGIFactory");
    if (!proc) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    const HRESULT result = proc(riid, factory);
    if (SUCCEEDED(result)) {
        WrapFactory(riid, factory);
    }
    return result;
}

extern "C" HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** factory)
{
    using CreateDXGIFactory1Proc = HRESULT(WINAPI*)(REFIID, void**);
    CreateDXGIFactory1Proc proc = GetSystemDxgiProc<CreateDXGIFactory1Proc>("CreateDXGIFactory1");
    if (!proc) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    const HRESULT result = proc(riid, factory);
    if (SUCCEEDED(result)) {
        WrapFactory(riid, factory);
    }
    return result;
}

extern "C" HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID riid, void** factory)
{
    using CreateDXGIFactory2Proc = HRESULT(WINAPI*)(UINT, REFIID, void**);
    CreateDXGIFactory2Proc proc = GetSystemDxgiProc<CreateDXGIFactory2Proc>("CreateDXGIFactory2");
    if (!proc) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    const HRESULT result = proc(flags, riid, factory);
    if (SUCCEEDED(result)) {
        WrapFactory(riid, factory);
    }
    return result;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        InitializeCriticalSection(&g_presentLock);
        g_presentLockReady = true;
        QueryPerformanceFrequency(&g_qpcFrequency);
        g_sharedTelemetry = reversa::MapSharedTelemetry(&g_sharedTelemetryMapping);
        DisableThreadLibraryCalls(instance);
        const std::wstring moduleDirectory = GetModuleDirectory();
        g_t7CompanionPresent = reversa::IsT7CompanionPresent(moduleDirectory.c_str());
        g_runtimeLane = reversa::DetectBO3RuntimeLane(moduleDirectory.c_str());

        wchar_t line[256]{};
        swprintf_s(line,
            L"DXGI proxy loaded; runtime %s; T7 companion %s",
            reversa::BO3RuntimeLaneName(g_runtimeLane),
            g_t7CompanionPresent ? L"detected" : L"not detected");
        LogLine(line);
        HANDLE thread = CreateThread(nullptr, 0, OverlayThread, nullptr, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    }
    else if (reason == DLL_PROCESS_DETACH) {
        if (g_presentLockReady) {
            g_presentLockReady = false;
            DeleteCriticalSection(&g_presentLock);
        }
        reversa::UnmapSharedTelemetry(g_sharedTelemetry, g_sharedTelemetryMapping);
    }
    return TRUE;
}
