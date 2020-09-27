
#include <filesystem>

#include "download.h"
#include "tinyxml2.h"
#include "endianness.h"
#include "ConsoleColor.h"
#include "ConsoleUtils.h"
#include "pkg_file.h"

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

int parse_update_xml(char* xmlurl, char* data)
{
	tinyxml2::XMLDocument doc;

	//std::ifstream ifs("NPUB30704-ver.xml");
	//std::string xml_data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	//std::cout << xml_data;
	//doc.Parse(xml_data.c_str());

	std::vector<char> v(data, data + strlen(data));

	std::wstring utf8data(v.begin(), v.end());

	std::string xml = ws2s(utf8data);

	if (debug)
		std::cout << std::endl << xml << std::endl << std::endl;

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
			if (debug)
				std::cout << tagElement->Name() << std::endl;

			tinyxml2::XMLElement* packageElement = tagElement->FirstChildElement();
			if (packageElement)
			{
				if (debug)
					std::cout << packageElement->Name() << std::endl;

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
			std::cout << std::string(what[1].first, what[1].second) << '\n';
			std::cout << std::string(what[2].first, what[2].second) << '\n';
		}
		return std::string(what[2].first, what[2].second);
	}

	return NULL;
}

int main(int argc, char* argv[]) {

	if (argc != 2) // argc should be 2 for correct execution
		std::cout << "usage: " << std::filesystem::path(argv[0]).filename() << " <content-id>" << std::endl;
	else {
		char* data = new char[16384];
		std::fill(data, data + 16384, 0);

		std::string file_name = GetFixetPath(argv[1]);

		//std::string c_id = argv[1];
		std::string c_id = LoadContentId(file_name);
		std::string t_id = TidFromCid(c_id);

		char* update_link = new char[1024];
		std::fill(update_link, update_link + 1024, 0);

		snprintf(update_link, 1023, "https://a0.ww.np.dl.playstation.net/tpl/np/%s/%s-ver.xml", t_id.c_str(), t_id.c_str());

		std::string xmllink(update_link);
		Url* xmlurl = new Url(xmllink);
		DownLoadToRam(xmlurl, data);

		std::fill(update_link, update_link + 1024, 0);
		parse_update_xml(update_link, data);

		if (strlen(update_link) == 0)
		{
			std::cout << "No updates found" << std::endl;

			return 0;
		}

		file_name = GetFileName(update_link);

		std::string link(update_link);
		Url* url = new Url(link);
		DownLoad(url, file_name.c_str());
	}
}

