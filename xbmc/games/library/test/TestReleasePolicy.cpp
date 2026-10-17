/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/library/ReleasePolicy.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
GameRelease Release(std::vector<std::string> regions, std::string revision = "")
{
  GameRelease release;
  release.regions = std::move(regions);
  release.revision = std::move(revision);
  release.files.emplace_back();
  return release;
}

CReleasePolicy PolicyFor(const std::string& region)
{
  return CReleasePolicy(CReleasePolicy::RegionPriority(region), true, true, true);
}
} // namespace

TEST(TestReleasePolicy, PutsThePlayersRegionFirst)
{
  EXPECT_EQ(CReleasePolicy::RegionPriority("United Kingdom"),
            (std::vector<std::string>{"United Kingdom", "Europe", "World", "USA", "Japan"}));
  EXPECT_EQ(CReleasePolicy::RegionPriority("Japan"),
            (std::vector<std::string>{"Japan", "World", "Europe", "USA"}));
  EXPECT_EQ(CReleasePolicy::RegionPriority("Canada"),
            (std::vector<std::string>{"Canada", "USA", "World", "Europe", "Japan"}));
}

TEST(TestReleasePolicy, PlaysTheReleaseFromThePlayersRegion)
{
  const std::vector<GameRelease> releases{Release({"USA"}), Release({"Europe"}),
                                          Release({"Japan"})};
  EXPECT_EQ(PolicyFor("United Kingdom").PickDefault(releases), 1);
  EXPECT_EQ(PolicyFor("Japan").PickDefault(releases), 2);
  EXPECT_EQ(PolicyFor("Brazil").PickDefault(releases), 0);
}

TEST(TestReleasePolicy, PlaysAWorldReleaseBeforeAnotherRegions)
{
  const std::vector<GameRelease> releases{Release({"USA"}), Release({"World"})};
  EXPECT_EQ(PolicyFor("Europe").PickDefault(releases), 1);
}

TEST(TestReleasePolicy, PrefersAFinishedRelease)
{
  std::vector<GameRelease> releases{Release({"Europe"}), Release({"USA"})};
  releases[0].status = ReleaseStatus::BETA;
  EXPECT_EQ(PolicyFor("Europe").PickDefault(releases), 1);

  const CReleasePolicy regionFirst(CReleasePolicy::RegionPriority("Europe"), false, true, true);
  EXPECT_EQ(regionFirst.PickDefault(releases), 0);
}

TEST(TestReleasePolicy, PrefersTheNewestRevision)
{
  EXPECT_EQ(PolicyFor("Europe").PickDefault(
                {Release({"Europe"}), Release({"Europe"}, "Rev 2"), Release({"Europe"}, "Rev 1")}),
            1);
  EXPECT_EQ(
      PolicyFor("Europe").PickDefault({Release({"Europe"}, "v1.1"), Release({"Europe"}, "v1.10")}),
      1);
  // Letters count by their place in the alphabet, whichever they are
  EXPECT_EQ(
      PolicyFor("Europe").PickDefault({Release({"Europe"}, "Rev A"), Release({"Europe"}, "Rev E"),
                                       Release({"Europe"}, "Rev B")}),
      1);
}

TEST(TestReleasePolicy, PrefersAVerifiedDump)
{
  std::vector<GameRelease> releases{Release({"Europe"}), Release({"Europe"})};
  releases[1].dump = DumpStatus::VERIFIED;
  EXPECT_EQ(PolicyFor("Europe").PickDefault(releases), 1);
}

TEST(TestReleasePolicy, NeverPlaysABadDumpWhenThereIsAnother)
{
  std::vector<GameRelease> releases{Release({"Europe"}, "Rev 1"), Release({"Japan"})};
  releases[0].dump = DumpStatus::BAD;
  EXPECT_EQ(PolicyFor("Europe").PickDefault(releases), 1);
}

TEST(TestReleasePolicy, PlaysOnlyAReleaseWithFiles)
{
  std::vector<GameRelease> releases{Release({"Europe"}), Release({"USA"})};
  releases[0].files.clear();
  EXPECT_EQ(PolicyFor("Europe").PickDefault(releases), 1);
  EXPECT_EQ(PolicyFor("Europe").PickDefault({}), -1);
}
