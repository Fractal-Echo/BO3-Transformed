#pragma once

#include <string>
#include <algorithm>
#include <cstdint>

class ScanData
{
private:
	static const size_t hexTable[];

public:
	unsigned char* data; //Byte Array
	size_t size = 0;


	ScanData(const std::string input);
	ScanData(size_t size);
	~ScanData();

	void print();
};


class ScanData {
public:
    unsigned char* data = nullptr;
    size_t size = 0;

    static const size_t hexTable[];

    ScanData(size_t size);
    ScanData(std::string input);
    ~ScanData();

    void print();

    // Pattern scanners
    static uintptr_t bruteForce(const ScanData& signature, const ScanData& data);
    static uintptr_t boyerMooreHorspool(const ScanData& signature, const ScanData& data);
};