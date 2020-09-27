#pragma once
#include "ConsoleUtils.h"

int verbose;
int debug;

//https://stackoverflow.com/questions/35800020/how-do-i-find-the-coordinates-of-the-cursor-in-a-console-window

COORD GetConsoleCursorPosition(HANDLE hConsoleOutput) {
	CONSOLE_SCREEN_BUFFER_INFO cbsi;
	if (GetConsoleScreenBufferInfo(hConsoleOutput, &cbsi))
	{
		return cbsi.dwCursorPosition;
	}
	else
	{
		// The function failed. Call GetLastError() for details.
		COORD invalid = { 0, 0 };
		return invalid;
	}
}

void ClearScreen() {
	HANDLE                     hStdOut;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD                      count;
	DWORD                      cellCount;
	COORD                      homeCoords = { 0, 0 };

	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) return;

	/* Get the number of cells in the current buffer */
	if (!GetConsoleScreenBufferInfo(hStdOut, &csbi)) return;
	cellCount = csbi.dwSize.X * csbi.dwSize.Y;

	/* Fill the entire buffer with spaces */
	if (!FillConsoleOutputCharacter(
		hStdOut,
		(TCHAR)' ',
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Fill the entire buffer with the current colors and attributes */
	if (!FillConsoleOutputAttribute(
		hStdOut,
		csbi.wAttributes,
		cellCount,
		homeCoords,
		&count
	)) return;

	/* Move the cursor home */
	SetConsoleCursorPosition(hStdOut, homeCoords);
}

#define CHECK(a) if ((a)!= 0) return -1;

StdOutRedirect::~StdOutRedirect()
{
	_close(fdStdOut);
	_close(fdStdOutPipe[WRITE_FD]);
	_close(fdStdOutPipe[READ_FD]);
}
StdOutRedirect::StdOutRedirect(int bufferSize)
{
	if (_pipe(fdStdOutPipe, bufferSize, O_TEXT) != 0)
	{
		//treat error eventually
	}
	fdStdOut = _dup(_fileno(stdout));
}

int StdOutRedirect::Start()
{
	fflush(stdout);
	CHECK(_dup2(fdStdOutPipe[WRITE_FD], _fileno(stdout)));
	std::ios::sync_with_stdio();
	setvbuf(stdout, NULL, _IONBF, 0); // absolutely needed
	return 0;
}

int StdOutRedirect::Stop()
{
	CHECK(_dup2(fdStdOut, _fileno(stdout)));
	std::ios::sync_with_stdio();
	return 0;
}

int StdOutRedirect::GetBuffer(char* buffer, int size)
{
	int nOutRead = _read(fdStdOutPipe[READ_FD], buffer, size);
	buffer[nOutRead] = '\0';
	return nOutRead;
}

//https://codereview.stackexchange.com/questions/419/converting-between-stdwstring-and-stdstring

std::wstring s2ws(const std::string& s) {
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	std::wstring r(len, L'\0');
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
	return r;
}

std::string ws2s(const std::wstring& s) {
	int len;
	int slength = (int)s.length() + 1;
	len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	std::string r(len, '\0');
	WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
	return r;
}

void Progress(LONG64 llBytes, LONG64 llFileSize) {
	char complete_char = '=';
	char incomplete_char = ' ';
	char next_char = '>';
	int  width = 50;

	if (llFileSize == 0) return;

	FLOAT progress = (FLOAT)llBytes / llFileSize;
	LONG64 pos = (LONG64)(width * progress);

	std::cout << "       \b\b\b\b\b\b\b\b\b\b\r"; // erase percent
	std::cout << "[";

	for (int i = 0; i < width; ++i) {
		if (i < pos) std::cout << complete_char;
		else if (i == pos) std::cout << next_char;
		else std::cout << incomplete_char;
	}

	std::cout << "] " << std::dec << LONG64(progress * 100.0) << "% ";
}

uint64_t GetDiskFreeSpaceX(const char* path) {

	DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;

	std::wstring wsPath = s2ws(path);

	if (!GetDiskFreeSpace(wsPath.c_str(), &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters))
		return (uint64_t)(-1LL);

	return ((uint64_t)BytesPerSector) * ((uint64_t)SectorsPerCluster) * ((uint64_t)NumberOfFreeClusters);

}

std::string exec(const char* cmd) {
	char buffer[1024 * 16];
	std::string result = "";
	FILE* pipe = _popen(cmd, "r");
	if (!pipe) throw std::runtime_error("popen() failed!");
	try {
		while (fgets(buffer, sizeof buffer, pipe) != NULL) {
			result += buffer;
		}
	}
	catch (...) {
		_pclose(pipe);
		throw;
	}
	_pclose(pipe);
	return result;
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{

	if (uMsg == BFFM_INITIALIZED)
	{
		std::string tmp = (const char*)lpData;
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

std::string BrowseFolder(std::string saved_path)
{
	TCHAR path[MAX_PATH];

	const char* path_param = saved_path.c_str();

	BROWSEINFO bi = { 0 };
	bi.lpszTitle = (L"Select folder...");
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)path_param;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl != 0)
	{
		//get the name of the folder and put it in path
		SHGetPathFromIDList(pidl, path);

		//free memory used
		IMalloc* imalloc = 0;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pidl);
			imalloc->Release();
		}

		return ws2s(path);
	}

	return "";
}

std::string BrowseFile(std::string filters, int flags)
{
	OPENFILENAME ofn;    // common dialog box structure
	char szFile[260];    // buffer for file name
	int ret;

	std::wstring wfilters = s2ws(filters);

	std::replace(wfilters.begin(), wfilters.end(), ';', '\0');

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = (LPWSTR)szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = wfilters.c_str();
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (flags & 1 << 0)
		ret = GetSaveFileName(&ofn);
	else
		ret = GetOpenFileName(&ofn);

	if (ret)
		return ws2s((LPWSTR)szFile);
	else
		return "";
}

std::string BytesToHex(unsigned char* data, int len) {
	constexpr char hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7',
								'8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

	std::string s(len * 2, ' ');
	for (int i = 0; i < len; ++i) {
		s[2 * i] = hexmap[(data[i] & 0xF0) >> 4];
		s[2 * i + 1] = hexmap[data[i] & 0x0F];
	}
	return s;
}

std::vector<uint8_t> HexToBytes(const std::string& hex) {
	std::vector<uint8_t> bytes;

	for (unsigned int i = 0; i < hex.length(); i += 2) {
		std::string byteString = hex.substr(i, 2);
		uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
		bytes.push_back(byte);
	}

	return bytes;
}

void FileTouch(std::string name)
{
	std::fstream fs;
	fs.open(name, std::ios::out);
	fs.close();
}

void Fill_File(std::ofstream* f, uint8_t patern, size_t block_size, size_t size)
{
	uint8_t* block = new uint8_t[block_size];

	memset((uint8_t*)block, patern, (size_t)block_size);

	while (size > block_size) {
		f->write((const char*)block, block_size); size -= block_size; f->flush();
	};

	f->write((const char*)block, size);

	f->flush();

	delete[] block;
}