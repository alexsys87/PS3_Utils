#include "Files.h"
#include "ConsoleColor.h"

namespace fs = std::filesystem;

void ScanFilesRecursive(std::string directory, std::vector<std::string>* file_paths) noexcept
{
	try
	{
		fs::recursive_directory_iterator dirIt(directory);

		for (const auto& dir_entry : dirIt)
		{
			if (!fs::is_directory(dir_entry.path().string()))
			{
				std::string file_path = GetFixetPath(dir_entry.path().string());

				file_paths->push_back(file_path);
			}
		}
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}

void ScanFilesDirsRecursive(std::string directory, std::vector<std::string>* file_paths) noexcept
{
	try
	{
		fs::recursive_directory_iterator dirIt(directory);

		for (const auto& dir_entry : dirIt)
		{
			std::string file_path = GetFixetPath(dir_entry.path().string());

			file_paths->push_back(file_path);
		}
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}


void FindFileWithExt(std::string directory, std::string ext, std::vector<std::string>* found_files, std::vector<std::string>* file_paths) noexcept
{
	try
	{
		for (std::vector<std::string>::iterator ti = file_paths->begin(); ti != file_paths->end(); ti++)
		{
			std::string file_ext = ToLow(GetFileExtension(ti->c_str()));

			if (file_ext.compare(ext) == 0)
				found_files->push_back(ti->c_str());
		}
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}

std::string FindFile(std::string file_name, std::vector<std::string>* file_paths) noexcept
{
	try
	{
		for (std::vector<std::string>::iterator it = file_paths->begin(); it != file_paths->end(); it++)
		{
			std::string name = GetFileName(it->c_str());

			if (file_name.compare(name) == 0)
				return it->c_str();
		}
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}

	return "";
}

void CreateDirRecursive(std::string directory) noexcept
{
	fs::path dir = fs::path(directory);
	size_t pos = 0;

	try
	{
		do
		{
			pos = dir.string().find_first_of("\\/", pos + 1);
			fs::create_directories(dir.string().substr(0, pos).c_str());
		} while ((pos != std::string::npos));
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}

void CopyDirRecursive(std::string directory, std::string target) noexcept
{
	std::string folder = target;

	try
	{

		fs::recursive_directory_iterator dirIt(directory);

		CopyDirStructure(directory, target);

		for (const auto& dir_entry : dirIt)
		{
			if (!fs::is_directory(dir_entry))
			{
				std::string file = GetFixetPath(dir_entry.path().string());
				std::string dest = GetFixetPath(folder.substr(0, target.rfind('/')+1) + (GetFileName(dir_entry.path().string())));

				std::cout << white << "Copy: " << GetFileName(file) << white << std::endl;

				fs::copy_file(file, dest, fs::copy_options::overwrite_existing);
			}
		}

	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}

void CopyDirStructure(std::string directory, std::string target) noexcept
{
	try
	{
		fs::recursive_directory_iterator dirIt(directory);

		if (!fs::exists(target))
		{
			fs::create_directories(target);
		}

		for (const auto& dir_entry : dirIt)
		{
			if (fs::is_directory(dir_entry))
			{
				size_t found = directory.length();

				if (found != std::string::npos) {

					std::string dir  = dir_entry.path().string().substr(found, dir_entry.path().string().length() - found);
					std::string dest = GetFixetPath(target + dir);

					if (!fs::exists(dest))
					{
						CreateDirRecursive(dest);
					}
				}
			}
		}

	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}

void RemoveDirRecursive(std::string directory) noexcept
{
	try
	{
		if (fs::exists(directory))
		{

			fs::recursive_directory_iterator dirIt(directory);

			for (const auto& dir_entry : dirIt)
			{
				fs::permissions(dir_entry, fs::perms::owner_all | fs::perms::group_all, fs::perm_options::add);
			}

			fs::remove_all(directory);
		}
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}

void RemoveFile(std::string file) noexcept
{
	try
	{
		if (fs::exists(file))
		{
			fs::permissions(file, fs::perms::owner_all | fs::perms::group_all, fs::perm_options::add);
			fs::remove(file);
		}
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
	}
}