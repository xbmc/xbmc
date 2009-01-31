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

#include "stdafx.h"
#include "DirectoryCache.h"
#include "Util.h"
#include "Settings.h"
#include "FileItem.h"

using namespace std;
using namespace DIRECTORY;

CDirectoryCache g_directoryCache;

CDirectoryCache::CDir::CDir(const CStdString &strPath, DIR_CACHE_TYPE cacheType)
{
  m_strPath = strPath;
  m_cacheType = cacheType;
  m_Items = new CFileItemList;
  m_Items->SetFastLookup(true);
}

CDirectoryCache::CDir::~CDir()
{
  delete m_Items;
}

CDirectoryCache::CDirectoryCache(void)
{
  m_iThumbCacheRefCount = 0;
  m_iMusicThumbCacheRefCount = 0;
}

CDirectoryCache::~CDirectoryCache(void)
{
}

bool CDirectoryCache::GetDirectory(const CStdString& strPath, CFileItemList &items) const
{
  CSingleLock lock (m_cs);

  CStdString storedPath = strPath;
  CUtil::RemoveSlashAtEnd(storedPath);

  for (civecCache i = m_vecCache.begin(); i != m_vecCache.end(); i++)
  {
    const CDir* dir = *i;
    if (dir->m_strPath == storedPath && dir->m_cacheType == DIRECTORY::DIR_CACHE_ALWAYS)
    {
      // make a copy of each item (see SetDirectory())
      for (int i = 0; i < dir->m_Items->Size(); i++)
      {
        CFileItemPtr newItem(new CFileItem(*dir->m_Items->Get(i)));
        items.Add(newItem);
      }
      return true;
    }
  }
  return false;
}

void CDirectoryCache::SetDirectory(const CStdString& strPath, const CFileItemList &items, DIR_CACHE_TYPE cacheType)
{
  if (cacheType == DIR_CACHE_NEVER)
    return; // nothing to do

  // caches the given directory using a copy of the items, rather than the items
  // themselves.  The reason we do this is because there is often some further
  // processing on the items (stacking, transparent rars/zips for instance) that
  // alters the URL of the items.  If we shared the pointers, we'd have problems
  // as the URLs in the cache would have changed, so things such as
  // CDirectoryCache::FileExists() would fail for files that really do exist (just their
  // URL's have been altered).  This is called from CFile::Exists() which causes
  // all sorts of hassles.
  // IDEALLY, any further processing on the item would actually create a new item 
  // instead of altering it, but we can't really enforce that in an easy way, so
  // this is the best solution for now.
  CSingleLock lock (m_cs);

  ClearDirectory(strPath);

  CStdString storedPath = strPath;
  CUtil::RemoveSlashAtEnd(storedPath);

  CDir* dir = new CDir(storedPath, cacheType);
  // make a copy of each item
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr newItem(new CFileItem(*items[i]));
    dir->m_Items->Add(newItem);
  }
  m_vecCache.push_back(dir);
}

void CDirectoryCache::ClearDirectory(const CStdString& strPath)
{
  CSingleLock lock (m_cs);

  CStdString storedPath = strPath;
  CUtil::RemoveSlashAtEnd(storedPath);

  for (ivecCache i = m_vecCache.begin(); i != m_vecCache.end(); i++)
  {
    CDir* dir = *i;
    if (dir->m_strPath == storedPath)
    {
      delete dir;
      m_vecCache.erase(i);
      return;
    }
  }
}

void CDirectoryCache::ClearSubPaths(const CStdString& strPath)
{
  CSingleLock lock (m_cs);

  CStdString storedPath = strPath;
  CUtil::RemoveSlashAtEnd(storedPath);

  ivecCache i = m_vecCache.begin();
  while (i != m_vecCache.end())
  {
    CDir* dir = *i;
    if (strncmp(dir->m_strPath.c_str(), storedPath.c_str(), storedPath.GetLength()) == 0)
    {
      delete dir;
      i = m_vecCache.erase(i);
    }
    else
      i++;
  }
}

bool CDirectoryCache::FileExists(const CStdString& strFile, bool& bInCache) const
{
  CSingleLock lock (m_cs);
  bInCache = false;

  CStdString strPath;
  CUtil::GetDirectory(strFile, strPath);
  CUtil::RemoveSlashAtEnd(strPath);

  for (civecCache i = m_vecCache.begin(); i != m_vecCache.end(); i++)
  {
    const CDir* dir = *i;
    if (dir->m_strPath == strPath)
    {
      bInCache = true;
      if (dir->m_Items->Contains(strFile))
        return true;
    }
  }
  return false;
}

void CDirectoryCache::Clear()
{
  // this routine clears everything except things we always cache
  CSingleLock lock (m_cs);

  ivecCache i = m_vecCache.begin();
  while (i != m_vecCache.end() )
  {
    CDir* dir = *i;
    if (!IsCacheDir(dir->m_strPath))
    {
      delete dir;
      i = m_vecCache.erase(i);
    }
    else
      i++;
  }
}

void CDirectoryCache::InitCache(set<CStdString>& dirs)
{
  set<CStdString>::iterator it;
  for (it = dirs.begin(); it != dirs.end(); ++it)
  {
    const CStdString& strDir = *it;
    CFileItemList items;
    CDirectory::GetDirectory(strDir, items, "", false);
    items.Clear();
  }
}

void CDirectoryCache::ClearCache(set<CStdString>& dirs)
{
  ivecCache i = m_vecCache.begin();
  while (i != m_vecCache.end() )
  {
    CDir* dir = *i;
    if (dirs.find(dir->m_strPath) != dirs.end())
    {
      delete dir;
      m_vecCache.erase(i);
    }
    else
      i++;
  }
}

bool CDirectoryCache::IsCacheDir(const CStdString &strPath) const
{
  if (m_thumbDirs.find(strPath) == m_thumbDirs.end())
    return false;
  if (m_musicThumbDirs.find(strPath) == m_musicThumbDirs.end())
    return false;

  return true;
}

void CDirectoryCache::InitThumbCache()
{
  CSingleLock lock (m_cs);

  if (m_iThumbCacheRefCount > 0)
  {
    m_iThumbCacheRefCount++;
    return ;
  }
  m_iThumbCacheRefCount++;

  // Init video, pictures cache directories
  if (m_thumbDirs.size() == 0)
  {
    // thumbnails directories
/*    m_thumbDirs.insert(g_settings.GetThumbnailsFolder());
    for (unsigned int hex=0; hex < 16; hex++)
    {
      CStdString strHex;
      strHex.Format("\\%x",hex);
      m_thumbDirs.insert(g_settings.GetThumbnailsFolder() + strHex);
    }*/
  }

  InitCache(m_thumbDirs);
}

void CDirectoryCache::ClearThumbCache()
{
  CSingleLock lock (m_cs);

  if (m_iThumbCacheRefCount > 1)
  {
    m_iThumbCacheRefCount--;
    return ;
  }

  m_iThumbCacheRefCount--;
  ClearCache(m_thumbDirs);
}

void CDirectoryCache::InitMusicThumbCache()
{
  CSingleLock lock (m_cs);

  if (m_iMusicThumbCacheRefCount > 0)
  {
    m_iMusicThumbCacheRefCount++;
    return ;
  }
  m_iMusicThumbCacheRefCount++;

  // Init music cache directories
  if (m_musicThumbDirs.size() == 0)
  {
    // music thumbnails directories
    for (int i = 0; i < 16; i++)
    {
      CStdString hex, folder;
      hex.Format("%x", i);
      CUtil::AddFileToFolder(g_settings.GetMusicThumbFolder(), hex, folder);
      m_musicThumbDirs.insert(folder);
    }
  }

  InitCache(m_musicThumbDirs);
}

void CDirectoryCache::ClearMusicThumbCache()
{
  CSingleLock lock (m_cs);

  if (m_iMusicThumbCacheRefCount > 1)
  {
    m_iMusicThumbCacheRefCount--;
    return ;
  }

  m_iMusicThumbCacheRefCount--;
  ClearCache(m_musicThumbDirs);
}
