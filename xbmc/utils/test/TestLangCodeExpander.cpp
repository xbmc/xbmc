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

  // Should return deu instead of failing. Side effect of failure in Convert but maybe happens for historical reasons and something
  // relies on that.
  // For some reason, the function allows the language code to match with the region 'deu',
  // but since it's not a valid ISO639-2/B code, it cannot be translated to its ISO-639-2/T code
  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(g_LangCodeExpander.ConvertToISO6392T("deu", varstr));
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
  std::string audioBcp47;
};

// clang-format off
const TestBcp47Conversion Bcp47ConversionTests[] = {
    {"en", true, "en", "en"}, // ISO 639-1
    {"eng", true, "en", "en"}, // identical ISO 639-2 B and T
    {"zho", true, "zh", "zh"}, // ISO 639-2/T
    {"chi", true, "zh", "zh"}, // ISO 639-2/B
    {"zzz", false, "", ""}, // not assigned
    {"en-AU", true, "en-AU", "en-AU"},
    {"zh-yue-Hant", true, "zh-yue-Hant", "zh-yue"},
    {"English", true, "en", "en"}, // Description of ISO 639-1 code
    {"Valencian", true, "ca", "ca"}, // additional description of cat, which is the alpha-3 of ca
    {"Adygei", true, "ady", "ady"}, // Description of ISO 639-2 code
    {"", false, "", ""},
    {" en ", true, "en", "en"},
    {"EN", true, "en", "en"},
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

  EXPECT_EQ(GetParam().status,
            g_LangCodeExpander.ConvertToBcp47(GetParam().input, output,
                                              CLangCodeExpander::Bcp47Usage::USAGE_AUDIO));
  EXPECT_EQ(GetParam().audioBcp47, output);
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
    // {"English", false, ""}, won't be rejected until registry support is added
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
    {"track name {en} more text", "en"},
    {"{en} track name", "en"},
    {"track name {", ""},
    {"track name {}", ""},
    {"}{en}{fr}", "en"},
    {"track name {English}", ""},
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
