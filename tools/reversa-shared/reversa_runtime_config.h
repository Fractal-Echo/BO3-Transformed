#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cwchar>

namespace reversa {

enum class BO3RuntimeLane {
    Unknown,
    SteamT7,
    BO3Enhanced
};

inline bool ReadEnvironmentBool(const wchar_t* name, bool& value)
{
    wchar_t raw[32]{};
    DWORD chars = GetEnvironmentVariableW(name, raw, static_cast<DWORD>(_countof(raw)));
    if (chars == 0) {
        return false;
    }

    if (_wcsicmp(raw, L"0") == 0 ||
        _wcsicmp(raw, L"false") == 0 ||
        _wcsicmp(raw, L"off") == 0 ||
        _wcsicmp(raw, L"no") == 0) {
        value = false;
    }
    else {
        value = true;
    }
    return true;
}

inline bool FileExistsInDirectory(const wchar_t* directory, const wchar_t* fileName)
{
    if (!directory || !directory[0] || !fileName || !fileName[0]) {
        return false;
    }

    wchar_t path[MAX_PATH]{};
    swprintf_s(path, L"%s\\%s", directory, fileName);
    DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

inline bool CurrentProcessImageNameEquals(const wchar_t* imageName)
{
    if (!imageName || !imageName[0]) {
        return false;
    }

    wchar_t path[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, path, static_cast<DWORD>(_countof(path))) == 0) {
        return false;
    }

    const wchar_t* baseName = wcsrchr(path, L'\\');
    baseName = baseName ? baseName + 1 : path;
    return _wcsicmp(baseName, imageName) == 0;
}

inline bool HasT7CompanionModule()
{
    return GetModuleHandleW(L"T7InternalWS.dll") != nullptr ||
        GetModuleHandleW(L"T7WSBootstrapper.dll") != nullptr;
}

inline bool HasT7CompanionFile(const wchar_t* moduleDirectory)
{
    return FileExistsInDirectory(moduleDirectory, L"T7InternalWS.dll") ||
        FileExistsInDirectory(moduleDirectory, L"T7WSBootstrapper.dll");
}

inline bool IsT7CompanionPresent(const wchar_t* moduleDirectory)
{
    return HasT7CompanionModule() || HasT7CompanionFile(moduleDirectory);
}

inline bool HasBO3EnhancedLaneFile(const wchar_t* moduleDirectory)
{
    return FileExistsInDirectory(moduleDirectory, L"WSBlackOps3.exe") ||
        FileExistsInDirectory(moduleDirectory, L"T7WSBootstrapper.dll");
}

inline BO3RuntimeLane DetectBO3RuntimeLane(const wchar_t* moduleDirectory)
{
    bool forcedEnhanced = false;
    if (ReadEnvironmentBool(L"REVERSA_BO3ENHANCED_COMPAT", forcedEnhanced)) {
        return forcedEnhanced ? BO3RuntimeLane::BO3Enhanced : BO3RuntimeLane::SteamT7;
    }

    if (CurrentProcessImageNameEquals(L"WSBlackOps3.exe") || HasBO3EnhancedLaneFile(moduleDirectory)) {
        return BO3RuntimeLane::BO3Enhanced;
    }

    if (CurrentProcessImageNameEquals(L"BlackOps3.exe") || FileExistsInDirectory(moduleDirectory, L"BlackOps3.exe")) {
        return BO3RuntimeLane::SteamT7;
    }

    return BO3RuntimeLane::Unknown;
}

inline const wchar_t* BO3RuntimeLaneName(BO3RuntimeLane lane)
{
    switch (lane) {
    case BO3RuntimeLane::BO3Enhanced:
        return L"BO3Enhanced";
    case BO3RuntimeLane::SteamT7:
        return L"Steam/T7";
    default:
        return L"Unknown";
    }
}

inline bool ShouldUseT7CompatibilityMode(const wchar_t* moduleDirectory)
{
    bool forced = false;
    if (ReadEnvironmentBool(L"REVERSA_T7_COMPAT", forced)) {
        return forced;
    }
    return IsT7CompanionPresent(moduleDirectory) ||
        DetectBO3RuntimeLane(moduleDirectory) == BO3RuntimeLane::BO3Enhanced;
}

inline bool ShouldEnableD3D11FactoryHooks(const wchar_t* moduleDirectory)
{
    bool forced = false;
    if (ReadEnvironmentBool(L"REVERSA_D3D11_FACTORY_HOOKS", forced)) {
        return forced;
    }
    return !ShouldUseT7CompatibilityMode(moduleDirectory);
}

} // namespace reversa
