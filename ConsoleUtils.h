#pragma once
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>

#include <filesystem>

#include <windows.h>
#include <wininet.h>
#include <shlobj.h>

extern int verbose;
extern int debug;

#define READ_FD  0
#define WRITE_FD 1

class StdOutRedirect
{
public:
	StdOutRedirect(int bufferSize);
	~StdOutRedirect();

	int Start();
	int Stop();
	int GetBuffer(char* buffer, int size);

private:
	int fdStdOutPipe[2];
	int fdStdOut;
};

COORD GetConsoleCursorPosition(HANDLE hConsoleOutput);
void Progress(LONG64 llBytes, LONG64 llFileSize);
void ClearScreen(void);
uint64_t GetDiskFreeSpaceX(const char* path);
std::string BrowseFolder(std::string saved_path);
std::string BrowseFile(std::string filters, int flags);
std::string exec(const char* cmd);
void FileTouch(std::string name);

void Fill_File(std::ofstream* f, uint8_t patern, size_t block_size, size_t size);

// c++
std::string  ws2s(const std::wstring& s);
std::wstring s2ws(const std::string& s);
std::string BytesToHex(unsigned char* data, int len);
std::vector<uint8_t> HexToBytes(const std::string& hex);

static inline std::string GetFixetPath(std::string file_path) {
	if (file_path.empty()) return "";
	std::replace(file_path.begin(), file_path.end(), '\\', '/'); // replace all '\\' to '/'
	if (file_path.empty()) file_path = "/";
	return file_path;
}

static inline std::string GetFileName(std::string filename) {
	const char* ch = NULL;
	if (filename.empty()) return "";
	ch = strrchr(filename.c_str(), '/') + 1;
	if (ch == NULL) return filename;
	return ch;
}

static inline std::string GetFileExtension(std::string fileName) {
	if (fileName.empty()) return "";
	if (fileName.find_last_of(".") != std::string::npos)
		return fileName.substr(fileName.find_last_of(".") + 1);
	return "";
}

static inline std::string ExtractName(std::string name) {
	if (name.empty()) return "";
	return name.substr(0, name.rfind("."));
}

static inline std::string ToLow(std::string string) {
	std::string data = string;
	if (string.empty()) return "";
	std::transform(data.begin(), data.end(), data.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return data;
}

