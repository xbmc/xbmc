/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StringUtils.h"

#include <algorithm>
#include <codecvt>
#include <string>
#include <string_view>

#include <gtest/gtest.h>
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

namespace TestStringUtils
{

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
} // namespace TestStringUtils

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
  // Won't need explicit string_view after removing old api
  varstr = StringUtils::ToUpper(std::string_view(varstr));
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, ToLower)
{
  std::string refstr = "test";

  std::string varstr = "TeSt";
  varstr = StringUtils::ToLower(std::string_view(varstr));
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

TEST(TestStringUtils, FoldCase)
{
  // Test case adapated from ICU FoldCase test. The
  // results are different, but good enough.

  std::string s1 = "I İ i ı"; // All four Turkic variants of 'i'
  std::string s2 = "i İ i ı"; // We are in trouble if lower(I) != i
  int result; // or upper(i) != I

  s1 = StringUtils::FoldCase(s1);
  s2 = StringUtils::FoldCase(s2);
  //std::cout << "Turkic folded s1: " << s1 << std::endl;
  //std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = s1.compare(s2);
  EXPECT_EQ(result, 0);

  std::string I = "I";
  std::string I_DOT = "İ";
  std::string I_DOT_I_DOT = "İİ";
  std::string i = "i";
  std::string ii = "ii";
  std::string i_DOTLESS = "ı";
  std::string i_DOTLESS_i_DOTTLESS = "ıı";
  std::string i_COMBINING_DOUBLE_DOT = "i̇";

  // (at least these) Characters outside of ASCII left alone

  s1 = I + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s2 = i + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s1 = StringUtils::FoldCase(s1);
  s2 = StringUtils::FoldCase(s2);
  //std::cout << "Turkic folded s1: " << s1 << std::endl;
  //std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = s1.compare(s2);
  EXPECT_EQ(result, 0);

  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcÇdefgĞhijklmnoÖprsŞtuÜvyz";

  //std::cout << "Turkic orig s1: " << s1 << std::endl;
  //std::cout << "Turkic orig s2: " << s2 << std::endl;

  s1 = StringUtils::FoldCase(s1);
  s2 = StringUtils::FoldCase(s2);
  //std::cout << "Turkic folded s1: " << s1 << std::endl;
  //std::cout << "Turkic folded s2: " << s2 << std::endl;

  result = s1.compare(s2);
  EXPECT_EQ(result, 0);
}

TEST(TestStringUtils, FoldCaseW)
{
  // Test case adapated from ICU FoldCase test. The
  // results are different, but good enough. The main thing is that
  // all known 'keys' which are toLowered/toUppered before lookup
  // in table (or other caseless comparison) are ASCII. Therefore,
  // things will work okay for such keys of the case fold does not
  // change the case of ASCII chars in a manner inconsistent with
  // what is considered normal (in English). (Turkish is one such
  // trouble making locale).
  //
  // To guard against the possibility of some keys containing some
  // non-ASCII characters, a local insensitive case-fold is much
  // preferred.

  std::wstring s1 = L"I İ i ı";
  std::wstring s2 = L"i İ i ı";
  int result;

  s1 = StringUtils::FoldCase(s1);
  s2 = StringUtils::FoldCase(s2);
  std::cout << "Turkic folded s1: " << StringUtils::WStringToUTF8(s1) << std::endl;
  std::cout << "Turkic folded s2: " << StringUtils::WStringToUTF8(s2) << std::endl;
  result = s1.compare(s2);
  EXPECT_EQ(result, 0);

  std::wstring I = L"I";
  std::wstring I_DOT = L"İ";
  std::wstring I_DOT_I_DOT = L"İİ";
  std::wstring i = L"i";
  std::wstring ii = L"ii";
  std::wstring i_DOTLESS = L"ı";
  std::wstring i_DOTLESS_i_DOTTLESS = L"ıı";
  std::wstring i_COMBINING_DOUBLE_DOT = L"i̇";

  // (at least these) Characters outside of ASCII left alone

  s1 = I + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s2 = i + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s1 = StringUtils::FoldCase(s1);
  s2 = StringUtils::FoldCase(s2);
  std::cout << "Turkic folded s1: " << StringUtils::WStringToUTF8(s1) << std::endl;
  std::cout << "Turkic folded s2: " << StringUtils::WStringToUTF8(s2) << std::endl;
  result = s1.compare(s2);
  EXPECT_EQ(result, 0);

  s1 = L"ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = L"abcÇdefgĞhijklmnoÖprsŞtuÜvyz";

  //std::cout << "Turkic orig s1: " << s1 << std::endl;
  //std::cout << "Turkic orig s2: " << s2 << std::endl;

  s1 = StringUtils::FoldCase(s1);
  s2 = StringUtils::FoldCase(s2);
  std::cout << "Turkic folded s1: " << StringUtils::WStringToUTF8(s1) << std::endl;
  std::cout << "Turkic folded s2: " << StringUtils::WStringToUTF8(s2) << std::endl;

  result = s1.compare(s2);
  EXPECT_EQ(result, 0);
}

TEST(TestStringUtils, FoldCase_W)
{
  std::wstring w_s1 = StringUtils::UTF8ToWString(
      std::string(TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  std::wstring w_s2 = StringUtils::UTF8ToWString(
      std::string(TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));

  w_s1 = StringUtils::FoldCase(w_s1);
  w_s2 = StringUtils::FoldCase(w_s2);
  int32_t result = w_s1.compare(w_s2);
  EXPECT_NE(result, 0);

  w_s1 = StringUtils::UTF8ToWString(
      std::string(TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_5));
  w_s2 = StringUtils::UTF8ToWString(
      std::string(TestStringUtils::UTF8_MULTI_CODEPOINT_CHAR1_VARIENT_1));
  w_s1 = StringUtils::FoldCase(w_s1);
  w_s2 = StringUtils::FoldCase(w_s2);
  std::cout << "Turkic folded w_s1: " << StringUtils::WStringToUTF8(w_s1) << std::endl;
  std::cout << "Turkic folded w_s2: " << StringUtils::WStringToUTF8(w_s2) << std::endl;
  result = w_s1.compare(w_s2);
  EXPECT_NE(result, 0);

  std::string s1 = "I İ i ı";
  std::string s2 = "i İ i ı";
  std::cout << "Turkic orig s1: " << s1 << std::endl;
  std::cout << "Turkic orig s2: " << s2 << std::endl;

  w_s1 = StringUtils::UTF8ToWString(s1);
  w_s2 = StringUtils::UTF8ToWString(s2);
  w_s1 = StringUtils::FoldCase(w_s1);
  w_s2 = StringUtils::FoldCase(w_s2);
  std::cout << "Turkic folded w_s1: " << StringUtils::WStringToUTF8(w_s1) << std::endl;
  std::cout << "Turkic folded w_s2: " << StringUtils::WStringToUTF8(w_s2) << std::endl;
  result = w_s1.compare(w_s2);
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
  s2 = i + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;

  std::cout << "Turkic orig s1: " << s1 << std::endl;
  std::cout << "Turkic orig s2: " << s2 << std::endl;

  w_s1 = StringUtils::UTF8ToWString(s1);
  w_s2 = StringUtils::UTF8ToWString(s2);
  w_s1 = StringUtils::FoldCase(w_s1);
  w_s2 = StringUtils::FoldCase(w_s2);
  std::cout << "Turkic folded w_s1: " << StringUtils::WStringToUTF8(w_s1) << std::endl;
  std::cout << "Turkic folded w_s2: " << StringUtils::WStringToUTF8(w_s2) << std::endl;
  result = w_s1.compare(w_s2);

  EXPECT_EQ(result, 0);

  s1 = "ABCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYZ";
  s2 = "abcçdefgğhijklmnoöprsştuüvyz";

  std::cout << "Turkic orig s1: " << s1 << std::endl;
  std::cout << "Turkic orig s2: " << s2 << std::endl;

  w_s1 = StringUtils::UTF8ToWString(s1);
  w_s2 = StringUtils::UTF8ToWString(s2);
  w_s1 = StringUtils::FoldCase(w_s1);
  w_s2 = StringUtils::FoldCase(w_s2);
  std::cout << "Turkic folded w_s1: " << StringUtils::WStringToUTF8(w_s1) << std::endl;
  std::cout << "Turkic folded w_s2: " << StringUtils::WStringToUTF8(w_s2) << std::endl;
  result = w_s1.compare(w_s2);
  EXPECT_EQ(result, 0);
}

TEST(TestStringUtils, FoldCaseUpper)
{
  // Test case adapated from ICU FoldCase test. The
  // Test case adapated from ICU FoldCase test. The
  // results are different, but good enough. The main thing is that
  // all known 'keys' which are toLowered/toUppered before lookup
  // in table (or other caseless comparison) are ASCII. Therefore,
  // things will work okay for such keys of the case fold does not
  // change the case of ASCII chars in a manner inconsistent with
  // what is considered normal (in English). (Turkish is one such
  // trouble making locale).
  //
  // To guard against the possibility of some keys containing some
  // non-ASCII characters, a local insensitive case-fold is much
  // preferred.

  std::string s1 = "I İ İ i ı";
  std::string s2 = "I İ İ I ı";
  int result;

  s1 = StringUtils::FoldCaseUpper(s1);
  s2 = StringUtils::FoldCaseUpper(s2);
  std::cout << "Turkic folded s1: " << s1 << std::endl;
  std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = s1.compare(s2);
  EXPECT_EQ(result, 0);

  std::string I = "I";
  std::string I_DOT = "İ";
  std::string I_DOT_I_DOT = "İİ";
  std::string i = "i";
  std::string ii = "ii";
  std::string i_DOTLESS = "ı";
  std::string i_DOTLESS_i_DOTTLESS = "ıı";
  std::string i_COMBINING_DOUBLE_DOT = "i̇";

  // (at least these) Characters outside of en_GB left alone

  s1 = I + I_DOT + i + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s2 = I + I_DOT + I + i_DOTLESS + i_COMBINING_DOUBLE_DOT;
  s1 = StringUtils::FoldCaseUpper(s1);
  s2 = StringUtils::FoldCaseUpper(s2);
  //std::cout << "Turkic folded s1: " << s1 << std::endl;
  //std::cout << "Turkic folded s2: " << s2 << std::endl;
  result = s1.compare(s2);
  EXPECT_EQ(result, 0);

  s1 = "AbCÇDEFGĞHIJKLMNOÖPRSŞTUÜVYz";
  s2 = "aBcÇdefgĞhijklmnoÖprsŞtuÜvyZ";

  //std::cout << "Turkic orig s1: " << s1 << std::endl;
  //std::cout << "Turkic orig s2: " << s2 << std::endl;

  s1 = StringUtils::FoldCaseUpper(s1);
  s2 = StringUtils::FoldCaseUpper(s2);
  //std::cout << "Turkic folded s1: " << s1 << std::endl;
  //std::cout << "Turkic folded s2: " << s2 << std::endl;

  result = s1.compare(s2);
  EXPECT_EQ(result, 0);
}

TEST(TestStringUtils, EqualsNoCase)
{
  std::string refstr = "TeSt";

  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "tEsT"));
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
  std::string origstr = "test";

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
  std::string origstr = "test";

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
  std::string refstr = "test test";

  std::string varstr = " test test   ";
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

  EXPECT_FALSE(StringUtils::StartsWithNoCase(refstr, "x"));

  EXPECT_TRUE(StringUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(StringUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::StartsWith(refstr, "Te"));

  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "TesT"));
}

TEST(TestStringUtils, EndsWith)
{
  std::string refstr = "test";

  EXPECT_FALSE(StringUtils::EndsWithNoCase(refstr, "x"));

  EXPECT_TRUE(StringUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(StringUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::EndsWith(refstr, "sT"));

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
