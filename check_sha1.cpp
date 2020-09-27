#include <stdio.h>
#include "SHA1_lib.h"
#include <fstream>
#include <iostream>

#define BUFSIZE 1024

int main(int argc, char** argv)
{
    unsigned char buffer[BUFSIZE];
    unsigned char sha1[SHA_DIGEST_LENGTH];
    SHA_CTX ctx;
    size_t len;

    if (argc < 2) {
        std::cout << "usage: %s <file>" << argv[0] << std::endl;
        return 1;
    }

    std::ifstream file(argv[1], std::fstream::in | std::fstream::binary);
    if (!file) {
        std::cout << "couldn't open " << argv[1] << std::endl;
        return 1;
    }

    SHA1_Init(&ctx);

    do {
        file.read((char*)buffer, BUFSIZE);
        len = file.gcount();
        SHA1_Update(&ctx, buffer, len);
    } while (len == BUFSIZE);

    SHA1_Final(sha1, &ctx);

    file.close();

    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        printf("%02x", sha1[i]);

    std::cout << std::endl;
    return 0;
}