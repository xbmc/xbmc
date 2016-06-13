/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if defined(TARGET_POSIX)

#include "PosixFile.h"
#include "utils/AliasShortcutUtils.h"
#include "URL.h"
#include "utils/log.h"
#include "filesystem/File.h"

#ifdef HAVE_CONFIG_H
#include "config.h" // for HAVE_POSIX_FADVISE
#endif // HAVE_CONFIG_H

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <assert.h>
#include <limits.h>
#include <algorithm>
#include <sys/ioctl.h>
#include <errno.h>

using namespace XFILE;

CPosixFile::CPosixFile() :
  m_fd(-1), m_filePos(-1), m_lastDropPos(-1), m_allowWrite(false)
{ }

CPosixFile::~CPosixFile()
{
  if (m_fd >= 0)
    close(m_fd);
}

// local helper
static std::string getFilename(const CURL& url)
{
  std::string filename(url.GetFileName());
  if (IsAliasShortcut(filename, false))
    TranslateAliasShortcut(filename);
  
  return filename;
}


bool CPosixFile::Open(const CURL& url)
{
  if (m_fd >= 0)
    return false;
  
  const std::string filename(getFilename(url));
  if (filename.empty())
    return false;
  
  m_fd = open(filename.c_str(), O_RDONLY, S_IRUSR | S_IRGRP | S_IROTH);
  m_filePos = 0;
  
  return m_fd != -1;
}

bool CPosixFile::OpenForWrite(const CURL& url, bool bOverWrite /* = false*/ )
{
  if (m_fd >= 0)
    return false;
  
  const std::string filename(getFilename(url));
  if (filename.empty())
    return false;
  
  m_fd = open(filename.c_str(), O_RDWR | O_CREAT | (bOverWrite ? O_TRUNC : 0), S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH);
  if (m_fd < 0)
    return false;
  
  m_filePos = 0;
  m_allowWrite = true;
  
  return true;
}

void CPosixFile::Close()
{
  if (m_fd >= 0)
  {
    close(m_fd);
    m_fd = -1;
    m_filePos = -1;
    m_lastDropPos = -1;
    m_allowWrite = false;
  }
}


ssize_t CPosixFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (m_fd < 0)
    return -1;
  
  assert(lpBuf != NULL || uiBufSize == 0);
  if (lpBuf == NULL && uiBufSize != 0)
    return -1;

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;
  
  const ssize_t res = read(m_fd, lpBuf, uiBufSize);
  if (res < 0)
  {
    Seek(0, SEEK_CUR); // force update file position
    return -1;
  }
  
  if (m_filePos >= 0)
  {
    m_filePos += res; // if m_filePos was known - update it
#if defined(HAVE_POSIX_FADVISE)
    // Drop the cache between then last drop and 16 MB behind where we
    // are now, to make sure the file doesn't displace everything else.
    // However, never throw out the first 16 MB of the file, as it might
    // be the header etc., and never ask the OS to drop in chunks of
    // less than 1 MB.
    const int64_t end_drop = m_filePos - 16 * 1024 * 1024;
    if (end_drop >= 17 * 1024 * 1024)
    {
      const int64_t start_drop = std::max<int64_t>(m_lastDropPos, 16 * 1024 * 1024);
      if (end_drop - start_drop >= 1 * 1024 * 1024 &&
          posix_fadvise(m_fd, start_drop, end_drop - start_drop, POSIX_FADV_DONTNEED) == 0)
        m_lastDropPos = end_drop;
    }
#endif
  }

  return res;
}

ssize_t CPosixFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (m_fd < 0)
    return -1;

  assert(lpBuf != NULL || uiBufSize == 0);
  if ((lpBuf == NULL && uiBufSize != 0) || !m_allowWrite)
    return -1;

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;
  
  const ssize_t res = write(m_fd, lpBuf, uiBufSize);
  if (res < 0)
  {
    Seek(0, SEEK_CUR); // force update file position
    return -1;
  }
  
  if (m_filePos >= 0)
    m_filePos += res; // if m_filePos was known - update it
  
  return res;
}

int64_t CPosixFile::Seek(int64_t iFilePosition, int iWhence /* = SEEK_SET*/)
{
  if (m_fd < 0)
    return -1;
  
#ifdef TARGET_ANDROID
  //! @todo properly support with detection in configure
  //! Android special case: Android doesn't substitute off64_t for off_t and similar functions
  m_filePos = lseek64(m_fd, (off64_t)iFilePosition, iWhence);
#else  // !TARGET_ANDROID
  const off_t filePosOffT = (off_t) iFilePosition;
  // check for parameter overflow
  if (sizeof(int64_t) != sizeof(off_t) && iFilePosition != filePosOffT)
    return -1;
  
  m_filePos = lseek(m_fd, filePosOffT, iWhence);
#endif // !TARGET_ANDROID
  
  return m_filePos;
}

int CPosixFile::Truncate(int64_t size)
{
  if (m_fd < 0)
    return -1;
  
  const off_t sizeOffT = (off_t) size;
  // check for parameter overflow
  if (sizeof(int64_t) != sizeof(off_t) && size != sizeOffT)
    return -1;
  
  return ftruncate(m_fd, sizeOffT);
}

int64_t CPosixFile::GetPosition()
{
  if (m_fd < 0)
    return -1;

  if (m_filePos < 0)
    m_filePos = lseek(m_fd, 0, SEEK_CUR);
  
  return m_filePos;
}

int64_t CPosixFile::GetLength()
{
  if (m_fd < 0)
    return -1;

  struct stat64 st;
  if (fstat64(m_fd, &st) != 0)
    return -1;
  
  return st.st_size;
}

void CPosixFile::Flush()
{
  if (m_fd >= 0)
    fsync(m_fd);
}

int CPosixFile::IoControl(EIoControl request, void* param)
{
  if (m_fd < 0)
    return -1;

  if (request == IOCTRL_NATIVE)
  {
    if(!param)
      return -1;
    return ioctl(m_fd, ((SNativeIoControl*)param)->request, ((SNativeIoControl*)param)->param);
  }
  else if (request == IOCTRL_SEEK_POSSIBLE)
  {
    if (GetPosition() < 0)
      return -1; // current position is unknown, can't test seeking
    else if (m_filePos > 0)
    {
      const int64_t orgPos = m_filePos;
      // try to seek one byte back
      const bool seekPossible = (Seek(orgPos - 1, SEEK_SET) == (orgPos - 1));
      // restore file position
      if (Seek(orgPos, SEEK_SET) != orgPos)
        return 0; // seeking is not possible
      
      return seekPossible ? 1 : 0;
    }
    else
    { // m_filePos == 0
      // try to seek one byte forward
      const bool seekPossible = (Seek(1, SEEK_SET) == 1);
      // restore file position
      if (Seek(0, SEEK_SET) != 0)
        return 0; // seeking is not possible
      
      if (seekPossible)
        return 1;

      if (GetLength() <= 0)
        return -1; // size of file is zero or can be zero, can't test seeking
      else
        return 0; // size of file is 1 byte or more and seeking not possible
    }
  }
  
  return -1;
}


bool CPosixFile::Delete(const CURL& url)
{
  const std::string filename(getFilename(url));
  if (filename.empty())
    return false;
  
  if (unlink(filename.c_str()) == 0)
    return true;
  
  if (errno == EACCES || errno == EPERM)
    CLog::LogF(LOGWARNING, "Can't access file \"%s\"", filename.c_str());
  
  return false;
}

bool CPosixFile::Rename(const CURL& url, const CURL& urlnew)
{
  const std::string name(getFilename(url)), newName(getFilename(urlnew));
  if (name.empty() || newName.empty())
    return false;
  
  if (name == newName)
    return true;
  
  if (rename(name.c_str(), newName.c_str()) == 0)
    return true;
  
  if (errno == EACCES || errno == EPERM)
    CLog::LogF(LOGWARNING, "Can't access file \"%s\" for rename to \"%s\"", name.c_str(), newName.c_str());

  // rename across mount points - need to copy/delete
  if (errno == EXDEV)
  {
    CLog::LogF(LOGDEBUG, "Source file \"%s\" and target file \"%s\" are located on different filesystems, copy&delete will be used instead of rename", name.c_str(), newName.c_str());
    if (XFILE::CFile::Copy(name, newName))
    {
      if (XFILE::CFile::Delete(name))
        return true;
      else
        XFILE::CFile::Delete(newName);
    }
  }

  return false;
}

bool CPosixFile::Exists(const CURL& url)
{
  const std::string filename(getFilename(url));
  if (filename.empty())
    return false;
  
  struct stat64 st;
  return stat64(filename.c_str(), &st) == 0 && !S_ISDIR(st.st_mode);
}

int CPosixFile::Stat(const CURL& url, struct __stat64* buffer)
{
  assert(buffer != NULL);
  const std::string filename(getFilename(url));
  if (filename.empty() || !buffer)
    return -1;
  
  return stat64(filename.c_str(), buffer);
}

int CPosixFile::Stat(struct __stat64* buffer)
{
  assert(buffer != NULL);
  if (m_fd < 0 || !buffer)
    return -1;
  
  return fstat64(m_fd, buffer);
}

#endif // TARGET_POSIX
