#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <cstdint>
#include <cwchar>
#include <new>
#include <string>

#include "../../reversa-shared/reversa_runtime_config.h"
#include "../../reversa-shared/reversa_telemetry.h"

namespace {

constexpr uint32_t kTelemetrySourceD3d11 = 0x2;

HMODULE g_module = nullptr;
HANDLE g_sharedTelemetryMapping = nullptr;
reversa::SharedPresentTelemetry* g_sharedTelemetry = nullptr;
LARGE_INTEGER g_qpcFrequency{};
LONGLONG g_previousPresentQpc = 0;
bool g_t7CompatibilityMode = false;
bool g_d3d11FactoryHooksEnabled = true;
bool g_factoryHookSkipLogged = false;
bool g_directSwapChainWrapSkipLogged = false;

using CreateSwapChainProc = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
using CreateSwapChainForHwndProc = HRESULT(STDMETHODCALLTYPE*)(
    IDXGIFactory2*,
    IUnknown*,
    HWND,
    const DXGI_SWAP_CHAIN_DESC1*,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,
    IDXGIOutput*,
    IDXGISwapChain1**);
using CreateSwapChainForCoreWindowProc = HRESULT(STDMETHODCALLTYPE*)(
    IDXGIFactory2*,
    IUnknown*,
    IUnknown*,
    const DXGI_SWAP_CHAIN_DESC1*,
    IDXGIOutput*,
    IDXGISwapChain1**);
using CreateSwapChainForCompositionProc = HRESULT(STDMETHODCALLTYPE*)(
    IDXGIFactory2*,
    IUnknown*,
    const DXGI_SWAP_CHAIN_DESC1*,
    IDXGIOutput*,
    IDXGISwapChain1**);

CreateSwapChainProc g_originalCreateSwapChain = nullptr;
CreateSwapChainForHwndProc g_originalCreateSwapChainForHwnd = nullptr;
CreateSwapChainForCoreWindowProc g_originalCreateSwapChainForCoreWindow = nullptr;
CreateSwapChainForCompositionProc g_originalCreateSwapChainForComposition = nullptr;

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

HMODULE LoadSystemD3D11()
{
    static HMODULE module = nullptr;
    if (module) {
        return module;
    }

    std::wstring path = GetModuleDirectory() + L"\\d3d11_system.dll";
    module = LoadLibraryW(path.c_str());
    if (!module) {
        LogLine(L"failed to load d3d11_system.dll");
    }
    return module;
}

template <typename T>
T GetSystemD3D11Proc(const char* name)
{
    HMODULE module = LoadSystemD3D11();
    if (!module) {
        return nullptr;
    }
    return reinterpret_cast<T>(GetProcAddress(module, name));
}

void RecordPresent(HRESULT result)
{
    reversa::RecordPresentTelemetry(g_sharedTelemetry, g_qpcFrequency, g_previousPresentQpc, result, kTelemetrySourceD3d11);
}

class SwapChainProxy final : public IDXGISwapChain1 {
public:
    explicit SwapChainProxy(IDXGISwapChain* inner)
        : swapChain_(inner)
    {
        if (swapChain_) {
            swapChain_->AddRef();
            swapChain_->QueryInterface(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(&swapChain1_));
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
            *ppvObject = static_cast<IDXGISwapChain*>(this);
            AddRef();
            return S_OK;
        }

        if (riid == __uuidof(IDXGISwapChain1) && swapChain1_) {
            *ppvObject = static_cast<IDXGISwapChain1*>(this);
            AddRef();
            return S_OK;
        }

        return swapChain_->QueryInterface(riid, ppvObject);
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
    }

    LONG refCount_ = 1;
    IDXGISwapChain* swapChain_ = nullptr;
    IDXGISwapChain1* swapChain1_ = nullptr;
};

void WrapSwapChain(IDXGISwapChain** swapChain)
{
    if (!swapChain || !*swapChain) {
        return;
    }

    IDXGISwapChain* raw = *swapChain;
    auto* proxy = new (std::nothrow) SwapChainProxy(raw);
    if (!proxy || !proxy->IsUsable()) {
        if (proxy) {
            proxy->Release();
        }
        return;
    }

    raw->Release();
    *swapChain = static_cast<IDXGISwapChain*>(proxy);
    LogLine(L"D3D11 swap chain wrapped for present telemetry");
}

void WrapSwapChain1(IDXGISwapChain1** swapChain)
{
    if (!swapChain || !*swapChain) {
        return;
    }

    IDXGISwapChain* baseSwapChain = nullptr;
    if (FAILED((*swapChain)->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&baseSwapChain))) || !baseSwapChain) {
        return;
    }

    IDXGISwapChain* wrappedBase = baseSwapChain;
    WrapSwapChain(&wrappedBase);
    baseSwapChain->Release();

    IDXGISwapChain1* wrappedSwapChain1 = nullptr;
    if (SUCCEEDED(wrappedBase->QueryInterface(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(&wrappedSwapChain1))) && wrappedSwapChain1) {
        (*swapChain)->Release();
        *swapChain = wrappedSwapChain1;
    }
    wrappedBase->Release();
}

bool PatchVtableSlot(IUnknown* object, size_t slot, void* hook, void** original)
{
    if (!object || !hook || !original) {
        return false;
    }

    void*** objectVtable = reinterpret_cast<void***>(object);
    void** vtable = *objectVtable;
    if (!vtable) {
        return false;
    }

    if (vtable[slot] == hook) {
        return true;
    }

    if (*original == nullptr) {
        *original = vtable[slot];
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(&vtable[slot], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    vtable[slot] = hook;
    DWORD ignored = 0;
    VirtualProtect(&vtable[slot], sizeof(void*), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), &vtable[slot], sizeof(void*));
    return true;
}

HRESULT STDMETHODCALLTYPE HookedCreateSwapChain(IDXGIFactory* factory, IUnknown* device, DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** swapChain)
{
    LogLine(L"hooked IDXGIFactory::CreateSwapChain called");
    HRESULT result = g_originalCreateSwapChain
        ? g_originalCreateSwapChain(factory, device, desc, swapChain)
        : E_FAIL;
    if (SUCCEEDED(result)) {
        WrapSwapChain(swapChain);
    }
    return result;
}

HRESULT STDMETHODCALLTYPE HookedCreateSwapChainForHwnd(
    IDXGIFactory2* factory,
    IUnknown* device,
    HWND hwnd,
    const DXGI_SWAP_CHAIN_DESC1* desc,
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc,
    IDXGIOutput* restrictToOutput,
    IDXGISwapChain1** swapChain)
{
    LogLine(L"hooked IDXGIFactory2::CreateSwapChainForHwnd called");
    HRESULT result = g_originalCreateSwapChainForHwnd
        ? g_originalCreateSwapChainForHwnd(factory, device, hwnd, desc, fullscreenDesc, restrictToOutput, swapChain)
        : E_FAIL;
    if (SUCCEEDED(result)) {
        WrapSwapChain1(swapChain);
    }
    return result;
}

HRESULT STDMETHODCALLTYPE HookedCreateSwapChainForCoreWindow(
    IDXGIFactory2* factory,
    IUnknown* device,
    IUnknown* window,
    const DXGI_SWAP_CHAIN_DESC1* desc,
    IDXGIOutput* restrictToOutput,
    IDXGISwapChain1** swapChain)
{
    LogLine(L"hooked IDXGIFactory2::CreateSwapChainForCoreWindow called");
    HRESULT result = g_originalCreateSwapChainForCoreWindow
        ? g_originalCreateSwapChainForCoreWindow(factory, device, window, desc, restrictToOutput, swapChain)
        : E_FAIL;
    if (SUCCEEDED(result)) {
        WrapSwapChain1(swapChain);
    }
    return result;
}

HRESULT STDMETHODCALLTYPE HookedCreateSwapChainForComposition(
    IDXGIFactory2* factory,
    IUnknown* device,
    const DXGI_SWAP_CHAIN_DESC1* desc,
    IDXGIOutput* restrictToOutput,
    IDXGISwapChain1** swapChain)
{
    LogLine(L"hooked IDXGIFactory2::CreateSwapChainForComposition called");
    HRESULT result = g_originalCreateSwapChainForComposition
        ? g_originalCreateSwapChainForComposition(factory, device, desc, restrictToOutput, swapChain)
        : E_FAIL;
    if (SUCCEEDED(result)) {
        WrapSwapChain1(swapChain);
    }
    return result;
}

void InstallFactoryHooks(ID3D11Device* device)
{
    if (!device) {
        return;
    }

    if (!g_d3d11FactoryHooksEnabled) {
        if (!g_factoryHookSkipLogged) {
            g_factoryHookSkipLogged = true;
            LogLine(L"D3D11 factory hooks skipped; T7 compatibility mode is active");
        }
        return;
    }

    IDXGIDevice* dxgiDevice = nullptr;
    IDXGIAdapter* adapter = nullptr;
    IDXGIFactory* factory = nullptr;
    IDXGIFactory2* factory2 = nullptr;

    if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice))) || !dxgiDevice) {
        LogLine(L"D3D11 device did not expose IDXGIDevice");
        return;
    }

    if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter)) && adapter) {
        if (SUCCEEDED(adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory))) && factory) {
            if (PatchVtableSlot(factory, 10, reinterpret_cast<void*>(&HookedCreateSwapChain), reinterpret_cast<void**>(&g_originalCreateSwapChain))) {
                LogLine(L"installed IDXGIFactory::CreateSwapChain hook");
            }
            if (SUCCEEDED(factory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory2))) && factory2) {
                if (PatchVtableSlot(factory2, 15, reinterpret_cast<void*>(&HookedCreateSwapChainForHwnd), reinterpret_cast<void**>(&g_originalCreateSwapChainForHwnd))) {
                    LogLine(L"installed IDXGIFactory2::CreateSwapChainForHwnd hook");
                }
                if (PatchVtableSlot(factory2, 16, reinterpret_cast<void*>(&HookedCreateSwapChainForCoreWindow), reinterpret_cast<void**>(&g_originalCreateSwapChainForCoreWindow))) {
                    LogLine(L"installed IDXGIFactory2::CreateSwapChainForCoreWindow hook");
                }
                if (PatchVtableSlot(factory2, 25, reinterpret_cast<void*>(&HookedCreateSwapChainForComposition), reinterpret_cast<void**>(&g_originalCreateSwapChainForComposition))) {
                    LogLine(L"installed IDXGIFactory2::CreateSwapChainForComposition hook");
                }
            }
        }
    }

    if (factory2) {
        factory2->Release();
    }
    if (factory) {
        factory->Release();
    }
    if (adapter) {
        adapter->Release();
    }
    dxgiDevice->Release();
}

} // namespace

extern "C" HRESULT WINAPI D3D11CreateDevice(
    IDXGIAdapter* adapter,
    D3D_DRIVER_TYPE driverType,
    HMODULE software,
    UINT flags,
    const D3D_FEATURE_LEVEL* featureLevels,
    UINT featureLevelsCount,
    UINT sdkVersion,
    ID3D11Device** device,
    D3D_FEATURE_LEVEL* featureLevel,
    ID3D11DeviceContext** immediateContext)
{
    using Proc = HRESULT(WINAPI*)(
        IDXGIAdapter*,
        D3D_DRIVER_TYPE,
        HMODULE,
        UINT,
        const D3D_FEATURE_LEVEL*,
        UINT,
        UINT,
        ID3D11Device**,
        D3D_FEATURE_LEVEL*,
        ID3D11DeviceContext**);
    Proc proc = GetSystemD3D11Proc<Proc>("D3D11CreateDevice");
    LogLine(L"D3D11CreateDevice called");
    HRESULT result = proc
        ? proc(adapter, driverType, software, flags, featureLevels, featureLevelsCount, sdkVersion, device, featureLevel, immediateContext)
        : HRESULT_FROM_WIN32(GetLastError());
    if (SUCCEEDED(result)) {
        LogLine(L"D3D11CreateDevice succeeded");
        if (device && *device) {
            InstallFactoryHooks(*device);
        }
    }
    return result;
}

extern "C" HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
    IDXGIAdapter* adapter,
    D3D_DRIVER_TYPE driverType,
    HMODULE software,
    UINT flags,
    const D3D_FEATURE_LEVEL* featureLevels,
    UINT featureLevelsCount,
    UINT sdkVersion,
    const DXGI_SWAP_CHAIN_DESC* swapChainDesc,
    IDXGISwapChain** swapChain,
    ID3D11Device** device,
    D3D_FEATURE_LEVEL* featureLevel,
    ID3D11DeviceContext** immediateContext)
{
    using Proc = HRESULT(WINAPI*)(
        IDXGIAdapter*,
        D3D_DRIVER_TYPE,
        HMODULE,
        UINT,
        const D3D_FEATURE_LEVEL*,
        UINT,
        UINT,
        const DXGI_SWAP_CHAIN_DESC*,
        IDXGISwapChain**,
        ID3D11Device**,
        D3D_FEATURE_LEVEL*,
        ID3D11DeviceContext**);
    Proc proc = GetSystemD3D11Proc<Proc>("D3D11CreateDeviceAndSwapChain");
    if (!proc) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LogLine(L"D3D11CreateDeviceAndSwapChain called");
    HRESULT result = proc(
        adapter,
        driverType,
        software,
        flags,
        featureLevels,
        featureLevelsCount,
        sdkVersion,
        swapChainDesc,
        swapChain,
        device,
        featureLevel,
        immediateContext);
    if (SUCCEEDED(result)) {
        if (device && *device) {
            InstallFactoryHooks(*device);
        }
        if (g_d3d11FactoryHooksEnabled) {
            WrapSwapChain(swapChain);
        }
        else if (!g_directSwapChainWrapSkipLogged) {
            g_directSwapChainWrapSkipLogged = true;
            LogLine(L"D3D11 direct swap-chain wrap skipped; T7 compatibility mode is active");
        }
    }
    return result;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
        QueryPerformanceFrequency(&g_qpcFrequency);
        g_sharedTelemetry = reversa::MapSharedTelemetry(&g_sharedTelemetryMapping);
        const std::wstring moduleDirectory = GetModuleDirectory();
        g_t7CompatibilityMode = reversa::ShouldUseT7CompatibilityMode(moduleDirectory.c_str());
        g_d3d11FactoryHooksEnabled = reversa::ShouldEnableD3D11FactoryHooks(moduleDirectory.c_str());

        wchar_t line[256]{};
        swprintf_s(line,
            L"D3D11 proxy loaded; T7 compatibility %s; D3D11 factory hooks %s",
            g_t7CompatibilityMode ? L"on" : L"off",
            g_d3d11FactoryHooksEnabled ? L"enabled" : L"disabled");
        LogLine(line);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        LogLine(L"D3D11 proxy unloading");
        reversa::UnmapSharedTelemetry(g_sharedTelemetry, g_sharedTelemetryMapping);
    }
    return TRUE;
}
