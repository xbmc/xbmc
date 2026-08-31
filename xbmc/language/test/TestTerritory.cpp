/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "language/Territory.h"

#include <gtest/gtest.h>

using namespace KODI::LANGUAGE;

TEST(TestTerritory, HoldsTheCodeAsBcp47StatesIt)
{
  const CTerritory territory{CTerritory::FromCode("GB")};

  EXPECT_EQ(territory.ToString(), "GB");

  // The case a code is written in is not part of it
  EXPECT_EQ(CTerritory::FromCode("gb"), territory);
  EXPECT_EQ(CTerritory::FromCode(" GB "), territory);
}

TEST(TestTerritory, NamesTheAlphaForms)
{
  const CTerritory territory{CTerritory::FromCode("GB")};

  EXPECT_EQ(territory.AsIso3166_1Alpha2(), "GB");
  EXPECT_EQ(territory.AsIso3166_1Alpha3(), "GBR");

  EXPECT_EQ(CTerritory::FromCode("NL").AsIso3166_1Alpha3(), "NLD");
  EXPECT_EQ(CTerritory::FromCode("mx").AsIso3166_1Alpha3(), "MEX");
}

TEST(TestTerritory, TakesTheAreasIso3166DoesNotCover)
{
  // BCP 47 states an area wider than a country with a UN M.49 number - es-419 is the
  // advancedsettings <languagecodes> worked example
  const CTerritory latinAmerica{CTerritory::FromCode("419")};

  EXPECT_EQ(latinAmerica.ToString(), "419");
  EXPECT_EQ(latinAmerica.ToEnglishName(), "Latin America and the Caribbean");
}

TEST(TestTerritory, AWiderAreaIsNotACountryAndHasNoIso3166Code)
{
  // The question a caller contracting to supply a country has to ask, answered by the type
  // rather than left to each of them
  const CTerritory latinAmerica{CTerritory::FromCode("419")};

  EXPECT_FALSE(latinAmerica.IsCountry());
  EXPECT_EQ(latinAmerica.AsIso3166_1Alpha2(), "");
  EXPECT_EQ(latinAmerica.AsIso3166_1Alpha3(), "");

  EXPECT_TRUE(CTerritory::FromCode("GB").IsCountry());
}

TEST(TestTerritory, TakesEveryRegionALanguageTagCanState)
{
  // A region means the same thing standing alone as it does inside a tag, because both ask
  // CBcp47. The registry accepts 23 alpha-2 regions ISO 3166-1 does not currently assign, and a
  // tag carrying one must not yield a territory that denies being a territory.
  for (const auto* region : {"EU", "UN", "IC", "EA", "DG", "AC", "TA", "EZ"}) // reserved
    EXPECT_TRUE(CTerritory::FromCode(region) != CTerritory{}) << region;

  for (const auto* region : {"DD", "SU", "YU", "AN", "CS", "ZR", "TP", "BU"}) // withdrawn
    EXPECT_TRUE(CTerritory::FromCode(region) != CTerritory{}) << region;

  // Accepted as regions, but ISO 3166-1 assigns them nothing, so they are not countries
  EXPECT_FALSE(CTerritory::FromCode("EU").IsCountry());
  EXPECT_FALSE(CTerritory::FromCode("DD").IsCountry());
  EXPECT_EQ(CTerritory::FromCode("EU").AsIso3166_1Alpha2(), "");

  // and they are still named
  EXPECT_EQ(CTerritory::FromCode("EU").ToEnglishName(), "European Union");

  // RFC 5646 permits the private-use codes, so a tag can state them and so can a territory
  EXPECT_NE(CTerritory::FromCode("AA"), CTerritory{});
  EXPECT_NE(CTerritory::FromCode("ZZ"), CTerritory{});
  EXPECT_NE(CTerritory::FromCode("QM"), CTerritory{});
}

TEST(TestTerritory, NamesThePlaceInEnglish)
{
  // The ISO 3166-1 table holds that standard's official names, which carry a trailing article
  // where the English name takes one. They are reproduced rather than tidied, so that the name
  // a caller shows is the one the standard publishes.
  EXPECT_EQ(CTerritory::FromCode("GB").ToEnglishName(),
            "United Kingdom of Great Britain and Northern Ireland (the)");
  EXPECT_EQ(CTerritory::FromCode("MX").ToEnglishName(), "Mexico");
}

TEST(TestTerritory, TextNamingNoPlaceYieldsNoTerritory)
{
  // Naming no place is an ordinary state rather than a failure, so text naming none yields the
  // same territory a tag stating no region does, and every member answers with nothing for it
  EXPECT_EQ(CTerritory::FromCode(""), CTerritory{});
  EXPECT_EQ(CTerritory::FromCode("not a place"), CTerritory{});
  EXPECT_EQ(CTerritory::FromCode("OO"), CTerritory{}); // region-shaped, assigned to nothing
  EXPECT_EQ(CTerritory::FromCode("999"), CTerritory{}); // not a registered M.49 area

  // BCP 47 has no alpha-3 region, so nothing states a place that way. Every region locale in
  // every official language pack is alpha-2, and the two that are not - ga_ie's GA-DUB and szl's
  // SZL, an ISO 639-3 language code - name no place in any notation.
  EXPECT_EQ(CTerritory::FromCode("GBR"), CTerritory{});
  EXPECT_EQ(CTerritory::FromCode("SZL"), CTerritory{});
  EXPECT_EQ(CTerritory::FromCode("GA-DUB"), CTerritory{});
}
