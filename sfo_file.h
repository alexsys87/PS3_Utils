#pragma once
#include <stdint.h>
#include <string>

// from https://www.psdevwiki.com/ps3/PARAM.SFO
typedef struct
{
	uint32_t magic; /************ Always PSF */ // 00 50 53 46
	uint32_t version; /********** Usually 1.1 */
	uint32_t key_table_start; /** Start offset of key_table */
	uint32_t data_table_start; /* Start offset of data_table */
	uint32_t tables_entries; /*** Number of entries in all tables */
} SFO_HEADER_t;

typedef struct
{
	uint16_t key_offset; /*** param_key offset (relative to start offset of key_table) */
	uint16_t data_fmt; /***** param_data data type */
	uint32_t data_len; /***** param_data used bytes */
	uint32_t data_max_len; /* param_data total bytes */
	uint32_t data_offset; /** param_data offset (relative to start offset of data_table) */
} SFO_INDEX_TABLE_ENTRY_t;

typedef struct
{
	char* key_table;
	char* data_table;
} SFO_PARAM_t;

std::string GetFromSfo(std::string path, std::string param);
void PrintSfo(std::string path, std::string param);