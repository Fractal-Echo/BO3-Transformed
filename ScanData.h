#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

class ScanData {
public:
    unsigned char* data = nullptr;
    size_t size = 0;

    static const unsigned char hexTable[];

    ScanData(size_t size);
    ScanData(std::string input);
    ~ScanData();

    void print();

    // Pattern scanners
    static uintptr_t bruteForce(const ScanData& signature, const ScanData& data);
    static uintptr_t boyerMooreHorspool(const ScanData& signature, const ScanData& data);
};