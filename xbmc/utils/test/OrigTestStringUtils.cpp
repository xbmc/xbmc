/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
// clang-format off
#include "utils/test/OrigStringUtils.h"
#include "utils/StringUtils.h"

#include <algorithm>

#include <gtest/gtest.h>
enum class OrigECG
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
enum class OrigECN
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
TEST(OrigTestStringUtils, Format)
{
  std::string refstr = "test 25 2.7 ff FF";

  std::string varstr =
      OrigStringUtils::Format("{} {} {:.1f} {:x} {:02X}", "test", 25, 2.743f, 0x00ff, 0x00ff);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());

  varstr = OrigStringUtils::Format("", "test", 25, 2.743f, 0x00ff, 0x00ff);
  EXPECT_STREQ("", varstr.c_str());
}

TEST(OrigTestStringUtils, FormatEnum)
{
  const char* zero = "0";
  const char* one = "1";

  std::string varstr = OrigStringUtils::Format("{}", OrigECG::A);
  EXPECT_STREQ(zero, varstr.c_str());

  varstr = OrigStringUtils::Format("{}", EG::C);
  EXPECT_STREQ(zero, varstr.c_str());

  varstr = OrigStringUtils::Format("{}", test_enum::OrigECN::A);
  EXPECT_STREQ(one, varstr.c_str());

  varstr = OrigStringUtils::Format("{}", test_enum::EN::C);
  EXPECT_STREQ(one, varstr.c_str());
}

TEST(OrigTestStringUtils, FormatEnumWidth)
{
  const char* one = "01";

  std::string varstr = OrigStringUtils::Format("{:02d}", OrigECG::B);
  EXPECT_STREQ(one, varstr.c_str());

  varstr = OrigStringUtils::Format("{:02}", EG::D);
  EXPECT_STREQ(one, varstr.c_str());
}

TEST(OrigTestStringUtils, ToUpper)
{
  std::string refstr = "TEST";

  std::string varstr = "TeSt";
  OrigStringUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}


TEST(OrigTestStringUtils, ToUpperNew)
{
  std::string refstr = "TEST";

  std::string varstr = "TeSt";
  varstr = StringUtils::ToUpper(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(OrigTestStringUtils, ToLower)
{
  std::string refstr = "test";

  std::string varstr = "TeSt";
  OrigStringUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(OrigTestStringUtils, ToLowerNew)
{
  std::string refstr = "test";

  std::string varstr = "TeSt";
  varstr = StringUtils::ToLower(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}
TEST(OrigTestStringUtils, ToCapitalize)
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
TEST(OrigTestStringUtils, ToCapitalizeNew)
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

TEST(OrigTestStringUtils, EqualsNoCase)
{
  std::string refstr = "TeSt";

  EXPECT_TRUE(OrigStringUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(OrigStringUtils::EqualsNoCase(refstr, "tEsT"));
}

TEST(OrigTestStringUtils, EqualsNoCaseNew)
{
  std::string refstr = "TeSt";

  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "TeSt"));
  EXPECT_TRUE(StringUtils::EqualsNoCase(refstr, "tEsT"));
}

TEST(OrigTestStringUtils, Left)
{
  std::string refstr, varstr;
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


TEST(OrigTestStringUtils, LeftNew)
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
TEST(OrigTestStringUtils, Mid)
{
  std::string refstr, varstr;
  std::string origstr = "test";

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

TEST(OrigTestStringUtils, MidNew)
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

TEST(OrigTestStringUtils, RightNew)
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

TEST(OrigTestStringUtils, Trim)
{
  std::string refstr = "test test";

  std::string varstr = " test test   ";
  OrigStringUtils::Trim(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}


TEST(OrigTestStringUtils, TrimNew)
{
  std::string refstr = "test test";

  std::string varstr = " test test   ";
  StringUtils::Trim(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(OrigTestStringUtils, TrimLeft)
{
  std::string refstr = "test test   ";

  std::string varstr = " test test   ";
  OrigStringUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}


TEST(OrigTestStringUtils, TrimLeftNew)
{
  std::string refstr = "test test   ";

  std::string varstr = " test test   ";
  StringUtils::TrimLeft(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(OrigTestStringUtils, TrimRight)
{
  std::string refstr = " test test";

  std::string varstr = " test test   ";
  OrigStringUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}


TEST(OrigTestStringUtils, TrimRightNew)
{
  std::string refstr = " test test";

  std::string varstr = " test test   ";
  StringUtils::TrimRight(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(OrigTestStringUtils, Replace)
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


TEST(OrigTestStringUtils, ReplaceNew)
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

TEST(OrigTestStringUtils, StartsWith)
{
  std::string refstr = "test";

  EXPECT_FALSE(OrigStringUtils::StartsWithNoCase(refstr, "x"));

  EXPECT_TRUE(OrigStringUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(OrigStringUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(OrigStringUtils::StartsWith(refstr, "Te"));

  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(OrigStringUtils::StartsWithNoCase(refstr, "TesT"));
}


TEST(OrigTestStringUtils, StartsWithNew)
{
  std::string refstr = "test";

  EXPECT_FALSE(StringUtils::StartsWithNoCase(refstr, "x"));

  EXPECT_TRUE(StringUtils::StartsWith(refstr, "te"));
  EXPECT_TRUE(StringUtils::StartsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::StartsWith(refstr, "Te"));

  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "Te"));
  EXPECT_TRUE(StringUtils::StartsWithNoCase(refstr, "TesT"));
}

TEST(OrigTestStringUtils, EndsWith)
{
  std::string refstr = "test";

  EXPECT_FALSE(OrigStringUtils::EndsWithNoCase(refstr, "x"));

  EXPECT_TRUE(OrigStringUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(OrigStringUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(OrigStringUtils::EndsWith(refstr, "sT"));

  EXPECT_TRUE(OrigStringUtils::EndsWithNoCase(refstr, "sT"));
  EXPECT_TRUE(OrigStringUtils::EndsWithNoCase(refstr, "TesT"));
}


TEST(OrigTestStringUtils, EndsWithNew)
{
  std::string refstr = "test";

  EXPECT_FALSE(StringUtils::EndsWithNoCase(refstr, "x"));

  EXPECT_TRUE(StringUtils::EndsWith(refstr, "st"));
  EXPECT_TRUE(StringUtils::EndsWith(refstr, "test"));
  EXPECT_FALSE(StringUtils::EndsWith(refstr, "sT"));

  EXPECT_TRUE(StringUtils::EndsWithNoCase(refstr, "sT"));
  EXPECT_TRUE(StringUtils::EndsWithNoCase(refstr, "TesT"));
}

TEST(OrigTestStringUtils, Join)
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
  varstr = OrigStringUtils::Join(strarray, ",");
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}


TEST(OrigTestStringUtils, JoinNew)
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

TEST(OrigTestStringUtils, Split)
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
  EXPECT_STREQ("d ef ghi ", OrigStringUtils::Split("a bc  d ef ghi ", " ", 4).at(3).c_str()) << "Last part must include rest of the input string";
  EXPECT_EQ(7U, OrigStringUtils::Split("a bc  d ef ghi ", " ").size()) << "Result must be 7 strings including two empty strings";
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
  EXPECT_EQ(7U, OrigStringUtils::Split("a bc  d ef ghi ", ' ').size()) << "Result must be 7 strings including two empty strings";
  EXPECT_STREQ("bc", OrigStringUtils::Split("a bc  d ef ghi ", ' ').at(1).c_str());
  EXPECT_STREQ("", OrigStringUtils::Split("a bc  d ef ghi ", ' ').at(2).c_str());
  EXPECT_STREQ("", OrigStringUtils::Split("a bc  d ef ghi ", ' ').at(6).c_str());

  EXPECT_EQ(1U, OrigStringUtils::Split("a bc  d ef ghi ", 'z').size());
  EXPECT_STREQ("a bc  d ef ghi ", OrigStringUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());

  EXPECT_EQ(1U, OrigStringUtils::Split("a bc  d ef ghi ", "").size());
  EXPECT_STREQ("a bc  d ef ghi ", OrigStringUtils::Split("a bc  d ef ghi ", 'z').at(0).c_str());
}


TEST(OrigTestStringUtils, SplitNew)
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

TEST(OrigTestStringUtils, FindNumber)
{
  EXPECT_EQ(3, StringUtils::FindNumber("aabcaadeaa", "aa"));
  EXPECT_EQ(1, StringUtils::FindNumber("aabcaadeaa", "b"));
}

TEST(OrigTestStringUtils, AlphaNumericCompare)
{
  int64_t ref, var;

  ref = 0;
  var = OrigStringUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
}


TEST(OrigTestStringUtils, AlphaNumericCompareNew)
{
  int64_t ref, var;

  ref = 0;
  var = StringUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
}

TEST(OrigTestStringUtils, TimeStringToSeconds)
{
  EXPECT_EQ(77455, OrigStringUtils::TimeStringToSeconds("21:30:55"));
  EXPECT_EQ(7*60, OrigStringUtils::TimeStringToSeconds("7 min"));
  EXPECT_EQ(7*60, OrigStringUtils::TimeStringToSeconds("7 min\t"));
  EXPECT_EQ(154*60, OrigStringUtils::TimeStringToSeconds("   154 min"));
  EXPECT_EQ(1*60+1, OrigStringUtils::TimeStringToSeconds("1:01"));
  EXPECT_EQ(4*60+3, OrigStringUtils::TimeStringToSeconds("4:03"));
  EXPECT_EQ(2*3600+4*60+3, OrigStringUtils::TimeStringToSeconds("2:04:03"));
  EXPECT_EQ(2*3600+4*60+3, OrigStringUtils::TimeStringToSeconds("   2:4:3"));
  EXPECT_EQ(2*3600+4*60+3, OrigStringUtils::TimeStringToSeconds("  \t\t 02:04:03 \n "));
  EXPECT_EQ(1*3600+5*60+2, OrigStringUtils::TimeStringToSeconds("01:05:02:04:03 \n "));
  EXPECT_EQ(0, OrigStringUtils::TimeStringToSeconds("blah"));
  EXPECT_EQ(0, OrigStringUtils::TimeStringToSeconds("ля-ля"));
}


TEST(OrigTestStringUtils, TimeStringToSecondsNew)
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

TEST(OrigTestStringUtils, RemoveCRLF)
{
  std::string refstr, varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  OrigStringUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}


TEST(OrigTestStringUtils, RemoveCRLFNew)
{
  std::string refstr, varstr;

  refstr = "test\r\nstring\nblah blah";
  varstr = "test\r\nstring\nblah blah\n";
  StringUtils::RemoveCRLF(varstr);
  EXPECT_STREQ(refstr.c_str(), varstr.c_str());
}

TEST(OrigTestStringUtils, utf8_strlen)
{
  size_t ref, var;

  ref = 9;
  var = OrigStringUtils::utf8_strlen("ｔｅｓｔ＿ＵＴＦ８");
  EXPECT_EQ(ref, var);
}

TEST(OrigTestStringUtils, utf8_strlenNew)
{
  size_t ref, var;

  ref = 9;
  var = StringUtils::utf8_strlen("ｔｅｓｔ＿ＵＴＦ８");
  EXPECT_EQ(ref, var);
}

TEST(OrigTestStringUtils, SecondsToTimeString)
{
  std::string ref, var;

  ref = "21:30:55";
  var = OrigStringUtils::SecondsToTimeString(77455);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}


TEST(OrigTestStringUtils, SecondsToTimeStringNew)
{
  std::string ref, var;

  ref = "21:30:55";
  var = StringUtils::SecondsToTimeString(77455);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}


TEST(OrigTestStringUtils, IsNaturalNumber)
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


TEST(OrigTestStringUtils, IsNaturalNumberNew)
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

TEST(OrigTestStringUtils, IsInteger)
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

TEST(OrigTestStringUtils, IsIntegerNew)
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

TEST(OrigTestStringUtils, SizeToString)
{
  std::string ref, var;

  ref = "2.00 GB";
  var = OrigStringUtils::SizeToString(2147483647);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "0.00 B";
  var = OrigStringUtils::SizeToString(0);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(OrigTestStringUtils, SizeToStringNew)
{
  std::string ref, var;

  ref = "2.00 GB";
  var = StringUtils::SizeToString(2147483647);
  EXPECT_STREQ(ref.c_str(), var.c_str());

  ref = "0.00 B";
  var = StringUtils::SizeToString(0);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(OrigTestStringUtils, EmptyString)
{
  EXPECT_STREQ("", OrigStringUtils::Empty.c_str());
}


TEST(OrigTestStringUtils, EmptyStringNew)
{
  EXPECT_STREQ("", StringUtils::Empty.c_str());
}

TEST(OrigTestStringUtils, FindWords)
{
  size_t ref, var;

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


TEST(OrigTestStringUtils, FindWordsNew)
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

TEST(OrigTestStringUtils, FindWords_NonAscii)
{
  size_t ref, var;

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

TEST(OrigTestStringUtils, FindWords_NonAsciiNew)
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

TEST(OrigTestStringUtils, FindEndBracket)
{
  int ref, var;

  ref = 11;
  var = OrigStringUtils::FindEndBracket("atest testbb test", 'a', 'b');
  EXPECT_EQ(ref, var);
}


TEST(OrigTestStringUtils, FindEndBracketNew)
{
  int ref, var;

  ref = 11;
  var = StringUtils::FindEndBracket("atest testbb test", 'a', 'b');
  EXPECT_EQ(ref, var);
}

TEST(OrigTestStringUtils, DateStringToYYYYMMDD)
{
  int ref, var;

  ref = 20120706;
  var = OrigStringUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);
}

TEST(OrigTestStringUtils, DateStringToYYYYMMDDNew)
{
  int ref, var;

  ref = 20120706;
  var = StringUtils::DateStringToYYYYMMDD("2012-07-06");
  EXPECT_EQ(ref, var);
}

TEST(OrigTestStringUtils, WordToDigits)
{
  std::string ref, var;

  ref = "8378 787464";
  var = "test string";
  OrigStringUtils::WordToDigits(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(OrigTestStringUtils, WordToDigitsNew)
{
  std::string ref, var;

  ref = "8378 787464";
  var = "test string";
  StringUtils::WordToDigits(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());
}

TEST(OrigTestStringUtils, CreateUUID)
{
  std::cout << "CreateUUID(): " << OrigStringUtils::CreateUUID() << std::endl;
}


TEST(OrigTestStringUtils, CreateUUIDNew)
{
  std::cout << "CreateUUID(): " << StringUtils::CreateUUID() << std::endl;
}

TEST(OrigTestStringUtils, ValidateUUID)
{
  EXPECT_TRUE(OrigStringUtils::ValidateUUID(OrigStringUtils::CreateUUID()));
}


TEST(OrigTestStringUtils, ValidateUUIDNew)
{
  EXPECT_TRUE(StringUtils::ValidateUUID(OrigStringUtils::CreateUUID()));
}

TEST(OrigTestStringUtils, CompareFuzzy)
{
  double ref, var;

  ref = 6.25;
  var = OrigStringUtils::CompareFuzzy("test string", "string test");
  EXPECT_EQ(ref, var);
}

TEST(OrigTestStringUtils, CompareFuzzyNew)
{
  double ref, var;

  ref = 6.25;
  var = StringUtils::CompareFuzzy("test string", "string test");
  EXPECT_EQ(ref, var);
}

TEST(OrigTestStringUtils, FindBestMatch)
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
  varint = OrigStringUtils::FindBestMatch("test", strarray, vardouble);
  EXPECT_EQ(refint, varint);
  EXPECT_EQ(refdouble, vardouble);
}

TEST(OrigTestStringUtils, FindBestMatchNew)
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

TEST(OrigTestStringUtils, Paramify)
{
  const char *input = "some, very \\ odd \"string\"";
  const char *ref   = "\"some, very \\\\ odd \\\"string\\\"\"";

  std::string result = OrigStringUtils::Paramify(input);
  EXPECT_STREQ(ref, result.c_str());
}


TEST(OrigTestStringUtils, ParamifyNew)
{
  const char *input = "some, very \\ odd \"string\"";
  const char *ref   = "\"some, very \\\\ odd \\\"string\\\"\"";

  std::string result = StringUtils::Paramify(input);
  EXPECT_STREQ(ref, result.c_str());
}

TEST(OrigTestStringUtils, sortstringbyname)
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

TEST(OrigTestStringUtils, sortstringbynameNew)
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

TEST(OrigTestStringUtils, FileSizeFormat)
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

TEST(OrigTestStringUtils, FileSizeFormatNew)
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

TEST(OrigTestStringUtils, ToHexadecimal)
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

TEST(OrigTestStringUtils, ToHexadecimalNew)
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
