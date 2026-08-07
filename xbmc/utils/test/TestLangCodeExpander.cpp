/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/LangCodeExpander.h"
#include "utils/XBMCTinyXML.h"

#include <gtest/gtest.h>

namespace
{

//! Reaches the protected lookup, which is not part of the public conversion surface
class CLangCodeExpanderTest : public CLangCodeExpander
{
public:
  static bool LookupUC(const std::string& desc, std::string& userCode)
  {
    return LookupUserCode(desc, userCode);
  }
};

} // namespace

TEST(TestLangCodeExpander, ParseUserCodes)
{
  const std::string xml =
      R"(<advancedsettings>
           <languagecodes>
             <code>
              <short>1</short>
              <long>2</long>
             </code>
             <code>
              <short>3</short>
              <long>4</long>
             </code>
           </languagecodes>
         </advancedsettings>)";

  CXBMCTinyXML doc;
  doc.Parse(xml);
  ASSERT_TRUE(doc.RootElement() != nullptr);
  ASSERT_TRUE(doc.RootElement()->FirstChildElement("languagecodes") != nullptr);
  CLangCodeExpander::LoadUserCodes(doc.RootElement()->FirstChildElement("languagecodes"));
  std::string code;
  EXPECT_TRUE(CLangCodeExpanderTest::LookupUC("2", code));
  EXPECT_EQ(code, "1");
  EXPECT_TRUE(CLangCodeExpanderTest::LookupUC("4", code));
  EXPECT_EQ(code, "3");

  // The codes are process-wide, so leaving them loaded would leak into other tests
  CLangCodeExpander::Clear();
}

TEST(TestLangCodeExpander, ConvertISO6391ToISO6392B)
{
  std::string refstr;
  std::string varstr;

  refstr = "eng";
  EXPECT_TRUE(CLangCodeExpander::ConvertISO6391ToISO6392B("en", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "fre";
  EXPECT_TRUE(CLangCodeExpander::ConvertISO6391ToISO6392B("fr", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(CLangCodeExpander::ConvertISO6391ToISO6392B("eng", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_FALSE(CLangCodeExpander::ConvertISO6391ToISO6392B("ac", varstr));
  EXPECT_EQ(refstr, varstr);
}

TEST(TestLangCodeExpander, ConvertToISO6392B)
{
  std::string refstr;
  std::string varstr;

  // ISO 639-2 with identical B and T forms
  refstr = "eng";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6392B("en", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6392B("eng", varstr));
  EXPECT_EQ(refstr, varstr);

  // ISO 639-2/B
  refstr = "fre";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6392B("fre", varstr));
  EXPECT_EQ(refstr, varstr);

  // ISO 639-2/T
  refstr = "cze";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6392B("ces", varstr));
  EXPECT_EQ(refstr, varstr);

  // ISO 639-2/T maps to the B form
  refstr = "fre";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6392B("fra", varstr));
  EXPECT_EQ(refstr, varstr);

  // An ISO 639-2 code with no ISO 639-1 equivalent is absent from the alpha-2 keyed table and has
  // to be recognized as already being the wanted code. ast is Asturian, a Kodi UI language.
  refstr = "ast";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6392B("ast", varstr));
  EXPECT_EQ(refstr, varstr);

  // A region code is not a language here. bol is Bolivia, and although ISO 639-3 assigns it to
  // Bole, ISO 639-2 does not, so there is nothing to convert to.
  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(CLangCodeExpander::ConvertToISO6392B("bol", varstr));
  EXPECT_EQ(refstr, varstr);

  // non-existent or non-convertible
  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(CLangCodeExpander::ConvertToISO6392B("ac", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_FALSE(CLangCodeExpander::ConvertToISO6392B("aaa", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_FALSE(CLangCodeExpander::ConvertToISO6392B("en-US", varstr));
  EXPECT_EQ(refstr, varstr);

  // Full english name, case insensitive
  refstr = "eng";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6392B("English", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "eng";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6392B("english", varstr));
  EXPECT_EQ(refstr, varstr);
}

TEST(TestLangCodeExpander, ConvertToISO6391)
{
  std::string refstr;
  std::string varstr;

  refstr = "en";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6391("en", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6391("eng", varstr));
  EXPECT_EQ(refstr, varstr);

  refstr = "fr";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6391("fre", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6391("fra", varstr));
  EXPECT_EQ(refstr, varstr);

  // ISO 3166-1 and ISO 639-1 share the two letter namespace without sharing meanings: bol is
  // Bolivia, whose alpha-2 bo is the code for Tibetan. A region must never be converted here.
  refstr = "invalid";
  varstr = "invalid";
  EXPECT_FALSE(CLangCodeExpander::ConvertToISO6391("bol", varstr));
  EXPECT_EQ(refstr, varstr);

  EXPECT_FALSE(CLangCodeExpander::ConvertToISO6391("aaa", varstr));
  EXPECT_EQ(refstr, varstr);

  // Full english name, with iso 639-1 match
  refstr = "en";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6391("English", varstr));
  EXPECT_EQ(refstr, varstr);

  // Full english name, with iso 639-2 match and conversion to iso 639-1
  refstr = "ab";
  EXPECT_TRUE(CLangCodeExpander::ConvertToISO6391("Abkhaz", varstr));
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
            CLangCodeExpander::ConvertISO6392ToISO6391(GetParam().input, output));
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
    // The four special-scope subtags have no alpha-2 code, so the alpha-3 is what a name resolves
    // to and what a caller gets back
    {"Undetermined", true, "und"},
    {"No linguistic content", true, "zxx"},
    {"Uncoded languages", true, "mis"},
    {"Multiple languages", true, "mul"},
    {"", false, ""},
    // A three letter region is not a language to ISO 639-2, but ISO 639-3 assigns bol to Bole
    // and every 639-3 code is a registered BCP 47 subtag, so this notation does resolve it
    {"bol", true, "bol"},
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
  EXPECT_EQ(GetParam().status, CLangCodeExpander::ConvertToBcp47(GetParam().input, output));
  EXPECT_EQ(GetParam().bcp47, output);
}

INSTANTIATE_TEST_SUITE_P(TestLangCodeExpander,
                         Bcp47ConversionTester,
                         testing::ValuesIn(Bcp47ConversionTests));

struct TestBcp47ToIso6392BConversion
{
  std::string input;
  std::string iso6392B;
};

std::ostream& operator<<(std::ostream& os, const TestBcp47ToIso6392BConversion& rhs)
{
  return os << rhs.input;
}

// clang-format off
const TestBcp47ToIso6392BConversion Bcp47ToIso6392BConversionTests[] = {
    {"en", "eng"}, // alpha-2 primary subtag
    {"zh", "chi"}, // B and T forms differ - B form expected
    {"ady", "ady"}, // alpha-3 primary subtag, no alpha-2 exists
    {"eng", "eng"}, // already an ISO 639-2/B code, passes through unharmed
    {"zho", "chi"}, // ISO 639-2/T maps to the B form
    // Region, script and variant subtags have no ISO 639 equivalent and are discarded
    {"en-AU", "eng"},
    {"pt-BR", "por"},
    {"zh-Hant-HK", "chi"},
    {"zh-yue-Hant-HK", "chi"},
    // No ISO 639-2 equivalent exists, so the tag is returned unchanged
    {"zyg", "zyg"},
    {"und", "und"}, // undetermined round-trips
    {"", ""},
    {"EN", "eng"},
    {" en ", "eng"},
    {"English", "eng"}, // full English name
    {"not a language", "not a language"}, // unrecognized text is returned as it stands
};
// clang-format on

class Bcp47ToIso6392BConversionTester
  : public testing::Test,
    public testing::WithParamInterface<TestBcp47ToIso6392BConversion>
{
};

TEST_P(Bcp47ToIso6392BConversionTester, Convert)
{
  EXPECT_EQ(GetParam().iso6392B, CLangCodeExpander::AsISO6392B(GetParam().input));
}

INSTANTIATE_TEST_SUITE_P(TestLangCodeExpander,
                         Bcp47ToIso6392BConversionTester,
                         testing::ValuesIn(Bcp47ToIso6392BConversionTests));

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
    {"zh-yue-Hant-HK", true, "Chinese (Cantonese, Han (Tr... [zh-yue-Hant-HK]"},
};
// clang-format on

class LookupTester : public testing::Test, public testing::WithParamInterface<TestLookup>
{
};

TEST_P(LookupTester, Lookup)
{
  std::string output;

  EXPECT_EQ(GetParam().status, CLangCodeExpander::Lookup(GetParam().input, output));
  EXPECT_EQ(GetParam().expected, output);
}

INSTANTIATE_TEST_SUITE_P(TestLangCodeExpander, LookupTester, testing::ValuesIn(LookupTests));
