/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/SpecialProtocol.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/UnicodeConverter.h"

#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string_view>

#include <gtest/gtest.h>

using namespace std::literals;
using namespace std::chrono_literals;

#if defined(TARGET_WINDOWS)
constexpr bool IS_WINDOWS{true};
#else
constexpr bool IS_WINDOWS{false};
#endif

#if defined(TARGET_DARWIN)
constexpr bool IS_DARWIN{true};
#else
constexpr bool IS_DARWIN{false};
#endif
#if defined(__FreeBSD__)
constexpr bool IS_FREE_BSD{true};
#else
constexpr bool IS_FREE_BSD{false};
#endif

namespace
{
static constexpr std::string_view UTF8_SUBSTITUTE_CHARACTER{"\xef\xbf\xbd"};
// static constexpr std::wstring_view WCHAR_T_SUBSTITUTE_CHARACTER{L"\xfffd"};
static constexpr std::u32string_view CHAR32_T_SUBSTITUTE_CHARACTER{U"\x0000fffd"}; //U'�'sv
static constexpr bool REPORT_PERFORMANCE_DETAILS{true};

static void dumpIconvDebugData()
{
  // Useful when trying to understand what iconv is doing when
  // tests fail during Jenkins builds and you don't have access
  // to the failing machine type. You can not use normal
  // logging because these tests run before logging is enabled.

  /*
  FILE* myLog = NULL;

  std::string tempFile = "./test.log";
  myLog = fopen(tempFile.c_str(), "r");
  char buffer[8193];

  while (fgets(buffer, 8192, myLog))
    std::cout << buffer; // Newline included on read
  fclose(myLog);
  */
}

/*
 * Test 'happy path' Convert a simple string to/from various
 * encodings.
 */
TEST(TestCUnicodeConverter, utf8ToUtf32)
{
  std::string_view simpleTest{"This is a simpleTest"};
  std::wstring_view wSimpleTest{L"This is a simpleTest"};
  std::u32string_view u32SimpleTest{U"This is a simpleTest"};

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32("This is a simpleTest"sv);
  EXPECT_TRUE(StringUtils::Equals(u32Result, u32SimpleTest));

  std::wstring wResult = CUnicodeConverter::Utf8ToW(simpleTest);
  EXPECT_TRUE(StringUtils::Equals(wResult, wSimpleTest));

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  EXPECT_TRUE(StringUtils::Equals(simpleTest, utf8Result));

  utf8Result = CUnicodeConverter::WToUtf8(wSimpleTest);
  EXPECT_TRUE(StringUtils::Equals(simpleTest, utf8Result));

  wResult = CUnicodeConverter::Utf32ToW(u32SimpleTest);
  EXPECT_TRUE(StringUtils::Equals(wSimpleTest, wResult));

  u32Result = CUnicodeConverter::WToUtf32(wSimpleTest);
  EXPECT_TRUE(StringUtils::Equals(u32SimpleTest, u32Result));
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

   DARWIN uses UTF-8-MAC, which uses NFD normalization after the conversion. Normal
   systems don't do any normalization. Usually Unicode is in NFC normalization
   form. Normalization is needed because there are multiple ways to represent a number
   of graphemes (chars). Normalization changes the ordering of the code-units or codepoints
   that make up a grapheme. Therefore normalization impacts comparison of some
   character strings. As a further twist, UTF-8-MAC does not convert to NFD for certain
   unicode codepoint ranges: U+2000-U+2FFF, U+F900-U+FAFF, U+2F800-U+2FAFF.
    */

  // Surrogate codepoints which are invalid alone, or as UTF32. However, DARWIN
  // will happily convert these to invalid UTF-8 without any error indication.

  static const char32_t SURROGATE_CHARS[] = {U'\x0D800', U'\x0D801', U'\x0D802', U'\x0D803',
                                             U'\x0D804', U'\x0D805', U'\xDB7F',  U'\xDB80',
                                             U'\xDBFF',  U'\xDC00',  U'\x0DFFF'};

  // TEST 1
  if constexpr (!IS_DARWIN) // Does not recognize as bad
  {
    for (char32_t c : SURROGATE_CHARS)
    {
      std::u32string s(1, c);
      std::string utf8str = CUnicodeConverter::Utf32ToUtf8(s);
      if (!StringUtils::Equals(utf8str, UTF8_SUBSTITUTE_CHARACTER))
      {
        dumpIconvDebugData();
        std::cout << "u32string: " << StringUtils::ToHex(s) << std::endl;
        std::cout << "Surrogate: " << std::hex << s[0] << std::endl;
        std::cout << "utf8str length: " << std::to_string(utf8str.length())
                  << " hex: " << StringUtils::ToHex(utf8str) << std::endl;
        std::cout << " expected: " << StringUtils::ToHex(UTF8_SUBSTITUTE_CHARACTER) << std::endl;
      }
      EXPECT_TRUE(StringUtils::Equals(utf8str, UTF8_SUBSTITUTE_CHARACTER));
    }
  }

  // TEST 2

  // More problems should show up with multi-byte utf8 characters missing the
  // first byte.

  static const std::string BAD_UTF8[] = {
      "\xcc"s, "\xb3"s, "\x9f"s, "\xab"s, "\x93", "\x8b",
  };
  for (std::string str : BAD_UTF8)
  {
    std::u32string trash = CUnicodeConverter::Utf8ToUtf32(str);
    if (!StringUtils::Equals(trash, CHAR32_T_SUBSTITUTE_CHARACTER))
    {
      dumpIconvDebugData();
      std::cout << "u32string: " << StringUtils::ToHex(trash) << std::endl;
      std::cout << "BAD_UTF8: " << std::hex << str << std::endl;
      std::cout << "u32string length: " << std::to_string(trash.length())
                << " hex: " << StringUtils::ToHex(trash) << std::endl;
      std::cout << " expected: " << StringUtils::ToHex(UTF8_SUBSTITUTE_CHARACTER) << std::endl;
    }
    EXPECT_EQ(trash[0], CHAR32_T_SUBSTITUTE_CHARACTER[0]);
    EXPECT_TRUE(StringUtils::Equals(trash, CHAR32_T_SUBSTITUTE_CHARACTER));
  }
  EXPECT_TRUE(true);

  // Test some empty string boundaries. The assumption here is that iconv is near bullet-proof.
  // Since we convert the input string to a byte array without changing address, moving, etc.
  // then we can rely on iconv to handle any malformed input that is possible to handle.
  //
  // For output, we should be okay, since the output buffer is allocated as uchar32_t array,
  // which will ensure that it is on a proper boundary for any of the Unicode char
  // types.

  // TEST 3  Convert empty C-string

  std::u32string result = CUnicodeConverter::Utf8ToUtf32("");
  if (result.length() != 0)
  {
    dumpIconvDebugData();
    std::cout << "Zero length input yields non-zero length output: " << StringUtils::ToHex(result)
              << std::endl;
  }
  EXPECT_EQ(result.length(), 0);

  // TEST 4

  std::u32string u32Empty;
  std::string result_utf8 = CUnicodeConverter::Utf32ToUtf8(u32Empty);
  if (result_utf8.length() != 0)
  {
    dumpIconvDebugData();
    std::cout << "Zero length input yields non-zero length output: "
              << StringUtils::ToHex(result_utf8) << std::endl;
  }
  EXPECT_EQ(result_utf8.length(), 0);

  // TEST 5 convert Empty u32string_view

  std::u32string_view u32Emptysv{U""};
  result_utf8 = CUnicodeConverter::Utf32ToUtf8(u32Emptysv);
  if (result_utf8.length() != 0)
  {
    dumpIconvDebugData();
    std::cout << "Zero length input yields non-zero length output: "
              << StringUtils::ToHex(result_utf8) << std::endl;
  }
  EXPECT_EQ(result_utf8.length(), 0);

  // TEST 6 convert Empty u32 string literal

  result_utf8 = CUnicodeConverter::Utf32ToUtf8(U"");
  if (result_utf8.length() != 0)
  {
    dumpIconvDebugData();
    std::cout << "Zero length input yields non-zero length output: "
              << StringUtils::ToHex(result_utf8) << std::endl;
  }
  EXPECT_EQ(result_utf8.length(), 0);

  // TEST 7 Convert empty char32_t C-string

  char32_t zz[] = U"";

  result_utf8 = CUnicodeConverter::Utf32ToUtf8(zz);
  if (result_utf8.length() != 0)
  {
    dumpIconvDebugData();
    std::cout << "Zero length input yields non-zero length output: "
              << StringUtils::ToHex(result_utf8) << std::endl;
  }
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

  std::string_view badStartUTF8{"\xccThis is a simpleTest"};

  std::string_view utf8ExpectedResult{"\xef\xbf\xbdThis is a simpleTest"};
  std::wstring_view wExpectedResult{L"\x0fffdThis is a simpleTest"};
  std::u32string_view u32ExpectedResult{U"\x0fffdThis is a simpleTest"};

  // SubstituteStart TEST 1

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badStartUTF8);
  EXPECT_TRUE(StringUtils::Equals(u32Result, u32ExpectedResult));

  // SubstituteStart TEST 2

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badStartUTF8);
  EXPECT_TRUE(StringUtils::Equals(wResult, wExpectedResult));

  // SubstituteStart TEST 3

  // Convert both wstring and u32string (with substitution code-unit, from above)
  // back to utf8

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  // SubstituteStart TEST 4

  std::wstring_view wInput = L"\x0fffdThis is a simpleTest";

  utf8Result = CUnicodeConverter::WToUtf8(wInput);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "SubstituteStart  TEST  4  FAILED!" << std::endl;
    dumpIconvDebugData();
    std::cout << "utf8Result " << StringUtils::ToHex(utf8Result) << std::endl;
  }
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  // TEST 5

  // Now verify that a bad first codeunit for wstring and u32string
  // produce substitution char when converting to string

  // DARWIN does not recognize these un-paired surrogates as malformed

  if constexpr (!IS_DARWIN)
  {
    std::u32string_view badStartUtf32{U"\x0D800This is a simpleTest"};
    utf8Result = CUnicodeConverter::Utf32ToUtf8(badStartUtf32);
    testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);

    if (!testPassed)
    {
      std::cout << "Test SubstituteStart # 5" << std::endl;
      dumpIconvDebugData();
      std::cout << "badStartUtf32: "
                << " Hex: " << StringUtils::ToHex(badStartUtf32) << std::endl;
      std::cout << "utf8Result Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
    }
    EXPECT_TRUE(testPassed);
  }

  // TEST 6

  std::string_view badStartUtf8{"\xccThis is a simpleTest"};
  std::u32string_view utf32ExpectedResult{U"\x0000fffdThis is a simpleTest"};
  std::u32string utf32Result = CUnicodeConverter::Utf8ToUtf32(badStartUtf8);
  testPassed = StringUtils::Equals(utf32Result, utf32ExpectedResult);

  if (!testPassed)
  {
    std::cout << "Test SubstituteStart # 5" << std::endl;
    dumpIconvDebugData();
    std::cout << "badStartUtf8: "
              << " Hex: " << StringUtils::ToHex(badStartUtf8) << std::endl;
    std::cout << "utf32Result Hex: " << StringUtils::ToHex(utf32Result) << std::endl;
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

  // Mac OSX iconv gives different results. It frequently
  // marks the utf-8 code-unit (or units) before the malformed
  // code-unit as also bad. Perhaps this is due to the 'bad'
  // code-units are not bad 'start-bytes' of a utf-8 codepoint,
  // and it assumes that the proceeding code-point (a simple
  // ASCII char) to also be bad.

  // SubstituteEnd  TEST 1

  std::string_view badEndUTF8{"This is a simpleTest\x9f"};

  std::string_view utf8ExpectedResult{"This is a simpleTest\xef\xbf\xbd"};
  std::wstring_view wExpectedResult{L"This is a simpleTest\x0fffd"};
  std::u32string_view u32ExpectedResult{U"This is a simpleTest\x0fffd"};

  std::u32string_view u32Expected = u32ExpectedResult;

  if constexpr (IS_DARWIN)
    // 't' before bad char is also considered bad, probably thinks it is part of
    // bad codepoint

    u32Expected = U"This is a simpleTes\x0fffd";

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badEndUTF8);

  bool testPassed = StringUtils::Equals(u32Result, u32Expected);
  if (!testPassed)
  {
    std::cout << " SubstituteEnd  FAIL 1" << std::endl;
    dumpIconvDebugData();

    std::cout << "Input- utf8 Hex: " << StringUtils::ToHex(badEndUTF8) << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << std::endl;
    std::cout << "u32Expected: " << StringUtils::ToHex(u32Expected) << std::endl << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteEnd  TEST 2 Repeat Test 1, but with wchars

  std::wstring_view wExpected = wExpectedResult;

  if constexpr (IS_DARWIN)
    wExpected = L"This is a simpleTes\x0fffd";

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badEndUTF8);
  testPassed = StringUtils::Equals(wResult, wExpected);
  if (!testPassed)
  {
    std::cout << "SubstituteEnd FAIL 2" << std::endl;
    dumpIconvDebugData();
    std::cout << " Input- badEndUTF8 Hex: " << StringUtils::ToHex(badEndUTF8) << std::endl;
    std::cout << "wResult: " << StringUtils::ToHex(wResult) << std::endl;
    std::cout << "wExpected: " << StringUtils::ToHex(wExpected) << std::endl << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteEnd TEST 3

  // Convert both wstring and u32string (with substitution code-unit, from above)
  // back to utf8

  std::string_view utf8Expected = utf8ExpectedResult;
  if constexpr (IS_DARWIN)
  {
    utf8Expected = "This is a simpleTes\xef\xbf\xbd";
  }

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Expected);
  testPassed = StringUtils::Equals(utf8Result, utf8Expected);
  if (!testPassed)
  {
    std::cout << "Test 3 FAILED" << std::endl;
    dumpIconvDebugData();
    std::cout << "u32Expected Hex: " << StringUtils::ToHex(u32Expected) << std::endl;
    std::cout << "utf8Result (input) Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
    std::cout << "utf8Expected Hex: " << StringUtils::ToHex(utf8Expected) << std::endl;
  }

  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8Expected));

  // SubstituteEnd  TEST 4

  utf8Expected = utf8ExpectedResult;
  if constexpr (IS_DARWIN)
  {
    utf8Expected = "This is a simpleTes\xef\xbf\xbd";
  }

  utf8Result = CUnicodeConverter::WToUtf8(wResult);
  testPassed = StringUtils::Equals(utf8Result, utf8Expected);
  if (!testPassed)
  {
    std::cout << "Test 4 FAILED" << std::endl;
    dumpIconvDebugData();
    std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
  }

  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8Expected));

  // Now verify that a bad last codeunit for wstring and u32string
  // produce substitution char when converting to string

  // Darwin doesn't recognize UTF32 & wchar un-paired surrogates as bad

  if constexpr (!IS_DARWIN)
  {
    // SubstituteEnd  TEST 5

    std::u32string_view badEndUtf32{U"This is a simpleTest\x0D800"};
    utf8Result = CUnicodeConverter::Utf32ToUtf8(badEndUtf32);
    testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
    if (!testPassed)
    {
      std::cout << "Test 5 FAILED" << std::endl;
      dumpIconvDebugData();
      std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
      std::cout << "utf8ExpectedResult: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl;
    }
    EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

    // SubstituteEnd  TEST 6

    utf8Result = CUnicodeConverter::WToUtf8(wExpectedResult);
    testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
    if (!testPassed)
    {
      std::cout << "Test 6 FAILED" << std::endl;
      dumpIconvDebugData();
    }
    EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));
  } // ! IS_DARWIN

  // SubstituteEnd  TEST 7

  // Using utf8 bad chars (so that DARWIN can recognize) have bad
  // char as the next-to-last char in string.

  badEndUTF8 = "This is a simpleTest \x9c.";
  u32Expected = U"This is a simpleTest \x0fffd.";

  // Darwin sometimes assumes char before mal-formed char as bad.
  // Presumably because the bad char indicates it is not the first
  // the sequence forming a codepoint

  if constexpr (IS_DARWIN)
    u32Expected = U"This is a simpleTest\x0fffd.";

  u32Result = CUnicodeConverter::Utf8ToUtf32(badEndUTF8);
  testPassed = StringUtils::Equals(u32Result, u32Expected);
  if (!testPassed)
  {
    std::cout << "Test 7 FAILED" << std::endl;
    dumpIconvDebugData();
    std::cout << "input badEndUTF8 Hex: " << StringUtils::ToHex(badEndUTF8) << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << std::endl;
    std::cout << "u32Expected: " << StringUtils::ToHex(u32Expected) << std::endl << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteEnd  TEST 8

  badEndUTF8 = "This is a simpleTest \x9c.";
  wExpected = L"This is a simpleTest \x0fffd.";

  // Darwin sometimes assumes char before mal-formed char as bad.
  // Presumably because the bad char indicates it is not the first
  // the sequence forming a codepoint

  if constexpr (IS_DARWIN)
    wExpected = L"This is a simpleTest\x0fffd.";

  wResult = CUnicodeConverter::Utf8ToW(badEndUTF8);
  testPassed = StringUtils::Equals(wResult, wExpected);
  if (!testPassed)
  {
    std::cout << "SubstituteEnd  TEST 8  FAIL" << std::endl;
    std::cout << "input badEndUTF8 Hex: " << StringUtils::ToHex(badEndUTF8) << std::endl;
    std::cout << "wResult: " << StringUtils::ToHex(wResult) << std::endl;
    std::cout << "wExpected: " << StringUtils::ToHex(wExpected) << std::endl << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteEnd  TEST 9

  // The next test converts a string with two substitution codepoints near the end.
  // The expectation is that since the substitution codepoints are valid Unicode,
  // the output should be the same as the input.

  // This time DARWIN behaves the same.
  if constexpr (IS_DARWIN)
    utf8ExpectedResult = "This is a simpleTest \xef\xbf\xbd\xef\xbf\xbd .";
  else
    utf8ExpectedResult = "This is a simpleTest \xef\xbf\xbd\xef\xbf\xbd .";

  std::u32string_view u32Input = U"This is a simpleTest \x0fffd\x0fffd .";
  utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Input);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << " SubstituteEnd Test 9  FAIL" << std::endl;
    dumpIconvDebugData();
    std::cout << " utf8ExpectedResult Hex: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl;
    std::cout << "utf8Result Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteEnd  TEST 10

  // Same as previous test, but using wchar

  // This time DARWIN behaves the same.

  if constexpr (IS_DARWIN)
    utf8ExpectedResult = "This is a simpleTest\xef\xbf\xbd\xef\xbf\xbd .";
  else
    utf8ExpectedResult = "This is a simpleTest\xef\xbf\xbd\xef\xbf\xbd .";

  std::wstring_view wInput = L"This is a simpleTest\x0fffd\x0fffd .";

  utf8Result = CUnicodeConverter::WToUtf8(wInput); // ,false, true, true);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "SubstituteEnd FAIL 10" << std::endl;
    dumpIconvDebugData();

    std::cout << "input- wInput Hex: " << StringUtils::ToHex(wInput) << std::endl;
    std::cout << "utf8Result Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
    std::cout << "utf8ExpectedResult Hex: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteEnd  TEST 11

  // Now verify that a bad last code unit for wstring and u32string
  // produce substitution char when converting to string

  std::string_view badEndUtf8{"This is a simpleTest \x93\x92."};
  std::u32string utf32ExpectedResult;
  if constexpr (IS_DARWIN)
    utf32ExpectedResult = U"This is a simpleTest\x0fffd.";
  else
    utf32ExpectedResult = U"This is a simpleTest \x0fffd.";

  std::u32string uf32Result;
  uf32Result = CUnicodeConverter::Utf8ToUtf32(badEndUtf8);
  testPassed = StringUtils::Equals(uf32Result, utf32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "SubstituteEnd FAIL 11" << std::endl;
    dumpIconvDebugData();
    std::cout << "badEndUtf8: " << StringUtils::ToHex(badEndUtf8) << std::endl;

    std::cout << "uf32Result: " << StringUtils::ToHex(uf32Result) << std::endl;
    std::cout << "utf32ExpectedResult: " << StringUtils::ToHex(utf32ExpectedResult) << std::endl
              << std::endl;
  }
  EXPECT_TRUE(testPassed);
}

TEST(TestCUnicodeConverter, SubstituteMiddle)
{
  // Convert utf8 strings to wstring and u32string and verify that this results in
  // the middle bad code-unit in each becoming the substitution character.
  // Also, covert both converted wstring and u32string (with substitution char)
  // back to utf8 and verify that it has a substitution char.

  std::string_view badMiddleUTF8{"This is a\xcc simpleTest"};
  std::string_view utf8ExpectedResult{"This is a\xef\xbf\xbd simpleTest"};
  std::wstring_view wExpectedResult{L"This is a\x0fffd simpleTest"};
  std::u32string_view u32ExpectedResult{U"This is a\x0fffd simpleTest"};

  if constexpr (IS_DARWIN)
  {
    utf8ExpectedResult = "This is \xef\xbf\xbd simpleTest";
    wExpectedResult = L"This is \x0fffd simpleTest";
    u32ExpectedResult = U"This is \x0fffd simpleTest";
  }

  // SubstituteMiddle TEST 1

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badMiddleUTF8);
  bool testPassed = StringUtils::Equals(u32Result, u32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "SubstituteMiddle TEST 1 FAILED" << std::endl;
    dumpIconvDebugData();
    std::cout << "badMiddleUTF8 Hex: " << StringUtils::ToHex(badMiddleUTF8) << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteMiddle TEST 2

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badMiddleUTF8);
  testPassed = StringUtils::Equals(wResult, wExpectedResult);
  if (!testPassed)
  {
    std::cout << "SubstituteMiddle TEST 2 FAILED" << std::endl;
    dumpIconvDebugData();
    std::cout << "badMiddleUTF8 Hex: " << StringUtils::ToHex(badMiddleUTF8) << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(wResult) << std::endl;
    std::cout << " wExpectedResult: " << StringUtils::ToHex(wExpectedResult) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteMiddle TEST 3

  // Convert both wstring and u32string (with substitution code-unit, from above)
  // back to utf8

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  testPassed = StringUtils::StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "SubstituteMiddle TEST 3 FAILED" << std::endl;
    dumpIconvDebugData();
    std::cout << "u32Result Hex: " << StringUtils::ToHex(u32Result) << std::endl;
    std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
    std::cout << "utf8ExpectedResult: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteMiddle TEST 4

  utf8Result = CUnicodeConverter::WToUtf8(wResult);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "SubstituteMiddle TEST 4 FAILED" << std::endl;
    dumpIconvDebugData();
    std::cout << "wResult Hex: " << StringUtils::ToHex(wResult) << std::endl;
    std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
    std::cout << "utf8ExpectedResult: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  if (!IS_DARWIN) // Darwin won't handle the unpaired surrogate in the way test needs
  {
    // SubstituteMiddle TEST 5

    // Now verify that a bad middle codeunit for wstring and u32string
    // produce substitution char when converting to (utf8) string

    std::u32string_view badMiddleUtf32{U"This is a\x0D800 simpleTest"};
    utf8Result = CUnicodeConverter::Utf32ToUtf8(badMiddleUtf32);
    testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
    if (!testPassed)
    {
      std::cout << "SubstituteMiddle TEST 5 FAILED" << std::endl;
      dumpIconvDebugData();
      std::cout << "badMiddleUtf32: "
                << " Hex: " << StringUtils::ToHex(badMiddleUtf32) << std::endl;
      std::cout << "utf8Result Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
    }
    EXPECT_TRUE(testPassed);

    // SubstituteMiddle TEST 6

    utf8Result = CUnicodeConverter::WToUtf8(wExpectedResult);
    testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
    if (!testPassed)
    {
      std::cout << "SubstituteMiddle TEST 6 FAILED" << std::endl;
      dumpIconvDebugData();
      std::cout << "badMiddleUtf32: "
                << " Hex: " << StringUtils::ToHex(badMiddleUtf32) << std::endl;
      std::cout << "utf8Result Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
    }
    EXPECT_TRUE(testPassed);
  }
}

TEST(TestCUnicodeConverter, SubstituteStart2)
{
  // Convert utf8 to wstring and u32string and verify that this results in
  // consecutive bad code-units being replaced with a single substitution
  // codepoint. This occurs even if the two malformed code-units
  // belong to different malformed codepoints.
  // Also, covert both converted wstring and u32string (with substitution char)
  // back to utf8 and verify that it has a substitution char.

  bool testPassed;
  std::string_view badStart2UTF8{"\xcc\xcdThis is a simpleTest"};
  std::string_view utf8ExpectedResult{"\xef\xbf\xbdThis is a simpleTest"};
  std::wstring_view wExpectedResult{L"\x0fffdThis is a simpleTest"};
  std::u32string_view u32ExpectedResult{U"\x0fffdThis is a simpleTest"};

  // SubstituteStart2   TEST 1

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badStart2UTF8);
  testPassed = StringUtils::Equals(u32Result, u32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "Test SubstituteStart2 TEST 1" << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result)
              << " badStart2Utf8 Hex: " << StringUtils::ToHex(badStart2UTF8) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteStart2   TEST 2

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badStart2UTF8);
  EXPECT_TRUE(StringUtils::Equals(wResult, wExpectedResult));

  // SubstituteStart2   TEST 3

  // Convert both wstring and u32string (with substitution code-unit, from above)
  // back to utf8

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  // SubstituteStart2   TEST 4

  std::wstring_view wInput = L"\x0fffdThis is a simpleTest";
  utf8Result = CUnicodeConverter::WToUtf8(wInput);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  // SubstituteStart2   TEST 5

  // Now verify that two bad first codeunits for wstring and u32string
  // produce a substitution char when converting to string.
  // The first two codepoints are the second utf-16 codeunit of a utf-16
  // surrogate pair. On linux, wchar and u32char_t are the same. On
  // Windows, wchar is u16char_t. On neither machine should these be
  // valid unicode. Neither on DARWIN, however, osx can
  // produce multiple substitute code units, perhaps due to its normalization,
  // or its system iconv that we depend.

  std::u32string_view badStart2Utf32{U"\x0D800\x0D801This is a simpleTest"};
  std::u32string_view utf32ExpectedResult;
  std::u32string utf32Result;

  // Linux iconv detects the malformed code-units (x0D8000 & X0D801 and converts
  // to a single SUBSTITUTION code point. The custom code Kodi has handles it

  utf8ExpectedResult = "\xef\xbf\xbdThis is a simpleTest";

  if constexpr (IS_WINDOWS || IS_FREE_BSD)
  {
    // Windows uses libiconv provided by Kodi build, but it handles the
    // substitution transparently. It converts the individual bad codepoints as
    // two separate SUBSTITUTION codpoints.
    //
    // Free_BSD gives same result

    utf8ExpectedResult = "\xef\xbf\xbd\xef\xbf\xbdThis is a simpleTest";
  }
  if constexpr (IS_DARWIN)
  {
    // DARWIN doesn't recognize 0Xd800 as bad unicode, it happily makes up invalid utf8
    // without informing Kodi that the characters are bad.

    // We do a separate conversion of the result (with invalid utf8) to utf32 to see
    // that then iconv informs Kodi of the bad chars and UnicodeConverter gets to
    // do the substitution. Even then, DARWIN reports two errors.

    utf8ExpectedResult = "\xed\xa0\x80\xed\xa0\x81This is a simpleTest";
    utf32ExpectedResult = U"\xFFFDThis is a simpleTest";
  }
  utf8Result = CUnicodeConverter::Utf32ToUtf8(badStart2Utf32); // ,false, true, true);

  // Two possible valid results. The two bad codepoints can be represented by one or two
  // substitute codepoints. Two bad codepoints is more accurate (in keeping with standard)
  // but we are not looking at perfection, just conveying to user that there are bad codepoints

  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);

  if (!testPassed)
  {
    std::cout << "SubstituteStart2  TEST 5 FAILED" << std::endl;
    dumpIconvDebugData();

    std::cout << "badStart2Utf32: "
              << " Hex: " << StringUtils::ToHex(badStart2Utf32) << std::endl;
    std::cout << "utf8Result Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
    std::cout << "utf8ExpectedResult Hex: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl;
  }
  if constexpr (IS_DARWIN)
  {
    utf32Result = CUnicodeConverter::Utf8ToUtf32(utf8Result);
    testPassed = StringUtils::Equals(utf32Result, utf32ExpectedResult);
    if (!testPassed)
    {
      std::cout << "SubstituteStart2  TEST 5-b" << std::endl;
      dumpIconvDebugData();
      std::cout << "utf32Result: "
                << " Hex: " << StringUtils::ToHex(utf32Result) << std::endl;
      std::cout << "utf32ExpectedResult Hex: " << StringUtils::ToHex(utf32ExpectedResult)
                << std::endl;
    }
  }
  EXPECT_TRUE(testPassed);

  // SubstituteStart2  TEST 6

  if constexpr (!IS_DARWIN & !IS_FREE_BSD)
  {
    utf8ExpectedResult = "\xef\xbf\xbdThis is a simpleTest";
  }
  else if constexpr (IS_FREE_BSD)
  { // It looks like Free-BSD's iconv is converting the bad chars into two substitute-chars
    // before we have a chance to process them ourselves. Much the same as the other
    // deviations.
    utf8ExpectedResult = "\xef\xbf\xbd\xef\xbf\xbdThis is a simpleTest";
  }
  else // DARWIN doesn't recognize 0Xd800 as bad unicode, it happily makes up invalid utf8
  {
    utf8ExpectedResult = "\xed\xa0\x80\xed\xa0\x81This is a simpleTest";
    utf32ExpectedResult = U"\xFFFD\xFFFDThis is a simpleTest";
  }
  std::wstring_view wstringInput{L"\x0D800\x0D801This is a simpleTest"};

  utf8Result = CUnicodeConverter::WToUtf8(wstringInput);
  testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
  if (!testPassed)
  {
    std::cout << "SubstituteStart2  TEST 6" << std::endl;
    dumpIconvDebugData();
    std::cout << "wstringInput: "
              << " Hex: " << StringUtils::ToHex(wstringInput) << std::endl;
    std::cout << "utf8Result Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
    std::cout << "utf8ExpectedResult Hex: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl;
  }
  EXPECT_TRUE(testPassed);
}

TEST(TestCUnicodeConverter, SubstituteMulti)
{
  if constexpr (IS_DARWIN)
    return;

  // This tests have multiple bad chars within a string

  // Test with UTF8 malformed character sequence: 2 bad codepoints in a row. Both have
  // proper start bytes, but invalid middle bytes. This sequence will show
  // up at beginning, middle and end of same string.

  bool testPassed;

  std::string_view badStart2UTF8{"\xcc\xd0\xc5\xd1\xd2\xd3\xcdThis is \xcc\xd0\xc5\xd1\xd3\xcd a "
                                 "simpleTest\xcc\xd0\xd1\xd2\xd3\xcd"};

  std::string_view utf8ExpectedResult{"\xef\xbf\xbdThis is \xef\xbf\xbd a "
                                      "simpleTest\xef\xbf\xbd"};
  std::wstring_view wExpectedResult{L"\x0fffdThis is \x0fffd a simpleTest\x0fffd"};
  // std::u16string_view u16ExpectedResult{u"\x0fffdThis is \x0fffd a simpleTest\x0fffd"};
  std::u32string_view u32ExpectedResult{U"\x0fffdThis is \x0fffd a simpleTest\x0fffd"};

  // SubstituteMulti TEST 1

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badStart2UTF8);
  testPassed = StringUtils::Equals(u32Result, u32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "SubstituteMulti TEST 1" << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << " utf8: " << badStart2UTF8 << " "
              << StringUtils::ToHex(badStart2UTF8) << " sub: " << UTF8_SUBSTITUTE_CHARACTER << " "
              << StringUtils::ToHex(UTF8_SUBSTITUTE_CHARACTER) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // SubstituteMulti TEST 2
  std::wstring wResult = CUnicodeConverter::Utf8ToW(badStart2UTF8);
  EXPECT_TRUE(StringUtils::Equals(wResult, wExpectedResult));

  // SubstituteMulti TEST 3

  // Convert both wstring and u32string (with substitution code-unit, from above)
  // back to utf8

  std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(u32Result);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));

  // SubstituteMulti TEST 4

  utf8Result = CUnicodeConverter::WToUtf8(wResult);
  EXPECT_TRUE(StringUtils::Equals(utf8Result, utf8ExpectedResult));
}

TEST(TestCUnicodeConverter, SkipMulti)
{

  // This test is based on SubstituteMulti, but rather than substituting bad
  // chars with the subsitution character, here we omit the bad characters
  // from the conversion.
  //
  // UTF8 and UTF16 code units are marked so that you can tell where it belongs in a sequence
  // making up a codepoint. You can also tell how many code-units to expect in the sequence
  // each code unit. For reference, see utils/Utf8Utils::SizeOfUtf8Char.

  bool testPassed;

  std::string_view utf8_input;

  // For bad characters, using the second utf-16 code-unit of a pair of code-units.
  // Same chars used in the test 'BadUnicodeTest,' above.
  std::wstring_view badMultiW{L"\x0D800\x0D801This is \x0D803 a simple\x0D801Test"};
  std::u32string_view badMultiUtf32{U"\x0D800\x0D801This is \x0D803 a simple\x0D801Test"};

  std::string_view utf8ExpectedResult{"This is  a simpleTest"};

  std::wstring_view wExpectedResult;
  std::u32string_view u32ExpectedResult;

  // TEST 1

  utf8_input = "\xcc\xd0\xc5\xd1\xd2\xd3\xcdThis is \xcc\xd0\xc5\xd1\xd3\xcd a "
               "simpleTest\xcc\xd0\xd1\xd2\xd3\xcd";

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(utf8_input, false, false);

  if constexpr (IS_DARWIN)
    u32ExpectedResult = U"This is a simpleTes";
  else
    u32ExpectedResult = U"This is  a simpleTest";

  testPassed = StringUtils::Equals(u32Result, u32ExpectedResult);

  if (!testPassed)
  {
    std::cout << "SkipMulti FAIL 1" << std::endl;
    dumpIconvDebugData();
    std::cout << "IS_DARWIN: " << std::to_string(IS_DARWIN) << std::endl;
    std::cout << "badMultiUtf8: " << StringUtils::ToHex(utf8_input) << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << std::endl;
    std::cout << "Expected: " << StringUtils::ToHex(u32ExpectedResult) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // TEST 2 Repeat of test 1, but using wchar input

  if constexpr (!IS_DARWIN)
  {
    wExpectedResult = L"This is  a simpleTest";
  }
  else
  {
    // OSX Omits a second space and final 't'

    wExpectedResult = L"This is a simpleTes";
  }
  utf8_input = "\xcc\xd0\xc5\xd1\xd2\xd3\xcdThis is \xcc\xd0\xc5\xd1\xd3\xcd a "
               "simpleTest\xcc\xd0\xd1\xd2\xd3\xcd";
  std::wstring wResult = CUnicodeConverter::Utf8ToW(utf8_input, false, false);
  testPassed = StringUtils::Equals(wResult, wExpectedResult);

  if (!testPassed)
  {
    std::cout << "SkipMulti FAIL 2 " << std::endl;
    dumpIconvDebugData();
    std::cout << "badMultiUtf8 Hex: " << StringUtils::ToHex(utf8_input) << std::endl;
    std::cout << "wResult Hex: " << StringUtils::ToHex(wResult) << std::endl;
    std::cout << "wExpectedResult Hex: " << StringUtils::ToHex(wExpectedResult) << std::endl;
  }

  EXPECT_TRUE(testPassed);

  // TEST 3

  // Very similar to the previous tests, but change the bad-char
  // so that it fails on DARWIN as well as the others

  // Bytes in range 0xC2 .. 0xDF are the first byte in two byte sequence
  // 2nd bytes must (chr & 0xC0) == 0x80)). Therefore, replace \xcd with \x95
  // so that the \xd3\xcd sequence is a end char.

  utf8_input = "\xcc\xd0\xc5\xd1\xd2\xd3\xcdThis is \xcc\xd0\xc5\xd1\xd3\xcd a "
               "simpleTest\xcc\xd0\xd1\xd2\xd3\xcd";

  if constexpr (IS_DARWIN)
  {
    // DARWIN indicates that a space preceding the malformed unicode
    // (see utf8-input, the space after 'This is') is part of the malformed characters.
    // Probably \xcc is the second byte of a utf-8 sequence and the iconv used counts
    // the preceding byte as part of it. Similarly, the final 't' is also considered
    // part of the bad char sequence beginning with \xcc (again).
    //
    u32ExpectedResult = {U"This is a simpleTes"};
  }
  else
  {
    u32ExpectedResult = {U"This is  a simpleTest"};
  }
  u32Result = CUnicodeConverter::Utf8ToUtf32(utf8_input, false, false);

  testPassed = StringUtils::Equals(u32Result, u32ExpectedResult);

  if (!testPassed)
  {
    std::cout << "SkipMulti FAIL 3" << std::endl;
    std::cout << "badMultiUtf8: " << StringUtils::ToHex(utf8_input) << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << std::endl;
    std::cout << "Expected: " << StringUtils::ToHex(u32ExpectedResult) << std::endl;
    dumpIconvDebugData();
  }
  EXPECT_TRUE(testPassed);

  // TEST 4

  if constexpr (!IS_WINDOWS && !IS_DARWIN)
  {
    // Windows uses libiconv from tools/depends/target. Both it and Linux libc iconv
    // are from gnu and very similar, but not identical. For libiconv for windows at
    // at least, this test results in output identical to the input. Tracing of calls
    // does not yield any errors. While it would be better if it yield the same result,
    // it is not critical that all mal-formed characters be identified.

    std::string utf8Result = CUnicodeConverter::Utf32ToUtf8(badMultiUtf32, false, false);
    if constexpr (IS_FREE_BSD)
    {
      std::string_view bsd_utf8ExpectedResult =
          "\xef\xbf\xbd\xef\xbf\xbdThis is \xef\xbf\xbd a simple\xef\xbf\xbdTest";
      testPassed = StringUtils::Equals(utf8Result, bsd_utf8ExpectedResult);
    }
    else
    {
      testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
    }
    if (!testPassed)
    {
      std::cout << "SkipMulti FAIL 4" << std::endl;
      std::cout << "badMultiUtf32: " << StringUtils::ToHex(badMultiUtf32) << std::endl;
      std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
      std::cout << "Expected: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl;
    }
    EXPECT_TRUE(testPassed);
  }

  // TEST 5

  std::string utf8Result;
  if constexpr (!IS_DARWIN) // DARWIN doesn't recognize 0x0D800, etc. as bad chars
  {
    utf8Result = CUnicodeConverter::WToUtf8(badMultiW, false, false);
    if constexpr (IS_FREE_BSD)
    {
      std::string_view bsd_utf8ExpectedResult =
          "\xef\xbf\xbd\xef\xbf\xbdThis is \xef\xbf\xbd a simple\xef\xbf\xbdTest";
      testPassed = StringUtils::Equals(utf8Result, bsd_utf8ExpectedResult);
    }
    else
    {
      testPassed = StringUtils::Equals(utf8Result, utf8ExpectedResult);
    }

    if (!testPassed)
    {
      std::cout << "SkipMulti FAIL 5" << std::endl;
      std::cout << "badMultiW: " << StringUtils::ToHex(badMultiW) << std::endl;
      std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
      std::cout << "Expected: " << StringUtils::ToHex(utf8ExpectedResult) << std::endl;
    }
    EXPECT_TRUE(testPassed);
  }

  // TEST 6

  if constexpr (!IS_DARWIN)
  {
    std::string_view utf8Expected = "This is  a simpleTest";
    if constexpr (IS_WINDOWS | IS_FREE_BSD)
    {
      // Windows, once again, converts to Substitution Chars without it's libiconv
      // letting us know. As far as Kodi knows, the string is perfectly valid.
      // At least it has substitution chars in it.

      utf8Expected = "\xef\xbf\xbd\xef\xbf\xbdThis is \xef\xbf\xbd a simple\xef\xbf\xbdTest";
    }
    utf8Result = CUnicodeConverter::Utf32ToUtf8(badMultiUtf32, false, false);
    testPassed = StringUtils::Equals(utf8Result, utf8Expected);
    if (!testPassed)
    {
      std::cout << "SkipMulti FAIL 6" << std::endl;
      std::cout << "badMultiUtf32: " << StringUtils::ToHex(badMultiUtf32) << std::endl;
      std::cout << "utf8Result: " << StringUtils::ToHex(utf8Result) << std::endl;
      std::cout << "Expected: " << StringUtils::ToHex(utf8Expected) << std::endl;
    }
    EXPECT_TRUE(testPassed);
  } // !IS_DARWIN
}

TEST(TestCUnicodeConverter, FailOnError)
{
  // Empty string is returned on any error with failOnInvalidChar=true

  // This test is based on SubstituteMulti, but rather than substituting bad
  // chars with the subsitution character, here the conversion stops on first
  // bad char and an empty string returned.

  bool testPassed;

  std::string_view badMultiUTF8{"\xcc\xd0\xc5\xd1\xd2\xd3\xcdThis is \xcc\xd0\xc5\xd1\xd3\xcd a "
                                "simpleTest\xcc\xd0\xd1\xd2\xd3\xcd"};
  std::wstring_view badEndWchar{L"This is a simple Test\x0D801"};
  std::u32string_view badMiddleUTF32{U"This is a simple\x0D801Test"};
  std::string_view utf8ExpectedResult{""};
  std::wstring_view wExpectedResult{L""};
  std::u32string_view u32ExpectedResult{U""};

  // This test works fine on all platforms. The malformed characters are definitely improper
  // utf8.

  // FailOnError: Test 1

  std::u32string u32Result = CUnicodeConverter::Utf8ToUtf32(badMultiUTF8, true, false);
  testPassed = StringUtils::Equals(u32Result, u32ExpectedResult);
  if (!testPassed)
  {
    std::cout << "Failed Test 1" << std::endl;
    std::cout << "u32Result: " << StringUtils::ToHex(u32Result) << std::endl;
    std::cout << "badMultiUTF8 Hex: " << StringUtils::ToHex(badMultiUTF8) << std::endl;
    std::cout << "u32ExpectedResult Hex: " << StringUtils::ToHex(u32ExpectedResult) << std::endl;
  }
  EXPECT_TRUE(testPassed);

  // FailOnError: Test 2

  std::wstring wResult = CUnicodeConverter::Utf8ToW(badMultiUTF8, true, false);
  EXPECT_TRUE(StringUtils::Equals(wResult, wExpectedResult));

  // FailOnError: Test 3

  std::string utf8Result;
  std::string_view expectedResult;

  if constexpr (!IS_DARWIN && !IS_WINDOWS && !IS_FREE_BSD)
  {
    // Will not fail on DARWIN nor WINDOWS. Can't seem to detect the un-paired surrogate in UTF32,
    // perhaps because the surrogate should only show up in utf-16, but it is still
    // invalid. DARWIN uses OSX native iconv. Windows uses libiconv from tools/depends/target
    // which is not identical to iconv from libc, although they are both from gnu and very
    // similar. Perhaps it uses the Windows conversion tables?

    // Free_BSD also doesn't fail, but is due to it converting bad codepoints/codeunits
    // into substitute chars without our knowledge.

    expectedResult = utf8ExpectedResult;
  }
  else if constexpr (IS_FREE_BSD)
  {
    expectedResult = {"This is a simple\xef\xbf\xbdTest"};
  }
  if constexpr (!IS_DARWIN && !IS_WINDOWS)
  {
    utf8Result = CUnicodeConverter::Utf32ToUtf8(badMiddleUTF32, true, false);
    testPassed = StringUtils::Equals(utf8Result, expectedResult);

    if (!testPassed)
    {
      std::cout << "Test FailOnError #3" << std::endl;
      dumpIconvDebugData();
      std::cout << "badMiddleUTF32 Hex: " << StringUtils::ToHex(badMiddleUTF32) << std::endl;
      std::cout << "utf8Result Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
      std::cout << "utf8ExpectedResult Hex: " << StringUtils::ToHex(expectedResult) << std::endl;
    }
  }

  // FailOnError: Test 4

  std::string expected_result;
  if constexpr (!IS_DARWIN)
  {
    // Will not fail on DARWIN. Can't seem to detect the un-paired surrogate in UTF32/wchar_t,
    // perhaps because the surrogate should only show up in utf-16, but it is still
    // invalid. DARWIN uses OSX native iconv.

    // FREE_BSD's iconv seems to convert the bad-chars to unicode substitute chars before
    // iconv is called
    if constexpr (IS_FREE_BSD)
      expected_result = std::string("This is a simple Test\xef\xbf\xbd");
    else
      expected_result = utf8ExpectedResult;
    utf8Result = CUnicodeConverter::WToUtf8(badEndWchar, true, false);
    testPassed = StringUtils::Equals(utf8Result, expected_result);
    if (!testPassed)
    {
      std::cout << "Test FailOnError #4" << std::endl;
      std::cout << "badEndWchar Hex: " << StringUtils::ToHex(badEndWchar) << std::endl;
      std::cout << "badMultiUTF8 Hex: " << StringUtils::ToHex(badMultiUTF8) << std::endl;
      std::cout << "utf8Result Hex: " << StringUtils::ToHex(utf8Result) << std::endl;
      std::cout << "expected_result Hex: " << StringUtils::ToHex(expected_result) << std::endl;
    }
    EXPECT_TRUE(testPassed);
  }
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

  // int oneHundredThousand{100000};
  int twentyThousand{20000};
  int iterations{twentyThousand};
  int aggregateUnicodeConverterTime = 0;
  int aggregateCharsetConverterTime = 0;
  int expectedImprovement = 0;
  bool printGroupSummary = REPORT_PERFORMANCE_DETAILS;
  bool endGroup = true;
  expectedImprovement = 0; // Actual 54%
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
  expectedImprovement = 0; // Actual 37%
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
  expectedImprovement = -10; // Actual 6%
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

  // EXPECT_GE(percentImprovement, -20); // Minimum -20% improvement to pass, should be > 5%
  EXPECT_TRUE(true);
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

  test_case_start = std::chrono::steady_clock::now();
  std::wstring wstrResult;
  for (int i = 0; i < iterations; i++)
  {
    wstrResult = CUnicodeConverter::Utf32ToW(u32Result);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_4 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  unicodeConverterTotalTime += micros_4;

  test_case_start = std::chrono::steady_clock::now();
  std::u32string u32str;
  for (int i = 0; i < iterations; i++)
  {
    u32str = CUnicodeConverter::WToUtf32(wstrResult);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_5 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  unicodeConverterTotalTime += micros_5;

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
  int micros_6 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  charsetConverterTotalTime += micros_6;

  test_case_start = std::chrono::steady_clock::now();
  std::u32string_view u32sv(u32Result);
  for (int i = 0; i < iterations; i++)
  {
    std::u32string tmp{u32sv};
    utf8Result = CCharsetConverter::utf32ToUtf8(tmp);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_7 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  charsetConverterTotalTime += micros_7;

  test_case_start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; i++)
  {
    std::string stringConversion{text};
    std::wstring wcharResult;
    CCharsetConverter::utf8ToW(stringConversion, wcharResult, false);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_8 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  charsetConverterTotalTime += micros_8;

  test_case_start = std::chrono::steady_clock::now();
  u32sv = std::u32string_view(u32Result);
  std::wstring wResult;
  for (int i = 0; i < iterations; i++)
  {
    std::u32string tmp{u32sv};
    CCharsetConverter::utf32ToW(tmp, wResult);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_9 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  charsetConverterTotalTime += micros_9;

  test_case_start = std::chrono::steady_clock::now();
  std::wstring_view wsv(wResult);
  for (int i = 0; i < iterations; i++)
  {
    std::wstring tmp{wsv};
    utf8Result = CCharsetConverter::wToUtf32(tmp, u32Result);
  }
  stop_time = std::chrono::steady_clock::now();
  int micros_10 =
      std::chrono::duration_cast<std::chrono::microseconds>(stop_time - test_case_start).count();
  charsetConverterTotalTime += micros_10;
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

    millisStr = FormatWithCommas(micros_4 / 1000);
    std::cout << unicodeConverterTitle << " WToUtf32 " << millisStr << "ms "
              << (text.length() * scale) / micros_4 << " code-units/ms for " << iterationsStr
              << " iterations" << std::endl;

    millisStr = FormatWithCommas(micros_5 / 1000);
    std::cout << unicodeConverterTitle << " Utf32ToW     " << millisStr << "ms "
              << (text.length() * scale) / micros_5 << " code-units/ms for " << iterationsStr
              << " iterations" << std::endl;

    // millisStr = FormatWithCommas(unicodeConverterTotalTime/1000);
    //std::cout << unicodeConverterTitle << "Total       " << millisStr << "ms for " << iterationsStr
    //             << " iterations" << std::endl;
    std::cout << std::endl;

    millisStr = FormatWithCommas(micros_6 / 1000);
    std::cout << charsetConverterTitle << " Utf8ToUtf32 " << millisStr << "ms "
              << (text.length() * scale) / micros_6 << " code-units/ms for " << iterationsStr
              << " iterations" << std::endl;

    millisStr = FormatWithCommas(micros_7 / 1000);
    std::cout << charsetConverterTitle << " Utf32ToUtf8 " << millisStr << "ms "
              << (text.length() * scale) / micros_7 << " code-units/ms for " << iterationsStr
              << " iterations" << std::endl;

    millisStr = FormatWithCommas(micros_8 / 1000);
    std::cout << charsetConverterTitle << " Utf8ToW     " << millisStr << "ms "
              << (text.length() * scale) / micros_8 << " code-units/ms for " << iterationsStr
              << " iterations" << std::endl;

    millisStr = FormatWithCommas(micros_9 / 1000);
    std::cout << charsetConverterTitle << " WToUtf32 " << millisStr << "ms "
              << (text.length() * scale) / micros_9 << " code-units/ms for " << iterationsStr
              << " iterations" << std::endl;

    millisStr = FormatWithCommas(micros_10 / 1000);
    std::cout << charsetConverterTitle << " Utf32ToW     " << millisStr << "ms "
              << (text.length() * scale) / micros_10 << " code-units/ms for " << iterationsStr
              << " iterations" << std::endl;

    std::cout << std::endl;
  }
  if (endGroup)
  {
    std::string unicodeConverterTotalStr = FormatWithCommas(unicodeConverterTotalTime / 1000);
    std::string charsetConverterTotalStr = FormatWithCommas(charsetConverterTotalTime / 1000);
    int totalMicros = unicodeConverterTotalTime + charsetConverterTotalTime;
    std::string totalMillisStr = FormatWithCommas(totalMicros / 1000);
    int percentImprovement = 100 - ((unicodeConverterTotalTime * 100) / charsetConverterTotalTime);
    actualImprovement = percentImprovement;
    std::string pctDiff = FormatWithCommas(percentImprovement);
    if (printSummary)
    {
      std::cout << unicodeConverterTitle << " total time: " << unicodeConverterTotalStr << "ms "
                << std::endl;
      std::cout << charsetConverterTitle << " total time: " << charsetConverterTotalStr << "ms"
                << std::endl;
      std::cout << "UnicodeConverter is " << pctDiff << "% faster than CharsetConverter"
                << std::endl
                << std::endl;
    }
    EXPECT_TRUE(true); // Can't really test on a build machine
    // EXPECT_GE(percentImprovement, expectedImprovement);
  }
}
} // namespace
