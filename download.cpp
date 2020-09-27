#pragma once

#include "download.h"
#include "ConsoleColor.h"
#include "ConsoleUtils.h"

#pragma comment (lib, "Wininet.lib")
#pragma comment (lib, "Winmm.Lib")

Url::Url() {
	this->Regex = std::regex("^(.*)://([A-Za-z0-9-.]+)([0-9]+)?(.*)$");
}

Url::Url(std::string& sRawUrl) : Url() {
	this->sRawUrl = sRawUrl;
	this->update(this->sRawUrl);
}

Url& Url::update(std::string& sRawUrl) {
	this->sRawUrl = sRawUrl;
	std::cmatch what;
	if (regex_match(sRawUrl.c_str(), what, Regex)) {
		this->sProtocol = std::string(what[1].first, what[1].second);
		this->sHost = std::string(what[2].first, what[2].second);
		this->sQuery = std::string(what[4].first, what[4].second);
	}
	return *this;
}

char* GetSize(char* pcStr, char* pcBufQuery, DWORD dwLengthBufQuery)
{
		for (int i = 0, k = 0; i < (int)(dwLengthBufQuery + 1); i++)
			if (pcBufQuery[i] != 0)
				pcStr[k++] = pcBufQuery[i];

	return pcStr;
}

// only http work!
int DownLoad(Url* url, const char* szFileName)
{
	ClearScreen();

	std::cout << "HOST: " << url->sHost << std::endl;
	std::cout << "protocol: " << url->sProtocol << std::endl;
	std::cout << "Query: " << url->sQuery << std::endl << std::endl;

	if (url->sHost.empty() || url->sProtocol != "http" || url->sQuery.empty())
	{
		std::cout << "Not valid Url" << std::endl;
		return -1;
	}

	std::wstring wsHost = s2ws(url->sHost);
	std::wstring wsQuery = s2ws(url->sQuery);

	HINTERNET hSession = InternetOpen(
		L"Mozilla/5.0", // User-Agent
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL,
		NULL,
		0);

	HINTERNET hConnect = InternetConnect(
		hSession,
		wsHost.c_str(), // HOST
		INTERNET_DEFAULT_HTTP_PORT, // INTERNET_DEFAULT_HTTPS_PORT, INTERNET_DEFAULT_FTP_PORT
		L"",
		L"",
		INTERNET_SERVICE_HTTP, // INTERNET_SERVICE_FTP
		0,
		0);

	HINTERNET hHttpFile = HttpOpenRequest(
		hConnect,
		L"GET", // METHOD
		wsQuery.c_str(),   // URI
		NULL,
		NULL,
		NULL,
		0, //INTERNET_FLAG_SECURE
		0);

	int Rep = 0;
	while (!HttpSendRequest(hHttpFile, NULL, 0, 0, 0)) {
		std::cout << "HttpSendRequest error : " << GetLastError() << std::endl;

		InternetErrorDlg(
			GetDesktopWindow(),
			hHttpFile,
			ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
			FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
			FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
			FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
			NULL);

		if (Rep > 10)
		{
			InternetCloseHandle(hHttpFile);
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hSession);

			return -1;
		}
		Rep++;
	}

	char cbufQuery[32];

	DWORD dwLengthBufQuery;
	dwLengthBufQuery = sizeof(cbufQuery);

	DWORD dwIndex;
	dwIndex = 0;

	BOOL bQuery;
	bQuery = HttpQueryInfo(
		hHttpFile,
		HTTP_QUERY_CONTENT_LENGTH,
		cbufQuery,
		&dwLengthBufQuery,
		&dwIndex);

	if (!bQuery)
		std::cout << "HttpQueryInfo error : " << GetLastError() << std::endl;

	char* pcStr = new char[(dwLengthBufQuery / 2) + 1];

	std::fill(pcStr, pcStr + (dwLengthBufQuery / 2) + 1, 0);

	LONG64 llFileSize = _atoi64(GetSize(pcStr, cbufQuery, dwLengthBufQuery));

	delete[] pcStr;

	if(llFileSize != 0)
		std::cout << "File size: " << llFileSize / 1024 << " KB " << std::endl;

	char* cbuffer = new char[16384];

	std::ofstream File(szFileName, std::ios::out | std::ios::binary);

	DWORD dwStart = timeGetTime();
	DWORD dwEnd;
	DWORD dwTime = 10;

	LONG64 llBytes = 0;

	COORD pCoord = GetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE));

	FLOAT fSpeed = 0;

	while (true) {
		DWORD dwBytesRead;
		BOOL bRead;

		bRead = InternetReadFile(
			hHttpFile,
			cbuffer,
			16384,
			&dwBytesRead);

		if (dwBytesRead == 0) break;

		if (!bRead) {
			std::cout << "InternetReadFile error : " << GetLastError() << std::endl;
		}
		else {
			File.write(cbuffer, dwBytesRead);
		}

		llBytes += dwBytesRead;

		fSpeed = (float)llBytes;

		fSpeed /= ((float)dwTime) / 1000.0f;

		fSpeed /= 1024.0f;

		if (llBytes % 10 == 0)
		{
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pCoord);

			Progress(llBytes, llFileSize);

			std::cout << std::endl << llBytes / 1024 << " KB " << (llFileSize > 0 ? llFileSize / 1024 : 0) << " / KB " << float(fSpeed) << " KB/s " << std::endl << std::endl;

			std::cout.flush();
		}

		dwEnd = timeGetTime();

		dwTime = dwEnd - dwStart;

		if (dwTime == 0) dwTime = 10;

	}

	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pCoord);

	Progress(llBytes, llFileSize);

	std::cout << std::endl << llBytes / 1024 << " KB " << (llFileSize > 0 ? llFileSize / 1024 : 0) << "/ KB " << float(fSpeed) << " KB/s " << std::endl << std::endl;

	std::cout.flush();

	InternetCloseHandle(hHttpFile);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hSession);

	File.close();

	return 0;
}

// only https work
int DownLoadToRam(Url* url,  char* szBuffer)
{
	std::cout << "HOST: " << url->sHost << std::endl;
	std::cout << "protocol: " << url->sProtocol << std::endl;
	std::cout << "Query: " << url->sQuery << std::endl << std::endl;

	if (url->sHost.empty() || url->sProtocol != "https" || url->sQuery.empty())
	{
		std::cout << "Not valid Url" << std::endl;
		return -1;
	}

	std::wstring wsHost = s2ws(url->sHost);
	std::wstring wsQuery = s2ws(url->sQuery);

	HINTERNET hSession = InternetOpen(
		L"Mozilla/4.0 (compatible; MSIE 6.0;Windows NT 5.1)", // User-Agent
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL,
		NULL,
		0);

	DWORD dwFlags = INTERNET_FLAG_NO_CACHE_WRITE |
		INTERNET_FLAG_KEEP_CONNECTION |
		INTERNET_FLAG_PRAGMA_NOCACHE |
		INTERNET_FLAG_SECURE |
		INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
		INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;

	HINTERNET hConnect = InternetConnect(
		hSession,
		wsHost.c_str(), // HOST
		INTERNET_DEFAULT_HTTPS_PORT, // INTERNET_DEFAULT_HTTPS_PORT, INTERNET_DEFAULT_FTP_PORT
		L"",
		L"",
		INTERNET_SERVICE_HTTP, // INTERNET_SERVICE_FTP
		0,
		0);

	HINTERNET hHttpFile = HttpOpenRequest(
		hConnect,
		L"GET", // METHOD
		wsQuery.c_str(),   // URI
		HTTP_VERSION,
		NULL,
		NULL,
		dwFlags,
		0);

	dwFlags = 0;
	DWORD dwBuffLen = sizeof(dwFlags);
	InternetQueryOption(hHttpFile, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwBuffLen);
	dwFlags = SECURITY_SET_MASK;
	InternetSetOption(hHttpFile, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

	int Rep = 0;
	while (!HttpSendRequest(hHttpFile, NULL, 0, 0, 0)) {
		std::cout << "HttpSendRequest error : " << GetLastError() << std::endl;

		InternetErrorDlg(
			GetDesktopWindow(),
			hHttpFile,
			ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
			FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
			FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
			FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
			NULL);
		if (Rep > 10)
		{
			InternetCloseHandle(hHttpFile);
			InternetCloseHandle(hConnect);
			InternetCloseHandle(hSession);

			return -1;
		}
		Rep++;
	}

	char cbufQuery[32];

	DWORD dwLengthBufQuery;
	dwLengthBufQuery = sizeof(cbufQuery);

	DWORD dwIndex;
	dwIndex = 0;

	BOOL bQuery;
	bQuery = HttpQueryInfo(
		hHttpFile,
		HTTP_QUERY_CONTENT_LENGTH,
		cbufQuery,
		&dwLengthBufQuery,
		&dwIndex);

	if (!bQuery)
		std::cout << "HttpQueryInfo error : " << GetLastError() << std::endl;

	char* pcStr = new char[(dwLengthBufQuery / 2) + 1];

	std::fill(pcStr, pcStr + (dwLengthBufQuery / 2) + 1, 0);

	LONG64 llFileSize = _atoi64(GetSize(pcStr, cbufQuery, dwLengthBufQuery));

	delete[] pcStr;

	char* cbuffer = new char[(uint32_t)llFileSize + 1];

	std::fill(cbuffer, cbuffer + llFileSize + 1, 0);

	LONG64 llBytes = 0;

	while (true) {
		DWORD dwBytesRead;
		BOOL bRead;

		bRead = InternetReadFile(
			hHttpFile,
			cbuffer,
			(DWORD)llFileSize,
			&dwBytesRead);

		if (dwBytesRead == 0) break;

		if (!bRead) {
			std::cout << "InternetReadFile error : " << GetLastError() << std::endl;
		}
		else {
			memcpy(szBuffer, cbuffer, dwBytesRead);
		}

		llBytes += dwBytesRead;

	}


	InternetCloseHandle(hHttpFile);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hSession);

	return 0;
}

