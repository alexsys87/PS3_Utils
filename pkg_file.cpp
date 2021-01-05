#include <string>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdint.h>

#include <filesystem>

#include "SHA1_lib.h"
#include "endianness.h"
#include "ConsoleColor.h"
#include "ConsoleUtils.h"

#include "pkg_file.h"

namespace fs = std::filesystem;

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

#define BUFFER_SIZE (1 * 1024 * 1024)

#if 0
static inline uint64_t align(uint64_t data, uint8_t align)
{
	std::size_t sz; void* p = (void*)data;
	return (uint64_t)std::align(align, sizeof(uint64_t), p, sz);
}
#endif

void calc_hash_init(uint8_t* digest, uint64_t* key)
{
	uint8_t sha1_key[0x40];

	memset(sha1_key, 0, 0x40);

	memcpy(sha1_key, digest, 8);
	memcpy(sha1_key + 0x8, digest, 8);
	memcpy(sha1_key + 0x10, digest + 0x8, 8);
	memcpy(sha1_key + 0x18, digest + 0x8, 8);

	memcpy(key, sha1_key, 0x40);
}

void calc_hash_update(uint8_t* sha1Digest, uint64_t* key, uint8_t* data, uint64_t len)
{
	while (len > 0)
	{
		SHA1((const unsigned char*)key, 0x40, sha1Digest);

		uint64_t count = 0x10;
		if (len < 0x10) { count = len; }

		for (int i = 0; i < count; i++)
		{
			*data++ ^= sha1Digest[i];
		}

		len -= count;

		key[7] = be64(be64(key[7]) + 1);

	}
}

void calc_hash(uint8_t* digest, uint8_t* data, uint64_t len, bool num)
{
	uint8_t sha1_key[0x40];

	memset(sha1_key, 0, 0x40);

	memcpy(sha1_key, digest, 8);
	memcpy(sha1_key + 0x8, digest, 8);
	memcpy(sha1_key + 0x10, digest + 0x8, 8);
	memcpy(sha1_key + 0x18, digest + 0x8, 8);

	uint8_t sha1Digest[SHA_DIGEST_LENGTH];
	uint64_t key[0x8];
	memcpy(key, sha1_key, 0x40);

	if(num)
		memset(sha1_key + 0x38, 0xff, 8);

#if _DEBUG
	auto start = std::chrono::system_clock::now();
#endif

	LONG64 llSize = len, llBytes = 0;

	while (len > 0)
	{
		SHA1((const unsigned char*)key, 0x40, sha1Digest);

		uint64_t count = 0x10;
		if (len < 0x10) { count = len; }

		for (int i = 0; i < count; i++)
		{
			*data++ ^= sha1Digest[i];
		}

		len -= count;

		key[7] = be64(be64(key[7]) + 1);

		llBytes += count;

		if ((llBytes % (1024*1024) == 0)) //update every mb
		{
			// update progressbar
			Progress(llBytes, llSize);
		}
	}

	// finish progressbar
	Progress(llBytes, llSize);

#if _DEBUG
	auto end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed = end - start;
	std::cout << std::endl << "Elapsed time: " << elapsed.count() << " s" << std::endl;
#endif
}

void scan_dir_recursive(std::string pkg_directory, std::vector<PKG_ITEM_RECORD_t>* pkg_files, std::vector<std::string>* file_paths)
{
	fs::recursive_directory_iterator dirIt(pkg_directory);

	for (const auto& dir_entry : dirIt)
	{
#if _DEBUG
		std::cout << std::endl << dir_entry.path() << std::endl;
#endif

		std::string file_path = GetFixetPath(dir_entry.path().string().substr(dir_entry.path().string().find_first_of("\\") + 1));

		uint32_t name_len = (uint32_t)file_path.length();
		uint64_t file_size = dir_entry.file_size();

		PKG_ITEM_RECORD_t pkg_file = {};

		pkg_file.flags = PKG_FILE_OVERWRITE;
		pkg_file.filename_size = be32(name_len);

		file_paths->push_back(file_path);

		if (is_directory(dirIt->status()))
		{
			// directory
			pkg_file.flags |= PKG_FILE_DIRECTORY;
#if _DEBUG
			std::cout << std::endl << "  directory: " << file_path << std::endl;
#endif
			// add
			pkg_file.flags = be32(pkg_file.flags);
			pkg_files->push_back(pkg_file);
		}
		else
		{
			// file
			pkg_file.flags |= PKG_FILE_RAW;
			pkg_file.data_size = be64(file_size);
#if _DEBUG
			std::cout << std::endl << "  raw data:" << file_path << std::endl;
#endif
			// add
			pkg_file.flags = be32(pkg_file.flags);
			pkg_files->push_back(pkg_file);
		}
	}
}

int pkg_write(PKG_t pkg, std::string directory, std::string pkg_file_name)
{
	// process pkg directory
	std::cout << std::endl << std::endl << green << "Packing file data ..." << white << std::endl << std::endl;

	if (!fs::exists(directory))
	{
		std::cerr << red << "error: directory not exist" << white << std::endl;

		return -1;
	}

	std::vector<PKG_ITEM_RECORD_t> pkg_files;
	std::vector<std::string> file_paths;

	auto start = std::chrono::system_clock::now();

	scan_dir_recursive(directory, &pkg_files, &file_paths);

	pkg.header.item_count = (uint32_t)pkg_files.size();

	uint64_t base_offset = sizeof(PKG_ITEM_RECORD_t) * pkg.header.item_count;
	uint32_t filename_offset = 0;
	uint64_t data_offset = 0;

	for (std::vector<PKG_ITEM_RECORD_t>::iterator it = pkg_files.begin(); it != pkg_files.end(); it++)
	{
		pkg.header.data_size += sizeof(PKG_ITEM_RECORD_t) + ALIGN(be64(it->data_size), 0x10);

		it->filename_offset = be32((uint32_t)(base_offset + filename_offset));
		filename_offset += (uint32_t)ALIGN(be32(it->filename_size), 0x10);
	}

	pkg.header.data_size += filename_offset;
	base_offset += filename_offset;

	for (std::vector<PKG_ITEM_RECORD_t>::iterator it = pkg_files.begin(); it != pkg_files.end(); it++)
	{
		it->data_offset = be64(base_offset + data_offset);
		data_offset += ALIGN(be64(it->data_size), 0x10);
	}

	base_offset -= filename_offset;

	FileTouch(pkg_file_name);

	std::ifstream ipkgFile(pkg_file_name, std::fstream::in  | std::fstream::binary);
	std::ofstream opkgFile(pkg_file_name, std::fstream::out | std::fstream::binary);

	//opkgFile.rdbuf()->pubsetbuf(0, 0);

	size_t pkg_offset = sizeof(pkg.header) + sizeof(pkg.header_digest) + sizeof(pkg.first_metadata) + sizeof(pkg.second_metadata) + sizeof(pkg.metadata_digest);

	Fill_File(&opkgFile, 0, BUFFER_SIZE, pkg_offset);

	opkgFile.write((const char*)&pkg_files.front(), sizeof(PKG_ITEM_RECORD_t) * pkg.header.item_count);

	for (uint32_t i = 0; i < pkg.header.item_count; i++)
	{
		uint32_t filename_size = be32(pkg_files.at(i).filename_size);
		uint32_t filename_size_align = (uint32_t)ALIGN(filename_size, 0x10);

		char* data = new char[filename_size_align];
		memset(data, 0, filename_size_align);
		memcpy(data, file_paths.at(i).c_str(), filename_size);

		opkgFile.seekp(pkg_offset + base_offset, std::fstream::beg);

		opkgFile.write((const char*)data, filename_size_align);

		opkgFile.flush();

		base_offset += filename_size_align;
	}

	data_offset = 0;

	char* path = new char[MAX_PATH];
	std::streamsize len;

	for (std::vector<std::string>::iterator it = file_paths.begin(); it != file_paths.end(); it++)
	{
		snprintf(path, MAX_PATH, "%s/%s", directory.c_str(), it->c_str());

		if (fs::is_directory(path)) continue;

		std::cout << " read file: " << GetFileName(path) << std::endl;

		std::ifstream File(path, std::ifstream::in | std::ifstream::binary);

		uint64_t data_size_align = ALIGN((size_t)fs::file_size(path), 0x10);

		char* data = new char[BUFFER_SIZE];
		memset(data, 0, BUFFER_SIZE);
		File.clear();
		File.seekg(0, std::fstream::beg);
		opkgFile.seekp(pkg_offset + base_offset + data_offset, std::fstream::beg);

		do {

			File.read(data, BUFFER_SIZE);
			File.sync();
			len = File.gcount();

			opkgFile.write((const char*)data, len);
			opkgFile.flush();

		} while (len == BUFFER_SIZE);

		data_offset += data_size_align;

		File.close();
		delete[] data;
	}

	delete[] path;

	// fix to minimum 100 KB
	uint8_t* size_fix = NULL;
	uint64_t pad_size = 0;

	pkg.header.total_size = be64(pkg.header.data_offset) + pkg.header.data_size + sizeof(DIGEST_t) + sizeof(PKG_FOOTER_t);

	if (pkg.header.total_size < (100 * 1024))
	{
		pad_size = (100 * 1024) - pkg.header.total_size;
		std::cout << std::endl << yellow << "Fixing pkg size to 100 KB ..." << white << std::endl;
		size_fix = new uint8_t[(size_t)pad_size];
		memset(size_fix, 0, (size_t)pad_size);
		pkg.header.total_size += pad_size;
	}

	// calculate new offsets and sizes
	std::cout << std::endl << green << "Calculating offsets ..." << white << std::endl;

	pkg.header.data_size  = be64(pkg.header.data_size);
	pkg.header.item_count = be32(pkg.header.item_count);
	pkg.header.total_size = be64(pkg.header.total_size);

	pkg.first_metadata.PackageSize.data2 = be32((uint32_t)be64(pkg.header.data_size));

	// print some pkg info
	std::cout << std::endl;
	std::cout << " PKG item count:   " << std::dec << be32(pkg.header.item_count)  << " (0x" << std::hex << be32(pkg.header.item_count)  << ")" << std::endl;
	std::cout << " PKG data offset:  " << std::dec << be64(pkg.header.data_offset) << " (0x" << std::hex << be64(pkg.header.data_offset) << ")" << std::endl;
	std::cout << " PKG data size:    " << std::dec << be64(pkg.header.data_size)   << " (0x" << std::hex << be64(pkg.header.data_size)   << ")" << std::endl;
	std::cout << " PKG total size:   " << std::dec << be64(pkg.header.total_size)  << " (0x" << std::hex << be64(pkg.header.total_size)  << ")" << std::endl;
	std::cout << std::endl;

	// calculate hashes
	std::cout << green << "Calculating hashes ..." << white << std::endl << std::endl;

	uint8_t sha1[SHA_DIGEST_LENGTH];
	SHA_CTX ctx_sha1;

	// calculate qa_digest

	std::cout << green << "Calculating qa digest" << white << std::endl << std::endl;

	SHA1_Init(&ctx_sha1);
	SHA1_Update(&ctx_sha1, (uint8_t*)&pkg.second_metadata, sizeof(pkg.second_metadata));

	uint8_t* buffer = new uint8_t[BUFFER_SIZE];
	LONG64 llSize = data_offset, llBytes = 0;
	memset(buffer, 0, BUFFER_SIZE);
	ipkgFile.clear();
	ipkgFile.seekg(pkg_offset, std::fstream::beg);
	do {
		ipkgFile.read((char*)buffer, BUFFER_SIZE); len = ipkgFile.gcount();
		SHA1_Update(&ctx_sha1, buffer, len);

		llBytes += len;

		if ((llBytes % (1024 * 1024) == 0)) //update every mb
		{
			// update progressbar
			Progress(llBytes, llSize);
		}
	} while (len == BUFFER_SIZE);
	SHA1_Final(sha1, &ctx_sha1);

	// Finish progressbar
	Progress(llBytes, llSize);

	memcpy(&pkg.second_metadata.qa_digest, sha1 + 3, sizeof(pkg.second_metadata.qa_digest));
	SHA1((uint8_t*)&pkg.second_metadata.qa_digest, sizeof(pkg.second_metadata.qa_digest), sha1);
	memcpy(pkg.header.digest, sha1 + 3, sizeof(pkg.second_metadata.qa_digest));

	calc_hash(pkg.header.digest, pkg.header.digest, sizeof(pkg.header.digest), 1);

	// k_licensee crypt
	calc_hash(pkg.header.digest, pkg.header.pkg_data_riv, sizeof(pkg.header.pkg_data_riv), 1);

	// header hash
	SHA1((uint8_t*)&pkg.header, sizeof(pkg.header), sha1);
	memcpy(pkg.header_digest.cmac_hash, sha1 + 3, sizeof(pkg.header_digest.cmac_hash));
	calc_hash(pkg.header_digest.cmac_hash, pkg.header_digest.npdrm_signature, sizeof(pkg.header_digest.npdrm_signature) + sizeof(pkg.header_digest.sha1_hash), 0);

	// metadata hash
	SHA1_Init(&ctx_sha1);
	SHA1_Update(&ctx_sha1, (uint8_t*)&pkg.first_metadata, sizeof(pkg.first_metadata));
	SHA1_Update(&ctx_sha1, (uint8_t*)&pkg.second_metadata, sizeof(pkg.second_metadata));
	SHA1_Final(sha1, &ctx_sha1);
	memcpy(pkg.metadata_digest.cmac_hash, sha1 + 3, sizeof(pkg.metadata_digest.cmac_hash));
	calc_hash(pkg.metadata_digest.cmac_hash, pkg.metadata_digest.npdrm_signature, sizeof(pkg.header_digest.npdrm_signature) + sizeof(pkg.header_digest.sha1_hash), 0);

	// data crypt

	std::cout << std::endl << std::endl << green << "Crypt data" << white << std::endl << std::endl;

	uint8_t sha1Digest[SHA_DIGEST_LENGTH];
	uint64_t key[0x8]; llBytes = 0;
	memset(buffer, 0, BUFFER_SIZE);
	memset(sha1Digest, 0, SHA_DIGEST_LENGTH);
	ipkgFile.clear();
	ipkgFile.seekg(pkg_offset, std::fstream::beg);
	opkgFile.seekp(pkg_offset, std::fstream::beg);
	calc_hash_init(pkg.header.digest, key);
	do {
		ipkgFile.read((char*)buffer, BUFFER_SIZE);
		ipkgFile.sync();
		len = ipkgFile.gcount();
		calc_hash_update(sha1Digest, key, buffer, len);
		opkgFile.write((char*)buffer, len);
		opkgFile.flush();

		llBytes += len;

		if ((llBytes % (1024 * 1024) == 0)) //update every mb
		{
			// update progressbar
			Progress(llBytes, llSize);
		}
	} while (len == BUFFER_SIZE);

	// Finish progressbar
	Progress(llBytes, llSize);

	// data hash
	std::cout << std::endl << std::endl << green << "Hash data" << white << std::endl << std::endl;

	SHA_CTX ctx_data;
	llBytes = 0;
	SHA1_Init(&ctx_data);
	memset(buffer, 0, BUFFER_SIZE);
	ipkgFile.clear();
	ipkgFile.seekg(pkg_offset, std::fstream::beg);
	do {
		ipkgFile.read((char*)buffer, BUFFER_SIZE); len = ipkgFile.gcount();
		SHA1_Update(&ctx_data, buffer, len);

		llBytes += len;

		if ((llBytes % (1024 * 1024) == 0)) //update every mb
		{
			// update progressbar
			Progress(llBytes, llSize);
		}
	} while (len == BUFFER_SIZE);
	SHA1_Final(sha1, &ctx_data);

	// Finish progressbar
	Progress(llBytes, llSize);

	memcpy(pkg.data_digest.cmac_hash, sha1 + 3, sizeof(pkg.data_digest.cmac_hash));
	calc_hash(pkg.data_digest.cmac_hash, pkg.data_digest.npdrm_signature, sizeof(pkg.header_digest.npdrm_signature) + sizeof(pkg.header_digest.sha1_hash), 0);

	// file sha1_digest - 0x20

	std::cout << std::endl << std::endl << green << "Calculating PKG Digest" << white << std::endl << std::endl;

	SHA1_Init(&ctx_sha1);
	SHA1_Update(&ctx_sha1, (uint8_t*)&pkg.header, sizeof(pkg.header));
	SHA1_Update(&ctx_sha1, (uint8_t*)&pkg.header_digest, sizeof(pkg.header_digest));
	SHA1_Update(&ctx_sha1, (uint8_t*)&pkg.first_metadata, sizeof(pkg.first_metadata));
	SHA1_Update(&ctx_sha1, (uint8_t*)&pkg.second_metadata, sizeof(pkg.second_metadata));
	SHA1_Update(&ctx_sha1, (uint8_t*)&pkg.metadata_digest, sizeof(pkg.metadata_digest));

	llBytes = 0;
	memset(buffer, 0, BUFFER_SIZE);
	ipkgFile.clear();
	ipkgFile.seekg(pkg_offset, std::fstream::beg);
	do {
		ipkgFile.read((char*)buffer, BUFFER_SIZE); len = ipkgFile.gcount();
		SHA1_Update(&ctx_sha1, buffer, len);

		llBytes += len;

		if ((llBytes % (1024 * 1024) == 0)) //update every mb
		{
			// update progressbar
			Progress(llBytes, llSize);
		}
	} while (len == BUFFER_SIZE);
	SHA1_Update(&ctx_sha1, (uint8_t*)&pkg.data_digest, sizeof(pkg.data_digest));

	if (size_fix != NULL)
	{
		SHA1_Update(&ctx_sha1, size_fix, (size_t)pad_size);
	}

	SHA1_Final(pkg.footer.data_sha1, &ctx_sha1);

	ipkgFile.close();

	// Finish progressbar
	Progress(llBytes, llSize);

	std::cout << std::endl; std::cout << std::endl;
	std::cout << " PKG QA_Digest:  " << BytesToHex(pkg.header.digest, sizeof(pkg.header.digest)) << std::endl;
	std::cout << " PKG QA_Digest2: " << BytesToHex(pkg.second_metadata.qa_digest, sizeof(pkg.second_metadata.qa_digest)) << std::endl;
	std::cout << " PKG Digest:     " << BytesToHex(pkg.footer.data_sha1, sizeof(pkg.footer.data_sha1) - 0xc) << std::endl;
	std::cout << std::endl;

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << std::endl << std::endl << "Elapsed time: " << elapsed.count() << " s" << std::endl << std::endl;

	// write pkg
	std::cout << green << "PKG built - writing to file" << white << std::endl;

	opkgFile.clear();
	opkgFile.seekp(0, std::fstream::beg);

	opkgFile.write((const char*)&pkg.header, sizeof(pkg.header));
	opkgFile.write((const char*)&pkg.header_digest, sizeof(pkg.header_digest));
	opkgFile.write((const char*)&pkg.first_metadata, sizeof(pkg.first_metadata));
	opkgFile.write((const char*)&pkg.second_metadata, sizeof(pkg.second_metadata));
	opkgFile.write((const char*)&pkg.metadata_digest, sizeof(pkg.metadata_digest));

	opkgFile.seekp(be64(pkg.header.data_size), std::fstream::cur);
	opkgFile.write((const char*)&pkg.data_digest, sizeof(pkg.data_digest));

	if (size_fix != NULL)
	{
		opkgFile.write((const char*)size_fix, sizeof(pad_size) - 8);
	}

	opkgFile.write((const char*)&pkg.footer, sizeof(pkg.footer));

	opkgFile.flush();

	opkgFile.close();

	std::cout << green << "PKG file successfully written." << white << std::endl;

	return 0;
}

