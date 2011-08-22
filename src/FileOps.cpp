#include "FileOps.h"

#include "Dir.h"
#include "Log.h"
#include "Platform.h"
#include "StringUtils.h"

#include <string.h>
#include <fstream>

#include "minizip/unzip.h"

#ifdef PLATFORM_UNIX
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#endif

FileOps::IOException::IOException(const std::string& error)
: m_errno(0)
{
	m_error = error;

#ifdef PLATFORM_UNIX
	m_errno = errno;

	if (m_errno > 0)
	{
		m_error += " details: " + std::string(strerror(m_errno));
	}
#endif

#ifdef PLATFORM_WINDOWS
	m_error += " GetLastError returned: " + intToStr(GetLastError());
#endif
}

FileOps::IOException::~IOException() throw ()
{
}

bool FileOps::fileExists(const char* path) throw (IOException)
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

void FileOps::setQtPermissions(const char* path, int qtPermissions) throw (IOException)
{
#ifdef PLATFORM_UNIX
	int mode = toUnixPermissions(qtPermissions);
	if (chmod(path,mode) != 0)
	{
		throw IOException("Failed to set permissions on " + std::string(path) + " to " + intToStr(qtPermissions));
	}
#else
	// TODO - Not implemented under Windows - all files
	// get default permissions
#endif
}

void FileOps::moveFile(const char* src, const char* dest) throw (IOException)
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

void FileOps::extractFromZip(const char* zipFilePath, const char* src, const char* dest) throw (IOException)
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

void FileOps::mkdir(const char* dir) throw (IOException)
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

void FileOps::rmdir(const char* dir) throw (IOException)
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

void FileOps::createSymLink(const char* link, const char* target) throw (IOException)
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

void FileOps::removeFile(const char* src) throw (IOException)
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
		if (GetLastError() != ERROR_FILE_NOT_FOUND)
		{
			throw IOException("Unable to remove file " + std::string(src));
		}
	}
#endif
}

std::string FileOps::fileName(const char* path)
{
#ifdef PLATFORM_UNIX
	char* pathCopy = strdup(path);
	std::string basename = ::basename(pathCopy);
	free(pathCopy);
	return basename;
#else
	char baseName[MAX_PATH];
	char extension[MAX_PATH];
	_splitpath(path, 0 /* drive */, 0 /* dir */, baseName, extension);
	return std::string(baseName) + std::string(extension);
#endif
}

std::string FileOps::dirname(const char* path)
{
#ifdef PLATFORM_UNIX
	char* pathCopy = strdup(path);
	std::string dirname = ::dirname(pathCopy);
	free(pathCopy);
	return dirname;
#else
	char dir[MAX_PATH];
	_splitpath(path, 0 /* drive */, dir, 0 /* filename */, 0/* extension */); 
	return std::string(dir);
#endif
}

void FileOps::touch(const char* path) throw (IOException)
{
#ifdef PLATFORM_UNIX
	// see http://pubs.opengroup.org/onlinepubs/9699919799/utilities/touch.html
	int fd = creat(path,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd != -1)
	{
		close(fd);
	}
	else
	{
		throw IOException("Unable to touch file " + std::string(path));
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

void FileOps::rmdirRecursive(const char* path) throw (IOException)
{
	// remove dir contents
	Dir dir(path);
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

std::string FileOps::canonicalPath(const char* path)
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

template <class InFlags, class OutFlags>
void addFlag(InFlags inFlags, int testBit, OutFlags& outFlags, int setBit)
{
	if (inFlags & testBit)
	{
		outFlags |= setBit;
	}
}

#ifdef PLATFORM_UNIX
int FileOps::toUnixPermissions(int qtPermissions)
{
	mode_t result = 0;
	addFlag(qtPermissions,ReadUser,result,S_IRUSR);
	addFlag(qtPermissions,WriteUser,result,S_IWUSR);
	addFlag(qtPermissions,ExecUser,result,S_IXUSR);
	addFlag(qtPermissions,ReadGroup,result,S_IRGRP);
	addFlag(qtPermissions,WriteGroup,result,S_IWGRP);
	addFlag(qtPermissions,ExecGroup,result,S_IXGRP);
	addFlag(qtPermissions,ReadOther,result,S_IROTH);
	addFlag(qtPermissions,WriteOther,result,S_IWOTH);
	addFlag(qtPermissions,ExecOther,result,S_IXOTH);
	return result;
}
#endif

