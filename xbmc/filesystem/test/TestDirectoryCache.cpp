/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/IDirectory.h"

#include <gtest/gtest.h>

using namespace XFILE;

class TestDirectoryCache : public ::testing::Test
{
protected:
  void SetUp() override { cache = std::make_unique<CDirectoryCache>(); }

  void TearDown() override { cache.reset(); }

  bool ExistsInCache(CDirectoryCache& cache, const std::string& dir)
  {
    CFileItemList items;
    bool found = cache.GetDirectory(CURL(dir), items, true);
    return found;
  }

  std::unique_ptr<CFileItemList> GetCacheDirectory(CDirectoryCache& cache,
                                                   const std::string& dir,
                                                   bool retrieveAll)
  {
    auto pItems = std::make_unique<CFileItemList>();
    cache.GetDirectory(CURL(dir), *pItems, retrieveAll);
    return pItems;
  }

  void AddFile(CFileItemList& items, const CURL& name, const bool isDir = false) const
  {
    items.Add(std::make_shared<CFileItem>(CURL(name), isDir));
  }

  void AddDir(CFileItemList& items, const CURL& name) const { AddFile(items, name, true); }

  std::unique_ptr<CDirectoryCache> cache;
};

TEST_F(TestDirectoryCache, EmptyCache)
{
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory/", false)->Size());
}

TEST_F(TestDirectoryCache, SetAndGetDirectory)
{
  CURL url("ftp://test/directory/");
  CFileItemList items;

  // Add some test items
  AddFile(items, CURL("ftp://test/directory/file1.txt"));
  AddFile(items, CURL("ftp://test/directory/file2.txt"));
  AddDir(items, CURL("ftp://test/directory/subdir/"));

  cache->SetDirectory(url, items, CacheType::ALWAYS);

  EXPECT_EQ(3, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
  EXPECT_EQ(3, GetCacheDirectory(*cache, "ftp://test/directory/", false)->Size());
}

// CacheType behavior tests
TEST_F(TestDirectoryCache, CacheTypeNever)
{
  CURL url("ftp://test/directory/");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  // NEVER cache type should not store anything
  cache->SetDirectory(url, items, CacheType::NEVER);

  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory/", false)->Size());
}

TEST_F(TestDirectoryCache, CacheTypeOnce)
{
  CURL url("ftp://test/directory/");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  cache->SetDirectory(url, items, CacheType::ONCE);

  // Should return items only when retrieveAll is true
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory/", false)->Size());
}

TEST_F(TestDirectoryCache, CacheTypeAlways)
{
  CURL url("ftp://test/directory/");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  cache->SetDirectory(url, items, CacheType::ALWAYS);

  // Should return items regardless of retrieveAll flag
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/", false)->Size());
}

// URL normalization tests
TEST_F(TestDirectoryCache, URLNormalization)
{
  CURL urlWithSlash("ftp://test/directory/");
  CURL urlWithoutSlash("ftp://test/directory");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  // Set with trailing slash
  // Get without trailing slash should work
  cache->SetDirectory(urlWithSlash, items, CacheType::ALWAYS);
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory", true)->Size());

  // Clear cache and test reverse
  cache->Clear();

  // Set without trailing slash
  // Get with trailing slash should work
  cache->SetDirectory(urlWithoutSlash, items, CacheType::ALWAYS);
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
}

TEST_F(TestDirectoryCache, URLWithOptions)
{
  CURL urlWithOptions("ftp://test/directory/?option=value");

  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  // Set with URL options
  cache->SetDirectory(urlWithOptions, items, CacheType::ALWAYS);

  // Get with and without options should work (URL options are stripped)
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/?different=option", true)->Size());
}

// Clear operations tests
TEST_F(TestDirectoryCache, ClearDirectory)
{
  CURL url("ftp://test/directory/");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  cache->SetDirectory(url, items, CacheType::ALWAYS);
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());

  cache->ClearDirectory(url);
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());

  CURL urlNotInCache("ftp://test/nonexistent/");

  cache->ClearDirectory(urlNotInCache);
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/nonexistent/", true)->Size());
}

TEST_F(TestDirectoryCache, ClearFile)
{
  CURL dirUrl("ftp://test/directory/");
  CURL fileUrl("ftp://test/directory/file1.txt");

  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));
  AddFile(items, CURL("ftp://test/directory/file2.txt"));

  cache->SetDirectory(dirUrl, items, CacheType::ALWAYS);
  EXPECT_EQ(2, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());

  // Clear a specific file - should clear the entire directory
  cache->ClearFile(fileUrl);
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
}

TEST_F(TestDirectoryCache, ClearSubPathsBug)
{
  // Set up nested directory structure
  CURL parentUrl("ftp://test/parent");
  CURL child1Url("ftp://test/parent/child1/");

  CFileItemList parentItems;
  AddDir(parentItems, child1Url);
  AddFile(parentItems, CURL("ftp://test/parent/file1.txt"));
  cache->SetDirectory(parentUrl, parentItems, CacheType::ALWAYS);

  EXPECT_EQ(2, GetCacheDirectory(*cache, "ftp://test/parent/", true)->Size());

  // Clear subpaths of parent
  cache->ClearSubPaths(parentUrl);

  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/parent/", true)->Size());
}

TEST_F(TestDirectoryCache, ClearSubPaths)
{
  // Set up nested directory structure
  CURL parentUrl("ftp://test/parent/");
  CURL child1Url("ftp://test/parent/child1/");
  CURL child2Url("ftp://test/parent/child2/");
  CURL unrelatedUrl("ftp://test/unrelated/");

  CFileItemList parentItems, child1Items, child2Items, unrelatedItems;

  AddDir(parentItems, child1Url);
  AddDir(parentItems, child2Url);
  AddFile(parentItems, CURL("ftp://test/parent/file1.txt"));

  AddFile(child1Items, CURL("ftp://test/parent/child1/file1.txt"));
  AddFile(child2Items, CURL("ftp://test/parent/child2/file1.txt"));

  AddFile(unrelatedItems, CURL("ftp://test/unrelated/file1.txt"));

  cache->SetDirectory(parentUrl, parentItems, CacheType::ALWAYS);
  cache->SetDirectory(child1Url, child1Items, CacheType::ALWAYS);
  cache->SetDirectory(child2Url, child2Items, CacheType::ALWAYS);
  cache->SetDirectory(unrelatedUrl, unrelatedItems, CacheType::ALWAYS);

  EXPECT_EQ(3, GetCacheDirectory(*cache, "ftp://test/parent/", true)->Size());
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/parent/child1/", true)->Size());
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/parent/child2/", true)->Size());
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/unrelated/", true)->Size());

  // Clear subpaths of parent
  cache->ClearSubPaths(parentUrl);

  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/parent/", true)->Size());
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/parent/child1/", true)->Size());
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/parent/child2/", true)->Size());
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/unrelated/", true)->Size());
}

TEST_F(TestDirectoryCache, ClearAll)
{
  CURL url1("ftp://test/directory1/");
  CURL url2("ftp://test/directory2/");
  CFileItemList items1, items2;
  AddFile(items1, CURL("ftp://test/directory1/file1.txt"));
  AddFile(items2, CURL("ftp://test/directory2/file1.txt"));

  cache->SetDirectory(url1, items1, CacheType::ALWAYS);
  cache->SetDirectory(url2, items2, CacheType::ALWAYS);

  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory1/", true)->Size());
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory2/", true)->Size());

  cache->Clear();

  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory1/", true)->Size());
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory2/", true)->Size());
}

// AddFile tests
TEST_F(TestDirectoryCache, AddFileSlashBug)
{
  CURL dirUrl("ftp://test/directory/");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  cache->SetDirectory(dirUrl, items, CacheType::ALWAYS);
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());

  CURL fileUrl("ftp://test/directory/subfolder/");

  cache->AddFile(fileUrl);
  EXPECT_EQ(2, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
}

TEST_F(TestDirectoryCache, AddFileToExistingDirectory)
{
  CURL dirUrl("ftp://test/directory/");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  cache->SetDirectory(dirUrl, items, CacheType::ALWAYS);
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());

  CURL fileUrl("ftp://test/directory/newfile.txt");

  cache->AddFile(fileUrl);
  EXPECT_EQ(2, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());
}

TEST_F(TestDirectoryCache, AddFileToNonExistentDirectory)
{
  CURL fileUrl("ftp://test/nonexistent/newfile.txt");

  // Should have no effect
  cache->AddFile(fileUrl);
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/nonexistent/", true)->Size());
}

TEST_F(TestDirectoryCache, FileExistsInCache)
{
  CURL dirUrl("ftp://test/directory/");
  CURL fileUrl("ftp://test/directory/file1.txt");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  cache->SetDirectory(dirUrl, items, CacheType::ALWAYS);

  bool foundInCache;
  bool exists = cache->FileExists(fileUrl, foundInCache);

  EXPECT_TRUE(exists);
  EXPECT_TRUE(foundInCache);
}

TEST_F(TestDirectoryCache, FileExistsNotInCache)
{
  CURL fileUrl("ftp://test/directory/file1.txt");

  bool foundInCache;
  bool exists = cache->FileExists(fileUrl, foundInCache);

  EXPECT_FALSE(exists);
  EXPECT_FALSE(foundInCache);
}

TEST_F(TestDirectoryCache, FileExistsDirectoryPath)
{
  CURL dirUrl("ftp://test/directory/");
  CURL fileUrl("ftp://test/directory/file1.txt");

  CFileItemList items;
  AddFile(items, fileUrl);

  cache->SetDirectory(dirUrl, items, CacheType::ALWAYS);

  // Check if directory itself exists (should return true for directory path)
  bool foundInCache;
  bool exists = cache->FileExists(fileUrl, foundInCache);

  EXPECT_TRUE(foundInCache);
  EXPECT_TRUE(exists);
}

TEST_F(TestDirectoryCache, FileDoesntExistsDirectoryPath)
{
  CURL dirUrl("ftp://test/directory/");
  CURL fileUrl("ftp://test/directory/file1.txt");

  CFileItemList items;
  AddFile(items, fileUrl);

  cache->SetDirectory(dirUrl, items, CacheType::ALWAYS);

  // Check if directory itself exists (should return true for directory path)
  bool foundInCache;
  bool exists = cache->FileExists(CURL("ftp://test/directory/DOES_NOT_EXIST.txt"), foundInCache);

  EXPECT_TRUE(foundInCache);
  EXPECT_FALSE(exists);
}

TEST_F(TestDirectoryCache, FileExistsWithURLNormalization)
{
  CURL dirUrl("ftp://test/directory/");
  CURL fileUrl1("ftp://test/directory/file1.txt");
  CURL fileUrl2("ftp://test/directory/file1.txt?option=value");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  cache->SetDirectory(dirUrl, items, CacheType::ALWAYS);

  bool foundInCache1, foundInCache2;
  bool exists1 = cache->FileExists(fileUrl1, foundInCache1);
  bool exists2 = cache->FileExists(fileUrl2, foundInCache2);

  EXPECT_TRUE(exists1);
  EXPECT_TRUE(foundInCache1);
  EXPECT_TRUE(exists2);
  EXPECT_TRUE(foundInCache2);
}

// Cache size management tests
TEST_F(TestDirectoryCache, CacheSizeLimit)
{
  // Add more directories than MAX_CACHED_DIRS (50)
  for (int i = 0; i < 60; i++)
  {
    CURL url("ftp://test/directory" + std::to_string(i) + "/");
    CFileItemList items;
    items.Add(std::make_shared<CFileItem>(
        CURL("ftp://test/directory" + std::to_string(i) + "/file1.txt"), false));

    // Use ONCE cache type so they can be evicted
    cache->SetDirectory(url, items, CacheType::ONCE);
  }

  // Check that some directories were evicted
  EXPECT_FALSE(ExistsInCache(*cache, "ftp://test/directory0/")); // Should have been evicted
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/directory0/", true)->Size());

  // Check that newer directories still exist
  EXPECT_TRUE(ExistsInCache(*cache, "ftp://test/directory59/"));
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory59/", true)->Size());
}

TEST_F(TestDirectoryCache, CacheSizeLimitWithAlwaysCache)
{
  // Add directories with ALWAYS cache type (should not be evicted)
  for (int i = 0; i < 30; i++)
  {
    CURL url("ftp://test/always" + std::to_string(i) + "/");
    CFileItemList items;
    items.Add(std::make_shared<CFileItem>(
        CURL("ftp://test/always" + std::to_string(i) + "/file1.txt"), false));

    cache->SetDirectory(url, items, CacheType::ALWAYS);
  }

  // Add more directories with ONCE cache type
  for (int i = 0; i < 60; i++)
  {
    CURL url("ftp://test/once" + std::to_string(i) + "/");
    CFileItemList items;
    items.Add(std::make_shared<CFileItem>(
        CURL("ftp://test/once" + std::to_string(i) + "/file1.txt"), false));

    cache->SetDirectory(url, items, CacheType::ONCE);
  }

  // ALWAYS cached directories should still exist
  EXPECT_TRUE(ExistsInCache(*cache, "ftp://test/always0/"));
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/always0/", true)->Size());

  EXPECT_TRUE(ExistsInCache(*cache, "ftp://test/always29/"));
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/always29/", true)->Size());

  // Some ONCE cached directories should have been evicted
  EXPECT_FALSE(ExistsInCache(*cache, "ftp://test/once0/"));
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/once0/", true)->Size());

  EXPECT_FALSE(ExistsInCache(*cache, "ftp://test/once9/"));
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/once9/", true)->Size());

  EXPECT_TRUE(ExistsInCache(*cache, "ftp://test/once10/"));
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/once10/", true)->Size());

  EXPECT_TRUE(ExistsInCache(*cache, "ftp://test/once59/"));
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/once59/", true)->Size());

  EXPECT_FALSE(ExistsInCache(*cache, "ftp://test/once60/"));
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/once60/", true)->Size());
}

TEST_F(TestDirectoryCache, EmptyDirectory)
{
  CURL url("ftp://test/empty/");
  CFileItemList items; // Empty list

  cache->SetDirectory(url, items, CacheType::ALWAYS);
  EXPECT_EQ(0, GetCacheDirectory(*cache, "ftp://test/empty/", true)->Size());

  bool foundInCache;
  bool exists = cache->FileExists(CURL("ftp://test/empty/DOES_NOT_EXIST.txt"), foundInCache);

  EXPECT_TRUE(foundInCache);
  EXPECT_FALSE(exists);
}

TEST_F(TestDirectoryCache, MultipleSetDirectoryCalls)
{
  CURL url("ftp://test/directory/");
  CFileItemList items1, items2;
  AddFile(items1, CURL("ftp://test/directory/file1_1.txt"));
  AddFile(items1, CURL("ftp://test/directory/file1_2.txt"));
  AddFile(items2, CURL("ftp://test/directory/file2_1.txt"));

  cache->SetDirectory(url, items1, CacheType::ALWAYS);
  EXPECT_EQ(2, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());

  // Second call should replace the first
  cache->SetDirectory(url, items2, CacheType::ALWAYS);
  EXPECT_EQ(1, GetCacheDirectory(*cache, "ftp://test/directory/", true)->Size());

  // Verify it's the second item
  CFileItemList retrieved;
  cache->GetDirectory(url, retrieved, true);
  EXPECT_EQ("ftp://test/directory/file2_1.txt", retrieved.Get(0)->GetPath());
}

TEST_F(TestDirectoryCache, GetDirectoryReturnValue)
{
  CURL url("ftp://test/directory/");
  CFileItemList items;
  AddFile(items, CURL("ftp://test/directory/file1.txt"));

  cache->SetDirectory(url, items, CacheType::ALWAYS);

  CFileItemList retrieved;
  bool found = cache->GetDirectory(url, retrieved, true);
  EXPECT_TRUE(found);
  EXPECT_EQ(1, retrieved.Size());

  // Non-existent directory
  CURL nonExistentUrl("ftp://test/nonexistent/");
  CFileItemList emptyRetrieved;
  bool notFound = cache->GetDirectory(nonExistentUrl, emptyRetrieved, true);
  EXPECT_FALSE(notFound);
  EXPECT_EQ(0, emptyRetrieved.Size());
}
