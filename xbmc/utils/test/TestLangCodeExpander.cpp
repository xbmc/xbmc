/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"

#include <gtest/gtest.h>

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

struct TestBcp47Conversion
{
  std::string input;
  bool status;
  std::string bcp47;
};

std::ostream& operator<<(std::ostream& os, const TestBcp47Conversion& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestBcp47Conversion Bcp47ConversionTests[] = {
    {"en", true, "en"}, // ISO 639-1
    {"eng", true, "en"}, // identical ISO 639-2 B and T
    {"zho", true, "zh"}, // ISO 639-2/T
    {"chi", true, "zh"}, // ISO 639-2/B
    {"zzz", false, ""}, // not assigned
    {"en-AU", true, "en-AU"},
    {"English", true, "en"}, // Description of ISO 639-1 code
    {"Valencian", true, "ca"}, // additional description of cat, which is the alpha-3 of ca
    {"Adygei", true, "ady"}, // Description of ISO 639-2 code
    {"Yang Zhuang", true, "zyg"}, // Description of BCP47 subtags registry language subtag
    {"Dimili", true, "zza"}, // Additional description of ISO 639-2 zza, defined in BCP47 subtags registry
    {"", false, ""},
    {" en ", true, "en"},
    {"EN", true, "en"},
};
// clang-format on

class Bcp47ConversionTester : public testing::Test,
                              public testing::WithParamInterface<TestBcp47Conversion>
{
};

TEST_P(Bcp47ConversionTester, Convert)
{
  std::string output;
  EXPECT_EQ(GetParam().status, g_LangCodeExpander.ConvertToBcp47(GetParam().input, output));
  EXPECT_EQ(GetParam().bcp47, output);
}

INSTANTIATE_TEST_SUITE_P(TestLangCodeExpander,
                         Bcp47ConversionTester,
                         testing::ValuesIn(Bcp47ConversionTests));

struct TestLookup
{
  std::string input;
  bool status;
  std::string expected;
};

// clang-format off
const TestLookup LookupTests[] = {
    {"en", true, "English"},
    {"eng", true, "English"},
    {"en-AU", true, "English (Australia)"},
    {"EN", true, "English"},
    {" en ", true, "English"},
    {"", false, ""},
    {"123", false, ""},
};
// clang-format on

class LookupTester : public testing::Test, public testing::WithParamInterface<TestLookup>
{
};

TEST_P(LookupTester, Lookup)
{
  std::string output;

  EXPECT_EQ(GetParam().status, g_LangCodeExpander.Lookup(GetParam().input, output));
  EXPECT_EQ(GetParam().expected, output);
}

INSTANTIATE_TEST_SUITE_P(TestLangCodeExpander, LookupTester, testing::ValuesIn(LookupTests));

struct TestFindTag
{
  std::string input;
  std::string expected;
};

// clang-format off
const TestFindTag FindTagTests[] = {
    {"track name", ""},
    {"track name {en}", "en"},
    {"track name {en-US}", "en-US"},
    {"track name {es-419}", "es-419"},
    {"track name {en} more text", "en"},
    {"{en} track name", "en"},
    {"track name {", ""},
    {"track name {}", ""},
    {"}{en}{fr}", "en"},
    {"track name {EN}", "en"},
    {"track name { en }", "en"},
};
// clang-format on

class FindTagTester : public testing::Test, public testing::WithParamInterface<TestFindTag>
{
};

TEST_P(FindTagTester, Find)
{
  EXPECT_EQ(GetParam().expected, g_LangCodeExpander.FindLanguageCodeWithSubtag(GetParam().input));
}

INSTANTIATE_TEST_SUITE_P(TestLangCodeExpander, FindTagTester, testing::ValuesIn(FindTagTests));
