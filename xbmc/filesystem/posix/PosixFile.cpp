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
#include "utils/auto_buffer.h"

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
  assert(url.GetProtocol().empty()); // function suitable only for local files
  
  std::string filename(url.GetFileName());
  if (IsAliasShortcut(filename))
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


unsigned int CPosixFile::Read(void* lpBuf, int64_t uiBufSize)
{
  assert(lpBuf != NULL);
  if (m_fd < 0 || !lpBuf)
    return 0; // TODO: return -1
  
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;
  
  const ssize_t res = read(m_fd, lpBuf, uiBufSize);
  if (res < 0)
    return 0; // TODO: return -1
  
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

  return (unsigned int) res;
}

int CPosixFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  assert(lpBuf != NULL);
  if (m_fd < 0 || !m_allowWrite || !lpBuf)
    return -1;

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;
  
  const ssize_t res = write(m_fd, lpBuf, uiBufSize);
  if (res < 0)
    return -1;
  
  if (m_filePos >= 0)
    m_filePos += res; // if m_filePos was known - update it
  
  return (int)res;
}

int64_t CPosixFile::Seek(int64_t iFilePosition, int iWhence /* = SEEK_SET*/)
{
  if (m_fd < 0)
    return -1;
  
  // check for parameter overflow
  if (sizeof(int64_t) != sizeof(off_t) && iFilePosition != (off_t) iFilePosition)
    return -1;
  
  m_filePos = lseek(m_fd, (off_t)iFilePosition, iWhence);
  
  return m_filePos;
}

int CPosixFile::Truncate(int64_t size)
{
  if (m_fd < 0)
    return -1;
  
  // check for parameter overflow
  if (sizeof(int64_t) != sizeof(off_t) && size != (off_t) size)
    return -1;
  
  return ftruncate(m_fd, (off_t)size);
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
      return -1;
    
    const int64_t orgPos = m_filePos;
    if (orgPos > 0)
    {
      // try to seek one byte back
      const int64_t newPos = Seek(orgPos - 1, SEEK_SET);
      bool seekPossible = (newPos == (orgPos -1));
      // restore file position
      if (Seek(orgPos, SEEK_SET) != orgPos)
        return 0;
      
      return seekPossible ? 1 : 0;
    }
    else
    { // m_filePos == 0
      const int64_t len = GetLength(); // GetLength() use seeking to determine length of file
      if (len > 0)
        return 1; // GetLength() returned valid result, seeking is possible
      else if (len == 0)
        return -1; // size of file is zero, can't test seeking
      else
        return 0; // GetLength() error, seeking is not possible
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
  
  // if no access, try to change access mode and retry
  if ((errno == EACCES || errno == EPERM) &&
      chmod(filename.c_str(), 0600) == 0 && unlink(filename.c_str()) == 0)
    return true;
  
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
  
  if (errno == EEXIST || errno == ENOTEMPTY || errno == EINVAL || errno == EISDIR || errno == ELOOP ||
      errno == EMLINK || errno == ENAMETOOLONG || errno == ENOENT || errno == ENOSPC || errno == ENOTDIR ||
      errno == EROFS)
    return false; // hard error, fail at this point
    
  // if no access, try to change access mode and retry
  if ((errno == EACCES || errno == EPERM) && chmod(name.c_str(), 0600) == 0)
  {
    if (rename(name.c_str(), newName.c_str()) == 0)
      return true;

    if (errno == EEXIST || errno == ENOTEMPTY || errno == EINVAL || errno == EISDIR || errno == ELOOP ||
        errno == EMLINK || errno == ENAMETOOLONG || errno == ENOENT || errno == ENOSPC || errno == ENOTDIR ||
        errno == EROFS)
      return false; // hard error, fail at this point
  }

  // try to copy to destination and delete source
  const int src_fd = open(name.c_str(), O_RDONLY);
  if (src_fd == -1)
    return false;

  struct stat64 st;
  if (fstat64(src_fd, &st) != 0 || !S_ISDIR(st.st_mode) || st.st_size < 0)
  { // file size is unknown or source is directory
    close(src_fd);
    return false;
  }
  
  const int64_t src_size = st.st_size;
  
  const int dst_fd = open(newName.c_str(), O_WRONLY | O_CREAT | O_EXCL,
                          S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH);
  if (dst_fd == -1)
  { // can't open destination file
    close(src_fd);
    return false;
  }
  XUTILS::auto_buffer buf(src_size > (1024*1024) ? (1024*1024) : (size_t) src_size);
  
  int64_t copied = 0;
  do
  {
    const ssize_t r = read(src_fd, buf.get(), buf.size());
    if (r == 0)
      break;  // end of file
    if (r < 0 || write(dst_fd, buf.get(), r) != r)
    {
      copied = -1; // indicate error
      break;
    }
    copied += r;
  } while (copied <= src_size);
  buf.clear(); // free memory
  close(src_fd);
  close(dst_fd);
  
  if (copied != src_size || unlink(name.c_str()) != 0)
  { // not full source content was copied or source file can't be removed
    unlink(newName.c_str());
    return false;
  }
  
  return true;
}

bool CPosixFile::Exists(const CURL& url)
{
  const std::string filename(getFilename(url));
  if (filename.empty())
    return false;
  
  struct stat64 st;
  // TODO: check only for files existence when VFS callers will be fixed
  // return stat64(filename.c_str(), &st) == 0 && !S_ISDIR(st.st_mode);
  return stat64(filename.c_str(), &st) == 0;
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
