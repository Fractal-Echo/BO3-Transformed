

#include <Windows.h>
#include <cstdint>

uintptr_t FindPattern(
    HANDLE hProc,
    DWORD procId,
    uintptr_t moduleBase,
    const char* pattern,
    const char* mask
);

uintptr_t FindPatternEx(
    HANDLE hProc,
    const char* pattern,
    const char* mask
);


uintptr_t FindPatternCached(
    const char* name,
    HANDLE hProc,
    DWORD procId,
    uintptr_t moduleBase,
    const char* pattern,
    const char* mask
);

uintptr_t FindPatternExCached(
    const char* name,
    HANDLE hProc,
    const char* pattern,
    const char* mask
);

void ClearPatternCache();