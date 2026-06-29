#include "proc.h"
#include <tlhelp32.h>
#include <iostream> // For error logging

// Retrieves the process ID of a running process by its name without noisy logs.
DWORD TryGetProcId(const wchar_t* procName)
{
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        std::cerr << "[ERROR] CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
        return 0;
    }

    PROCESSENTRY32 procEntry;
    procEntry.dwSize = sizeof(procEntry);
    if (Process32First(hSnap, &procEntry))
    {
        do
        {
            if (!_wcsicmp(procEntry.szExeFile, procName))
            {
                procId = procEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnap, &procEntry));
    }

    CloseHandle(hSnap);
    return procId;
}

// Retrieves the process ID of a running process by its name
DWORD GetProcId(const wchar_t* procName)
{
    DWORD procId = TryGetProcId(procName);
    if (procId == 0)
    {
        std::cerr << "[ERROR] Process not found: " << procName << std::endl;
    }
    return procId;
}

// Retrieves the base address of a module in the target process
uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        std::cerr << "[ERROR] CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
        return 0;
    }

    MODULEENTRY32 modEntry;
    modEntry.dwSize = sizeof(modEntry);
    if (Module32First(hSnap, &modEntry))
    {
        do
        {
            if (!_wcsicmp(modEntry.szModule, modName))
            {
                modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnap, &modEntry));
    }

    if (modBaseAddr == 0)
    {
        std::cerr << "[ERROR] Module not found: " << modName << std::endl;
    }

    CloseHandle(hSnap);
    return modBaseAddr;
}

// Finds a dynamic memory address in a process using a base address and a series of offsets
uintptr_t FindDMAAddy(HANDLE hProc, uintptr_t ptr, const std::vector<unsigned int>& offsets)
{
    uintptr_t addr = ptr;
    for (unsigned int i = 0; i < offsets.size(); ++i)
    {
        SIZE_T bytesRead;
        if (!ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(addr), &bytesRead) || bytesRead != sizeof(addr))
        {
           // std::cerr << "[ERROR] ReadProcessMemory failed at address: 0x" << std::hex << addr << " with error: " << GetLastError() << std::endl;
            return 0; // Return 0 to indicate failure
        }
        addr += offsets[i];
    }
    return addr;
}
