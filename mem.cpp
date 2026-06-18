#include <Windows.h>
#include "mem.h"



void mem::PatchEx(BYTE* dst, BYTE* src, size_t size, HANDLE hProcess)
{
	DWORD oldprotect;
	VirtualProtectEx(hProcess, dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
	WriteProcessMemory(hProcess, dst, src, size, nullptr);
	VirtualProtectEx(hProcess, dst, size, oldprotect, &oldprotect);
}
void mem::NopEx(BYTE* dst, size_t size, HANDLE hProcess)
{
	BYTE* nopArray = new BYTE[size];
	memset(nopArray, 0x90, size);

	PatchEx(dst, nopArray, size, hProcess);
	delete[] nopArray;
}

void mem::WriteStringEx(BYTE* dst, const std::string& str, HANDLE hProcess)
{
	PatchEx(dst, (BYTE*)str.c_str(), str.size() + 1, hProcess);
}


