#include "DirIterator.h"
#include "Log.h"
#include "FileUtils.h"
#include "StringUtils.h"

#include <iostream>
#include <vector>

// Simple utility for creating zip files from the
// contents of a directory
//
// The advantage of this over the 'zip' tool on Linux/Mac is consistent
// behavior across platforms and support for Windows.
//
// Usage: zip-tool <archive name> <dir>

// scan a directory and record paths to files that are found
void scanDir(std::vector<std::string>& filesFound, const std::string& path)
{
	DirIterator iter(path.c_str());
	while (iter.next())
	{
		if (iter.isDir())
		{
			if (iter.fileName() == "." || iter.fileName() == "..")
			{
				continue;
			}
			scanDir(filesFound, iter.filePath());
		}
		else
		{
			filesFound.push_back(iter.filePath());
		}
	}
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <archive name> <input dir>" << std::endl << std::endl
			<< "Recursively scan <input dir> and add all files found to the ZIP archive <archive name>"
			<< std::endl;
		return 1;
	}

	std::string archivePath(argv[1]);
	std::string inputDir(argv[2]);

	try
	{
		std::vector<std::string> paths;
		scanDir(paths, inputDir);
		for (std::vector<std::string>::const_iterator iter = paths.begin();
				iter != paths.end();
				++iter)
		{
			std::string path = iter->substr(inputDir.size()+1);
			std::string content = FileUtils::readFile(iter->c_str());
			LOG(Info, "Adding " + path + " to archive " + archivePath);
			FileUtils::addToZip(archivePath.c_str(), path.c_str(), content.data(), static_cast<int>(content.length()));
		}
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Creating zip file failed: " << ex.what() << std::endl;
	}

	return 0;
}
