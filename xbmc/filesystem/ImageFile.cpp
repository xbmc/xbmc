/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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

#include "ImageFile.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "TextureCache.h"

using namespace XFILE;
using namespace std;

CImageFile::CImageFile(void)
{
}

CImageFile::~CImageFile(void)
{
  Close();
}

bool CImageFile::Open(const CURL& url)
{
  CStdString file = url.Get();
  bool needsRecaching = false;
  CStdString cachedFile = CTextureCache::Get().CheckCachedImage(file, false, needsRecaching);
  if (cachedFile.IsEmpty())
  { // not in the cache, so cache it
    cachedFile = CTextureCache::Get().CacheImage(file);
  }
  if (!cachedFile.IsEmpty())
  { // in the cache, return what we have
    if (m_file.Open(cachedFile))
      return true;
  }
  return false;
}

bool CImageFile::Exists(const CURL& url)
{
  bool needsRecaching = false;
  CStdString cachedFile = CTextureCache::Get().CheckCachedImage(url.Get(), false, needsRecaching);
  if (!cachedFile.IsEmpty())
    return CFile::Exists(cachedFile);

  // need to check if the original can be cached on demand and that the file exists 
  if (!CTextureCache::CanCacheImageURL(url))
    return false;

  return CFile::Exists(url.GetHostName());
}

int CImageFile::Stat(const CURL& url, struct __stat64* buffer)
{
  bool needsRecaching = false;
  CStdString cachedFile = CTextureCache::Get().CheckCachedImage(url.Get(), false, needsRecaching);
  if (!cachedFile.IsEmpty())
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

unsigned int CImageFile::Read(void* lpBuf, int64_t uiBufSize)
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
