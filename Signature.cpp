#include "Signature.h"

#include <vector>
#include <cstring>
#include <tlhelp32.h>
#include <immintrin.h>
#include <intrin.h>
#include <unordered_map>
#include <string>
#include <mutex>

static std::unordered_map<std::string, uintptr_t> g_PatternCache;
static std::mutex g_PatternCacheMutex;


static const uint8_t* FindPatternSIMD(
    const uint8_t* data,
    size_t size,
    const uint8_t* pattern,
    const char* mask,
    size_t patternLen)
{
    if (!data || !pattern || !mask)
        return nullptr;

    if (patternLen == 0 || size < patternLen)
        return nullptr;

    // This SIMD scanner uses the first byte as the anchor.
    // Therefore the first mask byte must be fixed, not '?'.
    if (mask[0] == '?')
        return nullptr;

    uint8_t first = pattern[0];

    __m128i firstVec = _mm_set1_epi8((char)first);

    size_t i = 0;

    while (i + 16 <= size)
    {
        __m128i block = _mm_loadu_si128((const __m128i*)(data + i));
        __m128i cmp = _mm_cmpeq_epi8(block, firstVec);

        int maskBits = _mm_movemask_epi8(cmp);

        while (maskBits)
        {
            unsigned long bit = 0;
            _BitScanForward(&bit, maskBits);

            size_t pos = i + bit;

            if (pos + patternLen <= size)
            {
                bool match = true;

                for (size_t j = 1; j < patternLen; j++)
                {
                    if (mask[j] != '?' && data[pos + j] != pattern[j])
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                    return data + pos;
            }

            maskBits &= maskBits - 1;
        }

        i += 16;
    }

    // Tail scan so we do not miss the final bytes after the SIMD loop.
    for (; i + patternLen <= size; i++)
    {
        if (data[i] != pattern[0])
            continue;

        bool match = true;

        for (size_t j = 1; j < patternLen; j++)
        {
            if (mask[j] != '?' && data[i + j] != pattern[j])
            {
                match = false;
                break;
            }
        }

        if (match)
            return data + i;
    }

    return nullptr;
}

static size_t GetModuleSize(DWORD procId, uintptr_t moduleBase)
{
    HANDLE snap = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
        procId
    );

    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32W mod{};
    mod.dwSize = sizeof(mod);

    size_t size = 0;

    if (Module32FirstW(snap, &mod))
    {
        do
        {
            if ((uintptr_t)mod.modBaseAddr == moduleBase)
            {
                size = mod.modBaseSize;
                break;
            }

        } while (Module32NextW(snap, &mod));
    }

    CloseHandle(snap);

    return size;
}

uintptr_t FindPattern(
    HANDLE hProc,
    DWORD procId,
    uintptr_t moduleBase,
    const char* pattern,
    const char* mask)
{
    if (!hProc || !moduleBase || !pattern || !mask)
        return 0;

    size_t moduleSize = GetModuleSize(procId, moduleBase);

    if (moduleSize == 0)
        return 0;

    std::vector<uint8_t> buffer(moduleSize);

    SIZE_T bytesRead = 0;

    if (!ReadProcessMemory(
        hProc,
        (LPCVOID)moduleBase,
        buffer.data(),
        moduleSize,
        &bytesRead))
    {
        return 0;
    }

    size_t patternLen = strlen(mask);

    const uint8_t* result = FindPatternSIMD(
        buffer.data(),
        bytesRead,
        (const uint8_t*)pattern,
        mask,
        patternLen
    );

    if (!result)
        return 0;

    uintptr_t offset = result - buffer.data();

    return moduleBase + offset;
}

uintptr_t FindPatternEx(
    HANDLE hProc,
    const char* pattern,
    const char* mask)
{
    if (!hProc || !pattern || !mask)
        return 0;

    SYSTEM_INFO sys{};
    GetSystemInfo(&sys);

    uintptr_t start = (uintptr_t)sys.lpMinimumApplicationAddress;
    uintptr_t end = (uintptr_t)sys.lpMaximumApplicationAddress;

    size_t patternLen = strlen(mask);

    std::vector<uint8_t> buffer;

    uintptr_t addr = start;

    while (addr < end)
    {
        MEMORY_BASIC_INFORMATION mbi{};

        if (!VirtualQueryEx(hProc, (LPCVOID)addr, &mbi, sizeof(mbi)))
        {
            addr += 0x1000;
            continue;
        }

        uintptr_t base = (uintptr_t)mbi.BaseAddress;
        uintptr_t next = base + mbi.RegionSize;

        if (mbi.State != MEM_COMMIT)
        {
            addr = next;
            continue;
        }

        if (mbi.Protect & PAGE_GUARD)
        {
            addr = next;
            continue;
        }

        if (mbi.Protect & PAGE_NOACCESS)
        {
            addr = next;
            continue;
        }

        bool readable =
            (mbi.Protect & PAGE_READONLY) ||
            (mbi.Protect & PAGE_READWRITE) ||
            (mbi.Protect & PAGE_WRITECOPY) ||
            (mbi.Protect & PAGE_EXECUTE_READ) ||
            (mbi.Protect & PAGE_EXECUTE_READWRITE) ||
            (mbi.Protect & PAGE_EXECUTE_WRITECOPY);

        if (!readable)
        {
            addr = next;
            continue;
        }

        if (buffer.size() < mbi.RegionSize)
            buffer.resize(mbi.RegionSize);

        SIZE_T bytesRead = 0;

        if (ReadProcessMemory(
            hProc,
            (LPCVOID)base,
            buffer.data(),
            mbi.RegionSize,
            &bytesRead))
        {
            const uint8_t* result = FindPatternSIMD(
                buffer.data(),
                bytesRead,
                (const uint8_t*)pattern,
                mask,
                patternLen
            );

            if (result)
            {
                uintptr_t offset = result - buffer.data();
                return base + offset;
            }
        }

        if (next <= addr)
            break;

        addr = next;
    }

    return 0;
}




uintptr_t FindPatternCached(
    const char* name,
    HANDLE hProc,
    DWORD procId,
    uintptr_t moduleBase,
    const char* pattern,
    const char* mask)
{
    if (!name || !hProc || !moduleBase || !pattern || !mask)
        return 0;

    {
        std::lock_guard<std::mutex> lock(g_PatternCacheMutex);

        auto it = g_PatternCache.find(name);

        if (it != g_PatternCache.end())
            return it->second;
    }

    uintptr_t address = FindPattern(
        hProc,
        procId,
        moduleBase,
        pattern,
        mask
    );

    if (address)
    {
        std::lock_guard<std::mutex> lock(g_PatternCacheMutex);
        g_PatternCache[name] = address;
    }

    return address;
}


uintptr_t FindPatternExCached(
    const char* name,
    HANDLE hProc,
    const char* pattern,
    const char* mask)
{
    if (!name || !hProc || !pattern || !mask)
        return 0;

    {
        std::lock_guard<std::mutex> lock(g_PatternCacheMutex);

        auto it = g_PatternCache.find(name);

        if (it != g_PatternCache.end())
            return it->second;
    }

    uintptr_t address = FindPatternEx(
        hProc,
        pattern,
        mask
    );

    if (address)
    {
        std::lock_guard<std::mutex> lock(g_PatternCacheMutex);
        g_PatternCache[name] = address;
    }

    return address;
}

void ClearPatternCache()
{
    std::lock_guard<std::mutex> lock(g_PatternCacheMutex);
    g_PatternCache.clear();
}

