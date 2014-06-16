#include "PlexApplication.h"
#include "FileSystem/PlexDirectoryCache.h"
#include "gtest/gtest.h"

TEST(PlexCacheDirectoryAddToCache, CACHE_STARTEGY_NONE)
{
  CFileItemList List;
  List.Add(CFileItemPtr(new CFileItem));

  // Check CACHE_STARTEGY_NONE
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STARTEGY_NONE);
  EXPECT_FALSE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
}

TEST(PlexCacheDirectoryAddToCache, CACHE_STRATEGY_ITEM_COUNT_Low)
{
  CFileItemList List;
  List.Add(CFileItemPtr(new CFileItem));

  // Check CACHE_STRATEGY_ITEM_COUNT with low item number
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STRATEGY_ITEM_COUNT);
  EXPECT_FALSE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
}


TEST(PlexCacheDirectoryAddToCache, CACHE_STRATEGY_ITEM_COUNT_High)
{
  CFileItemList List;

  for (int i=1; i<100; i++)
  {
   List.Add(CFileItemPtr(new CFileItem));
  }

  // Check CACHE_STRATEGY_ITEM_COUNT with high item number
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STRATEGY_ITEM_COUNT);
  EXPECT_TRUE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
  g_plexApplication.directoryCache->Clear();
}

TEST(PlexCacheDirectoryAddToCache, CACHE_STRATEGY_ALWAYS)
{
  CFileItemList List;
  List.Add(CFileItemPtr(new CFileItem));

  // Check CACHE_STRATEGY_ALWAYS
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STRATEGY_ALWAYS);
  EXPECT_TRUE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
  g_plexApplication.directoryCache->Clear();
}

TEST(PlexCacheDirectoryAddToCache, Clear)
{
  CFileItemList List;

  for (int i=1; i<100; i++)
  {
   List.Add(CFileItemPtr(new CFileItem));
  }

  // Check CACHE_STRATEGY_ITEM_COUNT with high item number
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STRATEGY_ITEM_COUNT);
  g_plexApplication.directoryCache->Clear();
  EXPECT_FALSE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
}

TEST(PlexCacheDirectoryAddToCache, Enable)
{
  CFileItemList List;
  List.Add(CFileItemPtr(new CFileItem));

  g_plexApplication.directoryCache->Enable(false);
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STRATEGY_ALWAYS);
  EXPECT_FALSE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
}
