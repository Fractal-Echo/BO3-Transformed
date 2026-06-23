#include <Windows.h>
#include "mem.h"
#include <vector>



bool mem::TryPatchEx(BYTE* dst, const BYTE* src, size_t size, HANDLE hProcess)
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
bool mem::TryNopEx(BYTE* dst, size_t size, HANDLE hProcess)
{
	if (size == 0)
		return false;

	std::vector<BYTE> nopArray(size, 0x90);
	return TryPatchEx(dst, nopArray.data(), nopArray.size(), hProcess);
}

bool mem::TryWriteStringEx(BYTE* dst, const std::string& str, HANDLE hProcess)
{
	return TryPatchEx(dst, reinterpret_cast<const BYTE*>(str.c_str()), str.size() + 1, hProcess);
}

void mem::PatchEx(BYTE* dst, BYTE* src, size_t size, HANDLE hProcess)
{
	TryPatchEx(dst, src, size, hProcess);
}

void mem::NopEx(BYTE* dst, size_t size, HANDLE hProcess)
{
	TryNopEx(dst, size, hProcess);
}

void mem::WriteStringEx(BYTE* dst, const std::string& str, HANDLE hProcess)
{
	TryWriteStringEx(dst, str, hProcess);
}


