#include "sfo_file.h"

#include <fstream>

#include "ConsoleColor.h"
#include "ConsoleUtils.h"

std::string GetFromSfo(std::string path, std::string param)
{
	std::string string_data;

	std::ifstream File(path, std::ifstream::binary | std::ifstream::in);

	if (!File)
	{
		std::cout << red << "Error: Could not open the sfo file. (" << path << ")" << white << std::endl;
		return "";
	}

	SFO_HEADER_t sfo_header = { 0, 0, 0, 0, 0 };

	File.read((char*)&sfo_header, sizeof(SFO_HEADER_t));

	SFO_INDEX_TABLE_ENTRY_t* entry = new SFO_INDEX_TABLE_ENTRY_t[sfo_header.tables_entries + 1];
	memset(entry, 0, (sizeof(SFO_INDEX_TABLE_ENTRY_t) * sfo_header.tables_entries) + 1);

	SFO_PARAM_t* sfo_params = new SFO_PARAM_t[sfo_header.tables_entries + 1];
	memset(sfo_params, 0, (sizeof(SFO_PARAM_t) * sfo_header.tables_entries) + 1);

	for (uint32_t i = 0; i < sfo_header.tables_entries; i++)
		File.read((char*)&entry[i], sizeof(SFO_INDEX_TABLE_ENTRY_t));

	for (uint32_t i = 0; i < sfo_header.tables_entries; i++)
	{
		SFO_INDEX_TABLE_ENTRY_t* table_entry = &entry[i]; SFO_INDEX_TABLE_ENTRY_t* table_entry_next = &entry[i + 1];

		uint16_t key_entry_len = table_entry_next->key_offset - table_entry->key_offset;
		sfo_params->data_table = new char[table_entry->data_len];
		sfo_params->key_table = new char[key_entry_len];

		File.seekg((std::streamoff)sfo_header.data_table_start + table_entry->data_offset, std::ifstream::beg);
		File.read((char*)sfo_params->data_table, table_entry->data_len);

		File.seekg((std::streamoff)sfo_header.key_table_start + table_entry->key_offset, std::ifstream::beg);
		File.read((char*)sfo_params->key_table, key_entry_len);
		if (debug)
		{
			std::cout << "SFO keys:" << std::endl << sfo_params->key_table << std::endl;
		}

		if (table_entry->data_fmt == 0x0204)
		{
			std::string key_table = sfo_params->key_table;
			if (key_table.compare(param) == 0)
				string_data = (const char*)sfo_params->data_table;
		}

		delete[] sfo_params->key_table;
		delete[] sfo_params->data_table;
	}

	delete[] entry;
	delete[] sfo_params;

	File.close();

	return string_data;
}

void PrintSfo(std::string path, std::string param)
{
	std::ifstream File(path, std::ifstream::in | std::ifstream::binary);

	if (!File)
	{
		std::cout << red << "Error: Could not open the sfo file. (" << path << ")" << white << std::endl;
		return;
	}

	SFO_HEADER_t sfo_header = { 0, 0, 0, 0, 0 };

	File.read((char*)&sfo_header, sizeof(SFO_HEADER_t));

	SFO_INDEX_TABLE_ENTRY_t* entry = new SFO_INDEX_TABLE_ENTRY_t[sfo_header.tables_entries + 1];
	memset(entry, 0, (sizeof(SFO_INDEX_TABLE_ENTRY_t) * sfo_header.tables_entries) + 1);

	SFO_PARAM_t* sfo_params = new SFO_PARAM_t[sfo_header.tables_entries + 1];
	memset(sfo_params, 0, (sizeof(SFO_PARAM_t) * sfo_header.tables_entries) + 1);

	for (uint32_t i = 0; i < sfo_header.tables_entries; i++)
		File.read((char*)&entry[i], sizeof(SFO_INDEX_TABLE_ENTRY_t));

	for (uint32_t i = 0; i < sfo_header.tables_entries; i++)
	{
		SFO_INDEX_TABLE_ENTRY_t* table_entry = &entry[i]; SFO_INDEX_TABLE_ENTRY_t* table_entry_next = &entry[i + 1];

		uint16_t key_entry_len = table_entry_next->key_offset - table_entry->key_offset;
		sfo_params->data_table = new char[table_entry->data_len];
		sfo_params->key_table = new char[key_entry_len];

		File.seekg((std::streamoff)sfo_header.data_table_start + table_entry->data_offset, std::ifstream::beg);
		File.read((char*)sfo_params->data_table, table_entry->data_len);

		File.seekg((std::streamoff)sfo_header.key_table_start + table_entry->key_offset, std::ifstream::beg);
		File.read((char*)sfo_params->key_table, key_entry_len);

		std::string key_table = sfo_params->key_table;
		if ((key_table.compare(param) == 0) || param.empty())
		{
			std::cout << sfo_params->key_table << " = ";
			if (table_entry->data_fmt == 0x0404)
				std::cout << std::hex << BytesToHex((unsigned char*)sfo_params->data_table, table_entry->data_len) << std::endl;
			else
				std::cout << sfo_params->data_table << std::endl;
		}

		delete[] sfo_params->key_table;
		delete[] sfo_params->data_table;
	}

	File.close();

	delete[] entry;
	delete[] sfo_params;
}