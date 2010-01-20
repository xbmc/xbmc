/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ThumbnailCache.h"
#include "FileSystem/File.h"
#include "utils/SingleLock.h"

using namespace std;
using namespace XFILE;

CThumbnailCache* CThumbnailCache::m_pCacheInstance = NULL;

CCriticalSection CThumbnailCache::m_cs;

CThumbnailCache::~CThumbnailCache()
{}

CThumbnailCache::CThumbnailCache()
{
}

CThumbnailCache* CThumbnailCache::GetThumbnailCache()
{
  CSingleLock lock (m_cs);

  if (m_pCacheInstance == NULL)
    m_pCacheInstance = new CThumbnailCache;

  return m_pCacheInstance;
}

bool CThumbnailCache::ThumbExists(const CStdString& strFileName, bool bAddCache /*=false*/)
{
  CSingleLock lock (m_cs);

  if (strFileName.size() == 0) return false;
  map<CStdString, bool>::iterator it;
  it = m_Cache.find(strFileName);
  if (it != m_Cache.end())
    return it->second;

  bool bExists = CFile::Exists(strFileName);

  if (bAddCache)
    Add(strFileName, bExists);
  return bExists;
}

bool CThumbnailCache::IsCached(const CStdString& strFileName)
{
  CSingleLock lock (m_cs);

  map<CStdString, bool>::iterator it;
  it = m_Cache.find(strFileName);
  if (it != m_Cache.end())
    return true;

  return false;
}

void CThumbnailCache::Clear()
{
  CSingleLock lock (m_cs);

  if (m_pCacheInstance != NULL)
  {
    m_Cache.erase(m_Cache.begin(), m_Cache.end());
    delete m_pCacheInstance;
  }

  m_pCacheInstance = NULL;
}

void CThumbnailCache::Add(const CStdString& strFileName, bool bExists)
{
  CSingleLock lock (m_cs);

  map<CStdString, bool>::iterator it;
  it = m_Cache.find(strFileName);
  if (it != m_Cache.end())
  {
    it->second = bExists;
  }
  else
    m_Cache.insert(pair<CStdString, bool>(strFileName, bExists));
}
