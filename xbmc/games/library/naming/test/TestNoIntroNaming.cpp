/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/library/naming/NoIntroNaming.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestNoIntroNaming, ReadsARegion)
{
  EXPECT_EQ(CNoIntroNaming::RegionName("Europe"), "Europe");
  EXPECT_EQ(CNoIntroNaming::RegionName("usa"), "USA");
  EXPECT_EQ(CNoIntroNaming::RegionName("UK"), "United Kingdom");
  EXPECT_TRUE(CNoIntroNaming::RegionName("Proto").empty());
}

TEST(TestNoIntroNaming, ReadsALanguageWrittenItsWay)
{
  EXPECT_TRUE(CNoIntroNaming::IsLanguage("En"));
  EXPECT_TRUE(CNoIntroNaming::IsLanguage("Sv"));
  // Sufami Turbo, not Sesotho
  EXPECT_FALSE(CNoIntroNaming::IsLanguage("ST"));
  EXPECT_FALSE(CNoIntroNaming::IsLanguage("en"));
  EXPECT_FALSE(CNoIntroNaming::IsLanguage("Xx"));
}

TEST(TestNoIntroNaming, ReadsAFeatureTagThatListsSeveral)
{
  EXPECT_TRUE(CNoIntroNaming::IsFeatureTag("SGB Enhanced, GB Compatible"));
  EXPECT_TRUE(CNoIntroNaming::IsFeatureTag("NP"));
  EXPECT_FALSE(CNoIntroNaming::IsFeatureTag("SGB Enhanced, Doritos Promo"));
}

TEST(TestNoIntroNaming, ReadsADisc)
{
  int disc = 0;
  int discs = 0;
  EXPECT_TRUE(CNoIntroNaming::ParseDisc("Disc 2 of 3", disc, discs));
  EXPECT_EQ(disc, 2);
  EXPECT_EQ(discs, 3);
  EXPECT_TRUE(CNoIntroNaming::ParseDisc("Side B", disc, discs));
  EXPECT_EQ(disc, 2);
  EXPECT_FALSE(CNoIntroNaming::ParseDisc("Disc 99999999999", disc, discs));
}

TEST(TestNoIntroNaming, KnowsTheArticlesTheSetsMove)
{
  EXPECT_TRUE(CNoIntroNaming::IsArticle("The"));
  EXPECT_TRUE(CNoIntroNaming::IsArticle("Die"));
  EXPECT_TRUE(CNoIntroNaming::IsArticle("il"));
  EXPECT_FALSE(CNoIntroNaming::IsArticle("Of"));
}
