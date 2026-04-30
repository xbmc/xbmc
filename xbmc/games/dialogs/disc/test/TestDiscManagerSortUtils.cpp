/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/dialogs/disc/DiscManagerSortUtils.h"

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
void ExpectParsed(const std::string& label, const std::string& expectedStem, int expectedNumber)
{
  const auto parsed = GetNormalizedStemAndTrailingNumber(label);
  ASSERT_TRUE(parsed.has_value()) << label;
  EXPECT_EQ(parsed->first, expectedStem);
  EXPECT_EQ(parsed->second, expectedNumber);
}

void ExpectRejected(const std::string& label)
{
  const auto parsed = GetNormalizedStemAndTrailingNumber(label);
  EXPECT_FALSE(parsed.has_value()) << label;
}
} // namespace

TEST(TestDiscManagerDiscSortUtils, ParsesParenthesizedDiscSuffix)
{
  // Verifies classic parenthesized "Disc N" labels extract a common normalized stem.
  ExpectParsed("Metal Gear Solid (Europe) (Disc 2)", "metal gear solid (europe)", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesVersionedTitleWithParenthesizedDiscSuffix)
{
  // Verifies version metadata remains part of the normalized stem.
  ExpectParsed("Metal Gear Solid (USA) (v1.1) (Disc 2)", "metal gear solid (usa) (v1.1)", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesDiscWithoutParentheses)
{
  // Verifies plain trailing "Disc N" labels are still recognized conservatively.
  ExpectParsed("Final Fantasy VII Disc 1", "final fantasy vii", 1);
}

TEST(TestDiscManagerDiscSortUtils, ParsesLowercasePlainDiscSuffix)
{
  // Verifies lowercase markers are treated the same as title case.
  ExpectParsed("Metal Gear Solid disc 1", "metal gear solid", 1);
}

TEST(TestDiscManagerDiscSortUtils, ParsesDiscWithTotal)
{
  // Verifies explicit total counts in "Disc N of M" labels preserve the disc ordinal.
  ExpectParsed("Final Fantasy VII Disc 2 of 3", "final fantasy vii", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesCdPattern)
{
  // Verifies compact CD markers like "CD2" are supported for display-only sorting.
  ExpectParsed("Driver CD2", "driver", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesBracketedDiscOfTotal)
{
  // Verifies bracketed compact forms such as [disc2of2] are parsed with metadata suffixes.
  ExpectParsed("Game Title [disc2of2][SLUS-00776]", "game title", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesCompactBracketedDiscOfTotal)
{
  // Verifies compact bracketed forms without spaces keep the base title as the stem.
  ExpectParsed("Game Title [disc1of2][SLUS-00776]", "game title", 1);
}

TEST(TestDiscManagerDiscSortUtils, ParsesSpacedBracketedDiscOfTotal)
{
  // Verifies bracketed forms with spaces around the ordinal and total are supported.
  ExpectParsed("Game Title [disc 2 of 2]", "game title", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesBracketedDisc)
{
  // Verifies bracketed forms with spacing like [Disc 1] are parsed consistently.
  ExpectParsed("Game Title [Disc 1]", "game title", 1);
}

TEST(TestDiscManagerDiscSortUtils, ParsesRomanNumeralDiscSuffix)
{
  // Verifies Roman numerals are accepted only with explicit disc markers.
  ExpectParsed("Game Title Disc II", "game title", 2);
}

TEST(TestDiscManagerDiscSortUtils, RejectsBareNumericTitleSuffix)
{
  // Verifies trailing numbers without disc markers remain ambiguous.
  ExpectRejected("Metal Gear Solid 2");
}

TEST(TestDiscManagerDiscSortUtils, RejectsNonDiscMarkerNumericSuffix)
{
  // Verifies other numeric suffixes are not treated as disc ordinals.
  ExpectRejected("Game Title Part 2");
}

TEST(TestDiscManagerDiscSortUtils, RejectsBareDiscLabel)
{
  // Verifies labels without a distinct title stem are not parsed.
  ExpectRejected("Disc 1");
}

TEST(TestDiscManagerDiscSortUtils, PreservesDistinctStemsForDifferentSeries)
{
  // Verifies similarly formatted labels do not collapse across different series names.
  const auto gameADisc = GetNormalizedStemAndTrailingNumber("Game A Disc 1");
  const auto gameBDisc = GetNormalizedStemAndTrailingNumber("Game B Disc 2");

  ASSERT_TRUE(gameADisc.has_value());
  ASSERT_TRUE(gameBDisc.has_value());
  EXPECT_EQ(gameADisc->second, 1);
  EXPECT_EQ(gameBDisc->second, 2);
  EXPECT_NE(gameADisc->first, gameBDisc->first);
}
