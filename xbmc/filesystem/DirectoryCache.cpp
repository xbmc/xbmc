/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryCache.h"

#include "Directory.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <climits>
#include <mutex>

// Maximum number of directories to keep in our cache
#define MAX_CACHED_DIRS 50

using namespace XFILE;

namespace
{

std::string getKey(const CURL& url)
{
  // Get rid of any URL options, else the compare may be wrong
  std::string path = url.GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(path);
  return path;
}

std::string getDirKey(const CURL& url)
{
  const std::string filePath = getKey(url);
  std::string dirPath = URIUtils::GetDirectory(filePath);
  URIUtils::RemoveSlashAtEnd(dirPath);
  return dirPath;
}

} // Unnamed namespace

CDirectoryCache::CDir::CDir(CacheType cacheType) : m_Items(std::make_unique<CFileItemList>())
{
  m_cacheType = cacheType;
  m_lastAccess = 0;
  m_Items->SetIgnoreURLOptions(true);
  m_Items->SetFastLookup(true);
}

CDirectoryCache::CDir::~CDir() = default;

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

CDirectoryCache::~CDirectoryCache(void) = default;

bool CDirectoryCache::GetDirectory(const CURL& url, CFileItemList& items, bool retrieveAll)
{
  std::unique_lock lock(m_cs);

  const std::string storedPath = getKey(url);

  auto i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    CDir& dir = i->second;
    if (dir.m_cacheType == CacheType::ALWAYS || (dir.m_cacheType == CacheType::ONCE && retrieveAll))
    {
      items.Copy(*dir.m_Items);
      dir.SetLastAccess(m_accessCounter);
#ifdef _DEBUG
      m_cacheHits += items.Size();
#endif
      return true;
    }
  }
  return false;
}

void CDirectoryCache::SetDirectory(const CURL& url, const CFileItemList& items, CacheType cacheType)
{
  if (cacheType == CacheType::NEVER)
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
  std::unique_lock lock(m_cs);

  const std::string storedPath = getKey(url);
  m_cache.erase(storedPath);

  CheckIfFull();

  CDir dir(cacheType);
  dir.m_Items->Copy(items);
  dir.SetLastAccess(m_accessCounter);
  m_cache.emplace(storedPath, std::move(dir));
}

void CDirectoryCache::ClearFile(const CURL& url)
{
  const std::string dirPath = getDirKey(url);
  m_cache.erase(dirPath);
}

void CDirectoryCache::ClearDirectory(const CURL& url)
{
  std::unique_lock lock(m_cs);

  const std::string storedPath = getKey(url);
  m_cache.erase(storedPath);
}

void CDirectoryCache::ClearSubPaths(const CURL& url)
{
  std::unique_lock lock(m_cs);

  const std::string storedPath = getKey(url);
  std::erase_if(m_cache, [&storedPath](const auto& i)
                { return URIUtils::PathHasParent(i.first, storedPath); });
}

void CDirectoryCache::AddFile(const CURL& url)
{
  std::unique_lock lock(m_cs);

  const std::string dirPath = getDirKey(url);

  const auto i{m_cache.find(dirPath)};
  if (i != m_cache.cend())
  {
    CDir& dir{i->second};
    dir.m_Items->Add(std::make_shared<CFileItem>(url.Get(), false));
    dir.SetLastAccess(m_accessCounter);
  }
}

bool CDirectoryCache::FileExists(const CURL& url, bool& foundInCache)
{
  std::unique_lock lock(m_cs);
  foundInCache = false;

  const std::string filePath = getKey(url);
  const std::string dirPath = getDirKey(url);

  auto i = m_cache.find(dirPath);
  if (i != m_cache.end())
  {
    foundInCache = true;
    CDir& dir = i->second;
    dir.SetLastAccess(m_accessCounter);
#ifdef _DEBUG
    m_cacheHits++;
#endif
    return (URIUtils::PathEquals(filePath, dirPath) || dir.m_Items->Contains(url.Get()));
  }
#ifdef _DEBUG
  m_cacheMisses++;
#endif
  return false;
}

void CDirectoryCache::Clear()
{
  // this routine clears everything
  std::unique_lock lock(m_cs);
  m_cache.clear();
}

void CDirectoryCache::InitCache(const std::set<std::string>& dirs)
{
  for (const std::string& dirPath : dirs)
  {
    CFileItemList items;
    CDirectory::GetDirectory(dirPath, items, "", DIR_FLAG_NO_FILE_DIRS);
    items.Clear();
  }
}

void CDirectoryCache::ClearCache(std::set<std::string>& dirs)
{
  std::erase_if(m_cache, [&dirs](const auto& i) { return dirs.contains(i.first); });
}

void CDirectoryCache::CheckIfFull()
{
  std::unique_lock lock(m_cs);

  // find the last accessed folder, and remove if the number of cached folders is too many
  auto lastAccessed = m_cache.end();
  unsigned int numCached = 0;
  const auto end = m_cache.end();
  for (auto i = m_cache.begin(); i != end; ++i)
  {
    // ensure dirs that are always cached aren't cleared
    if (i->second.m_cacheType != CacheType::ALWAYS)
    {
      if (lastAccessed == end || i->second.GetLastAccess() < lastAccessed->second.GetLastAccess())
        lastAccessed = i;
      numCached++;
    }
  }
  if (lastAccessed != end && numCached >= MAX_CACHED_DIRS)
    m_cache.erase(lastAccessed);
}

#ifdef _DEBUG
void CDirectoryCache::PrintStats() const
{
  std::unique_lock lock(m_cs);
  CLog::Log(LOGDEBUG, "{} - total of {} cache hits, and {} cache misses", __FUNCTION__, m_cacheHits,
            m_cacheMisses);
  // run through and find the oldest and the number of items cached
  unsigned int oldest = UINT_MAX;
  unsigned int numItems = 0;
  unsigned int numDirs = 0;
  for (auto i = m_cache.begin(); i != m_cache.end(); i++)
  {
    const CDir& dir = i->second;
    oldest = std::min(oldest, dir.GetLastAccess());
    numItems += dir.m_Items->Size();
    numDirs++;
  }
  CLog::Log(LOGDEBUG, "{} - {} folders cached, with {} items total.  Oldest is {}, current is {}",
            __FUNCTION__, numDirs, numItems, oldest, m_accessCounter);
}
#endif
