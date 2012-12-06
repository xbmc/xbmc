/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "DirectoryCache.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "climits"

using namespace std;
using namespace XFILE;

CDirectoryCache::CDir::CDir(DIR_CACHE_TYPE cacheType)
{
  m_cacheType = cacheType;
  m_lastAccess = 0;
  m_Items = new CFileItemList;
  m_Items->SetFastLookup(true);
}

CDirectoryCache::CDir::~CDir()
{
  delete m_Items;
}

void CDirectoryCache::CDir::SetLastAccess(unsigned int &accessCounter)
{
  m_lastAccess = accessCounter++;
}

CDirectoryCache::CDirectoryCache(void)
{
  m_accessCounter = 0;
#ifdef _DEBUG
  m_cacheHits = 0;
  m_cacheMisses = 0;
#endif
}

CDirectoryCache::~CDirectoryCache(void)
{
}

bool CDirectoryCache::GetDirectory(const CStdString& strPath, CFileItemList &items, bool retrieveAll)
{
  CSingleLock lock (m_cs);

  CStdString storedPath = URIUtils::SubstitutePath(strPath);
  URIUtils::RemoveSlashAtEnd(storedPath);

  ciCache i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    CDir* dir = i->second;
    if (dir->m_cacheType == XFILE::DIR_CACHE_ALWAYS ||
       (dir->m_cacheType == XFILE::DIR_CACHE_ONCE && retrieveAll))
    {
      items.Copy(*dir->m_Items);
      dir->SetLastAccess(m_accessCounter);
#ifdef _DEBUG
      m_cacheHits+=items.Size();
#endif
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

  CStdString storedPath = URIUtils::SubstitutePath(strPath);
  URIUtils::RemoveSlashAtEnd(storedPath);

  ClearDirectory(storedPath);

  CheckIfFull();

  CDir* dir = new CDir(cacheType);
  dir->m_Items->Copy(items);
  dir->SetLastAccess(m_accessCounter);
  m_cache.insert(pair<CStdString, CDir*>(storedPath, dir));
}

void CDirectoryCache::ClearFile(const CStdString& strFile)
{
  CStdString strPath;
  URIUtils::GetDirectory(strFile, strPath);
  ClearDirectory(strPath);
}

void CDirectoryCache::ClearDirectory(const CStdString& strPath)
{
  CSingleLock lock (m_cs);

  CStdString storedPath = URIUtils::SubstitutePath(strPath);
  URIUtils::RemoveSlashAtEnd(storedPath);

  iCache i = m_cache.find(storedPath);
  if (i != m_cache.end())
    Delete(i);
}

void CDirectoryCache::ClearSubPaths(const CStdString& strPath)
{
  CSingleLock lock (m_cs);

  CStdString storedPath = URIUtils::SubstitutePath(strPath);
  URIUtils::RemoveSlashAtEnd(storedPath);

  iCache i = m_cache.begin();
  while (i != m_cache.end())
  {
    CStdString path = i->first;
    if (strncmp(path.c_str(), storedPath.c_str(), storedPath.GetLength()) == 0)
      Delete(i++);
    else
      i++;
  }
}

void CDirectoryCache::AddFile(const CStdString& strFile)
{
  CSingleLock lock (m_cs);

  CStdString strPath;
  URIUtils::GetDirectory(strFile, strPath);
  URIUtils::RemoveSlashAtEnd(strPath);

  ciCache i = m_cache.find(strPath);
  if (i != m_cache.end())
  {
    CDir *dir = i->second;
    CFileItemPtr item(new CFileItem(strFile, false));
    dir->m_Items->Add(item);
    dir->SetLastAccess(m_accessCounter);
  }
}

bool CDirectoryCache::FileExists(const CStdString& strFile, bool& bInCache)
{
  CSingleLock lock (m_cs);
  bInCache = false;

  CStdString strPath;
  URIUtils::GetDirectory(strFile, strPath);
  URIUtils::RemoveSlashAtEnd(strPath);

  ciCache i = m_cache.find(strPath);
  if (i != m_cache.end())
  {
    bInCache = true;
    CDir *dir = i->second;
    dir->SetLastAccess(m_accessCounter);
#ifdef _DEBUG
    m_cacheHits++;
#endif
    return dir->m_Items->Contains(strFile);
  }
#ifdef _DEBUG
  m_cacheMisses++;
#endif
  return false;
}

void CDirectoryCache::Clear()
{
  // this routine clears everything
  CSingleLock lock (m_cs);

  iCache i = m_cache.begin();
  while (i != m_cache.end() )
    Delete(i++);
}

void CDirectoryCache::InitCache(set<CStdString>& dirs)
{
  set<CStdString>::iterator it;
  for (it = dirs.begin(); it != dirs.end(); ++it)
  {
    const CStdString& strDir = *it;
    CFileItemList items;
    CDirectory::GetDirectory(strDir, items, "", DIR_FLAG_NO_FILE_DIRS);
    items.Clear();
  }
}

void CDirectoryCache::ClearCache(set<CStdString>& dirs)
{
  iCache i = m_cache.begin();
  while (i != m_cache.end())
  {
    if (dirs.find(i->first) != dirs.end())
      Delete(i++);
    else
      i++;
  }
}

void CDirectoryCache::CheckIfFull()
{
  CSingleLock lock (m_cs);
  static const unsigned int max_cached_dirs = 10;

  // find the last accessed folder, and remove if the number of cached folders is too many
  iCache lastAccessed = m_cache.end();
  unsigned int numCached = 0;
  for (iCache i = m_cache.begin(); i != m_cache.end(); i++)
  {
    // ensure dirs that are always cached aren't cleared
    if (i->second->m_cacheType != DIR_CACHE_ALWAYS)
    {
      if (lastAccessed == m_cache.end() || i->second->GetLastAccess() < lastAccessed->second->GetLastAccess())
        lastAccessed = i;
      numCached++;
    }
  }
  if (lastAccessed != m_cache.end() && numCached >= max_cached_dirs)
    Delete(lastAccessed);
}

void CDirectoryCache::Delete(iCache it)
{
  CDir* dir = it->second;
  delete dir;
  m_cache.erase(it);
}

#ifdef _DEBUG
void CDirectoryCache::PrintStats() const
{
  CSingleLock lock (m_cs);
  CLog::Log(LOGDEBUG, "%s - total of %u cache hits, and %u cache misses", __FUNCTION__, m_cacheHits, m_cacheMisses);
  // run through and find the oldest and the number of items cached
  unsigned int oldest = UINT_MAX;
  unsigned int numItems = 0;
  unsigned int numDirs = 0;
  for (ciCache i = m_cache.begin(); i != m_cache.end(); i++)
  {
    CDir *dir = i->second;
    oldest = min(oldest, dir->GetLastAccess());
    numItems += dir->m_Items->Size();
    numDirs++;
  }
  CLog::Log(LOGDEBUG, "%s - %u folders cached, with %u items total.  Oldest is %u, current is %u", __FUNCTION__, numDirs, numItems, oldest, m_accessCounter);
}
#endif
