#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cwchar>

namespace reversa {

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

inline bool ShouldUseT7CompatibilityMode(const wchar_t* moduleDirectory)
{
    bool forced = false;
    if (ReadEnvironmentBool(L"REVERSA_T7_COMPAT", forced)) {
        return forced;
    }
    return IsT7CompanionPresent(moduleDirectory);
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
