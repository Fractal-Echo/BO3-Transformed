#pragma once
#include <Windows.h>
#include <cstddef>
#include <string>



namespace mem {
    void PatchEx(BYTE* dst, BYTE* src, size_t size, HANDLE hProcess);
    void NopEx(BYTE* dst, size_t size, HANDLE hProcess);
    void WriteStringEx(BYTE* dst, const std::string& str, HANDLE hProcess);
}






