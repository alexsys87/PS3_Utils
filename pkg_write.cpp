#include "ConsoleColor.h"
#include "ConsoleUtils.h"
#include "endianness.h"
#include "pkg_file.h"
#include "sfo_file.h"

#include "optionparser.h"
#include <regex>

enum  optionIndex { UNKNOWN, VERBOSE, APPVER, PKGTYPE, PKGID, PKGDIR, PKGFILE, HELP };
const option::Descriptor usage[] =
{
 {UNKNOWN,   0, "",   "",            option::Arg::None,     "                         " },
 {VERBOSE,   0, "v",  "verbose",     option::Arg::None,     "  -v,--verbose           Verbose output" },
 {APPVER,    0, "a",  "app-ver",     option::Arg::NonEmpty, "  -a,--app-ver           Application version" },
 {PKGTYPE,   0, "t",  "pkg-type",    option::Arg::NonEmpty, "  -t,--pkg-type          Pakage type" },
 {PKGID,     0, "p",  "pkg-cid",     option::Arg::NonEmpty, "  -p,--pkg-cid           Content Id" },
 {PKGDIR,    0, "d",  "pkg-dir",     option::Arg::NonEmpty, "  -d,--pkg-dir           Content folder" },
 {PKGFILE,   0, "f",  "pkg-file",    option::Arg::NonEmpty, "  -f,--pkg-file          Target pakage" },
 {HELP,      0, "h",  "help",        option::Arg::None,     "  -h,--help              Print this help message" },
 {0,0,0,0,0,0}
};

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

		//std::cout << sfo_params->key_table << std::endl;
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

std::string ExtractVer(std::string ver)
{
	const std::regex regex("([0-9]+).([0-9]+)");
	std::cmatch what;

	if (regex_match(ver.c_str(), what, regex)) {
		//std::cout << std::string(what[1].first, what[1].second) << '\n';
		//std::cout << std::string(what[2].first, what[2].second) << '\n';
		std::string str1 = std::string(what[1].first, what[1].second);
		std::string str2 = std::string(what[2].first, what[2].second);

		return ("0x" + str1 + str2);
	}

	return "";
}

int main(int argc, char** argv)
{
	std::string content_id = {};
	std::string pkg_dir = {};
	std::string pkg_file = {};
	std::string app_ver = "0x0100";
	std::string package_type = "0x5E";

	uint32_t package, appver;

	PKG_t pkg = {};

	std::cout << std::endl << std::endl << green << "pkg_write version 0.3" << white << std::endl << std::endl;

	if (argc < 2)
	{
		pkg_dir = GetFixetPath(BrowseFolder(GetFixetPath(std::filesystem::current_path().string())));
		if (pkg_dir.empty())
			return -1;
		else
			pkg_dir = pkg_dir.substr(0, pkg_dir.length() - 1);
		//std::cout << "Selected folder " << pkg_dir << std::endl;
		std::string sfo_path = pkg_dir + "/PARAM.SFO";

		if (!std::filesystem::exists(sfo_path))
			return -1;

		app_ver = ExtractVer(GetFromSfo(sfo_path, "APP_VER"));
		content_id = "EP0000-" + GetFromSfo(sfo_path, "TITLE_ID") + "_00-0000000000000001";
		pkg_file = content_id + ".pkg";
	}
	else
	{
		argc -= (argc > 0); argv += (argc > 0); // skip program name argv[0] if present
		option::Stats  stats(usage, argc, argv);
		std::vector<option::Option> options(stats.options_max);
		std::vector<option::Option> buffer(stats.buffer_max);
		option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

		if (parse.error())
			return -1;

		if (debug)
		{
			for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
				std::cout << "Unknown option: " << std::string(opt->name, opt->namelen) << "\n";

			for (int i = 0; i < parse.nonOptionsCount(); ++i)
				std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << std::endl;
		}

		if (options[HELP] || argc == 0) {
			std::cout << "usage: " << argv[0] << " [-p<contentid>] [-d<pkg-directory>] [-f<target-pkg>] [-a<app-ver>] [-t<pkg-type>]" << std::endl;
			option::printUsage(std::cout, usage);
			return 0;
		}

		for (int i = 0; i < parse.optionsCount(); ++i) {
			option::Option& opt = buffer[i];

			switch (opt.index()) {
			case VERBOSE:
				verbose = true;
				break;
			case APPVER:
				app_ver = options[APPVER].arg;
				break;
			case PKGTYPE:
				package_type = options[PKGTYPE].arg;
				break;
			case PKGID:
				content_id = options[PKGID].arg;
				break;
			case PKGDIR:
				pkg_dir = options[PKGDIR].arg;
				break;
			case PKGFILE:
				pkg_file = options[PKGFILE].arg;
				break;
			case UNKNOWN:
				std::cout << "Unknown option: " << std::string(opt.name, opt.namelen) << "\n";
				break;
			default:
				break;
			}
		}
	}

	if (content_id.length() != 36)
	{
		std::cerr << red << "error: ContentID must be 36 characters" << white << std::endl;
		return 1;
	}

	if (app_ver.length() > 0)
	{
		std::istringstream converter(app_ver);
		converter >> std::hex >> appver;
		std::cout << green << "App version  is: " << white << "0x" << std::hex << appver << std::endl;
	}

	if (package_type.length() > 0)
	{
		std::istringstream converter(package_type);
		converter >> std::hex >> package;
		std::cout << green << "Package Type is: " << white << "0x" << std::hex << package << std::endl;
	}

	// header constants
	pkg.header.magic = be32(0x7f504b47);
	pkg.header.pkg_revision = be16(0x0);
	pkg.header.pkg_type = be16(0x1);
	pkg.header.pkg_metadata_offset = be32(sizeof(PKG_HEADER_t) + sizeof(DIGEST_t));
	pkg.header.pkg_metadata_count = be32(sizeof(PKG_FIRST_METADATA_t) / 8);
	pkg.header.pkg_metadata_size = be32(sizeof(PKG_HEADER_t) + sizeof(DIGEST_t));
	pkg.header.data_offset = be64(sizeof(PKG_HEADER_t) + sizeof(DIGEST_t) + sizeof(PKG_FIRST_METADATA_t) + sizeof(PKG_SECOND_METADATA_t) + sizeof(DIGEST_t));
	std::fill((char*)pkg.header.contentid, (char*)pkg.header.contentid + content_id.length(), 0);
	std::copy(content_id.c_str(), content_id.c_str() + content_id.length(), (char*)pkg.header.contentid);

	pkg.first_metadata.DrmType.packet_id = be32(0x1);
	pkg.first_metadata.DrmType.data_size = be32(0x4);
	pkg.first_metadata.DrmType.data		 = be32(0x3); // 0/5/6/7/8/9/0xA/0xB/0xC (unknown), 0x1 (network), 0x2 (local), 0x3 (DRMfree no license), 0x4 (PSP), 0xD (DRMfree with license), 0xE (PSM)

	pkg.first_metadata.ContentType.packet_id = be32(0x2);
	pkg.first_metadata.ContentType.size      = be32(0x4);
	pkg.first_metadata.ContentType.data		 = be32(0x5); // (gameexec) 00 00 00 07

	pkg.first_metadata.PackageType.packet_id = be32(0x3);
	pkg.first_metadata.PackageType.size = be32(0x4);
	pkg.first_metadata.PackageType.data = be32(package); // gameexec  4e - install, 5e - force install

	pkg.first_metadata.PackageSize.packet_id = be32(0x4);
	pkg.first_metadata.PackageSize.size = be32(0x8);
	pkg.first_metadata.PackageSize.data1 = be32(0x0);

	pkg.first_metadata.PackageVersion.packet_id = be32(0x5);
	pkg.first_metadata.PackageVersion.data_size = be32(0x4);
	pkg.first_metadata.PackageVersion.author = be16(0x1732);  // make_package_npdrm revision
	pkg.first_metadata.PackageVersion.version = be16(0x0100); // package.conf PACKAGE_VERSION

	pkg.second_metadata.packet_id_7 = be32(0x7);
	pkg.second_metadata.size_18 = be32(0x18);

	pkg.second_metadata.packet_id_8 = be32(0x8);
	pkg.second_metadata.size_8 = be32(0x8);
	pkg.second_metadata.flags = 0x85;
	pkg.second_metadata.fw_major = 0x3;
	pkg.second_metadata.fw_minor = be16(0x4000);
	pkg.second_metadata.pkg_version = be16(0x0100);
	pkg.second_metadata.app_version = be16(appver); // версия приложения

	pkg.second_metadata.unknown91 = be32(0x9);
	pkg.second_metadata.unknown92 = be32(0x8);
	pkg.second_metadata.unknown93 = be32(0x0);
	pkg.second_metadata.unknown94 = be32(0x0);

	pkg_write(pkg, pkg_dir, pkg_file);

	return 0;
}