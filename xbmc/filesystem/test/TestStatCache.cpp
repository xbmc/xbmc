/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URL.h"
#include "filesystem/StatCache.h"

#include <chrono>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "PlatformDefs.h"

using namespace XFILE;
using namespace std::chrono_literals;

namespace
{
struct __stat64 MakeStat(long long size)
{
  struct __stat64 buffer
  {
  };
  buffer.st_size = size;
  return buffer;
}
} // namespace

class TestStatCache : public ::testing::Test
{
protected:
  void SetUp() override { cache = std::make_unique<CStatCache>(); }

  void TearDown() override { cache.reset(); }

  // Directly age every entry currently in the cache so it looks like it was
  // stored STAT_CACHE_TTL (15s) or more in the past, without sleeping in the test.
  void ExpireAllEntries() const
  {
    for (auto& entry : cache->m_cache)
      entry.second.m_time -= (STAT_CACHE_TTL + 1s);
  }

  std::unique_ptr<CStatCache> cache;
};

TEST_F(TestStatCache, MissOnEmptyCache)
{
  struct __stat64 result
  {
  };
  EXPECT_FALSE(cache->Get(CURL("smb://server/share/file.mkv"), &result));
}

TEST_F(TestStatCache, SetThenGetHits)
{
  CURL url("smb://server/share/file.mkv");
  struct __stat64 stored = MakeStat(1234);
  cache->Set(url, &stored);

  struct __stat64 result
  {
  };
  ASSERT_TRUE(cache->Get(url, &result));
  EXPECT_EQ(1234, result.st_size);
}

TEST_F(TestStatCache, TrailingSlashIsNormalized)
{
  CURL urlWithSlash("smb://server/share/dir/");
  struct __stat64 stored = MakeStat(1);
  cache->Set(urlWithSlash, &stored);

  struct __stat64 result
  {
  };
  EXPECT_TRUE(cache->Get(CURL("smb://server/share/dir"), &result));
}

TEST_F(TestStatCache, DifferentOptionsAreDistinctKeys)
{
  CURL urlA("http://server/resource?id=1");
  CURL urlB("http://server/resource?id=2");
  CURL urlNoOptions("http://server/resource");

  struct __stat64 storedA = MakeStat(111);
  cache->Set(urlA, &storedA);

  struct __stat64 result
  {
  };
  EXPECT_TRUE(cache->Get(urlA, &result));
  EXPECT_EQ(111, result.st_size);

  // A different (or missing) query string is a different resource identity
  // and must not be served from the entry cached for urlA.
  EXPECT_FALSE(cache->Get(urlB, &result));
  EXPECT_FALSE(cache->Get(urlNoOptions, &result));
}

TEST_F(TestStatCache, RemoveDropsExactEntry)
{
  CURL url("smb://server/share/file.mkv");
  struct __stat64 stored = MakeStat(1);
  cache->Set(url, &stored);

  struct __stat64 result
  {
  };
  ASSERT_TRUE(cache->Get(url, &result));

  cache->Remove(url);
  EXPECT_FALSE(cache->Get(url, &result));
}

TEST_F(TestStatCache, RemoveOnMissingEntryIsNoop)
{
  // Should not throw or affect unrelated entries.
  CURL other("smb://server/share/other.mkv");
  struct __stat64 stored = MakeStat(1);
  cache->Set(other, &stored);

  cache->Remove(CURL("smb://server/share/nonexistent.mkv"));

  struct __stat64 result
  {
  };
  EXPECT_TRUE(cache->Get(other, &result));
}

TEST_F(TestStatCache, RemoveRecursiveRemovesSelfAndDescendants)
{
  CURL dirUrl("smb://server/share/dir");
  CURL childUrl("smb://server/share/dir/child.mkv");
  CURL grandchildUrl("smb://server/share/dir/sub/grandchild.mkv");
  CURL unrelatedUrl("smb://server/share/other.mkv");

  struct __stat64 stored = MakeStat(1);
  cache->Set(dirUrl, &stored);
  cache->Set(childUrl, &stored);
  cache->Set(grandchildUrl, &stored);
  cache->Set(unrelatedUrl, &stored);

  cache->RemoveRecursive(dirUrl);

  struct __stat64 result
  {
  };
  EXPECT_FALSE(cache->Get(dirUrl, &result));
  EXPECT_FALSE(cache->Get(childUrl, &result));
  EXPECT_FALSE(cache->Get(grandchildUrl, &result));
  EXPECT_TRUE(cache->Get(unrelatedUrl, &result));
}

TEST_F(TestStatCache, RemoveRecursiveIgnoresOptions)
{
  // Cache keys carry URL options, but RemoveRecursive must still find and
  // remove entries whose options differ from (or are absent from) the URL
  // it's asked to invalidate, since options aren't part of the path hierarchy.
  // Note: '?' is only parsed as URL options for a handful of protocols
  // (eg. nfs, http) - see CURL::Parse - so use one of those here.
  CURL dirUrl("nfs://server/share/dir?some=option");
  CURL childNoOptions("nfs://server/share/dir/child.mkv");

  struct __stat64 stored = MakeStat(1);
  cache->Set(dirUrl, &stored);
  cache->Set(childNoOptions, &stored);

  cache->RemoveRecursive(CURL("nfs://server/share/dir"));

  struct __stat64 result
  {
  };
  EXPECT_FALSE(cache->Get(dirUrl, &result));
  EXPECT_FALSE(cache->Get(childNoOptions, &result));
}

TEST_F(TestStatCache, ClearRemovesEverything)
{
  CURL url1("smb://server/share/file1.mkv");
  CURL url2("smb://server/share/file2.mkv");
  struct __stat64 stored = MakeStat(1);
  cache->Set(url1, &stored);
  cache->Set(url2, &stored);

  cache->Clear();

  struct __stat64 result
  {
  };
  EXPECT_FALSE(cache->Get(url1, &result));
  EXPECT_FALSE(cache->Get(url2, &result));
}

TEST_F(TestStatCache, EntryExpiresAfterTTL)
{
  CURL url("smb://server/share/file.mkv");
  struct __stat64 stored = MakeStat(1);
  cache->Set(url, &stored);

  struct __stat64 result
  {
  };
  ASSERT_TRUE(cache->Get(url, &result));

  ExpireAllEntries();

  EXPECT_FALSE(cache->Get(url, &result));
}

TEST_F(TestStatCache, EvictsLeastRecentlyUsedWhenFull)
{
  // MAX_CACHED_STATS is 200; fill the cache past that limit with unique
  // entries and expect the oldest (least recently accessed) to be evicted.
  struct __stat64 stored = MakeStat(1);
  for (int i = 0; i < MAX_CACHED_STATS; ++i)
    cache->Set(CURL("smb://server/share/file" + std::to_string(i) + ".mkv"), &stored);

  // Touch file0 so it becomes the most-recently-accessed entry and should
  // survive the eviction that happens when the next new entry is added.
  struct __stat64 result
  {
  };
  ASSERT_TRUE(cache->Get(CURL("smb://server/share/file0.mkv"), &result));

  cache->Set(CURL("smb://server/share/file" + std::to_string(MAX_CACHED_STATS) + ".mkv"), &stored);

  EXPECT_TRUE(cache->Get(CURL("smb://server/share/file0.mkv"), &result));
  EXPECT_TRUE(cache->Get(
      CURL("smb://server/share/file" + std::to_string(MAX_CACHED_STATS) + ".mkv"), &result));
  // file1 was the least recently accessed entry at the time of the new
  // insertion (file0 was refreshed, file2+ are newer), so it gets evicted.
  EXPECT_FALSE(cache->Get(CURL("smb://server/share/file1.mkv"), &result));
}
