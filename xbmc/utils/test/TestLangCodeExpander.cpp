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

TEST(TestLangCodeExpander, GetISO31661Alpha2)
{
  std::string testString, outString;

  // Test getting the ISO 3166-1 alpha-2 from the country name
  testString = "LB";
  outString = g_LangCodeExpander.GetISO31661Alpha2("Lebanon");
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 alpha-2 from the alpha-3
  testString = "NZ";
  outString = g_LangCodeExpander.GetISO31661Alpha2("NZL");
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 alpha-2 from the alpha-2
  testString = "CA";
  outString = g_LangCodeExpander.GetISO31661Alpha2("CA");
  EXPECT_EQ(testString, outString);

  // Test with mixed-case country name
  testString = "GF";
  outString = g_LangCodeExpander.GetISO31661Alpha2("FrEnCh GuIaNa");
  EXPECT_EQ(testString, outString);

  // Test with invalid country name
  testString = "";
  outString = g_LangCodeExpander.GetISO31661Alpha2("No-A-Real-Country");
  EXPECT_EQ(testString, outString);

  // Test with lower case code input
  testString = "";
  outString = g_LangCodeExpander.GetISO31661Alpha2("dnk");
  EXPECT_EQ(testString, outString);
}

TEST(TestLangCodeExpander, GetISO31661Alpha3)
{
  std::string testString, outString;

  // Test getting the ISO 3166-1 alpha-3 from the country name
  testString = "CIV";
  outString = g_LangCodeExpander.GetISO31661Alpha3("Côte d'Ivoire");
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 alpha-3 from the alpha-2
  testString = "MLI";
  outString = g_LangCodeExpander.GetISO31661Alpha3("ML");
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 alpha-3 from the alpha-3
  testString = "MTQ";
  outString = g_LangCodeExpander.GetISO31661Alpha3("MTQ");
  EXPECT_EQ(testString, outString);

  // Test with mixed-case country name
  testString = "LIE";
  outString = g_LangCodeExpander.GetISO31661Alpha3("LiEChtENsTeiN");
  EXPECT_EQ(testString, outString);

  // Test with invalid country name
  testString = "";
  outString = g_LangCodeExpander.GetISO31661Alpha3("e=mc2");
  EXPECT_EQ(testString, outString);

  // Test with lower case code input
  testString = "";
  outString = g_LangCodeExpander.GetISO31661Alpha2("ch");
  EXPECT_EQ(testString, outString);
}

TEST(TestLangCodeExpander, GetISO31661Name)
{
  std::string testString, outString;

  // Test getting the ISO 3166-1 name from the country name
  testString = "Norway";
  outString = g_LangCodeExpander.GetISO31661Name("Norway");
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 name from the alpha-2
  testString = "Papua New Guinea";
  outString = g_LangCodeExpander.GetISO31661Name("PG");
  EXPECT_EQ(testString, outString);

  // Test getting the ISO 3166-1 name from the alpha-3
  testString = "South Sudan";
  outString = g_LangCodeExpander.GetISO31661Name("SSD");
  EXPECT_EQ(testString, outString);

  // Test with mixed-case country name
  testString = "Seychelles";
  outString = g_LangCodeExpander.GetISO31661Name("SeYcHElLEs");
  EXPECT_EQ(testString, outString);

  // Test with invalid country name
  testString = "";
  outString = g_LangCodeExpander.GetISO31661Name("Some-Random-Text");
  EXPECT_EQ(testString, outString);
}
