/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <locale>
#include "utils/UnicodeUtils.h"
#include "utils/UnicodeUtils.h"
#include "unicode/uloc.h"

#include <algorithm>

#include <gtest/gtest.h>
enum class ECG
{
  A, B
};

enum EG
{
  C, D
};

namespace test_enum
{
enum class ECN
{
  A = 1, B
};
enum EN
{
  C = 1, D
};
}

namespace TestUnicodeUtils
{
// MULTICODEPOINT_CHARS

// There are multiple ways to compose some graphemes. These are the same grapheme:

// Each of these five series of unicode codepoints represent the
// SAME grapheme (character)! To compare them correctly they should be
// normalized first. Normalization should reduce each sequence to the
// single codepoint (although some graphemes require more than one
// codepoint after normalization).
//
// TODO: It may be a good idea to normalize everything on
// input and renormalize when something requires it.
// A: U+006f (o) + U+0302 (◌̂) + U+0323 (◌̣): o◌̣◌̂
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_1[] =
{ 0x006f, 0x0302, 0x0323 };
// B: U+006f (o) + U+0323 (◌̣) + U+0302 (◌̂)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_2[] =
{ 0x006f, 0x0323, 0x302 };
// C: U+00f4 (ô) + U+0323 (◌̣)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_3[] =
{ 0x00f4, 0x0323 };
// D: U+1ecd (ọ) + U+0302 (◌̂)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_4[] =
{ 0x1ecd, 0x0302 };
// E: U+1ed9 (ộ)
const char32_t MULTI_CODEPOINT_CHAR_1_VARIENT_5[] =
{ 0x1ed9 };

// UTF8 versions of the above.

const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1[] =
{ "\x6f\xcc\x82\xcc\xa3\x00" };
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2[] =
{ "\x6f\xcc\xa3\xcc\x82\x00" };
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_3[] =
{ "\xc3\xb4\xcc\xa3\x00" };
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_4[] =
{ "\xe1\xbb\x8d\xcc\x82\x00" };
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5[] =
{ "\xe1\xbb\x99\x00" };

// 			u"óóßChloë" // German "Sharp-S" ß is (mostly) equivalent to ss (lower case).
//                     Lower case of two SS characters can either be ss or ß,
//                     depending upon context.
// óóßChloë
const char UTF8_GERMAN_SAMPLE[] =
{ "\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab" };
// u"ÓÓSSCHLOË";
const char* UTF8_GERMAN_UPPER =
{ "\xc3\x93\xc3\x93\x53\x53\x43\x48\x4c\x4f\xc3\x8b" };
// u"óósschloë"
const char UTF8_GERMAN_LOWER[] =
{ "\xc3\xb3\xc3\xb3\xc3\x9f\x63\x68\x6c\x6f\xc3\xab" };

// óóßchloë
const char* UTF8_GERMAN_LOWER_SS =
{ "\xc3\xb3\xc3\xb3\x73\x73\x63\x68\x6c\x6f\xc3\xab" };
// u"óósschloë";
}

/**
 * Sample long, multicodepoint emojiis
 *  the transgender flag emoji (🏳️‍⚧️), consists of the five-codepoint sequence"
 *   U+1F3F3 U+FE0F U+200D U+26A7 U+FE0F, requires sixteen bytes to encode,
 *   the flag of Scotland (🏴󠁧󠁢󠁳󠁣󠁴󠁿) requires a total of twenty-eight bytes for the
 *   seven-codepoint sequence U+1F3F4 U+E0067 U+E0062 U+E0073 U+E0063 U+E0074 U+E007F.
 */

static icu::Locale getCLocale()
{
  icu::Locale c_locale = Unicode::GetICULocale(std::locale::classic().name().c_str());
  return c_locale;
}
static icu::Locale getTurkicLocale()
{
  icu::Locale turkic_locale = Unicode::GetICULocale("tr", "TR");
  return turkic_locale;
}
static icu::Locale getUSEnglishLocale()
{
  icu::Locale us_english_locale = icu::Locale::getUS(); // Unicode::GetICULocale("en", "US");
  return us_english_locale;
}

static icu::Locale getUkranianLocale()
{
  icu::Locale ukranian_locale = Unicode::GetICULocale("uk", "UA");
  return ukranian_locale;
}

TEST(TestUnicodeUtils, ToUpper)
{

  std::string refstr = "TEST";

  std::string varstr = "TeSt";
  UnicodeUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  varstr = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  UnicodeUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë
}

TEST(TestUnicodeUtils, ToUpper_w)
{
  std::wstring refstr = L"TEST";
  std::wstring varstr = L"TeSt";

  UnicodeUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_UPPER)); // ÓÓSSCHLOË
  varstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE)); // óóßChloë
  UnicodeUtils::ToUpper(varstr);
  int32_t cmp = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(cmp, 0);
}

TEST(TestUnicodeUtils, ToUpper_Locale)
{
  std::string refstr = "TWITCH";
  std::string varstr = "Twitch";
  UnicodeUtils::ToUpper(varstr, getCLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "ABCÇDEFGĞH IİI JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh ıİi jklmnoöprsştuüvyz";
  UnicodeUtils::ToUpper(varstr, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // refstr = "ABCÇDEFGĞH IİI JKLMNOÖPRSŞTUÜVYZ";
  refstr = "ABCÇDEFGĞH Iİİ JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh ıİi jklmnoöprsştuüvyz";
  UnicodeUtils::ToUpper(varstr, getTurkicLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "ABCÇDEFGĞH IİI JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh ıİi jklmnoöprsştuüvyz";
  UnicodeUtils::ToUpper(varstr, getUkranianLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  varstr = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  UnicodeUtils::ToUpper(varstr, getCLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë
}

TEST(TestUnicodeUtils, ToLower)
{
  std::string refstr = "test";
  std::string varstr = "TeSt";

  UnicodeUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  refstr = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  UnicodeUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë

  // ToLower of string with (with sharp-s) should not change it.

  varstr = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  refstr = TestUnicodeUtils::UTF8_GERMAN_LOWER; // óóßChloë
  UnicodeUtils::ToLower(varstr);
  int32_t cmp = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(cmp, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, ToLower_w)
{
  std::wstring refstr = L"test";
  std::wstring varstr = L"TeSt";

  UnicodeUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str()); // Binary compare should work

  varstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_UPPER)); // ÓÓSSCHLOË
  refstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_LOWER_SS)); // óóßChloë
  UnicodeUtils::ToLower(varstr);
  int32_t cmp = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(cmp, 0);

  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
  // Lower: óósschloë

  // ToLower of string with (with sharp-s) should not change it.

  varstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE)); // óóßChloë
  refstr = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_GERMAN_LOWER)); // óóßchloë
  UnicodeUtils::ToLower(varstr);
  cmp = UnicodeUtils::Compare(refstr, varstr);
  EXPECT_EQ(cmp, 0);

  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, Turkic_I)
{

  // Specifically test behavior of Turkic I
  // US English
  // First, ToLower

  std::string originalInput = "I i İ ı";

  /*
   * For US English locale behavior of the 4 versions of "I" used in Turkey
   * ToLower yields:
   *
   * I (Dotless I \u0049)        -> i (Dotted small I)                 \u0069
   * i (Dotted small I \u0069)   -> i (Dotted small I)                 \u0069
   * İ (Dotted I) \u0130         -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * ı (Dotless small I) \u0131  -> ı (Dotless small I)                \u0131
   */
  std::string varstr = std::string(originalInput); // hex: 0049 0069 0130 0131
  std::string refstr = "iii̇ı";  // hex: 0069 0069 0069 0307 0131
  // Convert to native Unicode, UChar32
  std::string orig = std::string(varstr);
  std::wstring w_varstr_in = Unicode::UTF8ToWString(varstr);
  UnicodeUtils::ToLower(varstr, getUSEnglishLocale());
  std::wstring w_varstr_out = Unicode::UTF8ToWString(varstr);
  std::stringstream ss;
  std::string prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  std::string hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  std::string hexOutput = ss.str();
  ss.clear();
  ss.str(std::string());
  // std::cout << "US English ToLower input: " << orig << " output: " << varstr << " input hex: "
  //    << hexInput << " output hex: " << hexOutput << std::endl;

  /*
   * For US English locale behavior of the 4 versions of "I" used in Turkey
   * ToUpper yields:
   *
   * I (Dotless I)       \u0049 -> I (Dotless I) \u0049
   * i (Dotted small I)  \u0069 -> I (Dotless I) \u0049
   * İ (Dotted I)        \u0130 -> İ (Dotted I)  \u0130
   * ı (Dotless small I) \u0131 -> I (Dotless I) \u0049
   */

  varstr = std::string(originalInput);
  refstr = "IIİI";
  orig = std::string(varstr);
  w_varstr_in = Unicode::UTF8ToWString(varstr);
  UnicodeUtils::ToUpper(varstr, getUSEnglishLocale());
  w_varstr_out = Unicode::UTF8ToWString(varstr);
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexOutput = ss.str();
  ss.clear();
  ss.str(std::string());
  // std::cout << "US English ToUpper input: " << orig << " output: " << varstr << " input hex: "
  //    << hexInput << " output hex: " << hexOutput << std::endl;

  /*
   * For Turkish locale behavior of the 4 versions of "I" used in Turkey
   * ToLower yields:
   *
   * I (Dotless I)       \u0049 -> ı (Dotless small I) \u0131
   * i (Dotted small I)  \u0069 -> i (Dotted small I)  \u0069
   * İ (Dotted I)        \u0130 -> i (Dotted small I)  \u0069
   * ı (Dotless small I) \u0131 -> ı (Dotless small I) \u0131
   */

  varstr = std::string(originalInput);
  refstr = "ıiiı";
  // Convert to native Unicode, UChar32
  orig = std::string(varstr);
  w_varstr_in = Unicode::UTF8ToWString(varstr);
  UnicodeUtils::ToLower(varstr, getTurkicLocale());
  w_varstr_out = Unicode::UTF8ToWString(varstr);
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexOutput = ss.str();
  ss.clear();
  ss.str(std::string());
  // std::cout << "Turkic ToLower input: " << orig << " output: " << varstr << " input hex: "
  //    << hexInput << " output hex: " << hexOutput << std::endl;
  /*
   * For Turkish locale behavior of the 4 versions of "I" used in Turkey
   * ToUpper yields:
   *
   * I (Dotless I)       \u0049 -> I (Dotless I) \u0049
   * i (Dotted small I)  \u0069 -> İ (Dotted I)  \u0130
   * İ (Dotted I)        \u0130 -> İ (Dotted I)  \u0130
   * ı (Dotless small I) \u0131 -> I (Dotless I) \u0049
   */

  varstr = std::string(originalInput);
  refstr = "IİİI";
  orig = std::string(varstr);
  w_varstr_in = Unicode::UTF8ToWString(varstr);
  UnicodeUtils::ToUpper(varstr, getTurkicLocale());
  w_varstr_out = Unicode::UTF8ToWString(varstr);
  ss.clear();
  ss.str(std::string());
  prefix = "\\u";
  for (const auto& item : w_varstr_in)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexInput = ss.str();
  ss.clear();
  ss.str(std::string());
  prefix = "";
  for (const auto& item : w_varstr_out)
  {
    ss << prefix << std::hex << int(item);
    prefix = " \\u";
  }
  hexOutput = ss.str();
  // std::cout << "Turkic ToUpper input: " << orig << " output: " << varstr << " input hex: "
  //    << hexInput << " output hex: " << hexOutput << std::endl;
}

TEST(TestUnicodeUtils, ToCapitalize)
{
  std::string refstr = "Test";
  std::string varstr = "test";

  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Just A Test";
  varstr = "just a test";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test -1;2:3, String For Case";
  varstr = "test -1;2:3, string for Case";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "  JuST Another\t\tTEst:\nWoRKs ";
  varstr = "  juST another\t\ttEst:\nwoRKs ";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N.Y.P.D";
  varstr = "n.y.p.d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N-Y-P-D";
  varstr = "n-y-p-d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, ToCapitalize_w)
{
  std::wstring refstr = L"Test";
  std::wstring varstr = L"test";

  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"Just A Test";
  varstr = L"just a test";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"Test -1;2:3, String For Case";
  varstr = L"test -1;2:3, string for Case";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"  JuST Another\t\tTEst:\nWoRKs ";
  varstr = L"  juST another\t\ttEst:\nwoRKs ";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"N.Y.P.D";
  varstr = L"n.y.p.d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = L"N-Y-P-D";
  varstr = L"n-y-p-d";
  UnicodeUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, TitleCase)
{
  // Different from ToCapitalize (single word not title cased)

  std::string refstr = "Test";
  std::string varstr = "test";
  std::string result;

  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "Just A Test";
  varstr = "just a test";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "Test -1;2:3, String For Case";
  varstr = "test -1;2:3, string for Case";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "  Just Another\t\tTest:\nWorks ";
  varstr = "  juST another\t\ttEst:\nwoRKs ";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "N.y.p.d";
  varstr = "n.y.p.d";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "N-Y-P-D";
  varstr = "n-y-p-d";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());
}

TEST(TestUnicodeUtils, TitleCase_w)
{
  // Different from ToCapitalize (single word not title cased)

  std::wstring refstr = L"Test";
  std::wstring varstr = L"test";
  std::wstring result;

  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"Just A Test";
  varstr = L"just a test";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"Test -1;2:3, String For Case";
  varstr = L"test -1;2:3, string for Case";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"  Just Another\t\tTest:\nWorks ";
  varstr = L"  juST another\t\ttEst:\nwoRKs ";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"N.y.p.d";
  varstr = L"n.y.p.d";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = L"N-Y-P-D";
  varstr = L"n-y-p-d";
  result = UnicodeUtils::TitleCase(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());
}

TEST(TestUnicodeUtils, EqualsNoCase)
{
  std::string refstr = "TeSt";

  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, "tEsT"));
}

TEST(TestUnicodeUtils, EqualsNoCase_Normalize)
{
  const std::string refstr = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5;
  const std::string varstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  EXPECT_FALSE(UnicodeUtils::EqualsNoCase(refstr, varstr));
  EXPECT_TRUE(UnicodeUtils::EqualsNoCase(refstr, varstr, StringOptions::FOLD_CASE_DEFAULT, true));
}

TEST(TestUnicodeUtils, Compare)
{

  std::string left;
  std::string right;
  int result;
  int expectedResult;

  left = "abciI123ABC ";
  right = "ABCIi123abc ";
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::Compare(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::Compare(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::Compare(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::Compare(left, right), expectedResult);

}

TEST(TestUnicodeUtils, CompareNoCase) {

  std::string left;
  std::string right;
  int result;
  int expectedResult;

  left = "abciI123ABC ";
  right = "ABCIi123abc ";
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

}

TEST(TestUnicodeUtils, CompareNoCase_Advanced) {

  std::string left;
  std::string right;
  bool normalize;
  StringOptions opt = StringOptions::FOLD_CASE_DEFAULT;
  int length;
  int result;
  int expectedResult;

  normalize = false;
  left = "abciI123ABC ";
  right = "ABCIi123abc ";
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  length = 5;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  right = "ABCIi";
  length = 5;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  right = "ABCIi";
  length = 6;
  EXPECT_NE(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  right = "ABCIi";
  length = 4;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  // Make left shorter this time.

  normalize = false;
  left = "abciI123ABC ";
  right = "ABCIi123abc ";
  length = left.length();
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  length = 5;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  left = "abciI";
  length = 5;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  left = "abciI";
  length = 6;
  EXPECT_NE(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  left = "abciI";
  length = 4;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  // Easy tests

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  // More interesting guessing the byte length for multibyte characters, eh?
  // These are ALL foldcase equivalent. Curious that the byte lengths are same
  // but character counts are not.

  //
  // const char UTF8_GERMAN_SAMPLE[] =
  //      ó      ó       ß        C   h   l   o   ë
  // { "\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab" };
  //     1       2       3        4   5   6   7   8          character count
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count
  //
  // const char* UTF8_GERMAN_UPPER =
  //  u"  Ó       Ó       S   S   C   H   L   O  Ë";
  // { "\xc3\x93\xc3\x93\x53\x53\x43\x48\x4c\x4f\xc3\x8b" };
  //     1       2       3    4   5  6   7   8   9           character count
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count  //
  // const char UTF8_GERMAN_LOWER_SS =
  //     ó        ó       s   s   c   h   l   o   ë"
  // { "\xc3\xb3\xc3\xb3\x73\x73\x63\x68\x6c\x6f\xc3\xab" };
  //     1        2       3   4   5   6   7   8   9
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count
  // const char* UTF8_GERMAN_LOWER =
  //   u"ó       ó       ß       C   h   l   o   ë         // "ß" becomes "ss" during fold
  // { "\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab" };
  //     1       2       3       4    5  6   7   8           character count
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  length = 4;   // max bytes to compare, here 4 is last byte of second char for both strings
  // and for lower case version
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length), expectedResult);

  length = 3;   // max bytes to compare, length is at first byte of second char for
  // both strings and lower case version
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length), expectedResult);

  length = 6;   // max bytes to compare, of left's second S and right's last byte of
  // sharp-s "ß" and after folding would be the second s, if all other
  // characters were single byte, but it may still work out, I'm not sure
  // how the other characters fold.
  // RESUME

  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestUnicodeUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right), expectedResult);

  left = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;  // 6 bytes
  right = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5; // 4 bytes
  length = 4; // Last byte of 4 byte version and at 4th byte of 6 bytes.
  expectedResult = 0;
  EXPECT_NE(UnicodeUtils::CompareNoCase(left, right), expectedResult);


  // Boundary Tests
  // length of 0 means no limit

  normalize = false;
  left =  "abciI123ABC ";
  right = "ABCIi123abc ";
  length = 0;
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  normalize = false;
  left = "";
  right = "ABCIi123abc ";
  length = 0;   // No length limit
  expectedResult = -1;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  normalize = false;
  left = "abciI123ABC ";
  right = "";
  length = left.length();
  expectedResult = 1;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  normalize = false;
  left = "abciI123ABC ";
  right = "";
  length = 0; // No limit
  expectedResult = 1;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  normalize = false;
  left = "";
  right = "";
  length = 0;
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

  normalize = false;
  left = "";
  right = "";
  length = 32;
  expectedResult = 0;
  EXPECT_EQ(UnicodeUtils::CompareNoCase(left, right, length, opt, normalize), expectedResult);

}

TEST(TestUnicodeUtils, StartsWith)
{
  std::string refstr = "test";
  std::string input;
  char* p;

  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "x"));
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "Te"));
  EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, "test "));
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, "test\0")); // Embedded null terminates string

  p = "tes";
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, p));

  // Boundary

  input = "";
  EXPECT_TRUE(UnicodeUtils::StartsWith(input, ""));
  EXPECT_FALSE(UnicodeUtils::StartsWith(input, "Four score and seven years ago"));

  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, ""));
  // EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, nullptr)); // Blows up
  // EXPECT_FALSE(UnicodeUtils::StartsWith(refstr, NULL));    // Blows up

  input = "";
  p = "";
  EXPECT_TRUE(UnicodeUtils::StartsWith(input, p));
  EXPECT_TRUE(UnicodeUtils::StartsWith(refstr, p));

}

TEST(TestUnicodeUtils, StartsWithNoCase)
{
  std::string refstr = "test";
  std::string input;
  char * p;

  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, "x"));
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(refstr, "TesT"));
  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, "Te st"));
  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, "test "));
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(refstr, "test\0")); // Embedded null terminates string operation

  p = "tEs";
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(refstr, p));

  // Boundary

  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, ""));
  // EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, nullptr)); // Blows up
  // EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, NULL));    // Blows up

  p = "";
  // TODO: Verify Non-empty string  doesn't begin with empty string.
  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, p));
  // Same behavior with char * and string
  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, ""));

  input = "";
  // TODO: Empty string does begin with empty string
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(input, ""));

  // Blows up with null pointers

  // EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, nullptr));
  // EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(refstr, NULL));

  EXPECT_FALSE(UnicodeUtils::StartsWithNoCase(input, "Four score and seven years ago"));

  input = "";
  p = "";
  EXPECT_TRUE(UnicodeUtils::StartsWithNoCase(input, p));
}


TEST(TestUnicodeUtils, EndsWith)
{
  std::string refstr = "test";
  EXPECT_TRUE(UnicodeUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(UnicodeUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(UnicodeUtils::EndsWith(refstr, "sT"));
}

TEST(TestUnicodeUtils, EndsWithNoCase)
{
  // TODO: Test with FoldCase option

  std::string refstr = "test";
  EXPECT_FALSE(UnicodeUtils::EndsWithNoCase(refstr, "x"));
  EXPECT_TRUE(UnicodeUtils::EndsWithNoCase(refstr, "sT"));
  EXPECT_TRUE(UnicodeUtils::EndsWithNoCase(refstr, "TesT"));
}

TEST(TestUnicodeUtils, Left_Basic)
{
  std::string refstr;
  std::string varstr;
  std::string origstr = "Test";

  // First, request n chars to copy from left end

  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "T";
  varstr = UnicodeUtils::Left(origstr, 1);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Te";
  varstr = UnicodeUtils::Left(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Left(origstr, 4);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Left(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Left(origstr, std::string::npos);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // # of characters to omit from right end

  refstr = "Tes";
  varstr = UnicodeUtils::Left(origstr, 1, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Left(origstr, 0, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "T";
  varstr = UnicodeUtils::Left(origstr, 3, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 4, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 5, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Left(origstr, std::string::npos, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // TODO: Add test to ensure count works properly for multi-codepoint characters
}

TEST(TestUnicodeUtils, Left_Advanced)
{
  std::string origstr;
  std::string refstr;
  std::string varstr;

  // Multi-byte characters character count != byte count

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1); // 3 codepoints, 1 char
  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 0, getUSEnglishLocale(), true);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // Interesting case. All five VARIENTs can be normalized
  // to a single codepoint. We are NOT normalizing here.

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5); // 2 codepoints, 1 char
  varstr = UnicodeUtils::Left(origstr, 2, getUSEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  origstr = std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE); // u"óóßChloë"
  refstr = "óó";
  varstr = UnicodeUtils::Left(origstr, 2, getUSEnglishLocale(), true);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // Get leftmost substring removing n characters from end of string

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = std::string("");
  varstr = UnicodeUtils::Left(origstr, 1, getUSEnglishLocale(), false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // Interesting case. All five VARIENTs can be normalized
  // to a single codepoint. We are NOT normalizing here.

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = std::string("");
  varstr = UnicodeUtils::Left(origstr, 2, getUSEnglishLocale(), false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  origstr = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  refstr = "";
  varstr = UnicodeUtils::Left(origstr, 5, getUSEnglishLocale(), false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  origstr = std::string(TestUnicodeUtils::UTF8_GERMAN_SAMPLE); // u"óóßChloë"
  refstr = "óóßChl";
  varstr = UnicodeUtils::Left(origstr, 2, getUSEnglishLocale(), false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // TODO: Add test to ensure count works properly for multi-codepoint characters
}

TEST(TestUnicodeUtils, Mid)
{
  // TODO: Need more tests
  std::string refstr;
  std::string varstr;
  std::string origstr = "test";

  refstr = "";
  varstr = UnicodeUtils::Mid(origstr, 0, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "te";
  varstr = UnicodeUtils::Mid(origstr, 0, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test";
  varstr = UnicodeUtils::Mid(origstr, 0, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = UnicodeUtils::Mid(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = UnicodeUtils::Mid(origstr, 2, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "es";
  varstr = UnicodeUtils::Mid(origstr, 1, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "est";
  varstr = UnicodeUtils::Mid(origstr, 1, std::string::npos);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "est";
  varstr = UnicodeUtils::Mid(origstr, 1, 4);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Mid(origstr, 1, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "t";
  varstr = UnicodeUtils::Mid(origstr, 3, 1);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Mid(origstr, 4, 1);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, Right)
{
  // TODO: Create Right_Advanced test
  std::string refstr;
  std::string varstr;
  std::string origstr = "Test";

  // First, request n chars to copy from right end

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = UnicodeUtils::Right(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, 4);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, std::string::npos);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  // # of characters to omit from left end

  refstr = "est";
  varstr = UnicodeUtils::Right(origstr, 1, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test";
  varstr = UnicodeUtils::Right(origstr, 0, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "t";
  varstr = UnicodeUtils::Right(origstr, 3, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, 4, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, 5, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "";
  varstr = UnicodeUtils::Right(origstr, std::string::npos, false);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, GetCharPosition)
{
  std::string testString;
  size_t result;
  size_t expectedResult;
  size_t testStringLength;

  icu::Locale icuLocale = icu::Locale::getEnglish();

  /*
   * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
   * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
   * left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
   * left=false keepLeft=false  Returns offset of first byte of nth char from right end.
   *                            Character 0 is AFTER the last character.  Used by Right(x)
   */

  bool left = true;
  bool keepLeft = true;
  testString = "Hello";
  testStringLength = testString.length();
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 0;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 1;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 4, left, keepLeft, icuLocale);
  expectedResult = 4;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 5, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 6, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  //  left=true  getBeginIndex=false Returns offset of last byte of nth character from right end. Used by Left(x, false)

  left = true;
  keepLeft = false;

  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 4;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 4, left, keepLeft, icuLocale);
  expectedResult = 0;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 5, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 6, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true  Return offset of first byte of nth character

  left = false;
  keepLeft = true;

  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 0;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 4, left, keepLeft, icuLocale);
  expectedResult = 4;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 6, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  // left=false keepLeft=false  Returns offset of first byte of nth char from right end. Used by Right(x)

  // Hello
  left = false;
  keepLeft = false;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 4; // Unicode::AFTER_END;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3; //4;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, 4, left, keepLeft, icuLocale);
  expectedResult = 0;
  EXPECT_EQ(expectedResult, result);

  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);


  result = UnicodeUtils::GetCharPosition(testString, std::string::npos, left, keepLeft, icuLocale);
  expectedResult = Unicode::BEFORE_START;
  EXPECT_EQ(expectedResult, result);

  testString = "„Wiener Übereinkommen über den Straßenverkehr\"";
  //testString = UnicodeUtils::Normalize(testString,  StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  // At 0 \xe2\x80\x9e
  // Ü \xc3\x9c
  // ü \xc3\xbc
  // ß \xc3\x9f
  //  left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.

  icuLocale = icu::Locale::getGerman();
  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 2;  // Last byte of lower open double quote
  EXPECT_EQ(expectedResult, result);

  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3;  // W after quote
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 0; // First Byte of lower open double quote
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3; // First Byte of W
  EXPECT_EQ(expectedResult, result);

  // left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)

  left = true;
  keepLeft = false;
  result = UnicodeUtils::GetCharPosition(testString, 45, left, keepLeft, icuLocale);
  expectedResult = 2; // Last Byte of \xe2\x80\x9e
  EXPECT_EQ(expectedResult, result);

  // left=false keepLeft=false  Returns offset of first byte of nth char from right end (0-n). Used by Right(x)
  // Note that char 0 is BEYOND the last character, which is sad to say, different from char 0 from Left
  // end which is the first character.

  left = false;
  keepLeft = false;
  result = UnicodeUtils::GetCharPosition(testString, 45, left, keepLeft, icuLocale);
  expectedResult = 0; // First Byte first char
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of (n -1)th character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 8, left, keepLeft, icuLocale);
  expectedResult = 10;
  EXPECT_EQ(expectedResult, result);

  //  left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.

  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 7, left, keepLeft, icuLocale);
  expectedResult = 9;  // Byte BEFORE U-umlaut
  EXPECT_EQ(expectedResult, result);

  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 8, left, keepLeft, icuLocale);
  expectedResult = 11;  // Last Byte of U-umlaut
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 8, left, keepLeft, icuLocale);
  expectedResult = 10; // First Byte of U-umlaut
  EXPECT_EQ(expectedResult, result);

  // left=false keepLeft=false  Returns offset of first byte of nth char from right end (0-n). Used by Right(x)

  left = false;
  keepLeft = false;
  result = UnicodeUtils::GetCharPosition(testString, 10, left, keepLeft, icuLocale);
  expectedResult = 39;
  EXPECT_EQ(expectedResult, result);

  // left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n).
  // Used by Left(x, false)

  left = true;
  keepLeft = false;
  result = UnicodeUtils::GetCharPosition(testString, 10, left, keepLeft, icuLocale); // S-sharp \xc3\x9f
  expectedResult = 40;  // Last Byte of S-Sharp
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 35, left, keepLeft, icuLocale); // S-sharp \xc3\x9f
  expectedResult = 39;
  EXPECT_EQ(expectedResult, result);

  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 35, left, keepLeft, icuLocale); // S-sharp \xc3\x9f
  expectedResult = 40;
  EXPECT_EQ(expectedResult, result);


  // Test with consecutive multi-byte characters at beginning and end of string

  testString = "诺贝尔生理学于1901年首次颁发";
  // UTF-8, Some beginning and ending characters separated by space
  // e8afba e8b49d e5b094 e7949f e79086,e5ada6,e4ba8e,31,39,30,31,e5b9b4
  // e9a696 e6aca1 e9a281 e58f91
  // 诺 e8afba
  // 贝 e8b49d
  // 尔 e5b094
  // ...
  // 次 e6aca1
  // 颁 e9a281
  // 发 e58f91

  //  left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.

  icuLocale = icu::Locale::getChinese();
  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 2;  // Last byte of 诺 e8 af ba
  EXPECT_EQ(expectedResult, result);

  left = true;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 5;  // Last byte of 贝 e8 b4 9d
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of (n - 1)th character (0-n). Used by Right(x, false)
  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 0, left, keepLeft, icuLocale);
  expectedResult = 0; // First Byte of 诺 e8 af ba
  EXPECT_EQ(expectedResult, result);

  //  left=false keepLeft=true   Returns offset of first byte of (n - 1)th character (0-n). Used by Right(x, false)

  left = false;
  keepLeft = true;
  result = UnicodeUtils::GetCharPosition(testString, 1, left, keepLeft, icuLocale);
  expectedResult = 3; // First Byte of 贝 e8 b4 9d
  EXPECT_EQ(expectedResult, result);

}

TEST(TestUnicodeUtils, Trim)
{
  std::string refstr = "test test";

  std::string varstr = " test test   ";
  std::string result;

  result = UnicodeUtils::Trim(varstr);
  EXPECT_STREQ(result.c_str(), refstr.c_str());

  refstr = "";
  varstr = " \n\r\t   ";
  result = UnicodeUtils::Trim(varstr);
  EXPECT_STREQ(result.c_str(), refstr.c_str());

  refstr = "\nx\r\t   x";
  varstr = "$ \nx\r\t   x?\t";
  result = UnicodeUtils::Trim(varstr, "$? \t");
  EXPECT_STREQ(result.c_str(), refstr.c_str());

  refstr = "";
  varstr=" ";
  result = UnicodeUtils::Trim(varstr, " \t");
  EXPECT_STREQ(result.c_str(), refstr.c_str());
}

TEST(TestUnicodeUtils, TrimLeft)
{
  std::string refstr = "test test   ";

  std::string varstr = " test test   ";
  std::string result;

  result = UnicodeUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "";
  varstr = " \n\r\t   ";
  result = UnicodeUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "\nx\r\t   x?\t";
  varstr = "$ \nx\r\t   x?\t";
  result = UnicodeUtils::TrimLeft(varstr, "$? \t");
  EXPECT_STREQ(refstr.c_str(), result.c_str());
}

TEST(TestUnicodeUtils, TrimRight)
{
  std::string refstr = " test test";

  std::string varstr = " test test   ";
  std::string result;

  result = UnicodeUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "";
  varstr = " \n\r\t   ";
  result = UnicodeUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "$ \nx\r\t   x";
  varstr = "$ \nx\r\t   x?\t";
  result = UnicodeUtils::TrimRight(varstr, "$? \t");
  EXPECT_STREQ(refstr.c_str(), result.c_str());
}


TEST(TestUnicodeUtils, Trim_Multiple)
{
  std::string input;
  std::string trimmableChars;
  std::string result;
  std::string expectedResult;

  input = " Test Test   ";
  trimmableChars = " Tt";
  expectedResult = "est Tes";

  result = UnicodeUtils::Trim(input, trimmableChars.data());
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = " \n\r\t   ";
  trimmableChars = "\t \n \r";
  expectedResult = "";
  result = UnicodeUtils::Trim(input, trimmableChars.data());
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = "$ \nx\r\t  x?\t";
  trimmableChars = "$\n?";
  expectedResult = " \nx\r\t  x?\t";
  result = UnicodeUtils::Trim(input, trimmableChars.data());
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  // "„Wiener Übereinkommen über den Straßenverkehr\"";

  bool trimStart;
  bool trimEnd;

  input = "„ßÜ„Wiener Übereinkommen über den Straßenverkehrüß\"";
  trimmableChars = "„ßÜ\"";
  std::vector<std::string> trimmableStrs = {"„", "ß", "Ü", "\""};
  trimStart = true;
  trimEnd = true;
  expectedResult = "Wiener Übereinkommen über den Straßenverkehrü";
  result = Unicode::Trim(input, trimmableStrs, trimStart, trimEnd);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());
}


TEST(TestUnicodeUtils, Replace)
{
  std::string refstr = "text text";
  std::string varstr = "test test";

  EXPECT_EQ(UnicodeUtils::Replace(varstr, 's', 'x'), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(UnicodeUtils::Replace(varstr, 's', 'x'), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = "test test";
  EXPECT_EQ(UnicodeUtils::Replace(varstr, "s", "x"), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(UnicodeUtils::Replace(varstr, "s", "x"), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, FoldCase)
{
  /*
   *  FOLD_CASE_DEFAULT
   * I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
   * İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   * ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   *
   * FOLD_CASE_EXCLUDE_SPECIAL_I
   * I (Dotless I)       \u0049 -> ı (Dotless small I) \u0131
   * İ (Dotted I)        \u0130 -> i (Dotted small I)  \u0069
   * i (Dotted small I)  \u0069 -> i (Dotted small I)  \u0069
   * ı (Dotless small I) \u0131 -> ı (Dotless small I) \u0131
   */
  std::string s1 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  std::string s2 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);

  UnicodeUtils::FoldCase(s1);
  UnicodeUtils::FoldCase(s2);
  int32_t result = UnicodeUtils::Compare(s1, s2);
  EXPECT_NE(result, 0);

  s1 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  s2 = std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1);
  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // td::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_NE(result, 0);

  s1 = "I İ İ i ı";
  s2 = "i i̇ i̇ i ı";

  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_EQ(result, 0);

  std::string I = "I";
  std::string I_DOT = "İ";
  std::string I_DOT_I_DOT = "İİ";
  std::string i = "i";
  std::string ii = "ii";
  std::string i_DOTLESS = "ı";
  std::string i_DOTLESS_i_DOTTLESS = "ıı";
  std::string i_COMBINING_DOUBLE_DOT = "i̇";

  s1 = I + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s2 = i + i_COMBINING_DOUBLE_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;

  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_EQ(result, 0);
  /*
   FOLD_CASE_EXCLUDE_SPECIAL_I
   * I (\u0049) -> ı (\u0131)
   * i (\u0069) -> i (\u0069)
   * İ (\u0130) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   *
   */
  s1 = I + I_DOT + i + i_DOTLESS;
  s2 = i_DOTLESS + i + i + i_DOTLESS;

  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_EQ(result, 0);

  /*
   *  FOLD_CASE_DEFAULT
   * I (\u0049) -> i (\u0069)
   * İ (\u0130) -> i̇ (\u0069 \u0307)
   * i (\u0069) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   */
  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  // std::cout << "Turkic orig s1: " << s1 << std::endl;
  // std::cout << "Turkic orig s2: " << s2 << std::endl;

  UnicodeUtils::FoldCase(s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;

  result = UnicodeUtils::Compare(s1, s2);
  EXPECT_EQ(result, 0);
}

TEST(TestUnicodeUtils, FoldCase_W)
{
  std::wstring w_s1 = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  std::wstring w_s2 = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));

  UnicodeUtils::FoldCase(w_s1);
  UnicodeUtils::FoldCase(w_s2);
  int32_t result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_NE(result, 0);

  w_s1 = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  w_s2 = Unicode::UTF8ToWString(std::string(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // td::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_NE(result, 0);

  std::string s1 = "I İ İ i ı";
  std::string s2 = "i i̇ i̇ i ı";
  w_s1 = Unicode::UTF8ToWString(s1);
  w_s2 = Unicode::UTF8ToWString(s2);
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_EQ(result, 0);

  std::string I = "I";
  std::string I_DOT = "İ";
  std::string I_DOT_I_DOT = "İİ";
  std::string i = "i";
  std::string ii = "ii";
  std::string i_DOTLESS = "ı";
  std::string i_DOTLESS_i_DOTTLESS = "ıı";
  std::string i_COMBINING_DOUBLE_DOT = "i̇";

  s1 = I + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s2 = i + i_COMBINING_DOUBLE_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;

  w_s1 = Unicode::UTF8ToWString(s1);
  w_s2 = Unicode::UTF8ToWString(s2);
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);

  EXPECT_EQ(result, 0);
  /*
   FOLD_CASE_EXCLUDE_SPECIAL_I
   * I (\u0049) -> ı (\u0131)
   * i (\u0069) -> i (\u0069)
   * İ (\u0130) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   *
   */
  s1 = I + I_DOT + i + i_DOTLESS;
  s2 = i_DOTLESS + i + i + i_DOTLESS;

  w_s1 = Unicode::UTF8ToWString(s1);
  w_s2 = Unicode::UTF8ToWString(s2);
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_EXCLUDE_SPECIAL_I);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_EQ(result, 0);

  /*
   *  FOLD_CASE_DEFAULT
   * I (\u0049) -> i (\u0069)
   * İ (\u0130) -> i̇ (\u0069 \u0307)
   * i (\u0069) -> i (\u0069)
   * ı (\u0131) -> ı (\u0131)
   */
  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  // std::cout << "Turkic orig s1: " << s1 << std::endl;
  // std::cout << "Turkic orig s2: " << s2 << std::endl;

  w_s1 = Unicode::UTF8ToWString(s1);
  w_s2 = Unicode::UTF8ToWString(s2);
  UnicodeUtils::FoldCase(w_s1, StringOptions::FOLD_CASE_DEFAULT);
  UnicodeUtils::FoldCase(w_s2, StringOptions::FOLD_CASE_DEFAULT);
  // std::cout << "Turkic folded s1: " << s1 << std::endl;
  // std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = UnicodeUtils::Compare(w_s1, w_s2);
  EXPECT_EQ(result, 0);
}

TEST(TestUnicodeUtils, Normalize)
{
  // TODO: These tests are all on essentially the same caseless string.
  // The FOLD_CASE_DEFAULT option (which only impacts NFKD and maybe NFKC)
  // is not being tested, neither are the other alternatives to it.

  std::string s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  std::string result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFD);
  int cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFC);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFKC);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFD);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2);
  EXPECT_EQ(cmp, 0);

  s1 = TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1;
  result = UnicodeUtils::Normalize(s1, StringOptions::FOLD_CASE_DEFAULT, NormalizerType::NFKD);
  cmp = UnicodeUtils::Compare(result, TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2);
  EXPECT_EQ(cmp, 0);
}

TEST(TestUnicodeUtils, Split)
{
  std::vector<std::string> varresults;

  // test overload with string as delimiter
  varresults = UnicodeUtils::Split("g,h,ij,k,lm,,n", ",");
  EXPECT_STREQ("g", varresults.at(0).c_str());
  EXPECT_STREQ("h", varresults.at(1).c_str());
  EXPECT_STREQ("ij", varresults.at(2).c_str());
  EXPECT_STREQ("k", varresults.at(3).c_str());
  EXPECT_STREQ("lm", varresults.at(4).c_str());
  EXPECT_STREQ("", varresults.at(5).c_str());
  EXPECT_STREQ("n", varresults.at(6).c_str());

  varresults = UnicodeUtils::Split(",a,b,cd,", ",");
  EXPECT_STREQ("", varresults.at(0).c_str());
  EXPECT_STREQ("a", varresults.at(1).c_str());
  EXPECT_STREQ("b", varresults.at(2).c_str());
  EXPECT_STREQ("cd", varresults.at(3).c_str());
  EXPECT_STREQ("", varresults.at(4).c_str());

  std::vector<std::string> expectedResult;
  varresults = UnicodeUtils::Split(",a,,aa,,", ",");
  expectedResult =
  { "", "a", "", "aa", "", "" };
  size_t idx = 0;
  EXPECT_EQ(expectedResult.size(), varresults.size());

  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }

  varresults = UnicodeUtils::Split(",a,,,aa,,,", ",");
  expectedResult =
  { "", "a", "", "", "aa", "", "", "" };
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }

  varresults = UnicodeUtils::Split(",-+a,,--++aa,,-+",
      { ",", "-", "+" });
  expectedResult =
  { "", "", "", "a", "", "", "", "", "", "aa", "", "", "", "" };
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }
  varresults = UnicodeUtils::Split(",-+a,,--++aa,,-+",
      { "-", "+", "," });
  expectedResult =
  { "", "", "", "a", "", "", "", "", "", "aa", "", "", "", "" };
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }
  varresults = UnicodeUtils::Split(",-+a,,--++aa,,-+",
      { "+", ",", "-" });
  expectedResult =
  { "", "", "", "a", "", "", "", "", "", "aa", "", "", "", "" };
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }
  // This result looks incorrect, but verified against Matrix 19.4 behavior
  varresults = UnicodeUtils::Split("a,,,a,,,,,,aa,,,a",
      { ",", "-", "+" });
  expectedResult =
  { "a", "", "", "a", "", "", "", "", "", "aa", "", "", "a" };
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }

  varresults = UnicodeUtils::Split("a,-+a,,--++aa,-+a",
      { ",", "-", "+" });
  expectedResult =
  { "a", "", "", "a", "", "", "", "", "", "aa", "", "", "a" };
  EXPECT_EQ(expectedResult.size(), varresults.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx >= 0)
    {
      EXPECT_STREQ(i.c_str(), varresults.at(idx).c_str());
      idx++;
    }
  }

  EXPECT_TRUE(UnicodeUtils::Split("", "|").empty());

  EXPECT_EQ(4U, UnicodeUtils::Split("a bc  d ef ghi ", " ", 4).size());
  EXPECT_STREQ("d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", " ", 4).at(3).c_str())
  << "Last part must include rest of the input string";
  EXPECT_EQ(7U, UnicodeUtils::Split("a bc  d ef ghi ", " ").size())
  << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", UnicodeUtils::Split("a bc  d ef ghi ", " ").at(1).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", " ").at(2).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", " ").at(6).c_str());

  EXPECT_EQ(2U, UnicodeUtils::Split("a bc  d ef ghi ", "  ").size());
  EXPECT_EQ(2U, UnicodeUtils::Split("a bc  d ef ghi ", "  ", 10).size());
  EXPECT_STREQ("a bc", UnicodeUtils::Split("a bc  d ef ghi ", "  ", 10).at(0).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", " z").size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", " z").at(0).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", "").at(0).c_str());

  // test overload with char as delimiter
  EXPECT_EQ(4U, UnicodeUtils::Split("a bc  d ef ghi ", ' ', 4).size());
  EXPECT_STREQ("d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", ' ', 4).at(3).c_str());
  EXPECT_EQ(7U, UnicodeUtils::Split("a bc  d ef ghi ", ' ').size())
  << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", UnicodeUtils::Split("a bc  d ef ghi ", ' ').at(1).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", ' ').at(2).c_str());
  EXPECT_STREQ("", UnicodeUtils::Split("a bc  d ef ghi ", ' ').at(6).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", 'z').size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());

  EXPECT_EQ(1U, UnicodeUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", UnicodeUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());

  std::string input;
  std::vector<std::string> delimiters;
  std::vector<std::string> result;
  input = "a/b#c/d/e/foo/g::h/"; // "#p/q/r:s/x&extraNarfy"};
  delimiters = {"/", "#", ":", "Narf"};
  expectedResult = {"a", "b", "c", "d", "e", "foo", "g", "", "h", ""}; // , "", "p", "q", "r", "s",
  // "x&extra", "y"};
  result.clear();
  Unicode::SplitTo(std::back_inserter(result), input, delimiters, 0);

  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }

  input = {"a/b#c/d/e/foo/g::h/", "#p/q/r:s/x&extraNarfy"};
  delimiters = {"/", "#", ":", "Narf"};
  expectedResult = {"a", "b#c", "d", "e", "foo", "/g::h/", "p", "q", "/r:s/x&extraNarfy"};
}

static void compareStrings(std::vector<std::string> result, std::vector<std::string> expected)
{
  EXPECT_EQ(expected.size(), result.size());
  size_t idx = 0;
  for (auto i : expected)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }
}

TEST(TestUnicodeUtils, SplitMulti)
{
  /*
   * SplitMulti is essentially equivalent to running Split(string, vector<delimiters>, maxstrings) over multiple
   * strings with the same delimiters and returning the aggregate results. Null strings are not returned.
   *
   * There are some significant differences when maxstrings alters the process. Here are the boring details:
   *
   * For each delimiter, Split(string<n> input, delimiter, maxstrings) is called for each input string.
   * The results are appended to results <vector<string>>
   *
   * After a delimiter has been applied to all input strings, the process is repeated with the
   * next delimiter, but this time with the vector<string> input being replaced with the
   * results of the previous pass.
   *
   * If the maxstrings limit is not reached, then, as stated above, the results are similar to
   *  running Split(string, vector<delimiters> maxstrings) over multiple strings. But when the limit is reached
   *  differences emerge.
   *
   *  Before a delimiter is applied to a string a check is made to see if maxstrings is exceeded. If so,
   *  then splitting stops and all split string results are returned, including any strings that have not
   *  been split by as many delimiters as others, leaving the delimiters in the results.
   *
   *  Differences between current behavior and prior versions: Earlier versions removed most empty strings,
   *  but a few slipped past. Now, all empty strings are removed. This means not as many empty strings
   *  will count against the maxstrings limit. This change should cause no harm since there is no reliable
   *  way to correlate a result with an input; they all get thrown in together.
   *
   * \param input vector of strings to be split
   * \param delimiters strings to be used to split the input strings
   * \param iMaxStrings (optional) Maximum number of resulting split strings
   *   *
   * static std::vector<std::string> SplitMulti(const std::vector<std::string>& input,
   *                                           const std::vector<std::string>& delimiters,
   *                                           size_t iMaxStrings = 0);
   */
  size_t idx;
  std::vector<std::string> expectedResult;
  std::vector<std::string> input;
  std::vector<std::string> delimiters;
  std::vector<std::string> result;

  input.push_back(",h,ij,k,lm,,n,");
  delimiters.push_back(",");
  result = UnicodeUtils::SplitMulti(input, delimiters);
  // Legacy (Matrix 19.4) behavior (strips most, but not all null strings)
  // EXPECT_STREQ("", result.at(0).c_str());
  // EXPECT_STREQ("h", result.at(1).c_str());
  // EXPECT_STREQ("ij", result.at(2).c_str());
  // EXPECT_STREQ("k", result.at(3).c_str());
  // EXPECT_STREQ("lm", result.at(4).c_str());
  // EXPECT_STREQ("", result.at(5).c_str());
  // EXPECT_STREQ("n", result.at(6).c_str());
  // EXPECT_STREQ("", result.at(7).c_str());

  expectedResult = {"h", "ij", "k", "lm", "n"};
  compareStrings(result, expectedResult);

  input.clear();
  delimiters.clear();
  input = {"abcde", "Where is the beef?", "cbcefa"};
  delimiters = {"a", "bc", "ef", "c"};
  result = UnicodeUtils::SplitMulti(input, delimiters);
  expectedResult = {"de", "Where is the be", "?"};
  result = UnicodeUtils::SplitMulti(input, delimiters);
  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }

  // These two tests verify the example in UnicodeUtils documentation for
  // SplitMulti.

  input = {"a/b#c/d/e/foo/g::h/", "#p/q/r:s/x&extraNarfy"};
  delimiters = {"/", "#", ":", "Narf"};

  // Legacy (Matrix 19.4) result does not remove all null strings
  // expectedResult = {"a", "b", "c", "d", "e", "foo", "g", "", "h", "", "", "p", "q", "r", "s",
  //                  "x&extra", "y"};
  expectedResult = {"a", "b", "c", "d", "e", "foo", "g", "h", "p", "q", "r", "s",
      "x&extra", "y"};
  result = UnicodeUtils::SplitMulti(input, delimiters, 0);
  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }

  input = {"a/b#c/d/e/foo/g::h/", "#p/q/r:s/x&extraNarfy"};
  delimiters = {"/", "#", ":", "Narf"};
  expectedResult = {"a", "b#c", "d", "e", "foo", "g::h/", "#p/q/r:s/x&extraNarfy"};
  result = UnicodeUtils::SplitMulti(input, delimiters, 7);
  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }

  input = {" a,b-e e-f,c d-", "-sworn enemy, is not here"};
  delimiters = {",", " ", "-"};
  // The following has a minor bug and is present in Matrix 19.4. Should not return null strings
  // expectedResult =  {"a", "b", "e", "e", "f", "c", "d", "", "", "sworn", "enemy", "is", "not", "here"};
  expectedResult =  {"a", "b", "e", "e", "f", "c", "d", "sworn", "enemy", "is", "not", "here"};

  result = UnicodeUtils::SplitMulti(input, delimiters);
  EXPECT_EQ(expectedResult.size(), result.size());
  idx = 0;
  for (auto i : expectedResult)
  {
    if (idx < result.size())
    {
      EXPECT_STREQ(i.c_str(), result.at(idx).c_str());
      idx++;
    }
  }
}

TEST(TestUnicodeUtils, FindNumber)
{
  EXPECT_EQ(3, UnicodeUtils::FindNumber("aabcaadeaa", "aa"));
  EXPECT_EQ(1, UnicodeUtils::FindNumber("aabcaadeaa", "b"));
}

TEST(TestUnicodeUtils, Collate)
{
  int32_t ref = 0;
  int32_t var;
  EXPECT_TRUE(Unicode::InitializeCollator(getUSEnglishLocale(), false));

  const std::wstring s1 = std::wstring(L"The Farmer's Daughter");
  const std::wstring s2 = std::wstring(L"Ate Pie");
  var = UnicodeUtils::Collate(s1, s2);
  EXPECT_GT(var, ref);

  EXPECT_TRUE(Unicode::InitializeCollator(getTurkicLocale(), true));
  const std::wstring s3 = std::wstring(
      Unicode::UTF8ToWString(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  const std::wstring s4 = std::wstring(
      Unicode::UTF8ToWString(TestUnicodeUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));
  var = UnicodeUtils::Collate(s3, s4);
  EXPECT_EQ(var, 0);

  EXPECT_TRUE(Unicode::InitializeCollator(getTurkicLocale(), false));
  var = UnicodeUtils::Collate(s3, s4); // No (extra) Normalization
  EXPECT_NE(var, 0);
}
TEST(TestUnicodeUtils, AlphaNumericCompare)
{
  int64_t ref;
  int64_t var;

  ref = 0;
  var = UnicodeUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
}
TEST(TestUnicodeUtils, TimeStringToSeconds)
{
  EXPECT_EQ(77455, UnicodeUtils::TimeStringToSeconds("21:30:55"));
  EXPECT_EQ(7 * 60, UnicodeUtils::TimeStringToSeconds("7 min"));
  EXPECT_EQ(7 * 60, UnicodeUtils::TimeStringToSeconds("7 min\t"));
  EXPECT_EQ(154 * 60, UnicodeUtils::TimeStringToSeconds("   154 min"));
  EXPECT_EQ(1 * 60 + 1, UnicodeUtils::TimeStringToSeconds("1:01"));
  EXPECT_EQ(4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("4:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("2:04:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("   2:4:3"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, UnicodeUtils::TimeStringToSeconds("  \t\t 02:04:03 \n "));
  EXPECT_EQ(1 * 3600 + 5 * 60 + 2, UnicodeUtils::TimeStringToSeconds("01:05:02:04:03 \n "));
  EXPECT_EQ(0, UnicodeUtils::TimeStringToSeconds("blah"));
  EXPECT_EQ(0, UnicodeUtils::TimeStringToSeconds("ля-ля"));
}

TEST(TestUnicodeUtils, RemoveCRLF)
{
  std::string refstr;
  std::string varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  UnicodeUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestUnicodeUtils, FindWord)
{
  bool ref;
  bool var;

  ref = true;
  var = UnicodeUtils::FindWord("test string", "string");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("12345string", "string");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("apple2012", "2012");
  EXPECT_EQ(ref, var);

  ref = false;
  var = UnicodeUtils::FindWord("12345string", "ring");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("12345string", "345");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("apple2012", "e2012");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("apple2012", "12");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("anyt]h(z_iILi234#!? 1a34#56bbc7 ", "1a34#56bbc7");
  ref = true;
  EXPECT_EQ(ref, var);
}

TEST(TestUnicodeUtils, FindWord_NonAscii)
{
  bool ref;
  bool var;

  ref = true;
  var = UnicodeUtils::FindWord("我的视频", "视频");
  EXPECT_EQ(ref, var);
  var = UnicodeUtils::FindWord("我的视频", "视");
  EXPECT_EQ(ref, var);
  ref = true;
  var = UnicodeUtils::FindWord("Apple ple", "ple");
  EXPECT_EQ(ref, var);
  ref = false;
  var = UnicodeUtils::FindWord("Apple ple", "le");
  EXPECT_EQ(ref, var);

  ref = true;
  var = UnicodeUtils::FindWord(" Apple ple", " Apple");
  EXPECT_EQ(ref, var);

  ref = true;
  var = UnicodeUtils::FindWord(" Apple ple", "Apple");
  EXPECT_EQ(ref, var);

  ref = true;
  var = UnicodeUtils::FindWord("Äpfel.pfel", "pfel");
  EXPECT_EQ(ref, var);

  ref = false;
  var = UnicodeUtils::FindWord("Äpfel.pfel", "pfeldumpling");
  EXPECT_EQ(ref, var);

  ref = true;
  var = UnicodeUtils::FindWord("Äpfel.pfel", "Äpfel.pfel");
  EXPECT_EQ(ref, var);

  // Yea old Turkic I problem....
  ref = true;
  var = UnicodeUtils::FindWord("abcçdefgğh ıİi jklmnoöprsştuüvyz", "ıiİ jklmnoöprsştuüvyz");
}

TEST(TestUnicodeUtils, DateStringToYYYYMMDD)
{
  // Must accept: YYYY, YYYY-MM, YYYY-MM-DD
  int ref;
  int var;

  ref = 20120706;
  var = UnicodeUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);

  ref = 201207;
  var = UnicodeUtils::DateStringToYYYYMMDD("2012-07");
  EXPECT_EQ(ref, var);

  ref = 2012;
  var = UnicodeUtils::DateStringToYYYYMMDD("2012");
  EXPECT_EQ(ref, var);
}

TEST(TestUnicodeUtils, sortstringbyname)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("B");
  strarray.emplace_back("c");
  strarray.emplace_back("a");
  std::sort(strarray.begin(), strarray.end(), sortstringbyname());

  EXPECT_STREQ("a", strarray[0].c_str());
  EXPECT_STREQ("B", strarray[1].c_str());
  EXPECT_STREQ("c", strarray[2].c_str());
}

TEST(TestUnicodeUtils, Paramify)
{
  std::string input;
  std::string expectedResult;
  std::string result;

  /*
   * std::string UnicodeUtils::Paramify(const std::string &param) {
   //std::cout << "UnicodeUtils.Paramify param: " << param << std::endl;
  // escape backspaces
  std::string result = Unicode::FindAndReplace(param,  "\\", "\\\\");

  // escape double quotes
  result = Unicode::FindAndReplace(result, "\"", "\\\"");

  // add double quotes around the whole string
  return "\"" + result + "\"";
   */

  input = "Vanilla string";
  expectedResult = R"("Vanilla string")";
  result = UnicodeUtils::Paramify(input);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = "\\";
  expectedResult = R"("\\")";
  result = UnicodeUtils::Paramify(input);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = R"(")";
  expectedResult = R"("\"")";
  result = UnicodeUtils::Paramify(input);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  input = R"("""Three quotes \\\ Three slashes "\"\\""\\\""")";
  expectedResult = R"("\"\"\"Three quotes \\\\\\ Three slashes \"\\\"\\\\\"\"\\\\\\\"\"\"")";
  result = UnicodeUtils::Paramify(input);
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());
}
