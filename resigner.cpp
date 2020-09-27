
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <sstream>

#include <filesystem>
#include <regex>

#include "aes.h"

#include "endianness.h"
#include "ConsoleColor.h"
#include "ConsoleUtils.h"
#include "Files.h"
#include "pkg_file.h"

static uint8_t rap_init_key[0x10] =
{
	0x86, 0x9F, 0x77, 0x45, 0xC1, 0x3F, 0xD8, 0x90, 0xCC, 0xF2, 0x91, 0x88, 0xE3, 0xCC, 0x3E, 0xDF
};

static uint8_t rap_pbox[0x10] =
{
	0x0C, 0x03, 0x06, 0x04, 0x01, 0x0B, 0x0F, 0x08, 0x02, 0x07, 0x00, 0x05, 0x0A, 0x0E, 0x0D, 0x09
};

static uint8_t rap_e1[0x10] =
{
	0xA9, 0x3E, 0x1F, 0xD6, 0x7C, 0x55, 0xA3, 0x29, 0xB7, 0x5F, 0xDD, 0xA6, 0x2A, 0x95, 0xC7, 0xA5
};

static uint8_t rap_e2[0x10] =
{
	0x67, 0xD4, 0x5D, 0xA3, 0x29, 0x6D, 0x00, 0x6A, 0x4E, 0x7C, 0x53, 0x7B, 0xF5, 0x53, 0x8C, 0x74
};

static bool rap2rif(uint8_t* rap, uint8_t* rif)
{
	
	aes_context aes_ctxt;
	int round_num;
	int i;

	if (rap == NULL)
		return false;

	aes_setkey_dec(&aes_ctxt, rap_init_key, 128);
	aes_crypt_ecb(&aes_ctxt, AES_DECRYPT, rap, rap);

	for (round_num = 0; round_num < 5; ++round_num)
	{
		for (i = 0; i < 16; ++i)
		{
			int p = rap_pbox[i];
			rap[p] ^= rap_e1[p];
		}
		for (i = 15; i >= 1; --i)
		{
			int p = rap_pbox[i];
			int pp = rap_pbox[i - 1];
			rap[p] ^= rap[pp];
		}
		int o = 0;
		for (i = 0; i < 16; ++i)
		{
			int p = rap_pbox[i];
			uint8_t kc = rap[p] - o;
			uint8_t ec2 = rap_e2[p];
			if (o != 1 || kc != 0xFF)
			{
				o = kc < ec2 ? 1 : 0;
				rap[p] = kc - ec2;
			}
			else if (kc == 0xFF)
				rap[p] = kc - ec2;
			else
				rap[p] = kc;
		}
	}

	memcpy(rif, rap, 0x10);

	return true;
}

//void find_file_with_ext(std::string directory, std::string ext, std::vector<std::string>* found_files, std::vector<std::string> *file_paths)
//{
//	for (std::vector<std::string>::iterator ti = file_paths->begin(); ti != file_paths->end(); ti++)
//	{
//		std::string file_ext = ToLow(GetFileExtension(ti->c_str()));
//
//		if (file_ext.compare(ext) == 0)
//			found_files->push_back(ti->c_str());
//	}
//}

std::string find_klicense(std::string klic_file, std::string c_id, std::string t_id)
{
	std::string klic;
	std::ifstream file(klic_file);
	if (file.is_open()) {
		std::string line;
		while (std::getline(file, line)) {

			if (line.find(c_id) != std::string::npos)
			{
				klic = line.substr(0, 0x20);
				std::cout << green << "klicense found for: " << c_id << white << std::endl;
				file.close();
				return klic;
			}

			if (line.find(t_id) != std::string::npos)
			{
				klic = line.substr(0, 0x20);
				std::cout << green << "klicense found for: " << t_id << white << std::endl;
				file.close();
				return klic;
			}

		}

		file.close();
	}

	std::cout << red << "klicense not found" << white << std::endl;

	return klic;
}

std::string TidFromCid(std::string cid)
{
	const std::regex regex("([A-Z0-9]+)-([A-Z0-9]+)_([A-Z0-9]+)-([A-Z0-9]+)");
	std::cmatch what;

	if (regex_match(cid.c_str(), what, regex)) {
		return std::string(what[2].first, what[2].second);
	}

	std::cout << red << "Invalid Title id" << cid << white << std::endl;

	return NULL;
}

std::string LoadContentId(std::string path)
{
	//
	// Open the package file.
	//
	std::ifstream pkgFile(path, std::ifstream::in | std::ifstream::binary);

	if (!pkgFile)
	{
		std::cout << red << "Error: Could not open the package file. (" << path << ")" << white << std::endl;

		return NULL;
	}

	//
	// Read in the package header.
	//

	PKG_HEADER_t pkgHeader;

	pkgFile.seekg(0);
	pkgFile.read((char*)&pkgHeader, sizeof(PKG_HEADER_t));
	pkgFile.close();

	return (char*)pkgHeader.contentid;

}

class psn_stuff {
public:
	std::string pkg_id;
	std::string name;
	std::string type;
	std::string region;
	std::string url;
	std::string rap_name;
	std::string rap_value;
	std::string Desc;
	std::string Posted;

	std::string extract_name(std::string& name);
};

std::string psn_stuff::extract_name(std::string& name) {
	return name.substr(0, name.rfind("."));
}

//bool replace(std::string& str, const std::string& from, const std::string& to) {
//	size_t start_pos = str.find(from);
//	if (start_pos == std::string::npos)
//		return false;
//	str.replace(start_pos, from.length(), to);
//	return true;
//}

std::string LoadRapFromDatabase(std::string path, std::string t_id)
{
	psn_stuff* psn = new psn_stuff();

	std::ifstream file(path);

	std::regex field_regex("(\"([^\"]*)\"|([^;]*))(;|$)");

	std::cout << green << "parsing database: " << white << std::endl;
	std::string line;
	while (std::getline(file, line))
	{
		if (line.find(t_id) != std::string::npos)
		{
			auto i = 0;
			for (auto it = std::sregex_iterator(line.begin(), line.end(), field_regex);
				it != std::sregex_iterator();
				++it, ++i)
			{
				auto match = *it;
				auto extracted = match[2].length() ? match[2].str() : match[3].str();
				//std::cout << "column[" << i << "]: " << extracted << std::endl;
				switch (i)
				{
				case 0:
					psn->pkg_id = extracted;
					break;
				case 1:
					psn->name = extracted;
					break;
				case 2:
					psn->type = extracted;
					break;
				case 3:
					psn->region = extracted;
					break;
				case 4:
					psn->url = extracted;
					break;
				case 5:
					psn->rap_name = extracted;
					break;
				case 6:
					psn->rap_value = extracted;
					break;
				case 7:
					psn->Desc = extracted;
					break;
				case 8:
					psn->Posted = extracted;
					break;
				}
			}
			//std::cout << std::endl;

			//replace(psn->Desc, "/n", " ");
			//replace(psn->Desc, ",", " ");

			std::cout << green << "Rap found for: " << t_id << white << std::endl;

			file.close();
			return psn->rap_value;
		}
	}

	std::cout << red << "Rap not found for: " << t_id << white << std::endl;

	file.close();

	return NULL;
}

void WriteToPkg(std::string path, std::string directory, std::string pkg_file)
{
	PKG_t pkg = {};
	PKG_t pkg_orig = {};
	//
	// Open the package file.
	//
	std::ifstream pkgFile(path, std::ifstream::in | std::ifstream::binary);

	if (!pkgFile)
	{
		std::cout << red << "Error: Could not open the package file. (" << path << ")" << white << std::endl;
	}

	//
	// Read in the package.
	//

	pkgFile.seekg(0);
	pkgFile.read((char*)&pkg_orig.header, sizeof(PKG_HEADER_t));
	pkgFile.read((char*)&pkg_orig.header_digest, sizeof(DIGEST_t));
	pkgFile.read((char*)&pkg_orig.first_metadata, sizeof(PKG_FIRST_METADATA_t));
	pkgFile.read((char*)&pkg_orig.second_metadata, sizeof(PKG_SECOND_METADATA_t));
	pkgFile.close();

	pkg.header.magic = be32(0x7f504b47);
	pkg.header.pkg_revision = be16(0x0);
	pkg.header.pkg_type = be16(0x1);
	pkg.header.pkg_metadata_offset = be32(sizeof(PKG_HEADER_t) + sizeof(DIGEST_t));
	pkg.header.pkg_metadata_count = be32(sizeof(PKG_FIRST_METADATA_t) / 8);
	pkg.header.pkg_metadata_size = be32(sizeof(PKG_HEADER_t) + sizeof(DIGEST_t));
	pkg.header.data_offset = be64(sizeof(PKG_HEADER_t) + sizeof(DIGEST_t) + sizeof(PKG_FIRST_METADATA_t) + sizeof(PKG_SECOND_METADATA_t) + sizeof(DIGEST_t));
	std::fill((char*)pkg.header.contentid, (char*)pkg.header.contentid + 0x30, 0);
	std::copy(directory.c_str(), directory.c_str() + directory.length(), (char*)pkg.header.contentid);

	pkg.first_metadata.DrmType.packet_id = be32(0x1);
	pkg.first_metadata.DrmType.data_size = be32(0x4);
	pkg.first_metadata.DrmType.data = be32(0x3); // 0/5/6/7/8/9/0xA/0xB/0xC (unknown), 0x1 (network), 0x2 (local), 0x3 (DRMfree no license), 0x4 (PSP), 0xD (DRMfree with license), 0xE (PSM)

	pkg.first_metadata.ContentType.packet_id = be32(0x2);
	pkg.first_metadata.ContentType.size = be32(0x4);
	pkg.first_metadata.ContentType.data = be32(0x5); // (gameexec) 00 00 00 07

	pkg.first_metadata.PackageType.packet_id = be32(0x3);
	pkg.first_metadata.PackageType.size = be32(0x4);
	pkg.first_metadata.PackageType.data = be32(0x4e); // gameexec  4e - install, 5e - force install

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
	pkg.second_metadata.app_version = be16(0x0100); // версия приложения

	pkg.second_metadata.unknown91 = be32(0x9);
	pkg.second_metadata.unknown92 = be32(0x8);
	pkg.second_metadata.unknown93 = be32(0x0);
	pkg.second_metadata.unknown94 = be32(0x0);

	// version increment?
	pkg.second_metadata.app_version = pkg_orig.second_metadata.app_version; // версия приложения

	pkg_write(pkg, directory, pkg_file);
}

std::string get_elf_type(std::string *cmd)
{
	std::string apptype;

	//std::cout << *cmd << std::endl;

	//std::istringstream s(*cmd);
	//std::string line;
	//while (std::getline(s, line)) {
	//	std::cout << line << std::endl;
	//	if ((line.find("Type") != std::string::npos) && (line.find("[EXEC]") != std::string::npos))
	//	{
	//		apptype = "EXEC";
	//		break;
	//	}
	//}

	//if (cmd->find("Type                   [EXEC]") != std::string::npos)   apptype = "EXEC";
	//if (cmd->find("Type                   [SPRX]") != std::string::npos)   apptype = "EXEC";

	if (cmd->find("Type     0x00000000") != std::string::npos)   apptype = "SPRX";
	if (cmd->find("Type     0x00000001") != std::string::npos)   apptype = "EXEC";
	if (cmd->find("Type     0x00000020") != std::string::npos)   apptype = "USPRX";
	if (cmd->find("Type     0x00000021") != std::string::npos)   apptype = "UEXEC";
	if (cmd->find("Type       [EXEC]")   != std::string::npos)   apptype = "EXEC";
	if (cmd->find("Type       [UEXEC]")  != std::string::npos)   apptype = "UEXEC";
	if (cmd->find("Type       [SPRX]")   != std::string::npos)   apptype = "SPRX";
	if (cmd->find("Type       [USPRX]")  != std::string::npos)   apptype = "USPRX";

	return apptype;
}

int main(int argc, char* argv[]) {

	if (argc != 2) // argc should be 2 for correct execution
		std::cout << "usage: " << std::filesystem::path(argv[0]).filename() << " <pkg-file>" << std::endl;
	else {

		uint8_t rif[0x10] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		uint8_t rap[0x10] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

		char cmd_line[1024] = {};

		std::string file_name = GetFixetPath(argv[1]);
		std::string c_id = LoadContentId(file_name);
		std::string t_id = TidFromCid(c_id);
		std::vector<uint8_t> v = HexToBytes(LoadRapFromDatabase("database", t_id));
		std::string klicense = find_klicense("klicense.txt", c_id, t_id);
		std::string temp_path = "TEMP/";

		std::copy(v.begin(), v.end(), rap);

		//std::cout << "Title id is: " << t_id << std::endl;
		std::string dir = GetFixetPath(std::filesystem::current_path().string().append("/").append(c_id));
		std::vector<std::string> file_paths;
		std::vector<std::string> sprx_file_paths;
		std::vector<std::string> self_file_paths;
		std::vector<std::string> edat_file_paths;
		std::vector<std::string> sdat_file_paths;

		std::string srap = BytesToHex(rap, 0x10);

		rap2rif(rap, rif);

		std::string srif = BytesToHex(rif, 0x10);

		std::cout << "RAP: " << std::hex << srap << std::endl;
		std::cout << "RIF: " << std::hex << srif << std::endl;

		snprintf(cmd_line, 1023, "pkg_read.exe -s \"%s\" %s", file_name.c_str(), t_id.c_str());

		std::system(cmd_line);

		std::cout << "Scan directory: "<< dir << std::endl;

		ScanFilesRecursive(dir, &file_paths);

		std::filesystem::create_directories(temp_path);

		// find and resign EBOOT.BIN
		std::cout << green <<  "Resign EBOOT.BIN" << white << std::endl;
		std::string eboot = FindFile("EBOOT.BIN", &file_paths);
		std::string boot = temp_path;

		snprintf(cmd_line, 1023, "scetool.exe --verbose --np-klicensee=%s --print-infos \"%s\"", srif.c_str(), eboot.c_str());

		std::string cmd_ret = exec(cmd_line);

		std::string apptype = get_elf_type(&cmd_ret);

		//std::cout << grey << cmd_ret << white << std::endl;

		snprintf(cmd_line, 1023, "scetool.exe -v -l %s -d \"%s\" \"%s\"", srif.c_str(), eboot.c_str(), boot.append("BOOT.BIN").c_str());

		std::system(cmd_line);

		if (std::filesystem::exists(boot))
		{

			snprintf(cmd_line, 1023, "scetool.exe --verbose --skip-sections=FALSE --sce-type=SELF --compress-data=FALSE --key-revision=0A \
									  --self-app-version=0001000000000000 --self-auth-id=1010000001000003 --self-vendor-id=01000002 \
									  --self-ctrl-flags=0000000000000000000000000000000000000000000000000000000000000000 \
									  --self-cap-flags=00000000000000000000000000000000000000000000003B0000000100020000 --self-type=NPDRM \
									  --self-fw-version=0003005500000000 --np-license-type=FREE --np-app-type=%s --np-content-id=%s \
									  --np-real-fname=%s --encrypt \"%s\" \"%s\"", apptype.c_str(), c_id.c_str(), GetFileName(eboot).c_str(), boot.c_str(), eboot.c_str());
			std::system(cmd_line);

			std::filesystem::remove("BOOT.BIN");
		}
		else
		{
			std::cout << red << "EBOOT.BIN not resigned" << white << std::endl;
		}


		// find and resign sprx files
		std::cout << green << "Resign sprx files: " << white << std::endl;
		FindFileWithExt(dir, "sprx", &sprx_file_paths, &file_paths);

		for (std::vector<std::string>::iterator it = sprx_file_paths.begin(); it != sprx_file_paths.end(); it++)
		{
			std::cout << white << GetFileName(it->c_str()) << white << std::endl;

			snprintf(cmd_line, 1023, "scetool.exe --verbose --np-klicensee=%s --print-infos \"%s\"", klicense.c_str(), it->c_str());

			std::string cmd_ret = exec(cmd_line);

			std::string apptype = get_elf_type(&cmd_ret);

			//std::cout << grey << cmd_ret << white << std::endl;

			std::string dec_file_name = temp_path; dec_file_name = dec_file_name.append(ExtractName(GetFileName(it->c_str())).append(".").append("prx"));

			snprintf(cmd_line, 1023, "scetool.exe --verbose --np-klicensee=%s --decrypt \"%s\" \"%s\"", srif.c_str(), it->c_str(), dec_file_name.c_str());

			std::system(cmd_line);

			if (std::filesystem::exists(dec_file_name))
			{
				snprintf(cmd_line, 1023, "scetool.exe --verbose --skip-sections=FALSE --sce-type=SELF --compress-data=FALSE --key-revision=0A  \
												  --self-app-version=0001000000000000 --self-auth-id=1010000001000003 --self-vendor-id=01000002 \
												  --self-ctrl-flags=0000000000000000000000000000000000000000000000000000000000000000 \
												  --self-cap-flags=00000000000000000000000000000000000000000000003B0000000100020000 --self-type=NPDRM \
												  --self-fw-version=0003005500000000 --np-license-type=FREE --np-app-type=%s \
												  --np-klicensee=%s --np-content-id=%s --np-real-fname=%s --encrypt \"%s\" \"%s\"",
												  apptype.c_str(), klicense.c_str(), c_id.c_str(), GetFileName(it->c_str()).c_str(), dec_file_name.c_str(), it->c_str());

				std::system(cmd_line);

				std::filesystem::remove(dec_file_name);
			}
			else
			{
				std::cout << red << GetFileName(it->c_str()) << " not resigned" << white << std::endl;
			}
		}

		// find and resign self files
		std::cout << green << "Resign self files: " << white << std::endl;
		FindFileWithExt(dir, "self", &self_file_paths, &file_paths);

		for (std::vector<std::string>::iterator it = self_file_paths.begin(); it != self_file_paths.end(); it++)
		{
			std::cout << white << GetFileName(it->c_str()) << white << std::endl;

			snprintf(cmd_line, 1023, "scetool.exe --verbose --np-klicensee=%s --print-infos \"%s\"", klicense.c_str(), it->c_str());

			std::string cmd_ret = exec(cmd_line);

			std::string apptype = get_elf_type(&cmd_ret);

			//std::cout << grey << cmd_ret << white << std::endl;

			std::string dec_file_name = temp_path; dec_file_name = dec_file_name.append(ExtractName(GetFileName(it->c_str())).append(".").append("elf"));

			snprintf(cmd_line, 1023, "scetool.exe --verbose --np-klicensee=%s --decrypt \"%s\" \"%s\"", srif.c_str(), it->c_str(), dec_file_name.c_str());

			std::system(cmd_line);

			if (std::filesystem::exists(dec_file_name))
			{
				snprintf(cmd_line, 1023, "scetool.exe --verbose --skip-sections=FALSE --sce-type=SELF --compress-data=FALSE --key-revision=0A \
												  --self-app-version=0001000000000000 --self-auth-id=1010000001000003 --self-vendor-id=01000002 \
												  --self-ctrl-flags=0000000000000000000000000000000000000000000000000000000000000000 \
												  --self-cap-flags=00000000000000000000000000000000000000000000003B0000000100040000 --self-type=NPDRM \
												  --self-fw-version=0003005500000000 --np-license-type=FREE --np-app-type=%s \
												  --np-klicensee=%s --np-content-id=%s --np-real-fname=%s --encrypt \"%s\" \"%s\"",
												apptype.c_str(), klicense.c_str(), c_id.c_str(), GetFileName(it->c_str()).c_str(), dec_file_name.c_str(), it->c_str());

				std::system(cmd_line);

				std::filesystem::remove(dec_file_name);
			}
			else
			{
				std::cout << red << GetFileName(it->c_str()) << " not resigned" << white << std::endl;
			}
		}

		// find and resign edat files
		std::cout << green << "Resign edat files: " << white << std::endl;
		FindFileWithExt(dir, "edat", &edat_file_paths, &file_paths);

		for (std::vector<std::string>::iterator it = edat_file_paths.begin(); it != edat_file_paths.end(); it++)
		{
			std::cout << white << GetFileName(it->c_str()) << white << std::endl;

			std::string dec_file_name = temp_path; dec_file_name = dec_file_name.append(ExtractName(GetFileName(it->c_str())).append(".").append("dat"));

			snprintf(cmd_line, 1023, "make_npdata.exe -v -d \"%s\" \"%s\" 9 %s", it->c_str(), dec_file_name.c_str(), srif.c_str());

			std::system(cmd_line);

			if (std::filesystem::exists(dec_file_name))
			{

				snprintf(cmd_line, 1023, "make_npdata.exe -v -e \"%s\" \"%s\" 1 1 3 0 16 3 00 %s 8 %s", dec_file_name.c_str(), it->c_str(), c_id.c_str(), klicense.c_str());

				std::system(cmd_line);

				std::filesystem::remove(dec_file_name);
			}
			else
			{
				std::cout << red << GetFileName(it->c_str()) << " not resigned" << white << std::endl;
			}
		}

		// find and resign sdat files
		std::cout << green << "Resign sdat files: " << white << std::endl;
		FindFileWithExt(dir, "sdat", &sdat_file_paths, &file_paths);

		for (std::vector<std::string>::iterator it = sdat_file_paths.begin(); it != sdat_file_paths.end(); it++)
		{
			std::cout << white << GetFileName(it->c_str()) << white << std::endl;

			std::string dec_file_name = temp_path; dec_file_name = dec_file_name.append(ExtractName(GetFileName(it->c_str())).append(".").append("dat"));

			snprintf(cmd_line, 1023, "make_npdata.exe -v -d \"%s\" \"%s\" 0", it->c_str(), dec_file_name.c_str());

			std::system(cmd_line);

			if (std::filesystem::exists(dec_file_name))
			{

				snprintf(cmd_line, 1023, "make_npdata.exe -v -e \"%s\" \"%s\" 0 1 3 1 16", dec_file_name.c_str(), it->c_str());

				std::system(cmd_line);

				std::filesystem::remove(dec_file_name);
			}
			else
			{
				std::cout << red << GetFileName(it->c_str()) << " not resigned" << white << std::endl;
			}
		}

		std::string pkg_file_name = ExtractName(file_name).append("_resigned").append(".").append("pkg");

		std::cout << green << "Generate pakage: " << pkg_file_name << white << std::endl;

		//snprintf(cmd_line, 1023, "pkg_write.exe -p %s -a %s -t 0x4e -d %s -f %s", c_id.c_str(), version.c_str(), c_id.c_str(), pkg_file_name.c_str());
		//std::system(cmd_line);

		WriteToPkg(file_name, c_id.c_str(), pkg_file_name.c_str());

		std::filesystem::remove_all(c_id.c_str());
		std::filesystem::remove_all(temp_path);
	}

	return 0;
}
