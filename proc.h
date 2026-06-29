#ifndef PROC_H
#define PROC_H

#include <Windows.h>
#include <vector>

// Retrieves the process ID of a running process by its name
DWORD GetProcId(const wchar_t* procName);

// Retrieves the process ID without logging when the process is not running.
DWORD TryGetProcId(const wchar_t* procName);

// Retrieves the base address of a module in the target process
uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName);

// Finds a dynamic memory address in a process using a base address and a series of offsets
uintptr_t FindDMAAddy(HANDLE hProc, uintptr_t ptr, const std::vector<unsigned int>& offsets);

#endif // PROC_H
