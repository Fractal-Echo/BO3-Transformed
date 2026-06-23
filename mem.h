#pragma once
#include <Windows.h>
#include <cstddef>
#include <string>



namespace mem {
    bool PatchEx(BYTE* dst, const BYTE* src, size_t size, HANDLE hProcess);
    bool NopEx(BYTE* dst, size_t size, HANDLE hProcess);
    bool WriteStringEx(BYTE* dst, const std::string& str, HANDLE hProcess);
}






