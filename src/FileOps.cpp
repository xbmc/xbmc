#include "FileOps.h"

#include "Platform.h"

#include <unistd.h>

bool FileOps::fileExists(const char* path) throw (IOException)
{
#ifdef PLATFORM_UNIX
#else

#endif
}

void FileOps::setPermissions(const char* path, int permissions) throw (IOException)
{
#ifdef PLATFORM_UNIX
#else
#endif
}

void FileOps::moveFile(const char* src, const char* dest) throw (IOException)
{
#ifdef PLATFORM_UNIX
#else
#endif
}

void FileOps::extractFromZip(const char* zipFile, const char* src, const char* dest) throw (IOException)
{
#ifdef PLATFORM_UNIX
#else
#endif
}

void FileOps::mkdir(const char* dir) throw (IOException)
{
#ifdef PLATFORM_UNIX
#else
#endif
}

void FileOps::rmdir(const char* dir) throw (IOException)
{
#ifdef PLATFORM_UNIX
#else
#endif
}

void FileOps::createSymLink(const char* link, const char* target) throw (IOException)
{
#ifdef PLATFORM_UNIX
#else
#endif
}

