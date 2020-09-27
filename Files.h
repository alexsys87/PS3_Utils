#pragma once

#include "ConsoleUtils.h"

void ScanFilesRecursive(std::string directory, std::vector<std::string>* file_paths) noexcept;
void ScanFilesDirsRecursive(std::string directory, std::vector<std::string>* file_paths) noexcept;
std::string FindFile(std::string file_name, std::vector<std::string>* file_paths) noexcept;
void FindFileWithExt(std::string directory, std::string ext, std::vector<std::string>* found_files, std::vector<std::string>* file_paths) noexcept;
void CopyDirRecursive(std::string directory, std::string target) noexcept;
void CopyDirStructure(std::string directory, std::string target) noexcept;
void CreateDirRecursive(std::string directory) noexcept;
void RemoveDirRecursive(std::string directory) noexcept;
void RemoveFile(std::string file) noexcept;
