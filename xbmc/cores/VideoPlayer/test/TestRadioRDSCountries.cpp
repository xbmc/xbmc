/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/VideoPlayer/RadioRDSCountries.h"
#include "language/Territory.h"

#include <array>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using KODI::LANGUAGE::CTerritory;

namespace
{

constexpr std::array<unsigned int, 4> EXTENDED_COUNTRY_CODES{0xA0, 0xD0, 0xE0, 0xF0};

// A broadcaster names a place by index, so a cell naming something the region registry does not
// recognise takes the country off the air with nothing to log and nobody to ask. The set of such
// cells is asserted whole rather than skipped, so a cell that stops naming a place is a failure
// even though one cell is known not to.
TEST(TestRadioRDSCountries, OnlyTheCellHoldingANameFailsToNameAPlace)
{
  std::vector<std::string> namesNowhere;

  for (const unsigned int ecc : EXTENDED_COUNTRY_CODES)
  {
    for (unsigned int country = 1; country <= KODI::RDS::COUNTRY_CODE_COUNT; ++country)
    {
      for (unsigned int index = 0; index < KODI::RDS::EXTENDED_COUNTRY_CODE_COUNT; ++index)
      {
        const auto code{KODI::RDS::CountryCode(ecc, country, index)};
        ASSERT_TRUE(code.has_value());
        if (code->empty())
          continue;

        const auto territory{KODI::RDS::Country(ecc, country, index)};
        ASSERT_TRUE(territory.has_value());
        if (territory->ToString().empty())
          namesNowhere.emplace_back(*code);
      }
    }
  }

  EXPECT_EQ(namesNowhere, std::vector<std::string>{"Zanzibar"});
}

// The one cell holding a name rather than a code. ISO 3166-1 assigns Zanzibar nothing - it is
// part of Tanzania, which the neighbouring cell already names - so there is no code to correct
// it to, and it reaches a skin as nowhere rather than as text no skin can use.
TEST(TestRadioRDSCountries, TheCellHoldingANameRatherThanACodeNamesNowhere)
{
  EXPECT_EQ(KODI::RDS::CountryCode(0xD0, 0xD, 2), "Zanzibar");
  EXPECT_TRUE(KODI::RDS::Country(0xD0, 0xD, 2)->ToString().empty());

  EXPECT_EQ(KODI::RDS::Country(0xD0, 0xD, 1), CTerritory::FromCode("TZ"));
}

// The standard predates three of the places it names. A withdrawn code is still a registered
// region subtag, so the broadcast is reported as sent rather than silently reassigned - none of
// the three has a single successor to reassign it to in any case.
TEST(TestRadioRDSCountries, WithdrawnCodesAreReportedAsTheStandardSendsThem)
{
  EXPECT_EQ(KODI::RDS::Country(0xA0, 0xD, 2)->ToString(), "AN"); // Netherlands Antilles
  EXPECT_EQ(KODI::RDS::Country(0xD0, 0xB, 2)->ToString(), "ZR"); // Zaire
  EXPECT_EQ(KODI::RDS::Country(0xE0, 0xD, 2)->ToString(), "YU"); // Yugoslavia

  // and none of them is a country any more
  EXPECT_FALSE(KODI::RDS::Country(0xA0, 0xD, 2)->IsCountry());
  EXPECT_FALSE(KODI::RDS::Country(0xD0, 0xB, 2)->IsCountry());
  EXPECT_FALSE(KODI::RDS::Country(0xE0, 0xD, 2)->IsCountry());
}

// The RBDS branch of the decoder is selected by comparing against these three, so the notation
// they come back in is load-bearing rather than cosmetic.
TEST(TestRadioRDSCountries, TheRbdsCountriesComeBackUppercase)
{
  EXPECT_EQ(KODI::RDS::Country(0xA0, 1, 0)->ToString(), "US");
  EXPECT_EQ(KODI::RDS::Country(0xA0, 0xB, 1)->ToString(), "CA");
  EXPECT_EQ(KODI::RDS::Country(0xA0, 0xB, 5)->ToString(), "MX");
}

TEST(TestRadioRDSCountries, AReservedCellNamesNowhere)
{
  EXPECT_TRUE(KODI::RDS::CountryCode(0xA0, 1, 1)->empty());
  EXPECT_TRUE(KODI::RDS::Country(0xA0, 1, 1)->ToString().empty());

  // xx is a placeholder the standard spells differently, and means the same thing
  EXPECT_TRUE(KODI::RDS::CountryCode(0xD0, 4, 3)->empty());
  EXPECT_TRUE(KODI::RDS::Country(0xD0, 4, 3)->ToString().empty());
}

TEST(TestRadioRDSCountries, ArgumentsNamingNoCellAnswerNothing)
{
  // The tables are 7 wide, and the ECC's low nibble can address 16 columns
  EXPECT_FALSE(KODI::RDS::CountryCode(0xA0, 1, KODI::RDS::EXTENDED_COUNTRY_CODE_COUNT).has_value());
  EXPECT_FALSE(KODI::RDS::Country(0xA0, 1, KODI::RDS::EXTENDED_COUNTRY_CODE_COUNT).has_value());
  EXPECT_FALSE(KODI::RDS::Country(0xA0, KODI::RDS::COUNTRY_CODE_COUNT, 7).has_value());

  // A PI country code outside 1 to 15
  EXPECT_FALSE(KODI::RDS::Country(0xA0, 0, 0).has_value());
  EXPECT_FALSE(KODI::RDS::Country(0xA0, KODI::RDS::COUNTRY_CODE_COUNT + 1, 0).has_value());

  // An extended country code the standard does not define
  EXPECT_FALSE(KODI::RDS::Country(0x00, 1, 0).has_value());
  EXPECT_FALSE(KODI::RDS::Country(0xB0, 1, 0).has_value());
  EXPECT_FALSE(KODI::RDS::Country(0xFF, 1, 0).has_value());
}

} // unnamed namespace
