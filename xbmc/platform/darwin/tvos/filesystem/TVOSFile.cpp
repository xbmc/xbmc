/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "TVOSFile.h"

#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/darwin/ios-common/DarwinNSUserDefaults.h"
#include "platform/posix/filesystem/PosixFile.h"

#include <sys/stat.h>

using namespace XFILE;

CTVOSFile::~CTVOSFile()
{
  Close();
}

bool CTVOSFile::WantsFile(const CURL& url)
{
  if (!StringUtils::EqualsNoCase(url.GetFileType(), "xml") ||
      StringUtils::StartsWithNoCase(url.GetFileNameWithoutPath(), "customcontroller.SiriRemote"))
    return false;
  return CDarwinNSUserDefaults::IsKeyFromPath(url.Get());
}

int CTVOSFile::CacheStat(const CURL& url, struct __stat64* buffer)
{
  if (buffer != nullptr)
  {
    size_t size = 0;
    // get the size from the data by passing in a nullptr
    if (CDarwinNSUserDefaults::GetKeyDataFromPath(url.Get(), nullptr, size))
    {
      memset(buffer, 0, sizeof(struct __stat64));
      // mimic stat
      // rw for world
      // regular file
      buffer->st_flags = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IWGRP | S_IWOTH | S_IFREG;
      buffer->st_size = size;
      buffer->st_blocks = 1; // we mimic one block
      buffer->st_blksize = (blksize_t)size; // with full size
      return 0;
    }
  }
  errno = ENOENT;
  return -1;
}

bool CTVOSFile::Open(const CURL& url)
{
  if (CDarwinNSUserDefaults::KeyFromPathExists(url.Get()))
  {
    m_url = url;
    m_position = 0;
    CacheStat(url, &m_cachedStat);
    return true;
  }
  else
  {
    // fallback to posixfile
    m_pFallbackFile = new CPosixFile();
    return m_pFallbackFile->Open(url);
  }
}

bool CTVOSFile::OpenForWrite(const CURL& url, bool bOverWrite /* = false */)
{
  if (CDarwinNSUserDefaults::KeyFromPathExists(url.Get()) && !bOverWrite)
    return false; // no overwrite

  bool ret = WantsFile(url); // if we want the file we can write it ...
  if (ret)
  {
    m_url = url;
    m_position = 0;
  }
  return ret;
}

bool CTVOSFile::Delete(const CURL& url)
{
  bool ret = CDarwinNSUserDefaults::DeleteKeyFromPath(url.Get(), true);

  if (!ret)
  {
    CPosixFile posix;
    ret = posix.Delete(url);
  }
  return ret;
}

bool CTVOSFile::Exists(const CURL& url)
{
  bool ret = CDarwinNSUserDefaults::KeyFromPathExists(url.Get());
  if (!ret)
  {
    CPosixFile posix;
    ret = posix.Exists(url);
  }
  return ret;
}

int CTVOSFile::Stat(const CURL& url, struct __stat64* buffer)
{
  int ret = CacheStat(url, buffer);
  if (ret < 0)
  {
    CPosixFile posix;
    ret = posix.Stat(url, buffer);
  }
  return ret;
}

bool CTVOSFile::Rename(const CURL& url, const CURL& urlnew)
{
  bool ret = false;
  if (Exists(url) && !Exists(urlnew) && WantsFile(urlnew))
  {
    void* lpBuf = nullptr;
    size_t uiBufSize = 0;
    if (CDarwinNSUserDefaults::GetKeyDataFromPath(url.Get(), lpBuf,
                                                  uiBufSize)) // get size from old file
    {
      lpBuf = static_cast<void*>(new char[uiBufSize]);
      if (CDarwinNSUserDefaults::GetKeyDataFromPath(url.Get(), lpBuf, uiBufSize)) // read old file
      {
        if (CDarwinNSUserDefaults::SetKeyDataFromPath(urlnew.Get(), lpBuf, uiBufSize,
                                                      true)) // write to new url
        {
          // remove old file
          Delete(url);
          ret = true;
        }
      }
      delete[] static_cast<char*>(lpBuf);
    }
  }

  if (!ret)
  {
    CPosixFile posix;
    ret = posix.Rename(url, urlnew);
  }
  return ret;
}

int CTVOSFile::Stat(struct __stat64* buffer)
{
  memcpy(buffer, &m_cachedStat, sizeof(struct __stat64));
  return 0;
}

ssize_t CTVOSFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (m_pFallbackFile != nullptr)
    return m_pFallbackFile->Read(lpBuf, uiBufSize);

  void* lpBufInternal = nullptr;

  if (m_position > 0 && m_position == GetLength())
    return 0; // simulate read 0 bytes on EOF

  if (CDarwinNSUserDefaults::GetKeyDataFromPath(m_url.Get(), lpBufInternal,
                                                uiBufSize)) // read size of file
  {
    lpBufInternal = static_cast<void*>(new char[uiBufSize]);
    if (CDarwinNSUserDefaults::GetKeyDataFromPath(m_url.Get(), lpBufInternal,
                                                  uiBufSize)) // read file
    {
      memcpy(lpBuf, lpBufInternal, uiBufSize);
    }
    delete[] static_cast<char*>(lpBufInternal);
    m_position = uiBufSize;
  }
  return uiBufSize;
}

ssize_t CTVOSFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (m_pFallbackFile != nullptr)
    return m_pFallbackFile->Write(lpBuf, uiBufSize);

  if (CDarwinNSUserDefaults::SetKeyDataFromPath(m_url.Get(), lpBuf, uiBufSize,
                                                true)) // write to file
  {
    m_position = uiBufSize;
    CacheStat(m_url, &m_cachedStat);
    return uiBufSize;
  }
  return -1;
}

int64_t CTVOSFile::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
{
  if (m_pFallbackFile != nullptr)
    return m_pFallbackFile->Seek(iFilePosition, iWhence);

  errno = EINVAL;
  return -1;
}

void CTVOSFile::Close()
{
  m_url.Reset();
  m_position = -1;
  memset(&m_cachedStat, 0, sizeof(m_cachedStat));
  if (m_pFallbackFile != nullptr)
  {
    m_pFallbackFile->Close();
    delete m_pFallbackFile;
    m_pFallbackFile = nullptr;
  }
}

int64_t CTVOSFile::GetPosition()
{
  if (m_pFallbackFile != nullptr)
    return m_pFallbackFile->GetPosition();

  return 0;
}

int64_t CTVOSFile::GetLength()
{
  if (m_pFallbackFile != nullptr)
    return m_pFallbackFile->GetLength();
  else
    return m_cachedStat.st_size;
}

int CTVOSFile::GetChunkSize()
{
  if (m_pFallbackFile != nullptr)
    return m_pFallbackFile->GetChunkSize();
  else
    return static_cast<int>(GetLength()); // only full file size can be read from nsuserdefaults...
}

int CTVOSFile::IoControl(EIoControl request, void* param)
{
  if (m_pFallbackFile != nullptr)
    return m_pFallbackFile->IoControl(request, param);

  if (request == IOCTRL_SEEK_POSSIBLE)
    return 0; // no seek support
  return -1;
}
