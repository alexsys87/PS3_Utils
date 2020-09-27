#include <string>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdint.h>

#include <filesystem>

#include "aes.h"
#include "SHA1_lib.h"
#include "endianness.h"
#include "ConsoleColor.h"

#include "tools.h"
#include "pkg.h"


#define USAGE_STR "Usage : %s -[e|l|s|p] filename.pkg [out_dir]\n"

#if __STATIC_LIB
int pkg_read(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    if (argc < 3 || argc > 4)
        die(USAGE_STR, argv[0]);

    if (argv[1][0] != '-' && argv[1][2] != 0)
        die(USAGE_STR, argv[0]);

    switch (argv[1][1]) {
    case 'e':
        pkg_unpack(argv[2], argc == 4 ? argv[3] : NULL);
        break;
    case 's':
        pkg_extract_sig(argv[2]);
        break;
    case 'p':
        pkg_print_params(argv[2]);
        break;
    case 'l':
        pkg_list(argv[2]);
        break;
    default:
        die(USAGE_STR, argv[0]);
    }
    return 0;
}
