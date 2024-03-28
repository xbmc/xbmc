/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LangCodeExpander.h"

#include <gtest/gtest.h>

TEST(TestLangCodeExpander, ConvertISO6391ToISO6392B)
{
  std::string refstr, varstr;

  refstr = "eng";
  g_LangCodeExpander.ConvertISO6391ToISO6392B("en", varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestLangCodeExpander, ConvertToISO6392B)
{
  std::string refstr, varstr;

  refstr = "eng";
  g_LangCodeExpander.ConvertToISO6392B("en", varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestLangCodeExpander, LookupISO31661)
{
  std::string testString, outString;

  // Test getting the coutry name from ISO 3166-1 alpha-2
  testString = "Australia";
  g_LangCodeExpander.LookupISO31661("AU", outString);
  EXPECT_EQ(testString, outString);

  // Test getting the coutry name from ISO 3166-1 alpha-3
  testString = "New Zealand";
  g_LangCodeExpander.LookupISO31661("NZL", outString);
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 alpha-3 from the ISO 3166-1 alpha-2
  testString = "DEU";
  g_LangCodeExpander.LookupISO31661("DE", outString, true);
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 alpha-2 from the ISO 3166-1 alpha-3
  testString = "MX";
  g_LangCodeExpander.LookupISO31661("MEX", outString, true);
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 alpha-2 from the country name
  testString = "LB";
  g_LangCodeExpander.LookupISO31661("Lebanon", outString, true);
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 alpha-3 from the country name with mixed case
  testString = "GUF";
  g_LangCodeExpander.LookupISO31661("FrEnCh GuIaNa", outString);
  EXPECT_EQ(testString, outString);

  // Test a few failure scenarios.  Return should be an empty string.
  // Test failure to get country name from ISO 3166-1 alpha-2
  testString = "";
  g_LangCodeExpander.LookupISO31661("!!", outString);
  EXPECT_EQ(testString, outString);

  // Test failure to get ISO 3166-1 alpha-3 from country name
  testString = "";
  g_LangCodeExpander.LookupISO31661("NotARealCountryName", outString);
  EXPECT_EQ(testString, outString);

  // Test failure to get ISO 3166-1 alpha-2 from country name
  testString = "";
  g_LangCodeExpander.LookupISO31661("NotARealCountryName", outString, true);
  EXPECT_EQ(testString, outString);
}
