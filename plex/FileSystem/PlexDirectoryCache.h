#ifndef PLEXDIRECTORYCACHE_H
#define PLEXDIRECTORYCACHE_H

#include <string>
#include "FileItem.h"
#include <boost/unordered_map.hpp>
#include "threads/SingleLock.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexDirectoryCacheEntry
{
public:
  ~CPlexDirectoryCacheEntry() {}
  unsigned long hash;
  CFileItemListPtr pitemList;
};


typedef boost::unordered_map<std::string,CPlexDirectoryCacheEntry> CacheMap;
typedef boost::unordered_map<std::string,CPlexDirectoryCacheEntry>::iterator CacheMapIterator;
typedef std::pair<std::string, CPlexDirectoryCacheEntry> CacheMapPair;

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexDirectoryCache
{
private:
  CacheMap m_cacheMap;
  CCriticalSection m_cacheLock;
  bool  m_bEnabled;

public:

  enum CacheStrategies
  {
    CACHE_STARTEGY_NONE = 0,
    CACHE_STRATEGY_ITEM_COUNT = 1,
    CACHE_STRATEGY_ALWAYS = 2
  };

  static int CACHE_THESHOLD_COUNT;

  CPlexDirectoryCache() : m_bEnabled(true) {}
  ~CPlexDirectoryCache();
  bool GetCacheHit(const std::string path, const unsigned long newHash, CFileItemList &List);
  void AddToCache(const std::string path, const unsigned long newHash, CFileItemList &List, CacheStrategies Startegy);
  void LogStats();
  void Clear();
  inline void Enable(bool bEnable) { m_bEnabled = bEnable; }

};


typedef boost::shared_ptr<CPlexDirectoryCache> CPlexDirectoryCachePtr;

#endif // PLEXDIRECTORYCACHE_H
