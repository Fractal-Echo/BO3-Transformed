#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <cwchar>

namespace reversa {

constexpr uint32_t kTelemetryMagic = 0x52565341; // RVSA
constexpr uint32_t kTelemetryVersion = 1;
constexpr double kFrameTimeEmaWeight = 0.10;

struct SharedPresentTelemetry {
    uint32_t magic = kTelemetryMagic;
    uint32_t version = kTelemetryVersion;
    volatile LONG lock = 0;
    uint64_t presentCount = 0;
    double presentLastMs = 0.0;
    double presentEmaMs = 0.0;
    HRESULT lastPresentResult = S_OK;
    uint32_t sourceFlags = 0;
};

inline void TelemetryNameForCurrentProcess(wchar_t* buffer, size_t count)
{
    swprintf_s(buffer, count, L"Local\\ReversaBO3Telemetry_%lu", GetCurrentProcessId());
}

inline SharedPresentTelemetry* MapSharedTelemetry(HANDLE* mappingHandle)
{
    wchar_t name[96]{};
    TelemetryNameForCurrentProcess(name, std::size(name));

    HANDLE mapping = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        static_cast<DWORD>(sizeof(SharedPresentTelemetry)),
        name);
    if (!mapping) {
        return nullptr;
    }

    void* view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedPresentTelemetry));
    if (!view) {
        CloseHandle(mapping);
        return nullptr;
    }

    auto* telemetry = static_cast<SharedPresentTelemetry*>(view);
    if (telemetry->magic != kTelemetryMagic || telemetry->version != kTelemetryVersion) {
        *telemetry = SharedPresentTelemetry{};
    }

    if (mappingHandle) {
        *mappingHandle = mapping;
    }
    else {
        CloseHandle(mapping);
    }
    return telemetry;
}

inline void UnmapSharedTelemetry(SharedPresentTelemetry*& telemetry, HANDLE& mappingHandle)
{
    if (telemetry) {
        UnmapViewOfFile(telemetry);
        telemetry = nullptr;
    }
    if (mappingHandle) {
        CloseHandle(mappingHandle);
        mappingHandle = nullptr;
    }
}

inline void LockTelemetry(SharedPresentTelemetry* telemetry)
{
    if (!telemetry) {
        return;
    }
    while (InterlockedCompareExchange(&telemetry->lock, 1, 0) != 0) {
        Sleep(0);
    }
}

inline bool TryLockTelemetry(SharedPresentTelemetry* telemetry)
{
    return telemetry && InterlockedCompareExchange(&telemetry->lock, 1, 0) == 0;
}

inline void UnlockTelemetry(SharedPresentTelemetry* telemetry)
{
    if (telemetry) {
        InterlockedExchange(&telemetry->lock, 0);
    }
}

inline void RecordPresentTelemetry(
    SharedPresentTelemetry* telemetry,
    LARGE_INTEGER frequency,
    LONGLONG& previousQpc,
    HRESULT result,
    uint32_t sourceFlag)
{
    if (!telemetry || frequency.QuadPart == 0) {
        return;
    }

    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);

    if (!TryLockTelemetry(telemetry)) {
        previousQpc = now.QuadPart;
        return;
    }

    if (previousQpc != 0) {
        const LONGLONG deltaQpc = now.QuadPart - previousQpc;
        const double deltaMs = (static_cast<double>(deltaQpc) * 1000.0) / static_cast<double>(frequency.QuadPart);
        if (deltaMs > 0.0 && deltaMs < 10000.0) {
            telemetry->presentLastMs = deltaMs;
            telemetry->presentEmaMs = telemetry->presentEmaMs == 0.0
                ? deltaMs
                : (telemetry->presentEmaMs * (1.0 - kFrameTimeEmaWeight)) + (deltaMs * kFrameTimeEmaWeight);
        }
    }
    previousQpc = now.QuadPart;
    ++telemetry->presentCount;
    telemetry->lastPresentResult = result;
    telemetry->sourceFlags |= sourceFlag;
    UnlockTelemetry(telemetry);
}

} // namespace reversa
