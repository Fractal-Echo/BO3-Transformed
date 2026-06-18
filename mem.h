#pragma once
#include <Windows.h>
#include <string>



namespace mem {
    void PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess);
    void NopEx(BYTE* dst, unsigned int size, HANDLE hProcess);
    void WriteStringEx(BYTE* dst, const std::string& str, HANDLE hProcess);
}






