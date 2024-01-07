/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ImageFile.h"

#include "ServiceBroker.h"
#include "TextureCache.h"
#include "URL.h"

using namespace XFILE;

CImageFile::CImageFile(void) = default;

CImageFile::~CImageFile(void)
{
  Close();
}

bool CImageFile::Open(const CURL& url)
{
  std::string file = url.Get();
  bool needsRecaching = false;
  std::string cachedFile =
      CServiceBroker::GetTextureCache()->CheckCachedImage(file, needsRecaching);
  if (cachedFile.empty())
  { // not in the cache, so cache it
    cachedFile = CServiceBroker::GetTextureCache()->CacheImage(file);
  }
  if (!cachedFile.empty())
  { // in the cache, return what we have
    if (m_file.Open(cachedFile))
      return true;
  }
  return false;
}

bool CImageFile::Exists(const CURL& url)
{
  bool needsRecaching = false;
  std::string cachedFile =
      CServiceBroker::GetTextureCache()->CheckCachedImage(url.Get(), needsRecaching);
  if (!cachedFile.empty())
  {
    if (CFile::Exists(cachedFile, false))
      return true;
    else
      // Remove from cache so it gets cached again on next Open()
      CServiceBroker::GetTextureCache()->ClearCachedImage(url.Get());
  }

  // need to check if the original can be cached on demand and that the file exists
  if (!CTextureCache::CanCacheImageURL(url))
    return false;

  return CFile::Exists(url.GetHostName());
}

int CImageFile::Stat(const CURL& url, struct __stat64* buffer)
{
  bool needsRecaching = false;
  std::string cachedFile =
      CServiceBroker::GetTextureCache()->CheckCachedImage(url.Get(), needsRecaching);
  if (!cachedFile.empty())
    return CFile::Stat(cachedFile, buffer);

  /*
   Doesn't exist in the cache yet. We have 3 options here:
   1. Cache the file and do the Stat() on the cached file.
   2. Do the Stat() on the original file.
   3. Return -1;
   Only 1 will return valid results, at the cost of being time consuming.  ATM we do 3 under
   the theory that the only user of this is the webinterface currently, where Stat() is not
   required.
   */
  return -1;
}

ssize_t CImageFile::Read(void* lpBuf, size_t uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}

int64_t CImageFile::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CImageFile::Close()
{
  m_file.Close();
}

int64_t CImageFile::GetPosition()
{
  return m_file.GetPosition();
}

int64_t CImageFile::GetLength()
{
  return m_file.GetLength();
}
