#include "ScanData.h"

#include <cctype>
#include <cstdio>

const unsigned char ScanData::hexTable[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00, 0x00, 0x0, 0x0, 0x0, 0x0, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };



ScanData::ScanData(size_t size)
{
	this->size = size;
	this->data = new unsigned char[size];
}

ScanData::ScanData(std::string input) {
	data = new unsigned char[input.size()];
	size_t index = 0;

	std::string::iterator end_pos = std::remove(input.begin(), input.end(), ' ');
	input.erase(end_pos, input.end()); //"FF0012?3B..."

	for (int x = 0, i = 0; i < input.size(); i += 2, x += 1)
	{
		size++;
		if (input[i] == '?') {
			data[x] = '?';
			i--;
		}
		else {// 'A' - '0' = 17 ... we want 10 instead}
			data[x] = (hexTable[toupper(input[i]) - '0'] << 4 | hexTable[toupper(input[i + 1]) - '0']);
		} //1100 or 0011 = 1111
	}
}

uintptr_t ScanData::bruteForce(const ScanData& signature, const ScanData& data) {
    for (size_t currentIndex = 0; currentIndex < data.size - signature.size; currentIndex++) {
        for (size_t sigIndex = 0; sigIndex < signature.size; sigIndex++) {
            if (data.data[currentIndex + sigIndex] != signature.data[sigIndex] && signature.data[sigIndex] != '?') {
                break;
            }
            else if (sigIndex == signature.size - 1) {
                return currentIndex;
            }
        }
    }
    return 0;
}

uintptr_t ScanData::boyerMooreHorspool(const ScanData& signature, const ScanData& data) {
    size_t maxShift = signature.size;
    size_t maxIndex = signature.size - 1;
    size_t wildCardIndex = 0;

    for (size_t i = 0; i < maxIndex; i++) {
        if (signature.data[i] == '?') {
            maxShift = maxIndex - i;
            wildCardIndex = i;
        }
    }

    size_t shiftTable[256];

    for (size_t i = 0; i <= 255; i++) {
        shiftTable[i] = maxShift;
    }

    for (size_t i = wildCardIndex + 1; i < maxIndex - 1; i++) {
        shiftTable[signature.data[i]] = maxIndex - i;
    }

    for (size_t currentIndex = 0; currentIndex < data.size - signature.size;) {
        for (size_t sigIndex = maxIndex; sigIndex >= 0; sigIndex--) {

            if (data.data[currentIndex + sigIndex] != signature.data[sigIndex] && signature.data[sigIndex] != '?') {
                currentIndex += shiftTable[data.data[currentIndex + maxIndex]];
                break;
            }
            else if (sigIndex == 0) {
                return currentIndex;
            }
        }
    }

    return 0;
}

	ScanData::~ScanData()
	{
		delete[] data;
	}
	void ScanData::print() {
		for (size_t i = 0; i < size; ++i)
		{
			printf_s("0x%02x", data[i]);
		}
		printf_s("\n");
	}