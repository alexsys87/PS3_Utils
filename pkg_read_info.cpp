// pkg_read.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "endianness.h"
#include "ConsoleColor.h"
#include "ConsoleUtils.h"
#include "pkg_file.h"

void pkgPrintHeader(PKG_HEADER_t* pkgHeader)
{
    std::cout << "\n[HEADER]\n" << std::endl;
    std::cout << "  magic:                    " << std::hex << be32(pkgHeader->magic) << std::endl;
    std::cout << "  pkg revision:             " << std::hex << be16(pkgHeader->pkg_revision) << std::endl;
    std::cout << "  pkg type:                 " << std::hex << be16(pkgHeader->pkg_type) << std::endl;
    std::cout << "  pkg metadata offset:      " << std::hex << be32(pkgHeader->pkg_metadata_offset) << std::endl;
    std::cout << "  pkg metadata count:       " << std::hex << be32(pkgHeader->pkg_metadata_count) << std::endl;
    std::cout << "  pkg metadata size:        " << std::hex << be32(pkgHeader->pkg_metadata_size) << std::endl;
    std::cout << "  item count:               " << std::hex << be32(pkgHeader->item_count) << std::endl;
    std::cout << "  total size:               " << std::hex << be64(pkgHeader->total_size) << std::endl;
    std::cout << "  data offset:              " << std::hex << be64(pkgHeader->data_offset) << std::endl;
    std::cout << "  data size:                " << std::hex << be64(pkgHeader->data_size) << std::endl;
    std::cout << "  contentid:                " << pkgHeader->contentid << std::endl;
    std::cout << "  pkgHashes:" << std::endl;
    std::cout << "  digest                    ";
    std::cout << BytesToHex(pkgHeader->digest, sizeof(pkgHeader->digest)) << std::endl;
    std::cout << "  aes-128-ctr IV            ";
    std::cout << BytesToHex(pkgHeader->pkg_data_riv, sizeof(pkgHeader->pkg_data_riv)) << std::endl;
}

void pkgPrintDigest(DIGEST_t* digest)
{
    std::cout << "  CMAC OMAC hash            ";
    std::cout << BytesToHex(digest->cmac_hash, sizeof(digest->cmac_hash)) << std::endl;
    std::cout << "  NPDRM ECDSA               ";
    std::cout << BytesToHex(&digest->npdrm_signature[0],  16) << std::endl;
    std::cout << "                            ";
    std::cout << BytesToHex(&digest->npdrm_signature[16], 16) << std::endl;
    std::cout << "                            ";
    std::cout << BytesToHex(&digest->npdrm_signature[32], 8)  << std::endl;
    std::cout << "  sha1 hash                 ";
    std::cout << BytesToHex(digest->sha1_hash, sizeof(digest->sha1_hash)) << std::endl;
}

int main(int argc, char** argv)
{
    //
    // Check the arguments.
    //

    if (argc < 2)
    {
        std::cout << "Usage: %s <pkg file>" << std::endl;
        return -1;
    }

    //
    // Open the package file.
    //

    std::ifstream pkgFile(argv[1], std::ifstream::in | std::ifstream::binary);

    if (!pkgFile)
    {
        std::cout << "Error: Could not open the package file. (" << argv[1] << ")" << std::endl;
        return -1;
    }

    //
    // Read in the package header.
    //

    PKG_HEADER_t pkgHeader;
    DIGEST_t digest;

    pkgFile.seekg(0);
    pkgFile.read((char *)&pkgHeader, sizeof(PKG_HEADER_t));
    pkgFile.read((char *)&digest, sizeof(DIGEST_t));

    pkgPrintHeader(&pkgHeader);
    pkgPrintDigest(&digest);

    pkgFile.close();

    return 0;
}