#include "FileOps.h"

#include "Platform.h"
#include "StringUtils.h"

#include <malloc.h>
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
	throw IOException("not implemented");
#endif
}

void FileOps::setPermissions(const char* path, int permissions) throw (IOException)
{
#ifdef PLATFORM_UNIX
	// TODO - Convert permissions correctly
	int mode = permissions;
	if (chmod(path,mode) != 0)
	{
		throw IOException("Failed to set permissions on " + std::string(path) + " to " + intToStr(permissions));
	}
#else
	throw IOException("not implemented");
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
	throw IOException("not implemented");
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
		int chunkSize = 4096;
		char buffer[chunkSize];

		std::ofstream outputFile(dest,std::ofstream::binary);
		if (!outputFile.good())
		{
			throw IOException("Unable to write to file " + std::string(src));
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
	throw IOException("not implemented");
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
	throw IOException("not implemented");
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
	throw IOException("not implemented");
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
	throw IOException("not implemented");
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
	throw IOException("not implemented");
#endif
}

void FileOps::touch(const char* path) throw (IOException)
{
#ifdef PLATFORM_UNIX
	// see http://pubs.opengroup.org/onlinepubs/9699919799/utilities/touch.html
	if (creat(path,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) != 0)
	{
		throw IOException("Unable to touch file " + std::string(path));
	}
#else
	throw IOException("not implemented");
#endif
}

