
#include "PlexDirectoryCache.h"
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include "log.h"


int CPlexDirectoryCache::CACHE_THESHOLD_COUNT = 20;

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexDirectoryCache::~CPlexDirectoryCache()
{
  m_cacheMap.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectoryCache::GetCacheHit(const std::string path, const unsigned long newHash, CFileItemList &List)
{
  CSingleLock lk(m_cacheLock);

  if (!m_bEnabled)
    return false;

  CacheMapIterator it = m_cacheMap.find(path);

  if (it != m_cacheMap.end())
  {
#ifdef _DEBUG
    CLog::Log(LOGDEBUG,"CPlexDirectoryCache Cache HIT for  : %s, with Hash %lX",path.c_str(),newHash);
#endif
    if (it->second.hash == newHash)
    {
      List.Copy(*it->second.pitemList);
      return true;
    }
  }
#ifdef _DEBUG
  CLog::Log(LOGDEBUG,"CPlexDirectoryCache Cache MISS for  : %s, with Hash %lX",path.c_str(),newHash);
#endif
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectoryCache::AddToCache(const std::string path, const unsigned long newHash, CFileItemList &List,CacheStrategies Startegy)
{
  CSingleLock lk(m_cacheLock);

  if (!m_bEnabled)
    return;

  switch(Startegy)
  {
    // No caching, return
    case CACHE_STARTEGY_NONE:
      return;

    // item count based caching
    case CACHE_STRATEGY_ITEM_COUNT:
    {
      if (List.Size() < CACHE_THESHOLD_COUNT)
        return;
      break;
    }

    default:
      break;
  }

  CLog::Log(LOGDEBUG,"CPlexDirectoryCache Adding an entry to cache : %s, with Hash %lX",path.c_str(),newHash);

  // Create new Item List or clear existing
  if (m_cacheMap.find(path) != m_cacheMap.end())
    m_cacheMap[path].pitemList->Clear();
  else
    m_cacheMap[path].pitemList = CFileItemListPtr(new CFileItemList());

  // set the new item properties
  m_cacheMap[path].hash = newHash;
  m_cacheMap[path].pitemList->Copy(List);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectoryCache::LogStats()
{
  CLog::Log(LOGDEBUG,"CPlexDirectoryCache Statistics");
  CLog::Log(LOGDEBUG,"Cache contains %d URL entries", (int)m_cacheMap.size());

  // count includes items
  int itemCount = 0;
  BOOST_FOREACH(CacheMapPair p, m_cacheMap)
  {
    itemCount += p.second.pitemList->Size();
  }

  CLog::Log(LOGDEBUG,"Cache totalizing %d FileItems", itemCount);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectoryCache::Clear()
{
  m_cacheMap.clear();
}
