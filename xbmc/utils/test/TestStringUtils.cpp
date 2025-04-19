/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/StringUtils.h"

#include <algorithm>
#include <limits>
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

TEST(TestStringUtils, EqualsNoCase)
{
  std::string refstr = "TeSt";

  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "tEsT"));
}

TEST(TestStringUtils, ReturnDigits)
{
  EXPECT_EQ(123456, StringUtils::ReturnDigits("H1el2lo 3Wor4ld56"));
}

TEST(TestStringUtils, Left)
{
  std::string refstr, varstr;
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
  std::string refstr, varstr;
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
  std::string refstr, varstr;
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

  varstr = "ababtest testabab";
  StringUtils::Trim(varstr, "ab");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, TrimLeft)
{
  std::string refstr = "test test   ";

  std::string varstr = " test test   ";
  StringUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "test testabab";
  varstr = "ababtest testabab";
  StringUtils::TrimLeft(varstr, "ab");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, TrimRight)
{
  std::string refstr = " test test";

  std::string varstr = " test test   ";
  StringUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  refstr = "ababtest test";
  varstr = "ababtest testabab";
  StringUtils::TrimRight(varstr, "ab");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, RemoveDuplicatedSpacesAndTabs)
{
  const std::string refstr = " test test ";

  std::string varstr = "\t\ttest  test \t";
  StringUtils::RemoveDuplicatedSpacesAndTabs(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, ReplaceSpecialCharactersWithSpace)
{
  const std::string input("a .-_+b,!'c\"\tde/\\f*?g#$%h&@(i)[j]{k}");
  const std::string output = StringUtils::ReplaceSpecialCharactersWithSpace(input);
  EXPECT_STREQ(output.c_str(), "a b c de f g h i j k ");
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
  std::string refstr, varstr;
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

TEST(TestStringUtils, SplitMulti)
{
  const std::vector<std::string> input{"aaa::bbb", "cc:c##ddd::eee"};
  const std::vector<std::string> delims{"::", "##"};

  EXPECT_EQ(std::vector<std::string>({"aaa", "bbb", "cc:c", "ddd", "eee"}),
            StringUtils::SplitMulti(input, delims));
  EXPECT_EQ(std::vector<std::string>({"aaa::bbb", "cc:c##ddd::eee"}),
            StringUtils::SplitMulti(input, delims, 1));
  EXPECT_EQ(std::vector<std::string>({"aaa::bbb", "cc:c##ddd::eee"}),
            StringUtils::SplitMulti(input, delims, 2));
  EXPECT_EQ(std::vector<std::string>({"aaa", "bbb", "cc:c##ddd::eee"}),
            StringUtils::SplitMulti(input, delims, 3));
  EXPECT_EQ(std::vector<std::string>({"aaa", "bbb", "cc:c##ddd", "eee"}),
            StringUtils::SplitMulti(input, delims, 4));
  EXPECT_EQ(std::vector<std::string>({"aaa", "bbb", "cc:c", "ddd", "eee"}),
            StringUtils::SplitMulti(input, delims, 5));
  EXPECT_EQ(std::vector<std::string>({"aaa", "bbb", "cc:c", "ddd", "eee"}),
            StringUtils::SplitMulti(input, delims, 6));
}

TEST(TestStringUtils, FindNumber)
{
  EXPECT_EQ(3, StringUtils::FindNumber("aabcaadeaa", "aa"));
  EXPECT_EQ(1, StringUtils::FindNumber("aabcaadeaa", "b"));
}

TEST(TestStringUtils, AlphaNumericCompare)
{
  // less than
  EXPECT_LT(StringUtils::AlphaNumericCompare(L"123abc", L"abc123"), 0);
  EXPECT_LT(StringUtils::AlphaNumericCompare(L"123abc", L"124abc"), 0);
  EXPECT_LT(StringUtils::AlphaNumericCompare(L"123abc", L"123bbc"), 0);
  EXPECT_LT(StringUtils::AlphaNumericCompare(L"abc123", L"abc124"), 0);
  EXPECT_LT(StringUtils::AlphaNumericCompare(L"abc123", L"bbc123"), 0);
  EXPECT_LT(StringUtils::AlphaNumericCompare(L"2", L"12"), 0);

  // equals
  EXPECT_EQ(StringUtils::AlphaNumericCompare(L"123abc", L"123abc"), 0);

  // greater than (same as less than but reversed arguments)
  EXPECT_GT(StringUtils::AlphaNumericCompare(L"abc123", L"123abc"), 0);
  EXPECT_GT(StringUtils::AlphaNumericCompare(L"124abc", L"123abc"), 0);
  EXPECT_GT(StringUtils::AlphaNumericCompare(L"123bbc", L"123abc"), 0);
  EXPECT_GT(StringUtils::AlphaNumericCompare(L"abc124", L"abc123"), 0);
  EXPECT_GT(StringUtils::AlphaNumericCompare(L"bbc123", L"abc123"), 0);
  EXPECT_GT(StringUtils::AlphaNumericCompare(L"12", L"2"), 0);

  // test that long numbers are treated correctly
  EXPECT_EQ(StringUtils::AlphaNumericCompare(L"12345678901234567890", L"12345678901234567890"), 0);
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
  std::string refstr, varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  StringUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(TestStringUtils, utf8_strlen)
{
  size_t ref, var;

  ref = 9;
  var = StringUtils::utf8_strlen("ｔｅｓｔ＿ＵＴＦ８");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, SecondsToTimeString)
{
  std::string ref, var;

  ref = "21:30:55";
  var = StringUtils::SecondsToTimeString(77455);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(TestStringUtils, MillisecondsToTimeString)
{
  EXPECT_STREQ(StringUtils::MillisecondsToTimeString(std::chrono::milliseconds(53254549)).c_str(),
               "14:47:34.549");
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
  std::string ref, var;

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
  size_t ref, var;

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
  size_t ref, var;

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
  int ref, var;

  ref = 11;
  var = StringUtils::FindEndBracket("atest testbb test", 'a', 'b');
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, DateStringToYYYYMMDD)
{
  int ref, var;

  ref = 20120706;
  var = StringUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, WordToDigits)
{
  std::string ref, var;

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
  double ref, var;

  ref = 6.25;
  var = StringUtils::CompareFuzzy("test string", "string test");
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, FindBestMatch)
{
  double refdouble, vardouble;
  int refint, varint;
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

TEST(TestStringUtils, ContainsKeyword)
{
  using namespace std::literals::string_literals;
  const auto str = "alpha beta gamma delta"s;
  EXPECT_TRUE(StringUtils::ContainsKeyword(str, std::vector{"alpha"s}));
  EXPECT_TRUE(StringUtils::ContainsKeyword(str, std::vector{"beta"s}));
  EXPECT_TRUE(StringUtils::ContainsKeyword(str, std::vector{"delta"s, "alpha"s}));
  EXPECT_TRUE(StringUtils::ContainsKeyword(str, std::vector{"other"s, "beta"s}));
  EXPECT_TRUE(StringUtils::ContainsKeyword(str, std::vector{"mm"s}));
  EXPECT_FALSE(StringUtils::ContainsKeyword(str, std::vector{"epsilon"s}));
  EXPECT_FALSE(StringUtils::ContainsKeyword(str, std::vector{"alphaa"s}));
  EXPECT_FALSE(StringUtils::ContainsKeyword(str, std::vector{"alpha gamma"s}));
}

TEST(TestStringUtils, BinaryStringToString)
{
  EXPECT_STREQ("abc", StringUtils::BinaryStringToString("\\97\\98\\99").c_str());
  EXPECT_STREQ("aabbcc", StringUtils::BinaryStringToString("a\\97\\98b\\99c").c_str());
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

TEST(TestStringUtils, Paramify)
{
  const char *input = "some, very \\ odd \"string\"";
  const char *ref   = "\"some, very \\\\ odd \\\"string\\\"\"";

  std::string result = StringUtils::Paramify(input);
  EXPECT_STREQ(ref, result.c_str());
}

TEST(TestStringUtils, DeParamify)
{
  std::string input("\"some, very \\\\ odd \\\"string\\\"\"");
  const std::string result = StringUtils::DeParamify(input);
  EXPECT_STREQ("some, very \\ odd \"string\"", result.c_str());
}

TEST(TestStringUtils, Tokenize)
{
  const std::string input("aaa,bbb;ccc,;ddd;");
  const std::string delims(",;.");
  const std::vector<std::string> result = StringUtils::Tokenize(input, delims);
  EXPECT_EQ(4, result.size());
  EXPECT_STREQ("aaa", result[0].c_str());
  EXPECT_STREQ("bbb", result[1].c_str());
  EXPECT_STREQ("ccc", result[2].c_str());
  EXPECT_STREQ("ddd", result[3].c_str());
}

TEST(TestStringUtils, ToUint32)
{
  EXPECT_EQ(0, StringUtils::ToUint32(""));
  EXPECT_EQ(0, StringUtils::ToUint32("abc"));
  EXPECT_EQ(0, StringUtils::ToUint32("abc123"));
  EXPECT_EQ(0, StringUtils::ToUint32("0"));
  EXPECT_EQ(0, StringUtils::ToUint32("-0"));
  EXPECT_EQ(127536, StringUtils::ToUint32("127536"));
  EXPECT_EQ(127536, StringUtils::ToUint32("127536abc"));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), StringUtils::ToUint32("-1"));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), StringUtils::ToUint32("4294967295"));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), StringUtils::ToUint32("999999999999"));
}

TEST(TestStringUtils, ToUint64)
{
  EXPECT_EQ(0, StringUtils::ToUint64(""));
  EXPECT_EQ(0, StringUtils::ToUint64("abc"));
  EXPECT_EQ(0, StringUtils::ToUint64("abc123"));
  EXPECT_EQ(0, StringUtils::ToUint64("0"));
  EXPECT_EQ(0, StringUtils::ToUint64("-0"));
  EXPECT_EQ(127536, StringUtils::ToUint64("127536"));
  EXPECT_EQ(127536, StringUtils::ToUint64("127536abc"));
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), StringUtils::ToUint64("-1"));
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), StringUtils::ToUint64("18446744073709551615"));
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(),
            StringUtils::ToUint64("9999999999999999999999999999"));
}

TEST(TestStringUtils, ToFloat)
{
  EXPECT_FLOAT_EQ(0.0, StringUtils::ToFloat(""));
  EXPECT_FLOAT_EQ(0.0, StringUtils::ToFloat("abc"));
  EXPECT_FLOAT_EQ(0.0, StringUtils::ToFloat("abc1.0"));
  EXPECT_FLOAT_EQ(0.0, StringUtils::ToFloat("0.0"));
  EXPECT_FLOAT_EQ(0.0, StringUtils::ToFloat("-0.0"));
  EXPECT_FLOAT_EQ(1.5, StringUtils::ToFloat("1.5"));
  EXPECT_FLOAT_EQ(-1.5, StringUtils::ToFloat("-1.5"));
}

TEST(TestStringUtils, FormatFileSize)
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

TEST(TestStringUtils, Contains)
{
  EXPECT_TRUE(StringUtils::Contains("abcDEFghi", "dEf"));
  EXPECT_FALSE(StringUtils::Contains("abcDEFghi", "dEf", false));
  EXPECT_FALSE(StringUtils::Contains("abcdefg", "cdfg"));
  EXPECT_FALSE(StringUtils::Contains("abcdefg", "cdfg", false));
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
