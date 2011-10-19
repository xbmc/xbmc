#include "FileUtils.h"

#include "DirIterator.h"
#include "Log.h"
#include "Platform.h"
#include "StringUtils.h"

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include "minizip/unzip.h"

#ifdef PLATFORM_UNIX
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#endif

FileUtils::IOException::IOException(const std::string& error)
{
	init(errno,error);
}

FileUtils::IOException::IOException(int errorCode, const std::string& error)
{
	init(errorCode,error);
}

void FileUtils::IOException::init(int errorCode, const std::string& error)
{
	m_error = error;

#ifdef PLATFORM_UNIX
	m_errorCode = errorCode;

	if (m_errorCode > 0)
	{
		m_error += " details: " + std::string(strerror(m_errorCode));
	}
#endif

#ifdef PLATFORM_WINDOWS
	m_errorCode = 0;
	m_error += " GetLastError returned: " + intToStr(GetLastError());
#endif
}

FileUtils::IOException::~IOException() throw ()
{
}

FileUtils::IOException::Type FileUtils::IOException::type() const
{
#ifdef PLATFORM_UNIX
	switch (m_errorCode)
	{
		case 0:
			return NoError;
		case EROFS:
			return ReadOnlyFileSystem;
		case ENOSPC:
			return DiskFull;
		default:
			return Unknown;
	}
#else
	return Unknown;
#endif
}

bool FileUtils::fileExists(const char* path) throw (IOException)
{
#ifdef PLATFORM_UNIX
	struct stat fileInfo;
	if (stat(path,&fileInfo) != 0)
	{
		if (errno == ENOENT)
		{
			return false;
		}
		else
		{
			throw IOException("Error checking for file " + std::string(path));
		}
	}
	return true;
#else
	DWORD result = GetFileAttributes(path);
	if (result == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	return true;
#endif
}

int FileUtils::fileMode(const char* path) throw (IOException)
{
#ifdef PLATFORM_UNIX
	struct stat fileInfo;
	if (stat(path,&fileInfo) != 0)
	{
		throw IOException("Error reading file permissions for " + std::string(path));
	}
	return fileInfo.st_mode;
#else
	// not implemented for Windows
	return 0;
#endif
}

void FileUtils::chmod(const char* path, int mode) throw (IOException)
{
#ifdef PLATFORM_UNIX
	if (::chmod(path,mode) != 0)
	{
		throw IOException("Failed to set permissions on " + std::string(path) + " to " + intToStr(mode));
	}
#else
	// TODO - Not implemented under Windows - all files
	// get default permissions
#endif
}

void FileUtils::moveFile(const char* src, const char* dest) throw (IOException)
{
#ifdef PLATFORM_UNIX
	if (rename(src,dest) != 0)
	{
		throw IOException("Unable to rename " + std::string(src) + " to " + std::string(dest));
	}
#else
	if (!MoveFile(src,dest))
	{
		throw IOException("Unable to rename " + std::string(src) + " to " + std::string(dest));
	}
#endif
}

void FileUtils::extractFromZip(const char* zipFilePath, const char* src, const char* dest) throw (IOException)
{
	unzFile zipFile = unzOpen(zipFilePath);
	int result = unzLocateFile(zipFile,src,0);
	if (result == UNZ_OK)
	{
		// found a match which is now the current file
		unzOpenCurrentFile(zipFile);
		const int chunkSize = 4096;
		char buffer[chunkSize];

		std::ofstream outputFile(dest,std::ofstream::binary);
		if (!outputFile.good())
		{
			throw IOException("Unable to write to file " + std::string(dest));
		}
		while (true)
		{
			int count = unzReadCurrentFile(zipFile,buffer,chunkSize);
			if (count <= 0)
			{
				if (count < 0)
				{
					throw IOException("Error extracting file from archive " + std::string(src));
				}
				break;
			}
			outputFile.write(buffer,count);
		}
		outputFile.close();

		unzCloseCurrentFile(zipFile);
	}
	else
	{
		throw IOException("Unable to find file " + std::string(src) + " in zip archive " + std::string(zipFilePath));
	}
	unzClose(zipFile);
}

void FileUtils::mkpath(const char* dir) throw (IOException)
{
	std::string currentPath;
	std::istringstream stream(dir);
	while (!stream.eof())
	{
		std::string segment;
		std::getline(stream,segment,'/');
		currentPath += segment;
		if (!currentPath.empty() && !fileExists(currentPath.c_str()))
		{
			mkdir(currentPath.c_str());
		}
		currentPath += '/';
	}
}

void FileUtils::mkdir(const char* dir) throw (IOException)
{
#ifdef PLATFORM_UNIX
	if (::mkdir(dir,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0)
	{
		throw IOException("Unable to create directory " + std::string(dir));
	}
#else
	if (!CreateDirectory(dir,0 /* default security attributes */))
	{
		throw IOException("Unable to create directory " + std::string(dir));
	}
#endif
}

void FileUtils::rmdir(const char* dir) throw (IOException)
{
#ifdef PLATFORM_UNIX
	if (::rmdir(dir) != 0)
	{
		throw IOException("Unable to remove directory " + std::string(dir));
	}
#else
	if (!RemoveDirectory(dir))
	{
		throw IOException("Unable to remove directory " + std::string(dir));
	}
#endif
}

void FileUtils::createSymLink(const char* link, const char* target) throw (IOException)
{
#ifdef PLATFORM_UNIX
	if (symlink(target,link) != 0)
	{
		throw IOException("Unable to create symlink " + std::string(link) + " to " + std::string(target));
	}
#else
	// symlinks are not supported under Windows (at least, not universally.
	// Windows Vista and later do actually support symlinks)
	LOG(Warn,"Skipping symlink creation - not implemented in Windows");
#endif
}

void FileUtils::removeFile(const char* src) throw (IOException)
{
#ifdef PLATFORM_UNIX
	if (unlink(src) != 0)
	{
		if (errno != ENOENT)
		{
			throw IOException("Unable to remove file " + std::string(src));
		}
	}
#else
	if (!DeleteFile(src))
	{
		if (GetLastError() == ERROR_ACCESS_DENIED)
		{
			// if another process is using the file, try moving it to
			// a temporary directory and then
			// scheduling it for deletion on reboot
			std::string tempDeletePathBase = tempPath();
			tempDeletePathBase += '/';
			tempDeletePathBase += fileName(src);

			int suffix = 0;
			std::string tempDeletePath = tempDeletePathBase;
			while (fileExists(tempDeletePath.c_str()))
			{
				++suffix;
				tempDeletePath = tempDeletePathBase + '_' + intToStr(suffix);
			}

			LOG(Warn,"Unable to remove file " + std::string(src) + " - it may be in use.  Moving to "
			         + tempDeletePath + " and scheduling delete on reboot.");
			moveFile(src,tempDeletePath.c_str());
			MoveFileEx(tempDeletePath.c_str(),0,MOVEFILE_DELAY_UNTIL_REBOOT);
		}
		else if (GetLastError() != ERROR_FILE_NOT_FOUND)
		{
			throw IOException("Unable to remove file " + std::string(src));
		}
	}
#endif
}

std::string FileUtils::fileName(const char* path)
{
#ifdef PLATFORM_UNIX
	char* pathCopy = strdup(path);
	std::string basename = ::basename(pathCopy);
	free(pathCopy);
	return basename;
#else
	char baseName[MAX_PATH];
	char extension[MAX_PATH];
	_splitpath_s(path,
	  0, /* drive */
	  0, /* drive length */
	  0, /* dir */
	  0, /* dir length */
	  baseName,
	  MAX_PATH, /* baseName length */
	  extension,
	  MAX_PATH /* extension length */
	  );
	return std::string(baseName) + std::string(extension);
#endif
}

std::string FileUtils::dirname(const char* path)
{
#ifdef PLATFORM_UNIX
	char* pathCopy = strdup(path);
	std::string dirname = ::dirname(pathCopy);
	free(pathCopy);
	return dirname;
#else
	char drive[3];
	char dir[MAX_PATH];

	_splitpath_s(path,
	  drive, /* drive */ 
	  3, /* drive length */
	  dir,
	  MAX_PATH, /* dir length */
	  0, /* filename */
	  0, /* filename length */
	  0, /* extension */
	  0  /* extension length */
	);

	std::string result;
	if (drive[0])
	{
		result += std::string(drive);
	}
	result += dir;

	return result;
#endif
}

void FileUtils::touch(const char* path) throw (IOException)
{
#ifdef PLATFORM_UNIX
	// see http://pubs.opengroup.org/onlinepubs/9699919799/utilities/touch.html
	//
	// we use utimes/futimes instead of utimensat/futimens for compatibility
	// with older Linux and Mac

	if (fileExists(path))
	{
		utimes(path,0 /* use current date/time */);
	}
	else
	{
		int fd = creat(path,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (fd != -1)
		{
			futimes(fd,0 /* use current date/time */);
			close(fd);
		}
		else
		{
			throw IOException("Unable to touch file " + std::string(path));
		}
	}
#else
	HANDLE result = CreateFile(path,GENERIC_WRITE,
	                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                           0,
							   CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               0);
	if (result == INVALID_HANDLE_VALUE)
	{
		throw IOException("Unable to touch file " + std::string(path));
	}
	else
	{
		CloseHandle(result);
	}
#endif
}

void FileUtils::rmdirRecursive(const char* path) throw (IOException)
{
	// remove dir contents
	DirIterator dir(path);
	while (dir.next())
	{
		std::string name = dir.fileName();
		if (name != "." && name != "..")
		{
			if (dir.isDir())
			{
				rmdir(dir.filePath().c_str());
			}
			else
			{
				removeFile(dir.filePath().c_str());
			}
		}
	}

	// remove the directory itself
	rmdir(path);
}

std::string FileUtils::canonicalPath(const char* path)
{
#ifdef PLATFORM_UNIX
	// on Linux and Mac OS 10.6, realpath() can allocate the required
	// amount of memory automatically, however Mac OS 10.5 does not support
	// this, so we used a fixed-sized buffer on all platforms
	char canonicalPathBuffer[PATH_MAX+1];
	if (realpath(path,canonicalPathBuffer) != 0)
	{
		return std::string(canonicalPathBuffer);
	}
	else
	{
		throw IOException("Error reading canonical path for " + std::string(path));
	}
#else
	throw IOException("canonicalPath() not implemented");
#endif
}

std::string FileUtils::toWindowsPathSeparators(const std::string& str)
{
	std::string result = str;
	std::replace(result.begin(),result.end(),'/','\\');
	return result;
}

std::string FileUtils::toUnixPathSeparators(const std::string& str)
{
	std::string result = str;
	std::replace(result.begin(),result.end(),'\\','/');
	return result;
}

std::string FileUtils::tempPath()
{
#ifdef PLATFORM_UNIX
	return "/tmp";
#else
	char buffer[MAX_PATH+1];
	GetTempPath(MAX_PATH+1,buffer);
	return toUnixPathSeparators(buffer);
#endif
}

bool startsWithDriveLetter(const char* path)
{
	return strlen(path) >= 2 &&
	      (path[0] >= 'A' && path[0] <= 'Z') &&
	       path[1] == ':';
}

bool FileUtils::isRelative(const char* path)
{
#ifdef PLATFORM_UNIX
	return strlen(path) == 0 || path[0] != '/';
#else
	// on Windows, a path is relative if it does not start with:
	// - '\\' (a UNC name)
	// - '[Drive Letter]:\'
	// - A single backslash
	//
	// the input path is assumed to have already been converted to use
	// Unix-style path separators

	std::string pathStr(path);

	if ((!pathStr.empty() && pathStr.at(0) == '/') ||
	    (startsWith(pathStr,"//")) ||
	    (startsWithDriveLetter(pathStr.c_str())))
	{
		return false;
	}
	else
	{
		return true;
	}
#endif
}

void FileUtils::writeFile(const char* path, const char* data, int length) throw (IOException)
{
	std::ofstream stream(path,std::ios::binary | std::ios::trunc);
	stream.write(data,length);
}

void FileUtils::copyFile(const char* src, const char* dest) throw (IOException)
{
#ifdef PLATFORM_UNIX
	std::ifstream inputFile(src,std::ios::binary);
	std::ofstream outputFile(dest,std::ios::binary | std::ios::trunc);

	if (!inputFile.good())
	{
		throw IOException("Failed to read file " + std::string(src));
	}
	if (!outputFile.good())
	{
		throw IOException("Failed to write file " + std::string(dest));
	}
	
	outputFile << inputFile.rdbuf();

	if (inputFile.bad())
	{
		throw IOException("Error reading file " + std::string(src));
	}
	if (outputFile.bad())
	{
		throw IOException("Error writing file " + std::string(dest));
	}

	chmod(dest,fileMode(src));
#else
	if (!CopyFile(src,dest,FALSE))
	{
		throw IOException("Failed to copy " + std::string(src) + " to " + std::string(dest));
	}
#endif
}

std::string FileUtils::makeAbsolute(const char* path, const char* basePath)
{
	if (isRelative(path))
	{
		assert(!isRelative(basePath));
		return std::string(basePath) + '/' + std::string(path);
	}
	else
	{
		return path;
	}
}

void FileUtils::chdir(const char* path) throw (IOException)
{
#ifdef PLATFORM_UNIX
	if (::chdir(path) != 0)
	{
		throw FileUtils::IOException("Unable to change directory");
	}
#else
	if (!SetCurrentDirectory(path))
	{
		throw FileUtils::IOException("Unable to change directory");
	}
#endif
}

std::string FileUtils::getcwd() throw (IOException)
{
#ifdef PLATFORM_UNIX
	char path[PATH_MAX];
	if (!::getcwd(path,PATH_MAX))
	{
		throw FileUtils::IOException("Failed to get current directory");
	}
	return std::string(path);
#else
	char path[MAX_PATH];
	if (GetCurrentDirectory(MAX_PATH,path) == 0)
	{
		throw FileUtils::IOException("Failed to get current directory");
	}
	return toUnixPathSeparators(std::string(path));
#endif
}

