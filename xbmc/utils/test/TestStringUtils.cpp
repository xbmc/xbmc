/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StringUtils.h"
#include "TestStringUtils.h"

#include <algorithm>
#include <locale>
#include <string_view>
#include <cstdint>

#include <gtest/gtest.h>

using namespace std::literals;

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
}

// ------------------------------------ NOTE ------------------------------------
//
// The ToLower, ToUpper, FoldCase and related tests were rewritten for Unicode (codepoint)
// comparision and not not for single-byte comparison (as was done prior to these
// changes). The ToLower/ToUpper API has not yet been changed, so these tests have
// been modified to pass with current behavior. A TODO note documents each of these.
//
// Also note that on Windows, ToLower and ToUpper will use UTF-16 instead of UTF-32.
// Additional testing will need to be done to correct for that difference.
//
// -----------------------------------------------------------------------------
namespace TestStringUtils {
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
//                     depending upon context.
// óóßChloë
const char UTF8_GERMAN_SAMPLE[] = {"\xc3\xb3\xc3\xb3\xc3\x9f\x43\x68\x6c\x6f\xc3\xab"};
// u"ÓÓSSCHLOË";
const char* UTF8_GERMAN_UPPER = {"\xc3\x93\xc3\x93\x53\x53\x43\x48\x4c\x4f\xc3\x8b"};
// u"óósschloë"
//const char UTF8_GERMAN_LOWER[] = {"\xc3\xb3\xc3\xb3\xc3\x9f\x63\x68\x6c\x6f\xc3\xab"};
// óóßchloë
const char* UTF8_GERMAN_LOWER_SS = {"\xc3\xb3\xc3\xb3\x73\x73\x63\x68\x6c\x6f\xc3\xab"};
// u"óósschloë";
}

TEST(TestStringUtils, Format)
{
  std::string refstr = "test 25 2.7 ff FF";

  std::string varstr =
      StringUtils::Format("{} {} {:.1f} {:x} {:02X}", "test", 25, 2.743f, 0x00ff, 0x00ff);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = StringUtils::Format("", "test", 25, 2.743f, 0x00ff, 0x00ff);
  EXPECT_STREQ("", varstr.c_str());
}

TEST(TestStringUtils, FormatEnum)
{
  const char* zero = "0";
  const char* one = "1";

  std::string varstr = StringUtils::Format("{}", ECG::A);
  EXPECT_STREQ(zero, varstr.c_str());

  varstr = StringUtils::Format("{}", EG::C);
  EXPECT_STREQ(zero, varstr.c_str());

  varstr = StringUtils::Format("{}", test_enum::ECN::A);
  EXPECT_STREQ(one, varstr.c_str());

  varstr = StringUtils::Format("{}", test_enum::EN::C);
  EXPECT_STREQ(one, varstr.c_str());
}

TEST(TestStringUtils, FormatEnumWidth)
{
  const char* one = "01";

  std::string varstr = StringUtils::Format("{:02d}", ECG::B);
  EXPECT_STREQ(one, varstr.c_str());

  varstr = StringUtils::Format("{:02}", EG::D);
  EXPECT_STREQ(one, varstr.c_str());
}

TEST(TestStringUtils, ToUpper)
{
  std::string refstr = "TEST";

  std::string varstr = "TeSt";
  StringUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, ToLower)
{
  std::string refstr = "test";

  std::string varstr = "TeSt";
  StringUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, ToCapitalize)
{
  std::string refstr = "Test";
  std::string varstr = "test";
  StringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Just A Test";
  varstr = "just a test";
  StringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "Test -1;2:3, String For Case";
  varstr = "test -1;2:3, string for Case";
  StringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "  JuST Another\t\tTEst:\nWoRKs ";
  varstr = "  juST another\t\ttEst:\nwoRKs ";
  StringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N.Y.P.D";
  varstr = "n.y.p.d";
  StringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "N-Y-P-D";
  varstr = "n-y-p-d";
  StringUtils::ToCapitalize(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Validate_FoldCase_Data)
{
  // Validate that FoldCase tables were generated properly. Walk
  // every Unicode character in the supported range.
  //
  // Case folding tables are derived from Unicode Inc.
  // Data file, CaseFolding.txt (CaseFolding-14.0.0.txt, 2021-03-08). Copyright follows below.
  //
  // These tables provide for "simple case folding" that is not locale sensitive. They do NOT
  // support "Full Case Folding" which can fold single characters into multiple, or multiple
  // into single, etc. CaseFolding.txt can be found in the ICUC4 source directory:
  // icu/source/data/unidata.
  //
  // Verify that:
  //
  //   For every character where FoldCase(c) != c that c is in
  //   UnicodeFoldUpperTable AND that FoldCase(c) is at the same index
  //   in UnicodeFoldLowerTable. ALSO, that if FoldCase(c) == c, then
  //   c is not found neither UnicodeFoldUpperTable nor UnicodeFoldLowerTable.

  size_t idx = 0;
  int iter = 0;
  int FoldsFound = 0;
  size_t maxChar = UnicodeFoldLowerTable[sizeof(UnicodeFoldLowerTable) / sizeof(char32_t) - 1];

  // Go through every possible Unicode character within the range of the values in
  // UnicodeFoldUpperTable.
  //
  // if the Unicode character == The next char in UnicodeFoldUpperTable, then
  // c != FoldCase(c). Also, UnicodeFoldLowerTable[idx] == FoldCase(c)

  for (char32_t c = 0; c < maxChar + 1; c++)
  {
    // std::cout << "idx: " << idx << " iter: " << iter << std::endl;
    iter++;

    // Convert char32_t to utf8 string

    std::u32string_view c_sv{&c, 1};
    std::string c_utf8 = StringUtils::ToUtf8(c_sv);
    std::string c_folded = StringUtils::FoldCase(c_utf8);

    // If folded, then character must be in unicode_fold_upper,
    // otherwise, it must NOT be in unicode_fold_upper

    std::u32string c_folded_utf32 = StringUtils::ToUtf32(c_folded);
    char32_t c_folded_char32 = c_folded_utf32[0];

    char32_t verify_folded_char32 = UnicodeFoldLowerTable[idx];

    std::u32string_view verify_folded_sv{&verify_folded_char32, 1};
    std::string verify_folded_utf8 = StringUtils::ToUtf8(verify_folded_sv);

    if (UnicodeFoldUpperTable[idx] == c) // Fold case equivalent in unicode_fold_lower
    {
      if (c_folded_char32 != verify_folded_char32)
      {
        FAIL() << "FoldCase returns different result than tables for 0x"
            << std::hex << c << " " << c_utf8 << " at idx " << idx
            << " folded: 0x" << std::hex << c_folded_char32 << " " << verify_folded_utf8;
        idx++;
        continue;
      }
      else
      {
        FoldsFound++;
      }
      idx++;
    }
    else if (c_folded != c_utf8)
    {
      FAIL() << "FoldCase entry found in built tables that is not present in Original"
          << std::hex << c << " " << c_utf8 << " at idx " << idx;
    }
  }
  if (idx != (sizeof(UnicodeFoldLowerTable) / 4))
  {
    FAIL() << "Failed, did not exhaust unicode_fold_upper maxChar: " << maxChar
        << " FoldsFound: " << FoldsFound << " expected folds: "
        << sizeof(UnicodeFoldLowerTable) / 4;
  }
}

TEST(TestStringUtils, FoldCase)
{
  std::string input = ""s;
  std::string result = StringUtils::FoldCase(input);
  EXPECT_TRUE(result.size() == 0);

  input = "a"s;
  result = StringUtils::FoldCase(input);
  EXPECT_EQ(result, input);

  input = "A"s;
  result = StringUtils::FoldCase(input);
  EXPECT_EQ(result, "a"s);

  input =                "What a WaIsT Of Time1234567890-=!@#$%^&*()\"_+QWERTYUIOP{}|qwertyuiop[];':\\<M>?/.,";
  std::string expected = "what a waist of time1234567890-=!@#$%^&*()\"_+qwertyuiop{}|qwertyuiop[];':\\<m>?/.,";

  result = StringUtils::FoldCase(input);
  EXPECT_EQ(result, expected);

  input = "aB\0c"s; // Embedded null
  result = StringUtils::FoldCase(input);
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
    * ToLower  en_US I (Dotless I)       \u0049 -> i (Dotted small I)      \u0069
    * ToLower  tr_TR I (Dotless I)       \u0049 -> ı (Dotless small I)     \u0131
    * ToUpper  en_US I (Dotless I)       \u0049 -> I (Dotless I)           \u0049
    * ToUpper  tr_TR I (Dotless I)       \u0049 -> I (Dotless I)           \u0049
    * FoldCase  N/A  I (Dotless I)       \u0049 -> i (Dotted small I)      \u0069
    *
    * ToLower  en_US i (Dotted small I)  \u0069 -> i (Dotted small I)      \u0069
    * ToLower  tr_TR i (Dotted small I)  \u0069 -> i (Dotted small I)      \u0069
    * ToUpper  en_US i (Dotted small I)  \u0069 -> I (Dotted small I)      \u0069
    * ToUpper  tr_TR i (Dotted small I)  \u0069 -> İ (Dotted I)            \u0130
    * FoldCase  N/A  i (Dotted small I)  \u0069 -> i (Dotted small I)      \u0069
    *
    * ToLower  en_US İ (Dotted I)        \u0130 -> i̇ (Dotted small I)      \u0069
    * ToLower  tr_TR İ (Dotted I)        \u0130 -> i (Dotted small I)      \u0069
    * ToUpper  en_US İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130
    * ToUpper  tr_TR İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130
    * FoldCase  N/A  İ (Dotted I)        \u0130 -> İ (Dotted I)            \u0130
    *
    * ToLower  en_US ı (Dotless small I) \u0131 -> ı (Dotless small I)     \u0131
    * ToLower  tr_TR ı (Dotless small I) \u0131 -> ı (Dotless small I)     \u0131
    * ToUpper  en_US ı (Dotless small I) \u0131 -> ı (Dotless small I)     \u0131
    * ToUpper  tr_TR ı (Dotless small I) \u0131 -> I (Dotless I)           \u0049
    * FoldCase  N/A  ı (Dotless small I) \u0131 -> ı (Dotless small I)     \u0131
    *
    *
    * Note that even with FoldCase, the non-ASCII Turkic "I" characters do NOT get folded to ASCII
    * "i".
    */

  std::string s1 = "I İ i ı";
  std::string s2 = "i İ i ı";

  if (PrintResults)
  {
    std::cout << "Turkic orig s1: " << s1 << std::endl;
    std::cout << "Turkic orig s2: " << s2 << std::endl;
  }

  std::string result1;
  std::string result2;

  result1 = StringUtils::FoldCase(s1);
  result2 = StringUtils::FoldCase(s2);
  if (PrintResults)
  {
    std::cout << "Turkic folded result1: " << result1 << std::endl;
    std::cout << "Turkic folded result2: " << result2 << std::endl;
  }
  EXPECT_EQ(result1.compare(result2), 0);

  EXPECT_EQ(result1.compare(s2), 0);

  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  result1 = StringUtils::FoldCase(s1);
  result2 = StringUtils::FoldCase(s2);

  EXPECT_EQ(result1.compare(result2), 0);
  EXPECT_EQ(result1.compare(s2), 0);

  // Note that 'ß' is one beyond the last character in Table group (char 0xDF)
  // This makes it an excellent boundary test for FoldCase since it should NOT
  // be found in the table and therefore FoldCase should return itself. Further,
  // The value of the char (0XDF) is the value at Table[0] (the number of entries).

  s1 = "óóßChloë"s;
  result1 = "óóßchloë"s;
  EXPECT_EQ(StringUtils::FoldCase(s1), result1);

  // Last char in FoldCase table

  s1 = "𞤡"s;
  result1 = "𞥃"s;
  EXPECT_EQ(StringUtils::FoldCase(s1), result1);

  // Last output char in FoldCase table

  s1 = "𞥃"s;
  EXPECT_EQ(StringUtils::FoldCase(s1), result1);

}

TEST(TestStringUtils, FoldCase_W)
{
  // This implementation of FoldCase is not able to detect that these two UTF8 strings actually
  // represent the same Unicode codepoint

  bool PrintResults = false;

  std::wstring w_s1 = StringUtils::ToWString(
      std::string(TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  std::wstring w_s2 = StringUtils::ToWString(
      std::string(TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));

  w_s1 = StringUtils::FoldCase(w_s1);
  w_s2 = StringUtils::FoldCase(w_s2);
  int32_t result = w_s1.compare(w_s2);
  EXPECT_NE(result, 0);

  std::string s1 = "I İ i ı";
  std::string s2 = "i İ i ı";
  if (PrintResults)
  {
    std::cout << "Turkic orig s1: " << s1 << std::endl;
    std::cout << "Turkic orig s2: " << s2 << std::endl;
  }

  w_s1 = StringUtils::ToWString(s1);
  w_s2 = StringUtils::ToWString(s2);
  w_s1 = StringUtils::FoldCase(w_s1);
  w_s2 = StringUtils::FoldCase(w_s2);
  if (PrintResults)
  {
    std::cout << "Turkic folded w_s1: " << StringUtils::ToUtf8(w_s1) << std::endl;
    std::cout << "Turkic folded w_s2: " << StringUtils::ToUtf8(w_s2) << std::endl;
  }
  result = w_s1.compare(w_s2);
  EXPECT_EQ(result, 0);

  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  // std::cout << "Turkic orig s1: " << s1 << std::endl;
  // std::cout << "Turkic orig s2: " << s2 << std::endl;

  w_s1 = StringUtils::ToWString(s1);
  w_s2 = StringUtils::ToWString(s2);
  w_s1 = StringUtils::FoldCase(w_s1);
  w_s2 = StringUtils::FoldCase(w_s2);
  // std::cout << "Turkic folded w_s1: " << StringUtils::ToUtf8(w_s1) << std::endl;
  // std::cout << "Turkic folded w_s2: " << StringUtils::ToUtf8(w_s2) << std::endl;
  result = w_s1.compare(w_s2);
  EXPECT_EQ(result, 0);
}

TEST(TestStringUtils, EqualsNoCase)
{
  std::string refstr = "TeSt";
  std::string refstrView = "TeSt"; // TODO: Change to string_view after API changed

  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "tEsT")); // sv));
  EXPECT_FALSE(StringUtils::EqualsNoCase(refstrView, "TeSt "));
  EXPECT_FALSE(StringUtils::EqualsNoCase(refstrView, "TeSt    x")); // sv));
  EXPECT_FALSE(StringUtils::EqualsNoCase(refstr, "TeStTeStTeStTeSt"s));
  EXPECT_FALSE(StringUtils::EqualsNoCase(refstr, R"(TeStTeStTeStTeStx)"));

  std::string TeSt{"TeSt"};
  std::string tEsT{"tEsT"};

  std::string TestArrayS = "TestArray"s;
  std::string TestArray2S = "TestArray2"s;
  std::string TestArray3S = "TestArRaY"s;

  const char* TestArray = TestArrayS.c_str();
  const char* TestArray2 = TestArray2S.c_str();
  const char* TestArray3 = TestArray3S.c_str();

  EXPECT_FALSE(StringUtils::EqualsNoCase(TestArray, TestArray2));
  EXPECT_TRUE(StringUtils::EqualsNoCase(TestArray, TestArray3));

  EXPECT_TRUE(StringUtils::EqualsNoCase("TeSt", "TeSt"));
  EXPECT_TRUE(StringUtils::EqualsNoCase(TeSt, tEsT));

  const std::string constTest{"Test"};
  EXPECT_TRUE(StringUtils::EqualsNoCase(TeSt, constTest));
  EXPECT_TRUE(StringUtils::EqualsNoCase(TeSt + TeSt + TeSt + TeSt, "TeStTeStTeStTeSt"));
  EXPECT_FALSE(StringUtils::EqualsNoCase(TeSt, "TeStTeStTeStTeStx"));

  EXPECT_TRUE(StringUtils::EqualsNoCase(StringUtils::Empty, StringUtils::Empty));
  EXPECT_FALSE(StringUtils::EqualsNoCase(StringUtils::Empty, "x"));
  EXPECT_FALSE(StringUtils::EqualsNoCase("x", StringUtils::Empty));

  // EqualsNoCase can handle embedded nulls, but you have to be careful
  // how you enter them. Ex: "abcd\0"  "ABCD\0"sv One has null, the other
  // is terminated at null.

  EXPECT_TRUE(StringUtils::EqualsNoCase("abcd\0", "ABCD\0")); //sv));
  EXPECT_TRUE(StringUtils::EqualsNoCase("abcd\0a", "ABCD\0a"));

  // EqualsNoCase can handle embedded nulls, but you have to be careful
  // how you enter them. Ex: "abcd\0"  "ABCD\0"sv One has null, the other
  // is terminated at null.

  // EXPECT_FALSE(StringUtils::EqualsNoCase("abcd\0x", "ABCD\0y")); // sv));

  EXPECT_TRUE(StringUtils::EqualsNoCase("abcd\0x", "ABCD\0y"));

  // EqualsNoCase can handle embedded nulls, but you have to be careful
  // how you enter them. Ex: "abcd\0"  "ABCD\0"sv One has null, the other
  // is terminated at null.

  // EXPECT_FALSE(StringUtils::EqualsNoCase("abcd\0", "ABCD\0a")); // sv));

  EXPECT_TRUE(StringUtils::EqualsNoCase("abcd\0", "ABCD\0a")); // sv));
}


TEST(TestStringUtils, CompareNoCase)
{
  std::string left;
  std::string right;
  int expectedResult{0};

  left =  "abciI123ABC "s;
  right = "ABCIi123abc "s;
  expectedResult = 0;
  EXPECT_EQ(StringUtils::CompareNoCase(left, right), expectedResult);

  // Since Kodi's simpleFoldCase can not handle characters that
  // change length when folding, the following fails.
  // German 'ß' is equivalent to 'ss'

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 1;

  // TODO: Single-byte comparison will fail, while a Unicode (4-byte)
  // comparision will pass. Change result after changing ToLower to use
  // Unicode comparison

  // EXPECT_EQ(StringUtils::CompareNoCase(left, right), expectedResult);
  EXPECT_NE(StringUtils::CompareNoCase(left, right), expectedResult);

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_LOWER_SS; // óósschloë // Does not convert SS to ß
  expectedResult = 0;

  // TODO: Single-byte comparison will fail, while a Unicode (4-byte)
  // comparision will pass. Change result after changing ToLower to use
  // Unicode comparison

  // EXPECT_EQ(StringUtils::CompareNoCase(left, right), expectedResult);
  EXPECT_NE(StringUtils::CompareNoCase(left, right), expectedResult);
}

TEST(TestStringUtils, CompareNoCase_Advanced)
{
  std::string left; // Don't use string_view!
  std::string right; // Don't use string_view!
  int expectedResult;

  left =  "abciI123ABC ";
  left = left.substr(0, 5); // Limits string to 5 code-units (bytes)
  right = "ABCIi123abc ";
  right = right.substr(0, 5); // Not very exciting
  expectedResult = 0;
  EXPECT_EQ(StringUtils::CompareNoCase(left, right), expectedResult);

  left = "abciI123ABC ";
  right = "ABCIi ";
  // Compare first 5 Unicode code-points ('graphemes'/'characters') formed from the UTF8
  EXPECT_EQ(StringUtils::CompareNoCase(left, right, 5), expectedResult);

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_LOWER_SS; // óósschloë

  // TODO: Once CompareCase API changed to use Foldcase, then rework
  // test results
  //
  // Single byte comparison results in < 0
  // Unicode codepoint comparision results in 0

  EXPECT_TRUE(StringUtils::CompareNoCase(left, right) < 0);

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

  left = TestStringUtils::UTF8_GERMAN_UPPER;   // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_SAMPLE; // óóßChloë

  // TODO:
  // Single byte comparison results in < 0
  // Unicode codepoint comparision results in > 0

  EXPECT_TRUE(StringUtils::CompareNoCase(left, right) < 0);

  left = {TestStringUtils::UTF8_GERMAN_UPPER, 4};   // ÓÓSSCHLOË byte 4 = end of 2nd Ó
  right = {TestStringUtils::UTF8_GERMAN_SAMPLE, 4}; // óóßChloë  byte 4 = end of 2nd ó

  // TODO:
  // Single byte comparison results in < 0 (always will be < 0, due to high-bit
  // set on all start bytes of multi-byte sequence)
  // Unicode codepoint comparision results in > 0

  EXPECT_TRUE(StringUtils::CompareNoCase(left, right) < 0);

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  right = TestStringUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  expectedResult = 0;

  // Compare first two code-points (same result as previous test with limit of 4th byte)

  // TODO:
  // Single byte comparison results in < 0
  // Unicode codepoint comparision results in > 0

  EXPECT_TRUE(StringUtils::CompareNoCase(left, right, 2) < 0);

  // A full-foldcase would recognize that "ß" and "ss" are equivalent.
  // Attempting here to confirm current behavior.
  // Limit by bytes, since limiting by the same # code-points doesn't produce
  // desired result

  left = {TestStringUtils::UTF8_GERMAN_UPPER, 0, 6}; // ÓÓSSCHLOË
  right = {TestStringUtils::UTF8_GERMAN_SAMPLE, 0, 6}; // óóßChloë

  // TODO:
  // Single byte comparison results in < 0
  // Unicode codepoint comparision results in > 0

  EXPECT_TRUE(StringUtils::CompareNoCase(left, right) < 0);

  // Limit by number of codepoints if we use utf32string using a rather
  // inefficient means

  left = TestStringUtils::UTF8_GERMAN_UPPER; // ÓÓSSCHLOË
  std::u32string leftUtf32 = {StringUtils::ToUtf32(left), 0, 4}; // => ÓÓSS
  left = StringUtils::ToUtf8(leftUtf32);

  right = TestStringUtils::UTF8_GERMAN_SAMPLE; // óóßChloë
  std::u32string rightUtf32 = {StringUtils::ToUtf32(right), 0, 3}; // => óóß
  right = StringUtils::ToUtf8(rightUtf32);

  // TODO:
  // Single byte comparison results in < 0
  // Unicode codepoint comparision results in > 0

  EXPECT_TRUE(StringUtils::CompareNoCase(left, right) < 0 );

  // At least this fold case recognizes these as equivalent by converting
  // to u32string. Comparison with UTF8 would fail.

  left = TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1; // 6 bytes
  right = TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5; // 4 bytes
  expectedResult = 0;
  EXPECT_NE(StringUtils::CompareNoCase(left, right), expectedResult);

  // Boundary Tests

  left = "abciI123ABC ";
  right = "ABCIi123abc ";
  expectedResult = 0;
  EXPECT_EQ(StringUtils::CompareNoCase(left, right, 0), expectedResult);

  left = "";
  right = "ABCIi123abc ";
  EXPECT_TRUE(StringUtils::CompareNoCase(left, right) < 0);

  left = "abciI123ABC ";
  right = "";
  EXPECT_TRUE(StringUtils::CompareNoCase(left, right) > 0);

  left = "";
  right = "";
  expectedResult = 0;
  EXPECT_EQ(StringUtils::CompareNoCase(left, right), expectedResult);
}

TEST(TestStringUtils, Left)
{
  std::string refstr;
  std::string varstr;
  std::string origstr = "test";

  refstr = "";
  varstr = StringUtils::Left(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "te";
  varstr = StringUtils::Left(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test";
  varstr = StringUtils::Left(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Mid)
{
  std::string refstr;
  std::string varstr;
  std::string origstr{"test"};

  refstr = "";
  varstr = StringUtils::Mid(origstr, 0, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "te";
  varstr = StringUtils::Mid(origstr, 0, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test";
  varstr = StringUtils::Mid(origstr, 0, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = StringUtils::Mid(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = StringUtils::Mid(origstr, 2, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "es";
  varstr = StringUtils::Mid(origstr, 1, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Right)
{
  std::string refstr;
  std::string varstr;
  std::string origstr{"test"};

  refstr = "";
  varstr = StringUtils::Right(origstr, 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "st";
  varstr = StringUtils::Right(origstr, 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test";
  varstr = StringUtils::Right(origstr, 10);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Trim)
{
  std::string refstr{"test test"};

  std::string varstr{" test test   "};
  StringUtils::Trim(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, TrimLeft)
{
  std::string refstr = "test test   ";

  std::string varstr = " test test   ";
  StringUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, TrimRight)
{
  std::string refstr = " test test";

  std::string varstr = " test test   ";
  StringUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Replace)
{
  std::string refstr = "text text";

  std::string varstr = "test test";
  EXPECT_EQ(StringUtils::Replace(varstr, 's', 'x'), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(StringUtils::Replace(varstr, 's', 'x'), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = "test test";
  EXPECT_EQ(StringUtils::Replace(varstr, "s", "x"), 2);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  EXPECT_EQ(StringUtils::Replace(varstr, "s", "x"), 0);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, StartsWith)
{
  std::string refstr = "test";
  std::string input;
  // std::string_view  p;
  std::string p;

  EXPECT_FALSE(StringUtils::StartsWith(refstr, "x"));
  EXPECT_TRUE(StringUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(StringUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::StartsWith(refstr, "Te"));
  EXPECT_FALSE(StringUtils::StartsWith(refstr, "test "));
  EXPECT_TRUE(StringUtils::StartsWith(refstr, "test\0")); // Embedded null terminates string

  p = "tes"s;
  EXPECT_TRUE(StringUtils::StartsWith(refstr, p));

  // Boundary

  input = "";
  EXPECT_TRUE(StringUtils::StartsWith(input, ""));
  EXPECT_FALSE(StringUtils::StartsWith(input, "Four score and seven years ago"));

  EXPECT_TRUE(StringUtils::StartsWith(refstr, ""));

  input = "";
  p = "";
  EXPECT_TRUE(StringUtils::StartsWith(input, p));
  EXPECT_TRUE(StringUtils::StartsWith(refstr, p));
}

TEST(TestStringUtils, StartsWithNoCase)
{
  std::string refstr = "test";
  std::string input;
  //  std::string_view p;
  std::string p;

  EXPECT_FALSE(StringUtils::StartsWithNoCase(refstr, "x"));
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "TesT"));
  EXPECT_FALSE(StringUtils::StartsWithNoCase(refstr, "Te st"));
  EXPECT_FALSE(StringUtils::StartsWithNoCase(refstr, "test "));
  EXPECT_TRUE(StringUtils::StartsWithNoCase(
      refstr, "test\0")); // Embedded null terminates string operation

  p = "tEs";
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, p));

  // Boundary

  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, ""));

  p = "";
  // TODO: Verify Non-empty string begins with empty string.
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, p));
  // Same behavior with char * and string
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, ""));

  input = "";
  // TODO: Empty string does begin with empty string
  EXPECT_TRUE(StringUtils::StartsWithNoCase(input, ""));
  EXPECT_FALSE(StringUtils::StartsWithNoCase(input, "Four score and seven years ago"));

  input = "";
  p = "";
  EXPECT_TRUE(StringUtils::StartsWithNoCase(input, p));
}

TEST(TestStringUtils, EndsWith)
{
  std::string refstr = "test";

  EXPECT_TRUE(StringUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(StringUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::EndsWith(refstr, "sT"));
}

TEST(TestStringUtils, EndsWithNoCase)
{
  std::string refstr = "test";
  EXPECT_FALSE(StringUtils::EndsWithNoCase(refstr, "x"));
  EXPECT_TRUE(StringUtils::EndsWithNoCase(refstr, "sT"));
  EXPECT_TRUE(StringUtils::EndsWithNoCase(refstr, "TesT"));
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
  varstr = StringUtils::Join(strarray, ",");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, Split)
{
  std::vector<std::string> varresults;

  // test overload with string as delimiter
  varresults = StringUtils::Split("g,h,ij,k,lm,,n", ",");
  EXPECT_STREQ("g", varresults.at(0).c_str());
  EXPECT_STREQ("h", varresults.at(1).c_str());
  EXPECT_STREQ("ij", varresults.at(2).c_str());
  EXPECT_STREQ("k", varresults.at(3).c_str());
  EXPECT_STREQ("lm", varresults.at(4).c_str());
  EXPECT_STREQ("", varresults.at(5).c_str());
  EXPECT_STREQ("n", varresults.at(6).c_str());

  EXPECT_TRUE(StringUtils::Split("", "|").empty());

  EXPECT_EQ(4U, StringUtils::Split("a bc  d ef ghi ", " ", 4).size());
  EXPECT_STREQ("d ef ghi ", StringUtils::Split("a bc  d ef ghi ", " ", 4).at(3).c_str()) << "Last part must include rest of the input string";
  EXPECT_EQ(7U, StringUtils::Split("a bc  d ef ghi ", " ").size()) << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", StringUtils::Split("a bc  d ef ghi ", " ").at(1).c_str());
  EXPECT_STREQ("", StringUtils::Split("a bc  d ef ghi ", " ").at(2).c_str());
  EXPECT_STREQ("", StringUtils::Split("a bc  d ef ghi ", " ").at(6).c_str());

  EXPECT_EQ(2U, StringUtils::Split("a bc  d ef ghi ", "  ").size());
  EXPECT_EQ(2U, StringUtils::Split("a bc  d ef ghi ", "  ", 10).size());
  EXPECT_STREQ("a bc", StringUtils::Split("a bc  d ef ghi ", "  ", 10).at(0).c_str());

  EXPECT_EQ(1U, StringUtils::Split("a bc  d ef ghi ", " z").size());
  EXPECT_STREQ("a bc  d ef ghi ", StringUtils::Split("a bc  d ef ghi ", " z").at(0).c_str());

  EXPECT_EQ(1U, StringUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", StringUtils::Split("a bc  d ef ghi ", "").at(0).c_str());

  // test overload with char as delimiter
  EXPECT_EQ(4U, StringUtils::Split("a bc  d ef ghi ", ' ', 4).size());
  EXPECT_STREQ("d ef ghi ", StringUtils::Split("a bc  d ef ghi ", ' ', 4).at(3).c_str());
  EXPECT_EQ(7U, StringUtils::Split("a bc  d ef ghi ", ' ').size()) << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", StringUtils::Split("a bc  d ef ghi ", ' ').at(1).c_str());
  EXPECT_STREQ("", StringUtils::Split("a bc  d ef ghi ", ' ').at(2).c_str());
  EXPECT_STREQ("", StringUtils::Split("a bc  d ef ghi ", ' ').at(6).c_str());

  EXPECT_EQ(1U, StringUtils::Split("a bc  d ef ghi ", 'z').size());
  EXPECT_STREQ("a bc  d ef ghi ", StringUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());

  EXPECT_EQ(1U, StringUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", StringUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());
}

TEST(TestStringUtils, FindNumber)
{
  EXPECT_EQ(3, StringUtils::FindNumber("aabcaadeaa", "aa"));
  EXPECT_EQ(1, StringUtils::FindNumber("aabcaadeaa", "b"));
}

TEST(TestStringUtils, AlphaNumericCompare)
{
  int64_t ref;
  int64_t var;

  ref = 0;
  var = StringUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
}

TEST(TestStringUtils, TimeStringToSeconds)
{
  EXPECT_EQ(77455, StringUtils::TimeStringToSeconds("21:30:55"));
  EXPECT_EQ(7*60, StringUtils::TimeStringToSeconds("7 min"));
  EXPECT_EQ(7*60, StringUtils::TimeStringToSeconds("7 min\t"));
  EXPECT_EQ(154*60, StringUtils::TimeStringToSeconds("   154 min"));
  EXPECT_EQ(1*60+1, StringUtils::TimeStringToSeconds("1:01"));
  EXPECT_EQ(4*60+3, StringUtils::TimeStringToSeconds("4:03"));
  EXPECT_EQ(2*3600+4*60+3, StringUtils::TimeStringToSeconds("2:04:03"));
  EXPECT_EQ(2*3600+4*60+3, StringUtils::TimeStringToSeconds("   2:4:3"));
  EXPECT_EQ(2*3600+4*60+3, StringUtils::TimeStringToSeconds("  \t\t 02:04:03 \n "));
  EXPECT_EQ(1*3600+5*60+2, StringUtils::TimeStringToSeconds("01:05:02:04:03 \n "));
  EXPECT_EQ(0, StringUtils::TimeStringToSeconds("blah"));
  EXPECT_EQ(0, StringUtils::TimeStringToSeconds("ля-ля"));
}

TEST(TestStringUtils, RemoveCRLF)
{
  std::string refstr;
  std::string varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  StringUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, utf8_strlen)
{
  size_t ref;
  size_t var;

  ref = 9;
  var = StringUtils::utf8_strlen("ｔｅｓｔ＿ＵＴＦ８");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, SecondsToTimeString)
{
  std::string refstr;
  std::string varstr;

  refstr = "21:30:55";
  varstr = StringUtils::SecondsToTimeString(77455);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, IsNaturalNumber)
{
  EXPECT_TRUE(StringUtils::IsNaturalNumber("10"));
  EXPECT_TRUE(StringUtils::IsNaturalNumber(" 10"));
  EXPECT_TRUE(StringUtils::IsNaturalNumber("0"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber(" 1 0"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("1.0"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("1.1"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("0x1"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("blah"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber("120 h"));
  EXPECT_FALSE(StringUtils::IsNaturalNumber(" "));
  EXPECT_FALSE(StringUtils::IsNaturalNumber(""));
}

TEST(TestStringUtils, IsInteger)
{
  EXPECT_TRUE(StringUtils::IsInteger("10"));
  EXPECT_TRUE(StringUtils::IsInteger(" -10"));
  EXPECT_TRUE(StringUtils::IsInteger("0"));
  EXPECT_FALSE(StringUtils::IsInteger(" 1 0"));
  EXPECT_FALSE(StringUtils::IsInteger("1.0"));
  EXPECT_FALSE(StringUtils::IsInteger("1.1"));
  EXPECT_FALSE(StringUtils::IsInteger("0x1"));
  EXPECT_FALSE(StringUtils::IsInteger("blah"));
  EXPECT_FALSE(StringUtils::IsInteger("120 h"));
  EXPECT_FALSE(StringUtils::IsInteger(" "));
  EXPECT_FALSE(StringUtils::IsInteger(""));
}

TEST(TestStringUtils, SizeToString)
{
  std::string ref;
  std::string var;

  ref = "2.00 GB";
  var = StringUtils::SizeToString(2147483647);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "0.00 B";
  var = StringUtils::SizeToString(0);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStringUtils, EmptyString)
{
  EXPECT_STREQ("", StringUtils::Empty.c_str());
}

TEST(TestStringUtils, FindWords)
{
  size_t ref;
  size_t var;

  ref = 5;
  var = StringUtils::FindWords("test string", "string");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("12345string", "string");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("apple2012", "2012");
  EXPECT_EQ(ref, var);
  ref = -1;
  var = StringUtils::FindWords("12345string", "ring");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("12345string", "345");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("apple2012", "e2012");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("apple2012", "12");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindWords_NonAscii)
{
  size_t ref;
  size_t var;

  ref = 6;
  var = StringUtils::FindWords("我的视频", "视频");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("我的视频", "视");
  EXPECT_EQ(ref, var);
  var = StringUtils::FindWords("Apple ple", "ple");
  EXPECT_EQ(ref, var);
  ref = 7;
  var = StringUtils::FindWords("Äpfel.pfel", "pfel");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindEndBracket)
{
  size_t ref;
  size_t var;

  ref = 11;
  var = StringUtils::FindEndBracket("atest testbb test", 'a', 'b');
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, DateStringToYYYYMMDD)
{
  size_t ref;
  size_t var;

  ref = 20120706;
  var = StringUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, WordToDigits)
{
  std::string ref;
  std::string var;

  ref = "8378 787464";
  var = "test string";
  StringUtils::WordToDigits(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStringUtils, CreateUUID)
{
  std::cout << "CreateUUID(): " << StringUtils::CreateUUID() << std::endl;
}

TEST(TestStringUtils, ValidateUUID)
{
  EXPECT_TRUE(StringUtils::ValidateUUID(StringUtils::CreateUUID()));
}

TEST(TestStringUtils, CompareFuzzy)
{
  double ref;
  double var;

  ref = 6.25;
  var = StringUtils::CompareFuzzy("test string", "string test");
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
  varint = StringUtils::FindBestMatch("test", strarray, vardouble);
  EXPECT_EQ(refint, varint);
  EXPECT_EQ(refdouble, vardouble);
}

TEST(TestStringUtils, Paramify)
{
  const char *input = "some, very \\ odd \"string\"";
  const char *ref   = "\"some, very \\\\ odd \\\"string\\\"\"";

  std::string result = StringUtils::Paramify(input);
  EXPECT_STREQ(ref, result.c_str());
}

TEST(TestStringUtils, sortstringbyname)
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

TEST(TestStringUtils, FileSizeFormat)
{
  EXPECT_STREQ("0B", StringUtils::FormatFileSize(0).c_str());

  EXPECT_STREQ("999B", StringUtils::FormatFileSize(999).c_str());
  EXPECT_STREQ("0.98kB", StringUtils::FormatFileSize(1000).c_str());

  EXPECT_STREQ("1.00kB", StringUtils::FormatFileSize(1024).c_str());
  EXPECT_STREQ("9.99kB", StringUtils::FormatFileSize(10229).c_str());

  EXPECT_STREQ("10.1kB", StringUtils::FormatFileSize(10387).c_str());
  EXPECT_STREQ("99.9kB", StringUtils::FormatFileSize(102297).c_str());

  EXPECT_STREQ("100kB", StringUtils::FormatFileSize(102400).c_str());
  EXPECT_STREQ("999kB", StringUtils::FormatFileSize(1023431).c_str());

  EXPECT_STREQ("0.98MB", StringUtils::FormatFileSize(1023897).c_str());
  EXPECT_STREQ("0.98MB", StringUtils::FormatFileSize(1024000).c_str());

  //Last unit should overflow the 3 digit limit
  EXPECT_STREQ("5432PB", StringUtils::FormatFileSize(6115888293969133568).c_str());
}

TEST(TestStringUtils, ToHexadecimal)
{
  EXPECT_STREQ("", StringUtils::ToHexadecimal("").c_str());
  EXPECT_STREQ("616263", StringUtils::ToHexadecimal("abc").c_str());
  std::string a{"a\0b\n", 4};
  EXPECT_STREQ("6100620a", StringUtils::ToHexadecimal(a).c_str());
  std::string nul{"\0", 1};
  EXPECT_STREQ("00", StringUtils::ToHexadecimal(nul).c_str());
  std::string ff{"\xFF", 1};
  EXPECT_STREQ("ff", StringUtils::ToHexadecimal(ff).c_str());
}
