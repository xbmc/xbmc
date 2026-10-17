/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/library/GameAgeRatings.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameAgeRatings, LettersFitTheAgeTheyStandFor)
{
  EXPECT_EQ(CGameAgeRatings::AgeFor("ESRB", "E"), 3);
  EXPECT_EQ(CGameAgeRatings::AgeFor("ESRB", "E10+"), 7);
  EXPECT_EQ(CGameAgeRatings::AgeFor("ESRB", "T"), 12);
  EXPECT_EQ(CGameAgeRatings::AgeFor("ESRB", "M"), 16);
  EXPECT_EQ(CGameAgeRatings::AgeFor("CERO", "B"), 12);
  EXPECT_EQ(CGameAgeRatings::AgeFor("BBFC", "U"), 3);
  EXPECT_EQ(CGameAgeRatings::AgeFor("BBFC", "15"), 16);
}

TEST(TestGameAgeRatings, AWrittenAgeFitsTheNearest)
{
  EXPECT_EQ(CGameAgeRatings::AgeFor("PEGI", "12"), 12);
  EXPECT_EQ(CGameAgeRatings::AgeFor("USK", "0"), 3);
  EXPECT_EQ(CGameAgeRatings::AgeFor("JV", "+3 ans"), 3);
  EXPECT_EQ(CGameAgeRatings::AgeFor("SS", "06"), 7);
  EXPECT_EQ(CGameAgeRatings::AgeFor("VRC", "13"), 12);
  // Halfway between two goes to the older
  EXPECT_EQ(CGameAgeRatings::AgeFor("CLASS_IND", "14"), 16);
  EXPECT_EQ(CGameAgeRatings::AgeFor("SS", "17"), 18);
}

TEST(TestGameAgeRatings, ABoardNeverSeenStillFits)
{
  EXPECT_EQ(CGameAgeRatings::AgeFor("Somewhere", "MA15+"), 16);
  EXPECT_EQ(CGameAgeRatings::AgeFor("Somewhere", "R18+"), 18);
}

TEST(TestGameAgeRatings, NotAnAgeFitsNone)
{
  EXPECT_EQ(CGameAgeRatings::AgeFor("ESRB", "RP"), 0);
  EXPECT_EQ(CGameAgeRatings::AgeFor("ESRB", "NOT RATED"), 0);
  EXPECT_EQ(CGameAgeRatings::AgeFor("GRAC", "Testing"), 0);
  EXPECT_EQ(CGameAgeRatings::AgeFor("PEGI", ""), 0);
}

TEST(TestGameAgeRatings, EachAgeHasABBFCClassification)
{
  EXPECT_EQ(CGameAgeRatings::ClassificationFor("BBFC", 3), "U");
  EXPECT_EQ(CGameAgeRatings::ClassificationFor("BBFC", 7), "PG");
  EXPECT_EQ(CGameAgeRatings::ClassificationFor("BBFC", 12), "12");
  EXPECT_EQ(CGameAgeRatings::ClassificationFor("BBFC", 16), "15");
  EXPECT_EQ(CGameAgeRatings::ClassificationFor("BBFC", 18), "18");
  EXPECT_EQ(CGameAgeRatings::ClassificationFor("JV", 3), "");
}

TEST(TestGameAgeRatings, AGamesAgeIsItsFirstThatIsOne)
{
  const std::vector<GameAgeRating> ratings{{"ESRB", "NOT RATED"}, {"ESRB", "T"}, {"PEGI", "16"}};
  EXPECT_EQ(CGameAgeRatings::AgeOf(ratings), 12);
  EXPECT_EQ(CGameAgeRatings::AgeOf({}), 0);
}
