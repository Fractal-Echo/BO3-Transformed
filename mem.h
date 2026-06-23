#pragma once
#include <Windows.h>
#include <cstddef>
#include <string>



namespace mem {
    bool TryPatchEx(BYTE* dst, const BYTE* src, size_t size, HANDLE hProcess);
    bool TryNopEx(BYTE* dst, size_t size, HANDLE hProcess);
    bool TryWriteStringEx(BYTE* dst, const std::string& str, HANDLE hProcess);

    void PatchEx(BYTE* dst, BYTE* src, size_t size, HANDLE hProcess);
    void NopEx(BYTE* dst, size_t size, HANDLE hProcess);
    void WriteStringEx(BYTE* dst, const std::string& str, HANDLE hProcess);
}






