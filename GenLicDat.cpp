
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include "crc32.h"

#include "endianness.h"

int main(int argc, char* argv[]) {

    std::string tid = argv[1];
    std::vector<char> licbuffer(0x10000);
    std::vector<uint8_t> header = { 0x50, 0x53, 0x33, 0x4c, 0x49, 0x43, 0x44, 0x41, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01 };
    std::vector<char> lictid = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

    std::fill(licbuffer.begin(), licbuffer.end(), 0x00);
    std::copy(header.data(), header.data() + header.size(), licbuffer.begin());
    std::copy(tid.c_str(), tid.c_str() + tid.size(), lictid.begin() + 1);
    std::copy(lictid.data(), lictid.data() + lictid.size(), licbuffer.begin() + 0x800);

	uint32_t crc = be32(crc32(licbuffer.data(), 0x900));

    std::vector<std::uint8_t> crc_data((std::uint8_t*) & crc, (std::uint8_t*) &(crc)+sizeof(std::uint32_t));

    std::copy(crc_data.data(), crc_data.data() + crc_data.size(), licbuffer.begin() + 0x20);

    std::ofstream licFile("LIC.DAT", std::ofstream::binary);
    licFile.write(licbuffer.data(), licbuffer.size());
    licFile.close();
}