#include <Windows.h>
#include "mem.h"
#include <vector>



bool mem::PatchEx(BYTE* dst, const BYTE* src, size_t size, HANDLE hProcess)
{
	if (!hProcess || !dst || !src || size == 0)
		return false;

	DWORD oldprotect = 0;
	if (!VirtualProtectEx(hProcess, dst, size, PAGE_EXECUTE_READWRITE, &oldprotect))
		return false;

	SIZE_T bytesWritten = 0;
	const bool wrote = WriteProcessMemory(hProcess, dst, src, size, &bytesWritten) && bytesWritten == size;

	DWORD ignored = 0;
	VirtualProtectEx(hProcess, dst, size, oldprotect, &ignored);
	return wrote;
}
bool mem::NopEx(BYTE* dst, size_t size, HANDLE hProcess)
{
	if (size == 0)
		return false;

	std::vector<BYTE> nopArray(size, 0x90);
	return PatchEx(dst, nopArray.data(), nopArray.size(), hProcess);
}

bool mem::WriteStringEx(BYTE* dst, const std::string& str, HANDLE hProcess)
{
	return PatchEx(dst, reinterpret_cast<const BYTE*>(str.c_str()), str.size() + 1, hProcess);
}


