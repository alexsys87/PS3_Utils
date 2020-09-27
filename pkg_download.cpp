#include "download.h"

int main(int argc, char* argv[]) {

	if (argc != 3) // argc should be 2 for correct execution
		std::cout << "usage: <url> <filename>" << std::endl;
	else {
			std::string link(argv[1]);
			Url* url = new Url(link);
			DownLoad(url, argv[2]);
	}
}

 
