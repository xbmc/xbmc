#include "PlexTest.h"
#include "PlexApplication.h"
#include "FileSystem/PlexDirectoryCache.h"

class PlexCacheDirectoryTests : public ::testing::Test
{
  void SetUp()
  {
    g_plexApplication.directoryCache = CPlexDirectoryCachePtr(new CPlexDirectoryCache);
  }

  void TearDown()
  {
    g_plexApplication.directoryCache.reset();
  }
};

TEST_F(PlexCacheDirectoryTests, Add_CACHE_STARTEGY_NONE)
{
  CFileItemList List;
  List.Add(CFileItemPtr(new CFileItem));

  // Check CACHE_STARTEGY_NONE
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STARTEGY_NONE);
  EXPECT_FALSE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
}

TEST_F(PlexCacheDirectoryTests, Add_CACHE_STRATEGY_ITEM_COUNT_Low)
{
  CFileItemList List;
  List.Add(CFileItemPtr(new CFileItem));

  // Check CACHE_STRATEGY_ITEM_COUNT with low item number
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STRATEGY_ITEM_COUNT);
  EXPECT_FALSE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
}


TEST_F(PlexCacheDirectoryTests, Add_CACHE_STRATEGY_ITEM_COUNT_High)
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

TEST_F(PlexCacheDirectoryTests, Add_CACHE_STRATEGY_ALWAYS)
{
  CFileItemList List;
  List.Add(CFileItemPtr(new CFileItem));

  // Check CACHE_STRATEGY_ALWAYS
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STRATEGY_ALWAYS);
  EXPECT_TRUE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
  g_plexApplication.directoryCache->Clear();
}

TEST_F(PlexCacheDirectoryTests, Clear)
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

TEST_F(PlexCacheDirectoryTests, Enable)
{
  CFileItemList List;
  List.Add(CFileItemPtr(new CFileItem));

  g_plexApplication.directoryCache->Enable(false);
  g_plexApplication.directoryCache->AddToCache("Test",1234567890,List,CPlexDirectoryCache::CACHE_STRATEGY_ALWAYS);
  EXPECT_FALSE(g_plexApplication.directoryCache->GetCacheHit("Test",1234567890,List));
}
