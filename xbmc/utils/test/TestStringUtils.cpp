/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <locale>
#include "utils/StringUtils.h"

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

TEST(TestStringUtils, Format)
{
  std::string refstr = "test 25 2.7 ff FF";

  std::string varstr = StringUtils::Format("{} {} {:.1f} {:x} {:02X}", "test", 25, 2.743f, 0x00ff,
      0x00ff);
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

TEST(TestStringUtils, AlphaNumericCompare)
{
  int64_t ref;
  int64_t var;

  ref = 0;
  var = StringUtils::AlphaNumericCompare(L"123abc", L"abc123");
  EXPECT_LT(var, ref);
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

TEST(TestStringUtils, FindEndBracket)
{
  int ref;
  int var;

  ref = 11;
  var = StringUtils::FindEndBracket("atest testbb test", 'a', 'b');
  EXPECT_EQ(ref, var);
}

TEST(TestStringUtils, CreateUUID)
{
  std::cout << "CreateUUID(): " << StringUtils::CreateUUID() << std::endl;
}

TEST(TestStringUtils, ValidateUUID)
{
  EXPECT_TRUE(StringUtils::ValidateUUID(StringUtils::CreateUUID()));
}

TEST(TestStringUtils, Tokenize)
{
  // \brief Split a string by the specified delimiters.

  //Splits a string using one or more delimiting characters, ignoring empty tokens.
  //Differs from Split() in two ways:
  //  1. The delimiters are treated as individual characters, rather than a single delimiting string.
  //  2. Empty tokens are ignored.
  // \return a vector of tokens

  std::vector<std::string> result;

  std::string input = "All good men:should not die!";
  std::string delimiters = "";
  result = StringUtils::Tokenize(input, delimiters);
  EXPECT_EQ(1, result.size());
  EXPECT_STREQ("All good men:should not die!", result[0].c_str());

  delimiters = " :!";
  result = StringUtils::Tokenize(input, delimiters);
  EXPECT_EQ(result.size(), 6);

  EXPECT_STREQ("All", result[0].c_str());
  EXPECT_STREQ("good", result[1].data());
  EXPECT_STREQ("men", result[2].data());
  EXPECT_STREQ("should", result[3].data());
  EXPECT_STREQ("not", result[4].data());
  EXPECT_STREQ("die", result[5].data());

  input = ":! All good men:should not die! :";
  result = StringUtils::Tokenize(input, delimiters);
  EXPECT_EQ(result.size(), 6);

  EXPECT_STREQ("All", result[0].c_str());
  EXPECT_STREQ("good", result[1].data());
  EXPECT_STREQ("men", result[2].data());
  EXPECT_STREQ("should", result[3].data());
  EXPECT_STREQ("not", result[4].data());
  EXPECT_STREQ("die", result[5].data());
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
  std::string a
  { "a\0b\n", 4 };
  EXPECT_STREQ("6100620a", StringUtils::ToHexadecimal(a).c_str());
  std::string nul
  { "\0", 1 };
  EXPECT_STREQ("00", StringUtils::ToHexadecimal(nul).c_str());
  std::string ff
  { "\xFF", 1 };
  EXPECT_STREQ("ff", StringUtils::ToHexadecimal(ff).c_str());
}

TEST(TestStringUtils, WordToDigits)
{
  // abc def ghi jkl mno pqrs tuv wxyz
  //"222 333 444 555 666 7777 888 9999";

  std::string ref;
  std::string var;

  ref = "8378 787464";
  var = "test string";
  StringUtils::WordToDigits(var);
  EXPECT_STREQ(ref.c_str(), var.c_str());

   ref = "83 8  8 464";
   var = "Te?t @t\tinG";
   StringUtils::WordToDigits(var);
   EXPECT_STREQ(ref.c_str(), var.c_str());
}
