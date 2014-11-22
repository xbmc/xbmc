#include "FileUtils.h"

#include "DirIterator.h"
#include "Log.h"
#include "Platform.h"
#include "StringUtils.h"
#include "AppInfo.h"

#include <algorithm>
#include <assert.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include "minizip/zip.h"
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

#include "sha1.hpp"
#include "bsdiff/bspatch.h"
#include "bzlib.h"
#include "ProcessUtils.h"

FileUtils::IOException::IOException(const std::string& error)
{
  init(errno, error);
}

FileUtils::IOException::IOException(int errorCode, const std::string& error)
{
  init(errorCode, error);
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

FileUtils::IOException::~IOException() throw()
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

bool FileUtils::fileIsLink(const char* path) throw(IOException)
{
#ifdef PLATFORM_UNIX
  struct stat fileInfo;
  if (lstat(path, &fileInfo) != 0)
  {
    if (errno == ENOENT)
    {
      throw IOException("Error, no such file " + std::string(path));
    }
    return false;
  }
  return S_ISLNK(fileInfo.st_mode);
#else // let's just pretend that windows don't have symlinks, which is almost true
  return false;
#endif
}

std::string FileUtils::getSymlinkTarget(const char* path) throw(IOException)
{
#ifdef PLATFORM_UNIX
  char target[PATH_MAX];

  if (!fileIsLink(path))
    return "";

  ssize_t targetLen = readlink(path, target, PATH_MAX);
  if (targetLen != -1)
  {
    std::string ret;
    ret.append(target, (size_t)targetLen);
    return ret;
  }

  return "";
#else
  return "";
#endif
}

bool FileUtils::isDirectory(const char* path) throw(IOException)
{
#ifdef PLATFORM_UNIX
  struct stat fileInfo;
  if (lstat(path, &fileInfo) != 0)
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
  return S_ISDIR(fileInfo.st_mode);
#else
  DWORD result = GetFileAttributes(path);
  if (result == INVALID_FILE_ATTRIBUTES)
    return false;
  return (result & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
#endif
}

bool FileUtils::fileExists(const char* path) throw(IOException)
{
#ifdef PLATFORM_UNIX
  struct stat fileInfo;
  if (lstat(path, &fileInfo) != 0)
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

int FileUtils::fileMode(const char* path) throw(IOException)
{
#ifdef PLATFORM_UNIX
  struct stat fileInfo;
  if (stat(path, &fileInfo) != 0)
  {
    throw IOException("Error reading file permissions for " + std::string(path));
  }
  return fileInfo.st_mode;
#else
  // not implemented for Windows
  return 0;
#endif
}

void FileUtils::chmod(const char* path, int mode) throw(IOException)
{
#ifdef PLATFORM_UNIX
  if (::chmod(path, static_cast<mode_t>(mode)) != 0)
  {
    throw IOException("Failed to set permissions on " + std::string(path) + " to " +
                      intToStr(mode));
  }
#else
// TODO - Not implemented under Windows - all files
// get default permissions
#endif
}

void FileUtils::moveFile(const char* src, const char* dest) throw(IOException)
{
#ifdef PLATFORM_UNIX
  if (rename(src, dest) != 0)
  {
    throw IOException("Unable to rename " + std::string(src) + " to " + std::string(dest));
  }
#else
  if (!MoveFile(src, dest))
  {
    throw IOException("Unable to rename " + std::string(src) + " to " + std::string(dest));
  }
#endif
}

void FileUtils::addToZip(const char* archivePath, const char* path, const char* content,
                         int length) throw(IOException)
{
  int result = ZIP_OK;

  int appendMode = fileExists(archivePath) ? APPEND_STATUS_ADDINZIP : APPEND_STATUS_CREATE;

  zipFile archive = zipOpen(archivePath, appendMode);
  result = zipOpenNewFileInZip(archive, path, 0 /* file attributes */, 0 /* extra field */,
                               0 /* extra field size */, 0 /* global extra field */,
                               0 /* global extra field size */, 0 /* comment */,
                               Z_DEFLATED /* method */, Z_DEFAULT_COMPRESSION /* level */);
  if (result != ZIP_OK)
  {
    throw IOException("Unable to add new file to zip archive");
  }
  result = zipWriteInFileInZip(archive, content, static_cast<unsigned int>(length));
  if (result != ZIP_OK)
  {
    throw IOException("Unable to write file data to zip archive");
  }
  result = zipCloseFileInZip(archive);
  if (result != ZIP_OK)
  {
    throw IOException("Unable to close file in zip archive");
  }
  result = zipClose(archive, 0 /* global comment */);
  if (result != ZIP_OK)
  {
    throw IOException("Unable to close zip archive");
  }
}

const std::string FileUtils::sha1FromFile(const char* filePath) throw(IOException)
{
  if (!fileExists(filePath))
  {
    throw IOException("Unable to find file: " + std::string(filePath));
  }

  std::string fileData = readFile(filePath);
  SHA1 hash;
  hash.update((uint8_t*)fileData.c_str(), fileData.length());
  return hash.end().hex();
}

void FileUtils::extractFromZip(const char* zipFilePath, const char* src,
                               const char* dest) throw(IOException)
{
  unzFile zipFile = unzOpen(zipFilePath);
  int result = unzLocateFile(zipFile, src, 0);
  if (result == UNZ_OK)
  {
    // found a match which is now the current file
    unzOpenCurrentFile(zipFile);
    const int chunkSize = 4096;
    char buffer[chunkSize];

    std::ofstream outputFile(dest, std::ofstream::binary);
    if (!outputFile.good())
    {
      throw IOException("Unable to write to file " + std::string(dest));
    }
    while (true)
    {
      int count = unzReadCurrentFile(zipFile, buffer, chunkSize);
      if (count <= 0)
      {
        if (count < 0)
        {
          throw IOException("Error extracting file from archive " + std::string(src));
        }
        break;
      }
      outputFile.write(buffer, count);
    }
    outputFile.close();

    unzCloseCurrentFile(zipFile);
  }
  else
  {
    throw IOException("Unable to find file " + std::string(src) + " in zip archive " +
                      std::string(zipFilePath));
  }
  unzClose(zipFile);
}

void FileUtils::mkpath(const char* dir) throw(IOException)
{
  std::string currentPath;
  std::istringstream stream(dir);
  while (!stream.eof())
  {
    std::string segment;
    std::getline(stream, segment, '/');
    currentPath += segment;
    if (!currentPath.empty() && !fileExists(currentPath.c_str()))
    {
      mkdir(currentPath.c_str());
    }
    currentPath += '/';
  }
}

void FileUtils::mkdir(const char* dir) throw(IOException)
{
#ifdef PLATFORM_UNIX
  if (::mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0)
  {
    throw IOException("Unable to create directory " + std::string(dir));
  }
#else
  if (!CreateDirectory(dir, 0 /* default security attributes */))
  {
    throw IOException("Unable to create directory " + std::string(dir));
  }
#endif
}

void FileUtils::rmdir(const char* dir) throw(IOException)
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

void FileUtils::createSymLink(const char* link, const char* target) throw(IOException)
{
#ifdef PLATFORM_UNIX
  if (symlink(target, link) != 0)
  {
    throw IOException("Unable to create symlink " + std::string(link) + " to " +
                      std::string(target));
  }
#else
  // symlinks are not supported under Windows (at least, not universally.
  // Windows Vista and later do actually support symlinks)
  LOG(Warn, "Skipping symlink creation - not implemented in Windows");
#endif
}

void FileUtils::removeFile(const char* src) throw(IOException)
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

      LOG(Warn, "Unable to remove file " + std::string(src) + " - it may be in use.  Moving to " +
                tempDeletePath + " and scheduling delete on reboot.");
      moveFile(src, tempDeletePath.c_str());
      MoveFileEx(tempDeletePath.c_str(), 0, MOVEFILE_DELAY_UNTIL_REBOOT);
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
  _splitpath_s(path, 0,            /* drive */
               0,                  /* drive length */
               0,                  /* dir */
               0,                  /* dir length */
               baseName, MAX_PATH, /* baseName length */
               extension, MAX_PATH /* extension length */
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

  _splitpath_s(path, drive,   /* drive */
               3,             /* drive length */
               dir, MAX_PATH, /* dir length */
               0,             /* filename */
               0,             /* filename length */
               0,             /* extension */
               0              /* extension length */
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

void FileUtils::touch(const char* path) throw(IOException)
{
#ifdef PLATFORM_UNIX
  // see http://pubs.opengroup.org/onlinepubs/9699919799/utilities/touch.html
  //
  // we use utimes/futimes instead of utimensat/futimens for compatibility
  // with older Linux and Mac

  if (fileExists(path))
  {
    utimes(path, 0 /* use current date/time */);
  }
  else
  {
    int fd = creat(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd != -1)
    {
      futimes(fd, 0 /* use current date/time */);
      close(fd);
    }
    else
    {
      throw IOException("Unable to touch file " + std::string(path));
    }
  }
#else
  HANDLE result =
  CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
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

void FileUtils::rmdirRecursive(const char* path) throw(IOException)
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
        rmdirRecursive(dir.filePath().c_str());
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
  char canonicalPathBuffer[PATH_MAX + 1];
  if (realpath(path, canonicalPathBuffer) != 0)
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
  std::replace(result.begin(), result.end(), '/', '\\');
  return result;
}

std::string FileUtils::toUnixPathSeparators(const std::string& str)
{
  std::string result = str;
  std::replace(result.begin(), result.end(), '\\', '/');
  return result;
}

void FileUtils::copyTree(const std::string& source, const std::string& destination,
                         std::string root) throw(IOException)
{
  if (root.empty())
    root = source;

  if (*root.rbegin() != '/')
    root += '/';

  DirIterator dir(source.c_str());

  while (dir.next())
  {
    if (dir.isDir())
    {
      if (dir.fileName() == ".." || dir.fileName() == ".")
        continue;

      std::string newDir = destination + '/' + dir.filePath().substr(root.length());
      copyTree(dir.filePath(), newDir);
    }
    else
    {
      if (!fileExists(destination.c_str()))
        mkpath(destination.c_str());
      if (fileExists(dir.filePath().c_str()))
      {
        if (!fileIsLink(dir.filePath().c_str()))
        {
          copyFile(dir.filePath().c_str(), (destination + '/' + dir.fileName()).c_str());
        }
        else
        {
          createSymLink((destination + "/" + dir.fileName()).c_str(),
                        getSymlinkTarget(dir.filePath().c_str()).c_str());
        }
      }
    }
  }
}

std::string FileUtils::tempPath()
{
#ifdef PLATFORM_UNIX
  std::string tmpDir(notNullString(getenv("TMPDIR")));
  if (tmpDir.empty())
  {
    tmpDir = "/tmp";
  }

  if (*tmpDir.rbegin() != '/')
    tmpDir += '/';

  tmpDir += "plexUpdater.XXXXXX";
  char* templ = new char[tmpDir.size() + 1];
  std::copy(tmpDir.begin(), tmpDir.end(), templ);
  templ[tmpDir.size()] = '\0';

  char* dtemp = mkdtemp(templ);
  if (dtemp == NULL || !*dtemp)
  {
    perror("mkdtemp");
    LOG(Error, "Failed to create temp directory " + std::string(templ));
    delete templ;
    return "/tmp/" + intToStr(ProcessUtils::currentProcessId());
  }

  std::string utmpDir(dtemp);

  delete templ;
  return utmpDir;
#else
  char buffer[MAX_PATH + 1];
  GetTempPath(MAX_PATH + 1, buffer);
  std::string baseDir = toUnixPathSeparators(buffer);
  return baseDir + '/' + AppInfo::name() + '-' + intToStr(ProcessUtils::currentProcessId());
#endif
}

bool startsWithDriveLetter(const char* path)
{
  return strlen(path) >= 2 && (isalpha(path[0])) && path[1] == ':';
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

  if ((!pathStr.empty() && pathStr.at(0) == '/') || (startsWith(pathStr, "//")) ||
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

void FileUtils::writeFile(const char* path, const char* data, int length) throw(IOException)
{
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  stream.write(data, length);
}

std::string FileUtils::readFile(const char* path) throw(IOException)
{
  std::ifstream inputFile(path, std::ios::in | std::ios::binary);
  std::string content;
  inputFile.seekg(0, std::ios::end);
  content.resize(static_cast<unsigned int>(inputFile.tellg()));
  inputFile.seekg(0, std::ios::beg);
  inputFile.read(&content[0], static_cast<int>(content.size()));
  return content;
}

void FileUtils::copyFile(const char* src, const char* dest) throw(IOException)
{
#ifdef PLATFORM_UNIX
  std::ifstream inputFile(src, std::ios::binary);
  std::ofstream outputFile(dest, std::ios::binary | std::ios::trunc);

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

  chmod(dest, fileMode(src));
#else
  if (!CopyFile(src, dest, FALSE))
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

void FileUtils::chdir(const char* path) throw(IOException)
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

std::string FileUtils::getcwd() throw(IOException)
{
#ifdef PLATFORM_UNIX
  char path[PATH_MAX];
  if (!::getcwd(path, PATH_MAX))
  {
    throw FileUtils::IOException("Failed to get current directory");
  }
  return std::string(path);
#else
  char path[MAX_PATH];
  if (GetCurrentDirectory(MAX_PATH, path) == 0)
  {
    throw FileUtils::IOException("Failed to get current directory");
  }
  return toUnixPathSeparators(std::string(path));
#endif
}

static int bz2_read(const struct bspatch_stream* stream, void* buffer, int length)
{
  int n;
  int bz2err;
  BZFILE* bz2;

  bz2 = (BZFILE*)stream->opaque;
  n = BZ2_bzRead(&bz2err, bz2, buffer, length);
  if (n != length)
    return -1;

  return 0;
}

bool FileUtils::patchFile(const char* oldFile, const char* newFile, const char* patchFile)
{
  if (!fileExists(oldFile) || !fileExists(patchFile))
  {
    LOG(Error, "Missing patch or old file!");
    return false;
  }

  FILE* patchFd = fopen(patchFile, "rb");
  if (!patchFd)
  {
    LOG(Error, "Failed to open patch file!");
    return false;
  }

  // BSDIFF header that we need to check
  uint8_t header[24];
  if (fread(header, 1, 24, patchFd) != 24)
  {
    LOG(Error, "Failed to read patch header...");

    fclose(patchFd);
    return false;
  }

  // check header magic
  if (memcmp(header, "ENDSLEY/BSDIFF43", 16) != 0)
  {
    LOG(Error, "Patch did not contain correct header!");

    fclose(patchFd);
    return false;
  }

  int64_t newSize = bspatch_offtin(header + 16);
  if (newSize == 0)
  {
    LOG(Error, "Failed to get newsize from patch file");

    fclose(patchFd);
    return false;
  }

  uint8_t* newData = (uint8_t*)malloc((size_t)newSize + 1);
  if (newData == NULL)
  {
    LOG(Error, "Failed to allocate " + intToStr(newSize) + " bytes of memory");

    fclose(patchFd);
    return false;
  }

  int bz2error;
  BZFILE* bfp = BZ2_bzReadOpen(&bz2error, patchFd, 0, 0, NULL, 0);
  if (bfp == NULL)
  {
    LOG(Error, "Failed to open BZ file after header.");

    free(newData);
    fclose(patchFd);
    return false;
  }

  std::string oldData = readFile(oldFile);

  struct bspatch_stream stream;
  stream.read = bz2_read;
  stream.opaque = (void*)bfp;

  if (bspatch((uint8_t*)oldData.c_str(), (int64_t)oldData.size(), newData, newSize, &stream) != 0)
  {
    LOG(Error, "Failed to patch file");

    fclose(patchFd);
    free(newData);
    BZ2_bzReadClose(&bz2error, bfp);
    return false;
  }

  fclose(patchFd);
  BZ2_bzReadClose(&bz2error, bfp);

  writeFile(newFile, (const char*)newData, (int)newSize);

  free(newData);

  return true;
}
