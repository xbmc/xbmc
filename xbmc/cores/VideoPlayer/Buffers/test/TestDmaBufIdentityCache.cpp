/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/Buffers/DmaBufIdentityCache.h"

#include <algorithm>

#include <gtest/gtest.h>

using namespace DRMPRIME;

namespace
{

DmaBufIdentity MakeIdentity(uint64_t inode, uint64_t pitch = 1920)
{
  DmaBufIdentity identity;
  identity.nbObjects = 1;
  identity.inode[0] = inode;
  identity.width = 1920;
  identity.height = 1080;
  identity.format = 0x3231564e; // NV12
  identity.nbPlanes = 2;
  identity.pitch[0] = pitch;
  identity.pitch[1] = pitch;
  return identity;
}

} // namespace

TEST(TestDmaBufIdentityCache, MissThenHit)
{
  CDmaBufIdentityCache cache{4};
  const DmaBufIdentity identity = MakeIdentity(7);

  EXPECT_EQ(cache.Lookup(identity), 0u);
  cache.Insert(identity, 100);
  EXPECT_EQ(cache.Lookup(identity), 100u);
  EXPECT_EQ(cache.Size(), 1u);
}

TEST(TestDmaBufIdentityCache, SaltDistinguishesEntries)
{
  CDmaBufIdentityCache cache{4};
  const DmaBufIdentity identity = MakeIdentity(7);

  cache.Insert(identity, 100, 1);
  cache.Insert(identity, 200, 2);
  EXPECT_EQ(cache.Lookup(identity, 1), 100u);
  EXPECT_EQ(cache.Lookup(identity, 2), 200u);
}

TEST(TestDmaBufIdentityCache, ExactLayoutRequiredForHit)
{
  CDmaBufIdentityCache cache{4};
  cache.Insert(MakeIdentity(7, 1920), 100);

  // same memory, different pitch: miss, and the old handle is doomed
  EXPECT_EQ(cache.Lookup(MakeIdentity(7, 2048)), 0u);
  EXPECT_EQ(cache.Size(), 0u);

  const auto reaped = cache.Reap(0, 0);
  ASSERT_EQ(reaped.size(), 1u);
  EXPECT_EQ(reaped[0], 100u);
}

TEST(TestDmaBufIdentityCache, FdRecyclingCannotAlias)
{
  CDmaBufIdentityCache cache{4};
  cache.Insert(MakeIdentity(7), 100);
  cache.Insert(MakeIdentity(8), 200);

  EXPECT_EQ(cache.Lookup(MakeIdentity(7)), 100u);
  EXPECT_EQ(cache.Lookup(MakeIdentity(8)), 200u);
  EXPECT_EQ(cache.Size(), 2u);
}

TEST(TestDmaBufIdentityCache, ReapDrainsDoomedExceptProtected)
{
  CDmaBufIdentityCache cache{4};
  cache.Insert(MakeIdentity(7, 1920), 100);
  EXPECT_EQ(cache.Lookup(MakeIdentity(7, 2048)), 0u); // dooms 100

  // 100 is protected: stays queued
  EXPECT_TRUE(cache.Reap(100, 0).empty());

  const auto reaped = cache.Reap(0, 0);
  ASSERT_EQ(reaped.size(), 1u);
  EXPECT_EQ(reaped[0], 100u);
}

TEST(TestDmaBufIdentityCache, EvictionOverCapIsLRU)
{
  CDmaBufIdentityCache cache{2};
  cache.Insert(MakeIdentity(1), 100);
  cache.Insert(MakeIdentity(2), 200);
  cache.Insert(MakeIdentity(3), 300);
  cache.Insert(MakeIdentity(4), 400);

  // touch the oldest so it is no longer the LRU victim
  EXPECT_EQ(cache.Lookup(MakeIdentity(1)), 100u);

  const auto reaped = cache.Reap(0, 0);
  ASSERT_EQ(reaped.size(), 2u);
  EXPECT_NE(std::find(reaped.begin(), reaped.end(), 200u), reaped.end());
  EXPECT_NE(std::find(reaped.begin(), reaped.end(), 300u), reaped.end());
  EXPECT_EQ(cache.Size(), 2u);
  EXPECT_EQ(cache.Lookup(MakeIdentity(1)), 100u);
  EXPECT_EQ(cache.Lookup(MakeIdentity(4)), 400u);
}

TEST(TestDmaBufIdentityCache, EvictionNeverReturnsProtected)
{
  CDmaBufIdentityCache cache{1};
  cache.Insert(MakeIdentity(1), 100);
  cache.Insert(MakeIdentity(2), 200);
  cache.Insert(MakeIdentity(3), 300);

  // the two LRU victims are protected: only 300 is evictable, cap stays exceeded
  const auto reaped = cache.Reap(100, 200);
  ASSERT_EQ(reaped.size(), 1u);
  EXPECT_EQ(reaped[0], 300u);
  EXPECT_EQ(cache.Size(), 2u);
}

TEST(TestDmaBufIdentityCache, TakeAllReturnsEverythingAndEmpties)
{
  CDmaBufIdentityCache cache{4};
  cache.Insert(MakeIdentity(7, 1920), 100);
  EXPECT_EQ(cache.Lookup(MakeIdentity(7, 2048)), 0u); // dooms 100
  cache.Insert(MakeIdentity(8), 200);
  cache.Insert(MakeIdentity(9), 300);

  auto all = cache.TakeAll();
  std::sort(all.begin(), all.end());
  ASSERT_EQ(all.size(), 3u);
  EXPECT_EQ(all[0], 100u);
  EXPECT_EQ(all[1], 200u);
  EXPECT_EQ(all[2], 300u);
  EXPECT_EQ(cache.Size(), 0u);
  EXPECT_TRUE(cache.Reap(0, 0).empty());
}
