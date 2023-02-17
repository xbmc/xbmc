// clang-format off
/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/test/OrigStringUtils.h"

#include <algorithm>
#include <cstdint>
#include <locale>
#include <string_view>

#include <gtest/gtest.h>

using namespace std::literals;

// true if C++ lib tolower/toupper used
// false if casemapping tables from icuc4 used
bool constexpr USE_BUILTIN_CASEMAPPING{false};

enum class ECG
{
  A,
  B
};

enum EG
{
  C,
  D
};

namespace test_enum
{
enum class ECN
{
  A = 1,
  B
};
enum EN
{
  C = 1,
  D
};
} // namespace test_enum

// ------------------------------------ NOTE ------------------------------------
//
// The ToLower, ToUpper, FoldCase and related tests were rewritten for Unicode (codepoint)
// comparison and not for single-byte comparison (as was done prior to these
// changes). The ToLower/ToUpper API has not yet been changed, so these tests have
// been modified to pass with current behavior. A TODO note documents each of these.
//
// Also note that on Windows, ToLower and ToUpper will use UTF-16 instead of UTF-32.
// Additional testing will need to be done to correct for that difference.
//
// -----------------------------------------------------------------------------
namespace TestStringUtils
{
//
// These represent the SAME Unicode character. Certain operations can
// change the number of code-units or even codepoints (Unicode 32-bit chars)
// required to represent what is commonly considered a character.  Normalization
// (which C++ does not have) can reorder and change the length of such
// characters.
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1[] = {"\x6f\xcc\x82\xcc\xa3\x00"};
// const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_2[] = {"\x6f\xcc\xa3\xcc\x82\x00"};
// const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_3[] = {"\xc3\xb4\xcc\xa3\x00"};
// const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_4[] = {"\xe1\xbb\x8d\xcc\x82\x00"};
const char UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5[] = {"\xe1\xbb\x99\x00"};

//      u"óóßChloë" // German "Sharp-S" ß is (mostly) equivalent to ss (lower case).
//                     Lower case of two SS characters can either be ss or ß,
//                     depending upon context, but beyond this implementation's
//                     capabilities.
// óóßChloë
const char UTF8_GERMAN_SAMPLE[] = {"\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab"};
// u"ÓÓSSCHLOË";
const char* UTF8_GERMAN_UPPER = {"\xc3\x93\xc3\x93\x53\x53\x43\x48\x4c\x4f\xc3\x8b"};
// u"óósschloë"
//const char UTF8_GERMAN_LOWER[] = {"\xc3\xb3\xc3\xb3\xc3\x9f\x63\x68\x6c\x6f\xc3\xab"};
// óóßchloë
const char* UTF8_GERMAN_LOWER_SS = {"\xc3\xb3\xc3\xb3\x73\x73\x63\x68\x6c\x6f\xc3\xab"};
// u"óósschloë";
} // namespace TestStringUtils

TEST(TestStringUtils, Format)
{
  std::string refstr = "test 25 2.7 ff FF";

  std::string varstr =
      OrigStringUtils::Format("{} {} {:.1f} {:x} {:02X}", "test", 25, 2.743f, 0x00ff, 0x00ff);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = OrigStringUtils::Format("", "test", 25, 2.743f, 0x00ff, 0x00ff);
  EXPECT_STREQ("", varstr.c_str());
}

TEST(TestStringUtils, FormatEnum)
{
  const char* zero = "0";
  const char* one = "1";

  std::string varstr = OrigStringUtils::Format("{}", ECG::A);
  EXPECT_STREQ(zero, varstr.c_str());

  varstr = OrigStringUtils::Format("{}", EG::C);
  EXPECT_STREQ(zero, varstr.c_str());

  varstr = OrigStringUtils::Format("{}", test_enum::ECN::A);
  EXPECT_STREQ(one, varstr.c_str());

  varstr = OrigStringUtils::Format("{}", test_enum::EN::C);
  EXPECT_STREQ(one, varstr.c_str());
}

TEST(TestStringUtils, FormatEnumWidth)
{
  const char* one = "01";

  std::string varstr = OrigStringUtils::Format("{:02d}", ECG::B);
  EXPECT_STREQ(one, varstr.c_str());

  varstr = OrigStringUtils::Format("{:02}", EG::D);
  EXPECT_STREQ(one, varstr.c_str());
}

TEST(TestStringUtils, ToUpper)
{
  std::string refstr = "TEST";

  std::string varstr = "TeSt";
  OrigStringUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, ToLower)
{
  std::string refstr = "test";

  std::string varstr = "TeSt";
  OrigStringUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

/*
TEST(TestStringUtils, ToUpper_Locale)
{
  // Note that the results for the Turkic I depends upon the tolower/toupper
  // implementation. These results are for G++ on Linux.
  //
  std::string result;
  std::string refstr = "TWITCH";
  std::string varstr = "Twitch";
  result = OrigStringUtils::ToUpper(varstr, OrigStringUtils::GetCLocale());
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  refstr = "ABCÇDEFGĞH IİII JKLMNOÖPRSŞTUÜVYZ";
  varstr = "abcçdefgğh Iİiı jklmnoöprsştuüvyz";
  result = OrigStringUtils::ToUpper(varstr, OrigStringUtils::GetEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  if (USE_BUILTIN_CASEMAPPING)
    refstr = "ABCÇDEFGĞH IİİI JKLMNOÖPRSŞTUÜVYZ";
  else
    refstr = "ABCÇDEFGĞH IİII JKLMNOÖPRSŞTUÜVYZ";

  // Not all test systems have Turkish installed
  // Test is NOT critical since Locale should not
  // impact these casmapping tests (unless
  // USE_BUILTIN_CASEMAPPING is true AND they
  // impact this result, but they don't.
  try
  {
    std::locale turkic = std::locale("tr_TR.UTF-8");
    result = OrigStringUtils::ToUpper(varstr, turkic);
    EXPECT_STREQ(refstr.c_str(), result.c_str());
  }
  catch (std::runtime_error& e)
  {
    std::cout << "Locale tr_TR.UTF-8 not supported" << std::endl;
  }

  // Not all test systems have Ukranian installed
  try
  {
    refstr = "ABCÇDEFGĞH IİII JKLMNOÖPRSŞTUÜVYZ";
    std::locale ukranian = std::locale("uk_UA.UTF-8");
    result = OrigStringUtils::ToUpper(varstr, ukranian);
    EXPECT_STREQ(refstr.c_str(), result.c_str());
  }
  catch (std::runtime_error& e)
  {
    std::cout << "Locale uk_UA.UTF-8 not supported" << std::endl;
  }
}
*/

/*
TEST(TestStringUtils, ToLower_Locale)
{
  // Note that the results for the Turkic I depends upon the tolower/toupper
  // implementation. These results are for G++ on Linux.
  //
  std::string result;
  std::string refstr = "twitch";
  std::string varstr = "Twitch";
  result = OrigStringUtils::ToLower(varstr, OrigStringUtils::GetCLocale());
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  varstr = "ABCÇDEFGĞH Iİiı JKLMNOÖPRSŞTUÜVYZ";
  refstr = "abcçdefgğh iiiı jklmnoöprsştuüvyz";
  result = OrigStringUtils::ToLower(varstr, OrigStringUtils::GetEnglishLocale());
  EXPECT_STREQ(refstr.c_str(), result.c_str());

  if (USE_BUILTIN_CASEMAPPING)
    refstr = "abcçdefgğh ıiiı jklmnoöprsştuüvyz";
  else
    refstr = "abcçdefgğh iiiı jklmnoöprsştuüvyz";

  try
  {
    std::locale turkic = std::locale("tr_TR.UTF-8");
    result = OrigStringUtils::ToLower(varstr, turkic);
    EXPECT_STREQ(refstr.c_str(), result.c_str());
  }
  catch (std::runtime_error& e)
  {
    std::cout << "Locale tr_TR.UTF-8 not supported" << std::endl;
  }
  try
  {
    refstr = "abcçdefgğh iiiı jklmnoöprsştuüvyz";
    std::locale ukranian = std::locale("uk_UA.UTF-8");
    result = OrigStringUtils::ToLower(varstr, ukranian);
    EXPECT_STREQ(refstr.c_str(), result.c_str());
  }
  catch (std::runtime_error& e)
  {
    std::cout << "Locale uk_UA.UTF-8 not supported" << std::endl;
  }
}
*/

TEST(TestStringUtils, ToCapitalize)
{
  std::string refstr = "Test";
  std::string varstr = "test";
  OrigStringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Just A Test";
  varstr = "just a test";
  OrigStringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test -1;2:3, String For Case";
  varstr = "test -1;2:3, string for Case";
  OrigStringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "  JuST Another\t\tTEst:\nWoRKs ";
  varstr = "  juST another\t\ttEst:\nwoRKs ";
  OrigStringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N.Y.P.D";
  varstr = "n.y.p.d";
  OrigStringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N-Y-P-D";
  varstr = "n-y-p-d";
  OrigStringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

/*
// Two large tables, at bottom of file.
extern const char32_t UnicodeFoldLowerTable[];
extern const char32_t UnicodeFoldUpperTable[];
extern size_t GetFoldingTableSize();

TEST(TestStringUtils, Validate_FoldCase_Data)
{
  // Validate that FoldCase tables were generated properly.
  //
  // Verify that:
  //
  // For every Upper Case character defined in UnicodeFoldUpperTable, Verify that
  // FoldCase(UnicodeFoldUpperTable[i]) == UnicodeFoldLowerTable[i].
  // This will confirm that the tables driving FoldCase were properly
  // constructed from UnicodeFoldUpperTable & UnicodeFoldLowerTable.

  size_t maxIdx = GetFoldingTableSize();

  bool passed = true;

  for (size_t i = 0; i < maxIdx; i++)
  {
    char32_t upper_char32 = UnicodeFoldUpperTable[i];
    std::u32string upper_u32(1, upper_char32); // 0
    std::string upper_utf8 = OrigStringUtils::ToUtf8(upper_u32);
    std::string folded = OrigStringUtils::FoldCase(upper_utf8);

    std::u32string folded_utf32 = OrigStringUtils::ToUtf32(folded);
    char32_t folded_char32 = folded_utf32[0];
    char32_t lower_char32 = UnicodeFoldLowerTable[i];

    if (folded_char32 != lower_char32)
    {
      passed = false;
      FAIL() << "Something wrong about conversion tables. Could not fold to expected character"
             << std::endl
             << "upper_char32: " << upper_char32 << " lower_char32: " << lower_char32 << " i: " << i
             << " upper_utf8: " << upper_utf8 << " hex: " << std::hex << upper_char32
             << " folded: " << folded << " hex: " << std::hex << folded_char32
             << " folded.length: " << folded_utf32.length() << std::endl;
    }
  }
  EXPECT_TRUE(passed);
}

TEST(TestStringUtils, BadUnicode)
{
  // Verify that code doesn't crater on malformed/bad Unicode. Is NOT an exhaustive
  // list.

  // Unassigned, private use or surrogates
  // Most do not trigger an error in wstring_convert (the Bad_UTF8 ones do).

  // For more info on substitute characters, see:
  // UnicodeConverter or TestUnicodeConverter for more info.
  //
  std::string_view UTF8_SUBSTITUTE_CHARACTER{"\xef\xbf\xbd"sv};
  // std::u16string_view UTF16_SUBSTITUTE_CHARACTER{u"\x0fffd"sv};
  // std::u16string_view UTF16_LE_SUBSTITUTE_CHARACTER{u"�"sv};
  std::u32string_view CHAR32_T_SUBSTITUTE_CHARACTER{U"\x0fffd"sv}; //U'�'sv};
  static const char32_t BAD_CHARS[]{
      U'\xFDD0',   U'\xFDEF',  U'\xFFFE',  U'\xFFFF',  U'\x1FFFE', U'\x1FFFF', U'\x2FFFE',
      U'\x2FFFF',  U'\x3FFFE', U'\x3FFFF', U'\x4FFFE', U'\x4FFFF', U'\x5FFFE', U'\x5FFFF',
      U'\x6FFFE',  U'\x6FFFF', U'\x7FFFE', U'\x7FFFF', U'\x8FFFE', U'\x8FFFF', U'\x9FFFE',
      U'\x9FFFF',  U'\xAFFFE', U'\xAFFFF', U'\xBFFFE', U'\xBFFFF', U'\xCFFFE', U'\xCFFFF',
      U'\xDFFFE',  U'\xDFFFF', U'\xEFFFE', U'\xEFFFF', U'\xFFFFE', U'\xFFFFF', U'\x10FFFE',
      U'\x10FFFF', U'\x0D800', U'\x0D801', U'\x0D802', U'\x0D803', U'\x0D804', U'\x0D805',
      U'\x0DFFF'};

  for (char32_t c : BAD_CHARS)
  {
    std::u32string s(1, c);
    std::string utf8str = OrigStringUtils::ToUtf8(s);

    // std::cout << "u32string: " << std::hex << s[0] << " utf8: " << utf8str << std::endl;

    std::string folded = OrigStringUtils::FoldCase(utf8str);
    std::u32string foldedU32 = OrigStringUtils::ToUtf32(folded);

    // std::cout << "folded: " << folded << " foldedU32: " << std::hex << foldedU32[0] << std::endl;
  }
  EXPECT_TRUE(true);

  static const char32_t REPLACED_CHARS[] = {U'\x0D800', U'\x0D801', U'\x0D802', U'\x0D803',
                                            U'\x0D804', U'\x0D805', U'\xDB7F',  U'\xDB80',
                                            U'\xDBFF',  U'\xDC00',  U'\x0DFFF'};

  for (char32_t c : REPLACED_CHARS)
  {
    std::u32string s(1, c);
    std::string utf8str = OrigStringUtils::ToUtf8(s);
    EXPECT_TRUE(OrigStringUtils::Equals(utf8str, UTF8_SUBSTITUTE_CHARACTER));

    // std::cout << "Bad Char: " << std::hex << s[0] << " converted to UTF8: "
    //     << std::hex << utf8str
    //    << " expected: " << std::hex << UTF8_SUBSTITUTE_CHARACTER << std::endl;

    std::string folded = OrigStringUtils::FoldCase(utf8str);
    std::u32string foldedU32 = OrigStringUtils::ToUtf32(folded);

    EXPECT_TRUE(OrigStringUtils::Equals(folded, UTF8_SUBSTITUTE_CHARACTER));

    EXPECT_EQ(foldedU32[0], CHAR32_T_SUBSTITUTE_CHARACTER[0]);
    // std::cout << "folded: " << folded << " foldedU32: " << std::hex << foldedU32[0] << std::endl;
  }
  EXPECT_TRUE(true);
  // More problems should show up with multi-byte utf8 characters missing the
  // first byte.

  static const std::string BAD_UTF8[] = {
      "\xcc"s, "\xb3"s, "\x9f"s, "\xab"s, "\x93", "\x8b",
  };
  for (std::string str : BAD_UTF8)
  {
    // std::cout << std::hex << str[0] << " utf8: " << str << std::endl;
    std::u32string trash = OrigStringUtils::ToUtf32(str);
    EXPECT_EQ(trash[0], CHAR32_T_SUBSTITUTE_CHARACTER[0]);
    EXPECT_EQ(trash[0], CHAR32_T_SUBSTITUTE_CHARACTER[0]);

    std::string folded = OrigStringUtils::FoldCase(str);
    std::u32string foldedU32 = OrigStringUtils::ToUtf32(folded);
    EXPECT_TRUE(OrigStringUtils::Equals(folded, UTF8_SUBSTITUTE_CHARACTER));
    EXPECT_EQ(foldedU32[0], CHAR32_T_SUBSTITUTE_CHARACTER[0]);

    // std::cout << "folded: " << folded << " foldedU32: " << std::hex << foldedU32[0] << std::endl;
  }
  EXPECT_TRUE(true);
}

TEST(TestStringUtils, FoldCase)
{
  std::string input = ""s;
  std::string result = OrigStringUtils::FoldCase(input);
  EXPECT_TRUE(result.size() == 0);

  input = "a"s;
  result = OrigStringUtils::FoldCase(input);
  EXPECT_EQ(result, input);

  input = "A"s;
  result = OrigStringUtils::FoldCase(input);
  EXPECT_EQ(result, "a"s);

  input = "What a WaIsT Of Time1234567890-=!@#$%^&*()\"_+QWERTYUIOP{}|qwertyuiop[];':\\<M>?/.,";
  std::string expected =
      "what a waist of time1234567890-=!@#$%^&*()\"_+qwertyuiop{}|qwertyuiop[];':\\<m>?/.,";

  result = OrigStringUtils::FoldCase(input);
  EXPECT_EQ(result, expected);

  input = "aB\0c"s; // Embedded null
  result = OrigStringUtils::FoldCase(input);
  EXPECT_EQ(result, "ab\0c"s);
  EXPECT_EQ(result.length(), 4);
  result = std::string(result.c_str());
  EXPECT_EQ(result, "ab"s);
  EXPECT_EQ(result.length(), 2);

  bool PrintResults = false;

  /*
   * The primary motivator for FoldCase's existence in Kodi: Turkic I....
   *
    * Behavior of ToLower, ToUpper & FoldCase on the four Turkic I characters. Note that
    * "Dotless I" is ASCII "I" and "Dotted small I" is ASCII "i". Turkic Locales
    * cause the Upper/Lower case rules for these four characters to be significantly
    * different from most languages.
    *
    *         Locale                    Unicode                               Unicode
    *                                  codepoint                          (hex 32-bit codepoint(s))
    * ToLower  C     I (Dotless I)       \u0049 -> i (Dotted small I)      \u0069 g++
    * ToLower  en_US I (Dotless I)       \u0049 -> i (Dotted small I)      \u0069 g++
    * ToLower  tr_TR I (Dotless I)       \u0049 -> i (Dotted small I)      \u0069 g++, custom
    * ToLower  tr_TR I (Dotless I)       \u0049 -> ı (Dotless small I)     \u0131 "correct" ICU4C

    * ToUpper  C     I (Dotless I)       \u0049 -> I (Dotless I)           \u0049 g++
    * ToUpper  en_US I (Dotless I)       \u0049 -> I (Dotless I)           \u0049 g++
    * ToUpper  tr_TR I (Dotless I)       \u0049 -> I (Dotless I)           \u0049 g++, custom
    * ToUpper  tr_TR I (Dotless I)       \u0049 -> I (Dotless I)           \u0049 "correct" ICU4C
    * FoldCase  N/A  I (Dotless I)       \u0049 -> i (Dotted small I)      \u0069 g++
    *
    * ToLower  C     i (Dotted small I)  \u0069 -> i (Dotted small I)      \u0069 g++
    * ToLower  en_US i (Dotted small I)  \u0069 -> i (Dotted small I)      \u0069 g++
    * ToLower  tr_TR i (Dotted small I)  \u0069 -> i (Dotted small I)      \u0069 g++
    * ToLower  tr_TR i (Dotted small I)  \u0069 -> i (Dotted small I)      \u0069 "correct" ICU4C

    * ToUpper  C     i (Dotted small I)  \u0069 -> I (Dotless I)           \u0049 g++
    * ToUpper  en_US i (Dotted small I)  \u0069 -> I (Dotless I)           \u0049 g++
    * ToUpper  tr_TR i (Dotted small I)  \u0069 -> İ (Dotted I)            \u0130 g++
    * ToUpper  tr_TR i (Dotted small I)  \u0069 -> I (Dotless I)           \u0049 "custom"
    * ToUpper  tr_TR i (Dotted small I)  \u0069 -> İ (Dotted I)            \u0130 "correct" ICU4C
    * FoldCase  N/A  i (Dotted small I)  \u0069 -> i (Dotted small I)      \u0069 g++
    *
    * ToLower  C     İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130
    * ToLower  en_US İ (Dotted I)        \u0130 -> i (Dotted small I)      \u0069 g++
    * ToLower  tr_TR İ (Dotted I)        \u0130 -> i (Dotted small I)      \u0069 g++
    * ToLower  tr_TR İ (Dotted I)        \u0130 -> i (Dotted small I)      \u0069 custom
    * ToLower  tr_TR İ (Dotted I)        \u0130 -> i (Dotted small I)      \u0069 "correct" ICU4C
    *
    * ToUpper  C     İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130 g++
    * ToUpper  en_US İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130 g++
    * ToUpper  tr_TR İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130 g++
    * ToUpper  tr_TR İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130 custom
    * ToUpper  tr_TR İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130 "correct" ICU4C
    * FoldCase  N/A  İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130
    *
    * ToLower  C     ı (Dotless small I) \u0131 -> ı (Dotless small I)     \u0131
    * ToLower  en_US ı (Dotless small I) \u0131 -> I (Dotless small I)     \u0131
    * ToLower  tr_TR ı (Dotless small I) \u0131 -> ı (Dotless small I)     \u0131
    * ToLower  tr_TR ı (Dotless small I) \u0131 -> I (Dotless I)           \u0049 "correct" ICU4C
    *
    * ToUpper  C     ı (Dotless small I) \u0131 -> ı (Dotless small I)     \u0131
    * ToUpper  en_US ı (Dotless small I) \u0131 -> I (Dotless I)           \u0049
    * ToUpper  tr_TR ı (Dotless small I) \u0131 -> I (Dotless I)           \u0049
    * ToUpper  tr_TR ı (Dotless small I) \u0131 -> I (Dotless I)           \u0049 "correct" ICU4C
    * FoldCase  N/A  ı (Dotless small I) \u0131 -> ı (Dotless small I)     \u0131
    *
    *
    * Note that even with FoldCase, the non-ASCII Turkic "I" characters do NOT get folded to ASCII
    *   "i", but they are in a 'normal' mapping:
    *    ToUpper(i) == I
    *    ToLower(I) == i
    *    ToUpper(Dotless small I) == Dotted I
    *    ToLower(Dotted I) == Dotless small I
    *    In this way the "normal" I and "Turkish I" characters are in two separate sets, but the
    *    casing behaves like you would expect.
    *
    *  Also note that the ICU4C based simple-case mapping implemented here maps the "I" characters
    *  the same, without paying attention to locale. The g++ version pays behaves differently
    *  depending upon the Locale, but it is still not "correct" Turkish behavior.
    *  It is "closer" to correct. In particular ToUpper("i") becomes Dotless I.
    *  So, at least for Turkish I, it is a bit of a toss up whether to use the
    *  locale insensitive version or the g++ lib. There are VERY FEW characters that are
    *  only locale sensitive. See SpecialCasing.txt, there are only about 120 rules.
    *  Only 13 of the rules depend upon locale.
    *
    * /
  std::locale C_UTF8 = OrigStringUtils::GetCLocale();
  std::locale en_GB_UTF8 = OrigStringUtils::GetEnglishLocale();
  std::string s1 = "I İ i ı";

  PrintResults = true;
  if (PrintResults)
  {
    std::cout << "Original string: " << s1 << std::endl;
  }

  std::string expectedResult;

  result = OrigStringUtils::ToUpper(s1, C_UTF8);
  expectedResult = "I İ I ı"; // Should only touch ASCII chars
  if (PrintResults)
  {
    std::cout << "C.UTF-8 ToUpper result: " << result << std::endl;
  }
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());
  expectedResult = "I İ I I";
  result = OrigStringUtils::ToUpper(s1, en_GB_UTF8);
  if (PrintResults)
  {
    std::cout << "en_GB.UTF-8 ToUpper result: " << result << std::endl;
  }
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  if (USE_BUILTIN_CASEMAPPING)
    expectedResult = "I İ İ I";
  else
    expectedResult = "I İ I I";

  try
  {
    std::locale tr_TR_UTF8 = std::locale("tr_TR.UTF-8");
    result = OrigStringUtils::ToUpper(s1, tr_TR_UTF8);
    EXPECT_STREQ(result.c_str(), expectedResult.c_str());
    if (PrintResults)
    {
      std::cout << "tr_TR.UTF-8 ToUpper result: " << result << std::endl;
    }
  }
  catch (std::runtime_error& e)
  {
    std::cout << "Locale tr_TR.UTF-8 not supported" << std::endl;
  }

  expectedResult = "i İ i ı"; // C locale ONLY changes ASCII chars
  result = OrigStringUtils::ToLower(s1, C_UTF8);
  if (PrintResults)
  {
    std::cout << "C.UTF-8 ToLower result: " << result << std::endl;
  }
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  expectedResult = "i i i ı";
  result = OrigStringUtils::ToLower(s1, en_GB_UTF8);
  if (PrintResults)
  {
    std::cout << "en_GB.UTF-8 ToLower result: " << result << std::endl;
  }
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  expectedResult = "i i i ı";
  try
   {
     std::locale tr_TR_UTF8 = std::locale("tr_TR.UTF-8");
     result = OrigStringUtils::ToLower(s1, tr_TR_UTF8);
     EXPECT_STREQ(result.c_str(), expectedResult.c_str());
     if (PrintResults)
     {
       std::cout << "tr_TR.UTF-8 ToLower result: " << result << std::endl;
     }
   }
   catch (std::runtime_error& e)
   {
     std::cout << "Locale tr_TR.UTF-8 not supported" << std::endl;
   }

  expectedResult = "i İ i ı";
  result = OrigStringUtils::FoldCase(s1);
  if (PrintResults)
  {
    std::cout << "FoldCase result: " << result << std::endl;
  }
  EXPECT_STREQ(result.c_str(), expectedResult.c_str());

  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  std::string s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  std::string result1 = OrigStringUtils::FoldCase(s1);
  std::string result2 = OrigStringUtils::FoldCase(s2);

  EXPECT_EQ(result1.compare(result2), 0);
  EXPECT_EQ(result1.compare(s2), 0);

  // Note that 'ß' is one beyond the last character in Table group (char 0xDF)
  // This makes it an excellent boundary test for FoldCase since it should NOT
  // be found in the table and therefore FoldCase should return itself. Further,
  // The value of the char (0XDF) is the value at Table[0] (the number of entries).

  s1 = "óóßChloë"s;
  result = "óóßchloë"s;
  EXPECT_EQ(OrigStringUtils::FoldCase(s1), result);

  // Last char in FoldCase table

// Darwin Unicode version too old to support these
#ifndef TARGET_DARWIN
  s1 = "𞤡"s;
  result = "𞥃"s;
  EXPECT_EQ(OrigStringUtils::FoldCase(s1), result);

  // Last output char in FoldCase table

  s1 = "𞥃"s;
  EXPECT_EQ(OrigStringUtils::FoldCase(s1), result);
#endif
}

TEST(TestStringUtils, FoldCase_W)
{
  // This implementation of FoldCase is not able to detect that these two UTF8 strings actually
  // represent the same Unicode codepoint

  bool PrintResults = false;

  std::wstring w_s1 =
      OrigStringUtils::ToWstring(std::string(TestOrigStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  std::wstring w_s2 =
      OrigStringUtils::ToWstring(std::string(TestOrigStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));

  w_s1 = OrigStringUtils::FoldCase(w_s1);
  w_s2 = OrigStringUtils::FoldCase(w_s2);
  int32_t result = w_s1.compare(w_s2);
  EXPECT_NE(result, 0);

  std::string s1 = "I İ i ı";
  std::string s2 = "i İ i ı";
  if (PrintResults)
  {
    std::cout << "Turkic orig s1: " << s1 << std::endl;
    std::cout << "Turkic orig s2: " << s2 << std::endl;
  }

  w_s1 = OrigStringUtils::ToWstring(s1);
  w_s2 = OrigStringUtils::ToWstring(s2);
  w_s1 = OrigStringUtils::FoldCase(w_s1);
  w_s2 = OrigStringUtils::FoldCase(w_s2);
  if (PrintResults)
  {
    std::cout << "Turkic folded w_s1: " << OrigStringUtils::ToUtf8(w_s1) << std::endl;
    std::cout << "Turkic folded w_s2: " << OrigStringUtils::ToUtf8(w_s2) << std::endl;
  }
  result = w_s1.compare(w_s2);
  EXPECT_EQ(result, 0);

  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  // std::cout << "Turkic orig s1: " << s1 << std::endl;
  // std::cout << "Turkic orig s2: " << s2 << std::endl;

  w_s1 = OrigStringUtils::ToWstring(s1);
  w_s2 = OrigStringUtils::ToWstring(s2);
  w_s1 = OrigStringUtils::FoldCase(w_s1);
  w_s2 = OrigStringUtils::FoldCase(w_s2);
  // std::cout << "Turkic folded w_s1: " << OrigStringUtils::ToUtf8(w_s1) << std::endl;
  // std::cout << "Turkic folded w_s2: " << OrigStringUtils::ToUtf8(w_s2) << std::endl;
  result = w_s1.compare(w_s2);
  EXPECT_EQ(result, 0);
}
*/

TEST(TestStringUtils, Equals)
{
  std::string refstr = "TeSt";
  std::string_view refstrView = "TeSt";
  EXPECT_TRUE(OrigStringUtils::Equals(refstr, "TeSt"));
  EXPECT_FALSE(OrigStringUtils::Equals(refstr, "tEsT"sv));
  EXPECT_FALSE(OrigStringUtils::Equals(refstrView, "TeSt "));
  EXPECT_TRUE(OrigStringUtils::Equals(refstrView, "TeSt"sv));
  EXPECT_FALSE(OrigStringUtils::Equals(refstr, "TeStTeStTeStTeSt"s));
  EXPECT_FALSE(OrigStringUtils::Equals(refstr, R"(TeStTeStTeStTeStx)"));

  std::string TeSt{"TeSt"};
  std::string tEsT{"tEsT"};

  std::string_view TestArrayS = "TestArray"sv;
  std::string_view TestArray2S = "TestArray2"sv;
  std::string_view TestArray3S = "TestArRaY"sv;

  const char* TestArray = TestArrayS.data();
  const char* TestArray2 = TestArray2S.data();
  const char* TestArray3 = TestArray3S.data();

  EXPECT_FALSE(OrigStringUtils::Equals(TestArray, TestArray2));
  EXPECT_FALSE(OrigStringUtils::Equals(TestArray, TestArray3));

  EXPECT_TRUE(OrigStringUtils::Equals("TeSt", "TeSt"));
  EXPECT_FALSE(OrigStringUtils::Equals(TeSt, tEsT));

  EXPECT_TRUE(OrigStringUtils::Equals(OrigStringUtils::Empty, OrigStringUtils::Empty));
  EXPECT_FALSE(OrigStringUtils::Equals(OrigStringUtils::Empty, "x"));
  EXPECT_FALSE(OrigStringUtils::Equals("x", OrigStringUtils::Empty));

  // Equals can handle embedded nulls, but you have to be careful
  // how you enter them. Ex: "abcd\0"  "ABCD\0"sv One has null, the other
  // is terminated at null.

  EXPECT_FALSE(OrigStringUtils::Equals("abcd\0", "ABCD\0"sv));
  EXPECT_TRUE(OrigStringUtils::Equals("abcd\0a", "abcd\0a"));
  EXPECT_FALSE(OrigStringUtils::Equals("abcd\0x", "ABCD\0y"sv));
  EXPECT_FALSE(OrigStringUtils::Equals("abcd\0", "ABCD\0a"sv));
}

TEST(TestStringUtils, EqualsNoCase)
{
  std::string refstr = "TeSt";
  std::string_view refstrView{"TeSt"};
  EXPECT_TRUE(OrigStringUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(OrigStringUtils::EqualsNoCase(refstr, "tEsT"sv));
  EXPECT_FALSE(OrigStringUtils::EqualsNoCase(refstrView, "TeSt "));
  EXPECT_FALSE(OrigStringUtils::EqualsNoCase(refstrView, "TeSt    x"sv));
  EXPECT_FALSE(OrigStringUtils::EqualsNoCase(refstr, "TeStTeStTeStTeSt"s));
  EXPECT_FALSE(OrigStringUtils::EqualsNoCase(refstr, R"(TeStTeStTeStTeStx)"));

  std::string TeSt{"TeSt"};
  std::string tEsT{"tEsT"};

  std::string_view TestArrayS = "TestArray"sv;
  std::string_view TestArray2S = "TestArray2"sv;
  std::string_view TestArray3S = "TestArRaY"sv;

  const char* TestArray = TestArrayS.data();
  const char* TestArray2 = TestArray2S.data();
  const char* TestArray3 = TestArray3S.data();

  EXPECT_FALSE(OrigStringUtils::EqualsNoCase(TestArray, TestArray2));
  EXPECT_TRUE(OrigStringUtils::EqualsNoCase(TestArray, TestArray3));

  EXPECT_TRUE(OrigStringUtils::EqualsNoCase("TeSt", "TeSt"));
  EXPECT_TRUE(OrigStringUtils::EqualsNoCase(TeSt, tEsT));

  const std::string constTest{"Test"};
  EXPECT_TRUE(OrigStringUtils::EqualsNoCase(TeSt, constTest));
  EXPECT_TRUE(OrigStringUtils::EqualsNoCase(TeSt + TeSt + TeSt + TeSt, "TeStTeStTeStTeSt"));
  EXPECT_FALSE(OrigStringUtils::EqualsNoCase(TeSt, "TeStTeStTeStTeStx"));

  EXPECT_TRUE(OrigStringUtils::EqualsNoCase(OrigStringUtils::Empty, OrigStringUtils::Empty));
  EXPECT_FALSE(OrigStringUtils::EqualsNoCase(OrigStringUtils::Empty, "x"));
  EXPECT_FALSE(OrigStringUtils::EqualsNoCase("x", OrigStringUtils::Empty));

  // EqualsNoCase can handle embedded nulls, but you have to be careful
  // how you enter them. Ex: "abcd\0"  "ABCD\0"sv One has null, the other
  // is terminated at null.

  EXPECT_FALSE(OrigStringUtils::EqualsNoCase("abcd\0", "ABCD\0"sv));
  EXPECT_TRUE(OrigStringUtils::EqualsNoCase("abcd\0a", "ABCD\0a"));
  EXPECT_FALSE(OrigStringUtils::EqualsNoCase("abcd\0x", "ABCD\0y"sv));
  EXPECT_FALSE(OrigStringUtils::EqualsNoCase("abcd\0", "ABCD\0a"sv));
}

TEST(TestStringUtils, CompareNoCase)
{
  std::string left;
  std::string right;
  int expectedResult{0};

  left = "abciI123ABC "s;
  right = "ABCIi123abc "s;
  expectedResult = 0;
  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), expectedResult);

  // Since Kodi's simple FoldCase can not handle characters that
  // change length when folding, the following fails to compare equal.
  // German 'ß' is equivalent to 'ss'

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 1;

  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), -1);

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;

  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), expectedResult);

  left = ""s;
  right = ""s;
  expectedResult = 0;
  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), expectedResult);

  EXPECT_EQ(OrigStringUtils::CompareNoCase(OrigStringUtils::Empty, OrigStringUtils::Empty), expectedResult);

  EXPECT_EQ(OrigStringUtils::CompareNoCase("a"sv, "a"s), 0);

  EXPECT_EQ(OrigStringUtils::CompareNoCase("a"sv, OrigStringUtils::Empty), 1);

  EXPECT_EQ(OrigStringUtils::CompareNoCase(OrigStringUtils::Empty, "a"sv), -1);

  EXPECT_EQ(OrigStringUtils::CompareNoCase("a"sv, "b"sv), -1);

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË

  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), 0);

  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right, 1), 0);

  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right, 100), 0);
}

TEST(TestStringUtils, CompareNoCase_Advanced)
{

  std::string haveNull = "Jane, you\0 are mistaken."s;
  // std::u32string u32Result = OrigStringUtils::ToUtf32(haveNull);

  // Length = # codepoints

  // EXPECT_EQ(9, u32Result.length());
  /// std::string sResult = OrigStringUtils::ToUtf8(u32Result);

  // EXPECT_EQ("Jane, you"s, sResult.data());

  std::vector<std::string> values{
      "This is a test"s, // 0
      "ThIS is A TeSt2"s, // 1
      "Another\x0Test"s, // 2
      "Are"s, // 3
      "\x0Null Not Special"s, // 4
      "x"s, // 5
      "\x0Lbbbbb"s // 6
  };

  // Both begin with null, which, after converting to u32string,
  // will make both empty u32strings

  int compareResult = OrigStringUtils::CompareNoCase(values[4], values[6]);
  EXPECT_EQ(1, compareResult);

  // Both will case fold to the same, except that values[1] is longer
  // When strings both equal until one neds, then the shorter one < longer
  // -1 == first shorter
  // 1 == second shorter
  compareResult = OrigStringUtils::CompareNoCase(values[1], values[0]);
  EXPECT_EQ(1, compareResult);

  // Emulate code for Windows device selection and sorting

  struct ci_less
  {
    bool operator()(const std::string& s1, const std::string& s2) const
    {
       return OrigStringUtils::CompareNoCase(s1, s2) < 0;
    }
  };
  std::set<std::string, ci_less> adapters;

  for (const auto& profile : values)
  {
    adapters.insert(profile);
  }

  std::vector<std::string> sResults;
  for (const std::string_view adapter : adapters)
  {
    sResults.push_back(std::string(adapter));
  }

  std::vector<std::string> expectedResults{
    "\x0Lbbbbb"s, // 0
      "\x0Null Not Special"s, // 1
      "Another"s, // 2
      "Are"s, // 3
      "This is a test"s, // 4   In case one is shorter and they are equal to that point
      //     shortest wins
      "ThIS is A TeSt2"s, // 5
      "x"s, // 6
  };

  EXPECT_EQ(sResults.size(), expectedResults.size());
  EXPECT_EQ(sResults[0].data(), expectedResults[0].data());
  EXPECT_EQ(sResults[1].data(), expectedResults[1].data());
  EXPECT_EQ(sResults[2].data(), expectedResults[2].data());
  EXPECT_EQ(sResults[3].data(), expectedResults[3].data());
  EXPECT_EQ(sResults[4].data(), expectedResults[4].data());
  EXPECT_EQ(sResults[5].data(), expectedResults[5].data());


  std::string left; // Don't use string_view!
  std::string right; // Don't use string_view!
  int expectedResult;

  left = "abciI123ABC ";
  left = left.substr(0, 5); // Limits string to 5 code-units (bytes)
  right = "ABCIi123abc ";
  right = right.substr(0, 5); // Not very exciting
  expectedResult = 0;
  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), expectedResult);

  left = "abciI123ABC ";
  right = "ABCIi ";
  // Compare first 5 Unicode code-points ('graphemes'/'characters') formed from the UTF8
  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right, 5), expectedResult);

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_LOWER_SS; // óósschloë

  // Single byte comparison results in < 0
  // Unicode codepoint comparision results in 0

  EXPECT_TRUE(OrigStringUtils::CompareNoCase(left, right) == 0);

  // More interesting guessing the byte length for multibyte characters, eh?
  // These are ALL full-foldcase equivalent (but not simple-foldcase, as is here).
  // Curious that the byte lengths are same but character / codepoint counts are not.
  //
  // const char UTF8_GERMAN_SAMPLE[] =
  //      ó      ó       ß        C   h   l   o   ë
  // { "\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab" };
  //     1       2       3       4   5   6   7   8           character count
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count
  //
  // const char* UTF8_GERMAN_UPPER =
  //  u"  Ó       Ó      S   S   C   H   L   O  Ë";
  // { "\xc3\x93\xc3\x93\x53\x53\x43\x48\x4c\x4f\xc3\x8b" };
  //     1       2       3   4   5  6   7   8   9           character count
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count  //
  // const char UTF8_GERMAN_LOWER_SS =
  //     ó        ó       s   s   c   h   l   o   ë"
  // { "\xc3\xb3\xc3\xb3\x73\x73\x63\x68\x6c\x6f\xc3\xab" };
  //     1        2       3   4   5   6   7   8   9
  //     1   2    3   4   5   6   7   8   9   10   11  13     byte count
  // const char* UTF8_GERMAN_LOWER =
  //   u"ó       ó       ß       C   h   l   o   ë         // "ß" becomes "ss" during fold
  // { "\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab" };
  //     1       2       3       4    5  6   7   8           character count
  //     1   2   3   4   5   6   7   8   9   10   11  13     byte count

  // This implementation will NOT FoldCase "ß" to "ss"

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), -1);

  left = {TestStringUtils::UTF8_GERMAN_UPPER, 4}; // ÓÓSSCHLOË byte 4 = end of 2nd Ó
  right = {TestStringUtils::UTF8_GERMAN_SAMPLE, 4}; // óóßChloë  byte 4 = end of 2nd ó

  // Single byte comparison results in < 0 (always will be < 0, due to high-bit
  // set on all start bytes of multi-byte sequence)
  // Unicode codepoint comparision results in == 0

  EXPECT_TRUE(OrigStringUtils::CompareNoCase(left, right) == 0);

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;

  // Compare first two code-points (same result as previous test with limit of 4th byte)
  // New API redefines the third argument to be the number of code-points ('characters')
  // instead of bytes.

  EXPECT_TRUE(OrigStringUtils::CompareNoCase(left, right, 2) == 0);

  // A full-foldcase would recognize that "ß" and "ss" are equivalent.
  // Attempting here to confirm current behavior.
  // Limit by bytes, since limiting by the same # code-points doesn't produce
  // desired result

  left = {TestStringUtils::UTF8_GERMAN_UPPER, 0, 6}; // ÓÓSSCHLOË
  right = {TestStringUtils::UTF8_GERMAN_SAMPLE, 0, 6}; // óóßChloë
  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), -1);

  /*
  // Limit by number of codepoints since we use utf32string inside

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  std::u32string leftUtf32 = {OrigStringUtils::ToUtf32(left), 0, 4}; // => ÓÓSS
  left = OrigStringUtils::ToUtf8(leftUtf32);

  right = TestStringUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  std::u32string rightUtf32 = {OrigStringUtils::ToUtf32(right), 0, 3}; // => óóß
  right = OrigStringUtils::ToUtf8(rightUtf32);
  EXPECT_TRUE(OrigStringUtils::CompareNoCase(left, right) < 0);

*/
  // Without normalization (beyond the capabilities of this implementation)
  // the code doesn't recognize that these characters are equivalent.
  // Fortunately, in normal use, normalization is rarely used, except
  // for processing, such as fold case. Still, these are not very likely
  // to occur.

  left = TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1; // 6 bytes
  right = TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5; // 4 bytes
  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), -1);

  // Boundary Tests

  left = "abciI123ABC ";
  right = "ABCIi123abc ";
  expectedResult = 0;
  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right, 0), expectedResult);

  left = "";
  right = "ABCIi123abc ";
  EXPECT_TRUE(OrigStringUtils::CompareNoCase(left, right) > 0);

  left = "abciI123ABC ";
  right = "";
  EXPECT_TRUE(OrigStringUtils::CompareNoCase(left, right) < 0);

  left = "";
  right = "";
  expectedResult = 0;
  EXPECT_EQ(OrigStringUtils::CompareNoCase(left, right), expectedResult);
}

TEST(TestStringUtils, Left)
{
  std::string refstr;
  std::string varstr;
  std::string origstr = "test";

  refstr = "";
  varstr = OrigStringUtils::Left(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "te";
  varstr = OrigStringUtils::Left(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test";
  varstr = OrigStringUtils::Left(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Mid)
{
  std::string refstr;
  std::string varstr;
  std::string origstr{"test"};

  refstr = "";
  varstr = OrigStringUtils::Mid(origstr, 0, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "te";
  varstr = OrigStringUtils::Mid(origstr, 0, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test";
  varstr = OrigStringUtils::Mid(origstr, 0, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = OrigStringUtils::Mid(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = OrigStringUtils::Mid(origstr, 2, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "es";
  varstr = OrigStringUtils::Mid(origstr, 1, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Right)
{
  std::string refstr;
  std::string varstr;
  std::string origstr{"test"};

  refstr = "";
  varstr = OrigStringUtils::Right(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = OrigStringUtils::Right(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test";
  varstr = OrigStringUtils::Right(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Trim)
{
  std::string refstr{"test test"};

  std::string varstr{" test test   "};
  OrigStringUtils::Trim(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, TrimLeft)
{
  std::string refstr = "test test   ";

  std::string varstr = " test test   ";
  OrigStringUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, TrimRight)
{
  std::string refstr = " test test";

  std::string varstr = " test test   ";
  OrigStringUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Replace)
{
  std::string refstr = "text text";

  std::string varstr = "test test";
  EXPECT_EQ(OrigStringUtils::Replace(varstr, 's', 'x'), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(OrigStringUtils::Replace(varstr, 's', 'x'), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = "test test";
  EXPECT_EQ(OrigStringUtils::Replace(varstr, "s", "x"), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(OrigStringUtils::Replace(varstr, "s", "x"), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, StartsWith)
{
  std::string refstr = "test";
  std::string input;
  std::string p;

  EXPECT_FALSE(OrigStringUtils::StartsWith(refstr, "x"));
  EXPECT_TRUE(OrigStringUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(OrigStringUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(OrigStringUtils::StartsWith(refstr, "Te"));
  EXPECT_FALSE(OrigStringUtils::StartsWith(refstr, "test "));
  EXPECT_TRUE(OrigStringUtils::StartsWith(refstr, "test\0")); // Embedded null terminates string

  p = {"tes"};
  EXPECT_TRUE(OrigStringUtils::StartsWith(refstr, p));

  // Boundary

  input = "";
  EXPECT_TRUE(OrigStringUtils::StartsWith(input, ""));
  EXPECT_FALSE(OrigStringUtils::StartsWith(input, "Four score and seven years ago"));

  EXPECT_TRUE(OrigStringUtils::StartsWith(refstr, ""));

  input = "";
  p = "";
  EXPECT_TRUE(OrigStringUtils::StartsWith(input, p));
  EXPECT_TRUE(OrigStringUtils::StartsWith(refstr, p));
}

TEST(TestStringUtils, StartsWithNoCase)
{
  std::string refstr = "test";
  std::string input;
  std::string p;

  EXPECT_FALSE(OrigStringUtils::StartsWithNoCase(refstr, "x"));
  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(refstr, "TesT"));
  EXPECT_FALSE(OrigStringUtils::StartsWithNoCase(refstr, "Te st"));
  EXPECT_FALSE(OrigStringUtils::StartsWithNoCase(refstr, "test "));
  EXPECT_TRUE(
      OrigStringUtils::StartsWithNoCase(refstr, "test\0")); // Embedded null terminates string operation

  p = "tEs";
  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(refstr, p));

  // Boundary

  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(refstr, ""));

  p = ""sv;
  // Verify Non-empty string begins with empty string.
  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(refstr, p));
  // Same behavior with char * and string
  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(refstr, ""));

  input = "";
  // Empty string does begin with empty string
  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(input, ""));
  EXPECT_FALSE(OrigStringUtils::StartsWithNoCase(input, "Four score and seven years ago"));

  input = "";
  p = "";
  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(input, p));
}

TEST(TestStringUtils, EndsWith)
{
  std::string refstr = "test";

  EXPECT_TRUE(OrigStringUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(OrigStringUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(OrigStringUtils::EndsWith(refstr, "sT"));
}

TEST(TestStringUtils, EndsWithNoCase)
{
  std::string refstr = "test";
  EXPECT_FALSE(OrigStringUtils::EndsWithNoCase(refstr, "x"));
  EXPECT_TRUE(OrigStringUtils::EndsWithNoCase(refstr, "sT"));
  EXPECT_TRUE(OrigStringUtils::EndsWithNoCase(refstr, "TesT"));
}

TEST(TestStringUtils, Join)
{
  std::string refstr;
  std::string varstr;
  std::vector<std::string> strarray;

  strarray.emplace_back("a");
  strarray.emplace_back("b");
  strarray.emplace_back("c");
  strarray.emplace_back("de");
  strarray.emplace_back(",");
  strarray.emplace_back("fg");
  strarray.emplace_back(",");
  refstr = "a,b,c,de,,,fg,,";
  varstr = OrigStringUtils::Join(strarray, ",");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Split)
{
  std::vector<std::string> varresults;

  // test overload with string as delimiter
  varresults = OrigStringUtils::Split("g,h,ij,k,lm,,n", ",");
  EXPECT_STREQ("g", varresults.at(0).c_str());
  EXPECT_STREQ("h", varresults.at(1).c_str());
  EXPECT_STREQ("ij", varresults.at(2).c_str());
  EXPECT_STREQ("k", varresults.at(3).c_str());
  EXPECT_STREQ("lm", varresults.at(4).c_str());
  EXPECT_STREQ("", varresults.at(5).c_str());
  EXPECT_STREQ("n", varresults.at(6).c_str());

  EXPECT_TRUE(OrigStringUtils::Split("", "|").empty());

  EXPECT_EQ(4U, OrigStringUtils::Split("a bc  d ef ghi ", " ", 4).size());
  EXPECT_STREQ("d ef ghi ", OrigStringUtils::Split("a bc  d ef ghi ", " ", 4).at(3).c_str())
      << "Last part must include rest of the input string";
  EXPECT_EQ(7U, OrigStringUtils::Split("a bc  d ef ghi ", " ").size())
      << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", OrigStringUtils::Split("a bc  d ef ghi ", " ").at(1).c_str());
  EXPECT_STREQ("", OrigStringUtils::Split("a bc  d ef ghi ", " ").at(2).c_str());
  EXPECT_STREQ("", OrigStringUtils::Split("a bc  d ef ghi ", " ").at(6).c_str());

  EXPECT_EQ(2U, OrigStringUtils::Split("a bc  d ef ghi ", "  ").size());
  EXPECT_EQ(2U, OrigStringUtils::Split("a bc  d ef ghi ", "  ", 10).size());
  EXPECT_STREQ("a bc", OrigStringUtils::Split("a bc  d ef ghi ", "  ", 10).at(0).c_str());

  EXPECT_EQ(1U, OrigStringUtils::Split("a bc  d ef ghi ", " z").size());
  EXPECT_STREQ("a bc  d ef ghi ", OrigStringUtils::Split("a bc  d ef ghi ", " z").at(0).c_str());

  EXPECT_EQ(1U, OrigStringUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", OrigStringUtils::Split("a bc  d ef ghi ", "").at(0).c_str());

  // test overload with char as delimiter
  EXPECT_EQ(4U, OrigStringUtils::Split("a bc  d ef ghi ", ' ', 4).size());
  EXPECT_STREQ("d ef ghi ", OrigStringUtils::Split("a bc  d ef ghi ", ' ', 4).at(3).c_str());
  EXPECT_EQ(7U, OrigStringUtils::Split("a bc  d ef ghi ", ' ').size())
      << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", OrigStringUtils::Split("a bc  d ef ghi ", ' ').at(1).c_str());
  EXPECT_STREQ("", OrigStringUtils::Split("a bc  d ef ghi ", ' ').at(2).c_str());
  EXPECT_STREQ("", OrigStringUtils::Split("a bc  d ef ghi ", ' ').at(6).c_str());

  EXPECT_EQ(1U, OrigStringUtils::Split("a bc  d ef ghi ", 'z').size());
  EXPECT_STREQ("a bc  d ef ghi ", OrigStringUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());

  EXPECT_EQ(1U, OrigStringUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", OrigStringUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());
}

TEST(TestStringUtils, FindNumber)
{
  EXPECT_EQ(3, OrigStringUtils::FindNumber("aabcaadeaa", "aa"));
  EXPECT_EQ(1, OrigStringUtils::FindNumber("aabcaadeaa", "b"));
}

TEST(TestStringUtils, AlphaNumericCompare)
{
  int64_t ref;
  int64_t var;

  ref = 0;
  var = OrigStringUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
}

TEST(TestStringUtils, TimeStringToSeconds)
{
  EXPECT_EQ(77455, OrigStringUtils::TimeStringToSeconds("21:30:55"));
  EXPECT_EQ(7 * 60, OrigStringUtils::TimeStringToSeconds("7 min"));
  EXPECT_EQ(7 * 60, OrigStringUtils::TimeStringToSeconds("7 min\t"));
  EXPECT_EQ(154 * 60, OrigStringUtils::TimeStringToSeconds("   154 min"));
  EXPECT_EQ(1 * 60 + 1, OrigStringUtils::TimeStringToSeconds("1:01"));
  EXPECT_EQ(4 * 60 + 3, OrigStringUtils::TimeStringToSeconds("4:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, OrigStringUtils::TimeStringToSeconds("2:04:03"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, OrigStringUtils::TimeStringToSeconds("   2:4:3"));
  EXPECT_EQ(2 * 3600 + 4 * 60 + 3, OrigStringUtils::TimeStringToSeconds("  \t\t 02:04:03 \n "));
  EXPECT_EQ(1 * 3600 + 5 * 60 + 2, OrigStringUtils::TimeStringToSeconds("01:05:02:04:03 \n "));
  EXPECT_EQ(0, OrigStringUtils::TimeStringToSeconds("blah"));
  EXPECT_EQ(0, OrigStringUtils::TimeStringToSeconds("ля-ля"));
}

TEST(TestStringUtils, RemoveCRLF)
{
  std::string refstr;
  std::string varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  OrigStringUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, utf8_strlen)
{
  size_t ref;
  size_t var;

  ref = 9;
  var = OrigStringUtils::utf8_strlen("ｔｅｓｔ＿ＵＴＦ８");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, SecondsToTimeString)
{
  std::string refstr;
  std::string varstr;

  refstr = "21:30:55";
  varstr = OrigStringUtils::SecondsToTimeString(77455);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, IsNaturalNumber)
{
  EXPECT_TRUE(OrigStringUtils::IsNaturalNumber("10"));
  EXPECT_TRUE(OrigStringUtils::IsNaturalNumber(" 10"));
  EXPECT_TRUE(OrigStringUtils::IsNaturalNumber("0"));
  EXPECT_FALSE(OrigStringUtils::IsNaturalNumber(" 1 0"));
  EXPECT_FALSE(OrigStringUtils::IsNaturalNumber("1.0"));
  EXPECT_FALSE(OrigStringUtils::IsNaturalNumber("1.1"));
  EXPECT_FALSE(OrigStringUtils::IsNaturalNumber("0x1"));
  EXPECT_FALSE(OrigStringUtils::IsNaturalNumber("blah"));
  EXPECT_FALSE(OrigStringUtils::IsNaturalNumber("120 h"));
  EXPECT_FALSE(OrigStringUtils::IsNaturalNumber(" "));
  EXPECT_FALSE(OrigStringUtils::IsNaturalNumber(""));
}

TEST(TestStringUtils, IsInteger)
{
  EXPECT_TRUE(OrigStringUtils::IsInteger("10"));
  EXPECT_TRUE(OrigStringUtils::IsInteger(" -10"));
  EXPECT_TRUE(OrigStringUtils::IsInteger("0"));
  EXPECT_FALSE(OrigStringUtils::IsInteger(" 1 0"));
  EXPECT_FALSE(OrigStringUtils::IsInteger("1.0"));
  EXPECT_FALSE(OrigStringUtils::IsInteger("1.1"));
  EXPECT_FALSE(OrigStringUtils::IsInteger("0x1"));
  EXPECT_FALSE(OrigStringUtils::IsInteger("blah"));
  EXPECT_FALSE(OrigStringUtils::IsInteger("120 h"));
  EXPECT_FALSE(OrigStringUtils::IsInteger(" "));
  EXPECT_FALSE(OrigStringUtils::IsInteger(""));
}

TEST(TestStringUtils, SizeToString)
{
  std::string ref;
  std::string var;

  ref = "2.00 GB";
  var = OrigStringUtils::SizeToString(2147483647);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "0.00 B";
  var = OrigStringUtils::SizeToString(0);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStringUtils, EmptyString)
{
  EXPECT_STREQ("", OrigStringUtils::Empty.c_str());
}

TEST(TestStringUtils, FindWords)
{
  size_t ref;
  size_t var;

  ref = 5;
  var = OrigStringUtils::FindWords("test string", "string");
  EXPECT_EQ(ref, var);
  var = OrigStringUtils::FindWords("12345string", "string");
  EXPECT_EQ(ref, var);
  var = OrigStringUtils::FindWords("apple2012", "2012");
  EXPECT_EQ(ref, var);
  ref = -1;
  var = OrigStringUtils::FindWords("12345string", "ring");
  EXPECT_EQ(ref, var);
  var = OrigStringUtils::FindWords("12345string", "345");
  EXPECT_EQ(ref, var);
  var = OrigStringUtils::FindWords("apple2012", "e2012");
  EXPECT_EQ(ref, var);
  var = OrigStringUtils::FindWords("apple2012", "12");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindWords_NonAscii)
{
  size_t ref;
  size_t var;

  ref = 6;
  var = OrigStringUtils::FindWords("我的视频", "视频");
  EXPECT_EQ(ref, var);
  var = OrigStringUtils::FindWords("我的视频", "视");
  EXPECT_EQ(ref, var);
  var = OrigStringUtils::FindWords("Apple ple", "ple");
  EXPECT_EQ(ref, var);
  ref = 7;
  var = OrigStringUtils::FindWords("Äpfel.pfel", "pfel");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindEndBracket)
{
  size_t ref;
  size_t var;

  ref = 11;
  var = OrigStringUtils::FindEndBracket("atest testbb test", 'a', 'b');
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, DateStringToYYYYMMDD)
{
  size_t ref;
  size_t var;

  ref = 20120706;
  var = OrigStringUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, WordToDigits)
{
  std::string ref;
  std::string var;

  ref = "8378 787464";
  var = "test string";
  OrigStringUtils::WordToDigits(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStringUtils, CreateUUID)
{
  std::cout << "CreateUUID(): " << OrigStringUtils::CreateUUID() << std::endl;
}

TEST(TestStringUtils, ValidateUUID)
{
  EXPECT_TRUE(OrigStringUtils::ValidateUUID(OrigStringUtils::CreateUUID()));
}

TEST(TestStringUtils, CompareFuzzy)
{
  double ref;
  double var;

  ref = 6.25;
  var = OrigStringUtils::CompareFuzzy("test string", "string test");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindBestMatch)
{
  double refdouble;
  double vardouble;
  int refint;
  int varint;
  std::vector<std::string> strarray;

  refint = 3;
  refdouble = 0.5625;
  strarray.emplace_back("");
  strarray.emplace_back("a");
  strarray.emplace_back("e");
  strarray.emplace_back("es");
  strarray.emplace_back("t");
  varint = OrigStringUtils::FindBestMatch("test", strarray, vardouble);
  EXPECT_EQ(refint, varint);
  EXPECT_EQ(refdouble, vardouble);
}

TEST(TestStringUtils, Paramify)
{
  const char* input = "some, very \\ odd \"string\"";
  const char* ref = "\"some, very \\\\ odd \\\"string\\\"\"";

  std::string result = OrigStringUtils::Paramify(input);
  EXPECT_STREQ(ref, result.c_str());
}

TEST(TestStringUtils, sortstringbyname)
{
  std::vector<std::string> strarray;
  strarray.emplace_back("B");
  strarray.emplace_back("c");
  strarray.emplace_back("a");
  std::sort(strarray.begin(), strarray.end(), origsortstringbyname());

  EXPECT_STREQ("a", strarray[0].c_str());
  EXPECT_STREQ("B", strarray[1].c_str());
  EXPECT_STREQ("c", strarray[2].c_str());
}

TEST(TestStringUtils, FileSizeFormat)
{
  EXPECT_STREQ("0B", OrigStringUtils::FormatFileSize(0).c_str());

  EXPECT_STREQ("999B", OrigStringUtils::FormatFileSize(999).c_str());
  EXPECT_STREQ("0.98kB", OrigStringUtils::FormatFileSize(1000).c_str());

  EXPECT_STREQ("1.00kB", OrigStringUtils::FormatFileSize(1024).c_str());
  EXPECT_STREQ("9.99kB", OrigStringUtils::FormatFileSize(10229).c_str());

  EXPECT_STREQ("10.1kB", OrigStringUtils::FormatFileSize(10387).c_str());
  EXPECT_STREQ("99.9kB", OrigStringUtils::FormatFileSize(102297).c_str());

  EXPECT_STREQ("100kB", OrigStringUtils::FormatFileSize(102400).c_str());
  EXPECT_STREQ("999kB", OrigStringUtils::FormatFileSize(1023431).c_str());

  EXPECT_STREQ("0.98MB", OrigStringUtils::FormatFileSize(1023897).c_str());
  EXPECT_STREQ("0.98MB", OrigStringUtils::FormatFileSize(1024000).c_str());

  //Last unit should overflow the 3 digit limit
  EXPECT_STREQ("5432PB", OrigStringUtils::FormatFileSize(6115888293969133568).c_str());
}

TEST(TestStringUtils, ToHexadecimal)
{
  EXPECT_STREQ("", OrigStringUtils::ToHexadecimal("").c_str());
  EXPECT_STREQ("616263", OrigStringUtils::ToHexadecimal("abc").c_str());
  std::string a{"a\0b\n", 4};
  EXPECT_STREQ("6100620a", OrigStringUtils::ToHexadecimal(a).c_str());
  std::string nul{"\0", 1};
  EXPECT_STREQ("00", OrigStringUtils::ToHexadecimal(nul).c_str());
  std::string ff{"\xFF", 1};
  EXPECT_STREQ("ff", OrigStringUtils::ToHexadecimal(ff).c_str());
}

// Sorry, there are about 600 lines here....
//
// Case folding tables in StringUtils.cpp "FOLDCASE_..." are derived from
// Unicode Inc.'s data file, CaseFolding.txt (CaseFolding-14.0.0.txt, 2021-03-08).
// Below, UnicodeFoldUpperTable & UnicodeFoldLowerTable were created directly
// from CaseFolding.txt by simple editing. These tables are fed to
// the utility in xbmc/utils/unicode_tools to produce the FOLDCASE_... tables.
//
// This test verifies that FOLDCASE tables are correct.
//
// These tables provide for "simple case folding" that is not locale sensitive. They do NOT
// support "Full Case Folding" which can fold single characters into multiple, or multiple
// into single, etc. CaseFolding.txt can be found in the ICUC4 source directory:
// icu/source/data/unidata.
//
// clang-format off
const char32_t UnicodeFoldUpperTable[]{
    U'\x0041',  U'\x0042',  U'\x0043',  U'\x0044',  U'\x0045',  U'\x0046',  U'\x0047',  U'\x0048',
    U'\x0049',  U'\x004A',  U'\x004B',  U'\x004C',  U'\x004D',  U'\x004E',  U'\x004F',  U'\x0050',
    U'\x0051',  U'\x0052',  U'\x0053',  U'\x0054',  U'\x0055',  U'\x0056',  U'\x0057',  U'\x0058',
    U'\x0059',  U'\x005A',  U'\x00B5',  U'\x00C0',  U'\x00C1',  U'\x00C2',  U'\x00C3',  U'\x00C4',
    U'\x00C5',  U'\x00C6',  U'\x00C7',  U'\x00C8',  U'\x00C9',  U'\x00CA',  U'\x00CB',  U'\x00CC',
    U'\x00CD',  U'\x00CE',  U'\x00CF',  U'\x00D0',  U'\x00D1',  U'\x00D2',  U'\x00D3',  U'\x00D4',
    U'\x00D5',  U'\x00D6',  U'\x00D8',  U'\x00D9',  U'\x00DA',  U'\x00DB',  U'\x00DC',  U'\x00DD',
    U'\x00DE',  U'\x0100',  U'\x0102',  U'\x0104',  U'\x0106',  U'\x0108',  U'\x010A',  U'\x010C',
    U'\x010E',  U'\x0110',  U'\x0112',  U'\x0114',  U'\x0116',  U'\x0118',  U'\x011A',  U'\x011C',
    U'\x011E',  U'\x0120',  U'\x0122',  U'\x0124',  U'\x0126',  U'\x0128',  U'\x012A',  U'\x012C',
    U'\x012E',  U'\x0132',  U'\x0134',  U'\x0136',  U'\x0139',  U'\x013B',  U'\x013D',  U'\x013F',
    U'\x0141',  U'\x0143',  U'\x0145',  U'\x0147',  U'\x014A',  U'\x014C',  U'\x014E',  U'\x0150',
    U'\x0152',  U'\x0154',  U'\x0156',  U'\x0158',  U'\x015A',  U'\x015C',  U'\x015E',  U'\x0160',
    U'\x0162',  U'\x0164',  U'\x0166',  U'\x0168',  U'\x016A',  U'\x016C',  U'\x016E',  U'\x0170',
    U'\x0172',  U'\x0174',  U'\x0176',  U'\x0178',  U'\x0179',  U'\x017B',  U'\x017D',  U'\x017F',
    U'\x0181',  U'\x0182',  U'\x0184',  U'\x0186',  U'\x0187',  U'\x0189',  U'\x018A',  U'\x018B',
    U'\x018E',  U'\x018F',  U'\x0190',  U'\x0191',  U'\x0193',  U'\x0194',  U'\x0196',  U'\x0197',
    U'\x0198',  U'\x019C',  U'\x019D',  U'\x019F',  U'\x01A0',  U'\x01A2',  U'\x01A4',  U'\x01A6',
    U'\x01A7',  U'\x01A9',  U'\x01AC',  U'\x01AE',  U'\x01AF',  U'\x01B1',  U'\x01B2',  U'\x01B3',
    U'\x01B5',  U'\x01B7',  U'\x01B8',  U'\x01BC',  U'\x01C4',  U'\x01C5',  U'\x01C7',  U'\x01C8',
    U'\x01CA',  U'\x01CB',  U'\x01CD',  U'\x01CF',  U'\x01D1',  U'\x01D3',  U'\x01D5',  U'\x01D7',
    U'\x01D9',  U'\x01DB',  U'\x01DE',  U'\x01E0',  U'\x01E2',  U'\x01E4',  U'\x01E6',  U'\x01E8',
    U'\x01EA',  U'\x01EC',  U'\x01EE',  U'\x01F1',  U'\x01F2',  U'\x01F4',  U'\x01F6',  U'\x01F7',
    U'\x01F8',  U'\x01FA',  U'\x01FC',  U'\x01FE',  U'\x0200',  U'\x0202',  U'\x0204',  U'\x0206',
    U'\x0208',  U'\x020A',  U'\x020C',  U'\x020E',  U'\x0210',  U'\x0212',  U'\x0214',  U'\x0216',
    U'\x0218',  U'\x021A',  U'\x021C',  U'\x021E',  U'\x0220',  U'\x0222',  U'\x0224',  U'\x0226',
    U'\x0228',  U'\x022A',  U'\x022C',  U'\x022E',  U'\x0230',  U'\x0232',  U'\x023A',  U'\x023B',
    U'\x023D',  U'\x023E',  U'\x0241',  U'\x0243',  U'\x0244',  U'\x0245',  U'\x0246',  U'\x0248',
    U'\x024A',  U'\x024C',  U'\x024E',  U'\x0345',  U'\x0370',  U'\x0372',  U'\x0376',  U'\x037F',
    U'\x0386',  U'\x0388',  U'\x0389',  U'\x038A',  U'\x038C',  U'\x038E',  U'\x038F',  U'\x0391',
    U'\x0392',  U'\x0393',  U'\x0394',  U'\x0395',  U'\x0396',  U'\x0397',  U'\x0398',  U'\x0399',
    U'\x039A',  U'\x039B',  U'\x039C',  U'\x039D',  U'\x039E',  U'\x039F',  U'\x03A0',  U'\x03A1',
    U'\x03A3',  U'\x03A4',  U'\x03A5',  U'\x03A6',  U'\x03A7',  U'\x03A8',  U'\x03A9',  U'\x03AA',
    U'\x03AB',  U'\x03C2',  U'\x03CF',  U'\x03D0',  U'\x03D1',  U'\x03D5',  U'\x03D6',  U'\x03D8',
    U'\x03DA',  U'\x03DC',  U'\x03DE',  U'\x03E0',  U'\x03E2',  U'\x03E4',  U'\x03E6',  U'\x03E8',
    U'\x03EA',  U'\x03EC',  U'\x03EE',  U'\x03F0',  U'\x03F1',  U'\x03F4',  U'\x03F5',  U'\x03F7',
    U'\x03F9',  U'\x03FA',  U'\x03FD',  U'\x03FE',  U'\x03FF',  U'\x0400',  U'\x0401',  U'\x0402',
    U'\x0403',  U'\x0404',  U'\x0405',  U'\x0406',  U'\x0407',  U'\x0408',  U'\x0409',  U'\x040A',
    U'\x040B',  U'\x040C',  U'\x040D',  U'\x040E',  U'\x040F',  U'\x0410',  U'\x0411',  U'\x0412',
    U'\x0413',  U'\x0414',  U'\x0415',  U'\x0416',  U'\x0417',  U'\x0418',  U'\x0419',  U'\x041A',
    U'\x041B',  U'\x041C',  U'\x041D',  U'\x041E',  U'\x041F',  U'\x0420',  U'\x0421',  U'\x0422',
    U'\x0423',  U'\x0424',  U'\x0425',  U'\x0426',  U'\x0427',  U'\x0428',  U'\x0429',  U'\x042A',
    U'\x042B',  U'\x042C',  U'\x042D',  U'\x042E',  U'\x042F',  U'\x0460',  U'\x0462',  U'\x0464',
    U'\x0466',  U'\x0468',  U'\x046A',  U'\x046C',  U'\x046E',  U'\x0470',  U'\x0472',  U'\x0474',
    U'\x0476',  U'\x0478',  U'\x047A',  U'\x047C',  U'\x047E',  U'\x0480',  U'\x048A',  U'\x048C',
    U'\x048E',  U'\x0490',  U'\x0492',  U'\x0494',  U'\x0496',  U'\x0498',  U'\x049A',  U'\x049C',
    U'\x049E',  U'\x04A0',  U'\x04A2',  U'\x04A4',  U'\x04A6',  U'\x04A8',  U'\x04AA',  U'\x04AC',
    U'\x04AE',  U'\x04B0',  U'\x04B2',  U'\x04B4',  U'\x04B6',  U'\x04B8',  U'\x04BA',  U'\x04BC',
    U'\x04BE',  U'\x04C0',  U'\x04C1',  U'\x04C3',  U'\x04C5',  U'\x04C7',  U'\x04C9',  U'\x04CB',
    U'\x04CD',  U'\x04D0',  U'\x04D2',  U'\x04D4',  U'\x04D6',  U'\x04D8',  U'\x04DA',  U'\x04DC',
    U'\x04DE',  U'\x04E0',  U'\x04E2',  U'\x04E4',  U'\x04E6',  U'\x04E8',  U'\x04EA',  U'\x04EC',
    U'\x04EE',  U'\x04F0',  U'\x04F2',  U'\x04F4',  U'\x04F6',  U'\x04F8',  U'\x04FA',  U'\x04FC',
    U'\x04FE',  U'\x0500',  U'\x0502',  U'\x0504',  U'\x0506',  U'\x0508',  U'\x050A',  U'\x050C',
    U'\x050E',  U'\x0510',  U'\x0512',  U'\x0514',  U'\x0516',  U'\x0518',  U'\x051A',  U'\x051C',
    U'\x051E',  U'\x0520',  U'\x0522',  U'\x0524',  U'\x0526',  U'\x0528',  U'\x052A',  U'\x052C',
    U'\x052E',  U'\x0531',  U'\x0532',  U'\x0533',  U'\x0534',  U'\x0535',  U'\x0536',  U'\x0537',
    U'\x0538',  U'\x0539',  U'\x053A',  U'\x053B',  U'\x053C',  U'\x053D',  U'\x053E',  U'\x053F',
    U'\x0540',  U'\x0541',  U'\x0542',  U'\x0543',  U'\x0544',  U'\x0545',  U'\x0546',  U'\x0547',
    U'\x0548',  U'\x0549',  U'\x054A',  U'\x054B',  U'\x054C',  U'\x054D',  U'\x054E',  U'\x054F',
    U'\x0550',  U'\x0551',  U'\x0552',  U'\x0553',  U'\x0554',  U'\x0555',  U'\x0556',  U'\x10A0',
    U'\x10A1',  U'\x10A2',  U'\x10A3',  U'\x10A4',  U'\x10A5',  U'\x10A6',  U'\x10A7',  U'\x10A8',
    U'\x10A9',  U'\x10AA',  U'\x10AB',  U'\x10AC',  U'\x10AD',  U'\x10AE',  U'\x10AF',  U'\x10B0',
    U'\x10B1',  U'\x10B2',  U'\x10B3',  U'\x10B4',  U'\x10B5',  U'\x10B6',  U'\x10B7',  U'\x10B8',
    U'\x10B9',  U'\x10BA',  U'\x10BB',  U'\x10BC',  U'\x10BD',  U'\x10BE',  U'\x10BF',  U'\x10C0',
    U'\x10C1',  U'\x10C2',  U'\x10C3',  U'\x10C4',  U'\x10C5',  U'\x10C7',  U'\x10CD',  U'\x13F8',
    U'\x13F9',  U'\x13FA',  U'\x13FB',  U'\x13FC',  U'\x13FD',  U'\x1C80',  U'\x1C81',  U'\x1C82',
    U'\x1C83',  U'\x1C84',  U'\x1C85',  U'\x1C86',  U'\x1C87',  U'\x1C88',  U'\x1C90',  U'\x1C91',
    U'\x1C92',  U'\x1C93',  U'\x1C94',  U'\x1C95',  U'\x1C96',  U'\x1C97',  U'\x1C98',  U'\x1C99',
    U'\x1C9A',  U'\x1C9B',  U'\x1C9C',  U'\x1C9D',  U'\x1C9E',  U'\x1C9F',  U'\x1CA0',  U'\x1CA1',
    U'\x1CA2',  U'\x1CA3',  U'\x1CA4',  U'\x1CA5',  U'\x1CA6',  U'\x1CA7',  U'\x1CA8',  U'\x1CA9',
    U'\x1CAA',  U'\x1CAB',  U'\x1CAC',  U'\x1CAD',  U'\x1CAE',  U'\x1CAF',  U'\x1CB0',  U'\x1CB1',
    U'\x1CB2',  U'\x1CB3',  U'\x1CB4',  U'\x1CB5',  U'\x1CB6',  U'\x1CB7',  U'\x1CB8',  U'\x1CB9',
    U'\x1CBA',  U'\x1CBD',  U'\x1CBE',  U'\x1CBF',  U'\x1E00',  U'\x1E02',  U'\x1E04',  U'\x1E06',
    U'\x1E08',  U'\x1E0A',  U'\x1E0C',  U'\x1E0E',  U'\x1E10',  U'\x1E12',  U'\x1E14',  U'\x1E16',
    U'\x1E18',  U'\x1E1A',  U'\x1E1C',  U'\x1E1E',  U'\x1E20',  U'\x1E22',  U'\x1E24',  U'\x1E26',
    U'\x1E28',  U'\x1E2A',  U'\x1E2C',  U'\x1E2E',  U'\x1E30',  U'\x1E32',  U'\x1E34',  U'\x1E36',
    U'\x1E38',  U'\x1E3A',  U'\x1E3C',  U'\x1E3E',  U'\x1E40',  U'\x1E42',  U'\x1E44',  U'\x1E46',
    U'\x1E48',  U'\x1E4A',  U'\x1E4C',  U'\x1E4E',  U'\x1E50',  U'\x1E52',  U'\x1E54',  U'\x1E56',
    U'\x1E58',  U'\x1E5A',  U'\x1E5C',  U'\x1E5E',  U'\x1E60',  U'\x1E62',  U'\x1E64',  U'\x1E66',
    U'\x1E68',  U'\x1E6A',  U'\x1E6C',  U'\x1E6E',  U'\x1E70',  U'\x1E72',  U'\x1E74',  U'\x1E76',
    U'\x1E78',  U'\x1E7A',  U'\x1E7C',  U'\x1E7E',  U'\x1E80',  U'\x1E82',  U'\x1E84',  U'\x1E86',
    U'\x1E88',  U'\x1E8A',  U'\x1E8C',  U'\x1E8E',  U'\x1E90',  U'\x1E92',  U'\x1E94',  U'\x1E9B',
    U'\x1E9E',  U'\x1EA0',  U'\x1EA2',  U'\x1EA4',  U'\x1EA6',  U'\x1EA8',  U'\x1EAA',  U'\x1EAC',
    U'\x1EAE',  U'\x1EB0',  U'\x1EB2',  U'\x1EB4',  U'\x1EB6',  U'\x1EB8',  U'\x1EBA',  U'\x1EBC',
    U'\x1EBE',  U'\x1EC0',  U'\x1EC2',  U'\x1EC4',  U'\x1EC6',  U'\x1EC8',  U'\x1ECA',  U'\x1ECC',
    U'\x1ECE',  U'\x1ED0',  U'\x1ED2',  U'\x1ED4',  U'\x1ED6',  U'\x1ED8',  U'\x1EDA',  U'\x1EDC',
    U'\x1EDE',  U'\x1EE0',  U'\x1EE2',  U'\x1EE4',  U'\x1EE6',  U'\x1EE8',  U'\x1EEA',  U'\x1EEC',
    U'\x1EEE',  U'\x1EF0',  U'\x1EF2',  U'\x1EF4',  U'\x1EF6',  U'\x1EF8',  U'\x1EFA',  U'\x1EFC',
    U'\x1EFE',  U'\x1F08',  U'\x1F09',  U'\x1F0A',  U'\x1F0B',  U'\x1F0C',  U'\x1F0D',  U'\x1F0E',
    U'\x1F0F',  U'\x1F18',  U'\x1F19',  U'\x1F1A',  U'\x1F1B',  U'\x1F1C',  U'\x1F1D',  U'\x1F28',
    U'\x1F29',  U'\x1F2A',  U'\x1F2B',  U'\x1F2C',  U'\x1F2D',  U'\x1F2E',  U'\x1F2F',  U'\x1F38',
    U'\x1F39',  U'\x1F3A',  U'\x1F3B',  U'\x1F3C',  U'\x1F3D',  U'\x1F3E',  U'\x1F3F',  U'\x1F48',
    U'\x1F49',  U'\x1F4A',  U'\x1F4B',  U'\x1F4C',  U'\x1F4D',  U'\x1F59',  U'\x1F5B',  U'\x1F5D',
    U'\x1F5F',  U'\x1F68',  U'\x1F69',  U'\x1F6A',  U'\x1F6B',  U'\x1F6C',  U'\x1F6D',  U'\x1F6E',
    U'\x1F6F',  U'\x1F88',  U'\x1F89',  U'\x1F8A',  U'\x1F8B',  U'\x1F8C',  U'\x1F8D',  U'\x1F8E',
    U'\x1F8F',  U'\x1F98',  U'\x1F99',  U'\x1F9A',  U'\x1F9B',  U'\x1F9C',  U'\x1F9D',  U'\x1F9E',
    U'\x1F9F',  U'\x1FA8',  U'\x1FA9',  U'\x1FAA',  U'\x1FAB',  U'\x1FAC',  U'\x1FAD',  U'\x1FAE',
    U'\x1FAF',  U'\x1FB8',  U'\x1FB9',  U'\x1FBA',  U'\x1FBB',  U'\x1FBC',  U'\x1FBE',  U'\x1FC8',
    U'\x1FC9',  U'\x1FCA',  U'\x1FCB',  U'\x1FCC',  U'\x1FD8',  U'\x1FD9',  U'\x1FDA',  U'\x1FDB',
    U'\x1FE8',  U'\x1FE9',  U'\x1FEA',  U'\x1FEB',  U'\x1FEC',  U'\x1FF8',  U'\x1FF9',  U'\x1FFA',
    U'\x1FFB',  U'\x1FFC',  U'\x2126',  U'\x212A',  U'\x212B',  U'\x2132',  U'\x2160',  U'\x2161',
    U'\x2162',  U'\x2163',  U'\x2164',  U'\x2165',  U'\x2166',  U'\x2167',  U'\x2168',  U'\x2169',
    U'\x216A',  U'\x216B',  U'\x216C',  U'\x216D',  U'\x216E',  U'\x216F',  U'\x2183',  U'\x24B6',
    U'\x24B7',  U'\x24B8',  U'\x24B9',  U'\x24BA',  U'\x24BB',  U'\x24BC',  U'\x24BD',  U'\x24BE',
    U'\x24BF',  U'\x24C0',  U'\x24C1',  U'\x24C2',  U'\x24C3',  U'\x24C4',  U'\x24C5',  U'\x24C6',
    U'\x24C7',  U'\x24C8',  U'\x24C9',  U'\x24CA',  U'\x24CB',  U'\x24CC',  U'\x24CD',  U'\x24CE',
    U'\x24CF',  U'\x2C00',  U'\x2C01',  U'\x2C02',  U'\x2C03',  U'\x2C04',  U'\x2C05',  U'\x2C06',
    U'\x2C07',  U'\x2C08',  U'\x2C09',  U'\x2C0A',  U'\x2C0B',  U'\x2C0C',  U'\x2C0D',  U'\x2C0E',
    U'\x2C0F',  U'\x2C10',  U'\x2C11',  U'\x2C12',  U'\x2C13',  U'\x2C14',  U'\x2C15',  U'\x2C16',
    U'\x2C17',  U'\x2C18',  U'\x2C19',  U'\x2C1A',  U'\x2C1B',  U'\x2C1C',  U'\x2C1D',  U'\x2C1E',
    U'\x2C1F',  U'\x2C20',  U'\x2C21',  U'\x2C22',  U'\x2C23',  U'\x2C24',  U'\x2C25',  U'\x2C26',
    U'\x2C27',  U'\x2C28',  U'\x2C29',  U'\x2C2A',  U'\x2C2B',  U'\x2C2C',  U'\x2C2D',  U'\x2C2E',
    U'\x2C2F',  U'\x2C60',  U'\x2C62',  U'\x2C63',  U'\x2C64',  U'\x2C67',  U'\x2C69',  U'\x2C6B',
    U'\x2C6D',  U'\x2C6E',  U'\x2C6F',  U'\x2C70',  U'\x2C72',  U'\x2C75',  U'\x2C7E',  U'\x2C7F',
    U'\x2C80',  U'\x2C82',  U'\x2C84',  U'\x2C86',  U'\x2C88',  U'\x2C8A',  U'\x2C8C',  U'\x2C8E',
    U'\x2C90',  U'\x2C92',  U'\x2C94',  U'\x2C96',  U'\x2C98',  U'\x2C9A',  U'\x2C9C',  U'\x2C9E',
    U'\x2CA0',  U'\x2CA2',  U'\x2CA4',  U'\x2CA6',  U'\x2CA8',  U'\x2CAA',  U'\x2CAC',  U'\x2CAE',
    U'\x2CB0',  U'\x2CB2',  U'\x2CB4',  U'\x2CB6',  U'\x2CB8',  U'\x2CBA',  U'\x2CBC',  U'\x2CBE',
    U'\x2CC0',  U'\x2CC2',  U'\x2CC4',  U'\x2CC6',  U'\x2CC8',  U'\x2CCA',  U'\x2CCC',  U'\x2CCE',
    U'\x2CD0',  U'\x2CD2',  U'\x2CD4',  U'\x2CD6',  U'\x2CD8',  U'\x2CDA',  U'\x2CDC',  U'\x2CDE',
    U'\x2CE0',  U'\x2CE2',  U'\x2CEB',  U'\x2CED',  U'\x2CF2',  U'\xA640',  U'\xA642',  U'\xA644',
    U'\xA646',  U'\xA648',  U'\xA64A',  U'\xA64C',  U'\xA64E',  U'\xA650',  U'\xA652',  U'\xA654',
    U'\xA656',  U'\xA658',  U'\xA65A',  U'\xA65C',  U'\xA65E',  U'\xA660',  U'\xA662',  U'\xA664',
    U'\xA666',  U'\xA668',  U'\xA66A',  U'\xA66C',  U'\xA680',  U'\xA682',  U'\xA684',  U'\xA686',
    U'\xA688',  U'\xA68A',  U'\xA68C',  U'\xA68E',  U'\xA690',  U'\xA692',  U'\xA694',  U'\xA696',
    U'\xA698',  U'\xA69A',  U'\xA722',  U'\xA724',  U'\xA726',  U'\xA728',  U'\xA72A',  U'\xA72C',
    U'\xA72E',  U'\xA732',  U'\xA734',  U'\xA736',  U'\xA738',  U'\xA73A',  U'\xA73C',  U'\xA73E',
    U'\xA740',  U'\xA742',  U'\xA744',  U'\xA746',  U'\xA748',  U'\xA74A',  U'\xA74C',  U'\xA74E',
    U'\xA750',  U'\xA752',  U'\xA754',  U'\xA756',  U'\xA758',  U'\xA75A',  U'\xA75C',  U'\xA75E',
    U'\xA760',  U'\xA762',  U'\xA764',  U'\xA766',  U'\xA768',  U'\xA76A',  U'\xA76C',  U'\xA76E',
    U'\xA779',  U'\xA77B',  U'\xA77D',  U'\xA77E',  U'\xA780',  U'\xA782',  U'\xA784',  U'\xA786',
    U'\xA78B',  U'\xA78D',  U'\xA790',  U'\xA792',  U'\xA796',  U'\xA798',  U'\xA79A',  U'\xA79C',
    U'\xA79E',  U'\xA7A0',  U'\xA7A2',  U'\xA7A4',  U'\xA7A6',  U'\xA7A8',  U'\xA7AA',  U'\xA7AB',
    U'\xA7AC',  U'\xA7AD',  U'\xA7AE',  U'\xA7B0',  U'\xA7B1',  U'\xA7B2',  U'\xA7B3',  U'\xA7B4',
    U'\xA7B6',  U'\xA7B8',  U'\xA7BA',  U'\xA7BC',  U'\xA7BE',  U'\xA7C0',  U'\xA7C2',  U'\xA7C4',
    U'\xA7C5',  U'\xA7C6',  U'\xA7C7',  U'\xA7C9',  U'\xA7D0',  U'\xA7D6',  U'\xA7D8',  U'\xA7F5',
    U'\xAB70',  U'\xAB71',  U'\xAB72',  U'\xAB73',  U'\xAB74',  U'\xAB75',  U'\xAB76',  U'\xAB77',
    U'\xAB78',  U'\xAB79',  U'\xAB7A',  U'\xAB7B',  U'\xAB7C',  U'\xAB7D',  U'\xAB7E',  U'\xAB7F',
    U'\xAB80',  U'\xAB81',  U'\xAB82',  U'\xAB83',  U'\xAB84',  U'\xAB85',  U'\xAB86',  U'\xAB87',
    U'\xAB88',  U'\xAB89',  U'\xAB8A',  U'\xAB8B',  U'\xAB8C',  U'\xAB8D',  U'\xAB8E',  U'\xAB8F',
    U'\xAB90',  U'\xAB91',  U'\xAB92',  U'\xAB93',  U'\xAB94',  U'\xAB95',  U'\xAB96',  U'\xAB97',
    U'\xAB98',  U'\xAB99',  U'\xAB9A',  U'\xAB9B',  U'\xAB9C',  U'\xAB9D',  U'\xAB9E',  U'\xAB9F',
    U'\xABA0',  U'\xABA1',  U'\xABA2',  U'\xABA3',  U'\xABA4',  U'\xABA5',  U'\xABA6',  U'\xABA7',
    U'\xABA8',  U'\xABA9',  U'\xABAA',  U'\xABAB',  U'\xABAC',  U'\xABAD',  U'\xABAE',  U'\xABAF',
    U'\xABB0',  U'\xABB1',  U'\xABB2',  U'\xABB3',  U'\xABB4',  U'\xABB5',  U'\xABB6',  U'\xABB7',
    U'\xABB8',  U'\xABB9',  U'\xABBA',  U'\xABBB',  U'\xABBC',  U'\xABBD',  U'\xABBE',  U'\xABBF',
    U'\xFF21',  U'\xFF22',  U'\xFF23',  U'\xFF24',  U'\xFF25',  U'\xFF26',  U'\xFF27',  U'\xFF28',
    U'\xFF29',  U'\xFF2A',  U'\xFF2B',  U'\xFF2C',  U'\xFF2D',  U'\xFF2E',  U'\xFF2F',  U'\xFF30',
    U'\xFF31',  U'\xFF32',  U'\xFF33',  U'\xFF34',  U'\xFF35',  U'\xFF36',  U'\xFF37',  U'\xFF38',
    U'\xFF39',  U'\xFF3A',  U'\x10400', U'\x10401', U'\x10402', U'\x10403', U'\x10404', U'\x10405',
    U'\x10406', U'\x10407', U'\x10408', U'\x10409', U'\x1040A', U'\x1040B', U'\x1040C', U'\x1040D',
    U'\x1040E', U'\x1040F', U'\x10410', U'\x10411', U'\x10412', U'\x10413', U'\x10414', U'\x10415',
    U'\x10416', U'\x10417', U'\x10418', U'\x10419', U'\x1041A', U'\x1041B', U'\x1041C', U'\x1041D',
    U'\x1041E', U'\x1041F', U'\x10420', U'\x10421', U'\x10422', U'\x10423', U'\x10424', U'\x10425',
    U'\x10426', U'\x10427', U'\x104B0', U'\x104B1', U'\x104B2', U'\x104B3', U'\x104B4', U'\x104B5',
    U'\x104B6', U'\x104B7', U'\x104B8', U'\x104B9', U'\x104BA', U'\x104BB', U'\x104BC', U'\x104BD',
    U'\x104BE', U'\x104BF', U'\x104C0', U'\x104C1', U'\x104C2', U'\x104C3', U'\x104C4', U'\x104C5',
    U'\x104C6', U'\x104C7', U'\x104C8', U'\x104C9', U'\x104CA', U'\x104CB', U'\x104CC', U'\x104CD',
    U'\x104CE', U'\x104CF', U'\x104D0', U'\x104D1', U'\x104D2', U'\x104D3', U'\x10570', U'\x10571',
    U'\x10572', U'\x10573', U'\x10574', U'\x10575', U'\x10576', U'\x10577', U'\x10578', U'\x10579',
    U'\x1057A', U'\x1057C', U'\x1057D', U'\x1057E', U'\x1057F', U'\x10580', U'\x10581', U'\x10582',
    U'\x10583', U'\x10584', U'\x10585', U'\x10586', U'\x10587', U'\x10588', U'\x10589', U'\x1058A',
    U'\x1058C', U'\x1058D', U'\x1058E', U'\x1058F', U'\x10590', U'\x10591', U'\x10592', U'\x10594',
    U'\x10595', U'\x10C80', U'\x10C81', U'\x10C82', U'\x10C83', U'\x10C84', U'\x10C85', U'\x10C86',
    U'\x10C87', U'\x10C88', U'\x10C89', U'\x10C8A', U'\x10C8B', U'\x10C8C', U'\x10C8D', U'\x10C8E',
    U'\x10C8F', U'\x10C90', U'\x10C91', U'\x10C92', U'\x10C93', U'\x10C94', U'\x10C95', U'\x10C96',
    U'\x10C97', U'\x10C98', U'\x10C99', U'\x10C9A', U'\x10C9B', U'\x10C9C', U'\x10C9D', U'\x10C9E',
    U'\x10C9F', U'\x10CA0', U'\x10CA1', U'\x10CA2', U'\x10CA3', U'\x10CA4', U'\x10CA5', U'\x10CA6',
    U'\x10CA7', U'\x10CA8', U'\x10CA9', U'\x10CAA', U'\x10CAB', U'\x10CAC', U'\x10CAD', U'\x10CAE',
    U'\x10CAF', U'\x10CB0', U'\x10CB1', U'\x10CB2', U'\x118A0', U'\x118A1', U'\x118A2', U'\x118A3',
    U'\x118A4', U'\x118A5', U'\x118A6', U'\x118A7', U'\x118A8', U'\x118A9', U'\x118AA', U'\x118AB',
    U'\x118AC', U'\x118AD', U'\x118AE', U'\x118AF', U'\x118B0', U'\x118B1', U'\x118B2', U'\x118B3',
    U'\x118B4', U'\x118B5', U'\x118B6', U'\x118B7', U'\x118B8', U'\x118B9', U'\x118BA', U'\x118BB',
    U'\x118BC', U'\x118BD', U'\x118BE', U'\x118BF', U'\x16E40', U'\x16E41', U'\x16E42', U'\x16E43',
    U'\x16E44', U'\x16E45', U'\x16E46', U'\x16E47', U'\x16E48', U'\x16E49', U'\x16E4A', U'\x16E4B',
    U'\x16E4C', U'\x16E4D', U'\x16E4E', U'\x16E4F', U'\x16E50', U'\x16E51', U'\x16E52', U'\x16E53',
    U'\x16E54', U'\x16E55', U'\x16E56', U'\x16E57', U'\x16E58', U'\x16E59', U'\x16E5A', U'\x16E5B',
    U'\x16E5C', U'\x16E5D', U'\x16E5E', U'\x16E5F', U'\x1E900', U'\x1E901', U'\x1E902', U'\x1E903',
    U'\x1E904', U'\x1E905', U'\x1E906', U'\x1E907', U'\x1E908', U'\x1E909', U'\x1E90A', U'\x1E90B',
    U'\x1E90C', U'\x1E90D', U'\x1E90E', U'\x1E90F', U'\x1E910', U'\x1E911', U'\x1E912', U'\x1E913',
    U'\x1E914', U'\x1E915', U'\x1E916', U'\x1E917', U'\x1E918', U'\x1E919', U'\x1E91A', U'\x1E91B',
    U'\x1E91C', U'\x1E91D', U'\x1E91E', U'\x1E91F', U'\x1E920', U'\x1E921'};

const char32_t UnicodeFoldLowerTable[]{
    U'\x0061',  U'\x0062',  U'\x0063',  U'\x0064',  U'\x0065',  U'\x0066',  U'\x0067',  U'\x0068',
    U'\x0069',  U'\x006A',  U'\x006B',  U'\x006C',  U'\x006D',  U'\x006E',  U'\x006F',  U'\x0070',
    U'\x0071',  U'\x0072',  U'\x0073',  U'\x0074',  U'\x0075',  U'\x0076',  U'\x0077',  U'\x0078',
    U'\x0079',  U'\x007A',  U'\x03BC',  U'\x00E0',  U'\x00E1',  U'\x00E2',  U'\x00E3',  U'\x00E4',
    U'\x00E5',  U'\x00E6',  U'\x00E7',  U'\x00E8',  U'\x00E9',  U'\x00EA',  U'\x00EB',  U'\x00EC',
    U'\x00ED',  U'\x00EE',  U'\x00EF',  U'\x00F0',  U'\x00F1',  U'\x00F2',  U'\x00F3',  U'\x00F4',
    U'\x00F5',  U'\x00F6',  U'\x00F8',  U'\x00F9',  U'\x00FA',  U'\x00FB',  U'\x00FC',  U'\x00FD',
    U'\x00FE',  U'\x0101',  U'\x0103',  U'\x0105',  U'\x0107',  U'\x0109',  U'\x010B',  U'\x010D',
    U'\x010F',  U'\x0111',  U'\x0113',  U'\x0115',  U'\x0117',  U'\x0119',  U'\x011B',  U'\x011D',
    U'\x011F',  U'\x0121',  U'\x0123',  U'\x0125',  U'\x0127',  U'\x0129',  U'\x012B',  U'\x012D',
    U'\x012F',  U'\x0133',  U'\x0135',  U'\x0137',  U'\x013A',  U'\x013C',  U'\x013E',  U'\x0140',
    U'\x0142',  U'\x0144',  U'\x0146',  U'\x0148',  U'\x014B',  U'\x014D',  U'\x014F',  U'\x0151',
    U'\x0153',  U'\x0155',  U'\x0157',  U'\x0159',  U'\x015B',  U'\x015D',  U'\x015F',  U'\x0161',
    U'\x0163',  U'\x0165',  U'\x0167',  U'\x0169',  U'\x016B',  U'\x016D',  U'\x016F',  U'\x0171',
    U'\x0173',  U'\x0175',  U'\x0177',  U'\x00FF',  U'\x017A',  U'\x017C',  U'\x017E',  U'\x0073',
    U'\x0253',  U'\x0183',  U'\x0185',  U'\x0254',  U'\x0188',  U'\x0256',  U'\x0257',  U'\x018C',
    U'\x01DD',  U'\x0259',  U'\x025B',  U'\x0192',  U'\x0260',  U'\x0263',  U'\x0269',  U'\x0268',
    U'\x0199',  U'\x026F',  U'\x0272',  U'\x0275',  U'\x01A1',  U'\x01A3',  U'\x01A5',  U'\x0280',
    U'\x01A8',  U'\x0283',  U'\x01AD',  U'\x0288',  U'\x01B0',  U'\x028A',  U'\x028B',  U'\x01B4',
    U'\x01B6',  U'\x0292',  U'\x01B9',  U'\x01BD',  U'\x01C6',  U'\x01C6',  U'\x01C9',  U'\x01C9',
    U'\x01CC',  U'\x01CC',  U'\x01CE',  U'\x01D0',  U'\x01D2',  U'\x01D4',  U'\x01D6',  U'\x01D8',
    U'\x01DA',  U'\x01DC',  U'\x01DF',  U'\x01E1',  U'\x01E3',  U'\x01E5',  U'\x01E7',  U'\x01E9',
    U'\x01EB',  U'\x01ED',  U'\x01EF',  U'\x01F3',  U'\x01F3',  U'\x01F5',  U'\x0195',  U'\x01BF',
    U'\x01F9',  U'\x01FB',  U'\x01FD',  U'\x01FF',  U'\x0201',  U'\x0203',  U'\x0205',  U'\x0207',
    U'\x0209',  U'\x020B',  U'\x020D',  U'\x020F',  U'\x0211',  U'\x0213',  U'\x0215',  U'\x0217',
    U'\x0219',  U'\x021B',  U'\x021D',  U'\x021F',  U'\x019E',  U'\x0223',  U'\x0225',  U'\x0227',
    U'\x0229',  U'\x022B',  U'\x022D',  U'\x022F',  U'\x0231',  U'\x0233',  U'\x2C65',  U'\x023C',
    U'\x019A',  U'\x2C66',  U'\x0242',  U'\x0180',  U'\x0289',  U'\x028C',  U'\x0247',  U'\x0249',
    U'\x024B',  U'\x024D',  U'\x024F',  U'\x03B9',  U'\x0371',  U'\x0373',  U'\x0377',  U'\x03F3',
    U'\x03AC',  U'\x03AD',  U'\x03AE',  U'\x03AF',  U'\x03CC',  U'\x03CD',  U'\x03CE',  U'\x03B1',
    U'\x03B2',  U'\x03B3',  U'\x03B4',  U'\x03B5',  U'\x03B6',  U'\x03B7',  U'\x03B8',  U'\x03B9',
    U'\x03BA',  U'\x03BB',  U'\x03BC',  U'\x03BD',  U'\x03BE',  U'\x03BF',  U'\x03C0',  U'\x03C1',
    U'\x03C3',  U'\x03C4',  U'\x03C5',  U'\x03C6',  U'\x03C7',  U'\x03C8',  U'\x03C9',  U'\x03CA',
    U'\x03CB',  U'\x03C3',  U'\x03D7',  U'\x03B2',  U'\x03B8',  U'\x03C6',  U'\x03C0',  U'\x03D9',
    U'\x03DB',  U'\x03DD',  U'\x03DF',  U'\x03E1',  U'\x03E3',  U'\x03E5',  U'\x03E7',  U'\x03E9',
    U'\x03EB',  U'\x03ED',  U'\x03EF',  U'\x03BA',  U'\x03C1',  U'\x03B8',  U'\x03B5',  U'\x03F8',
    U'\x03F2',  U'\x03FB',  U'\x037B',  U'\x037C',  U'\x037D',  U'\x0450',  U'\x0451',  U'\x0452',
    U'\x0453',  U'\x0454',  U'\x0455',  U'\x0456',  U'\x0457',  U'\x0458',  U'\x0459',  U'\x045A',
    U'\x045B',  U'\x045C',  U'\x045D',  U'\x045E',  U'\x045F',  U'\x0430',  U'\x0431',  U'\x0432',
    U'\x0433',  U'\x0434',  U'\x0435',  U'\x0436',  U'\x0437',  U'\x0438',  U'\x0439',  U'\x043A',
    U'\x043B',  U'\x043C',  U'\x043D',  U'\x043E',  U'\x043F',  U'\x0440',  U'\x0441',  U'\x0442',
    U'\x0443',  U'\x0444',  U'\x0445',  U'\x0446',  U'\x0447',  U'\x0448',  U'\x0449',  U'\x044A',
    U'\x044B',  U'\x044C',  U'\x044D',  U'\x044E',  U'\x044F',  U'\x0461',  U'\x0463',  U'\x0465',
    U'\x0467',  U'\x0469',  U'\x046B',  U'\x046D',  U'\x046F',  U'\x0471',  U'\x0473',  U'\x0475',
    U'\x0477',  U'\x0479',  U'\x047B',  U'\x047D',  U'\x047F',  U'\x0481',  U'\x048B',  U'\x048D',
    U'\x048F',  U'\x0491',  U'\x0493',  U'\x0495',  U'\x0497',  U'\x0499',  U'\x049B',  U'\x049D',
    U'\x049F',  U'\x04A1',  U'\x04A3',  U'\x04A5',  U'\x04A7',  U'\x04A9',  U'\x04AB',  U'\x04AD',
    U'\x04AF',  U'\x04B1',  U'\x04B3',  U'\x04B5',  U'\x04B7',  U'\x04B9',  U'\x04BB',  U'\x04BD',
    U'\x04BF',  U'\x04CF',  U'\x04C2',  U'\x04C4',  U'\x04C6',  U'\x04C8',  U'\x04CA',  U'\x04CC',
    U'\x04CE',  U'\x04D1',  U'\x04D3',  U'\x04D5',  U'\x04D7',  U'\x04D9',  U'\x04DB',  U'\x04DD',
    U'\x04DF',  U'\x04E1',  U'\x04E3',  U'\x04E5',  U'\x04E7',  U'\x04E9',  U'\x04EB',  U'\x04ED',
    U'\x04EF',  U'\x04F1',  U'\x04F3',  U'\x04F5',  U'\x04F7',  U'\x04F9',  U'\x04FB',  U'\x04FD',
    U'\x04FF',  U'\x0501',  U'\x0503',  U'\x0505',  U'\x0507',  U'\x0509',  U'\x050B',  U'\x050D',
    U'\x050F',  U'\x0511',  U'\x0513',  U'\x0515',  U'\x0517',  U'\x0519',  U'\x051B',  U'\x051D',
    U'\x051F',  U'\x0521',  U'\x0523',  U'\x0525',  U'\x0527',  U'\x0529',  U'\x052B',  U'\x052D',
    U'\x052F',  U'\x0561',  U'\x0562',  U'\x0563',  U'\x0564',  U'\x0565',  U'\x0566',  U'\x0567',
    U'\x0568',  U'\x0569',  U'\x056A',  U'\x056B',  U'\x056C',  U'\x056D',  U'\x056E',  U'\x056F',
    U'\x0570',  U'\x0571',  U'\x0572',  U'\x0573',  U'\x0574',  U'\x0575',  U'\x0576',  U'\x0577',
    U'\x0578',  U'\x0579',  U'\x057A',  U'\x057B',  U'\x057C',  U'\x057D',  U'\x057E',  U'\x057F',
    U'\x0580',  U'\x0581',  U'\x0582',  U'\x0583',  U'\x0584',  U'\x0585',  U'\x0586',  U'\x2D00',
    U'\x2D01',  U'\x2D02',  U'\x2D03',  U'\x2D04',  U'\x2D05',  U'\x2D06',  U'\x2D07',  U'\x2D08',
    U'\x2D09',  U'\x2D0A',  U'\x2D0B',  U'\x2D0C',  U'\x2D0D',  U'\x2D0E',  U'\x2D0F',  U'\x2D10',
    U'\x2D11',  U'\x2D12',  U'\x2D13',  U'\x2D14',  U'\x2D15',  U'\x2D16',  U'\x2D17',  U'\x2D18',
    U'\x2D19',  U'\x2D1A',  U'\x2D1B',  U'\x2D1C',  U'\x2D1D',  U'\x2D1E',  U'\x2D1F',  U'\x2D20',
    U'\x2D21',  U'\x2D22',  U'\x2D23',  U'\x2D24',  U'\x2D25',  U'\x2D27',  U'\x2D2D',  U'\x13F0',
    U'\x13F1',  U'\x13F2',  U'\x13F3',  U'\x13F4',  U'\x13F5',  U'\x0432',  U'\x0434',  U'\x043E',
    U'\x0441',  U'\x0442',  U'\x0442',  U'\x044A',  U'\x0463',  U'\xA64B',  U'\x10D0',  U'\x10D1',
    U'\x10D2',  U'\x10D3',  U'\x10D4',  U'\x10D5',  U'\x10D6',  U'\x10D7',  U'\x10D8',  U'\x10D9',
    U'\x10DA',  U'\x10DB',  U'\x10DC',  U'\x10DD',  U'\x10DE',  U'\x10DF',  U'\x10E0',  U'\x10E1',
    U'\x10E2',  U'\x10E3',  U'\x10E4',  U'\x10E5',  U'\x10E6',  U'\x10E7',  U'\x10E8',  U'\x10E9',
    U'\x10EA',  U'\x10EB',  U'\x10EC',  U'\x10ED',  U'\x10EE',  U'\x10EF',  U'\x10F0',  U'\x10F1',
    U'\x10F2',  U'\x10F3',  U'\x10F4',  U'\x10F5',  U'\x10F6',  U'\x10F7',  U'\x10F8',  U'\x10F9',
    U'\x10FA',  U'\x10FD',  U'\x10FE',  U'\x10FF',  U'\x1E01',  U'\x1E03',  U'\x1E05',  U'\x1E07',
    U'\x1E09',  U'\x1E0B',  U'\x1E0D',  U'\x1E0F',  U'\x1E11',  U'\x1E13',  U'\x1E15',  U'\x1E17',
    U'\x1E19',  U'\x1E1B',  U'\x1E1D',  U'\x1E1F',  U'\x1E21',  U'\x1E23',  U'\x1E25',  U'\x1E27',
    U'\x1E29',  U'\x1E2B',  U'\x1E2D',  U'\x1E2F',  U'\x1E31',  U'\x1E33',  U'\x1E35',  U'\x1E37',
    U'\x1E39',  U'\x1E3B',  U'\x1E3D',  U'\x1E3F',  U'\x1E41',  U'\x1E43',  U'\x1E45',  U'\x1E47',
    U'\x1E49',  U'\x1E4B',  U'\x1E4D',  U'\x1E4F',  U'\x1E51',  U'\x1E53',  U'\x1E55',  U'\x1E57',
    U'\x1E59',  U'\x1E5B',  U'\x1E5D',  U'\x1E5F',  U'\x1E61',  U'\x1E63',  U'\x1E65',  U'\x1E67',
    U'\x1E69',  U'\x1E6B',  U'\x1E6D',  U'\x1E6F',  U'\x1E71',  U'\x1E73',  U'\x1E75',  U'\x1E77',
    U'\x1E79',  U'\x1E7B',  U'\x1E7D',  U'\x1E7F',  U'\x1E81',  U'\x1E83',  U'\x1E85',  U'\x1E87',
    U'\x1E89',  U'\x1E8B',  U'\x1E8D',  U'\x1E8F',  U'\x1E91',  U'\x1E93',  U'\x1E95',  U'\x1E61',
    U'\x00DF',  U'\x1EA1',  U'\x1EA3',  U'\x1EA5',  U'\x1EA7',  U'\x1EA9',  U'\x1EAB',  U'\x1EAD',
    U'\x1EAF',  U'\x1EB1',  U'\x1EB3',  U'\x1EB5',  U'\x1EB7',  U'\x1EB9',  U'\x1EBB',  U'\x1EBD',
    U'\x1EBF',  U'\x1EC1',  U'\x1EC3',  U'\x1EC5',  U'\x1EC7',  U'\x1EC9',  U'\x1ECB',  U'\x1ECD',
    U'\x1ECF',  U'\x1ED1',  U'\x1ED3',  U'\x1ED5',  U'\x1ED7',  U'\x1ED9',  U'\x1EDB',  U'\x1EDD',
    U'\x1EDF',  U'\x1EE1',  U'\x1EE3',  U'\x1EE5',  U'\x1EE7',  U'\x1EE9',  U'\x1EEB',  U'\x1EED',
    U'\x1EEF',  U'\x1EF1',  U'\x1EF3',  U'\x1EF5',  U'\x1EF7',  U'\x1EF9',  U'\x1EFB',  U'\x1EFD',
    U'\x1EFF',  U'\x1F00',  U'\x1F01',  U'\x1F02',  U'\x1F03',  U'\x1F04',  U'\x1F05',  U'\x1F06',
    U'\x1F07',  U'\x1F10',  U'\x1F11',  U'\x1F12',  U'\x1F13',  U'\x1F14',  U'\x1F15',  U'\x1F20',
    U'\x1F21',  U'\x1F22',  U'\x1F23',  U'\x1F24',  U'\x1F25',  U'\x1F26',  U'\x1F27',  U'\x1F30',
    U'\x1F31',  U'\x1F32',  U'\x1F33',  U'\x1F34',  U'\x1F35',  U'\x1F36',  U'\x1F37',  U'\x1F40',
    U'\x1F41',  U'\x1F42',  U'\x1F43',  U'\x1F44',  U'\x1F45',  U'\x1F51',  U'\x1F53',  U'\x1F55',
    U'\x1F57',  U'\x1F60',  U'\x1F61',  U'\x1F62',  U'\x1F63',  U'\x1F64',  U'\x1F65',  U'\x1F66',
    U'\x1F67',  U'\x1F80',  U'\x1F81',  U'\x1F82',  U'\x1F83',  U'\x1F84',  U'\x1F85',  U'\x1F86',
    U'\x1F87',  U'\x1F90',  U'\x1F91',  U'\x1F92',  U'\x1F93',  U'\x1F94',  U'\x1F95',  U'\x1F96',
    U'\x1F97',  U'\x1FA0',  U'\x1FA1',  U'\x1FA2',  U'\x1FA3',  U'\x1FA4',  U'\x1FA5',  U'\x1FA6',
    U'\x1FA7',  U'\x1FB0',  U'\x1FB1',  U'\x1F70',  U'\x1F71',  U'\x1FB3',  U'\x03B9',  U'\x1F72',
    U'\x1F73',  U'\x1F74',  U'\x1F75',  U'\x1FC3',  U'\x1FD0',  U'\x1FD1',  U'\x1F76',  U'\x1F77',
    U'\x1FE0',  U'\x1FE1',  U'\x1F7A',  U'\x1F7B',  U'\x1FE5',  U'\x1F78',  U'\x1F79',  U'\x1F7C',
    U'\x1F7D',  U'\x1FF3',  U'\x03C9',  U'\x006B',  U'\x00E5',  U'\x214E',  U'\x2170',  U'\x2171',
    U'\x2172',  U'\x2173',  U'\x2174',  U'\x2175',  U'\x2176',  U'\x2177',  U'\x2178',  U'\x2179',
    U'\x217A',  U'\x217B',  U'\x217C',  U'\x217D',  U'\x217E',  U'\x217F',  U'\x2184',  U'\x24D0',
    U'\x24D1',  U'\x24D2',  U'\x24D3',  U'\x24D4',  U'\x24D5',  U'\x24D6',  U'\x24D7',  U'\x24D8',
    U'\x24D9',  U'\x24DA',  U'\x24DB',  U'\x24DC',  U'\x24DD',  U'\x24DE',  U'\x24DF',  U'\x24E0',
    U'\x24E1',  U'\x24E2',  U'\x24E3',  U'\x24E4',  U'\x24E5',  U'\x24E6',  U'\x24E7',  U'\x24E8',
    U'\x24E9',  U'\x2C30',  U'\x2C31',  U'\x2C32',  U'\x2C33',  U'\x2C34',  U'\x2C35',  U'\x2C36',
    U'\x2C37',  U'\x2C38',  U'\x2C39',  U'\x2C3A',  U'\x2C3B',  U'\x2C3C',  U'\x2C3D',  U'\x2C3E',
    U'\x2C3F',  U'\x2C40',  U'\x2C41',  U'\x2C42',  U'\x2C43',  U'\x2C44',  U'\x2C45',  U'\x2C46',
    U'\x2C47',  U'\x2C48',  U'\x2C49',  U'\x2C4A',  U'\x2C4B',  U'\x2C4C',  U'\x2C4D',  U'\x2C4E',
    U'\x2C4F',  U'\x2C50',  U'\x2C51',  U'\x2C52',  U'\x2C53',  U'\x2C54',  U'\x2C55',  U'\x2C56',
    U'\x2C57',  U'\x2C58',  U'\x2C59',  U'\x2C5A',  U'\x2C5B',  U'\x2C5C',  U'\x2C5D',  U'\x2C5E',
    U'\x2C5F',  U'\x2C61',  U'\x026B',  U'\x1D7D',  U'\x027D',  U'\x2C68',  U'\x2C6A',  U'\x2C6C',
    U'\x0251',  U'\x0271',  U'\x0250',  U'\x0252',  U'\x2C73',  U'\x2C76',  U'\x023F',  U'\x0240',
    U'\x2C81',  U'\x2C83',  U'\x2C85',  U'\x2C87',  U'\x2C89',  U'\x2C8B',  U'\x2C8D',  U'\x2C8F',
    U'\x2C91',  U'\x2C93',  U'\x2C95',  U'\x2C97',  U'\x2C99',  U'\x2C9B',  U'\x2C9D',  U'\x2C9F',
    U'\x2CA1',  U'\x2CA3',  U'\x2CA5',  U'\x2CA7',  U'\x2CA9',  U'\x2CAB',  U'\x2CAD',  U'\x2CAF',
    U'\x2CB1',  U'\x2CB3',  U'\x2CB5',  U'\x2CB7',  U'\x2CB9',  U'\x2CBB',  U'\x2CBD',  U'\x2CBF',
    U'\x2CC1',  U'\x2CC3',  U'\x2CC5',  U'\x2CC7',  U'\x2CC9',  U'\x2CCB',  U'\x2CCD',  U'\x2CCF',
    U'\x2CD1',  U'\x2CD3',  U'\x2CD5',  U'\x2CD7',  U'\x2CD9',  U'\x2CDB',  U'\x2CDD',  U'\x2CDF',
    U'\x2CE1',  U'\x2CE3',  U'\x2CEC',  U'\x2CEE',  U'\x2CF3',  U'\xA641',  U'\xA643',  U'\xA645',
    U'\xA647',  U'\xA649',  U'\xA64B',  U'\xA64D',  U'\xA64F',  U'\xA651',  U'\xA653',  U'\xA655',
    U'\xA657',  U'\xA659',  U'\xA65B',  U'\xA65D',  U'\xA65F',  U'\xA661',  U'\xA663',  U'\xA665',
    U'\xA667',  U'\xA669',  U'\xA66B',  U'\xA66D',  U'\xA681',  U'\xA683',  U'\xA685',  U'\xA687',
    U'\xA689',  U'\xA68B',  U'\xA68D',  U'\xA68F',  U'\xA691',  U'\xA693',  U'\xA695',  U'\xA697',
    U'\xA699',  U'\xA69B',  U'\xA723',  U'\xA725',  U'\xA727',  U'\xA729',  U'\xA72B',  U'\xA72D',
    U'\xA72F',  U'\xA733',  U'\xA735',  U'\xA737',  U'\xA739',  U'\xA73B',  U'\xA73D',  U'\xA73F',
    U'\xA741',  U'\xA743',  U'\xA745',  U'\xA747',  U'\xA749',  U'\xA74B',  U'\xA74D',  U'\xA74F',
    U'\xA751',  U'\xA753',  U'\xA755',  U'\xA757',  U'\xA759',  U'\xA75B',  U'\xA75D',  U'\xA75F',
    U'\xA761',  U'\xA763',  U'\xA765',  U'\xA767',  U'\xA769',  U'\xA76B',  U'\xA76D',  U'\xA76F',
    U'\xA77A',  U'\xA77C',  U'\x1D79',  U'\xA77F',  U'\xA781',  U'\xA783',  U'\xA785',  U'\xA787',
    U'\xA78C',  U'\x0265',  U'\xA791',  U'\xA793',  U'\xA797',  U'\xA799',  U'\xA79B',  U'\xA79D',
    U'\xA79F',  U'\xA7A1',  U'\xA7A3',  U'\xA7A5',  U'\xA7A7',  U'\xA7A9',  U'\x0266',  U'\x025C',
    U'\x0261',  U'\x026C',  U'\x026A',  U'\x029E',  U'\x0287',  U'\x029D',  U'\xAB53',  U'\xA7B5',
    U'\xA7B7',  U'\xA7B9',  U'\xA7BB',  U'\xA7BD',  U'\xA7BF',  U'\xA7C1',  U'\xA7C3',  U'\xA794',
    U'\x0282',  U'\x1D8E',  U'\xA7C8',  U'\xA7CA',  U'\xA7D1',  U'\xA7D7',  U'\xA7D9',  U'\xA7F6',
    U'\x13A0',  U'\x13A1',  U'\x13A2',  U'\x13A3',  U'\x13A4',  U'\x13A5',  U'\x13A6',  U'\x13A7',
    U'\x13A8',  U'\x13A9',  U'\x13AA',  U'\x13AB',  U'\x13AC',  U'\x13AD',  U'\x13AE',  U'\x13AF',
    U'\x13B0',  U'\x13B1',  U'\x13B2',  U'\x13B3',  U'\x13B4',  U'\x13B5',  U'\x13B6',  U'\x13B7',
    U'\x13B8',  U'\x13B9',  U'\x13BA',  U'\x13BB',  U'\x13BC',  U'\x13BD',  U'\x13BE',  U'\x13BF',
    U'\x13C0',  U'\x13C1',  U'\x13C2',  U'\x13C3',  U'\x13C4',  U'\x13C5',  U'\x13C6',  U'\x13C7',
    U'\x13C8',  U'\x13C9',  U'\x13CA',  U'\x13CB',  U'\x13CC',  U'\x13CD',  U'\x13CE',  U'\x13CF',
    U'\x13D0',  U'\x13D1',  U'\x13D2',  U'\x13D3',  U'\x13D4',  U'\x13D5',  U'\x13D6',  U'\x13D7',
    U'\x13D8',  U'\x13D9',  U'\x13DA',  U'\x13DB',  U'\x13DC',  U'\x13DD',  U'\x13DE',  U'\x13DF',
    U'\x13E0',  U'\x13E1',  U'\x13E2',  U'\x13E3',  U'\x13E4',  U'\x13E5',  U'\x13E6',  U'\x13E7',
    U'\x13E8',  U'\x13E9',  U'\x13EA',  U'\x13EB',  U'\x13EC',  U'\x13ED',  U'\x13EE',  U'\x13EF',
    U'\xFF41',  U'\xFF42',  U'\xFF43',  U'\xFF44',  U'\xFF45',  U'\xFF46',  U'\xFF47',  U'\xFF48',
    U'\xFF49',  U'\xFF4A',  U'\xFF4B',  U'\xFF4C',  U'\xFF4D',  U'\xFF4E',  U'\xFF4F',  U'\xFF50',
    U'\xFF51',  U'\xFF52',  U'\xFF53',  U'\xFF54',  U'\xFF55',  U'\xFF56',  U'\xFF57',  U'\xFF58',
    U'\xFF59',  U'\xFF5A',  U'\x10428', U'\x10429', U'\x1042A', U'\x1042B', U'\x1042C', U'\x1042D',
    U'\x1042E', U'\x1042F', U'\x10430', U'\x10431', U'\x10432', U'\x10433', U'\x10434', U'\x10435',
    U'\x10436', U'\x10437', U'\x10438', U'\x10439', U'\x1043A', U'\x1043B', U'\x1043C', U'\x1043D',
    U'\x1043E', U'\x1043F', U'\x10440', U'\x10441', U'\x10442', U'\x10443', U'\x10444', U'\x10445',
    U'\x10446', U'\x10447', U'\x10448', U'\x10449', U'\x1044A', U'\x1044B', U'\x1044C', U'\x1044D',
    U'\x1044E', U'\x1044F', U'\x104D8', U'\x104D9', U'\x104DA', U'\x104DB', U'\x104DC', U'\x104DD',
    U'\x104DE', U'\x104DF', U'\x104E0', U'\x104E1', U'\x104E2', U'\x104E3', U'\x104E4', U'\x104E5',
    U'\x104E6', U'\x104E7', U'\x104E8', U'\x104E9', U'\x104EA', U'\x104EB', U'\x104EC', U'\x104ED',
    U'\x104EE', U'\x104EF', U'\x104F0', U'\x104F1', U'\x104F2', U'\x104F3', U'\x104F4', U'\x104F5',
    U'\x104F6', U'\x104F7', U'\x104F8', U'\x104F9', U'\x104FA', U'\x104FB', U'\x10597', U'\x10598',
    U'\x10599', U'\x1059A', U'\x1059B', U'\x1059C', U'\x1059D', U'\x1059E', U'\x1059F', U'\x105A0',
    U'\x105A1', U'\x105A3', U'\x105A4', U'\x105A5', U'\x105A6', U'\x105A7', U'\x105A8', U'\x105A9',
    U'\x105AA', U'\x105AB', U'\x105AC', U'\x105AD', U'\x105AE', U'\x105AF', U'\x105B0', U'\x105B1',
    U'\x105B3', U'\x105B4', U'\x105B5', U'\x105B6', U'\x105B7', U'\x105B8', U'\x105B9', U'\x105BB',
    U'\x105BC', U'\x10CC0', U'\x10CC1', U'\x10CC2', U'\x10CC3', U'\x10CC4', U'\x10CC5', U'\x10CC6',
    U'\x10CC7', U'\x10CC8', U'\x10CC9', U'\x10CCA', U'\x10CCB', U'\x10CCC', U'\x10CCD', U'\x10CCE',
    U'\x10CCF', U'\x10CD0', U'\x10CD1', U'\x10CD2', U'\x10CD3', U'\x10CD4', U'\x10CD5', U'\x10CD6',
    U'\x10CD7', U'\x10CD8', U'\x10CD9', U'\x10CDA', U'\x10CDB', U'\x10CDC', U'\x10CDD', U'\x10CDE',
    U'\x10CDF', U'\x10CE0', U'\x10CE1', U'\x10CE2', U'\x10CE3', U'\x10CE4', U'\x10CE5', U'\x10CE6',
    U'\x10CE7', U'\x10CE8', U'\x10CE9', U'\x10CEA', U'\x10CEB', U'\x10CEC', U'\x10CED', U'\x10CEE',
    U'\x10CEF', U'\x10CF0', U'\x10CF1', U'\x10CF2', U'\x118C0', U'\x118C1', U'\x118C2', U'\x118C3',
    U'\x118C4', U'\x118C5', U'\x118C6', U'\x118C7', U'\x118C8', U'\x118C9', U'\x118CA', U'\x118CB',
    U'\x118CC', U'\x118CD', U'\x118CE', U'\x118CF', U'\x118D0', U'\x118D1', U'\x118D2', U'\x118D3',
    U'\x118D4', U'\x118D5', U'\x118D6', U'\x118D7', U'\x118D8', U'\x118D9', U'\x118DA', U'\x118DB',
    U'\x118DC', U'\x118DD', U'\x118DE', U'\x118DF', U'\x16E60', U'\x16E61', U'\x16E62', U'\x16E63',
    U'\x16E64', U'\x16E65', U'\x16E66', U'\x16E67', U'\x16E68', U'\x16E69', U'\x16E6A', U'\x16E6B',
    U'\x16E6C', U'\x16E6D', U'\x16E6E', U'\x16E6F', U'\x16E70', U'\x16E71', U'\x16E72', U'\x16E73',
    U'\x16E74', U'\x16E75', U'\x16E76', U'\x16E77', U'\x16E78', U'\x16E79', U'\x16E7A', U'\x16E7B',
    U'\x16E7C', U'\x16E7D', U'\x16E7E', U'\x16E7F', U'\x1E922', U'\x1E923', U'\x1E924', U'\x1E925',
    U'\x1E926', U'\x1E927', U'\x1E928', U'\x1E929', U'\x1E92A', U'\x1E92B', U'\x1E92C', U'\x1E92D',
    U'\x1E92E', U'\x1E92F', U'\x1E930', U'\x1E931', U'\x1E932', U'\x1E933', U'\x1E934', U'\x1E935',
    U'\x1E936', U'\x1E937', U'\x1E938', U'\x1E939', U'\x1E93A', U'\x1E93B', U'\x1E93C', U'\x1E93D',
    U'\x1E93E', U'\x1E93F', U'\x1E940', U'\x1E941', U'\x1E942', U'\x1E943'};
// clang-format off

size_t GetFoldingTableSize()
{
#ifdef TARGET_DARWIN
  size_t maxIdx = 1193;
#else
  size_t maxIdx = (sizeof(UnicodeFoldLowerTable) / sizeof(char32_t)) - 1;
#endif
  return maxIdx;
}
