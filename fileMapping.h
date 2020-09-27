#ifndef AFISKON_FILEMAPPING_H
#define AFISKON_FILEMAPPING_H

#include <windows.h>

struct FileMapping;

FileMapping* fileMappingCreate(LPCWSTR fname);
unsigned char* fileMappingGetPointer(FileMapping * mapping);
unsigned int fileMappingGetSize(FileMapping * mapping);
void fileMappingClose(FileMapping * mapping);

#endif //AFISKON_FILEMAPPING_H
