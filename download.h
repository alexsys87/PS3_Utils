#pragma once

#include <windows.h>
#include <wininet.h>
#include <stdio.h>

#include <fstream>
#include <iostream>
#include <regex>

class Url {
public:
	std::regex Regex;
	std::string sRawUrl;

	std::string sProtocol;
	std::string sHost;
	std::string sQuery;

	Url();

	Url(std::string& sRawUrl);

	Url& update(std::string& sRawUrl);
};

int DownLoad(Url* url, const char* ccFileName);
int DownLoadToRam(Url* url, char* szBuffer);