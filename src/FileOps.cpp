#include "FileOps.h"

#include "Platform.h"
#include "StringUtils.h"

#ifdef PLATFORM_UNIX
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#endif

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

void FileOps::extractFromZip(const char* zipFile, const char* src, const char* dest) throw (IOException)
{
#ifdef PLATFORM_UNIX
	throw IOException("not implemented");
#else
	throw IOException("not implemented");
#endif
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
		throw IOException("Unable to remove file " + std::string(src));
	}
#else
	throw IOException("not implemented");
#endif
}

std::string FileOps::dirname(const char* path)
{
#ifdef PLATFORM_UNIX
	throw IOException("not implemented");
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

