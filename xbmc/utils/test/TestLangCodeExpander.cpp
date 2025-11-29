/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"

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
  std::string refstr;
  std::string varstr;

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
  std::string refstr;
  std::string varstr;

  // ISO 639-2 with identical B and T forms
  refstr = "eng";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("en", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("eng", varstr));
  EXPECT_EQ(refstr, varstr);

  // ISO 639-2/B
  refstr = "fre";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("fre", varstr));
  EXPECT_EQ(refstr, varstr);

  // ISO 639-2/T
  refstr = "cze";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("ces", varstr));
  EXPECT_EQ(refstr, varstr);

  // win_id != iso639_2b
  refstr = "fre";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("fra", varstr, true));
  EXPECT_EQ(refstr, varstr);

  //! \todo analyze, v.suspicious. What old situation required matching languages with regions?
  // Region code
  refstr = "bol";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("bol", varstr));
  EXPECT_EQ(refstr, varstr);

  // non-existent or non-convertible
  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(g_LangCodeExpander.ConvertToISO6392B("ac", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_FALSE(g_LangCodeExpander.ConvertToISO6392B("aaa", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_FALSE(g_LangCodeExpander.ConvertToISO6392B("en-US", varstr));
  EXPECT_EQ(refstr, varstr);

  // Full english name, case insensitive
  refstr = "eng";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("English", varstr, true));
  EXPECT_EQ(refstr, varstr);

  refstr = "eng";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392B("english", varstr, true));
  EXPECT_EQ(refstr, varstr);
}

TEST(TestLangCodeExpander, ConvertToISO6391)
{
  std::string refstr;
  std::string varstr;

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
  std::string refstr;
  std::string varstr;

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
  std::string refstr;
  std::string varstr;

  refstr = "deu";
  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392T("de", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392T("ger", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392T("deu", varstr, true));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(g_LangCodeExpander.ConvertToISO6392T("deu", varstr));
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

struct TestISO6392ToISO6391
{
  std::string input;
  bool status;
  std::string expected;
};

// clang-format off
const TestISO6392ToISO6391 ISO6392ToISO6391Tests[] = {
    {"", false, ""},
    {"en", false, ""}, // ISO 639-1
    {"eng", true, "en"},
    {"tib", true, "bo"},
    {"bod", true, "bo"},
    {"zzz", false, ""},  // not assigned
    {" eng ", true, "en"},
    {"ENG", true, "en"}, // case-insensitive conversion
};
// clang-format on

class ISO6392ToISO6391Tester : public testing::Test,
                               public testing::WithParamInterface<TestISO6392ToISO6391>
{
};

TEST_P(ISO6392ToISO6391Tester, Lookup)
{
  std::string output;

  EXPECT_EQ(GetParam().status,
            g_LangCodeExpander.ConvertISO6392ToISO6391(GetParam().input, output));
  EXPECT_EQ(GetParam().expected, output);
}

INSTANTIATE_TEST_SUITE_P(TestLangCodeExpander,
                         ISO6392ToISO6391Tester,
                         testing::ValuesIn(ISO6392ToISO6391Tests));
