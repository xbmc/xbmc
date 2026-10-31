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
// A territory's code, or "World", which is a market rather than a territory
GameRegion RegionOf(const std::string& code)
{
  return code == "World" ? GameRegion::Make({}, code) : GameRegion::Make(code);
}

GameRelease Release(const std::vector<std::string>& regions, std::string revision = "")
{
  GameRelease release;
  for (const std::string& region : regions)
    release.regions.emplace_back(RegionOf(region));
  release.revision = std::move(revision);
  return release;
}

CReleasePolicy PolicyFor(const std::string& region, const std::string& second = "")
{
  return CReleasePolicy(CReleasePolicy::RegionPriority({RegionOf(region), RegionOf(second)}));
}

std::vector<std::string> PriorityFor(const std::string& region, const std::string& second = "")
{
  std::vector<std::string> codes;
  for (const GameRegion& preferred :
       CReleasePolicy::RegionPriority({RegionOf(region), RegionOf(second)}))
    codes.emplace_back(preferred.ToString());
  return codes;
}
} // namespace

TEST(TestReleasePolicy, PutsThePlayersRegionFirst)
{
  EXPECT_EQ(PriorityFor("GB"), (std::vector<std::string>{"GB", "150", "World", "US", "JP"}));
  EXPECT_EQ(PriorityFor("JP"), (std::vector<std::string>{"JP", "World", "150", "US"}));
  EXPECT_EQ(PriorityFor("CN"), (std::vector<std::string>{"CN", "142", "World", "150", "US", "JP"}));
  EXPECT_EQ(PriorityFor("CA"), (std::vector<std::string>{"CA", "US", "World", "150", "JP"}));
  EXPECT_EQ(PriorityFor("NL"), (std::vector<std::string>{"NL", "150", "World", "US", "JP"}));
  EXPECT_EQ(PriorityFor("SE"), (std::vector<std::string>{"SE", "150", "World", "US", "JP"}));
}

TEST(TestReleasePolicy, TriesASecondChoiceBeforeTheOtherRegions)
{
  EXPECT_EQ(PriorityFor("JP", "US"), (std::vector<std::string>{"JP", "World", "US", "150"}));
  EXPECT_EQ(PriorityFor("GB", "JP"), (std::vector<std::string>{"GB", "150", "World", "JP", "US"}));
  EXPECT_EQ(PolicyFor("JP", "US").PickDefault({Release({"150"}), Release({"US"})}), 1);
}

TEST(TestReleasePolicy, FallsBackWithoutAKnownRegion)
{
  EXPECT_EQ(PriorityFor(""), (std::vector<std::string>{"World", "150", "US", "JP"}));
}

TEST(TestReleasePolicy, PlaysTheReleaseFromThePlayersRegion)
{
  const std::vector<GameRelease> releases{Release({"US"}), Release({"150"}), Release({"JP"})};
  EXPECT_EQ(PolicyFor("GB").PickDefault(releases), 1);
  EXPECT_EQ(PolicyFor("JP").PickDefault(releases), 2);
  EXPECT_EQ(PolicyFor("BR").PickDefault(releases), 0);
}

TEST(TestReleasePolicy, PlaysAWorldReleaseBeforeAnotherRegions)
{
  const std::vector<GameRelease> releases{Release({"US"}), Release({"World"})};
  EXPECT_EQ(PolicyFor("150").PickDefault(releases), 1);
}

TEST(TestReleasePolicy, PrefersAFinishedRelease)
{
  std::vector<GameRelease> releases{Release({"150"}), Release({"US"})};
  releases[0].status = ReleaseStatus::BETA;
  EXPECT_EQ(PolicyFor("150").PickDefault(releases), 1);
}

TEST(TestReleasePolicy, PrefersTheNewestRevision)
{
  EXPECT_EQ(PolicyFor("150").PickDefault(
                {Release({"150"}), Release({"150"}, "Rev 2"), Release({"150"}, "Rev 1")}),
            1);
  EXPECT_EQ(PolicyFor("150").PickDefault({Release({"150"}, "v1.1"), Release({"150"}, "v1.10")}), 1);
  // Each part of a version is compared as a number
  EXPECT_EQ(PolicyFor("150").PickDefault({Release({"150"}, "v2.0"), Release({"150"}, "v1.10")}), 0);
  EXPECT_EQ(PolicyFor("150").PickDefault({Release({"150"}, "v1.9"), Release({"150"}, "v1.10")}), 1);
  // A letter after a number makes a later revision of it, ahead of a verified older one
  EXPECT_EQ(PolicyFor("150").PickDefault({Release({"150"}, "v1.03a"), Release({"150"}, "v1.03b")}),
            1);
  GameRelease verified = Release({"150"}, "v1.03a");
  verified.dump = DumpStatus::VERIFIED;
  EXPECT_EQ(PolicyFor("150").PickDefault({verified, Release({"150"}, "v1.03b")}), 1);
  // Letters count by their place in the alphabet, whichever they are
  EXPECT_EQ(PolicyFor("150").PickDefault(
                {Release({"150"}, "Rev A"), Release({"150"}, "Rev E"), Release({"150"}, "Rev B")}),
            1);
}

TEST(TestReleasePolicy, PrefersAVerifiedDump)
{
  std::vector<GameRelease> releases{Release({"150"}), Release({"150"})};
  releases[1].dump = DumpStatus::VERIFIED;
  EXPECT_EQ(PolicyFor("150").PickDefault(releases), 1);
}

TEST(TestReleasePolicy, NeverPlaysABadDumpWhenThereIsAnother)
{
  std::vector<GameRelease> releases{Release({"150"}, "Rev 1"), Release({"JP"})};
  releases[0].dump = DumpStatus::BAD;
  EXPECT_EQ(PolicyFor("150").PickDefault(releases), 1);
}

TEST(TestReleasePolicy, OffersNothingWithoutReleases)
{
  EXPECT_EQ(PolicyFor("150").PickDefault({}), -1);
}
