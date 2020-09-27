
#include "download.h"
#include "tinyxml2.h"
#include "endianness.h"
#include "ConsoleColor.h"
#include "ConsoleUtils.h"
#include "pkg_file.h"
#include "sfo_file.h"
#include "Files.h"
#include "extractps3iso.h"
#include "crc32.h"

#include <thread>

#include "optionparser.h"

extern int make_npdata(int argc, char** argv);
extern int pkg_read(int argc, char* argv[]);


enum  optionIndex { UNKNOWN, VERBOSE, INDIR, OUTDIR, NOPKG, PATCH, HELP };
const option::Descriptor usage[] =
{
 {UNKNOWN,   0, "",   "",            option::Arg::None,     "                         " },
 {VERBOSE,   0, "v",  "verbose",     option::Arg::None,     "  -v,--verbose           Verbose output" },
 {INDIR,     0, "i",  "in-dir",		 option::Arg::NonEmpty,	"  -i,--in-dir            Input iso file or dir" },
 {OUTDIR,    0, "o",  "out-dir",	 option::Arg::NonEmpty,	"  -t,--out-dir           Output pkg file or dir" },
 {NOPKG,     0, "n",  "not-pkg",     option::Arg::None,		"  -n,--not-pkg           No pack pkg" },
 {PATCH,     0, "p",  "patch",		 option::Arg::NonEmpty,	"  -p,--patch             Patch file" },
 {HELP,      0, "h",  "help",        option::Arg::None,     "  -h,--help              Print this help message" },
 {0,0,0,0,0,0}
};

std::string LoadContentId(std::string path)
{
	//
	// Open the package file.
	//
	std::ifstream pkgFile(path, std::ifstream::in | std::ifstream::binary);

	if (!pkgFile)
	{
		std::cout << red << "Error: Could not open the package file. (" << path << ")" << white << std::endl;

		return "";
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

int ParseUpdateXml(char* xmlurl, char* data)
{
	tinyxml2::XMLDocument doc;

	std::vector<char> v(data, data + strlen(data));

	std::wstring utf8data(v.begin(), v.end());

	std::string xml = ws2s(utf8data);

	if (debug)
	{
		std::cout << std::endl << "XML file:"  << std::endl << xml << std::endl << std::endl;
	}

	doc.Parse(xml.c_str());

	tinyxml2::XMLElement* titlepatch = doc.FirstChildElement();
	if (titlepatch) {
		const char* titleid; const char* status;
		titlepatch->QueryStringAttribute("titleid", &titleid);
		titlepatch->QueryStringAttribute("status", &status);
		std::cout << "titleid:        " << titleid << std::endl;
		std::cout << "status:         " << status << std::endl;

		tinyxml2::XMLElement* tagElement = titlepatch->FirstChildElement();
		if (tagElement)
		{
			//std::cout << tagElement->Name() << std::endl;
			tinyxml2::XMLElement* packageElement = tagElement->FirstChildElement();
			if (packageElement)
			{
				//std::cout << packageElement->Name() << std::endl;
				const char* version; const char* size; const char* sha1sum; const char* url; const char* ps3_system_ver;
				packageElement->QueryStringAttribute("version", &version);
				packageElement->QueryStringAttribute("size", &size);
				packageElement->QueryStringAttribute("sha1sum", &sha1sum);
				packageElement->QueryStringAttribute("url", &url);
				packageElement->QueryStringAttribute("ps3_system_ver", &ps3_system_ver);
				std::cout << "version:        " << version << std::endl;
				std::cout << "size:           " << size << std::endl;
				std::cout << "sha1sum:        " << sha1sum << std::endl;
				std::cout << "url:            " << url << std::endl;
				std::cout << "ps3_system_ver: " << ps3_system_ver << std::endl;

				memcpy(xmlurl, url, strlen(url));

				tinyxml2::XMLElement* paramsfoElement = packageElement->FirstChildElement();
				if (paramsfoElement)
				{
					std::cout << "title:          " << paramsfoElement->FirstChildElement()->GetText() << std::endl;
				}
			}
		}
	}

	return doc.ErrorID();
}

std::string TidFromCid(std::string cid)
{
	const std::regex regex("([A-Z0-9]+)-([A-Z0-9]+)_([A-Z0-9]+)-([A-Z0-9]+)");
	std::cmatch what;

	if (regex_match(cid.c_str(), what, regex)) {
		if (debug)
		{
			std::cout << std::string(what[1].first, what[1].second) << std::endl;
			std::cout << std::string(what[2].first, what[2].second) << std::endl;
		}
		return std::string(what[2].first, what[2].second);
	}

	return "";
}

std::string DownloadPatch(std::string t_id)
{
	char* data = new char[16384];
	std::fill(data, data + 16384, 0);

	std::string file_name;

	char* update_link = new char[1024];
	std::fill(update_link, update_link + 1024, 0);

	snprintf(update_link, 1023, "https://a0.ww.np.dl.playstation.net/tpl/np/%s/%s-ver.xml", t_id.c_str(), t_id.c_str());

	std::string xmllink(update_link);
	Url* xmlurl = new Url(xmllink);
	if (DownLoadToRam(xmlurl, data) == 0)
	{
		std::fill(update_link, update_link + 1024, 0);
		ParseUpdateXml(update_link, data);

		if (strlen(update_link) == 0)
		{
			std::cout << "No updates found" << std::endl;

			return "";
		}

		file_name = GetFileName(update_link);

		std::string link(update_link);
		Url* url = new Url(link);
		DownLoad(url, file_name.c_str());
	}

	delete[] update_link;
	delete[] data;

	return file_name;
}

void WriteToPkg(std::string directory, std::string  contentID, std::string pkg_file)
{
	PKG_t pkg = {};

	pkg.header.magic = be32(0x7f504b47);
	pkg.header.pkg_revision = be16(0x0);
	pkg.header.pkg_type = be16(0x1);
	pkg.header.pkg_metadata_offset = be32(sizeof(PKG_HEADER_t) + sizeof(DIGEST_t));
	pkg.header.pkg_metadata_count = be32(sizeof(PKG_FIRST_METADATA_t) / 8);
	pkg.header.pkg_metadata_size = be32(sizeof(PKG_HEADER_t) + sizeof(DIGEST_t));
	pkg.header.data_offset = be64(sizeof(PKG_HEADER_t) + sizeof(DIGEST_t) + sizeof(PKG_FIRST_METADATA_t) + sizeof(PKG_SECOND_METADATA_t) + sizeof(DIGEST_t));
	std::fill((char*)pkg.header.contentid, (char*)pkg.header.contentid + contentID.length(), 0);
	std::copy(contentID.c_str(), contentID.c_str() + contentID.length(), (char*)pkg.header.contentid);

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

	pkg_write(pkg, directory, pkg_file);
}

int main(int argc, char* argv[]) {
	namespace fs = std::filesystem;

	std::string file_name = {};
	std::string dest_dir = {};
	std::string src_dir = {};
	std::string iso_dir = {};
	std::string patch_pkg = {};
	bool nopack = false;

	if (argc < 2)
	{
		file_name = GetFixetPath(BrowseFile("SFO;*.SFO;ISO;*.iso", 0));
		if (file_name.empty())
			return -1;

		if (GetFileName(file_name) == "PARAM.SFO")
		{
			file_name = fs::path(file_name).parent_path().string().replace(file_name.rfind("/PS3_GAME"), file_name.length(), "");
		}

		dest_dir = fs::path(GetFixetPath(argv[0])).parent_path().string() + "/Converted";
		iso_dir = fs::path(GetFixetPath(argv[0])).parent_path().string() + "/ExtractedIso";
		fs::current_path(fs::path(GetFixetPath(argv[0])).parent_path().string()); // set current path
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

		if (options[HELP]) {
			std::cout << "usage: " << argv[0] << " <iso-file or dir> [out-dir] [key]" << std::endl;
			option::printUsage(std::cout, usage);
			return 0;
		}

		for (int i = 0; i < parse.optionsCount(); ++i) {
			option::Option& opt = buffer[i];

			switch (opt.index()) {
			case VERBOSE:
				verbose = true;
				break;
			case UNKNOWN:
				std::cout << "Unknown option: " << std::string(opt.name, opt.namelen) << "\n";
				break;
			case INDIR:
				file_name = options[INDIR].arg;
				break;
			case OUTDIR:
				dest_dir = options[OUTDIR].arg;
				break;
			case NOPKG:
				nopack = true;
				break;
			case PATCH:
				patch_pkg = options[PATCH].arg;
				break;
			default:
				break;
			}
		}
	}

	if (file_name.empty())
		file_name = GetFixetPath(argv[1]);

	if (dest_dir.empty())
		dest_dir = GetFixetPath(fs::current_path().string()) + "/Converted";

	if (iso_dir.empty())
		iso_dir = GetFixetPath(fs::current_path().string()) + "/ExtractedIso";

	src_dir = file_name + "/PS3_GAME";

	if (!fs::exists(dest_dir))
		fs::create_directories(dest_dir);

	std::string ps3_dir = dest_dir + "/PS3_GAME";

	//extract iso file
	if (!fs::is_directory(file_name))
	{
		std::cout << green << "Extract ISO file" << white << std::endl << std::endl;

		ExtractIso(file_name, iso_dir);

		src_dir = iso_dir + "/PS3_GAME";
	}

	std::cout << std::endl << green << "Copy PS3_GAME folder" << white << std::endl << std::endl;

	if (!fs::exists(src_dir))
	{
		std::cout << std::endl << red << src_dir << " not found" << white << std::endl << std::endl;
		return -1;
	}

	CopyDirStructure(src_dir + "/LICDIR",  ps3_dir + "/LICDIR");
	CopyDirStructure(src_dir + "/TROPDIR", ps3_dir + "/TROPDIR");
	CopyDirStructure(src_dir + "/USRDIR",  ps3_dir + "/USRDIR");

	std::vector<std::string> copy_paths;
	ScanFilesRecursive(src_dir, &copy_paths);

	// copy all files
	for (auto const& entry :fs::directory_iterator(src_dir)) {

		std::string file = GetFixetPath(entry.path().string());
		std::string dst = GetFixetPath(dest_dir + "/PS3_GAME/" + GetFileName(file));

		if (!fs::is_directory(file))
		{
			std::cout << white << GetFileName(file) << white << std::endl;

			fs::copy(file, dst, fs::copy_options::overwrite_existing);
		}
	}

	// copy LICDIR
	for (std::vector<std::string>::iterator it = copy_paths.begin(); it != copy_paths.end(); it++)
	{
		if ((it->rfind("/LICDIR") != std::string::npos)) {

			std::cout << white << GetFileName(it->c_str()) << white << std::endl;

			fs::copy(it->c_str(), GetFixetPath(dest_dir + it->substr(it->rfind("/PS3_GAME"), it->length())), fs::copy_options::overwrite_existing);
		}
	}

	// copy TROPDIR
	for (std::vector<std::string>::iterator it = copy_paths.begin(); it != copy_paths.end(); it++)
	{
		if ((it->rfind("/TROPDIR") != std::string::npos)) {

			std::cout << white << GetFileName(it->c_str()) << white << std::endl;

			fs::copy(it->c_str(), GetFixetPath(dest_dir + it->substr(it->rfind("/PS3_GAME"), it->length())), fs::copy_options::overwrite_existing | fs::copy_options::recursive);
		}
	}

	// copy sdat
	for (std::vector<std::string>::iterator it = copy_paths.begin(); it != copy_paths.end(); it++)
	{
		std::string file = GetFixetPath(it->c_str());

		if (ToLow(GetFileExtension(file)).compare("sdat") != 0) continue;

		std::cout << white << GetFileName(it->c_str()) << white << std::endl;

		fs::copy(it->c_str(), GetFixetPath(dest_dir + it->substr(it->rfind("/PS3_GAME"), it->length())), fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	}

	// copy edat
	for (std::vector<std::string>::iterator it = copy_paths.begin(); it != copy_paths.end(); it++)
	{
		std::string file = GetFixetPath(it->c_str());

		if (ToLow(GetFileExtension(file)).compare("edat") != 0) continue;

		std::cout << white << GetFileName(it->c_str()) << white << std::endl;

		fs::copy(it->c_str(), GetFixetPath(dest_dir + it->substr(it->rfind("/PS3_GAME"), it->length())), fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	}

	// Sfo read
	std::cout << std::endl << green << "Read sfo" << white << std::endl << std::endl;
	std::string sfo_path = src_dir + "/PARAM.SFO";

	std::string tid   = GetFromSfo(sfo_path, "TITLE_ID");
	std::string ver   = GetFromSfo(sfo_path, "APP_VER");
	std::string title = GetFromSfo(sfo_path, "TITLE");

	if (tid.empty() || ver.empty() || title.empty())
	{
		std::cout << std::endl << red << "PARAM.SFO read error" << white << std::endl << std::endl;

		return -1;
	}

	std::cout << white << "PARAM.SFO read successfully" << white << std::endl;

	std::string new_dir = dest_dir + "/" + tid;
	std::string new_tid = "NPEB00000"; new_tid.replace(new_tid.begin() + 4, new_tid.end(), tid.begin() + 4, tid.end());
	new_dir.replace(new_dir.rfind('/') + 1, new_dir.rfind('/') + 5, new_tid);

	// if exist remove it
	if (fs::exists(new_dir))
		fs::remove_all(new_dir);

	if (fs::exists(ps3_dir))
		fs::rename(ps3_dir, new_dir);

	std::string tid_dir = {};

	if (patch_pkg.empty())
	{
		// download patch
		std::cout << std::endl << green << "Download patch" << white << std::endl << std::endl;
		patch_pkg = DownloadPatch(tid);
		tid_dir = dest_dir + "/" + tid;

		if (patch_pkg.empty())
		{
			std::cout << std::endl << red << "Patch not found, abort" << white << std::endl << std::endl;

			return -1;
		}
	}

	if (!fs::exists(tid_dir))
		fs::create_directories(tid_dir);

	// mast be fixed

	//char cmd_line[1024] = {};
	//snprintf(cmd_line, 1023, "pkg_read.exe -e \"%s\" \"%s\"", patch_pkg.c_str(), tid.c_str());
	//std::string ret = exec(cmd_line);
	//if (verbose)
		//std::cout << white << ret << white << std::endl;

	std::cout << std::endl << green << "Extracting patch" << white << std::endl << std::endl;

	char* pkg_read_arg[10] = { (char*)"pkg_read", (char*)"-e", (char*)patch_pkg.c_str(), (char*)tid.c_str(), NULL,NULL,NULL,NULL,NULL,NULL };

	pkg_read(4, pkg_read_arg);

	if (fs::exists(tid))
	{
		fs::copy(tid, tid_dir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);

		fs::remove_all(tid);
	}
	//

	// copy EBOOT.BIN
	std::cout << std::endl << green << "Copy EBOOT.BIN" << white << std::endl << std::endl;
	std::string eboot     = tid_dir + "/USRDIR/EBOOT.BIN";
	std::string new_eboot = new_dir + "/USRDIR/EBOOT.BIN";

	if (fs::exists(eboot))
		fs::copy_file(eboot, new_eboot, fs::copy_options::overwrite_existing);
	else
	{
		std::cout << std::endl << red << "EBOOT.BIN not found, abort" << white << std::endl << std::endl;

		return -1;
	}

	std::cout << white << "EBOOT.BIN copied successfully" << white << std::endl;

	// find all files and sign to sdat
	std::cout << std::endl << green << "Sign files: " << white << std::endl << std::endl;
	std::string usrdir = src_dir + "/USRDIR";
	std::vector<std::string> file_paths;
	ScanFilesRecursive(usrdir, &file_paths);

	for (std::vector<std::string>::iterator it = file_paths.begin(); it != file_paths.end(); it++)
	{
		std::string sign_file_name = GetFixetPath(new_dir + it->substr(it->rfind("/USRDIR"), it->length()));

		if (GetFileName(sign_file_name).compare("EBOOT.BIN") == 0) continue; // Do not process EBOOT.BIN

		if (ToLow(GetFileExtension(sign_file_name)).compare("edat") == 0) continue; // Do not process edat files
		if (ToLow(GetFileExtension(sign_file_name)).compare("sdat") == 0) continue; // Do not process sdat files

		std::cout << white << GetFileName(it->c_str()) << white << std::endl;

		//snprintf(cmd_line, 1023, "make_npdata.exe -v -e \"%s\" \"%s\" 0 1 3 0 16", it->c_str(), sign_file_name.c_str());
		//std::string ret = exec(cmd_line);
		//if (verbose)
		//	std::cout << white << ret << white << std::endl;

		char* arg_array[20] = { (char*)"make_npdata", (char*)"-v", (char*)"-e", (char*)it->c_str(), (char*)sign_file_name.c_str(), (char*)"0", (char*)"1", (char*)"3", (char*)"0", (char*)"16", NULL,NULL,NULL,NULL,NULL };

		StdOutRedirect stdoutRedirect(1024);
		stdoutRedirect.Start();
		//make_npdata(10, arg_array);
		std::thread* t = new std::thread(make_npdata, 10, arg_array); t->join();
		stdoutRedirect.Stop();

		if (verbose)
		{
			char szBuffer[1024];
			int nOutRead = stdoutRedirect.GetBuffer(szBuffer, 1024);
			if (nOutRead)
				std::cout << white << szBuffer << white << std::endl;
		}

		if (!fs::exists(sign_file_name))
		{
			std::cout << red << GetFileName(sign_file_name) << " not signed" << white << std::endl;
			return -1;
		}
	}

	// Lic dir
	std::cout << std::endl << green << "Sign lic.dat " << white << std::endl << std::endl;
	std::string licdat_file = """" + new_dir + "/LICDIR/LIC.DAT" + """";
	std::string licedat_file = """" + new_dir + "/LICDIR/LIC.EDAT" + """";
	std::string contentID = "EP0000-" + new_tid + "_00-0000000000000001";

	if (!fs::exists(licdat_file))
	{
		//generate licdat
		std::cout << white << "LIC.DAT not found generate it" << white << std::endl;

		std::vector<char> licbuffer(0x10000);
		std::vector<uint8_t> licheader = { 0x50, 0x53, 0x33, 0x4c, 0x49, 0x43, 0x44, 0x41, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00,
										   0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01 };

		std::vector<char> lictid = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };

		std::fill(licbuffer.begin(), licbuffer.end(), 0x00);
		std::copy(licheader.data(), licheader.data() + licheader.size(), licbuffer.begin());
		std::copy(tid.c_str(), tid.c_str() + tid.size(), lictid.begin() + 1);
		std::copy(lictid.data(), lictid.data() + lictid.size(), licbuffer.begin() + 0x800);

		uint32_t crc = be32(crc32(licbuffer.data(), 0x900));

		std::vector<std::uint8_t> crc_data((std::uint8_t*) & crc, (std::uint8_t*) & (crc)+sizeof(std::uint32_t));

		std::copy(crc_data.data(), crc_data.data() + crc_data.size(), licbuffer.begin() + 0x20);

		if (!fs::exists(new_dir + "/LICDIR"))
			fs::create_directories(new_dir + "/LICDIR");


		std::ofstream licFile(licdat_file, std::ofstream::binary);
		licFile.write(licbuffer.data(), licbuffer.size());
		licFile.close();
	}

	//char cmd_line[1024] = {};
	//snprintf(cmd_line, 1023, "make_npdata.exe -v -e \"%s\" \"%s\" 1 1 3 0 16 3 00 %s 1", licdat_file.c_str(), licedat_file.c_str(), contentID.c_str());
	//std::string ret = exec(cmd_line);
	//if (verbose)
	//	std::cout << white << ret << white << std::endl;

	static char* licdat_arg[20] = { (char*)"make_npdata", (char*)"-v", (char*)"-e", (char*)licdat_file.c_str(), (char*)licedat_file.c_str(), (char*)"1", (char*)"1", (char*)"3", (char*)"0", (char*)"16", (char*)"3", (char*)"00", (char*)contentID.c_str(), (char*)"1", NULL, NULL, NULL, NULL, NULL, NULL };

	StdOutRedirect stdoutNPDRedirect(1024);
	stdoutNPDRedirect.Start();
	//make_npdata(14, licdat_arg);
	std::thread* t = new std::thread(make_npdata, 14, licdat_arg); t->join();
	stdoutNPDRedirect.Stop();

	if (verbose)
	{
		char szBuffer[1024];
		int nOutRead = stdoutNPDRedirect.GetBuffer(szBuffer, 1024);
		if (nOutRead)
			std::cout << white << szBuffer << white << std::endl;
	}

	RemoveFile(licdat_file);

	RemoveDirRecursive(iso_dir);

	if (nopack)
		return 0;

	// pkg
	std::string pkg_file_name = contentID + ".pkg";

	WriteToPkg(new_dir, contentID, pkg_file_name);

	RemoveDirRecursive(new_dir);
	RemoveDirRecursive(tid_dir);
	RemoveDirRecursive("Converted");

	return 0;

}

