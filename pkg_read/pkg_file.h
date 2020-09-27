#pragma once
#include <stdint.h>

#define PKG_FILE_OVERWRITE	0x80000000
#define PKG_FILE_SELFNPDRM	0x1
#define PKG_FILE_DIRECTORY	0x4
#define PKG_FILE_RAW		0x3


typedef struct {
	uint32_t magic;                             // 7F 50 4B 47 (.PKG)
	uint16_t pkg_revision;                      // 80 00 for finalized (retail), 00 00 for non finalized (debug)
	uint16_t pkg_type;                          // 00 01 for PS3
	uint32_t pkg_metadata_offset;               // usually 0xC0 for PS3
	uint32_t pkg_metadata_count;                // metadata item count
	uint32_t pkg_metadata_size;                 // usually 0xC0 for PS3
	uint32_t item_count;                        // files and folders into the encrypted data
	uint64_t total_size;                        // total pkg file size
	uint64_t data_offset;                       // encrypted data offset
	uint64_t data_size;                         // encrypted data size
	uint8_t  contentid[0x30];                   // PKG Content ID + 0x0C bytes padding
	uint8_t  digest[0x10];                      // sha1 from debug files and attributes together merged in one block
	uint8_t  pkg_data_riv[0x10];                // aes-128-ctr IV. Used with gpkg_aes_key to decrypt data.
} PKG_HEADER_t; // 128 bytes

typedef struct {
	uint8_t  cmac_hash[0x10];					// CMAC OMAC hash from 0x00-0x7F. PS3 gpkg_key used as key.
	uint8_t  npdrm_signature[0x28];				// PKG header NPDRM ECDSA (R_sig, S_sig)
	uint8_t  sha1_hash[0x8];                    // last 8 bytes of sha1 hash of 0x00-0x7F
} DIGEST_t; // 64 bytes

typedef struct {
	uint32_t packet_id;							// 0x1
	uint32_t data_size;							// 0x4
	uint32_t data;								// 0x1 (network), 0x2 (local), 0x3 (free)
} PKG_DrmType;

typedef struct {
	uint32_t packet_id;							// 0x2
	uint32_t size;								// 0x4
	uint32_t data;								// 0x5 (gameexec) (GameData, Theme, License)
} PKG_ContentType;

typedef struct {
	uint32_t packet_id;							// 0x3
	uint32_t size;								// 0x4
	uint32_t data;								// 0x4e (HDDGamePatch) 4rd byte, bit: x1xx xxxx, ex: 00 00 00 4E, 00 00 00 5E
} PKG_PackageType;

typedef struct {
	uint32_t packet_id;							// 0x4
	uint32_t size;								// 0x8
	uint32_t data1;								// 0x0
	uint32_t data2;								// Package Size
} PKG_PackageSize;

typedef struct {
	uint32_t packet_id;							// 0x5
	uint32_t data_size;							// 0x4
	uint16_t author;							// make_package_npdrm Revision (2 bytes)
	uint16_t version;							// Package Version (2 bytes)
} PKG_PackageVersion;


typedef struct {

	PKG_DrmType DrmType;

	PKG_ContentType ContentType;

	PKG_PackageType PackageType;

	PKG_PackageSize PackageSize;

	PKG_PackageVersion PackageVersion;

} PKG_FIRST_METADATA_t; // 64 bytes

typedef struct {
	uint32_t packet_id_7;						// 0x7
	uint32_t size_18;							// 0x18
	uint32_t qa_digest0;						// 0x0
	uint32_t qa_digest1;						// 0x0
	uint8_t  qa_digest[0x10];

	uint32_t packet_id_8;						// 0x8 unk(1 byte) + PS3 / PSP / PSP2 System Version(3 bytes) + Package Version(2 bytes) + App Version(2 bytes)
	uint32_t size_8;							// 0x8
	uint8_t  flags;								// 0x85? 0x81?
	uint8_t  fw_major;
	uint16_t fw_minor;
	uint16_t pkg_version;
	uint16_t app_version;

	uint32_t unknown91;							// 0x9
	uint32_t unknown92;							// 0x8
	uint32_t unknown93;							// 0x0
	uint32_t unknown94;							// 0x0
} PKG_SECOND_METADATA_t; // 64 bytes

typedef struct {
	uint32_t filename_offset;
	uint32_t filename_size;
	uint64_t data_offset;
	uint64_t data_size;
	uint32_t flags;								// 0x80000000 (overwrite), 0x3 (file), 0x4 (dir)
	uint32_t padding;
} PKG_ITEM_RECORD_t; // 32 bytes

typedef struct {
	uint8_t data_sha1[0x20];					// sha1_digest of everything above
} PKG_FOOTER_t; // 32 bytes

typedef struct {
	// structs
	PKG_HEADER_t header;
	DIGEST_t header_digest;
	PKG_FIRST_METADATA_t first_metadata;
	PKG_SECOND_METADATA_t second_metadata;
	DIGEST_t metadata_digest;
	uint8_t* data;
	DIGEST_t data_digest;
	PKG_FOOTER_t footer;
} PKG_t;

int pkg_write(PKG_t pkg, std::string content_file_dir, std::string pkg_file_name);