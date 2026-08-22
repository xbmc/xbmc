/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "video/VideoDatabaseActorCache.h"

#include <string>

#include <gtest/gtest.h>

using namespace KODI::VIDEO;

// The cache stands in for a lookup the database does with LIKE, so what matters is that it matches
// names the same way that lookup would, and that a caller can tell whether it is answering at all.

TEST(TestVideoDatabaseActorCacheStoredName, TrimsSurroundingWhitespace)
{
  EXPECT_EQ(CActorCache::StoredName("  Heath Ledger  "), "Heath Ledger");
  EXPECT_EQ(CActorCache::StoredName("\t\nHeath Ledger\r\n"), "Heath Ledger");
  EXPECT_EQ(CActorCache::StoredName("Heath Ledger"), "Heath Ledger");
}

TEST(TestVideoDatabaseActorCacheStoredName, KeepsInnerWhitespaceAndCase)
{
  EXPECT_EQ(CActorCache::StoredName("Joseph Gordon-Levitt"), "Joseph Gordon-Levitt");
  EXPECT_EQ(CActorCache::StoredName("HEATH LEDGER"), "HEATH LEDGER");
}

TEST(TestVideoDatabaseActorCacheStoredName, TruncatesToTheColumnWidth)
{
  // The actor table holds 255 characters, so anything longer has to be cut the same way here or a
  // lookup would miss the row it created
  const std::string longName(300, 'a');
  EXPECT_EQ(CActorCache::StoredName(longName).size(), 255U);
  EXPECT_EQ(CActorCache::StoredName(std::string(255, 'a')).size(), 255U);
  EXPECT_EQ(CActorCache::StoredName(std::string(254, 'a')).size(), 254U);
}

TEST(TestVideoDatabaseActorCacheStoredName, TrimsBeforeTruncating)
{
  // 255 characters of name plus padding must still yield the full 255, not 255 including spaces
  const std::string padded{"  " + std::string(255, 'a') + "  "};
  EXPECT_EQ(CActorCache::StoredName(padded), std::string(255, 'a'));
}

TEST(TestVideoDatabaseActorCacheStoredName, HandlesEmptyAndWhitespaceOnly)
{
  EXPECT_EQ(CActorCache::StoredName(""), "");
  EXPECT_EQ(CActorCache::StoredName("   "), "");
}

TEST(TestVideoDatabaseActorCache, StartsEmptyAndUnloaded)
{
  const CActorCache cache;

  EXPECT_FALSE(cache.IsLoaded());
  EXPECT_EQ(cache.Find("Heath Ledger"), nullptr);
}

TEST(TestVideoDatabaseActorCache, FindsWhatWasSet)
{
  CActorCache cache;
  cache.Set("Heath Ledger", 42, "thumb://ledger");

  const auto* actor{cache.Find("Heath Ledger")};
  ASSERT_NE(actor, nullptr);
  EXPECT_EQ(actor->id, 42);
  EXPECT_EQ(actor->artUrls, "thumb://ledger");
}

TEST(TestVideoDatabaseActorCache, MissesNamesItDoesNotHold)
{
  CActorCache cache;
  cache.Set("Heath Ledger", 42, "");

  EXPECT_EQ(cache.Find("Julia Stiles"), nullptr);
  EXPECT_EQ(cache.Find(""), nullptr);
}

TEST(TestVideoDatabaseActorCache, MatchesAsciiCaseInsensitively)
{
  // AddActor() matches with LIKE, which folds ASCII case, so the cache has to agree
  CActorCache cache;
  cache.Set("Heath Ledger", 42, "");

  EXPECT_NE(cache.Find("HEATH LEDGER"), nullptr);
  EXPECT_NE(cache.Find("heath ledger"), nullptr);
  EXPECT_NE(cache.Find("hEaTh LeDgEr"), nullptr);
}

TEST(TestVideoDatabaseActorCache, DoesNotFoldNonAsciiCase)
{
  // SQLite's LIKE only folds ASCII, so these are separate actors in the library and must stay
  // separate here - folding them would merge two rows the database keeps apart
  CActorCache cache;
  cache.Set("BJ\xC3\x96RK", 1, ""); // BJORK with a capital O-umlaut

  EXPECT_NE(cache.Find("bj\xC3\x96rk"), nullptr); // ASCII letters folded, umlaut untouched
  EXPECT_EQ(cache.Find("bj\xC3\xB6rk"), nullptr); // lower case umlaut is a different name
}

TEST(TestVideoDatabaseActorCache, SetReplacesAnEntryRegardlessOfCase)
{
  CActorCache cache;
  cache.Set("Heath Ledger", 1, "first");
  cache.Set("HEATH LEDGER", 2, "second");

  const auto* actor{cache.Find("heath ledger")};
  ASSERT_NE(actor, nullptr);
  EXPECT_EQ(actor->id, 2);
  EXPECT_EQ(actor->artUrls, "second");
}

TEST(TestVideoDatabaseActorCache, RoundTripsNamesNeedingNormalisation)
{
  // How a caller is meant to use it: normalise once, then look up
  CActorCache cache;
  cache.Set(CActorCache::StoredName("  Heath Ledger  "), 42, "");

  EXPECT_NE(cache.Find(CActorCache::StoredName("Heath Ledger")), nullptr);
  EXPECT_NE(cache.Find(CActorCache::StoredName("\theath ledger ")), nullptr);
}

TEST(TestVideoDatabaseActorCache, ReleaseEmptiesAndUnloads)
{
  CActorCache cache;
  cache.Set("Heath Ledger", 42, "");
  cache.Release();

  EXPECT_FALSE(cache.IsLoaded());
  EXPECT_EQ(cache.Find("Heath Ledger"), nullptr);
}

TEST(TestVideoDatabaseActorCache, LoadWithoutADatasetLeavesItUnloaded)
{
  // Nothing may be treated as authoritative if the read didn't happen, or a name missing from the
  // cache would be taken for a new actor and inserted over an existing row
  CActorCache cache;
  cache.Set("Heath Ledger", 42, "");

  cache.Load(nullptr);

  EXPECT_FALSE(cache.IsLoaded());
  EXPECT_EQ(cache.Find("Heath Ledger"), nullptr);
}
