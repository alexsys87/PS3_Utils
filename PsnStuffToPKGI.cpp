#include <windows.h>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <regex>

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

	bool has_rap_only;
	std::string filter;

	std::string extract_name(std::string& name);
};

std::string psn_stuff::extract_name(std::string& name) {
	return name.substr(0, name.rfind("."));
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

int main(int argc, char* argv[])
{

	if (argc < 3) // argc should be > 3 for correct execution
		std::cout << "usage: <database> <filename> [--has_rap] [--filter TYPE]" << std::endl;
	else {

		psn_stuff* psn = new psn_stuff();

		for (int i = 1; i < argc; i++) {
			if (0 == _stricmp(argv[i], "--has_rap")) {
				psn->has_rap_only = true;
			}

			if (0 == _stricmp(argv[i], "--filter")) {
				if (i++ == argc) {
					std::cout << "--filter switch requires an argument" << std::endl;
					return E_INVALIDARG;
				}
				psn->filter = std::string(argv[i]);
			}
		}

		std::ifstream file(argv[1]);

		std::ofstream out_txt(argv[2], std::ios::out);

		std::string s;

		std::regex field_regex("(\"([^\"]*)\"|([^;]*))(;|$)");

		while (std::getline(file, s))
		{
			//std::cout << "parsing database: " << s << std::endl;
			auto i = 0;
			for (auto it = std::sregex_iterator(s.begin(), s.end(), field_regex);
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

			replace(psn->Desc, "/n", " ");
			replace(psn->Desc, ",", " ");

			if (!psn->rap_name.empty())
				psn->pkg_id = psn->extract_name(psn->rap_name);

			if ((!psn->filter.empty() && psn->type == psn->filter) || psn->filter.empty())
			{
				if ((psn->has_rap_only && !psn->rap_name.empty()) || !psn->has_rap_only)
				{
					std::cout << psn->pkg_id << " | " << psn->type << " | " << psn->name << std::endl;

					out_txt << psn->pkg_id << "," << "0," << psn->name << "," << psn->Desc << "," << psn->rap_value << "," << psn->url << "," << "0," << "" << std::endl;
				}
			}
		}

		file.close();
		out_txt.close();
	}
}