/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LangCodeExpander.h"

#include <gtest/gtest.h>

// Trick to make protected methods accessible for testing
class CLangCodeExpanderTest : public CLangCodeExpander
{
public:
  static bool CallLookupInISO639Tables(const std::string& code, std::string& desc)
  {
    return LookupInISO639Tables(code, desc);
  }
};

TEST(TestLangCodeExpander, ConvertISO6391ToISO6392B)
{
  std::string refstr, varstr;

  refstr = "eng";
  EXPECT_TRUE(g_LangCodeExpander.ConvertISO6391ToISO6392B("en", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "fre";
  EXPECT_TRUE(g_LangCodeExpander.ConvertISO6391ToISO6392B("fr", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "fra";
  EXPECT_TRUE(g_LangCodeExpander.ConvertISO6391ToISO6392B("fr", varstr, true));
  EXPECT_EQ(refstr, varstr);

  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(g_LangCodeExpander.ConvertISO6391ToISO6392B("eng", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_FALSE(g_LangCodeExpander.ConvertISO6391ToISO6392B("ac", varstr));
  EXPECT_EQ(refstr, varstr);
}

TEST(TestLangCodeExpander, ConvertToISO6392B)
{
  std::string refstr, varstr;

  refstr = "eng";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("en", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("eng", varstr));
  EXPECT_EQ(refstr, varstr);

  // win_id != iso639_2b
  refstr = "fra";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("fra", varstr, true));
  EXPECT_EQ(refstr, varstr);

  // Region code
  refstr = "bol";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("bol", varstr));
  EXPECT_EQ(refstr, varstr);

  // non existent
  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(g_LangCodeExpander.ConvertToISO6392B("ac", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_FALSE(g_LangCodeExpander.ConvertToISO6392B("aaa", varstr));
  EXPECT_EQ(refstr, varstr);

  // Full english name, case insensitive
  refstr = "eng";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("English", varstr, true));
  EXPECT_EQ(refstr, varstr);

  refstr = "eng";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("english", varstr, true));
  EXPECT_EQ(refstr, varstr);

  // Existing bug
  //refstr = "ger";
  //EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("deu", varstr));
  //EXPECT_EQ(refstr, varstr);
}

TEST(TestLangCodeExpander, ConvertToISO6391)
{
  std::string refstr, varstr;

  refstr = "en";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6391("en", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6391("eng", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "fr";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6391("fre", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6391("fra", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "bo";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6391("bol", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(g_LangCodeExpander.ConvertToISO6391("aaa", varstr));
  EXPECT_EQ(refstr, varstr);

  // Full english name, with iso 639-1 match
  refstr = "en";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6391("English", varstr));
  EXPECT_EQ(refstr, varstr);

  // Full english name, with iso 639-2 match and conversion to iso 639-1
  refstr = "ab";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6391("Abkhaz", varstr));
  EXPECT_EQ(refstr, varstr);
}

#ifdef TARGET_WINDOWS
TEST(TestLangCodeExpander, ConvertWindowsLanguageCodeToISO6392B)
{
  std::string refstr, varstr;

  refstr = "slo";
  EXPECT_TRUE(g_LangCodeExpander.ConvertWindowsLanguageCodeToISO6392B("slk", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertWindowsLanguageCodeToISO6392B("slo", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(g_LangCodeExpander.ConvertWindowsLanguageCodeToISO6392B("aaa", varstr));
  EXPECT_EQ(refstr, varstr);
}
#endif

TEST(TestLangCodeExpander, ConvertToISO6392T)
{
  std::string refstr, varstr;

  refstr = "deu";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392T("de", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392T("ger", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392T("deu", varstr, true));
  EXPECT_EQ(refstr, varstr);

  // Should return deu instead of failing. Side effect of failure in Convert but maybe happens for historical reasons and something
  // relies on that.
  // For some reason, the function allows the language code to match with the region 'deu',
  // but since it's not a valid ISO639-2/B code, it cannot be translated to its ISO-639-2/T code
  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(g_LangCodeExpander.ConvertToISO6392T("deu", varstr));
  EXPECT_EQ(refstr, varstr);
}

TEST(TestLangCodeExpander, LookupInISO639Tables)
{
  std::string refstr, varstr;

  refstr = "English";
  EXPECT_TRUE(CLangCodeExpanderTest::CallLookupInISO639Tables("en", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(CLangCodeExpanderTest::CallLookupInISO639Tables("eng", varstr));
  EXPECT_EQ(refstr, varstr);

  // There are no ISO 639-1 codes reserved for private use - use currently unassigned zz to test expected failure
  EXPECT_FALSE(CLangCodeExpanderTest::CallLookupInISO639Tables("zz", varstr));

  // Not alpha-2 or alpha-3 code format
  EXPECT_FALSE(CLangCodeExpanderTest::CallLookupInISO639Tables("a", varstr));
  EXPECT_FALSE(CLangCodeExpanderTest::CallLookupInISO639Tables("fr-CA", varstr));
}
