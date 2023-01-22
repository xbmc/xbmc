/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/UnicodeConverter.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string_view>

#include <gtest/gtest.h>

using namespace std::literals;
using namespace std::chrono_literals;

#if defined(TARGET_DARWIN)
constexpr bool IS_DARWIN{true};
#else
constexpr bool IS_DARWIN{false};
#endif

namespace
{
static constexpr std::string_view UTF8_SUBSTITUTE_CHARACTER{"\xef\xbf\xbd"sv};
// static constexpr std::wstring_view WCHAR_T_SUBSTITUTE_CHARACTER{L"\xfffd"sv};
static constexpr std::u32string_view CHAR32_T_SUBSTITUTE_CHARACTER{U"\x0000fffd"sv}; //U'�'sv
static constexpr bool REPORT_PERFORMANCE_DETAILS{true};

static int Compare(const std::u32string_view str1,
                   const std::u32string_view str2,
                   const size_t n = 0)
{
  // n is the maximum number of Unicode codepoints (for practical purposes
  // equivalent to characters).
  //
  // Much better to avoid using n by using StartsWith or EndsWith, etc.
  //
  // if n == 0, then do simple utf8 compare
  //
  // Otherwise, convert to Unicode then compare codepoints.
  //

  if (n == 0)
    return str1.compare(str2);

  return str1.compare(0, n, str2, 0, n);
}

static int Compare(const std::wstring_view str1, const std::wstring_view str2, const size_t n = 0)
{
  // n is the maximum number of Unicode codepoints (for practical purposes
  // equivalent to characters).
  //
  // Much better to avoid using n by using StartsWith or EndsWith, etc.
  //
  // if n == 0, then do simple utf8 compare
  //
  // Otherwise, convert to Unicode then compare, then compare codepoints.
  //

  if (n == 0)
    return str1.compare(str2);

  // Have to convert to Unicode codepoints

  std::u32string utf32Str1 = StringUtils::ToUtf32(str1);
  std::u32string utf32Str2 = StringUtils::ToUtf32(str2);

  return utf32Str1.compare(0, n, utf32Str2, 0, n);
}

static bool Equals(const std::string_view str1, const std::string_view str2)
{
  return StringUtils::Equals(str1, str2);
}

static bool Equals(const std::u32string_view str1, const std::u32string_view str2)
{
  return Compare(str1, str2) == 0;
}

static bool Equals(const std::wstring_view str1, const std::wstring_view str2)
{
  return Compare(str1, str2) == 0;
}

TEST(TestCUnicodeConverter, utf8ToUtf32)
{
  std::string_view simpleTest{"This is a simpleTest"};
  std::wstring_view wSimpleTest{L"This is a simpleTest"};
  std::u32string_view u32SimpleTest{U"This is a simpleTest"};

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32("This is a simpleTest"sv);
  EXPECT_TRUE(Equals(u32Result, u32SimpleTest));

  std::wstring wResult = CUnicodeConverter::Utf8ToW(simpleTest);
  EXPECT_TRUE(Equals(wResult, wSimpleTest));

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  EXPECT_TRUE(Equals(simpleTest, utf8Result));

  utf8Result = CUnicodeConverter::WToUtf8(wSimpleTest);
  EXPECT_TRUE(Equals(simpleTest, utf8Result));

  wResult = CUnicodeConverter::Utf32ToW(u32SimpleTest);
  EXPECT_TRUE(Equals(wSimpleTest, wResult));

  u32Result = CUnicodeConverter::WToUtf32(wSimpleTest);
  EXPECT_TRUE(Equals(u32SimpleTest, u32Result));
}

TEST(TestCUnicodeConverter, BadUnicode)
{
  // Verify that code doesn't crater on malformed/bad Unicode. Is NOT an exhaustive
  // list.

  // Unassigned, private use or surrogates
  // Most do not trigger an error in wstring_convert (the Bad_UTF8 ones do).

  /*   static const char32_t BAD_CHARS[] = {
   U'\xFDD0',   U'\xFDEF',  U'\xFFFE',  U'\xFFFF',  U'\x1FFFE', U'\x1FFFF', U'\x2FFFE',
   U'\x2FFFF',  U'\x3FFFE', U'\x3FFFF', U'\x4FFFE', U'\x4FFFF', U'\x5FFFE', U'\x5FFFF',
   U'\x6FFFE',  U'\x6FFFF', U'\x7FFFE', U'\x7FFFF', U'\x8FFFE', U'\x8FFFF', U'\x9FFFE',
   U'\x9FFFF',  U'\xAFFFE', U'\xAFFFF', U'\xBFFFE', U'\xBFFFF', U'\xCFFFE', U'\xCFFFF',
   U'\xDFFFE',  U'\xDFFFF', U'\xEFFFE', U'\xEFFFF', U'\xFFFFE', U'\xFFFFF', U'\x10FFFE',
   U'\x10FFFF'};

   for (char32_t c : BAD_CHARS)
   {
   std::u32string s(1, c);
   std::string utf8str = CUnicodeConverter::Utf32ToUtf8(s);

   std::cout << "u32string: " << StringUtils::ToHex(s)  << " utf8: " << utf8str <<
   " " << StringUtils::ToHex(utf8str) << " sub: " << UTF8_SUBSTITUTE_CHARACTER
   << " " << StringUtils::ToHex(UTF8_SUBSTITUTE_CHARACTER) << std::endl;

   EXPECT_TRUE(StringUtils::Equals(utf8str, UTF8_SUBSTITUTE_CHARACTER));
   }
   EXPECT_TRUE(true);  */

  static const char32_t REPLACED_CHARS[] = {U'\x0D800', U'\x0D801', U'\x0D802', U'\x0D803',
                                            U'\x0D804', U'\x0D805', U'\xDB7F',  U'\xDB80',
                                            U'\xDBFF',  U'\xDC00',  U'\x0DFFF'};

  for (char32_t c : REPLACED_CHARS)
  {
    std::u32string s(1, c);
    std::string utf8str = CUnicodeConverter::Utf32ToUtf8(s);
    EXPECT_TRUE(StringUtils::Equals(utf8str, UTF8_SUBSTITUTE_CHARACTER));

    // std::cout << "Bad Char: " << std::hex << s[0] << " converted to UTF8: "
    //     << std::hex << utf8str
    //    << " expected: " << std::hex << UTF8_SUBSTITUTE_CHARACTER << std::endl;
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
    std::u32string trash = StringUtils::ToUtf32(str);
    EXPECT_EQ(trash[0], CHAR32_T_SUBSTITUTE_CHARACTER[0]);
    EXPECT_EQ(Compare(trash, CHAR32_T_SUBSTITUTE_CHARACTER), 0);
  }
  EXPECT_TRUE(true);

  // TODO: These are single char tests. Add more advanced tests:
  // First char in multi-char string bad
  // Last char in multi-char string bad
  // Next to last char bad
  // second char bad
  // Every other char bad with first and last bad
  // Every other char bad with second and next to last char bad
  // Multiple consecutive bad chars at start, end, middle and next to end of string
  // Include wchar_t
}

TEST(TestCUnicodeConverter, SubstituteStart)
{
  bool testPassed;

  // badStartUTF8 is a string that starts with an invalid utf8 code-unit
  //
  // Convert it to wstring and u32string and verify that this results in
  // the first code-unit in each being the substitution character.
  // Also, covert both converted wstring and u32string (with substitution char)
  // back to utf8 and verify that it has a substitution char.

  std::string_view badStartUTF8{"\xcc"sv
                                "This is a simpleTest"sv};

  // std::cout << "badStartUTF8: " << badStartUTF8 << " Hex: " << StringUtils::ToHex(badStartUTF8) << std::endl;

  std::string_view utf8ExpectedResult{"\xef\xbf\xbdThis is a simpleTest"};
  std::wstring_view wExpectedResult{L"\x0fffdThis is a simpleTest"};
  std::u32string_view u32ExpectedResult{U"\x0fffdThis is a simpleTest"};

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badStartUTF8);
  //std::cout << "u32Result: " << StringUtils::ToHex(u32Result)  << " utf8: " << badStartUTF8 <<
  //       " " << StringUtils::ToHex(badStartUTF8) << " sub: " << UTF8_SUBSTITUTE_CHARACTER
  //        << " " << StringUtils::ToHex(UTF8_SUBSTITUTE_CHARACTER) << std::endl;

  EXPECT_TRUE(Equals(u32Result, u32ExpectedResult));

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badStartUTF8);
  EXPECT_TRUE(Equals(wResult, wExpectedResult));

  // Convert both wstring and u32string (with substitution code-unit, from above)
  // back to utf8

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  utf8Result = CUnicodeConverter::WToUtf8(wResult);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  // Now verify that a bad first codeunit for wstring and u32string
  // produce substitution char when converting to string

  std::u32string_view badStartUtf32{U"\x0D800This is a simpleTest"};
  utf8Result = CUnicodeConverter::Utf32ToUtf8(badStartUtf32);
  testPassed = Equals(u32Result, u32ExpectedResult);

  if (!testPassed)
  {
    std::cout << "badStartUtf32: "
              << " Hex: " << StringUtils::ToHex(badStartUtf32) << std::endl;
    std::cout << "utf8Result: " << utf8Result << " Hex: " << StringUtils::ToHex(utf8Result)
              << std::endl;
  }
  EXPECT_TRUE(testPassed);

  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    utf8Result = CUnicodeConverter::WToUtf8(wExpectedResult);
    std::cout << "badStartUtf32: "
              << " Hex: " << StringUtils::ToHex(badStartUtf32) << std::endl;
    std::cout << "utf8Result: " << utf8Result << " Hex: " << StringUtils::ToHex(utf8Result)
              << std::endl;
  }
  EXPECT_TRUE(testPassed);
}

TEST(TestCUnicodeConverter, SubstituteEnd)
{
  // badEndUTF8 is a string that ends with an invalid utf8 code-unit
  //
  // Convert it to wstring and u32string and verify that this results in
  // the last code-unit in each being the substitution character.
  // Also, convert both converted wstring and u32string (with substitution char)
  // back to utf8 and verify that it has a substitution char.

  // Mac OSX iconv appears to have a bug. If last utf-8 (perhaps more) character
  // is malformed, then it marks the character before it also as bad.

  if (!IS_DARWIN)
  {
    std::string_view badEndUTF8{"This is a simpleTest\x9f"sv};

    std::string_view utf8ExpectedResult{"This is a simpleTest\xef\xbf\xbd"};
    std::wstring_view wExpectedResult{L"This is a simpleTest\x0fffd"};
    std::u32string_view u32ExpectedResult{U"This is a simpleTest\x0fffd"};

    std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badEndUTF8);

    bool testPassed = Equals(u32Result, u32ExpectedResult);
    if (!testPassed)
    {
      // FYI: Maximum codepoint supported by OSX is around FF50;

      std::cout << " FAIL 1: utf8: " << badEndUTF8 << " " << StringUtils::ToHex(badEndUTF8)
                << std::endl;

      std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << std::endl;
      std::cout << "u32ExpectedResult: " << StringUtils::ToHex(u32ExpectedResult) << std::endl
                << std::endl;
    }
    EXPECT_TRUE(testPassed);

    std::wstring wResult = CUnicodeConverter::Utf8ToW(badEndUTF8);
    testPassed = Equals(wResult, wExpectedResult);
    if (!testPassed)
    {
      // Maximum codepoint supported by the OSX is around FF50;
      std::cout << " FAIL 2: utf8: " << badEndUTF8 << " " << StringUtils::ToHex(badEndUTF8)
                << std::endl;

      std::cout << "wResult: " << StringUtils::ToHex(wResult) << std::endl;
      std::cout << "u32Result: " << StringUtils::ToHex(u32ExpectedResult) << std::endl << std::endl;
    }
    EXPECT_TRUE(testPassed);

    // Convert both wstring and u32string (with substitution code-unit, from above)
    // back to utf8

    std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
    EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

    utf8Result = CUnicodeConverter::WToUtf8(wResult);
    EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

    // ==========================
    // Now verify that a bad last codeunit for wstring and u32string
    // produce substitution char when converting to string

    // std::wstring_view badEndW{L"This is a simpleTest\x0D800"};
    std::u32string_view badEndUtf32{U"This is a simpleTest\x0D800"};

    utf8Result = CUnicodeConverter::Utf32ToUtf8(badEndUtf32);
    EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

    utf8Result = CUnicodeConverter::WToUtf8(wExpectedResult);
    EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));
  }
  // Repeat above experiment, but bad char will be next to the last char

  {
    std::string_view badEndUTF8{"This is a simpleTest \x9f."sv};
    std::string_view utf8ExpectedResult{"This is a simpleTest \xef\xbf\xbd."};
    std::wstring_view wExpectedResult;
    std::u32string_view u32ExpectedResult;
    if (!IS_DARWIN)
    {
      u32ExpectedResult = U"This is a simpleTest \x0fffd."sv;
      wExpectedResult = L"This is a simpleTest \x0fffd."sv;
    }
    else
    {
      // No space, two substitutions
      u32ExpectedResult = U"This is a simpleTest\x0fffd\x0fffd."sv;
      wExpectedResult = L"This is a simpleTest\x0fffd\x0fffd."sv;
    }
    std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badEndUTF8);
    bool testPassed = Equals(u32Result, u32ExpectedResult);
    if (!testPassed)
    {
      // Maximum codepoint supported by the OSX that we are using
      // is around FF50;
      std::cout << " FAIL 3: utf8: " << badEndUTF8 << " " << StringUtils::ToHex(badEndUTF8)
                << std::endl;

      std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << std::endl;
      std::cout << "u32ExpectedResult: " << StringUtils::ToHex(u32ExpectedResult) << std::endl
                << std::endl;
    }
    // if (!IS_DARWIN)
    //  EXPECT_TRUE(testPassed);

    std::wstring wResult = CUnicodeConverter::Utf8ToW(badEndUTF8);
    testPassed = Equals(wResult, wExpectedResult);
    if (!testPassed)
    {
      // Maximum codepoint supported by the OSX that we are using
      // is around FF50;
      std::cout << " FAIL 4: utf8: " << badEndUTF8 << " " << StringUtils::ToHex(badEndUTF8)
                << std::endl;

      std::cout << "wResult: " << StringUtils::ToHex(wResult) << std::endl;
      std::cout << "wExpectedResult: " << StringUtils::ToHex(wExpectedResult) << std::endl
                << std::endl;
    }
    if (!IS_DARWIN)
    {
      EXPECT_TRUE(testPassed);
    }
    if (IS_DARWIN)
    {
      // OSX, same as before.
      utf8ExpectedResult = "This is a simpleTest\0xef\0xbf\0xbd\0xef\0xbf\0xbd."s;
    }
    // Convert both wstring and u32string (with substitution code-unit, from above)
    // back to utf8

    std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
    testPassed = Equals(utf8Result, utf8ExpectedResult);
    if (!testPassed)
    {
      // Maximum codepoint supported by the OSX that we are using
      // is around FF50;
      std::cout << " FAIL 5: input- u32Result: " << StringUtils::ToHex(u32Result) << std::endl;

      std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
      std::cout << "utf8ExpectedResult: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl
                << std::endl;
    }
    if (!IS_DARWIN)
    {
      EXPECT_TRUE(testPassed);
    }
    utf8Result = CUnicodeConverter::WToUtf8(wResult);
    testPassed = Equals(utf8Result, utf8ExpectedResult);
    if (!testPassed)
    {
      // Maximum codepoint supported by the OSX that we are using
      // is around FF50;
      std::cout << " FAIL 6: input- wResult: " << StringUtils::ToHex(wResult) << std::endl;

      std::cout << "wResult: " << StringUtils::ToHex(wResult) << std::endl;
      std::cout << "utf8ExpectedResult: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl
                << std::endl;
    }
    if (!IS_DARWIN)
    {
      EXPECT_TRUE(testPassed);
    }

    // Now verify that a bad last codeunit for wstring and u32string
    // produce substitution char when converting to string

    std::u32string_view badEndUtf32{U"This is a simpleTest \x0D800."};
    utf8Result = CUnicodeConverter::Utf32ToUtf8(badEndUtf32);
    testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
    if (!testPassed)
    {
      // Maximum codepoint supported by the OSX that we are using
      // is around FF50;
      std::cout << " FAIL 7: badEndUtf32: " << StringUtils::ToHex(badEndUtf32) << std::endl;

      std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
      std::cout << "utf8ExpectedResult: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl
                << std::endl;
    }
    if (!IS_DARWIN)
    {
      EXPECT_TRUE(testPassed);
    }
    utf8Result = CUnicodeConverter::WToUtf8(wResult);
    testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
    if (!testPassed)
    {
      // Maximum codepoint supported by the OSX that we are using
      // is around FF50;
      std::cout << " FAIL8 : wResult: " << StringUtils::ToHex(wResult) << std::endl;

      std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
      std::cout << "utf8ExpectedResult: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl
                << std::endl;
    }
    if (!IS_DARWIN)
    {
      EXPECT_TRUE(testPassed);
    }
  }
}

TEST(TestCUnicodeConverter, SubstituteMiddle)
{
  if (IS_DARWIN)
  {
    return;
  }

  // Convert it to wstring and u32string and verify that this results in
  // the middle bad code-unit in each being the substitution character.
  // Also, covert both converted wstring and u32string (with substitution char)
  // back to utf8 and verify that it has a substitution char.

  std::string_view badMiddleUTF8{"This is a\xcc simpleTest"sv};
  std::string_view utf8ExpectedResult{"This is a\xef\xbf\xbd simpleTest"};
  std::wstring_view wExpectedResult{L"This is a\x0fffd simpleTest"};
  std::u32string_view u32ExpectedResult{U"This is a\x0fffd simpleTest"};

  // DARWIN appears buggy

  if (IS_DARWIN)
  {
    badMiddleUTF8 = "This is a\xcc simpleTest"sv;
    utf8ExpectedResult = "This is \xef\xbf\xbd\xef\xbf\xbd simpleTest";
    wExpectedResult = L"This is \x0fffd\x0fffd simpleTest";
    u32ExpectedResult = U"This is \x0fffd\x0fffd simpleTest";
  }
  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badMiddleUTF8);

  bool testPassed = Equals(u32Result, u32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "badMiddleUTF8: " << badMiddleUTF8 << " Hex: " << StringUtils::ToHex(badMiddleUTF8)
              << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << " utf8: " << badMiddleUTF8 << " "
              << StringUtils::ToHex(badMiddleUTF8) << " sub: " << UTF8_SUBSTITUTE_CHARACTER << " "
              << StringUtils::ToHex(UTF8_SUBSTITUTE_CHARACTER) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badMiddleUTF8);
  testPassed = Equals(wResult, wExpectedResult);
  if (!testPassed)
  {
    std::cout << "badMiddleUTF8: " << badMiddleUTF8 << " Hex: " << StringUtils::ToHex(badMiddleUTF8)
              << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(wResult) << std::endl;
    std::cout << " wExpectedResult: " << StringUtils::ToHex(wExpectedResult) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // Convert both wstring and u32string (with substitution code-unit, from above)
  // back to utf8

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);

  EXPECT_TRUE(testPassed);

  utf8Result = CUnicodeConverter::WToUtf8(wResult);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);

  EXPECT_TRUE(testPassed);

  // Now verify that a bad first codeunit for wstring and u32string
  // produce substitution char when converting to string

  // std::wstring_view badMiddleW{L"This is a\x0D800 simpleTest"};
  std::u32string_view badMiddleUtf32{U"This is a\x0D800 simpleTest"};
  utf8Result = CUnicodeConverter::Utf32ToUtf8(badMiddleUtf32);

  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "badMiddleUtf32: "
              << " Hex: " << StringUtils::ToHex(badMiddleUtf32) << std::endl;
    std::cout << "utf8Result: " << utf8Result << " Hex: " << StringUtils::ToHex(utf8Result)
              << std::endl;
  }
  EXPECT_TRUE(testPassed);

  utf8Result = CUnicodeConverter::WToUtf8(wExpectedResult);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "badMiddleUtf32: "
              << " Hex: " << StringUtils::ToHex(badMiddleUtf32) << std::endl;
    std::cout << "utf8Result: " << utf8Result << " Hex: " << StringUtils::ToHex(utf8Result)
              << std::endl;
  }
  EXPECT_TRUE(testPassed);
}

TEST(TestCUnicodeConverter, SubstituteStart2)
{
  // Convert it to wstring and u32string and verify that this results in
  // consecutive bad code-units being replaced with a single substitution
  // codepoint . This occurs even if the two malformed code-units
  // belong to different malformed codepoints.
  // Also, covert both converted wstring and u32string (with substitution char)
  // back to utf8 and verify that it has a substitution char.

  bool testPassed;
  std::string_view badStart2UTF8{"\xcc\xcdThis is a simpleTest"sv};
  std::string_view utf8ExpectedResult{"\xef\xbf\xbdThis is a simpleTest"};
  std::wstring_view wExpectedResult{L"\x0fffdThis is a simpleTest"};
  std::u32string_view u32ExpectedResult{U"\x0fffdThis is a simpleTest"};

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badStart2UTF8);
  testPassed = Equals(u32Result, u32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << " utf8: " << badStart2UTF8 << " "
              << StringUtils::ToHex(badStart2UTF8) << " sub: " << UTF8_SUBSTITUTE_CHARACTER << " "
              << StringUtils::ToHex(UTF8_SUBSTITUTE_CHARACTER) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badStart2UTF8);
  EXPECT_TRUE(Equals(wResult, wExpectedResult));

  // Convert both wstring and u32string (with substitution code-unit, from above)
  // back to utf8

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  utf8Result = CUnicodeConverter::WToUtf8(wResult);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  // Now verify that a bad first codeunit for wstring and u32string
  // produce substitution char when converting to string

  // std::wstring_view badStart2W{L"\x0D800\x0D801This is a simpleTest"};
  std::u32string_view badStart2Utf32{U"\x0D800\x0D801This is a simpleTest"};
  utf8Result = CUnicodeConverter::Utf32ToUtf8(badStart2Utf32);

  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "badStart2Utf32: "
              << " Hex: " << StringUtils::ToHex(badStart2Utf32) << std::endl;
    std::cout << "utf8Result: " << utf8Result << " Hex: " << StringUtils::ToHex(utf8Result)
              << std::endl;
  }
  EXPECT_TRUE(testPassed);

  utf8Result = CUnicodeConverter::WToUtf8(wExpectedResult);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "badStart2Utf32: "
              << " Hex: " << StringUtils::ToHex(badStart2Utf32) << std::endl;
    std::cout << "utf8Result: " << utf8Result << " Hex: " << StringUtils::ToHex(utf8Result)
              << std::endl;
  }
  EXPECT_TRUE(testPassed);
}

TEST(TestCUnicodeConverter, SubstituteMulti)
{
  if (IS_DARWIN)
  {
    return;
  }

  // This tests having multiple bad chars within a string

  // Test with UTF8 malformed character sequence: 2 bad codepoints in a row. Both have
  // proper start bytes, but invalid middle bytes. This sequence will show
  // up at beginning, middle and end of same string.

  bool testPassed;

  std::string_view badStart2UTF8{
      "\xcc\xd0\xc5\xd1\xd2\xd3\xcdThis is \xcc\xd0\xc5\xd1\xd3\xcd a simpleTest\xcc\xd0\xd1\xd2\xd3\xcd"sv};

  std::string_view utf8ExpectedResult{"\xef\xbf\xbdThis is \xef\xbf\xbd a "
                                      "simpleTest\xef\xbf\xbd"};
  std::wstring_view wExpectedResult{L"\x0fffdThis is \x0fffd a simpleTest\x0fffd"};
  std::u32string_view u32ExpectedResult{U"\x0fffdThis is \x0fffd a simpleTest\x0fffd"};

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badStart2UTF8);
  testPassed = Equals(u32Result, u32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << " utf8: " << badStart2UTF8 << " "
              << StringUtils::ToHex(badStart2UTF8) << " sub: " << UTF8_SUBSTITUTE_CHARACTER << " "
              << StringUtils::ToHex(UTF8_SUBSTITUTE_CHARACTER) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badStart2UTF8);
  EXPECT_TRUE(Equals(wResult, wExpectedResult));

  // Convert both wstring and u32string (with substitution code-unit, from above)
  // back to utf8

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  utf8Result = CUnicodeConverter::WToUtf8(wResult);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));
}

TEST(TestCUnicodeConverter, SkipMulti)
{
  //if (IS_DARWIN)
  //  return;

  // This test is based on SubstituteMulti, but rather than substituting bad
  // chars with the subsitution character, here we omit the bad characters
  // from the conversion.

  bool testPassed;

  std::string_view badStart2UTF8{
      "\xcc\xd0\xc5\xd1\xd2\xd3\xcdThis is \xcc\xd0\xc5\xd1\xd3\xcd a simpleTest\xcc\xd0\xd1\xd2\xd3\xcd"sv};
  std::wstring_view badStart2W{L"\x0D800\x0D801This is a simple\x0D801Test"};
  std::u32string_view badStart2Utf32{U"\x0D800\x0D801This is a simple\x0D801Test"};

  std::string_view utf8ExpectedResult{"This is  a simpleTest"};
  std::wstring_view wExpectedResult{L"This is  a simpleTest"};

  // OSX Omits a second space and final 't'
  std::wstring_view osx_wExpectedResult{L"This is a simpleTes"};

  std::u32string_view u32ExpectedResult{U"This is  a simpleTest"};

  // OSX Omits a second space and final 't'
  std::u32string_view osx_u32ExpectedResult{U"This is a simpleTes"};

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badStart2UTF8, false, false);

  if (IS_DARWIN)
  {
    u32ExpectedResult = osx_u32ExpectedResult;
  }
  testPassed = Equals(u32Result, u32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "FAIL 1 u32Result: " << StringUtils::ToHex(u32Result) << std::endl;
    std::cout << "utf8: " << badStart2UTF8 << " " << StringUtils::ToHex(badStart2UTF8) << std::endl;
  }
  if (!IS_DARWIN || testPassed)
  {
    EXPECT_TRUE(testPassed); // FAILS ON OSX
  }
  if (IS_DARWIN && !testPassed)
  {
    GTEST_SKIP();
  }

  if (IS_DARWIN)
  {
    wExpectedResult = osx_wExpectedResult;
  }
  std::wstring wResult = CUnicodeConverter::Utf8ToW(badStart2UTF8, false, false);
  testPassed = Equals(wResult, wExpectedResult);
  if (!IS_DARWIN || testPassed)
  {
    EXPECT_TRUE(testPassed); // FAILS ON OSX
  }
  utf8ExpectedResult = "This is a simpleTest";
  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(badStart2Utf32, false, false);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!IS_DARWIN || testPassed)
    EXPECT_TRUE(testPassed);

  utf8Result = CUnicodeConverter::WToUtf8(badStart2W, false, false);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!IS_DARWIN || testPassed)
  {
    EXPECT_TRUE(testPassed);
  }
}

TEST(TestCUnicodeConverter, FailOnError)
{
  // Empty string is returned on any error with failOnInvalidChar=true

  // if (IS_DARWIN)
  //  return;

  // This test is based on SubstituteMulti, but rather than substituting bad
  // chars with the subsitution character, here the conversion stops on first
  // bad char, resulting in truncated substitution.

  bool testPassed;

  std::string_view badStart2UTF8{
      "\xcc\xd0\xc5\xd1\xd2\xd3\xcdThis is \xcc\xd0\xc5\xd1\xd3\xcd a simpleTest\xcc\xd0\xd1\xd2\xd3\xcd"sv};
  std::wstring_view badStart2W{L"This is a simple Test\x0D801"};
  std::u32string_view badStart2Utf32{U"This is a simple\x0D801Test"};
  std::string_view utf8ExpectedResult{""};
  std::wstring_view wExpectedResult{L""};
  std::u32string_view u32ExpectedResult{U""};

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badStart2UTF8, true, false);
  testPassed = Equals(u32Result, u32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << " utf8: " << badStart2UTF8 << " "
              << StringUtils::ToHex(badStart2UTF8) << " sub: " << UTF8_SUBSTITUTE_CHARACTER << " "
              << StringUtils::ToHex(UTF8_SUBSTITUTE_CHARACTER) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badStart2UTF8, true, false);
  EXPECT_TRUE(Equals(wResult, wExpectedResult));

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(badStart2Utf32, true, false);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  utf8Result = CUnicodeConverter::WToUtf8(badStart2W, true, false);

  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "input: " << StringUtils::ToHex(badStart2W) << " utf8: " << badStart2UTF8
              << std::endl;
    std::cout << "output: " << StringUtils::ToHex(utf8Result) << " utf8: " << utf8Result
              << std::endl;
    std::cout << " expected: " << StringUtils::ToHex(utf8ExpectedResult)
              << " utf8: " << utf8ExpectedResult << std::endl;
  }
  EXPECT_TRUE(testPassed);
}

void runTest(int iterations,
             std::string_view title,
             std::string_view text,
             int expectedImprovement,
             int& actualImprovement,
             int& aggregateUnicodeConverterTime,
             int& aggregateCharsetConverterTime,
             bool endGroup = false,
             bool printSummary = false);

template<class T>
std::string FormatWithCommas(T value)
{
  std::stringstream ss;
  std::locale myLocale("en_GB.UTF-8");
  ss.imbue(myLocale);
  ss << std::fixed << (value);
  return ss.str();
}

TEST(TestCUnicodeConverter, Performance)
{
  // Confirm that performance no worse than CCharsetConverter
  //
  // This converter has several performance improvements over CCharsetConverter
  // The improvements are most apparent on shorter strings. Most strings are short
  // With optimized code:
  //   short strings are typically 56% faster
  //   medium strings 37% faster
  //   long strings 6% faster

  std::string shortString{"House"};
  std::string shortGermanString{"Haus"};
  std::string shortUkranianString{"Дім"};
  std::string shortJapaneseString{"家"};
  std::string mediumString{"Medium sized string about a sentence long."};
  std::string mediumGermanString{"Mittelgroße Zeichenfolge um einen Satz lang"};
  std::string mediumUkranianString{"Рядок середнього розміру про довге речення"};
  std::string mediumJapaneseString{"文の長さに関する中サイズの文字列"};

  std::string longString{
      "Four score and seven years ago our fathers brought "
      "forth on this continent, a new nation, conceived in Liberty, and dedicated "
      "to the proposition that all men are created equal.\n\n"
      "Now we are engaged in a great civil war, testing whether that nation, or any "
      "nation so conceived and so dedicated, can long endure. We are met on a "
      "great battle-field of that war. We have come to dedicate a portion of that "
      "field, as a final resting place for those who here gave their lives that "
      "that nation might live. It is altogether fitting and proper that we should do this.\n\n"
      "But, in a larger sense, we can not dedicate—we can not consecrate—we "
      "can not hallow—this ground. The brave men, living and dead, who struggled here, "
      "have consecrated it, far above our poor power to add or detract. The world "
      "will little note, nor long remember what we say here, but it can never forget "
      "what they did here. It is for us the living, rather, to be dedicated here to "
      "the unfinished work which they who fought here have thus far so nobly advanced. "
      "It is rather for us to be here dedicated to the great task remaining before us—that "
      "from these honored dead we take increased devotion to that cause for which they gave "
      "the last full measure of devotion—that we here highly resolve that these dead shall"
      "not have died in vain—that this nation, under God, shall have a new birth of "
      "freedom—and that government of the people, by the people, for the people, shall "
      "not perish from the earth.\n\n"
      "  —Abraham Lincoln"};
  std::string longGermanString{
      "Vor vier und sieben Jahren brachten unsere Väter \n"
      "\n„Auf diesem Kontinent eine neue Nation, in Freiheit gezeugt und geweiht "
      "zu der Aussage, dass alle Menschen gleich geschaffen sind.\n\n"
      "\n„Jetzt sind wir in einen großen Bürgerkrieg verwickelt und testen, ob diese Nation oder "
      "irgendeine "
      "Nation, die so konzipiert und so engagiert ist, kann lange bestehen. Wir treffen uns auf "
      "einem "
      "großes Schlachtfeld dieses Krieges. Wir sind gekommen, um einen Teil davon zu widmen "
      "\n„Feld, als letzte Ruhestätte für die, die hier ihr Leben gaben "
      "diese Nation könnte leben. Es ist absolut passend und angemessen, dass wir dies tun "
      "sollten.\n\n"
      "\n„Aber im weiteren Sinne können wir uns nicht weihen – wir können uns nicht weihen "
      "\n„kann diesen Boden nicht heiligen. Die tapferen Männer, lebend und tot, die hier gekämpft "
      "haben. "
      "haben es geweiht, weit über unserer armen Macht hinzuzufügen oder zu schmälern. Die Welt "
      "\n„Werde wenig merken und sich nicht lange daran erinnern, was wir hier sagen, aber er kann "
      "es nie vergessen "
      "\n„was sie hier getan haben. Es ist vielmehr für uns Lebende, sich hier zu widmen. "
      "\n„das unvollendete Werk, das sie, die hier gekämpft haben, bisher so edel vorangebracht "
      "haben. "
      "\n„Es liegt vielmehr an uns, hier der großen Aufgabe gewidmet zu sein, die vor uns liegt – "
      "dass \n„"
      "\n„Von diesen geehrten Toten nehmen wir eine verstärkte Hingabe an die Sache, für die sie "
      "gaben "
      "das letzte volle Maß an Hingabe - dass wir hier hochentschlossen sind, dass diese Toten es "
      "tun werden "
      "nicht umsonst gestorben sind - dass diese Nation unter Gott eine neue Geburt haben wird "
      "\n„Freiheit – und diese Regierung des Volkes durch das Volk für das Volk soll "
      "nicht von der Erde vergehen.\n\n"
      "  -Abraham Lincoln"};
  std::string longUkranianString{
      "Чотири рахунки і сім років тому наші батьки принесли» "
      "на цьому континенті нова нація, зачата в Свободі та присвячена "
      "до положення про те, що всі люди створені рівними.\n\n"
      "«Зараз ми беремо участь у великій громадянській війні, перевіряючи, чи ця нація, чи "
      "будь-яка» "
      "нація, яка так задумана і настільки віддана, може довго витримати. Нас зустріли на "
      "«велике поле битви тієї війни. Ми прийшли, щоб присвятити частину цього» "
      "«поле, як місце останнього спочинку тих, хто тут віддав своє життя, що» "
      "та нація може жити. Цілком доречно і належно, щоб ми це зробили.\n\n "
      "«Але в більш широкому сенсі ми не можемо присвятити—ми не можемо освятити—ми» "
      "«не можу освятити—цю землю. Відважні люди, живі й мертві, які боролися тут,» "
      "«освятили його, набагато вище нашої бідної сили додати або применшити. Світ» "
      "буде мало пам'ятати, ні довго пам'ятати, що ми тут говоримо, але це ніколи не можна забути "
      "що вони зробили тут. Це для нас, живих, скоріше, щоб бути тут присвячені "
      "«незавершена робота, яку ті, хто тут воював, так шляхетно просунули вперед». "
      "«Нам краще бути тут, присвяченим великому завданню, яке стоїть перед нами,— "
      "від цих почесних мертвих ми беремо більшу відданість тій справі, за яку вони віддали "
      "«остання повна міра відданості — що ми тут твердо постановили, що ці мертві будуть» "
      "«не померли даремно — що ця нація, під Богом, матиме нове народження» "
      "свобода - і цей уряд народу, народом, для народу, буде "
      "не згинути із землі.\n\n"
      "  -Абрахам Лінкольн"};
  std::string longJapanseString{
      "4 点と 7 年前に私たちの父親が持ってきた」 "
      "「この大陸に出て、自由の中で生まれ、献身した新しい国」 "
      "「すべての人間は平等に作られているという命題に。\n\n」 "
      "「今、私たちは大規模な内戦に従事しており、その国、または他の国かどうかをテストしています」 "
      "「このように構想され、献身的な国は、長く耐えることができます。私たちは「 "
      "「あの戦争の偉大な戦場。我々はその一部を捧げるために来た」 "
      "「野原はここで命を落とした人々の永眠の地」 "
      "「その国は生きているかもしれません。私たちがこれを行うのは、まったく適切で適切なことです。\n"
      "\n」 "
      "「しかし、より大きな意味で、私たちは献身することはできません—奉献することはできません—私たち"
      "は」 "
      "「神聖化できない――この大地。生死を問わず、ここで奮闘した勇者たち」 "
      "「私たちの貧しい力を加えたり減らしたりする力をはるかに超えて、それを奉献しました。世界」 "
      "「ここで私たちが言ったことをほとんど覚えていないか、長く覚えていませんが、決して忘れることは"
      "できません」 "
      "「彼らがここで何をしたか。むしろ、ここで献身するのは私たちの命です」 "
      "「ここで戦った彼らがこれまで気高く進めてきた未完の仕事。」 "
      "「私たちがここにいるのは、私たちの前に残っている大きな仕事に専念するためです。それは」 "
      "「これらの名誉ある死者から、私たちは彼らが与えた大義へのさらなる献身を取ります」 "
      "「献身の最後の完全な尺度—ここで私たちはこれらの死者がそうするであろうことを強く決意します」 "
      "「無駄に死んだのではなく、この国が神の下で新たに誕生するように」 "
      "「自由、そして人民の、人民による、人民のための政府は」 "
      "「地球から滅びないで。\n\n」 "
      "  -アブラハムリンカーン"};

  // int oneHundredThousand = 100000;
  int twentyThousand = 20000;
  int iterations = twentyThousand;
  int aggregateUnicodeConverterTime = 0;
  int aggregateCharsetConverterTime = 0;
  int expectedImprovement = 0;
  bool printGroupSummary = REPORT_PERFORMANCE_DETAILS;
  bool endGroup = true;
  expectedImprovement = 25; // Actual 54%
  int actualShortStringImprovment;
  runTest(iterations, "Short English  ", shortString, expectedImprovement,
          actualShortStringImprovment, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime);
  runTest(iterations, "Short German   ", shortGermanString, expectedImprovement,
          actualShortStringImprovment, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime);
  runTest(iterations, "Short Ukranian ", shortUkranianString, expectedImprovement,
          actualShortStringImprovment, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime);
  runTest(iterations, "Short Japanese ", shortJapaneseString, expectedImprovement,
          actualShortStringImprovment, aggregateUnicodeConverterTime, aggregateCharsetConverterTime,
          endGroup, printGroupSummary);

  aggregateUnicodeConverterTime = 0;
  aggregateCharsetConverterTime = 0;
  expectedImprovement = 15; // Actual 37%
  int actualMediumStringImprovement;
  runTest(iterations, "Medium English ", mediumString, expectedImprovement,
          actualMediumStringImprovement, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime);
  runTest(iterations, "Medium German  ", mediumGermanString, expectedImprovement,
          actualMediumStringImprovement, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime);
  runTest(iterations, "Medium Ukranian", mediumUkranianString, expectedImprovement,
          actualMediumStringImprovement, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime);
  runTest(iterations, "Medium Japanese", mediumJapaneseString, expectedImprovement,
          actualMediumStringImprovement, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime, endGroup, printGroupSummary);

  aggregateUnicodeConverterTime = 0;
  aggregateCharsetConverterTime = 0;
  expectedImprovement = 0; // Actual 6%
  int actualLongStringImprovement;
  runTest(iterations, "Long English   ", longString, expectedImprovement,
          actualLongStringImprovement, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime);
  runTest(iterations, "Long German    ", longGermanString, expectedImprovement,
          actualLongStringImprovement, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime);
  runTest(iterations, "Long Ukranian  ", longUkranianString, expectedImprovement,
          actualLongStringImprovement, aggregateUnicodeConverterTime,
          aggregateCharsetConverterTime);
  runTest(iterations, "Long Japanese  ", longJapanseString, expectedImprovement,
          actualLongStringImprovement, aggregateUnicodeConverterTime, aggregateCharsetConverterTime,
          endGroup, printGroupSummary);

  std::cout << std::endl;

  int overallImprovment =
      (actualShortStringImprovment + actualMediumStringImprovement + actualLongStringImprovement) /
      3;

  int totalMicros = aggregateUnicodeConverterTime + aggregateCharsetConverterTime;
  std::string totalMillisStr = FormatWithCommas(totalMicros / 1000);
  std::string aggregateUnicodeConverteTimeStr =
      FormatWithCommas(aggregateUnicodeConverterTime / 1000);
  std::string aggregateCharsetConverterConverterTimeStr =
      FormatWithCommas(aggregateCharsetConverterTime / 1000);
  int percentImprovement =
      100 - ((aggregateUnicodeConverterTime * 100) / aggregateCharsetConverterTime);
  std::string pctDiffOverall = FormatWithCommas(percentImprovement);
  std::string pctDiffByGroup = FormatWithCommas(overallImprovment);
  if (REPORT_PERFORMANCE_DETAILS)
  {
    std::cout << "UnicodeConverter total time: " << aggregateUnicodeConverteTimeStr << "ms "
              << " CharsetConverter total time: " << aggregateCharsetConverterConverterTimeStr
              << "ms" << std::endl;
    std::cout << "UnicodeConverter is " << pctDiffOverall
              << "% faster than CharsetConverter overall" << std::endl;
    std::cout << pctDiffByGroup << "% faster by group (above number over-represents long strings)"
              << std::endl;
  }
  // Test passes if UnicodeConverter is faster than CharsetConverter.
  // UnicodeConverter should be about 55% faster than CharSetConveter for short strings
  // Long strings should be about 5% faster

  EXPECT_GE(percentImprovement, 0); // Minimum 0% improvement to pass
}

void runTest(int iterations,
             std::string_view title,
             std::string_view text,
             int expectedImprovement,
             int& actualImprovement,
             int& aggregateUnicodeConverterTime,
             int& aggregateCharsetConverterTime,
             bool endGroup, // = false,
             bool printSummary) //  = false)
{
  int scale = 100000; // / 1000; // To calculate code units / ms
  std::string iterationsStr = FormatWithCommas(iterations);
  std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point test_case_start;

  std::string unicodeConverterTitle("UnicodeConverter ");
  unicodeConverterTitle.append(title);
  std::string charsetConverterTitle("CharsetConverter ");
  charsetConverterTitle.append(title);
  int unicodeConverterTotalTime{0};
  int charsetConverterTotalTime{0};

  // First set of tests is with NEW CUnicodeConverter
  // - utf8->utf32
  // - utf32->utf8
  // - utf8->wstring

  test_case_start = std::chrono::steady_clock::now();
  std::u32string u32Result;
  for (int i = 0; i < iterations; i++)
  {
    u32Result = CUnicodeConverter::Utf8ToUtf32(text);
  }
  std::chrono::time_point<std::chrono::steady_clock> stop_time = std::chrono::steady_clock::now();
  int micros_1 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  unicodeConverterTotalTime += micros_1;

  test_case_start = std::chrono::steady_clock::now();
  std::string utf8Result;
  for (int i = 0; i < iterations; i++)
  {
    utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_2 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  unicodeConverterTotalTime += micros_2;

  test_case_start = std::chrono::steady_clock::now();
  std::wstring wcharResult;
  for (int i = 0; i < iterations; i++)
  {
    wcharResult = CUnicodeConverter::Utf8ToW(text);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_3 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  unicodeConverterTotalTime += micros_3;
  aggregateUnicodeConverterTime += unicodeConverterTotalTime;

  // Using CCharsetConverter

  start_time = std::chrono::steady_clock::now();
  test_case_start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; i++)
  {
    std::string stringConversion{text};
    u32Result = CCharsetConverter::utf8ToUtf32(stringConversion);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_4 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  charsetConverterTotalTime += micros_4;

  test_case_start = std::chrono::steady_clock::now();
  std::u32string_view u32sv(u32Result);
  for (int i = 0; i < iterations; i++)
  {
    std::u32string tmp{u32sv};
    utf8Result = CCharsetConverter::utf32ToUtf8(tmp);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_5 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  charsetConverterTotalTime += micros_5;

  test_case_start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; i++)
  {
    std::string stringConversion{text};
    std::wstring wcharResult;
    CCharsetConverter::utf8ToW(stringConversion, wcharResult, false);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_6 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();

  charsetConverterTotalTime += micros_6;
  aggregateCharsetConverterTime += charsetConverterTotalTime;
  std::string millisStr;

  if (REPORT_PERFORMANCE_DETAILS)
  {
    millisStr = FormatWithCommas(micros_1 / 1000);
    std::cout << unicodeConverterTitle << " Utf8ToUtf32 " << millisStr << "ms "
              << (text.length() * scale) / micros_1 << " code-units/ms for " << iterationsStr
              << " iterations" << std::endl;

    millisStr = FormatWithCommas(micros_2 / 1000);
    std::cout << unicodeConverterTitle << " Utf32ToUtf8 " << millisStr << "ms "
        << (text.length() * scale) / micros_2 << " code-units/ms for " << iterationsStr
        << " iterations" << std::endl;

    millisStr = FormatWithCommas(micros_3 / 1000);
    std::cout << unicodeConverterTitle << " Utf8ToW     " << millisStr << "ms "
        << (text.length() * scale) / micros_3 << " code-units/ms for " << iterationsStr
        << " iterations" << std::endl;

    // millisStr = FormatWithCommas(unicodeConverterTotalTime/1000);
    //std::cout << unicodeConverterTitle << "Total       " << millisStr << "ms for " << iterationsStr
    //             << " iterations" << std::endl;
    std::cout << std::endl;

    millisStr = FormatWithCommas(micros_4/1000);
    std::cout << charsetConverterTitle << " Utf8ToUtf32 " << millisStr << "ms "
        << (text.length() * scale) / micros_4 << " code-units/ms for "
        << iterationsStr << " iterations" << std::endl;

    millisStr = FormatWithCommas(micros_5/1000);
    std::cout << charsetConverterTitle << " Utf32ToUtf8 " << millisStr << "ms "
        << (text.length() * scale) / micros_5 << " code-units/ms for "
        << iterationsStr << " iterations" << std::endl;

    millisStr = FormatWithCommas(micros_6/1000);
    std::cout << charsetConverterTitle << " Utf8ToW     " << millisStr << "ms "
        << (text.length() * scale) / micros_6 << " code-units/ms for "
        << iterationsStr << " iterations" << std::endl;

    std::cout << std::endl;
  }
  if (endGroup)
  {
    std::string unicodeConverterTotalStr = FormatWithCommas(unicodeConverterTotalTime / 1000);
    std::string charsetConverterTotalStr = FormatWithCommas(charsetConverterTotalTime/1000);
    int totalMicros = unicodeConverterTotalTime + charsetConverterTotalTime;
    std::string totalMillisStr = FormatWithCommas(totalMicros/1000);
    int percentImprovement = 100 - ((unicodeConverterTotalTime * 100) / charsetConverterTotalTime);
    actualImprovement = percentImprovement;
    std::string pctDiff = FormatWithCommas(percentImprovement);
    if (printSummary)
    {
      std::cout << unicodeConverterTitle << " total time: " << unicodeConverterTotalStr << "ms " << std::endl;
      std::cout << charsetConverterTitle << " total time: " << charsetConverterTotalStr << "ms" << std::endl;
      std::cout << "UnicodeConverter is " << pctDiff << "% faster than CharsetConverter" << std::endl << std::endl;
    }
    EXPECT_GE(percentImprovement, expectedImprovement);
  }
}
} // namespace
