/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
//-----------------------------------------------------------------------
//
//  File:      StringUtils.cpp
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to use J O'Leary's std::string class by kraqh3d
//
//------------------------------------------------------------------------

#ifdef HAVE_NEW_CROSSGUID
#include <crossguid/guid.hpp>
#else
#include <guid.h>
#endif

#if defined(TARGET_ANDROID)
#include <androidjni/JNIThreading.h>
#endif

#include "LangInfo.h"
#include "StringUtils.h"
#include "UnicodeConverter.h"
#include "XBDateTime.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <assert.h>
#include <codecvt>
#include <functional>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fstrcmp.h>
#include <memory.h>

// don't move or std functions end up in PCRE namespace
// clang-format off
#include "utils/RegExp.h"
// clang-format on

#define FORMAT_BLOCK_SIZE 512 // # of bytes for initial allocation for printf

using namespace std::literals;

static constexpr const char* ADDON_GUID_RE =
    "^(\\{){0,1}[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{12}"
    "(\\}){0,1}$";

/* empty string for use in returns by ref */
const std::string StringUtils::Empty{""};

namespace
{
// Configures the behavior of iconv.
// When true, FAIL_ON_ERROR causes conversion to return an empty string on any error.
// Otherwise, the behavior depends upon SUBSTITUTE_BAD_CHARS
static constexpr bool FAIL_ON_ERROR{false};

// When FAIL_ON_ERROR is false:
//   then SUBSTITUTE_BAD_CHARS, when true, causes any invalid codepoints to be replaced
//   with a substitution codepoint (U"x0fffd" which is displayed as: '�').
//   When false then any bad codepoints are omitted from the returned string.
static constexpr bool SUBSTITUTE_BAD_CHARS{true};

// MacOS does not like C.UTF-8

#if defined(TARGET_DARWIN)
static constexpr std::string_view C_LOCALE_NAME{"C"};
#else
static constexpr std::string_view C_LOCALE_NAME{"C.UTF-8"};
#endif

static const std::locale C_LOCALE = []() {
  std::locale cLocale;
  try
  {
    cLocale = std::locale(C_LOCALE_NAME.data()); // MacOS does not like C.UTF-8
  }
  catch (std::runtime_error& e)
  {
    // Have to be careful about logging in StringUtils. Can get recursive during
    // formatting of message.

    std::cout << "Locale C.UTF-8 not supported trying 'C'" << std::endl;
    try
    {
      cLocale = std::locale("C"); // This shouldn't fail
    }
    catch (std::runtime_error& e2)
    {
      // Have to be careful about logging in StringUtils. Can get recursive

      std::cout << "Locale C not supported" << std::endl;
    }
  }
  return cLocale;
}();

static const std::ctype<char>& C_CTYPE_CHAR = std::use_facet<std::ctype<char>>(C_LOCALE);
static const std::ctype<wchar_t>& C_CTYPE_WCHAR = std::use_facet<std::ctype<wchar_t>>(C_LOCALE);

// Unicode Replacement Character, commonly used to substitute for bad
// or malformed unicode characters.
static const std::u32string REPLACMENT_CHARACTER{U"\x00FFFD"};

/*!
 * \brief Converts a string to a number of a specified type, by using istringstream.
 * \param str The string to convert
 * \param fallback [OPT] The number to return when the conversion fails
 * \return The converted number, otherwise fallback if conversion fails
 */
template<typename T>
T NumberFromSS(std::string_view str, T fallback) noexcept
{
  std::istringstream iss{str.data()};
  T result{fallback};
  iss >> result;
  return result;
}

/*!
 * \brief Folds the case of the given Unicode (32-bit) character
 *
 * Performs "simple" case folding using data from Unicode Inc.'s ICUC4.
 * Read the description preceding the tables (FOLDCASE_0000) or
 * from the API documentation for FoldCase.
 *
 * \param c character to fold case
 * \return case folded version of c
 */
static char32_t FoldCaseChar(const char32_t c);
static char32_t ToUpperUnicode(const char32_t c);
static char32_t ToLowerUnicode(const char32_t c);

} // namespace

std::string StringUtils::FormatV(const char* fmt, va_list args)
{
  if (!fmt || !fmt[0])
    return "";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;

  while (true)
  {
    char* cstr = reinterpret_cast<char*>(malloc(sizeof(char) * size));
    if (!cstr)
      return "";

    va_copy(argCopy, args);
    int nActual = vsnprintf(cstr, size, fmt, argCopy);
    va_end(argCopy);

    if (nActual > -1 && nActual < size) // We got a valid result
    {
      std::string str(cstr, nActual);
      free(cstr);
      return str;
    }
    free(cstr);
#ifndef TARGET_WINDOWS
    if (nActual > -1) // Exactly what we will need (glibc 2.1)
      size = nActual + 1;
    else // Let's try to double the size (glibc 2.0)
      size *= 2;
#else // TARGET_WINDOWS
    va_copy(argCopy, args);
    size = _vscprintf(fmt, argCopy);
    va_end(argCopy);
    if (size < 0)
      return "";
    else
      size++; // increment for null-termination
#endif // TARGET_WINDOWS
  }

  return ""; // unreachable
}

std::wstring StringUtils::FormatV(const wchar_t* fmt, va_list args)
{
  if (!fmt || !fmt[0])
    return L"";

  int size = FORMAT_BLOCK_SIZE;
  va_list argCopy;

  while (true)
  {
    wchar_t* cstr = reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t) * size));
    if (!cstr)
      return L"";

    va_copy(argCopy, args);
    int nActual = vswprintf(cstr, size, fmt, argCopy);
    va_end(argCopy);

    if (nActual > -1 && nActual < size) // We got a valid result
    {
      std::wstring str(cstr, nActual);
      free(cstr);
      return str;
    }
    free(cstr);

#ifndef TARGET_WINDOWS
    if (nActual > -1) // Exactly what we will need (glibc 2.1)
      size = nActual + 1;
    else // Let's try to double the size (glibc 2.0)
      size *= 2;
#else // TARGET_WINDOWS
    va_copy(argCopy, args);
    size = _vscwprintf(fmt, argCopy);
    va_end(argCopy);
    if (size < 0)
      return L"";
    else
      size++; // increment for null-termination
#endif // TARGET_WINDOWS
  }

  return L"";
}

//
// --------------  Unicode encoding converters --------------
//
// Using iconv for conversions:
// - C++ does not convert between wstring and u32string
// - iconv behavior better understood
// - iconv can handle DARWIN UTF-8-MAC encoding

std::string StringUtils::ToUtf8(std::u32string_view str)
{
  std::string utf8Str = CUnicodeConverter::Utf32ToUtf8(str, FAIL_ON_ERROR, SUBSTITUTE_BAD_CHARS);
  return utf8Str;
}

std::string StringUtils::ToUtf8(std::wstring_view str)
{
  std::string utf8Str = CUnicodeConverter::WToUtf8(str, FAIL_ON_ERROR, SUBSTITUTE_BAD_CHARS);
  return utf8Str;
}

std::u32string StringUtils::ToUtf32(std::string_view str)
{
  std::u32string utf32Str =
      CUnicodeConverter::Utf8ToUtf32(str, FAIL_ON_ERROR, SUBSTITUTE_BAD_CHARS);
  return utf32Str;
}

std::u32string StringUtils::ToUtf32(std::wstring_view str)
{
  std::u32string utf32Str;
  utf32Str = CUnicodeConverter::WToUtf32(str, FAIL_ON_ERROR, SUBSTITUTE_BAD_CHARS);
  return utf32Str;
}

std::wstring StringUtils::ToWstring(std::string_view str)
{
  std::wstring wStr = CUnicodeConverter::Utf8ToW(str, FAIL_ON_ERROR, SUBSTITUTE_BAD_CHARS);
  return wStr;
}

std::wstring StringUtils::ToWstring(std::u32string_view str)
{
  std::wstring result;
  result = CUnicodeConverter::Utf32ToW(str, FAIL_ON_ERROR, SUBSTITUTE_BAD_CHARS);
  return result;
}

namespace
{
static int FoldAndCompare(std::u32string_view str1, std::u32string_view str2)
{
  // FoldAndEquals is faster for equality checks.

  size_t length = std::min(str1.length(), str2.length());
  for (size_t i = 0; i < length; i++)
  {
    char32_t fold_c1 = FoldCaseChar(str1[i]);
    char32_t fold_c2 = FoldCaseChar(str2[i]);

    if (fold_c1 != fold_c2)
    {
      if (fold_c1 < fold_c2)
        return -1;
      else
        return 1;
    }
  }
  // Break a tie using length

  if (str1.length() < str2.length())
    return -1;
  if (str2.length() < str1.length())
    return 1;

  return 0;
}

std::wstring ToUpperC(std::wstring_view str)
{
  std::wstring result;
  result.reserve(str.size());

  std::transform(str.cbegin(), str.cend(), std::back_inserter(result),
                 [](wchar_t c) { return C_CTYPE_WCHAR.toupper(c); });

  return result;
}

std::string ToUpperC(std::string_view str)
{
  std::string result;
  result.reserve(str.size());

  std::transform(str.cbegin(), str.cend(), std::back_inserter(result),
                 [](unsigned char c) { return C_CTYPE_CHAR.toupper(c); });

  return result;
}

std::wstring ToLowerC(std::wstring_view str)
{
  std::wstring result;
  result.reserve(str.size());

  std::transform(str.cbegin(), str.cend(), std::back_inserter(result),
                 [](wchar_t c) { return C_CTYPE_WCHAR.tolower(c); });

  return result;
}

std::string ToLowerC(std::string_view str)
{
  std::string result;
  result.reserve(str.size());

  std::transform(str.cbegin(), str.cend(), std::back_inserter(result),
                 [](unsigned char c) { return C_CTYPE_CHAR.tolower(c); });

  return result;
}
} // namespace

std::locale StringUtils::GetCLocale()
{
  return C_LOCALE;
}

std::u32string StringUtils::ToUpper(std::u32string_view str,
                                    const std::locale locale /* = GetSystemLocale() */)
{
  std::u32string result;
  result.reserve(str.size());

  std::transform(str.cbegin(), str.cend(), std::back_inserter(result),
                 [](char32_t c) { return ToUpperUnicode(c); });

  return result;
}

std::string StringUtils::ToUpper(std::string_view str,
                                 const std::locale locale /* = GetSystemLocale()) */)
{
  if (str.length() == 0)
    return std::string();

  // If Locale is "C" or "C.UTF-8", then only ASCII characters are upper-cased
  // All others are left as is. No point in changing to larger code-units.

  if (locale == C_LOCALE)
    return ToUpperC(str);

  std::u32string str32 = ToUtf32(str);
  std::u32string u32Result = ToUpper(str32, locale);
  std::string result = ToUtf8(u32Result);
  return result;
}

std::wstring StringUtils::ToUpper(std::wstring_view str,
                                  const std::locale locale /* = GetSystemLocale()) */)
{
  if (str.length() == 0)
    return std::wstring();

  if (locale == C_LOCALE)
    return ToUpperC(str);

  std::u32string str32 = ToUtf32(str);
  std::u32string u32Result = ToUpper(str32, locale);
  std::wstring result = ToWstring(u32Result);
  return result;
}

std::u32string StringUtils::ToLower(std::u32string_view str,
                                    const std::locale locale /* = GetSystemLocale() */)
{
  std::u32string result;
  if (str.length() == 0)
    return result;

  result.reserve(str.size());
  std::transform(str.cbegin(), str.cend(), std::back_inserter(result),
                 [](char32_t c) { return ToLowerUnicode(c); });

  return result;
}

std::string StringUtils::ToLower(std::string_view str,
                                 const std::locale locale /* = GetSystemLocale() */)
{
  if (str.length() == 0)
    return std::string();

  if (locale == C_LOCALE)
    return ToLowerC(str);

  std::u32string str32 = ToUtf32(str);
  std::u32string u32Result = ToLower(str32, locale);
  std::string result = ToUtf8(u32Result);
  return result;
}

std::wstring StringUtils::ToLower(std::wstring_view str,
                                  const std::locale locale /* = GetSystemLocale() */)
{
  if (str.length() == 0)
    return std::wstring();

  if (locale == C_LOCALE)
    return ToLowerC(str);

  std::u32string str32 = ToUtf32(str);
  std::u32string u32Result = ToLower(str32, locale);
  std::wstring result = ToWstring(u32Result);

  return result;
}

std::u32string StringUtils::FoldCase(std::u32string_view str)
{
  // Common code to do actual case folding
  //
  // In the multi-lingual world, FoldCase is used instead of ToLower to 'normalize'
  // unique-ids, such as property ids, map keys, etc. In the good-'ol days of ASCII
  // ToLower was fine, but when you have ToLower changing behavior depending upon
  // the current locale, you have a disaster. For the curious, look up "Turkish-I"
  // problem: http://www.i18nguy.com/unicode/turkish-i18n.html.
  //
  // FoldCase is designed to transform strings so that unimportant differences
  // (letter case, some accents, etc.) are neutralized by monocasing, etc..
  // Full case folding goes further and converts strings into a canonical
  // form (ex: German sharfes-S (aka Eszett, 'ß') is converted to "ss").
  //
  // The FoldCase here does not support full case folding, but as long as key ids
  // are not too exotic, then this FoldCase should be fine. (A library,
  // such as ICUC4 is required for more advanced Folding).
  //
  // Even though Kodi appears to have fairly well behaved unique-ids, it is
  // very easy to create bad ones and they can be hard to detect.
  //  - The "Turkish-I" problem caught everyone by surprise. No one knew
  //    that in a few Turkic locales, ToLower("I") produces "ı" (lower case
  //    dotless I) and ToUpper("i") produces "İ" (upper case dotted I). The
  //    problem prevented Kodi from starting at all.
  //  - As I write this, there are at least five instances where a translated
  //    value for a device name ("keyboard," etc.) is used as the unique-id.
  //    This was only found by detecting non-ASCII ids AND setting locale to
  //    Russian. The problem might be benign, but a lot of testing or
  //    research is required to be sure.
  //  - Python addons face the same problem. This code does NOT address
  //    that issue.
  //
  // The FoldCase here is based on character data from ICU4C. (The library
  // and data are constantly changing. The current version is from 2021.)
  // The data is in UTF32 format. Since C++ string functions, such as
  // tolower(char) only examines one byte at a time (for UTF8),
  // it is unable to properly process multi-byte characters. Similarly
  // for UTF16, multi-UTF16 code-unit characters are not properly
  // processed (fortunately there are far fewer characters longer than
  // 16-bits than for 8-bit ones). For this reason, both versions of
  // FoldCase (UTF8 and wstring) converts its argument to utf32 to
  // perform the fold and then back to the original UTF8, wstring,
  // etc. after the fold.
  //
  if (str.length() == 0)
    return std::u32string(str);

  // This FoldCase doesn't change string length; more sophisticated libs,
  // such as ICU can.

  std::u32string result;
  result.reserve(str.size());

  std::transform(str.cbegin(), str.cend(), std::back_inserter(result),
                 [](char32_t c) { return FoldCaseChar(c); });

  return result;
}

std::wstring StringUtils::FoldCase(std::wstring_view str)
{
  if (str.empty())
    return std::wstring(str);

  std::u32string utf32Str;
  utf32Str = StringUtils::ToUtf32(str);
  std::u32string foldedStr = StringUtils::FoldCase(utf32Str);
  std::wstring result = StringUtils::ToWstring(foldedStr);

  return result;
}

std::string StringUtils::FoldCase(std::string_view str)
{
  std::u32string utf32Str = StringUtils::ToUtf32(str);
  std::u32string foldedStr = StringUtils::FoldCase(utf32Str);
  std::string result = StringUtils::ToUtf8(foldedStr);
  return result;
}

bool StringUtils::FoldAndCompareStart(std::u32string_view str1, std::u32string_view str2)
{
  if (str1.length() < str2.length())
    return false;

  for (size_t i = 0; i < str2.length(); i++)
  {
    char32_t fold_c1 = FoldCaseChar(str1[i]);
    char32_t fold_c2 = FoldCaseChar(str2[i]);
    if (fold_c1 != fold_c2)
      return false;
  }
  return true;
}

bool StringUtils::FoldAndCompareEnd(std::u32string_view str1, std::u32string_view str2)
{
  if (str1.length() < str2.length())
    return false;

  if (str2.empty())
    return true;

  size_t str1_delta = str1.length() - str2.length();
  size_t i = str2.length();
  while (i > 0)
  {
    i--;
    char32_t fold_c1 = FoldCaseChar(str1[i + str1_delta]);
    char32_t fold_c2 = FoldCaseChar(str2[i]);
    if (fold_c1 != fold_c2)
      return false;
  }
  return true;
}

bool StringUtils::FoldAndEquals(std::u32string_view str1, std::u32string_view str2)
{
  // A bit faster than FoldAndCompare because it can immediately return if lengths
  // not the same. Simple FoldCase does not change codepoint length.
  //
  // Embedded NULLS can occur. Does NOT consider NULLS string terminators (doing so would
  // require string scan for NULLs to determine length, etc.).

  if (str1.length() != str2.length())
    return false;

  for (size_t i = 0; i < str1.length(); i++)
  {
    char32_t fold_c1 = FoldCaseChar(str1[i]);
    char32_t fold_c2 = FoldCaseChar(str2[i]);
    if (fold_c1 != fold_c2)
      return false;
  }
  return true;
}

void StringUtils::ToCapitalize(std::string& str)
{
  std::wstring wstr = StringUtils::ToWstring(str);
  ToCapitalize(wstr);
  std::string tstr = StringUtils::ToUtf8(wstr);
  str.swap(tstr);
}

void StringUtils::ToCapitalize(std::wstring& str)
{
  const std::locale& loc = g_langInfo.GetSystemLocale();
  bool isFirstLetter = true;
  for (std::wstring::iterator it = str.begin(); it < str.end(); ++it)
  {
    // capitalize after spaces and punctuation characters (except apostrophes)
    if (std::isspace(*it, loc) || (std::ispunct(*it, loc) && *it != '\''))
      isFirstLetter = true;
    else if (isFirstLetter)
    {
      *it = std::toupper(*it, loc);
      isFirstLetter = false;
    }
  }
}

bool StringUtils::Equals(std::string_view str1, std::string_view str2)
{
  // The Simple Unicode support that is supplied here allows a quick
  // binary comparison of the two strings without conversion.

  return str1 == str2; // Binary compare not lexicographic
}

bool StringUtils::Equals(std::wstring_view str1, std::wstring_view str2)
{
  // The Simple Unicode support that is supplied here allows a quick
  // binary comparison of the two strings without conversion.

  return str1 == str2; // Binary compare not lexicographic
}

bool StringUtils::Equals(std::u32string_view str1, std::u32string_view str2)
{
  // The Simple Unicode support that is supplied here allows a quick
  // binary comparison of the two strings without conversion.

  return str1 == str2; // Binary compare not lexicographic
}

bool StringUtils::EqualsNoCase(std::string_view str1, std::string_view str2)
{
  // FoldCase both strings and then compare the string. Slower than a byte at a time,
  // but more accurate for multi-byte characters. Does not impact char32_t length,
  // like some Unicode libs do, such as ICUC4. UTF-8 length CAN change during folding.
  //
  // Using Utf32 (Unicode code points) is most accurate, so case fold and
  // compare in that form.

  std::u32string utf32Str1 = StringUtils::ToUtf32(str1);
  std::u32string utf32Str2 = StringUtils::ToUtf32(str2);

  return StringUtils::FoldAndEquals(utf32Str1, utf32Str2);
}

int StringUtils::CompareNoCase(std::string_view str1,
                               std::string_view str2,
                               const size_t n /* = 0 */)
{
  // n is the maximum number of Unicode codepoints (for practical purposes
  // equivalent to characters).
  //
  // Much better to avoid using n by using "StartsWith or EndsWith, etc.
  //
  // Using Utf32 (Unicode code points) is most accurate, so case fold and
  // compare in that form.
  //

  // Convert to codepoints

  std::u32string utf32Str1 = StringUtils::ToUtf32(str1);
  std::u32string utf32Str2 = StringUtils::ToUtf32(str2);

  if (n > 0 && n < utf32Str1.length())
    utf32Str1 = utf32Str1.substr(0, n);

  if (n > 0 && n < utf32Str2.length())
    utf32Str2 = utf32Str2.substr(0, n);

  int result = FoldAndCompare(utf32Str1, utf32Str2);
  return result;
}

int StringUtils::CompareNoCase(std::wstring_view str1,
                               std::wstring_view str2,
                               const size_t n /* = 0 */)
{
  // n is the maximum number of Unicode codepoints (for practical purposes
  // equivalent to characters).
  //
  // Much better to avoid using n by using "StartsWith or EndsWith, etc.
  //
  // Using Utf32 (Unicode code points) is most accurate, so case fold and
  // compare in that form.
  //

  // Convert to codepoints

  std::u32string utf32Str1 = StringUtils::ToUtf32(str1);
  std::u32string utf32Str2 = StringUtils::ToUtf32(str2);

  if (n > 0 && n < utf32Str1.length())
    utf32Str1 = utf32Str1.substr(0, n);

  if (n > 0 && n < utf32Str2.length())
    utf32Str2 = utf32Str2.substr(0, n);

  int result = FoldAndCompare(utf32Str1, utf32Str2);
  return result;
}

std::string StringUtils::Left(const std::string& str, size_t count)
{
  count = std::max((size_t)0, std::min(count, str.size()));
  return str.substr(0, count);
}

std::string StringUtils::Mid(const std::string& str,
                             size_t first,
                             size_t count /* = string::npos */)
{
  if (first + count > str.size())
    count = str.size() - first;

  if (first > str.size())
    return std::string();

  assert(first + count <= str.size());

  return str.substr(first, count);
}

std::string StringUtils::Right(const std::string& str, size_t count)
{
  count = std::max((size_t)0, std::min(count, str.size()));
  return str.substr(str.size() - count);
}

std::string& StringUtils::Trim(std::string& str)
{
  TrimLeft(str);
  return TrimRight(str);
}

std::string& StringUtils::Trim(std::string& str, const char* const chars)
{
  TrimLeft(str, chars);
  return TrimRight(str, chars);
}

namespace
{
// hack to check only first byte of UTF-8 character
// without this hack "TrimX" functions failed on Win32 and OS X with UTF-8 strings
static int isspace_c(char c)
{
  return (c & 0x80) == 0 && ::isspace(c);
}
} // namespace

std::string& StringUtils::TrimLeft(std::string& str)
{
  str.erase(str.begin(),
            std::find_if(str.begin(), str.end(), [](char s) { return isspace_c(s) == 0; }));
  return str;
}

std::string& StringUtils::TrimLeft(std::string& str, const char* const chars)
{
  size_t nidx = str.find_first_not_of(chars);
  str.erase(0, nidx);
  return str;
}

std::string& StringUtils::TrimRight(std::string& str)
{
  str.erase(std::find_if(str.rbegin(), str.rend(), [](char s) { return isspace_c(s) == 0; }).base(),
            str.end());
  return str;
}

std::string& StringUtils::TrimRight(std::string& str, const char* const chars)
{
  size_t nidx = str.find_last_not_of(chars);
  str.erase(str.npos == nidx ? 0 : ++nidx);
  return str;
}

int StringUtils::ReturnDigits(const std::string& str)
{
  std::stringstream ss;
  for (const auto& character : str)
  {
    if (isdigit(character))
      ss << character;
  }
  return atoi(ss.str().c_str());
}

std::string& StringUtils::RemoveDuplicatedSpacesAndTabs(std::string& str)
{
  std::string::iterator it = str.begin();
  bool onSpace = false;
  while (it != str.end())
  {
    if (*it == '\t')
      *it = ' ';

    if (*it == ' ')
    {
      if (onSpace)
      {
        it = str.erase(it);
        continue;
      }
      else
        onSpace = true;
    }
    else
      onSpace = false;

    ++it;
  }
  return str;
}

int StringUtils::Replace(std::string& str, char oldChar, char newChar)
{
  int replacedChars = 0;
  for (std::string::iterator it = str.begin(); it != str.end(); ++it)
  {
    if (*it == oldChar)
    {
      *it = newChar;
      replacedChars++;
    }
  }

  return replacedChars;
}

int StringUtils::Replace(std::string& str, const std::string& oldStr, const std::string& newStr)
{
  if (oldStr.empty())
    return 0;

  int replacedChars = 0;
  size_t index = 0;

  while (index < str.size() && (index = str.find(oldStr, index)) != std::string::npos)
  {
    str.replace(index, oldStr.size(), newStr);
    index += newStr.size();
    replacedChars++;
  }

  return replacedChars;
}

int StringUtils::Replace(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr)
{
  if (oldStr.empty())
    return 0;

  int replacedChars = 0;
  size_t index = 0;

  while (index < str.size() && (index = str.find(oldStr, index)) != std::string::npos)
  {
    str.replace(index, oldStr.size(), newStr);
    index += newStr.size();
    replacedChars++;
  }

  return replacedChars;
}

bool StringUtils::StartsWith(std::string_view str1, std::string_view str2)
{
  return str1.substr(0, str2.size()) == str2; // Binary compare
}

bool StringUtils::StartsWithNoCase(std::string_view str1, std::string_view str2)
{
  // FoldCase both strings and then compare the string. Slower than a byte at a time,
  // but more accurate for multi-byte characters. In "full case folding" (which
  // is not done here) the length of strings in Unicode codepoints can change.
  // Here, a less advanced algorithm is used which does not change the number
  // of 32-bit Unicode codepoints in a string. HOWEVER, the length in UTF-8
  // BYTES can change. Therefore it is not correct to compare raw UTF8 lengths.

  // Using Utf32 (Unicode code points) is most accurate, so case fold and
  // compare are in that form.

  std::u32string utf32Str1 = StringUtils::ToUtf32(str1);
  std::u32string utf32Str2 = StringUtils::ToUtf32(str2);
  bool result = StringUtils::FoldAndCompareStart(utf32Str1, utf32Str2);
  return result;
}

bool StringUtils::EndsWith(std::string_view str1, std::string_view str2)
{
  // No character conversion required

  if (str1.size() < str2.size())
    return false;
  return str1.substr(str1.size() - str2.size(), str2.size()) == str2; // Binary compare
}

bool StringUtils::EndsWithNoCase(std::string_view str1, std::string_view str2)
{
  // FoldCase both strings and then compare the end of str1 for str2.
  // Slower than a byte at a time, but more accurate for multi-byte characters. In "full case
  // folding" (which is not done here) the length of strings can change during folding. It still
  // may be possible for length to change, but at least for now, assume that it does not.
  //
  // Using Utf32 (Unicode code points) is most accurate, so case fold and
  // compare in that form.
  //

  std::u32string utf32Str1 = StringUtils::ToUtf32(str1);
  std::u32string utf32Str2 = StringUtils::ToUtf32(str2);
  bool result = StringUtils::FoldAndCompareEnd(utf32Str1, utf32Str2);
  return result;
}

std::vector<std::string> StringUtils::Split(const std::string& input,
                                            const std::string& delimiter,
                                            unsigned int iMaxStrings)
{
  std::vector<std::string> result;
  SplitTo(std::back_inserter(result), input, delimiter, iMaxStrings);
  return result;
}

std::vector<std::string> StringUtils::Split(const std::string& input,
                                            const char delimiter,
                                            size_t iMaxStrings)
{
  std::vector<std::string> result;
  SplitTo(std::back_inserter(result), input, delimiter, iMaxStrings);
  return result;
}

std::vector<std::string> StringUtils::Split(const std::string& input,
                                            const std::vector<std::string>& delimiters)
{
  std::vector<std::string> result;
  SplitTo(std::back_inserter(result), input, delimiters);
  return result;
}

std::vector<std::string> StringUtils::SplitMulti(const std::vector<std::string>& input,
                                                 const std::vector<std::string>& delimiters,
                                                 size_t iMaxStrings /* = 0 */)
{
  if (input.empty())
    return std::vector<std::string>();

  std::vector<std::string> results(input);

  if (delimiters.empty() || (iMaxStrings > 0 && iMaxStrings <= input.size()))
    return results;

  std::vector<std::string> strings1;
  if (iMaxStrings == 0)
  {
    for (size_t di = 0; di < delimiters.size(); di++)
    {
      for (size_t i = 0; i < results.size(); i++)
      {
        std::vector<std::string> substrings = StringUtils::Split(results[i], delimiters[di]);
        for (size_t j = 0; j < substrings.size(); j++)
          strings1.push_back(substrings[j]);
      }
      results = strings1;
      strings1.clear();
    }
    return results;
  }

  // Control the number of strings input is split into, keeping the original strings.
  // Note iMaxStrings > input.size()
  int64_t iNew = iMaxStrings - results.size();
  for (size_t di = 0; di < delimiters.size(); di++)
  {
    for (size_t i = 0; i < results.size(); i++)
    {
      if (iNew > 0)
      {
        std::vector<std::string> substrings =
            StringUtils::Split(results[i], delimiters[di], iNew + 1);
        iNew = iNew - substrings.size() + 1;
        for (size_t j = 0; j < substrings.size(); j++)
          strings1.push_back(substrings[j]);
      }
      else
        strings1.push_back(results[i]);
    }
    results = strings1;
    iNew = iMaxStrings - results.size();
    strings1.clear();
    if ((iNew <= 0))
      break; //Stop trying any more delimiters
  }
  return results;
}

// returns the number of occurrences of strFind in strInput.
int StringUtils::FindNumber(const std::string& strInput, const std::string& strFind)
{
  size_t pos = strInput.find(strFind, 0);
  int numfound = 0;
  while (pos != std::string::npos)
  {
    numfound++;
    pos = strInput.find(strFind, pos + 1);
  }
  return numfound;
}

namespace
{
// Plane maps for MySQL utf8_general_ci (now known as utf8mb3_general_ci) collation
// Derived from https://github.com/MariaDB/server/blob/10.5/strings/ctype-utf8.c

// clang-format off
static constexpr uint16_t plane00[]
{
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
  0x0060, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
  0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
  0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x039C, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x00C6, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049,
  0x00D0, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x00D7, 0x00D8, 0x0055, 0x0055, 0x0055, 0x0055, 0x0059, 0x00DE, 0x0053,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x00C6, 0x0043, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049,
  0x00D0, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x00F7, 0x00D8, 0x0055, 0x0055, 0x0055, 0x0055, 0x0059, 0x00DE, 0x0059
};

static constexpr uint16_t plane01[]
{
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0043, 0x0044, 0x0044,
  0x0110, 0x0110, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0047, 0x0047, 0x0047, 0x0047,
  0x0047, 0x0047, 0x0047, 0x0047, 0x0048, 0x0048, 0x0126, 0x0126, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049, 0x0049,
  0x0049, 0x0049, 0x0132, 0x0132, 0x004A, 0x004A, 0x004B, 0x004B, 0x0138, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x013F,
  0x013F, 0x0141, 0x0141, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x0149, 0x014A, 0x014A, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x0152, 0x0152, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053,
  0x0053, 0x0053, 0x0054, 0x0054, 0x0054, 0x0054, 0x0166, 0x0166, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055,
  0x0055, 0x0055, 0x0055, 0x0055, 0x0057, 0x0057, 0x0059, 0x0059, 0x0059, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x0053,
  0x0180, 0x0181, 0x0182, 0x0182, 0x0184, 0x0184, 0x0186, 0x0187, 0x0187, 0x0189, 0x018A, 0x018B, 0x018B, 0x018D, 0x018E, 0x018F,
  0x0190, 0x0191, 0x0191, 0x0193, 0x0194, 0x01F6, 0x0196, 0x0197, 0x0198, 0x0198, 0x019A, 0x019B, 0x019C, 0x019D, 0x019E, 0x019F,
  0x004F, 0x004F, 0x01A2, 0x01A2, 0x01A4, 0x01A4, 0x01A6, 0x01A7, 0x01A7, 0x01A9, 0x01AA, 0x01AB, 0x01AC, 0x01AC, 0x01AE, 0x0055,
  0x0055, 0x01B1, 0x01B2, 0x01B3, 0x01B3, 0x01B5, 0x01B5, 0x01B7, 0x01B8, 0x01B8, 0x01BA, 0x01BB, 0x01BC, 0x01BC, 0x01BE, 0x01F7,
  0x01C0, 0x01C1, 0x01C2, 0x01C3, 0x01C4, 0x01C4, 0x01C4, 0x01C7, 0x01C7, 0x01C7, 0x01CA, 0x01CA, 0x01CA, 0x0041, 0x0041, 0x0049,
  0x0049, 0x004F, 0x004F, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x018E, 0x0041, 0x0041,
  0x0041, 0x0041, 0x00C6, 0x00C6, 0x01E4, 0x01E4, 0x0047, 0x0047, 0x004B, 0x004B, 0x004F, 0x004F, 0x004F, 0x004F, 0x01B7, 0x01B7,
  0x004A, 0x01F1, 0x01F1, 0x01F1, 0x0047, 0x0047, 0x01F6, 0x01F7, 0x004E, 0x004E, 0x0041, 0x0041, 0x00C6, 0x00C6, 0x00D8, 0x00D8
};

static constexpr uint16_t plane02[]
{
  0x0041, 0x0041, 0x0041, 0x0041, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004F, 0x004F, 0x004F, 0x004F,
  0x0052, 0x0052, 0x0052, 0x0052, 0x0055, 0x0055, 0x0055, 0x0055, 0x0053, 0x0053, 0x0054, 0x0054, 0x021C, 0x021C, 0x0048, 0x0048,
  0x0220, 0x0221, 0x0222, 0x0222, 0x0224, 0x0224, 0x0041, 0x0041, 0x0045, 0x0045, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x0059, 0x0059, 0x0234, 0x0235, 0x0236, 0x0237, 0x0238, 0x0239, 0x023A, 0x023B, 0x023C, 0x023D, 0x023E, 0x023F,
  0x0240, 0x0241, 0x0242, 0x0243, 0x0244, 0x0245, 0x0246, 0x0247, 0x0248, 0x0249, 0x024A, 0x024B, 0x024C, 0x024D, 0x024E, 0x024F,
  0x0250, 0x0251, 0x0252, 0x0181, 0x0186, 0x0255, 0x0189, 0x018A, 0x0258, 0x018F, 0x025A, 0x0190, 0x025C, 0x025D, 0x025E, 0x025F,
  0x0193, 0x0261, 0x0262, 0x0194, 0x0264, 0x0265, 0x0266, 0x0267, 0x0197, 0x0196, 0x026A, 0x026B, 0x026C, 0x026D, 0x026E, 0x019C,
  0x0270, 0x0271, 0x019D, 0x0273, 0x0274, 0x019F, 0x0276, 0x0277, 0x0278, 0x0279, 0x027A, 0x027B, 0x027C, 0x027D, 0x027E, 0x027F,
  0x01A6, 0x0281, 0x0282, 0x01A9, 0x0284, 0x0285, 0x0286, 0x0287, 0x01AE, 0x0289, 0x01B1, 0x01B2, 0x028C, 0x028D, 0x028E, 0x028F,
  0x0290, 0x0291, 0x01B7, 0x0293, 0x0294, 0x0295, 0x0296, 0x0297, 0x0298, 0x0299, 0x029A, 0x029B, 0x029C, 0x029D, 0x029E, 0x029F,
  0x02A0, 0x02A1, 0x02A2, 0x02A3, 0x02A4, 0x02A5, 0x02A6, 0x02A7, 0x02A8, 0x02A9, 0x02AA, 0x02AB, 0x02AC, 0x02AD, 0x02AE, 0x02AF,
  0x02B0, 0x02B1, 0x02B2, 0x02B3, 0x02B4, 0x02B5, 0x02B6, 0x02B7, 0x02B8, 0x02B9, 0x02BA, 0x02BB, 0x02BC, 0x02BD, 0x02BE, 0x02BF,
  0x02C0, 0x02C1, 0x02C2, 0x02C3, 0x02C4, 0x02C5, 0x02C6, 0x02C7, 0x02C8, 0x02C9, 0x02CA, 0x02CB, 0x02CC, 0x02CD, 0x02CE, 0x02CF,
  0x02D0, 0x02D1, 0x02D2, 0x02D3, 0x02D4, 0x02D5, 0x02D6, 0x02D7, 0x02D8, 0x02D9, 0x02DA, 0x02DB, 0x02DC, 0x02DD, 0x02DE, 0x02DF,
  0x02E0, 0x02E1, 0x02E2, 0x02E3, 0x02E4, 0x02E5, 0x02E6, 0x02E7, 0x02E8, 0x02E9, 0x02EA, 0x02EB, 0x02EC, 0x02ED, 0x02EE, 0x02EF,
  0x02F0, 0x02F1, 0x02F2, 0x02F3, 0x02F4, 0x02F5, 0x02F6, 0x02F7, 0x02F8, 0x02F9, 0x02FA, 0x02FB, 0x02FC, 0x02FD, 0x02FE, 0x02FF
};

static constexpr uint16_t plane03[]
{
  0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 0x0307, 0x0308, 0x0309, 0x030A, 0x030B, 0x030C, 0x030D, 0x030E, 0x030F,
  0x0310, 0x0311, 0x0312, 0x0313, 0x0314, 0x0315, 0x0316, 0x0317, 0x0318, 0x0319, 0x031A, 0x031B, 0x031C, 0x031D, 0x031E, 0x031F,
  0x0320, 0x0321, 0x0322, 0x0323, 0x0324, 0x0325, 0x0326, 0x0327, 0x0328, 0x0329, 0x032A, 0x032B, 0x032C, 0x032D, 0x032E, 0x032F,
  0x0330, 0x0331, 0x0332, 0x0333, 0x0334, 0x0335, 0x0336, 0x0337, 0x0338, 0x0339, 0x033A, 0x033B, 0x033C, 0x033D, 0x033E, 0x033F,
  0x0340, 0x0341, 0x0342, 0x0343, 0x0344, 0x0399, 0x0346, 0x0347, 0x0348, 0x0349, 0x034A, 0x034B, 0x034C, 0x034D, 0x034E, 0x034F,
  0x0350, 0x0351, 0x0352, 0x0353, 0x0354, 0x0355, 0x0356, 0x0357, 0x0358, 0x0359, 0x035A, 0x035B, 0x035C, 0x035D, 0x035E, 0x035F,
  0x0360, 0x0361, 0x0362, 0x0363, 0x0364, 0x0365, 0x0366, 0x0367, 0x0368, 0x0369, 0x036A, 0x036B, 0x036C, 0x036D, 0x036E, 0x036F,
  0x0370, 0x0371, 0x0372, 0x0373, 0x0374, 0x0375, 0x0376, 0x0377, 0x0378, 0x0379, 0x037A, 0x037B, 0x037C, 0x037D, 0x037E, 0x037F,
  0x0380, 0x0381, 0x0382, 0x0383, 0x0384, 0x0385, 0x0391, 0x0387, 0x0395, 0x0397, 0x0399, 0x038B, 0x039F, 0x038D, 0x03A5, 0x03A9,
  0x0399, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x03A2, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x0399, 0x03A5, 0x0391, 0x0395, 0x0397, 0x0399,
  0x03A5, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x03A3, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7, 0x03A8, 0x03A9, 0x0399, 0x03A5, 0x039F, 0x03A5, 0x03A9, 0x03CF,
  0x0392, 0x0398, 0x03D2, 0x03D2, 0x03D2, 0x03A6, 0x03A0, 0x03D7, 0x03D8, 0x03D9, 0x03DA, 0x03DA, 0x03DC, 0x03DC, 0x03DE, 0x03DE,
  0x03E0, 0x03E0, 0x03E2, 0x03E2, 0x03E4, 0x03E4, 0x03E6, 0x03E6, 0x03E8, 0x03E8, 0x03EA, 0x03EA, 0x03EC, 0x03EC, 0x03EE, 0x03EE,
  0x039A, 0x03A1, 0x03A3, 0x03F3, 0x03F4, 0x03F5, 0x03F6, 0x03F7, 0x03F8, 0x03F9, 0x03FA, 0x03FB, 0x03FC, 0x03FD, 0x03FE, 0x03FF
};

static constexpr uint16_t plane04[]
{
  0x0415, 0x0415, 0x0402, 0x0413, 0x0404, 0x0405, 0x0406, 0x0406, 0x0408, 0x0409, 0x040A, 0x040B, 0x041A, 0x0418, 0x0423, 0x040F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0415, 0x0415, 0x0402, 0x0413, 0x0404, 0x0405, 0x0406, 0x0406, 0x0408, 0x0409, 0x040A, 0x040B, 0x041A, 0x0418, 0x0423, 0x040F,
  0x0460, 0x0460, 0x0462, 0x0462, 0x0464, 0x0464, 0x0466, 0x0466, 0x0468, 0x0468, 0x046A, 0x046A, 0x046C, 0x046C, 0x046E, 0x046E,
  0x0470, 0x0470, 0x0472, 0x0472, 0x0474, 0x0474, 0x0474, 0x0474, 0x0478, 0x0478, 0x047A, 0x047A, 0x047C, 0x047C, 0x047E, 0x047E,
  0x0480, 0x0480, 0x0482, 0x0483, 0x0484, 0x0485, 0x0486, 0x0487, 0x0488, 0x0489, 0x048A, 0x048B, 0x048C, 0x048C, 0x048E, 0x048E,
  0x0490, 0x0490, 0x0492, 0x0492, 0x0494, 0x0494, 0x0496, 0x0496, 0x0498, 0x0498, 0x049A, 0x049A, 0x049C, 0x049C, 0x049E, 0x049E,
  0x04A0, 0x04A0, 0x04A2, 0x04A2, 0x04A4, 0x04A4, 0x04A6, 0x04A6, 0x04A8, 0x04A8, 0x04AA, 0x04AA, 0x04AC, 0x04AC, 0x04AE, 0x04AE,
  0x04B0, 0x04B0, 0x04B2, 0x04B2, 0x04B4, 0x04B4, 0x04B6, 0x04B6, 0x04B8, 0x04B8, 0x04BA, 0x04BA, 0x04BC, 0x04BC, 0x04BE, 0x04BE,
  0x04C0, 0x0416, 0x0416, 0x04C3, 0x04C3, 0x04C5, 0x04C6, 0x04C7, 0x04C7, 0x04C9, 0x04CA, 0x04CB, 0x04CB, 0x04CD, 0x04CE, 0x04CF,
  0x0410, 0x0410, 0x0410, 0x0410, 0x04D4, 0x04D4, 0x0415, 0x0415, 0x04D8, 0x04D8, 0x04D8, 0x04D8, 0x0416, 0x0416, 0x0417, 0x0417,
  0x04E0, 0x04E0, 0x0418, 0x0418, 0x0418, 0x0418, 0x041E, 0x041E, 0x04E8, 0x04E8, 0x04E8, 0x04E8, 0x042D, 0x042D, 0x0423, 0x0423,
  0x0423, 0x0423, 0x0423, 0x0423, 0x0427, 0x0427, 0x04F6, 0x04F7, 0x042B, 0x042B, 0x04FA, 0x04FB, 0x04FC, 0x04FD, 0x04FE, 0x04FF
};

static constexpr uint16_t plane05[]
{
  0x0500, 0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0506, 0x0507, 0x0508, 0x0509, 0x050A, 0x050B, 0x050C, 0x050D, 0x050E, 0x050F,
  0x0510, 0x0511, 0x0512, 0x0513, 0x0514, 0x0515, 0x0516, 0x0517, 0x0518, 0x0519, 0x051A, 0x051B, 0x051C, 0x051D, 0x051E, 0x051F,
  0x0520, 0x0521, 0x0522, 0x0523, 0x0524, 0x0525, 0x0526, 0x0527, 0x0528, 0x0529, 0x052A, 0x052B, 0x052C, 0x052D, 0x052E, 0x052F,
  0x0530, 0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F,
  0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F,
  0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0x0557, 0x0558, 0x0559, 0x055A, 0x055B, 0x055C, 0x055D, 0x055E, 0x055F,
  0x0560, 0x0531, 0x0532, 0x0533, 0x0534, 0x0535, 0x0536, 0x0537, 0x0538, 0x0539, 0x053A, 0x053B, 0x053C, 0x053D, 0x053E, 0x053F,
  0x0540, 0x0541, 0x0542, 0x0543, 0x0544, 0x0545, 0x0546, 0x0547, 0x0548, 0x0549, 0x054A, 0x054B, 0x054C, 0x054D, 0x054E, 0x054F,
  0x0550, 0x0551, 0x0552, 0x0553, 0x0554, 0x0555, 0x0556, 0x0587, 0x0588, 0x0589, 0x058A, 0x058B, 0x058C, 0x058D, 0x058E, 0x058F,
  0x0590, 0x0591, 0x0592, 0x0593, 0x0594, 0x0595, 0x0596, 0x0597, 0x0598, 0x0599, 0x059A, 0x059B, 0x059C, 0x059D, 0x059E, 0x059F,
  0x05A0, 0x05A1, 0x05A2, 0x05A3, 0x05A4, 0x05A5, 0x05A6, 0x05A7, 0x05A8, 0x05A9, 0x05AA, 0x05AB, 0x05AC, 0x05AD, 0x05AE, 0x05AF,
  0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7, 0x05B8, 0x05B9, 0x05BA, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF,
  0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05C4, 0x05C5, 0x05C6, 0x05C7, 0x05C8, 0x05C9, 0x05CA, 0x05CB, 0x05CC, 0x05CD, 0x05CE, 0x05CF,
  0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7, 0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
  0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7, 0x05E8, 0x05E9, 0x05EA, 0x05EB, 0x05EC, 0x05ED, 0x05EE, 0x05EF,
  0x05F0, 0x05F1, 0x05F2, 0x05F3, 0x05F4, 0x05F5, 0x05F6, 0x05F7, 0x05F8, 0x05F9, 0x05FA, 0x05FB, 0x05FC, 0x05FD, 0x05FE, 0x05FF
};

static constexpr uint16_t plane1E[]
{
  0x0041, 0x0041, 0x0042, 0x0042, 0x0042, 0x0042, 0x0042, 0x0042, 0x0043, 0x0043, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044, 0x0044,
  0x0044, 0x0044, 0x0044, 0x0044, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0046, 0x0046,
  0x0047, 0x0047, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0048, 0x0049, 0x0049, 0x0049, 0x0049,
  0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004B, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004C, 0x004D, 0x004D,
  0x004D, 0x004D, 0x004D, 0x004D, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004E, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x0050, 0x0050, 0x0050, 0x0050, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052, 0x0052,
  0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0053, 0x0054, 0x0054, 0x0054, 0x0054, 0x0054, 0x0054,
  0x0054, 0x0054, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0056, 0x0056, 0x0056, 0x0056,
  0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0057, 0x0058, 0x0058, 0x0058, 0x0058, 0x0059, 0x0059,
  0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x005A, 0x0048, 0x0054, 0x0057, 0x0059, 0x1E9A, 0x0053, 0x1E9C, 0x1E9D, 0x1E9E, 0x1E9F,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041,
  0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0041, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045,
  0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0045, 0x0049, 0x0049, 0x0049, 0x0049, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F, 0x004F,
  0x004F, 0x004F, 0x004F, 0x004F, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055, 0x0055,
  0x0055, 0x0055, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x0059, 0x1EFA, 0x1EFB, 0x1EFC, 0x1EFD, 0x1EFE, 0x1EFF
};

static constexpr uint16_t plane1F[]
{
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391,
  0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x1F16, 0x1F17, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x0395, 0x1F1E, 0x1F1F,
  0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397,
  0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399,
  0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x1F46, 0x1F47, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x039F, 0x1F4E, 0x1F4F,
  0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x1F58, 0x03A5, 0x1F5A, 0x03A5, 0x1F5C, 0x03A5, 0x1F5E, 0x03A5,
  0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9,
  0x0391, 0x1FBB, 0x0395, 0x1FC9, 0x0397, 0x1FCB, 0x0399, 0x1FDB, 0x039F, 0x1FF9, 0x03A5, 0x1FEB, 0x03A9, 0x1FFB, 0x1F7E, 0x1F7F,
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391,
  0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397, 0x0397,
  0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9, 0x03A9,
  0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x1FB5, 0x0391, 0x0391, 0x0391, 0x0391, 0x0391, 0x1FBB, 0x0391, 0x1FBD, 0x0399, 0x1FBF,
  0x1FC0, 0x1FC1, 0x0397, 0x0397, 0x0397, 0x1FC5, 0x0397, 0x0397, 0x0395, 0x1FC9, 0x0397, 0x1FCB, 0x0397, 0x1FCD, 0x1FCE, 0x1FCF,
  0x0399, 0x0399, 0x0399, 0x1FD3, 0x1FD4, 0x1FD5, 0x0399, 0x0399, 0x0399, 0x0399, 0x0399, 0x1FDB, 0x1FDC, 0x1FDD, 0x1FDE, 0x1FDF,
  0x03A5, 0x03A5, 0x03A5, 0x1FE3, 0x03A1, 0x03A1, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x03A5, 0x1FEB, 0x03A1, 0x1FED, 0x1FEE, 0x1FEF,
  0x1FF0, 0x1FF1, 0x03A9, 0x03A9, 0x03A9, 0x1FF5, 0x03A9, 0x03A9, 0x039F, 0x1FF9, 0x03A9, 0x1FFB, 0x03A9, 0x1FFD, 0x1FFE, 0x1FFF
};

static constexpr uint16_t plane21[]
{
  0x2100, 0x2101, 0x2102, 0x2103, 0x2104, 0x2105, 0x2106, 0x2107, 0x2108, 0x2109, 0x210A, 0x210B, 0x210C, 0x210D, 0x210E, 0x210F,
  0x2110, 0x2111, 0x2112, 0x2113, 0x2114, 0x2115, 0x2116, 0x2117, 0x2118, 0x2119, 0x211A, 0x211B, 0x211C, 0x211D, 0x211E, 0x211F,
  0x2120, 0x2121, 0x2122, 0x2123, 0x2124, 0x2125, 0x2126, 0x2127, 0x2128, 0x2129, 0x212A, 0x212B, 0x212C, 0x212D, 0x212E, 0x212F,
  0x2130, 0x2131, 0x2132, 0x2133, 0x2134, 0x2135, 0x2136, 0x2137, 0x2138, 0x2139, 0x213A, 0x213B, 0x213C, 0x213D, 0x213E, 0x213F,
  0x2140, 0x2141, 0x2142, 0x2143, 0x2144, 0x2145, 0x2146, 0x2147, 0x2148, 0x2149, 0x214A, 0x214B, 0x214C, 0x214D, 0x214E, 0x214F,
  0x2150, 0x2151, 0x2152, 0x2153, 0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215A, 0x215B, 0x215C, 0x215D, 0x215E, 0x215F,
  0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F,
  0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169, 0x216A, 0x216B, 0x216C, 0x216D, 0x216E, 0x216F,
  0x2180, 0x2181, 0x2182, 0x2183, 0x2184, 0x2185, 0x2186, 0x2187, 0x2188, 0x2189, 0x218A, 0x218B, 0x218C, 0x218D, 0x218E, 0x218F,
  0x2190, 0x2191, 0x2192, 0x2193, 0x2194, 0x2195, 0x2196, 0x2197, 0x2198, 0x2199, 0x219A, 0x219B, 0x219C, 0x219D, 0x219E, 0x219F,
  0x21A0, 0x21A1, 0x21A2, 0x21A3, 0x21A4, 0x21A5, 0x21A6, 0x21A7, 0x21A8, 0x21A9, 0x21AA, 0x21AB, 0x21AC, 0x21AD, 0x21AE, 0x21AF,
  0x21B0, 0x21B1, 0x21B2, 0x21B3, 0x21B4, 0x21B5, 0x21B6, 0x21B7, 0x21B8, 0x21B9, 0x21BA, 0x21BB, 0x21BC, 0x21BD, 0x21BE, 0x21BF,
  0x21C0, 0x21C1, 0x21C2, 0x21C3, 0x21C4, 0x21C5, 0x21C6, 0x21C7, 0x21C8, 0x21C9, 0x21CA, 0x21CB, 0x21CC, 0x21CD, 0x21CE, 0x21CF,
  0x21D0, 0x21D1, 0x21D2, 0x21D3, 0x21D4, 0x21D5, 0x21D6, 0x21D7, 0x21D8, 0x21D9, 0x21DA, 0x21DB, 0x21DC, 0x21DD, 0x21DE, 0x21DF,
  0x21E0, 0x21E1, 0x21E2, 0x21E3, 0x21E4, 0x21E5, 0x21E6, 0x21E7, 0x21E8, 0x21E9, 0x21EA, 0x21EB, 0x21EC, 0x21ED, 0x21EE, 0x21EF,
  0x21F0, 0x21F1, 0x21F2, 0x21F3, 0x21F4, 0x21F5, 0x21F6, 0x21F7, 0x21F8, 0x21F9, 0x21FA, 0x21FB, 0x21FC, 0x21FD, 0x21FE, 0x21FF
};

static constexpr uint16_t plane24[]
{
  0x2400, 0x2401, 0x2402, 0x2403, 0x2404, 0x2405, 0x2406, 0x2407, 0x2408, 0x2409, 0x240A, 0x240B, 0x240C, 0x240D, 0x240E, 0x240F,
  0x2410, 0x2411, 0x2412, 0x2413, 0x2414, 0x2415, 0x2416, 0x2417, 0x2418, 0x2419, 0x241A, 0x241B, 0x241C, 0x241D, 0x241E, 0x241F,
  0x2420, 0x2421, 0x2422, 0x2423, 0x2424, 0x2425, 0x2426, 0x2427, 0x2428, 0x2429, 0x242A, 0x242B, 0x242C, 0x242D, 0x242E, 0x242F,
  0x2430, 0x2431, 0x2432, 0x2433, 0x2434, 0x2435, 0x2436, 0x2437, 0x2438, 0x2439, 0x243A, 0x243B, 0x243C, 0x243D, 0x243E, 0x243F,
  0x2440, 0x2441, 0x2442, 0x2443, 0x2444, 0x2445, 0x2446, 0x2447, 0x2448, 0x2449, 0x244A, 0x244B, 0x244C, 0x244D, 0x244E, 0x244F,
  0x2450, 0x2451, 0x2452, 0x2453, 0x2454, 0x2455, 0x2456, 0x2457, 0x2458, 0x2459, 0x245A, 0x245B, 0x245C, 0x245D, 0x245E, 0x245F,
  0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465, 0x2466, 0x2467, 0x2468, 0x2469, 0x246A, 0x246B, 0x246C, 0x246D, 0x246E, 0x246F,
  0x2470, 0x2471, 0x2472, 0x2473, 0x2474, 0x2475, 0x2476, 0x2477, 0x2478, 0x2479, 0x247A, 0x247B, 0x247C, 0x247D, 0x247E, 0x247F,
  0x2480, 0x2481, 0x2482, 0x2483, 0x2484, 0x2485, 0x2486, 0x2487, 0x2488, 0x2489, 0x248A, 0x248B, 0x248C, 0x248D, 0x248E, 0x248F,
  0x2490, 0x2491, 0x2492, 0x2493, 0x2494, 0x2495, 0x2496, 0x2497, 0x2498, 0x2499, 0x249A, 0x249B, 0x249C, 0x249D, 0x249E, 0x249F,
  0x24A0, 0x24A1, 0x24A2, 0x24A3, 0x24A4, 0x24A5, 0x24A6, 0x24A7, 0x24A8, 0x24A9, 0x24AA, 0x24AB, 0x24AC, 0x24AD, 0x24AE, 0x24AF,
  0x24B0, 0x24B1, 0x24B2, 0x24B3, 0x24B4, 0x24B5, 0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB, 0x24BC, 0x24BD, 0x24BE, 0x24BF,
  0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5, 0x24C6, 0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF,
  0x24B6, 0x24B7, 0x24B8, 0x24B9, 0x24BA, 0x24BB, 0x24BC, 0x24BD, 0x24BE, 0x24BF, 0x24C0, 0x24C1, 0x24C2, 0x24C3, 0x24C4, 0x24C5,
  0x24C6, 0x24C7, 0x24C8, 0x24C9, 0x24CA, 0x24CB, 0x24CC, 0x24CD, 0x24CE, 0x24CF, 0x24EA, 0x24EB, 0x24EC, 0x24ED, 0x24EE, 0x24EF,
  0x24F0, 0x24F1, 0x24F2, 0x24F3, 0x24F4, 0x24F5, 0x24F6, 0x24F7, 0x24F8, 0x24F9, 0x24FA, 0x24FB, 0x24FC, 0x24FD, 0x24FE, 0x24FF
};

static constexpr uint16_t planeFF[]
{
  0xFF00, 0xFF01, 0xFF02, 0xFF03, 0xFF04, 0xFF05, 0xFF06, 0xFF07, 0xFF08, 0xFF09, 0xFF0A, 0xFF0B, 0xFF0C, 0xFF0D, 0xFF0E, 0xFF0F,
  0xFF10, 0xFF11, 0xFF12, 0xFF13, 0xFF14, 0xFF15, 0xFF16, 0xFF17, 0xFF18, 0xFF19, 0xFF1A, 0xFF1B, 0xFF1C, 0xFF1D, 0xFF1E, 0xFF1F,
  0xFF20, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
  0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFF3B, 0xFF3C, 0xFF3D, 0xFF3E, 0xFF3F,
  0xFF40, 0xFF21, 0xFF22, 0xFF23, 0xFF24, 0xFF25, 0xFF26, 0xFF27, 0xFF28, 0xFF29, 0xFF2A, 0xFF2B, 0xFF2C, 0xFF2D, 0xFF2E, 0xFF2F,
  0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37, 0xFF38, 0xFF39, 0xFF3A, 0xFF5B, 0xFF5C, 0xFF5D, 0xFF5E, 0xFF5F,
  0xFF60, 0xFF61, 0xFF62, 0xFF63, 0xFF64, 0xFF65, 0xFF66, 0xFF67, 0xFF68, 0xFF69, 0xFF6A, 0xFF6B, 0xFF6C, 0xFF6D, 0xFF6E, 0xFF6F,
  0xFF70, 0xFF71, 0xFF72, 0xFF73, 0xFF74, 0xFF75, 0xFF76, 0xFF77, 0xFF78, 0xFF79, 0xFF7A, 0xFF7B, 0xFF7C, 0xFF7D, 0xFF7E, 0xFF7F,
  0xFF80, 0xFF81, 0xFF82, 0xFF83, 0xFF84, 0xFF85, 0xFF86, 0xFF87, 0xFF88, 0xFF89, 0xFF8A, 0xFF8B, 0xFF8C, 0xFF8D, 0xFF8E, 0xFF8F,
  0xFF90, 0xFF91, 0xFF92, 0xFF93, 0xFF94, 0xFF95, 0xFF96, 0xFF97, 0xFF98, 0xFF99, 0xFF9A, 0xFF9B, 0xFF9C, 0xFF9D, 0xFF9E, 0xFF9F,
  0xFFA0, 0xFFA1, 0xFFA2, 0xFFA3, 0xFFA4, 0xFFA5, 0xFFA6, 0xFFA7, 0xFFA8, 0xFFA9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE, 0xFFAF,
  0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6, 0xFFB7, 0xFFB8, 0xFFB9, 0xFFBA, 0xFFBB, 0xFFBC, 0xFFBD, 0xFFBE, 0xFFBF,
  0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF,
  0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD, 0xFFDE, 0xFFDF,
  0xFFE0, 0xFFE1, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6, 0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA, 0xFFEB, 0xFFEC, 0xFFED, 0xFFEE, 0xFFEF,
  0xFFF0, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF
};

static constexpr const uint16_t* const planemap[256]
{
    plane00, plane01, plane02, plane03, plane04, plane05, NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, plane1E, plane1F, NULL,
    plane21, NULL,    NULL,    plane24, NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, NULL, NULL,    NULL,    NULL,
    NULL,    NULL,    planeFF
};
// clang-format on

static wchar_t GetCollationWeight(const wchar_t& r)
{
  // Lookup the "weight" of a UTF8 char, equivalent lowercase ascii letter, in the plane map,
  // the character comparison value used by using "accent folding" collation utf8_general_ci
  // in MySQL (AKA utf8mb3_general_ci in MariaDB 10)
  auto index = r >> 8;
  if (index > 255)
    return 0xFFFD;
  auto plane = planemap[index];
  if (plane == nullptr)
    return r;
  return static_cast<wchar_t>(plane[r & 0xFF]);
}
} // namespace

// Compares separately the numeric and alphabetic parts of a wide string.
// returns negative if left < right, positive if left > right
// and 0 if they are identical.
// See also the equivalent StringUtils::AlphaNumericCollation() for UFT8 data
int64_t StringUtils::AlphaNumericCompare(const wchar_t* left, const wchar_t* right)
{
  const wchar_t* l = left;
  const wchar_t* r = right;
  const wchar_t *ld, *rd;
  wchar_t lc, rc;
  int64_t lnum, rnum;
  bool lsym, rsym;
  while (*l != 0 && *r != 0)
  {
    // check if we have a numerical value
    if (*l >= L'0' && *l <= L'9' && *r >= L'0' && *r <= L'9')
    {
      ld = l;
      lnum = *ld++ - L'0';
      while (*ld >= L'0' && *ld <= L'9' && ld < l + 15)
      { // compare only up to 15 digits
        lnum *= 10;
        lnum += *ld++ - L'0';
      }
      rd = r;
      rnum = *rd++ - L'0';
      while (*rd >= L'0' && *rd <= L'9' && rd < r + 15)
      { // compare only up to 15 digits
        rnum *= 10;
        rnum += *rd++ - L'0';
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return lnum - rnum;
      }
      l = ld;
      r = rd;
      continue;
    }

    lc = *l;
    rc = *r;
    // Put ascii punctuation and symbols e.g. !#$&()*+,-./:;<=>?@[\]^_ `{|}~ above the other
    // alphanumeric ascii, rather than some being mixed between the numbers and letters, and
    // above all other unicode letters, symbols and punctuation.
    // (Locale collation of these chars varies across platforms)
    lsym = (lc >= 32 && lc < L'0') || (lc > L'9' && lc < L'A') || (lc > L'Z' && lc < L'a') ||
           (lc > L'z' && lc < 128);
    rsym = (rc >= 32 && rc < L'0') || (rc > L'9' && rc < L'A') || (rc > L'Z' && rc < L'a') ||
           (rc > L'z' && rc < 128);
    if (lsym && !rsym)
      return -1;
    if (!lsym && rsym)
      return 1;
    if (lsym && rsym)
    {
      if (lc != rc)
        return static_cast<int64_t>(lc) - static_cast<int64_t>(rc);
      else
      { // Same symbol advance to next wchar
        l++;
        r++;
        continue;
      }
    }
    if (!g_langInfo.UseLocaleCollation())
    {
      // Apply case sensitive accent folding collation to non-ascii chars.
      // This mimics utf8_general_ci collation, and provides simple collation of LATIN-1 chars
      // for any platformthat doesn't have a language specific collate facet implemented
      if (lc > 128)
        lc = GetCollationWeight(lc);
      if (rc > 128)
        rc = GetCollationWeight(rc);
    }
    // Do case less comparison, convert ascii upper case to lower case
    if (lc >= L'A' && lc <= L'Z')
      lc += L'a' - L'A';
    if (rc >= L'A' && rc <= L'Z')
      rc += L'a' - L'A';

    if (lc != rc)
    {
      if (!g_langInfo.UseLocaleCollation())
      {
        // Compare unicode (having applied accent folding collation to non-ascii chars).
        int i = wcsncmp(&lc, &rc, 1);
        return i;
      }
      else
      {
        // Fetch collation facet from locale to do comparison of wide char although on some
        // platforms this is not language specific but just compares unicode
        const std::collate<wchar_t>& coll =
            std::use_facet<std::collate<wchar_t>>(StringUtils::GetSystemLocale());
        int cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1);
        if (cmp_res != 0)
          return cmp_res;
      }
    }
    l++;
    r++;
  }
  if (*r)
  { // r is longer
    return -1;
  }
  else if (*l)
  { // l is longer
    return 1;
  }
  return 0; // files are the same
}

namespace
{
/*
  Convert the UTF8 character to which z points into a 31-bit Unicode point.
  Return how many bytes (0 to 3) of UTF8 data encode the character.
  This only works right if z points to a well-formed UTF8 string.
  Byte-0    Byte-1    Byte-2    Byte-3     Value
  0xxxxxxx                                 00000000 00000000 0xxxxxxx
  110yyyyy  10xxxxxx                       00000000 00000yyy yyxxxxxx
  1110zzzz  10yyyyyy  10xxxxxx             00000000 zzzzyyyy yyxxxxxx
  11110uuu  10uuzzzz  10yyyyyy  10xxxxxx   000uuuuu zzzzyyyy yyxxxxxx
*/
static uint32_t UTF8ToUnicode(const unsigned char* z, int nKey, unsigned char& bytes)
{
  // Lookup table used decode the first byte of a multi-byte UTF8 character
  // clang-format off
  static const unsigned char utf8Trans1[] {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x00, 0x00,
  };
  // clang-format on

  uint32_t c;
  bytes = 0;
  c = z[0];
  if (c >= 0xc0)
  {
    c = utf8Trans1[c - 0xc0];
    int index = 1;
    while (index < nKey && (z[index] & 0xc0) == 0x80)
    {
      c = (c << 6) + (0x3f & z[index]);
      index++;
    }
    if (c < 0x80 || (c & 0xFFFFF800) == 0xD800 || (c & 0xFFFFFFFE) == 0xFFFE)
      c = 0xFFFD;
    bytes = static_cast<unsigned char>(index - 1);
  }
  return c;
}
} // namespace

/*
  SQLite collating function, see sqlite3_create_collation
  The equivalent of AlphaNumericCompare() but for comparing UTF8 encoded data

  This only processes enough data to find a difference, and avoids expensive data conversions.
  When sorting in memory item data is converted once to wstring in advance prior to sorting, the
  SQLite callback function can not do that kind of preparation. Instead, in order to use
  AlphaNumericCompare(), it would have to repeatedly convert the full input data to wstring for
  every pair comparison made. That approach was found to be 10 times slower than using this
  separate routine.
*/
int StringUtils::AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2)
{
  // Get exact matches of shorter text to start of larger test fast
  int n = std::min(nKey1, nKey2);
  int r = memcmp(pKey1, pKey2, n);
  if (r == 0)
    return nKey1 - nKey2;

  //Not a binary match, so process character at a time
  const unsigned char* zA = static_cast<const unsigned char*>(pKey1);
  const unsigned char* zB = static_cast<const unsigned char*>(pKey2);
  wchar_t lc;
  wchar_t rc;
  unsigned char bytes;
  int64_t lnum;
  int64_t rnum;
  bool lsym;
  bool rsym;
  int ld;
  int rd;
  int i = 0;
  int j = 0;
  // Looping Unicode point at a time through potentially 1 to 4 multi-byte encoded UTF8 data
  while (i < nKey1 && j < nKey2)
  {
    // Check if we have numerical values, compare only up to 15 digits
    if (isdigit(zA[i]) && isdigit(zB[j]))
    {
      lnum = zA[i] - '0';
      ld = i + 1;
      while (ld < nKey1 && isdigit(zA[ld]) && ld < i + 15)
      {
        lnum *= 10;
        lnum += zA[ld] - '0';
        ld++;
      }
      rnum = zB[j] - '0';
      rd = j + 1;
      while (rd < nKey2 && isdigit(zB[rd]) && rd < j + 15)
      {
        rnum *= 10;
        rnum += zB[rd] - '0';
        rd++;
      }
      // do we have numbers?
      if (lnum != rnum)
      { // yes - and they're different!
        return static_cast<int>(lnum - rnum);
      }
      // Advance to after digits
      i = ld;
      j = rd;
      continue;
    }
    // Put ascii punctuation and symbols e.g. !#$&()*+,-./:;<=>?@[\]^_ `{|}~ before the other
    // alphanumeric ascii, rather than some being mixed between the numbers and letters, and
    // above all other unicode letters, symbols and punctuation.
    // (Locale collation of these chars varies across platforms)
    lsym = (zA[i] >= 32 && zA[i] < '0') || (zA[i] > '9' && zA[i] < 'A') ||
           (zA[i] > 'Z' && zA[i] < 'a') || (zA[i] > 'z' && zA[i] < 128);
    rsym = (zB[j] >= 32 && zB[j] < '0') || (zB[j] > '9' && zB[j] < 'A') ||
           (zB[j] > 'Z' && zB[j] < 'a') || (zB[j] > 'z' && zB[j] < 128);
    if (lsym && !rsym)
      return -1;
    if (!lsym && rsym)
      return 1;
    if (lsym && rsym)
    {
      if (zA[i] != zB[j])
        return static_cast<int>(zA[i]) - static_cast<int>(zB[j]);
      else
      { // Same symbol advance to next
        i++;
        j++;
        continue;
      }
    }
    //Decode single (1 to 4 bytes) UTF8 character to Unicode
    lc = UTF8ToUnicode(&zA[i], nKey1 - i, bytes);
    i += bytes;
    rc = UTF8ToUnicode(&zB[j], nKey2 - j, bytes);
    j += bytes;
    if (!g_langInfo.UseLocaleCollation())
    {
      // Apply case sensitive accent folding collation to non-ascii chars.
      // This mimics utf8_general_ci collation, and provides simple collation of LATIN-1 chars
      // for any platform that doesn't have a language specific collate facet implemented
      if (lc > 128)
        lc = GetCollationWeight(lc);
      if (rc > 128)
        rc = GetCollationWeight(rc);
    }
    // Caseless comparison so convert ascii upper case to lower case
    if (lc >= 'A' && lc <= 'Z')
      lc += 'a' - 'A';
    if (rc >= 'A' && rc <= 'Z')
      rc += 'a' - 'A';

    if (lc != rc)
    {
      if (!g_langInfo.UseLocaleCollation() || (lc <= 128 && rc <= 128))
        // Compare unicode (having applied accent folding collation to non-ascii chars).
        return static_cast<int>(lc) - static_cast<int>(rc);
      else
      {
        // Fetch collation facet from locale to do comparison of wide char although on some
        // platforms this is not language specific but just compares unicode
        const std::collate<wchar_t>& coll =
            std::use_facet<std::collate<wchar_t>>(StringUtils::GetSystemLocale());
        int cmp_res = coll.compare(&lc, &lc + 1, &rc, &rc + 1);
        if (cmp_res != 0)
          return cmp_res;
      }
    }
    i++;
    j++;
  }
  // Compared characters of shortest are the same as longest, length determines order
  return (nKey1 - nKey2);
}

int StringUtils::DateStringToYYYYMMDD(const std::string& dateString)
{
  std::vector<std::string> days = StringUtils::Split(dateString, '-');
  if (days.size() == 1)
    return atoi(days[0].c_str());
  else if (days.size() == 2)
    return atoi(days[0].c_str()) * 100 + atoi(days[1].c_str());
  else if (days.size() == 3)
    return atoi(days[0].c_str()) * 10000 + atoi(days[1].c_str()) * 100 + atoi(days[2].c_str());
  else
    return -1;
}

std::string StringUtils::ISODateToLocalizedDate(const std::string& strIsoDate)
{
  // Convert ISO8601 date strings YYYY, YYYY-MM, or YYYY-MM-DD to (partial) localized date strings
  CDateTime date;
  std::string formattedDate = strIsoDate;
  if (formattedDate.size() == 10)
  {
    date.SetFromDBDate(strIsoDate);
    formattedDate = date.GetAsLocalizedDate();
  }
  else if (formattedDate.size() == 7)
  {
    std::string strFormat = date.GetAsLocalizedDate(false);
    std::string tempdate;
    // find which date separator we are using.  Can be -./
    size_t pos = strFormat.find_first_of("-./");
    if (pos != std::string::npos)
    {
      bool yearFirst = strFormat.find("1601") == 0; // true if year comes first
      std::string sep = strFormat.substr(pos, 1);
      if (yearFirst)
      { // build formatted date with year first, then separator and month
        tempdate = formattedDate.substr(0, 4);
        tempdate += sep;
        tempdate += formattedDate.substr(5, 2);
      }
      else
      {
        tempdate = formattedDate.substr(5, 2);
        tempdate += sep;
        tempdate += formattedDate.substr(0, 4);
      }
      formattedDate = tempdate;
    }
    // return either just the year or the locally formatted version of the ISO date
  }
  return formattedDate;
}

long StringUtils::TimeStringToSeconds(const std::string& timeString)
{
  std::string strCopy(timeString);
  StringUtils::Trim(strCopy);
  if (StringUtils::EndsWithNoCase(strCopy, " min"))
  {
    // this is imdb format of "XXX min"
    return 60 * atoi(strCopy.c_str());
  }
  else
  {
    std::vector<std::string> secs = StringUtils::Split(strCopy, ':');
    int timeInSecs = 0;
    for (unsigned int i = 0; i < 3 && i < secs.size(); i++)
    {
      timeInSecs *= 60;
      timeInSecs += atoi(secs[i].c_str());
    }
    return timeInSecs;
  }
}

std::string StringUtils::SecondsToTimeString(long lSeconds, TIME_FORMAT format)
{
  bool isNegative = lSeconds < 0;
  lSeconds = std::abs(lSeconds);

  std::string strHMS;
  if (format == TIME_FORMAT_SECS)
    strHMS = std::to_string(lSeconds);
  else if (format == TIME_FORMAT_MINS)
    strHMS = std::to_string(lrintf(static_cast<float>(lSeconds) / 60.0f));
  else if (format == TIME_FORMAT_HOURS)
    strHMS = std::to_string(lrintf(static_cast<float>(lSeconds) / 3600.0f));
  else if (format & TIME_FORMAT_M)
    strHMS += std::to_string(lSeconds % 3600 / 60);
  else
  {
    int hh = lSeconds / 3600;
    lSeconds = lSeconds % 3600;
    int mm = lSeconds / 60;
    int ss = lSeconds % 60;

    if (format == TIME_FORMAT_GUESS)
      format = (hh >= 1) ? TIME_FORMAT_HH_MM_SS : TIME_FORMAT_MM_SS;
    if (format & TIME_FORMAT_HH)
      strHMS += StringUtils::Format("{:02}", hh);
    else if (format & TIME_FORMAT_H)
      strHMS += std::to_string(hh);
    if (format & TIME_FORMAT_MM)
      strHMS += StringUtils::Format(strHMS.empty() ? "{:02}" : ":{:02}", mm);
    if (format & TIME_FORMAT_SS)
      strHMS += StringUtils::Format(strHMS.empty() ? "{:02}" : ":{:02}", ss);
  }

  if (isNegative)
    strHMS = "-" + strHMS;

  return strHMS;
}

bool StringUtils::IsNaturalNumber(const std::string& str)
{
  size_t i = 0;
  size_t n = 0;
  // allow whitespace,digits,whitespace
  while (i < str.size() && isspace((unsigned char)str[i]))
    i++;
  while (i < str.size() && isdigit((unsigned char)str[i]))
  {
    i++;
    n++;
  }
  while (i < str.size() && isspace((unsigned char)str[i]))
    i++;
  return i == str.size() && n > 0;
}

bool StringUtils::IsInteger(const std::string& str)
{
  size_t i = 0;
  size_t n = 0;
  // allow whitespace,-,digits,whitespace
  while (i < str.size() && isspace((unsigned char)str[i]))
    i++;
  if (i < str.size() && str[i] == '-')
    i++;
  while (i < str.size() && isdigit((unsigned char)str[i]))
  {
    i++;
    n++;
  }
  while (i < str.size() && isspace((unsigned char)str[i]))
    i++;
  return i == str.size() && n > 0;
}

int StringUtils::asciidigitvalue(char chr)
{
  if (!isasciidigit(chr))
    return -1;

  return chr - '0';
}

int StringUtils::asciixdigitvalue(char chr)
{
  int v = asciidigitvalue(chr);
  if (v >= 0)
    return v;
  if (chr >= 'a' && chr <= 'f')
    return chr - 'a' + 10;
  if (chr >= 'A' && chr <= 'F')
    return chr - 'A' + 10;

  return -1;
}

void StringUtils::RemoveCRLF(std::string& strLine)
{
  StringUtils::TrimRight(strLine, "\n\r");
}

std::string StringUtils::SizeToString(int64_t size)
{
  std::string strLabel;
  constexpr std::array<char, 9> prefixes = {' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};
  unsigned int i = 0;
  double s = (double)size;
  while (i < prefixes.size() && s >= 1000.0)
  {
    s /= 1024.0;
    i++;
  }

  if (!i)
    strLabel = StringUtils::Format("{:.2f} B", s);
  else if (i == prefixes.size())
  {
    if (s >= 1000.0)
      strLabel = StringUtils::Format(">999.99 {}B", prefixes[i - 1]);
    else
      strLabel = StringUtils::Format("{:.2f} {}B", s, prefixes[i - 1]);
  }
  else if (s >= 100.0)
    strLabel = StringUtils::Format("{:.1f} {}B", s, prefixes[i]);
  else
    strLabel = StringUtils::Format("{:.2f} {}B", s, prefixes[i]);

  return strLabel;
}

std::string StringUtils::BinaryStringToString(const std::string& in)
{
  std::string out;
  out.reserve(in.size() / 2);
  for (const char *cur = in.c_str(), *end = cur + in.size(); cur != end; ++cur)
  {
    if (*cur == '\\')
    {
      ++cur;
      if (cur == end)
      {
        break;
      }
      if (isdigit(*cur))
      {
        char* end;
        unsigned long num = strtol(cur, &end, 10);
        cur = end - 1;
        out.push_back(num);
        continue;
      }
    }
    out.push_back(*cur);
  }
  return out;
}

std::string StringUtils::ToHexadecimal(const std::string& in)
{
  std::ostringstream ss;
  ss << std::hex;
  for (unsigned char ch : in)
  {
    ss << std::setw(2) << std::setfill('0') << static_cast<unsigned long>(ch);
  }
  return ss.str();
}

std::string StringUtils::ToHex(std::string_view in)
{
  int width = sizeof(char) * 2;
  std::string gap;
  std::ostringstream ss;
  ss << std::noshowbase; // manually show 0x (due to 0 omitting it)
  ss << std::internal;
  ss << std::setfill('0');
  for (char ch : in)
  {
    ss << gap << "0x" << std::setw(width) << std::hex
       << static_cast<int>(static_cast<unsigned char>(ch));
    gap = " "s;
  }
  return ss.str();
}

std::string StringUtils::ToHex(std::wstring_view in)
{
  int width = sizeof(wchar_t) * 2;
  std::string gap;
  std::ostringstream ss;
  ss << std::noshowbase; // manually show 0x (due to 0 omitting it)
  ss << std::internal;
  ss << std::setfill('0');
  for (wchar_t ch : in)
  {
    ss << gap << std::setw(width) << std::hex << static_cast<unsigned long>(ch);
    gap = " "s;
  }
  return ss.str();
}

std::string StringUtils::ToHex(std::u32string_view in)
{
  int width = sizeof(char32_t) * 2;
  std::string gap;
  std::ostringstream ss;
  ss << std::noshowbase; // manually show 0x (due to 0 omitting it)
  ss << std::internal;
  ss << std::setfill('0');
  for (char32_t ch : in)
  {
    ss << gap << std::setw(width) << std::hex << ch;
    gap = " "s;
  }
  return ss.str();
}
namespace
{
// return -1 if not, else return the utf8 char length.
static int IsUTF8Letter(const unsigned char* str)
{
  // reference:
  // unicode -> utf8 table: http://www.utf8-chartable.de/
  // latin characters in unicode: http://en.wikipedia.org/wiki/Latin_characters_in_Unicode
  unsigned char ch = str[0];
  if (!ch)
    return -1;
  if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
    return 1;
  if (!(ch & 0x80))
    return -1;
  unsigned char ch2 = str[1];
  if (!ch2)
    return -1;
  // check latin 1 letter table: http://en.wikipedia.org/wiki/C1_Controls_and_Latin-1_Supplement
  if (ch == 0xC3 && ch2 >= 0x80 && ch2 <= 0xBF && ch2 != 0x97 && ch2 != 0xB7)
    return 2;
  // check latin extended A table: http://en.wikipedia.org/wiki/Latin_Extended-A
  if (ch >= 0xC4 && ch <= 0xC7 && ch2 >= 0x80 && ch2 <= 0xBF)
    return 2;
  // check latin extended B table: http://en.wikipedia.org/wiki/Latin_Extended-B
  // and International Phonetic Alphabet: http://en.wikipedia.org/wiki/IPA_Extensions_(Unicode_block)
  if (((ch == 0xC8 || ch == 0xC9) && ch2 >= 0x80 && ch2 <= 0xBF) ||
      (ch == 0xCA && ch2 >= 0x80 && ch2 <= 0xAF))
    return 2;
  return -1;
}
} // namespace

size_t StringUtils::FindWords(const char* str, const char* wordLowerCase)
{
  // NOTE: This assumes word is lowercase!
  const unsigned char* s = (const unsigned char*)str;
  do
  {
    // start with a compare
    const unsigned char* c = s;
    const unsigned char* w = (const unsigned char*)wordLowerCase;
    bool same = true;
    while (same && *c && *w)
    {
      unsigned char lc = *c++;
      if (lc >= 'A' && lc <= 'Z')
        lc += 'a' - 'A';

      if (lc != *w++) // different
        same = false;
    }
    if (same && *w == 0) // only the same if word has been exhausted
      return (const char*)s - str;

    // otherwise, skip current word (composed by latin letters) or number
    int l;
    if (*s >= '0' && *s <= '9')
    {
      ++s;
      while (*s >= '0' && *s <= '9')
        ++s;
    }
    else if ((l = IsUTF8Letter(s)) > 0)
    {
      s += l;
      while ((l = IsUTF8Letter(s)) > 0)
        s += l;
    }
    else
      ++s;
    while (*s && *s == ' ')
      s++;

    // and repeat until we're done
  } while (*s);

  return std::string::npos;
}

// assumes it is called from after the first open bracket is found
int StringUtils::FindEndBracket(const std::string& str, char opener, char closer, int startPos)
{
  int blocks = 1;
  for (unsigned int i = startPos; i < str.size(); i++)
  {
    if (str[i] == opener)
      blocks++;
    else if (str[i] == closer)
    {
      blocks--;
      if (!blocks)
        return i;
    }
  }

  return (int)std::string::npos;
}

void StringUtils::WordToDigits(std::string& word)
{
  static const char word_to_letter[] = "22233344455566677778889999";
  std::string digits = StringUtils::FoldCase(word);
  for (unsigned int i = 0; i < digits.size(); ++i)
  { // NB: This assumes ascii, which probably needs extending at some  point.
    uint8_t letter = static_cast<uint8_t>(digits[i]);
    if (letter > 0x7f)
    {
      // Note that error can occur prior to logger being ready which can cause message
      // to be lost or crash Kodi.

      CLog::LogF(LOGWARNING, "Non-ASCII input: {}", word);
    }

    if ((letter >= 'a' && letter <= 'z')) // assume contiguous letter range
    {
      digits[i] = word_to_letter[letter - 'a'];
    }
    else if (letter < '0' || letter > '9') // We want to keep 0-9!
    {
      digits[i] = ' '; // replace everything else with a space
    }
  }
  digits.swap(word);
}

std::string StringUtils::CreateUUID()
{
#ifdef HAVE_NEW_CROSSGUID
#ifdef TARGET_ANDROID
  JNIEnv* env = xbmc_jnienv();
  return xg::newGuid(env).str();
#else
  return xg::newGuid().str();
#endif /* TARGET_ANDROID */
#else
  static GuidGenerator guidGenerator;
  auto guid = guidGenerator.newGuid();

  std::stringstream strGuid;
  strGuid << guid;
  return strGuid.str();
#endif
}

bool StringUtils::ValidateUUID(const std::string& uuid)
{
  CRegExp guidRE;
  guidRE.RegComp(ADDON_GUID_RE);
  return (guidRE.RegFind(uuid.c_str()) == 0);
}

double StringUtils::CompareFuzzy(const std::string& left, const std::string& right)
{
  return (0.5 + fstrcmp(left.c_str(), right.c_str()) * (left.length() + right.length())) / 2.0;
}

int StringUtils::FindBestMatch(const std::string& str,
                               const std::vector<std::string>& strings,
                               double& matchscore)
{
  int best = -1;
  matchscore = 0;

  int i = 0;
  for (std::vector<std::string>::const_iterator it = strings.begin(); it != strings.end();
       ++it, i++)
  {
    int maxlength = std::max(str.length(), it->length());
    double score = StringUtils::CompareFuzzy(str, *it) / maxlength;
    if (score > matchscore)
    {
      matchscore = score;
      best = i;
    }
  }
  return best;
}

bool StringUtils::ContainsKeyword(const std::string& str, const std::vector<std::string>& keywords)
{
  for (std::vector<std::string>::const_iterator it = keywords.begin(); it != keywords.end(); ++it)
  {
    if (str.find(*it) != str.npos)
      return true;
  }
  return false;
}

size_t StringUtils::utf8_strlen(const char* s)
{
  size_t length = 0;
  while (*s)
  {
    if ((*s++ & 0xC0) != 0x80)
      length++;
  }
  return length;
}

std::string StringUtils::Paramify(const std::string& param)
{
  std::string result = param;
  // escape backspaces
  StringUtils::Replace(result, "\\", "\\\\");
  // escape double quotes
  StringUtils::Replace(result, "\"", "\\\"");

  // add double quotes around the whole string
  return "\"" + result + "\"";
}

std::string StringUtils::DeParamify(const std::string& param)
{
  std::string result = param;

  // remove double quotes around the whole string
  if (StringUtils::StartsWith(result, "\"") && StringUtils::EndsWith(result, "\""))
  {
    result.erase(0, 1);
    result.pop_back();

    // unescape double quotes
    StringUtils::Replace(result, "\\\"", "\"");

    // unescape backspaces
    StringUtils::Replace(result, "\\\\", "\\");
  }

  return result;
}

std::vector<std::string> StringUtils::Tokenize(const std::string& input,
                                               const std::string& delimiters)
{
  std::vector<std::string> tokens;
  Tokenize(input, tokens, delimiters);
  return tokens;
}

void StringUtils::Tokenize(const std::string& input,
                           std::vector<std::string>& tokens,
                           const std::string& delimiters)
{
  tokens.clear();
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiters);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find_first_of(delimiters, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiters, nextDelimPos);
  }
}

std::vector<std::string> StringUtils::Tokenize(const std::string& input, const char delimiter)
{
  std::vector<std::string> tokens;
  Tokenize(input, tokens, delimiter);
  return tokens;
}

void StringUtils::Tokenize(const std::string& input,
                           std::vector<std::string>& tokens,
                           const char delimiter)
{
  tokens.clear();
  // Skip delimiters at beginning.
  std::string::size_type dataPos = input.find_first_not_of(delimiter);
  while (dataPos != std::string::npos)
  {
    // Find next delimiter
    const std::string::size_type nextDelimPos = input.find(delimiter, dataPos);
    // Found a token, add it to the vector.
    tokens.push_back(input.substr(dataPos, nextDelimPos - dataPos));
    // Skip delimiters.  Note the "not_of"
    dataPos = input.find_first_not_of(delimiter, nextDelimPos);
  }
}

uint32_t StringUtils::ToUint32(std::string_view str, uint32_t fallback /* = 0 */) noexcept
{
  return NumberFromSS(str, fallback);
}

uint64_t StringUtils::ToUint64(std::string_view str, uint64_t fallback /* = 0 */) noexcept
{
  return NumberFromSS(str, fallback);
}

float StringUtils::ToFloat(std::string_view str, float fallback /* = 0.0f */) noexcept
{
  return NumberFromSS(str, fallback);
}

std::string StringUtils::FormatFileSize(uint64_t bytes)
{
  const std::array<std::string, 6> units{{"B", "kB", "MB", "GB", "TB", "PB"}};
  if (bytes < 1000)
    return Format("{}B", bytes);

  size_t i = 0;
  double value = static_cast<double>(bytes);
  while (i + 1 < units.size() && value >= 999.5)
  {
    ++i;
    value /= 1024.0;
  }
  unsigned int decimals = value < 9.995 ? 2 : (value < 99.95 ? 1 : 0);
  return Format("{:.{}f}{}", value, decimals, units[i]);
}

const std::locale& StringUtils::GetOriginalLocale() noexcept
{
  return g_langInfo.GetOriginalLocale();
}

const std::locale& StringUtils::GetSystemLocale() noexcept
{
  return g_langInfo.GetSystemLocale();
}

std::string StringUtils::CreateFromCString(const char* cstr)
{
  return cstr != nullptr ? std::string(cstr) : std::string();
}

namespace
{
/*
 *                               C A S E   C H A N G E   T A B L E S
 *
 * What follows are the large tables for case changing: FOLD, LOWER & UPPER as well
 * as the code which does the character conversion using the tables. The code is at
 * the end.
 *
 *
 * Case folding tables are derived from Unicode Inc.
 * Data file, CaseFolding.txt (CaseFolding-14.0.0.txt, 2021-03-08). Copyright follows below.
 *
 * These tables provide for "simple case folding" that is not locale sensitive. They do NOT
 * support "Full Case Folding" which can fold single characters into multiple, or multiple
 * into single, etc. CaseFolding.txt can be found in the ICUC4 source directory:
 * icu/source/data/unidata.
 *
 * The home-grown tool to produce the case folding tables can be found in
 * xbmc/utils/unicode_tools. They are built and run by hand with the table data
 * pasted and formatted here. This only needs to be done if an error or an update
 * to CaseFolding.txt occurs.
 *
 * Terms of use:
 *
 * UNICODE, INC. LICENSE AGREEMENT - DATA FILES AND SOFTWARE
 *
 * See Terms of Use <https://www.unicode.org/copyright.html>
 * for definitions of Unicode Inc.’s Data Files and Software.
 *
 * NOTICE TO USER: Carefully read the following legal agreement.
 * BY DOWNLOADING, INSTALLING, COPYING OR OTHERWISE USING UNICODE INC.'S
 * DATA FILES ("DATA FILES"), AND/OR SOFTWARE ("SOFTWARE"),
 * YOU UNEQUIVOCALLY ACCEPT, AND AGREE TO BE BOUND BY, ALL OF THE
 * TERMS AND CONDITIONS OF THIS AGREEMENT.
 * IF YOU DO NOT AGREE, DO NOT DOWNLOAD, INSTALL, COPY, DISTRIBUTE OR USE
 * THE DATA FILES OR SOFTWARE.
 *
 * COPYRIGHT AND PERMISSION NOTICE
 *
 * Copyright © 1991-2022 Unicode, Inc. All rights reserved.
 * Distributed under the Terms of Use in https://www.unicode.org/copyright.html.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of the Unicode data files and any associated documentation
 * (the "Data Files") or Unicode software and any associated documentation
 * (the "Software") to deal in the Data Files or Software
 * without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, and/or sell copies of
 * the Data Files or Software, and to permit persons to whom the Data Files
 * or Software are furnished to do so, provided that either
 * (a) this copyright and permission notice appear with all copies
 * of the Data Files or Software, or
 * (b) this copyright and permission notice appear in associated
 * Documentation.
 *
 * THE DATA FILES AND SOFTWARE ARE PROVIDED "AS IS", WITHOUT WARRANTY OF
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT OF THIRD PARTY RIGHTS.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS
 * NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL
 * DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THE DATA FILES OR SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in these Data Files or Software without prior
 * written authorization of the copyright holder.
 */

// The tables below are logically indexed by the 32-bit Unicode value of the
// character which is to be case folded. The value found in the table is
// the case folded value, or 0 if no such value exists.
//
// FOLDCASE, TO_UPPER_CASE & TO_LOWER_CASE all use the same methodology.
//
// A char32_t contains a 32-bit Unicode codepoint, although only 24 bits is
// used. The array FOLDCASE_INDEX is indexed by the upper 16-bits of the
// the 24-bit codepoint, yielding a pointer to another table indexed by
// the lower 8-bits of the codepoint (FOLDCASE_0x0001, etc.) to get the lower-case equivalent
// for the original codepoint (see FoldCaseChar, below).
//
// Specifically, FOLDCASE_0x...[0] contains the number of elements in the
// array. This helps reduce the size of the table. All other non-zero elements
// contain the upper-case Unicode value for a fold-case codepoint. This
// means that "A", 0x041, the FoldCase value can be found by:
//
//   high_bytes = 0x41 >> 8; => 0
//   char32_t* table = FOLDCASE_INDEX[high_bytes]; => address of FOLDCASE_0000
//   uint16_t low_byte = c & 0xFF; => 0x41
//   char32_t foldedChar = table[low_byte + 1]; => 0x61 'a'
//
// clang-format off

static constexpr const char32_t FOLDCASE_0x00000[]
{
 U'\x000df',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00061',  U'\x00062',  U'\x00063',  U'\x00064',  U'\x00065',
 U'\x00066',  U'\x00067',  U'\x00068',  U'\x00069',  U'\x0006a',  U'\x0006b',  U'\x0006c',
 U'\x0006d',  U'\x0006e',  U'\x0006f',  U'\x00070',  U'\x00071',  U'\x00072',  U'\x00073',
 U'\x00074',  U'\x00075',  U'\x00076',  U'\x00077',  U'\x00078',  U'\x00079',  U'\x0007a',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003bc',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x000e0',  U'\x000e1',  U'\x000e2',  U'\x000e3',
 U'\x000e4',  U'\x000e5',  U'\x000e6',  U'\x000e7',  U'\x000e8',  U'\x000e9',  U'\x000ea',
 U'\x000eb',  U'\x000ec',  U'\x000ed',  U'\x000ee',  U'\x000ef',  U'\x000f0',  U'\x000f1',
 U'\x000f2',  U'\x000f3',  U'\x000f4',  U'\x000f5',  U'\x000f6',  U'\x00000',  U'\x000f8',
 U'\x000f9',  U'\x000fa',  U'\x000fb',  U'\x000fc',  U'\x000fd',  U'\x000fe'
};

static constexpr const char32_t FOLDCASE_0x00001[]
{
 U'\x000ff',
 U'\x00101',  U'\x00000',  U'\x00103',  U'\x00000',  U'\x00105',  U'\x00000',  U'\x00107',
 U'\x00000',  U'\x00109',  U'\x00000',  U'\x0010b',  U'\x00000',  U'\x0010d',  U'\x00000',
 U'\x0010f',  U'\x00000',  U'\x00111',  U'\x00000',  U'\x00113',  U'\x00000',  U'\x00115',
 U'\x00000',  U'\x00117',  U'\x00000',  U'\x00119',  U'\x00000',  U'\x0011b',  U'\x00000',
 U'\x0011d',  U'\x00000',  U'\x0011f',  U'\x00000',  U'\x00121',  U'\x00000',  U'\x00123',
 U'\x00000',  U'\x00125',  U'\x00000',  U'\x00127',  U'\x00000',  U'\x00129',  U'\x00000',
 U'\x0012b',  U'\x00000',  U'\x0012d',  U'\x00000',  U'\x0012f',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00133',  U'\x00000',  U'\x00135',  U'\x00000',  U'\x00137',  U'\x00000',
 U'\x00000',  U'\x0013a',  U'\x00000',  U'\x0013c',  U'\x00000',  U'\x0013e',  U'\x00000',
 U'\x00140',  U'\x00000',  U'\x00142',  U'\x00000',  U'\x00144',  U'\x00000',  U'\x00146',
 U'\x00000',  U'\x00148',  U'\x00000',  U'\x00000',  U'\x0014b',  U'\x00000',  U'\x0014d',
 U'\x00000',  U'\x0014f',  U'\x00000',  U'\x00151',  U'\x00000',  U'\x00153',  U'\x00000',
 U'\x00155',  U'\x00000',  U'\x00157',  U'\x00000',  U'\x00159',  U'\x00000',  U'\x0015b',
 U'\x00000',  U'\x0015d',  U'\x00000',  U'\x0015f',  U'\x00000',  U'\x00161',  U'\x00000',
 U'\x00163',  U'\x00000',  U'\x00165',  U'\x00000',  U'\x00167',  U'\x00000',  U'\x00169',
 U'\x00000',  U'\x0016b',  U'\x00000',  U'\x0016d',  U'\x00000',  U'\x0016f',  U'\x00000',
 U'\x00171',  U'\x00000',  U'\x00173',  U'\x00000',  U'\x00175',  U'\x00000',  U'\x00177',
 U'\x00000',  U'\x000ff',  U'\x0017a',  U'\x00000',  U'\x0017c',  U'\x00000',  U'\x0017e',
 U'\x00000',  U'\x00073',  U'\x00000',  U'\x00253',  U'\x00183',  U'\x00000',  U'\x00185',
 U'\x00000',  U'\x00254',  U'\x00188',  U'\x00000',  U'\x00256',  U'\x00257',  U'\x0018c',
 U'\x00000',  U'\x00000',  U'\x001dd',  U'\x00259',  U'\x0025b',  U'\x00192',  U'\x00000',
 U'\x00260',  U'\x00263',  U'\x00000',  U'\x00269',  U'\x00268',  U'\x00199',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x0026f',  U'\x00272',  U'\x00000',  U'\x00275',  U'\x001a1',
 U'\x00000',  U'\x001a3',  U'\x00000',  U'\x001a5',  U'\x00000',  U'\x00280',  U'\x001a8',
 U'\x00000',  U'\x00283',  U'\x00000',  U'\x00000',  U'\x001ad',  U'\x00000',  U'\x00288',
 U'\x001b0',  U'\x00000',  U'\x0028a',  U'\x0028b',  U'\x001b4',  U'\x00000',  U'\x001b6',
 U'\x00000',  U'\x00292',  U'\x001b9',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x001bd',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x001c6',  U'\x001c6',  U'\x00000',  U'\x001c9',  U'\x001c9',  U'\x00000',  U'\x001cc',
 U'\x001cc',  U'\x00000',  U'\x001ce',  U'\x00000',  U'\x001d0',  U'\x00000',  U'\x001d2',
 U'\x00000',  U'\x001d4',  U'\x00000',  U'\x001d6',  U'\x00000',  U'\x001d8',  U'\x00000',
 U'\x001da',  U'\x00000',  U'\x001dc',  U'\x00000',  U'\x00000',  U'\x001df',  U'\x00000',
 U'\x001e1',  U'\x00000',  U'\x001e3',  U'\x00000',  U'\x001e5',  U'\x00000',  U'\x001e7',
 U'\x00000',  U'\x001e9',  U'\x00000',  U'\x001eb',  U'\x00000',  U'\x001ed',  U'\x00000',
 U'\x001ef',  U'\x00000',  U'\x00000',  U'\x001f3',  U'\x001f3',  U'\x00000',  U'\x001f5',
 U'\x00000',  U'\x00195',  U'\x001bf',  U'\x001f9',  U'\x00000',  U'\x001fb',  U'\x00000',
 U'\x001fd',  U'\x00000',  U'\x001ff'
};

static constexpr const char32_t FOLDCASE_0x00002[]
{
 U'\x0004f',
 U'\x00201',  U'\x00000',  U'\x00203',  U'\x00000',  U'\x00205',  U'\x00000',  U'\x00207',
 U'\x00000',  U'\x00209',  U'\x00000',  U'\x0020b',  U'\x00000',  U'\x0020d',  U'\x00000',
 U'\x0020f',  U'\x00000',  U'\x00211',  U'\x00000',  U'\x00213',  U'\x00000',  U'\x00215',
 U'\x00000',  U'\x00217',  U'\x00000',  U'\x00219',  U'\x00000',  U'\x0021b',  U'\x00000',
 U'\x0021d',  U'\x00000',  U'\x0021f',  U'\x00000',  U'\x0019e',  U'\x00000',  U'\x00223',
 U'\x00000',  U'\x00225',  U'\x00000',  U'\x00227',  U'\x00000',  U'\x00229',  U'\x00000',
 U'\x0022b',  U'\x00000',  U'\x0022d',  U'\x00000',  U'\x0022f',  U'\x00000',  U'\x00231',
 U'\x00000',  U'\x00233',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x02c65',  U'\x0023c',  U'\x00000',  U'\x0019a',  U'\x02c66',
 U'\x00000',  U'\x00000',  U'\x00242',  U'\x00000',  U'\x00180',  U'\x00289',  U'\x0028c',
 U'\x00247',  U'\x00000',  U'\x00249',  U'\x00000',  U'\x0024b',  U'\x00000',  U'\x0024d',
 U'\x00000',  U'\x0024f'
};

static constexpr const char32_t FOLDCASE_0x00003[]
{
 U'\x00100',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003b9',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00371',  U'\x00000',  U'\x00373',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00377',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x003f3',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x003ac',  U'\x00000',  U'\x003ad',  U'\x003ae',  U'\x003af',  U'\x00000',
 U'\x003cc',  U'\x00000',  U'\x003cd',  U'\x003ce',  U'\x00000',  U'\x003b1',  U'\x003b2',
 U'\x003b3',  U'\x003b4',  U'\x003b5',  U'\x003b6',  U'\x003b7',  U'\x003b8',  U'\x003b9',
 U'\x003ba',  U'\x003bb',  U'\x003bc',  U'\x003bd',  U'\x003be',  U'\x003bf',  U'\x003c0',
 U'\x003c1',  U'\x00000',  U'\x003c3',  U'\x003c4',  U'\x003c5',  U'\x003c6',  U'\x003c7',
 U'\x003c8',  U'\x003c9',  U'\x003ca',  U'\x003cb',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003c3',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003d7',  U'\x003b2',  U'\x003b8',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x003c6',  U'\x003c0',  U'\x00000',  U'\x003d9',
 U'\x00000',  U'\x003db',  U'\x00000',  U'\x003dd',  U'\x00000',  U'\x003df',  U'\x00000',
 U'\x003e1',  U'\x00000',  U'\x003e3',  U'\x00000',  U'\x003e5',  U'\x00000',  U'\x003e7',
 U'\x00000',  U'\x003e9',  U'\x00000',  U'\x003eb',  U'\x00000',  U'\x003ed',  U'\x00000',
 U'\x003ef',  U'\x00000',  U'\x003ba',  U'\x003c1',  U'\x00000',  U'\x00000',  U'\x003b8',
 U'\x003b5',  U'\x00000',  U'\x003f8',  U'\x00000',  U'\x003f2',  U'\x003fb',  U'\x00000',
 U'\x00000',  U'\x0037b',  U'\x0037c',  U'\x0037d'
};

static constexpr const char32_t FOLDCASE_0x00004[]
{
 U'\x000ff',
 U'\x00450',  U'\x00451',  U'\x00452',  U'\x00453',  U'\x00454',  U'\x00455',  U'\x00456',
 U'\x00457',  U'\x00458',  U'\x00459',  U'\x0045a',  U'\x0045b',  U'\x0045c',  U'\x0045d',
 U'\x0045e',  U'\x0045f',  U'\x00430',  U'\x00431',  U'\x00432',  U'\x00433',  U'\x00434',
 U'\x00435',  U'\x00436',  U'\x00437',  U'\x00438',  U'\x00439',  U'\x0043a',  U'\x0043b',
 U'\x0043c',  U'\x0043d',  U'\x0043e',  U'\x0043f',  U'\x00440',  U'\x00441',  U'\x00442',
 U'\x00443',  U'\x00444',  U'\x00445',  U'\x00446',  U'\x00447',  U'\x00448',  U'\x00449',
 U'\x0044a',  U'\x0044b',  U'\x0044c',  U'\x0044d',  U'\x0044e',  U'\x0044f',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00461',  U'\x00000',
 U'\x00463',  U'\x00000',  U'\x00465',  U'\x00000',  U'\x00467',  U'\x00000',  U'\x00469',
 U'\x00000',  U'\x0046b',  U'\x00000',  U'\x0046d',  U'\x00000',  U'\x0046f',  U'\x00000',
 U'\x00471',  U'\x00000',  U'\x00473',  U'\x00000',  U'\x00475',  U'\x00000',  U'\x00477',
 U'\x00000',  U'\x00479',  U'\x00000',  U'\x0047b',  U'\x00000',  U'\x0047d',  U'\x00000',
 U'\x0047f',  U'\x00000',  U'\x00481',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0048b',  U'\x00000',
 U'\x0048d',  U'\x00000',  U'\x0048f',  U'\x00000',  U'\x00491',  U'\x00000',  U'\x00493',
 U'\x00000',  U'\x00495',  U'\x00000',  U'\x00497',  U'\x00000',  U'\x00499',  U'\x00000',
 U'\x0049b',  U'\x00000',  U'\x0049d',  U'\x00000',  U'\x0049f',  U'\x00000',  U'\x004a1',
 U'\x00000',  U'\x004a3',  U'\x00000',  U'\x004a5',  U'\x00000',  U'\x004a7',  U'\x00000',
 U'\x004a9',  U'\x00000',  U'\x004ab',  U'\x00000',  U'\x004ad',  U'\x00000',  U'\x004af',
 U'\x00000',  U'\x004b1',  U'\x00000',  U'\x004b3',  U'\x00000',  U'\x004b5',  U'\x00000',
 U'\x004b7',  U'\x00000',  U'\x004b9',  U'\x00000',  U'\x004bb',  U'\x00000',  U'\x004bd',
 U'\x00000',  U'\x004bf',  U'\x00000',  U'\x004cf',  U'\x004c2',  U'\x00000',  U'\x004c4',
 U'\x00000',  U'\x004c6',  U'\x00000',  U'\x004c8',  U'\x00000',  U'\x004ca',  U'\x00000',
 U'\x004cc',  U'\x00000',  U'\x004ce',  U'\x00000',  U'\x00000',  U'\x004d1',  U'\x00000',
 U'\x004d3',  U'\x00000',  U'\x004d5',  U'\x00000',  U'\x004d7',  U'\x00000',  U'\x004d9',
 U'\x00000',  U'\x004db',  U'\x00000',  U'\x004dd',  U'\x00000',  U'\x004df',  U'\x00000',
 U'\x004e1',  U'\x00000',  U'\x004e3',  U'\x00000',  U'\x004e5',  U'\x00000',  U'\x004e7',
 U'\x00000',  U'\x004e9',  U'\x00000',  U'\x004eb',  U'\x00000',  U'\x004ed',  U'\x00000',
 U'\x004ef',  U'\x00000',  U'\x004f1',  U'\x00000',  U'\x004f3',  U'\x00000',  U'\x004f5',
 U'\x00000',  U'\x004f7',  U'\x00000',  U'\x004f9',  U'\x00000',  U'\x004fb',  U'\x00000',
 U'\x004fd',  U'\x00000',  U'\x004ff'
};

static constexpr const char32_t FOLDCASE_0x00005[]
{
 U'\x00057',
 U'\x00501',  U'\x00000',  U'\x00503',  U'\x00000',  U'\x00505',  U'\x00000',  U'\x00507',
 U'\x00000',  U'\x00509',  U'\x00000',  U'\x0050b',  U'\x00000',  U'\x0050d',  U'\x00000',
 U'\x0050f',  U'\x00000',  U'\x00511',  U'\x00000',  U'\x00513',  U'\x00000',  U'\x00515',
 U'\x00000',  U'\x00517',  U'\x00000',  U'\x00519',  U'\x00000',  U'\x0051b',  U'\x00000',
 U'\x0051d',  U'\x00000',  U'\x0051f',  U'\x00000',  U'\x00521',  U'\x00000',  U'\x00523',
 U'\x00000',  U'\x00525',  U'\x00000',  U'\x00527',  U'\x00000',  U'\x00529',  U'\x00000',
 U'\x0052b',  U'\x00000',  U'\x0052d',  U'\x00000',  U'\x0052f',  U'\x00000',  U'\x00000',
 U'\x00561',  U'\x00562',  U'\x00563',  U'\x00564',  U'\x00565',  U'\x00566',  U'\x00567',
 U'\x00568',  U'\x00569',  U'\x0056a',  U'\x0056b',  U'\x0056c',  U'\x0056d',  U'\x0056e',
 U'\x0056f',  U'\x00570',  U'\x00571',  U'\x00572',  U'\x00573',  U'\x00574',  U'\x00575',
 U'\x00576',  U'\x00577',  U'\x00578',  U'\x00579',  U'\x0057a',  U'\x0057b',  U'\x0057c',
 U'\x0057d',  U'\x0057e',  U'\x0057f',  U'\x00580',  U'\x00581',  U'\x00582',  U'\x00583',
 U'\x00584',  U'\x00585',  U'\x00586'
};

static constexpr const char32_t FOLDCASE_0x00010[]
{
 U'\x000ce',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02d00',
 U'\x02d01',  U'\x02d02',  U'\x02d03',  U'\x02d04',  U'\x02d05',  U'\x02d06',  U'\x02d07',
 U'\x02d08',  U'\x02d09',  U'\x02d0a',  U'\x02d0b',  U'\x02d0c',  U'\x02d0d',  U'\x02d0e',
 U'\x02d0f',  U'\x02d10',  U'\x02d11',  U'\x02d12',  U'\x02d13',  U'\x02d14',  U'\x02d15',
 U'\x02d16',  U'\x02d17',  U'\x02d18',  U'\x02d19',  U'\x02d1a',  U'\x02d1b',  U'\x02d1c',
 U'\x02d1d',  U'\x02d1e',  U'\x02d1f',  U'\x02d20',  U'\x02d21',  U'\x02d22',  U'\x02d23',
 U'\x02d24',  U'\x02d25',  U'\x00000',  U'\x02d27',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x02d2d'
};

static constexpr const char32_t FOLDCASE_0x00013[]
{
 U'\x000fe',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x013f0',  U'\x013f1',  U'\x013f2',  U'\x013f3',
 U'\x013f4',  U'\x013f5'
};

static constexpr const char32_t FOLDCASE_0x0001c[]
{
 U'\x000c0',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00432',  U'\x00434',  U'\x0043e',  U'\x00441',  U'\x00442',
 U'\x00442',  U'\x0044a',  U'\x00463',  U'\x0a64b',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x010d0',  U'\x010d1',  U'\x010d2',
 U'\x010d3',  U'\x010d4',  U'\x010d5',  U'\x010d6',  U'\x010d7',  U'\x010d8',  U'\x010d9',
 U'\x010da',  U'\x010db',  U'\x010dc',  U'\x010dd',  U'\x010de',  U'\x010df',  U'\x010e0',
 U'\x010e1',  U'\x010e2',  U'\x010e3',  U'\x010e4',  U'\x010e5',  U'\x010e6',  U'\x010e7',
 U'\x010e8',  U'\x010e9',  U'\x010ea',  U'\x010eb',  U'\x010ec',  U'\x010ed',  U'\x010ee',
 U'\x010ef',  U'\x010f0',  U'\x010f1',  U'\x010f2',  U'\x010f3',  U'\x010f4',  U'\x010f5',
 U'\x010f6',  U'\x010f7',  U'\x010f8',  U'\x010f9',  U'\x010fa',  U'\x00000',  U'\x00000',
 U'\x010fd',  U'\x010fe',  U'\x010ff'
};

static constexpr const char32_t FOLDCASE_0x0001e[]
{
 U'\x000ff',
 U'\x01e01',  U'\x00000',  U'\x01e03',  U'\x00000',  U'\x01e05',  U'\x00000',  U'\x01e07',
 U'\x00000',  U'\x01e09',  U'\x00000',  U'\x01e0b',  U'\x00000',  U'\x01e0d',  U'\x00000',
 U'\x01e0f',  U'\x00000',  U'\x01e11',  U'\x00000',  U'\x01e13',  U'\x00000',  U'\x01e15',
 U'\x00000',  U'\x01e17',  U'\x00000',  U'\x01e19',  U'\x00000',  U'\x01e1b',  U'\x00000',
 U'\x01e1d',  U'\x00000',  U'\x01e1f',  U'\x00000',  U'\x01e21',  U'\x00000',  U'\x01e23',
 U'\x00000',  U'\x01e25',  U'\x00000',  U'\x01e27',  U'\x00000',  U'\x01e29',  U'\x00000',
 U'\x01e2b',  U'\x00000',  U'\x01e2d',  U'\x00000',  U'\x01e2f',  U'\x00000',  U'\x01e31',
 U'\x00000',  U'\x01e33',  U'\x00000',  U'\x01e35',  U'\x00000',  U'\x01e37',  U'\x00000',
 U'\x01e39',  U'\x00000',  U'\x01e3b',  U'\x00000',  U'\x01e3d',  U'\x00000',  U'\x01e3f',
 U'\x00000',  U'\x01e41',  U'\x00000',  U'\x01e43',  U'\x00000',  U'\x01e45',  U'\x00000',
 U'\x01e47',  U'\x00000',  U'\x01e49',  U'\x00000',  U'\x01e4b',  U'\x00000',  U'\x01e4d',
 U'\x00000',  U'\x01e4f',  U'\x00000',  U'\x01e51',  U'\x00000',  U'\x01e53',  U'\x00000',
 U'\x01e55',  U'\x00000',  U'\x01e57',  U'\x00000',  U'\x01e59',  U'\x00000',  U'\x01e5b',
 U'\x00000',  U'\x01e5d',  U'\x00000',  U'\x01e5f',  U'\x00000',  U'\x01e61',  U'\x00000',
 U'\x01e63',  U'\x00000',  U'\x01e65',  U'\x00000',  U'\x01e67',  U'\x00000',  U'\x01e69',
 U'\x00000',  U'\x01e6b',  U'\x00000',  U'\x01e6d',  U'\x00000',  U'\x01e6f',  U'\x00000',
 U'\x01e71',  U'\x00000',  U'\x01e73',  U'\x00000',  U'\x01e75',  U'\x00000',  U'\x01e77',
 U'\x00000',  U'\x01e79',  U'\x00000',  U'\x01e7b',  U'\x00000',  U'\x01e7d',  U'\x00000',
 U'\x01e7f',  U'\x00000',  U'\x01e81',  U'\x00000',  U'\x01e83',  U'\x00000',  U'\x01e85',
 U'\x00000',  U'\x01e87',  U'\x00000',  U'\x01e89',  U'\x00000',  U'\x01e8b',  U'\x00000',
 U'\x01e8d',  U'\x00000',  U'\x01e8f',  U'\x00000',  U'\x01e91',  U'\x00000',  U'\x01e93',
 U'\x00000',  U'\x01e95',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x01e61',  U'\x00000',  U'\x00000',  U'\x000df',  U'\x00000',  U'\x01ea1',
 U'\x00000',  U'\x01ea3',  U'\x00000',  U'\x01ea5',  U'\x00000',  U'\x01ea7',  U'\x00000',
 U'\x01ea9',  U'\x00000',  U'\x01eab',  U'\x00000',  U'\x01ead',  U'\x00000',  U'\x01eaf',
 U'\x00000',  U'\x01eb1',  U'\x00000',  U'\x01eb3',  U'\x00000',  U'\x01eb5',  U'\x00000',
 U'\x01eb7',  U'\x00000',  U'\x01eb9',  U'\x00000',  U'\x01ebb',  U'\x00000',  U'\x01ebd',
 U'\x00000',  U'\x01ebf',  U'\x00000',  U'\x01ec1',  U'\x00000',  U'\x01ec3',  U'\x00000',
 U'\x01ec5',  U'\x00000',  U'\x01ec7',  U'\x00000',  U'\x01ec9',  U'\x00000',  U'\x01ecb',
 U'\x00000',  U'\x01ecd',  U'\x00000',  U'\x01ecf',  U'\x00000',  U'\x01ed1',  U'\x00000',
 U'\x01ed3',  U'\x00000',  U'\x01ed5',  U'\x00000',  U'\x01ed7',  U'\x00000',  U'\x01ed9',
 U'\x00000',  U'\x01edb',  U'\x00000',  U'\x01edd',  U'\x00000',  U'\x01edf',  U'\x00000',
 U'\x01ee1',  U'\x00000',  U'\x01ee3',  U'\x00000',  U'\x01ee5',  U'\x00000',  U'\x01ee7',
 U'\x00000',  U'\x01ee9',  U'\x00000',  U'\x01eeb',  U'\x00000',  U'\x01eed',  U'\x00000',
 U'\x01eef',  U'\x00000',  U'\x01ef1',  U'\x00000',  U'\x01ef3',  U'\x00000',  U'\x01ef5',
 U'\x00000',  U'\x01ef7',  U'\x00000',  U'\x01ef9',  U'\x00000',  U'\x01efb',  U'\x00000',
 U'\x01efd',  U'\x00000',  U'\x01eff'
};

static constexpr const char32_t FOLDCASE_0x0001f[]
{
 U'\x000fd',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x01f00',  U'\x01f01',  U'\x01f02',  U'\x01f03',  U'\x01f04',  U'\x01f05',
 U'\x01f06',  U'\x01f07',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f10',  U'\x01f11',  U'\x01f12',  U'\x01f13',
 U'\x01f14',  U'\x01f15',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f20',  U'\x01f21',
 U'\x01f22',  U'\x01f23',  U'\x01f24',  U'\x01f25',  U'\x01f26',  U'\x01f27',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x01f30',  U'\x01f31',  U'\x01f32',  U'\x01f33',  U'\x01f34',  U'\x01f35',  U'\x01f36',
 U'\x01f37',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x01f40',  U'\x01f41',  U'\x01f42',  U'\x01f43',  U'\x01f44',
 U'\x01f45',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f51',  U'\x00000',
 U'\x01f53',  U'\x00000',  U'\x01f55',  U'\x00000',  U'\x01f57',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f60',
 U'\x01f61',  U'\x01f62',  U'\x01f63',  U'\x01f64',  U'\x01f65',  U'\x01f66',  U'\x01f67',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f80',  U'\x01f81',  U'\x01f82',  U'\x01f83',
 U'\x01f84',  U'\x01f85',  U'\x01f86',  U'\x01f87',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f90',  U'\x01f91',
 U'\x01f92',  U'\x01f93',  U'\x01f94',  U'\x01f95',  U'\x01f96',  U'\x01f97',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x01fa0',  U'\x01fa1',  U'\x01fa2',  U'\x01fa3',  U'\x01fa4',  U'\x01fa5',  U'\x01fa6',
 U'\x01fa7',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x01fb0',  U'\x01fb1',  U'\x01f70',  U'\x01f71',  U'\x01fb3',
 U'\x00000',  U'\x003b9',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f72',  U'\x01f73',  U'\x01f74',
 U'\x01f75',  U'\x01fc3',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01fd0',
 U'\x01fd1',  U'\x01f76',  U'\x01f77',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x01fe0',  U'\x01fe1',  U'\x01f7a',  U'\x01f7b',  U'\x01fe5',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f78',  U'\x01f79',  U'\x01f7c',  U'\x01f7d',
 U'\x01ff3'
};

static constexpr const char32_t FOLDCASE_0x00021[]
{
 U'\x00084',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x003c9',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x0006b',  U'\x000e5',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x0214e',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02170',  U'\x02171',
 U'\x02172',  U'\x02173',  U'\x02174',  U'\x02175',  U'\x02176',  U'\x02177',  U'\x02178',
 U'\x02179',  U'\x0217a',  U'\x0217b',  U'\x0217c',  U'\x0217d',  U'\x0217e',  U'\x0217f',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02184'
};

static constexpr const char32_t FOLDCASE_0x00024[]
{
 U'\x000d0',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x024d0',  U'\x024d1',  U'\x024d2',  U'\x024d3',  U'\x024d4',  U'\x024d5',  U'\x024d6',
 U'\x024d7',  U'\x024d8',  U'\x024d9',  U'\x024da',  U'\x024db',  U'\x024dc',  U'\x024dd',
 U'\x024de',  U'\x024df',  U'\x024e0',  U'\x024e1',  U'\x024e2',  U'\x024e3',  U'\x024e4',
 U'\x024e5',  U'\x024e6',  U'\x024e7',  U'\x024e8',  U'\x024e9'
};

static constexpr const char32_t FOLDCASE_0x0002c[]
{
 U'\x000f3',
 U'\x02c30',  U'\x02c31',  U'\x02c32',  U'\x02c33',  U'\x02c34',  U'\x02c35',  U'\x02c36',
 U'\x02c37',  U'\x02c38',  U'\x02c39',  U'\x02c3a',  U'\x02c3b',  U'\x02c3c',  U'\x02c3d',
 U'\x02c3e',  U'\x02c3f',  U'\x02c40',  U'\x02c41',  U'\x02c42',  U'\x02c43',  U'\x02c44',
 U'\x02c45',  U'\x02c46',  U'\x02c47',  U'\x02c48',  U'\x02c49',  U'\x02c4a',  U'\x02c4b',
 U'\x02c4c',  U'\x02c4d',  U'\x02c4e',  U'\x02c4f',  U'\x02c50',  U'\x02c51',  U'\x02c52',
 U'\x02c53',  U'\x02c54',  U'\x02c55',  U'\x02c56',  U'\x02c57',  U'\x02c58',  U'\x02c59',
 U'\x02c5a',  U'\x02c5b',  U'\x02c5c',  U'\x02c5d',  U'\x02c5e',  U'\x02c5f',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02c61',  U'\x00000',
 U'\x0026b',  U'\x01d7d',  U'\x0027d',  U'\x00000',  U'\x00000',  U'\x02c68',  U'\x00000',
 U'\x02c6a',  U'\x00000',  U'\x02c6c',  U'\x00000',  U'\x00251',  U'\x00271',  U'\x00250',
 U'\x00252',  U'\x00000',  U'\x02c73',  U'\x00000',  U'\x00000',  U'\x02c76',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x0023f',  U'\x00240',  U'\x02c81',  U'\x00000',  U'\x02c83',  U'\x00000',  U'\x02c85',
 U'\x00000',  U'\x02c87',  U'\x00000',  U'\x02c89',  U'\x00000',  U'\x02c8b',  U'\x00000',
 U'\x02c8d',  U'\x00000',  U'\x02c8f',  U'\x00000',  U'\x02c91',  U'\x00000',  U'\x02c93',
 U'\x00000',  U'\x02c95',  U'\x00000',  U'\x02c97',  U'\x00000',  U'\x02c99',  U'\x00000',
 U'\x02c9b',  U'\x00000',  U'\x02c9d',  U'\x00000',  U'\x02c9f',  U'\x00000',  U'\x02ca1',
 U'\x00000',  U'\x02ca3',  U'\x00000',  U'\x02ca5',  U'\x00000',  U'\x02ca7',  U'\x00000',
 U'\x02ca9',  U'\x00000',  U'\x02cab',  U'\x00000',  U'\x02cad',  U'\x00000',  U'\x02caf',
 U'\x00000',  U'\x02cb1',  U'\x00000',  U'\x02cb3',  U'\x00000',  U'\x02cb5',  U'\x00000',
 U'\x02cb7',  U'\x00000',  U'\x02cb9',  U'\x00000',  U'\x02cbb',  U'\x00000',  U'\x02cbd',
 U'\x00000',  U'\x02cbf',  U'\x00000',  U'\x02cc1',  U'\x00000',  U'\x02cc3',  U'\x00000',
 U'\x02cc5',  U'\x00000',  U'\x02cc7',  U'\x00000',  U'\x02cc9',  U'\x00000',  U'\x02ccb',
 U'\x00000',  U'\x02ccd',  U'\x00000',  U'\x02ccf',  U'\x00000',  U'\x02cd1',  U'\x00000',
 U'\x02cd3',  U'\x00000',  U'\x02cd5',  U'\x00000',  U'\x02cd7',  U'\x00000',  U'\x02cd9',
 U'\x00000',  U'\x02cdb',  U'\x00000',  U'\x02cdd',  U'\x00000',  U'\x02cdf',  U'\x00000',
 U'\x02ce1',  U'\x00000',  U'\x02ce3',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02cec',  U'\x00000',  U'\x02cee',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02cf3'
};

static constexpr const char32_t FOLDCASE_0x000a6[]
{
 U'\x0009b',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x0a641',  U'\x00000',  U'\x0a643',  U'\x00000',  U'\x0a645',  U'\x00000',
 U'\x0a647',  U'\x00000',  U'\x0a649',  U'\x00000',  U'\x0a64b',  U'\x00000',  U'\x0a64d',
 U'\x00000',  U'\x0a64f',  U'\x00000',  U'\x0a651',  U'\x00000',  U'\x0a653',  U'\x00000',
 U'\x0a655',  U'\x00000',  U'\x0a657',  U'\x00000',  U'\x0a659',  U'\x00000',  U'\x0a65b',
 U'\x00000',  U'\x0a65d',  U'\x00000',  U'\x0a65f',  U'\x00000',  U'\x0a661',  U'\x00000',
 U'\x0a663',  U'\x00000',  U'\x0a665',  U'\x00000',  U'\x0a667',  U'\x00000',  U'\x0a669',
 U'\x00000',  U'\x0a66b',  U'\x00000',  U'\x0a66d',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x0a681',  U'\x00000',  U'\x0a683',  U'\x00000',  U'\x0a685',
 U'\x00000',  U'\x0a687',  U'\x00000',  U'\x0a689',  U'\x00000',  U'\x0a68b',  U'\x00000',
 U'\x0a68d',  U'\x00000',  U'\x0a68f',  U'\x00000',  U'\x0a691',  U'\x00000',  U'\x0a693',
 U'\x00000',  U'\x0a695',  U'\x00000',  U'\x0a697',  U'\x00000',  U'\x0a699',  U'\x00000',
 U'\x0a69b'
};

static constexpr const char32_t FOLDCASE_0x000a7[]
{
 U'\x000f6',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a723',
 U'\x00000',  U'\x0a725',  U'\x00000',  U'\x0a727',  U'\x00000',  U'\x0a729',  U'\x00000',
 U'\x0a72b',  U'\x00000',  U'\x0a72d',  U'\x00000',  U'\x0a72f',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x0a733',  U'\x00000',  U'\x0a735',  U'\x00000',  U'\x0a737',  U'\x00000',
 U'\x0a739',  U'\x00000',  U'\x0a73b',  U'\x00000',  U'\x0a73d',  U'\x00000',  U'\x0a73f',
 U'\x00000',  U'\x0a741',  U'\x00000',  U'\x0a743',  U'\x00000',  U'\x0a745',  U'\x00000',
 U'\x0a747',  U'\x00000',  U'\x0a749',  U'\x00000',  U'\x0a74b',  U'\x00000',  U'\x0a74d',
 U'\x00000',  U'\x0a74f',  U'\x00000',  U'\x0a751',  U'\x00000',  U'\x0a753',  U'\x00000',
 U'\x0a755',  U'\x00000',  U'\x0a757',  U'\x00000',  U'\x0a759',  U'\x00000',  U'\x0a75b',
 U'\x00000',  U'\x0a75d',  U'\x00000',  U'\x0a75f',  U'\x00000',  U'\x0a761',  U'\x00000',
 U'\x0a763',  U'\x00000',  U'\x0a765',  U'\x00000',  U'\x0a767',  U'\x00000',  U'\x0a769',
 U'\x00000',  U'\x0a76b',  U'\x00000',  U'\x0a76d',  U'\x00000',  U'\x0a76f',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x0a77a',  U'\x00000',  U'\x0a77c',  U'\x00000',  U'\x01d79',
 U'\x0a77f',  U'\x00000',  U'\x0a781',  U'\x00000',  U'\x0a783',  U'\x00000',  U'\x0a785',
 U'\x00000',  U'\x0a787',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a78c',
 U'\x00000',  U'\x00265',  U'\x00000',  U'\x00000',  U'\x0a791',  U'\x00000',  U'\x0a793',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a797',  U'\x00000',  U'\x0a799',  U'\x00000',
 U'\x0a79b',  U'\x00000',  U'\x0a79d',  U'\x00000',  U'\x0a79f',  U'\x00000',  U'\x0a7a1',
 U'\x00000',  U'\x0a7a3',  U'\x00000',  U'\x0a7a5',  U'\x00000',  U'\x0a7a7',  U'\x00000',
 U'\x0a7a9',  U'\x00000',  U'\x00266',  U'\x0025c',  U'\x00261',  U'\x0026c',  U'\x0026a',
 U'\x00000',  U'\x0029e',  U'\x00287',  U'\x0029d',  U'\x0ab53',  U'\x0a7b5',  U'\x00000',
 U'\x0a7b7',  U'\x00000',  U'\x0a7b9',  U'\x00000',  U'\x0a7bb',  U'\x00000',  U'\x0a7bd',
 U'\x00000',  U'\x0a7bf',  U'\x00000',  U'\x0a7c1',  U'\x00000',  U'\x0a7c3',  U'\x00000',
 U'\x0a794',  U'\x00282',  U'\x01d8e',  U'\x0a7c8',  U'\x00000',  U'\x0a7ca',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7d1',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7d7',  U'\x00000',  U'\x0a7d9',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x0a7f6'
};

static constexpr const char32_t FOLDCASE_0x000ab[]
{
 U'\x000c0',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x013a0',  U'\x013a1',  U'\x013a2',  U'\x013a3',  U'\x013a4',  U'\x013a5',  U'\x013a6',
 U'\x013a7',  U'\x013a8',  U'\x013a9',  U'\x013aa',  U'\x013ab',  U'\x013ac',  U'\x013ad',
 U'\x013ae',  U'\x013af',  U'\x013b0',  U'\x013b1',  U'\x013b2',  U'\x013b3',  U'\x013b4',
 U'\x013b5',  U'\x013b6',  U'\x013b7',  U'\x013b8',  U'\x013b9',  U'\x013ba',  U'\x013bb',
 U'\x013bc',  U'\x013bd',  U'\x013be',  U'\x013bf',  U'\x013c0',  U'\x013c1',  U'\x013c2',
 U'\x013c3',  U'\x013c4',  U'\x013c5',  U'\x013c6',  U'\x013c7',  U'\x013c8',  U'\x013c9',
 U'\x013ca',  U'\x013cb',  U'\x013cc',  U'\x013cd',  U'\x013ce',  U'\x013cf',  U'\x013d0',
 U'\x013d1',  U'\x013d2',  U'\x013d3',  U'\x013d4',  U'\x013d5',  U'\x013d6',  U'\x013d7',
 U'\x013d8',  U'\x013d9',  U'\x013da',  U'\x013db',  U'\x013dc',  U'\x013dd',  U'\x013de',
 U'\x013df',  U'\x013e0',  U'\x013e1',  U'\x013e2',  U'\x013e3',  U'\x013e4',  U'\x013e5',
 U'\x013e6',  U'\x013e7',  U'\x013e8',  U'\x013e9',  U'\x013ea',  U'\x013eb',  U'\x013ec',
 U'\x013ed',  U'\x013ee',  U'\x013ef'
};

static constexpr const char32_t FOLDCASE_0x000ff[]
{
 U'\x0003b',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0ff41',  U'\x0ff42',
 U'\x0ff43',  U'\x0ff44',  U'\x0ff45',  U'\x0ff46',  U'\x0ff47',  U'\x0ff48',  U'\x0ff49',
 U'\x0ff4a',  U'\x0ff4b',  U'\x0ff4c',  U'\x0ff4d',  U'\x0ff4e',  U'\x0ff4f',  U'\x0ff50',
 U'\x0ff51',  U'\x0ff52',  U'\x0ff53',  U'\x0ff54',  U'\x0ff55',  U'\x0ff56',  U'\x0ff57',
 U'\x0ff58',  U'\x0ff59',  U'\x0ff5a'
};

static constexpr const char32_t FOLDCASE_0x00104[]
{
 U'\x000d4',
 U'\x10428',  U'\x10429',  U'\x1042a',  U'\x1042b',  U'\x1042c',  U'\x1042d',  U'\x1042e',
 U'\x1042f',  U'\x10430',  U'\x10431',  U'\x10432',  U'\x10433',  U'\x10434',  U'\x10435',
 U'\x10436',  U'\x10437',  U'\x10438',  U'\x10439',  U'\x1043a',  U'\x1043b',  U'\x1043c',
 U'\x1043d',  U'\x1043e',  U'\x1043f',  U'\x10440',  U'\x10441',  U'\x10442',  U'\x10443',
 U'\x10444',  U'\x10445',  U'\x10446',  U'\x10447',  U'\x10448',  U'\x10449',  U'\x1044a',
 U'\x1044b',  U'\x1044c',  U'\x1044d',  U'\x1044e',  U'\x1044f',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x104d8',  U'\x104d9',  U'\x104da',  U'\x104db',  U'\x104dc',  U'\x104dd',
 U'\x104de',  U'\x104df',  U'\x104e0',  U'\x104e1',  U'\x104e2',  U'\x104e3',  U'\x104e4',
 U'\x104e5',  U'\x104e6',  U'\x104e7',  U'\x104e8',  U'\x104e9',  U'\x104ea',  U'\x104eb',
 U'\x104ec',  U'\x104ed',  U'\x104ee',  U'\x104ef',  U'\x104f0',  U'\x104f1',  U'\x104f2',
 U'\x104f3',  U'\x104f4',  U'\x104f5',  U'\x104f6',  U'\x104f7',  U'\x104f8',  U'\x104f9',
 U'\x104fa',  U'\x104fb'
};

static constexpr const char32_t FOLDCASE_0x00105[]
{
 U'\x00096',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x10597',  U'\x10598',  U'\x10599',  U'\x1059a',  U'\x1059b',  U'\x1059c',  U'\x1059d',
 U'\x1059e',  U'\x1059f',  U'\x105a0',  U'\x105a1',  U'\x00000',  U'\x105a3',  U'\x105a4',
 U'\x105a5',  U'\x105a6',  U'\x105a7',  U'\x105a8',  U'\x105a9',  U'\x105aa',  U'\x105ab',
 U'\x105ac',  U'\x105ad',  U'\x105ae',  U'\x105af',  U'\x105b0',  U'\x105b1',  U'\x00000',
 U'\x105b3',  U'\x105b4',  U'\x105b5',  U'\x105b6',  U'\x105b7',  U'\x105b8',  U'\x105b9',
 U'\x00000',  U'\x105bb',  U'\x105bc'
};

static constexpr const char32_t FOLDCASE_0x0010c[]
{
 U'\x000b3',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x10cc0',  U'\x10cc1',  U'\x10cc2',  U'\x10cc3',  U'\x10cc4',
 U'\x10cc5',  U'\x10cc6',  U'\x10cc7',  U'\x10cc8',  U'\x10cc9',  U'\x10cca',  U'\x10ccb',
 U'\x10ccc',  U'\x10ccd',  U'\x10cce',  U'\x10ccf',  U'\x10cd0',  U'\x10cd1',  U'\x10cd2',
 U'\x10cd3',  U'\x10cd4',  U'\x10cd5',  U'\x10cd6',  U'\x10cd7',  U'\x10cd8',  U'\x10cd9',
 U'\x10cda',  U'\x10cdb',  U'\x10cdc',  U'\x10cdd',  U'\x10cde',  U'\x10cdf',  U'\x10ce0',
 U'\x10ce1',  U'\x10ce2',  U'\x10ce3',  U'\x10ce4',  U'\x10ce5',  U'\x10ce6',  U'\x10ce7',
 U'\x10ce8',  U'\x10ce9',  U'\x10cea',  U'\x10ceb',  U'\x10cec',  U'\x10ced',  U'\x10cee',
 U'\x10cef',  U'\x10cf0',  U'\x10cf1',  U'\x10cf2'
};

static constexpr const char32_t FOLDCASE_0x00118[]
{
 U'\x000c0',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x118c0',
 U'\x118c1',  U'\x118c2',  U'\x118c3',  U'\x118c4',  U'\x118c5',  U'\x118c6',  U'\x118c7',
 U'\x118c8',  U'\x118c9',  U'\x118ca',  U'\x118cb',  U'\x118cc',  U'\x118cd',  U'\x118ce',
 U'\x118cf',  U'\x118d0',  U'\x118d1',  U'\x118d2',  U'\x118d3',  U'\x118d4',  U'\x118d5',
 U'\x118d6',  U'\x118d7',  U'\x118d8',  U'\x118d9',  U'\x118da',  U'\x118db',  U'\x118dc',
 U'\x118dd',  U'\x118de',  U'\x118df'
};

static constexpr const char32_t FOLDCASE_0x0016e[]
{
 U'\x00060',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
 U'\x00000',  U'\x16e60',  U'\x16e61',  U'\x16e62',  U'\x16e63',  U'\x16e64',  U'\x16e65',
 U'\x16e66',  U'\x16e67',  U'\x16e68',  U'\x16e69',  U'\x16e6a',  U'\x16e6b',  U'\x16e6c',
 U'\x16e6d',  U'\x16e6e',  U'\x16e6f',  U'\x16e70',  U'\x16e71',  U'\x16e72',  U'\x16e73',
 U'\x16e74',  U'\x16e75',  U'\x16e76',  U'\x16e77',  U'\x16e78',  U'\x16e79',  U'\x16e7a',
 U'\x16e7b',  U'\x16e7c',  U'\x16e7d',  U'\x16e7e',  U'\x16e7f'
};

static constexpr const char32_t FOLDCASE_0x001e9[]
{
 U'\x00022',
 U'\x1e922',  U'\x1e923',  U'\x1e924',  U'\x1e925',  U'\x1e926',  U'\x1e927',  U'\x1e928',
 U'\x1e929',  U'\x1e92a',  U'\x1e92b',  U'\x1e92c',  U'\x1e92d',  U'\x1e92e',  U'\x1e92f',
 U'\x1e930',  U'\x1e931',  U'\x1e932',  U'\x1e933',  U'\x1e934',  U'\x1e935',  U'\x1e936',
 U'\x1e937',  U'\x1e938',  U'\x1e939',  U'\x1e93a',  U'\x1e93b',  U'\x1e93c',  U'\x1e93d',
 U'\x1e93e',  U'\x1e93f',  U'\x1e940',  U'\x1e941',  U'\x1e942',  U'\x1e943'
};

static constexpr const char32_t* const FOLDCASE_INDEX []
{
 FOLDCASE_0x00000,  FOLDCASE_0x00001,  FOLDCASE_0x00002,  FOLDCASE_0x00003,  FOLDCASE_0x00004,  FOLDCASE_0x00005,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x00010,  0x0,
 0x0,  FOLDCASE_0x00013,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x0001c,  0x0,
 FOLDCASE_0x0001e,  FOLDCASE_0x0001f,  0x0,  FOLDCASE_0x00021,  0x0,  0x0,
 FOLDCASE_0x00024,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  FOLDCASE_0x0002c,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x000a6,  FOLDCASE_0x000a7,
 0x0,  0x0,  0x0,  FOLDCASE_0x000ab,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  FOLDCASE_0x000ff,  0x0,  0x0,
 0x0,  0x0,  FOLDCASE_0x00104,  FOLDCASE_0x00105,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x0010c,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  FOLDCASE_0x00118,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 FOLDCASE_0x0016e,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
 0x0,  0x0,  0x0,  FOLDCASE_0x001e9
};

static constexpr const char32_t LOWER_CASE_0x000[]
{
  U'\x000de',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00061',  U'\x00062',  U'\x00063',  U'\x00064',  U'\x00065',
  U'\x00066',  U'\x00067',  U'\x00068',  U'\x00069',  U'\x0006a',  U'\x0006b',  U'\x0006c',
  U'\x0006d',  U'\x0006e',  U'\x0006f',  U'\x00070',  U'\x00071',  U'\x00072',  U'\x00073',
  U'\x00074',  U'\x00075',  U'\x00076',  U'\x00077',  U'\x00078',  U'\x00079',  U'\x0007a',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x000e0',  U'\x000e1',  U'\x000e2',  U'\x000e3',
  U'\x000e4',  U'\x000e5',  U'\x000e6',  U'\x000e7',  U'\x000e8',  U'\x000e9',  U'\x000ea',
  U'\x000eb',  U'\x000ec',  U'\x000ed',  U'\x000ee',  U'\x000ef',  U'\x000f0',  U'\x000f1',
  U'\x000f2',  U'\x000f3',  U'\x000f4',  U'\x000f5',  U'\x000f6',  U'\x00000',  U'\x000f8',
  U'\x000f9',  U'\x000fa',  U'\x000fb',  U'\x000fc',  U'\x000fd',  U'\x000fe'
};

static constexpr const char32_t LOWER_CASE_0x001[]
{
  U'\x000fe',
  U'\x00101',  U'\x00000',  U'\x00103',  U'\x00000',  U'\x00105',  U'\x00000',  U'\x00107',
  U'\x00000',  U'\x00109',  U'\x00000',  U'\x0010b',  U'\x00000',  U'\x0010d',  U'\x00000',
  U'\x0010f',  U'\x00000',  U'\x00111',  U'\x00000',  U'\x00113',  U'\x00000',  U'\x00115',
  U'\x00000',  U'\x00117',  U'\x00000',  U'\x00119',  U'\x00000',  U'\x0011b',  U'\x00000',
  U'\x0011d',  U'\x00000',  U'\x0011f',  U'\x00000',  U'\x00121',  U'\x00000',  U'\x00123',
  U'\x00000',  U'\x00125',  U'\x00000',  U'\x00127',  U'\x00000',  U'\x00129',  U'\x00000',
  U'\x0012b',  U'\x00000',  U'\x0012d',  U'\x00000',  U'\x0012f',  U'\x00000',  U'\x00069',
  U'\x00000',  U'\x00133',  U'\x00000',  U'\x00135',  U'\x00000',  U'\x00137',  U'\x00000',
  U'\x00000',  U'\x0013a',  U'\x00000',  U'\x0013c',  U'\x00000',  U'\x0013e',  U'\x00000',
  U'\x00140',  U'\x00000',  U'\x00142',  U'\x00000',  U'\x00144',  U'\x00000',  U'\x00146',
  U'\x00000',  U'\x00148',  U'\x00000',  U'\x00000',  U'\x0014b',  U'\x00000',  U'\x0014d',
  U'\x00000',  U'\x0014f',  U'\x00000',  U'\x00151',  U'\x00000',  U'\x00153',  U'\x00000',
  U'\x00155',  U'\x00000',  U'\x00157',  U'\x00000',  U'\x00159',  U'\x00000',  U'\x0015b',
  U'\x00000',  U'\x0015d',  U'\x00000',  U'\x0015f',  U'\x00000',  U'\x00161',  U'\x00000',
  U'\x00163',  U'\x00000',  U'\x00165',  U'\x00000',  U'\x00167',  U'\x00000',  U'\x00169',
  U'\x00000',  U'\x0016b',  U'\x00000',  U'\x0016d',  U'\x00000',  U'\x0016f',  U'\x00000',
  U'\x00171',  U'\x00000',  U'\x00173',  U'\x00000',  U'\x00175',  U'\x00000',  U'\x00177',
  U'\x00000',  U'\x000ff',  U'\x0017a',  U'\x00000',  U'\x0017c',  U'\x00000',  U'\x0017e',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00253',  U'\x00183',  U'\x00000',  U'\x00185',
  U'\x00000',  U'\x00254',  U'\x00188',  U'\x00000',  U'\x00256',  U'\x00257',  U'\x0018c',
  U'\x00000',  U'\x00000',  U'\x001dd',  U'\x00259',  U'\x0025b',  U'\x00192',  U'\x00000',
  U'\x00260',  U'\x00263',  U'\x00000',  U'\x00269',  U'\x00268',  U'\x00199',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x0026f',  U'\x00272',  U'\x00000',  U'\x00275',  U'\x001a1',
  U'\x00000',  U'\x001a3',  U'\x00000',  U'\x001a5',  U'\x00000',  U'\x00280',  U'\x001a8',
  U'\x00000',  U'\x00283',  U'\x00000',  U'\x00000',  U'\x001ad',  U'\x00000',  U'\x00288',
  U'\x001b0',  U'\x00000',  U'\x0028a',  U'\x0028b',  U'\x001b4',  U'\x00000',  U'\x001b6',
  U'\x00000',  U'\x00292',  U'\x001b9',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x001bd',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x001c6',  U'\x001c6',  U'\x00000',  U'\x001c9',  U'\x001c9',  U'\x00000',  U'\x001cc',
  U'\x001cc',  U'\x00000',  U'\x001ce',  U'\x00000',  U'\x001d0',  U'\x00000',  U'\x001d2',
  U'\x00000',  U'\x001d4',  U'\x00000',  U'\x001d6',  U'\x00000',  U'\x001d8',  U'\x00000',
  U'\x001da',  U'\x00000',  U'\x001dc',  U'\x00000',  U'\x00000',  U'\x001df',  U'\x00000',
  U'\x001e1',  U'\x00000',  U'\x001e3',  U'\x00000',  U'\x001e5',  U'\x00000',  U'\x001e7',
  U'\x00000',  U'\x001e9',  U'\x00000',  U'\x001eb',  U'\x00000',  U'\x001ed',  U'\x00000',
  U'\x001ef',  U'\x00000',  U'\x00000',  U'\x001f3',  U'\x001f3',  U'\x00000',  U'\x001f5',
  U'\x00000',  U'\x00195',  U'\x001bf',  U'\x001f9',  U'\x00000',  U'\x001fb',  U'\x00000',
  U'\x001fd',  U'\x00000',  U'\x001ff'
};

static constexpr const char32_t LOWER_CASE_0x002[]
{
  U'\x0004e',
  U'\x00201',  U'\x00000',  U'\x00203',  U'\x00000',  U'\x00205',  U'\x00000',  U'\x00207',
  U'\x00000',  U'\x00209',  U'\x00000',  U'\x0020b',  U'\x00000',  U'\x0020d',  U'\x00000',
  U'\x0020f',  U'\x00000',  U'\x00211',  U'\x00000',  U'\x00213',  U'\x00000',  U'\x00215',
  U'\x00000',  U'\x00217',  U'\x00000',  U'\x00219',  U'\x00000',  U'\x0021b',  U'\x00000',
  U'\x0021d',  U'\x00000',  U'\x0021f',  U'\x00000',  U'\x0019e',  U'\x00000',  U'\x00223',
  U'\x00000',  U'\x00225',  U'\x00000',  U'\x00227',  U'\x00000',  U'\x00229',  U'\x00000',
  U'\x0022b',  U'\x00000',  U'\x0022d',  U'\x00000',  U'\x0022f',  U'\x00000',  U'\x00231',
  U'\x00000',  U'\x00233',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x02c65',  U'\x0023c',  U'\x00000',  U'\x0019a',  U'\x02c66',
  U'\x00000',  U'\x00000',  U'\x00242',  U'\x00000',  U'\x00180',  U'\x00289',  U'\x0028c',
  U'\x00247',  U'\x00000',  U'\x00249',  U'\x00000',  U'\x0024b',  U'\x00000',  U'\x0024d',
  U'\x00000',  U'\x0024f'
};

static constexpr const char32_t LOWER_CASE_0x003[]
{
  U'\x000ff',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00371',  U'\x00000',  U'\x00373',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00377',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x003f3',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x003ac',  U'\x00000',  U'\x003ad',  U'\x003ae',  U'\x003af',  U'\x00000',
  U'\x003cc',  U'\x00000',  U'\x003cd',  U'\x003ce',  U'\x00000',  U'\x003b1',  U'\x003b2',
  U'\x003b3',  U'\x003b4',  U'\x003b5',  U'\x003b6',  U'\x003b7',  U'\x003b8',  U'\x003b9',
  U'\x003ba',  U'\x003bb',  U'\x003bc',  U'\x003bd',  U'\x003be',  U'\x003bf',  U'\x003c0',
  U'\x003c1',  U'\x00000',  U'\x003c3',  U'\x003c4',  U'\x003c5',  U'\x003c6',  U'\x003c7',
  U'\x003c8',  U'\x003c9',  U'\x003ca',  U'\x003cb',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003d7',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003d9',
  U'\x00000',  U'\x003db',  U'\x00000',  U'\x003dd',  U'\x00000',  U'\x003df',  U'\x00000',
  U'\x003e1',  U'\x00000',  U'\x003e3',  U'\x00000',  U'\x003e5',  U'\x00000',  U'\x003e7',
  U'\x00000',  U'\x003e9',  U'\x00000',  U'\x003eb',  U'\x00000',  U'\x003ed',  U'\x00000',
  U'\x003ef',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003b8',
  U'\x00000',  U'\x00000',  U'\x003f8',  U'\x00000',  U'\x003f2',  U'\x003fb',  U'\x00000',
  U'\x00000',  U'\x0037b',  U'\x0037c',  U'\x0037d'
};

static constexpr const char32_t LOWER_CASE_0x004[]
{
  U'\x000fe',
  U'\x00450',  U'\x00451',  U'\x00452',  U'\x00453',  U'\x00454',  U'\x00455',  U'\x00456',
  U'\x00457',  U'\x00458',  U'\x00459',  U'\x0045a',  U'\x0045b',  U'\x0045c',  U'\x0045d',
  U'\x0045e',  U'\x0045f',  U'\x00430',  U'\x00431',  U'\x00432',  U'\x00433',  U'\x00434',
  U'\x00435',  U'\x00436',  U'\x00437',  U'\x00438',  U'\x00439',  U'\x0043a',  U'\x0043b',
  U'\x0043c',  U'\x0043d',  U'\x0043e',  U'\x0043f',  U'\x00440',  U'\x00441',  U'\x00442',
  U'\x00443',  U'\x00444',  U'\x00445',  U'\x00446',  U'\x00447',  U'\x00448',  U'\x00449',
  U'\x0044a',  U'\x0044b',  U'\x0044c',  U'\x0044d',  U'\x0044e',  U'\x0044f',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00461',  U'\x00000',
  U'\x00463',  U'\x00000',  U'\x00465',  U'\x00000',  U'\x00467',  U'\x00000',  U'\x00469',
  U'\x00000',  U'\x0046b',  U'\x00000',  U'\x0046d',  U'\x00000',  U'\x0046f',  U'\x00000',
  U'\x00471',  U'\x00000',  U'\x00473',  U'\x00000',  U'\x00475',  U'\x00000',  U'\x00477',
  U'\x00000',  U'\x00479',  U'\x00000',  U'\x0047b',  U'\x00000',  U'\x0047d',  U'\x00000',
  U'\x0047f',  U'\x00000',  U'\x00481',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0048b',  U'\x00000',
  U'\x0048d',  U'\x00000',  U'\x0048f',  U'\x00000',  U'\x00491',  U'\x00000',  U'\x00493',
  U'\x00000',  U'\x00495',  U'\x00000',  U'\x00497',  U'\x00000',  U'\x00499',  U'\x00000',
  U'\x0049b',  U'\x00000',  U'\x0049d',  U'\x00000',  U'\x0049f',  U'\x00000',  U'\x004a1',
  U'\x00000',  U'\x004a3',  U'\x00000',  U'\x004a5',  U'\x00000',  U'\x004a7',  U'\x00000',
  U'\x004a9',  U'\x00000',  U'\x004ab',  U'\x00000',  U'\x004ad',  U'\x00000',  U'\x004af',
  U'\x00000',  U'\x004b1',  U'\x00000',  U'\x004b3',  U'\x00000',  U'\x004b5',  U'\x00000',
  U'\x004b7',  U'\x00000',  U'\x004b9',  U'\x00000',  U'\x004bb',  U'\x00000',  U'\x004bd',
  U'\x00000',  U'\x004bf',  U'\x00000',  U'\x004cf',  U'\x004c2',  U'\x00000',  U'\x004c4',
  U'\x00000',  U'\x004c6',  U'\x00000',  U'\x004c8',  U'\x00000',  U'\x004ca',  U'\x00000',
  U'\x004cc',  U'\x00000',  U'\x004ce',  U'\x00000',  U'\x00000',  U'\x004d1',  U'\x00000',
  U'\x004d3',  U'\x00000',  U'\x004d5',  U'\x00000',  U'\x004d7',  U'\x00000',  U'\x004d9',
  U'\x00000',  U'\x004db',  U'\x00000',  U'\x004dd',  U'\x00000',  U'\x004df',  U'\x00000',
  U'\x004e1',  U'\x00000',  U'\x004e3',  U'\x00000',  U'\x004e5',  U'\x00000',  U'\x004e7',
  U'\x00000',  U'\x004e9',  U'\x00000',  U'\x004eb',  U'\x00000',  U'\x004ed',  U'\x00000',
  U'\x004ef',  U'\x00000',  U'\x004f1',  U'\x00000',  U'\x004f3',  U'\x00000',  U'\x004f5',
  U'\x00000',  U'\x004f7',  U'\x00000',  U'\x004f9',  U'\x00000',  U'\x004fb',  U'\x00000',
  U'\x004fd',  U'\x00000',  U'\x004ff'
};

static constexpr const char32_t LOWER_CASE_0x005[]
{
  U'\x00056',
  U'\x00501',  U'\x00000',  U'\x00503',  U'\x00000',  U'\x00505',  U'\x00000',  U'\x00507',
  U'\x00000',  U'\x00509',  U'\x00000',  U'\x0050b',  U'\x00000',  U'\x0050d',  U'\x00000',
  U'\x0050f',  U'\x00000',  U'\x00511',  U'\x00000',  U'\x00513',  U'\x00000',  U'\x00515',
  U'\x00000',  U'\x00517',  U'\x00000',  U'\x00519',  U'\x00000',  U'\x0051b',  U'\x00000',
  U'\x0051d',  U'\x00000',  U'\x0051f',  U'\x00000',  U'\x00521',  U'\x00000',  U'\x00523',
  U'\x00000',  U'\x00525',  U'\x00000',  U'\x00527',  U'\x00000',  U'\x00529',  U'\x00000',
  U'\x0052b',  U'\x00000',  U'\x0052d',  U'\x00000',  U'\x0052f',  U'\x00000',  U'\x00000',
  U'\x00561',  U'\x00562',  U'\x00563',  U'\x00564',  U'\x00565',  U'\x00566',  U'\x00567',
  U'\x00568',  U'\x00569',  U'\x0056a',  U'\x0056b',  U'\x0056c',  U'\x0056d',  U'\x0056e',
  U'\x0056f',  U'\x00570',  U'\x00571',  U'\x00572',  U'\x00573',  U'\x00574',  U'\x00575',
  U'\x00576',  U'\x00577',  U'\x00578',  U'\x00579',  U'\x0057a',  U'\x0057b',  U'\x0057c',
  U'\x0057d',  U'\x0057e',  U'\x0057f',  U'\x00580',  U'\x00581',  U'\x00582',  U'\x00583',
  U'\x00584',  U'\x00585',  U'\x00586'
};

static constexpr const char32_t LOWER_CASE_0x010[]
{
  U'\x000cd',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02d00',
  U'\x02d01',  U'\x02d02',  U'\x02d03',  U'\x02d04',  U'\x02d05',  U'\x02d06',  U'\x02d07',
  U'\x02d08',  U'\x02d09',  U'\x02d0a',  U'\x02d0b',  U'\x02d0c',  U'\x02d0d',  U'\x02d0e',
  U'\x02d0f',  U'\x02d10',  U'\x02d11',  U'\x02d12',  U'\x02d13',  U'\x02d14',  U'\x02d15',
  U'\x02d16',  U'\x02d17',  U'\x02d18',  U'\x02d19',  U'\x02d1a',  U'\x02d1b',  U'\x02d1c',
  U'\x02d1d',  U'\x02d1e',  U'\x02d1f',  U'\x02d20',  U'\x02d21',  U'\x02d22',  U'\x02d23',
  U'\x02d24',  U'\x02d25',  U'\x00000',  U'\x02d27',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x02d2d'
};

static constexpr const char32_t LOWER_CASE_0x013[]
{
  U'\x000f5',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0ab70',
  U'\x0ab71',  U'\x0ab72',  U'\x0ab73',  U'\x0ab74',  U'\x0ab75',  U'\x0ab76',  U'\x0ab77',
  U'\x0ab78',  U'\x0ab79',  U'\x0ab7a',  U'\x0ab7b',  U'\x0ab7c',  U'\x0ab7d',  U'\x0ab7e',
  U'\x0ab7f',  U'\x0ab80',  U'\x0ab81',  U'\x0ab82',  U'\x0ab83',  U'\x0ab84',  U'\x0ab85',
  U'\x0ab86',  U'\x0ab87',  U'\x0ab88',  U'\x0ab89',  U'\x0ab8a',  U'\x0ab8b',  U'\x0ab8c',
  U'\x0ab8d',  U'\x0ab8e',  U'\x0ab8f',  U'\x0ab90',  U'\x0ab91',  U'\x0ab92',  U'\x0ab93',
  U'\x0ab94',  U'\x0ab95',  U'\x0ab96',  U'\x0ab97',  U'\x0ab98',  U'\x0ab99',  U'\x0ab9a',
  U'\x0ab9b',  U'\x0ab9c',  U'\x0ab9d',  U'\x0ab9e',  U'\x0ab9f',  U'\x0aba0',  U'\x0aba1',
  U'\x0aba2',  U'\x0aba3',  U'\x0aba4',  U'\x0aba5',  U'\x0aba6',  U'\x0aba7',  U'\x0aba8',
  U'\x0aba9',  U'\x0abaa',  U'\x0abab',  U'\x0abac',  U'\x0abad',  U'\x0abae',  U'\x0abaf',
  U'\x0abb0',  U'\x0abb1',  U'\x0abb2',  U'\x0abb3',  U'\x0abb4',  U'\x0abb5',  U'\x0abb6',
  U'\x0abb7',  U'\x0abb8',  U'\x0abb9',  U'\x0abba',  U'\x0abbb',  U'\x0abbc',  U'\x0abbd',
  U'\x0abbe',  U'\x0abbf',  U'\x013f8',  U'\x013f9',  U'\x013fa',  U'\x013fb',  U'\x013fc',
  U'\x013fd'
};

static constexpr const char32_t LOWER_CASE_0x01c[]
{
  U'\x000bf',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x010d0',  U'\x010d1',  U'\x010d2',
  U'\x010d3',  U'\x010d4',  U'\x010d5',  U'\x010d6',  U'\x010d7',  U'\x010d8',  U'\x010d9',
  U'\x010da',  U'\x010db',  U'\x010dc',  U'\x010dd',  U'\x010de',  U'\x010df',  U'\x010e0',
  U'\x010e1',  U'\x010e2',  U'\x010e3',  U'\x010e4',  U'\x010e5',  U'\x010e6',  U'\x010e7',
  U'\x010e8',  U'\x010e9',  U'\x010ea',  U'\x010eb',  U'\x010ec',  U'\x010ed',  U'\x010ee',
  U'\x010ef',  U'\x010f0',  U'\x010f1',  U'\x010f2',  U'\x010f3',  U'\x010f4',  U'\x010f5',
  U'\x010f6',  U'\x010f7',  U'\x010f8',  U'\x010f9',  U'\x010fa',  U'\x00000',  U'\x00000',
  U'\x010fd',  U'\x010fe',  U'\x010ff'
};

static constexpr const char32_t LOWER_CASE_0x01e[]
{
  U'\x000fe',
  U'\x01e01',  U'\x00000',  U'\x01e03',  U'\x00000',  U'\x01e05',  U'\x00000',  U'\x01e07',
  U'\x00000',  U'\x01e09',  U'\x00000',  U'\x01e0b',  U'\x00000',  U'\x01e0d',  U'\x00000',
  U'\x01e0f',  U'\x00000',  U'\x01e11',  U'\x00000',  U'\x01e13',  U'\x00000',  U'\x01e15',
  U'\x00000',  U'\x01e17',  U'\x00000',  U'\x01e19',  U'\x00000',  U'\x01e1b',  U'\x00000',
  U'\x01e1d',  U'\x00000',  U'\x01e1f',  U'\x00000',  U'\x01e21',  U'\x00000',  U'\x01e23',
  U'\x00000',  U'\x01e25',  U'\x00000',  U'\x01e27',  U'\x00000',  U'\x01e29',  U'\x00000',
  U'\x01e2b',  U'\x00000',  U'\x01e2d',  U'\x00000',  U'\x01e2f',  U'\x00000',  U'\x01e31',
  U'\x00000',  U'\x01e33',  U'\x00000',  U'\x01e35',  U'\x00000',  U'\x01e37',  U'\x00000',
  U'\x01e39',  U'\x00000',  U'\x01e3b',  U'\x00000',  U'\x01e3d',  U'\x00000',  U'\x01e3f',
  U'\x00000',  U'\x01e41',  U'\x00000',  U'\x01e43',  U'\x00000',  U'\x01e45',  U'\x00000',
  U'\x01e47',  U'\x00000',  U'\x01e49',  U'\x00000',  U'\x01e4b',  U'\x00000',  U'\x01e4d',
  U'\x00000',  U'\x01e4f',  U'\x00000',  U'\x01e51',  U'\x00000',  U'\x01e53',  U'\x00000',
  U'\x01e55',  U'\x00000',  U'\x01e57',  U'\x00000',  U'\x01e59',  U'\x00000',  U'\x01e5b',
  U'\x00000',  U'\x01e5d',  U'\x00000',  U'\x01e5f',  U'\x00000',  U'\x01e61',  U'\x00000',
  U'\x01e63',  U'\x00000',  U'\x01e65',  U'\x00000',  U'\x01e67',  U'\x00000',  U'\x01e69',
  U'\x00000',  U'\x01e6b',  U'\x00000',  U'\x01e6d',  U'\x00000',  U'\x01e6f',  U'\x00000',
  U'\x01e71',  U'\x00000',  U'\x01e73',  U'\x00000',  U'\x01e75',  U'\x00000',  U'\x01e77',
  U'\x00000',  U'\x01e79',  U'\x00000',  U'\x01e7b',  U'\x00000',  U'\x01e7d',  U'\x00000',
  U'\x01e7f',  U'\x00000',  U'\x01e81',  U'\x00000',  U'\x01e83',  U'\x00000',  U'\x01e85',
  U'\x00000',  U'\x01e87',  U'\x00000',  U'\x01e89',  U'\x00000',  U'\x01e8b',  U'\x00000',
  U'\x01e8d',  U'\x00000',  U'\x01e8f',  U'\x00000',  U'\x01e91',  U'\x00000',  U'\x01e93',
  U'\x00000',  U'\x01e95',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x000df',  U'\x00000',  U'\x01ea1',
  U'\x00000',  U'\x01ea3',  U'\x00000',  U'\x01ea5',  U'\x00000',  U'\x01ea7',  U'\x00000',
  U'\x01ea9',  U'\x00000',  U'\x01eab',  U'\x00000',  U'\x01ead',  U'\x00000',  U'\x01eaf',
  U'\x00000',  U'\x01eb1',  U'\x00000',  U'\x01eb3',  U'\x00000',  U'\x01eb5',  U'\x00000',
  U'\x01eb7',  U'\x00000',  U'\x01eb9',  U'\x00000',  U'\x01ebb',  U'\x00000',  U'\x01ebd',
  U'\x00000',  U'\x01ebf',  U'\x00000',  U'\x01ec1',  U'\x00000',  U'\x01ec3',  U'\x00000',
  U'\x01ec5',  U'\x00000',  U'\x01ec7',  U'\x00000',  U'\x01ec9',  U'\x00000',  U'\x01ecb',
  U'\x00000',  U'\x01ecd',  U'\x00000',  U'\x01ecf',  U'\x00000',  U'\x01ed1',  U'\x00000',
  U'\x01ed3',  U'\x00000',  U'\x01ed5',  U'\x00000',  U'\x01ed7',  U'\x00000',  U'\x01ed9',
  U'\x00000',  U'\x01edb',  U'\x00000',  U'\x01edd',  U'\x00000',  U'\x01edf',  U'\x00000',
  U'\x01ee1',  U'\x00000',  U'\x01ee3',  U'\x00000',  U'\x01ee5',  U'\x00000',  U'\x01ee7',
  U'\x00000',  U'\x01ee9',  U'\x00000',  U'\x01eeb',  U'\x00000',  U'\x01eed',  U'\x00000',
  U'\x01eef',  U'\x00000',  U'\x01ef1',  U'\x00000',  U'\x01ef3',  U'\x00000',  U'\x01ef5',
  U'\x00000',  U'\x01ef7',  U'\x00000',  U'\x01ef9',  U'\x00000',  U'\x01efb',  U'\x00000',
  U'\x01efd',  U'\x00000',  U'\x01eff'
};

static constexpr const char32_t LOWER_CASE_0x01f[]
{
  U'\x000fc',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x01f00',  U'\x01f01',  U'\x01f02',  U'\x01f03',  U'\x01f04',  U'\x01f05',
  U'\x01f06',  U'\x01f07',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f10',  U'\x01f11',  U'\x01f12',  U'\x01f13',
  U'\x01f14',  U'\x01f15',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f20',  U'\x01f21',
  U'\x01f22',  U'\x01f23',  U'\x01f24',  U'\x01f25',  U'\x01f26',  U'\x01f27',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x01f30',  U'\x01f31',  U'\x01f32',  U'\x01f33',  U'\x01f34',  U'\x01f35',  U'\x01f36',
  U'\x01f37',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x01f40',  U'\x01f41',  U'\x01f42',  U'\x01f43',  U'\x01f44',
  U'\x01f45',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f51',  U'\x00000',
  U'\x01f53',  U'\x00000',  U'\x01f55',  U'\x00000',  U'\x01f57',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f60',
  U'\x01f61',  U'\x01f62',  U'\x01f63',  U'\x01f64',  U'\x01f65',  U'\x01f66',  U'\x01f67',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f80',  U'\x01f81',  U'\x01f82',  U'\x01f83',
  U'\x01f84',  U'\x01f85',  U'\x01f86',  U'\x01f87',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f90',  U'\x01f91',
  U'\x01f92',  U'\x01f93',  U'\x01f94',  U'\x01f95',  U'\x01f96',  U'\x01f97',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x01fa0',  U'\x01fa1',  U'\x01fa2',  U'\x01fa3',  U'\x01fa4',  U'\x01fa5',  U'\x01fa6',
  U'\x01fa7',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x01fb0',  U'\x01fb1',  U'\x01f70',  U'\x01f71',  U'\x01fb3',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f72',  U'\x01f73',  U'\x01f74',
  U'\x01f75',  U'\x01fc3',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01fd0',
  U'\x01fd1',  U'\x01f76',  U'\x01f77',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x01fe0',  U'\x01fe1',  U'\x01f7a',  U'\x01f7b',  U'\x01fe5',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f78',  U'\x01f79',  U'\x01f7c',  U'\x01f7d',
  U'\x01ff3'
};

static constexpr const char32_t LOWER_CASE_0x021[]
{
  U'\x00083',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003c9',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x0006b',  U'\x000e5',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x0214e',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02170',  U'\x02171',
  U'\x02172',  U'\x02173',  U'\x02174',  U'\x02175',  U'\x02176',  U'\x02177',  U'\x02178',
  U'\x02179',  U'\x0217a',  U'\x0217b',  U'\x0217c',  U'\x0217d',  U'\x0217e',  U'\x0217f',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02184'
};

static constexpr const char32_t LOWER_CASE_0x024[]
{
  U'\x000cf',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x024d0',  U'\x024d1',  U'\x024d2',  U'\x024d3',  U'\x024d4',  U'\x024d5',  U'\x024d6',
  U'\x024d7',  U'\x024d8',  U'\x024d9',  U'\x024da',  U'\x024db',  U'\x024dc',  U'\x024dd',
  U'\x024de',  U'\x024df',  U'\x024e0',  U'\x024e1',  U'\x024e2',  U'\x024e3',  U'\x024e4',
  U'\x024e5',  U'\x024e6',  U'\x024e7',  U'\x024e8',  U'\x024e9'
};

static constexpr const char32_t LOWER_CASE_0x02c[]
{
  U'\x000f2',
  U'\x02c30',  U'\x02c31',  U'\x02c32',  U'\x02c33',  U'\x02c34',  U'\x02c35',  U'\x02c36',
  U'\x02c37',  U'\x02c38',  U'\x02c39',  U'\x02c3a',  U'\x02c3b',  U'\x02c3c',  U'\x02c3d',
  U'\x02c3e',  U'\x02c3f',  U'\x02c40',  U'\x02c41',  U'\x02c42',  U'\x02c43',  U'\x02c44',
  U'\x02c45',  U'\x02c46',  U'\x02c47',  U'\x02c48',  U'\x02c49',  U'\x02c4a',  U'\x02c4b',
  U'\x02c4c',  U'\x02c4d',  U'\x02c4e',  U'\x02c4f',  U'\x02c50',  U'\x02c51',  U'\x02c52',
  U'\x02c53',  U'\x02c54',  U'\x02c55',  U'\x02c56',  U'\x02c57',  U'\x02c58',  U'\x02c59',
  U'\x02c5a',  U'\x02c5b',  U'\x02c5c',  U'\x02c5d',  U'\x02c5e',  U'\x02c5f',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02c61',  U'\x00000',
  U'\x0026b',  U'\x01d7d',  U'\x0027d',  U'\x00000',  U'\x00000',  U'\x02c68',  U'\x00000',
  U'\x02c6a',  U'\x00000',  U'\x02c6c',  U'\x00000',  U'\x00251',  U'\x00271',  U'\x00250',
  U'\x00252',  U'\x00000',  U'\x02c73',  U'\x00000',  U'\x00000',  U'\x02c76',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x0023f',  U'\x00240',  U'\x02c81',  U'\x00000',  U'\x02c83',  U'\x00000',  U'\x02c85',
  U'\x00000',  U'\x02c87',  U'\x00000',  U'\x02c89',  U'\x00000',  U'\x02c8b',  U'\x00000',
  U'\x02c8d',  U'\x00000',  U'\x02c8f',  U'\x00000',  U'\x02c91',  U'\x00000',  U'\x02c93',
  U'\x00000',  U'\x02c95',  U'\x00000',  U'\x02c97',  U'\x00000',  U'\x02c99',  U'\x00000',
  U'\x02c9b',  U'\x00000',  U'\x02c9d',  U'\x00000',  U'\x02c9f',  U'\x00000',  U'\x02ca1',
  U'\x00000',  U'\x02ca3',  U'\x00000',  U'\x02ca5',  U'\x00000',  U'\x02ca7',  U'\x00000',
  U'\x02ca9',  U'\x00000',  U'\x02cab',  U'\x00000',  U'\x02cad',  U'\x00000',  U'\x02caf',
  U'\x00000',  U'\x02cb1',  U'\x00000',  U'\x02cb3',  U'\x00000',  U'\x02cb5',  U'\x00000',
  U'\x02cb7',  U'\x00000',  U'\x02cb9',  U'\x00000',  U'\x02cbb',  U'\x00000',  U'\x02cbd',
  U'\x00000',  U'\x02cbf',  U'\x00000',  U'\x02cc1',  U'\x00000',  U'\x02cc3',  U'\x00000',
  U'\x02cc5',  U'\x00000',  U'\x02cc7',  U'\x00000',  U'\x02cc9',  U'\x00000',  U'\x02ccb',
  U'\x00000',  U'\x02ccd',  U'\x00000',  U'\x02ccf',  U'\x00000',  U'\x02cd1',  U'\x00000',
  U'\x02cd3',  U'\x00000',  U'\x02cd5',  U'\x00000',  U'\x02cd7',  U'\x00000',  U'\x02cd9',
  U'\x00000',  U'\x02cdb',  U'\x00000',  U'\x02cdd',  U'\x00000',  U'\x02cdf',  U'\x00000',
  U'\x02ce1',  U'\x00000',  U'\x02ce3',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02cec',  U'\x00000',  U'\x02cee',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02cf3'
};

static constexpr const char32_t LOWER_CASE_0x0a6[]
{
  U'\x0009a',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x0a641',  U'\x00000',  U'\x0a643',  U'\x00000',  U'\x0a645',  U'\x00000',
  U'\x0a647',  U'\x00000',  U'\x0a649',  U'\x00000',  U'\x0a64b',  U'\x00000',  U'\x0a64d',
  U'\x00000',  U'\x0a64f',  U'\x00000',  U'\x0a651',  U'\x00000',  U'\x0a653',  U'\x00000',
  U'\x0a655',  U'\x00000',  U'\x0a657',  U'\x00000',  U'\x0a659',  U'\x00000',  U'\x0a65b',
  U'\x00000',  U'\x0a65d',  U'\x00000',  U'\x0a65f',  U'\x00000',  U'\x0a661',  U'\x00000',
  U'\x0a663',  U'\x00000',  U'\x0a665',  U'\x00000',  U'\x0a667',  U'\x00000',  U'\x0a669',
  U'\x00000',  U'\x0a66b',  U'\x00000',  U'\x0a66d',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x0a681',  U'\x00000',  U'\x0a683',  U'\x00000',  U'\x0a685',
  U'\x00000',  U'\x0a687',  U'\x00000',  U'\x0a689',  U'\x00000',  U'\x0a68b',  U'\x00000',
  U'\x0a68d',  U'\x00000',  U'\x0a68f',  U'\x00000',  U'\x0a691',  U'\x00000',  U'\x0a693',
  U'\x00000',  U'\x0a695',  U'\x00000',  U'\x0a697',  U'\x00000',  U'\x0a699',  U'\x00000',
  U'\x0a69b'
};

static constexpr const char32_t LOWER_CASE_0x0a7[]
{
  U'\x000f5',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a723',
  U'\x00000',  U'\x0a725',  U'\x00000',  U'\x0a727',  U'\x00000',  U'\x0a729',  U'\x00000',
  U'\x0a72b',  U'\x00000',  U'\x0a72d',  U'\x00000',  U'\x0a72f',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x0a733',  U'\x00000',  U'\x0a735',  U'\x00000',  U'\x0a737',  U'\x00000',
  U'\x0a739',  U'\x00000',  U'\x0a73b',  U'\x00000',  U'\x0a73d',  U'\x00000',  U'\x0a73f',
  U'\x00000',  U'\x0a741',  U'\x00000',  U'\x0a743',  U'\x00000',  U'\x0a745',  U'\x00000',
  U'\x0a747',  U'\x00000',  U'\x0a749',  U'\x00000',  U'\x0a74b',  U'\x00000',  U'\x0a74d',
  U'\x00000',  U'\x0a74f',  U'\x00000',  U'\x0a751',  U'\x00000',  U'\x0a753',  U'\x00000',
  U'\x0a755',  U'\x00000',  U'\x0a757',  U'\x00000',  U'\x0a759',  U'\x00000',  U'\x0a75b',
  U'\x00000',  U'\x0a75d',  U'\x00000',  U'\x0a75f',  U'\x00000',  U'\x0a761',  U'\x00000',
  U'\x0a763',  U'\x00000',  U'\x0a765',  U'\x00000',  U'\x0a767',  U'\x00000',  U'\x0a769',
  U'\x00000',  U'\x0a76b',  U'\x00000',  U'\x0a76d',  U'\x00000',  U'\x0a76f',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x0a77a',  U'\x00000',  U'\x0a77c',  U'\x00000',  U'\x01d79',
  U'\x0a77f',  U'\x00000',  U'\x0a781',  U'\x00000',  U'\x0a783',  U'\x00000',  U'\x0a785',
  U'\x00000',  U'\x0a787',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a78c',
  U'\x00000',  U'\x00265',  U'\x00000',  U'\x00000',  U'\x0a791',  U'\x00000',  U'\x0a793',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a797',  U'\x00000',  U'\x0a799',  U'\x00000',
  U'\x0a79b',  U'\x00000',  U'\x0a79d',  U'\x00000',  U'\x0a79f',  U'\x00000',  U'\x0a7a1',
  U'\x00000',  U'\x0a7a3',  U'\x00000',  U'\x0a7a5',  U'\x00000',  U'\x0a7a7',  U'\x00000',
  U'\x0a7a9',  U'\x00000',  U'\x00266',  U'\x0025c',  U'\x00261',  U'\x0026c',  U'\x0026a',
  U'\x00000',  U'\x0029e',  U'\x00287',  U'\x0029d',  U'\x0ab53',  U'\x0a7b5',  U'\x00000',
  U'\x0a7b7',  U'\x00000',  U'\x0a7b9',  U'\x00000',  U'\x0a7bb',  U'\x00000',  U'\x0a7bd',
  U'\x00000',  U'\x0a7bf',  U'\x00000',  U'\x0a7c1',  U'\x00000',  U'\x0a7c3',  U'\x00000',
  U'\x0a794',  U'\x00282',  U'\x01d8e',  U'\x0a7c8',  U'\x00000',  U'\x0a7ca',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7d1',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7d7',  U'\x00000',  U'\x0a7d9',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x0a7f6'
};

static constexpr const char32_t LOWER_CASE_0x0ff[]
{
  U'\x0003a',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0ff41',  U'\x0ff42',
  U'\x0ff43',  U'\x0ff44',  U'\x0ff45',  U'\x0ff46',  U'\x0ff47',  U'\x0ff48',  U'\x0ff49',
  U'\x0ff4a',  U'\x0ff4b',  U'\x0ff4c',  U'\x0ff4d',  U'\x0ff4e',  U'\x0ff4f',  U'\x0ff50',
  U'\x0ff51',  U'\x0ff52',  U'\x0ff53',  U'\x0ff54',  U'\x0ff55',  U'\x0ff56',  U'\x0ff57',
  U'\x0ff58',  U'\x0ff59',  U'\x0ff5a'
};

static constexpr const char32_t LOWER_CASE_0x104[]
{
  U'\x000d3',
  U'\x10428',  U'\x10429',  U'\x1042a',  U'\x1042b',  U'\x1042c',  U'\x1042d',  U'\x1042e',
  U'\x1042f',  U'\x10430',  U'\x10431',  U'\x10432',  U'\x10433',  U'\x10434',  U'\x10435',
  U'\x10436',  U'\x10437',  U'\x10438',  U'\x10439',  U'\x1043a',  U'\x1043b',  U'\x1043c',
  U'\x1043d',  U'\x1043e',  U'\x1043f',  U'\x10440',  U'\x10441',  U'\x10442',  U'\x10443',
  U'\x10444',  U'\x10445',  U'\x10446',  U'\x10447',  U'\x10448',  U'\x10449',  U'\x1044a',
  U'\x1044b',  U'\x1044c',  U'\x1044d',  U'\x1044e',  U'\x1044f',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x104d8',  U'\x104d9',  U'\x104da',  U'\x104db',  U'\x104dc',  U'\x104dd',
  U'\x104de',  U'\x104df',  U'\x104e0',  U'\x104e1',  U'\x104e2',  U'\x104e3',  U'\x104e4',
  U'\x104e5',  U'\x104e6',  U'\x104e7',  U'\x104e8',  U'\x104e9',  U'\x104ea',  U'\x104eb',
  U'\x104ec',  U'\x104ed',  U'\x104ee',  U'\x104ef',  U'\x104f0',  U'\x104f1',  U'\x104f2',
  U'\x104f3',  U'\x104f4',  U'\x104f5',  U'\x104f6',  U'\x104f7',  U'\x104f8',  U'\x104f9',
  U'\x104fa',  U'\x104fb'
};

static constexpr const char32_t LOWER_CASE_0x105[]
{
  U'\x00095',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x10597',  U'\x10598',  U'\x10599',  U'\x1059a',  U'\x1059b',  U'\x1059c',  U'\x1059d',
  U'\x1059e',  U'\x1059f',  U'\x105a0',  U'\x105a1',  U'\x00000',  U'\x105a3',  U'\x105a4',
  U'\x105a5',  U'\x105a6',  U'\x105a7',  U'\x105a8',  U'\x105a9',  U'\x105aa',  U'\x105ab',
  U'\x105ac',  U'\x105ad',  U'\x105ae',  U'\x105af',  U'\x105b0',  U'\x105b1',  U'\x00000',
  U'\x105b3',  U'\x105b4',  U'\x105b5',  U'\x105b6',  U'\x105b7',  U'\x105b8',  U'\x105b9',
  U'\x00000',  U'\x105bb',  U'\x105bc'
};

static constexpr const char32_t LOWER_CASE_0x10c[]
{
  U'\x000b2',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x10cc0',  U'\x10cc1',  U'\x10cc2',  U'\x10cc3',  U'\x10cc4',
  U'\x10cc5',  U'\x10cc6',  U'\x10cc7',  U'\x10cc8',  U'\x10cc9',  U'\x10cca',  U'\x10ccb',
  U'\x10ccc',  U'\x10ccd',  U'\x10cce',  U'\x10ccf',  U'\x10cd0',  U'\x10cd1',  U'\x10cd2',
  U'\x10cd3',  U'\x10cd4',  U'\x10cd5',  U'\x10cd6',  U'\x10cd7',  U'\x10cd8',  U'\x10cd9',
  U'\x10cda',  U'\x10cdb',  U'\x10cdc',  U'\x10cdd',  U'\x10cde',  U'\x10cdf',  U'\x10ce0',
  U'\x10ce1',  U'\x10ce2',  U'\x10ce3',  U'\x10ce4',  U'\x10ce5',  U'\x10ce6',  U'\x10ce7',
  U'\x10ce8',  U'\x10ce9',  U'\x10cea',  U'\x10ceb',  U'\x10cec',  U'\x10ced',  U'\x10cee',
  U'\x10cef',  U'\x10cf0',  U'\x10cf1',  U'\x10cf2'
};

static constexpr const char32_t LOWER_CASE_0x118[]
{
  U'\x000bf',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x118c0',
  U'\x118c1',  U'\x118c2',  U'\x118c3',  U'\x118c4',  U'\x118c5',  U'\x118c6',  U'\x118c7',
  U'\x118c8',  U'\x118c9',  U'\x118ca',  U'\x118cb',  U'\x118cc',  U'\x118cd',  U'\x118ce',
  U'\x118cf',  U'\x118d0',  U'\x118d1',  U'\x118d2',  U'\x118d3',  U'\x118d4',  U'\x118d5',
  U'\x118d6',  U'\x118d7',  U'\x118d8',  U'\x118d9',  U'\x118da',  U'\x118db',  U'\x118dc',
  U'\x118dd',  U'\x118de',  U'\x118df'
};

static constexpr const char32_t LOWER_CASE_0x16e[]
{
  U'\x0005f',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x16e60',  U'\x16e61',  U'\x16e62',  U'\x16e63',  U'\x16e64',  U'\x16e65',
  U'\x16e66',  U'\x16e67',  U'\x16e68',  U'\x16e69',  U'\x16e6a',  U'\x16e6b',  U'\x16e6c',
  U'\x16e6d',  U'\x16e6e',  U'\x16e6f',  U'\x16e70',  U'\x16e71',  U'\x16e72',  U'\x16e73',
  U'\x16e74',  U'\x16e75',  U'\x16e76',  U'\x16e77',  U'\x16e78',  U'\x16e79',  U'\x16e7a',
  U'\x16e7b',  U'\x16e7c',  U'\x16e7d',  U'\x16e7e',  U'\x16e7f'
};

static constexpr const char32_t LOWER_CASE_0x1e9[]
{
  U'\x00021',
  U'\x1e922',  U'\x1e923',  U'\x1e924',  U'\x1e925',  U'\x1e926',  U'\x1e927',  U'\x1e928',
  U'\x1e929',  U'\x1e92a',  U'\x1e92b',  U'\x1e92c',  U'\x1e92d',  U'\x1e92e',  U'\x1e92f',
  U'\x1e930',  U'\x1e931',  U'\x1e932',  U'\x1e933',  U'\x1e934',  U'\x1e935',  U'\x1e936',
  U'\x1e937',  U'\x1e938',  U'\x1e939',  U'\x1e93a',  U'\x1e93b',  U'\x1e93c',  U'\x1e93d',
  U'\x1e93e',  U'\x1e93f',  U'\x1e940',  U'\x1e941',  U'\x1e942',  U'\x1e943'
};

static constexpr const char32_t * const LOWER_CASE_INDEX[]
{
  LOWER_CASE_0x000,  LOWER_CASE_0x001,  LOWER_CASE_0x002,  LOWER_CASE_0x003,  LOWER_CASE_0x004,
  LOWER_CASE_0x005,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  LOWER_CASE_0x010,  0x0,  0x0,  LOWER_CASE_0x013,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  LOWER_CASE_0x01c,  0x0,
  LOWER_CASE_0x01e,  LOWER_CASE_0x01f,  0x0,  LOWER_CASE_0x021,  0x0,
  0x0,  LOWER_CASE_0x024,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  LOWER_CASE_0x02c,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  LOWER_CASE_0x0a6,  LOWER_CASE_0x0a7,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  LOWER_CASE_0x0ff,  0x0,  0x0,  0x0,  0x0,
  LOWER_CASE_0x104,  LOWER_CASE_0x105,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  LOWER_CASE_0x10c,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  LOWER_CASE_0x118,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  LOWER_CASE_0x16e,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  LOWER_CASE_0x1e9
};

static constexpr const char32_t UPPER_CASE_0x000[]
{
  U'\x000ff',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00041',
  U'\x00042',  U'\x00043',  U'\x00044',  U'\x00045',  U'\x00046',  U'\x00047',  U'\x00048',
  U'\x00049',  U'\x0004a',  U'\x0004b',  U'\x0004c',  U'\x0004d',  U'\x0004e',  U'\x0004f',
  U'\x00050',  U'\x00051',  U'\x00052',  U'\x00053',  U'\x00054',  U'\x00055',  U'\x00056',
  U'\x00057',  U'\x00058',  U'\x00059',  U'\x0005a',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0039c',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x000c0',  U'\x000c1',  U'\x000c2',  U'\x000c3',  U'\x000c4',  U'\x000c5',  U'\x000c6',
  U'\x000c7',  U'\x000c8',  U'\x000c9',  U'\x000ca',  U'\x000cb',  U'\x000cc',  U'\x000cd',
  U'\x000ce',  U'\x000cf',  U'\x000d0',  U'\x000d1',  U'\x000d2',  U'\x000d3',  U'\x000d4',
  U'\x000d5',  U'\x000d6',  U'\x00000',  U'\x000d8',  U'\x000d9',  U'\x000da',  U'\x000db',
  U'\x000dc',  U'\x000dd',  U'\x000de',  U'\x00178'
};

static constexpr const char32_t UPPER_CASE_0x001[]
{
  U'\x000ff',
  U'\x00000',  U'\x00100',  U'\x00000',  U'\x00102',  U'\x00000',  U'\x00104',  U'\x00000',
  U'\x00106',  U'\x00000',  U'\x00108',  U'\x00000',  U'\x0010a',  U'\x00000',  U'\x0010c',
  U'\x00000',  U'\x0010e',  U'\x00000',  U'\x00110',  U'\x00000',  U'\x00112',  U'\x00000',
  U'\x00114',  U'\x00000',  U'\x00116',  U'\x00000',  U'\x00118',  U'\x00000',  U'\x0011a',
  U'\x00000',  U'\x0011c',  U'\x00000',  U'\x0011e',  U'\x00000',  U'\x00120',  U'\x00000',
  U'\x00122',  U'\x00000',  U'\x00124',  U'\x00000',  U'\x00126',  U'\x00000',  U'\x00128',
  U'\x00000',  U'\x0012a',  U'\x00000',  U'\x0012c',  U'\x00000',  U'\x0012e',  U'\x00000',
  U'\x00049',  U'\x00000',  U'\x00132',  U'\x00000',  U'\x00134',  U'\x00000',  U'\x00136',
  U'\x00000',  U'\x00000',  U'\x00139',  U'\x00000',  U'\x0013b',  U'\x00000',  U'\x0013d',
  U'\x00000',  U'\x0013f',  U'\x00000',  U'\x00141',  U'\x00000',  U'\x00143',  U'\x00000',
  U'\x00145',  U'\x00000',  U'\x00147',  U'\x00000',  U'\x00000',  U'\x0014a',  U'\x00000',
  U'\x0014c',  U'\x00000',  U'\x0014e',  U'\x00000',  U'\x00150',  U'\x00000',  U'\x00152',
  U'\x00000',  U'\x00154',  U'\x00000',  U'\x00156',  U'\x00000',  U'\x00158',  U'\x00000',
  U'\x0015a',  U'\x00000',  U'\x0015c',  U'\x00000',  U'\x0015e',  U'\x00000',  U'\x00160',
  U'\x00000',  U'\x00162',  U'\x00000',  U'\x00164',  U'\x00000',  U'\x00166',  U'\x00000',
  U'\x00168',  U'\x00000',  U'\x0016a',  U'\x00000',  U'\x0016c',  U'\x00000',  U'\x0016e',
  U'\x00000',  U'\x00170',  U'\x00000',  U'\x00172',  U'\x00000',  U'\x00174',  U'\x00000',
  U'\x00176',  U'\x00000',  U'\x00000',  U'\x00179',  U'\x00000',  U'\x0017b',  U'\x00000',
  U'\x0017d',  U'\x00053',  U'\x00243',  U'\x00000',  U'\x00000',  U'\x00182',  U'\x00000',
  U'\x00184',  U'\x00000',  U'\x00000',  U'\x00187',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x0018b',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00191',
  U'\x00000',  U'\x00000',  U'\x001f6',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00198',
  U'\x0023d',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00220',  U'\x00000',  U'\x00000',
  U'\x001a0',  U'\x00000',  U'\x001a2',  U'\x00000',  U'\x001a4',  U'\x00000',  U'\x00000',
  U'\x001a7',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x001ac',  U'\x00000',
  U'\x00000',  U'\x001af',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x001b3',  U'\x00000',
  U'\x001b5',  U'\x00000',  U'\x00000',  U'\x001b8',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x001bc',  U'\x00000',  U'\x001f7',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x001c4',  U'\x001c4',  U'\x00000',  U'\x001c7',  U'\x001c7',  U'\x00000',
  U'\x001ca',  U'\x001ca',  U'\x00000',  U'\x001cd',  U'\x00000',  U'\x001cf',  U'\x00000',
  U'\x001d1',  U'\x00000',  U'\x001d3',  U'\x00000',  U'\x001d5',  U'\x00000',  U'\x001d7',
  U'\x00000',  U'\x001d9',  U'\x00000',  U'\x001db',  U'\x0018e',  U'\x00000',  U'\x001de',
  U'\x00000',  U'\x001e0',  U'\x00000',  U'\x001e2',  U'\x00000',  U'\x001e4',  U'\x00000',
  U'\x001e6',  U'\x00000',  U'\x001e8',  U'\x00000',  U'\x001ea',  U'\x00000',  U'\x001ec',
  U'\x00000',  U'\x001ee',  U'\x00000',  U'\x00000',  U'\x001f1',  U'\x001f1',  U'\x00000',
  U'\x001f4',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x001f8',  U'\x00000',  U'\x001fa',
  U'\x00000',  U'\x001fc',  U'\x00000',  U'\x001fe'
};

static constexpr const char32_t UPPER_CASE_0x002[]
{
  U'\x0009e',
  U'\x00000',  U'\x00200',  U'\x00000',  U'\x00202',  U'\x00000',  U'\x00204',  U'\x00000',
  U'\x00206',  U'\x00000',  U'\x00208',  U'\x00000',  U'\x0020a',  U'\x00000',  U'\x0020c',
  U'\x00000',  U'\x0020e',  U'\x00000',  U'\x00210',  U'\x00000',  U'\x00212',  U'\x00000',
  U'\x00214',  U'\x00000',  U'\x00216',  U'\x00000',  U'\x00218',  U'\x00000',  U'\x0021a',
  U'\x00000',  U'\x0021c',  U'\x00000',  U'\x0021e',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00222',  U'\x00000',  U'\x00224',  U'\x00000',  U'\x00226',  U'\x00000',  U'\x00228',
  U'\x00000',  U'\x0022a',  U'\x00000',  U'\x0022c',  U'\x00000',  U'\x0022e',  U'\x00000',
  U'\x00230',  U'\x00000',  U'\x00232',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0023b',  U'\x00000',  U'\x00000',
  U'\x02c7e',  U'\x02c7f',  U'\x00000',  U'\x00241',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00246',  U'\x00000',  U'\x00248',  U'\x00000',  U'\x0024a',  U'\x00000',
  U'\x0024c',  U'\x00000',  U'\x0024e',  U'\x02c6f',  U'\x02c6d',  U'\x02c70',  U'\x00181',
  U'\x00186',  U'\x00000',  U'\x00189',  U'\x0018a',  U'\x00000',  U'\x0018f',  U'\x00000',
  U'\x00190',  U'\x0a7ab',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00193',  U'\x0a7ac',
  U'\x00000',  U'\x00194',  U'\x00000',  U'\x0a78d',  U'\x0a7aa',  U'\x00000',  U'\x00197',
  U'\x00196',  U'\x0a7ae',  U'\x02c62',  U'\x0a7ad',  U'\x00000',  U'\x00000',  U'\x0019c',
  U'\x00000',  U'\x02c6e',  U'\x0019d',  U'\x00000',  U'\x00000',  U'\x0019f',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02c64',
  U'\x00000',  U'\x00000',  U'\x001a6',  U'\x00000',  U'\x0a7c5',  U'\x001a9',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x0a7b1',  U'\x001ae',  U'\x00244',  U'\x001b1',  U'\x001b2',
  U'\x00245',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x001b7',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7b2',  U'\x0a7b0'
};

static constexpr const char32_t UPPER_CASE_0x003[]
{
  U'\x000fb',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00399',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00370',  U'\x00000',  U'\x00372',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00376',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003fd',  U'\x003fe',  U'\x003ff',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00386',  U'\x00388',  U'\x00389',
  U'\x0038a',  U'\x00000',  U'\x00391',  U'\x00392',  U'\x00393',  U'\x00394',  U'\x00395',
  U'\x00396',  U'\x00397',  U'\x00398',  U'\x00399',  U'\x0039a',  U'\x0039b',  U'\x0039c',
  U'\x0039d',  U'\x0039e',  U'\x0039f',  U'\x003a0',  U'\x003a1',  U'\x003a3',  U'\x003a3',
  U'\x003a4',  U'\x003a5',  U'\x003a6',  U'\x003a7',  U'\x003a8',  U'\x003a9',  U'\x003aa',
  U'\x003ab',  U'\x0038c',  U'\x0038e',  U'\x0038f',  U'\x00000',  U'\x00392',  U'\x00398',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x003a6',  U'\x003a0',  U'\x003cf',  U'\x00000',
  U'\x003d8',  U'\x00000',  U'\x003da',  U'\x00000',  U'\x003dc',  U'\x00000',  U'\x003de',
  U'\x00000',  U'\x003e0',  U'\x00000',  U'\x003e2',  U'\x00000',  U'\x003e4',  U'\x00000',
  U'\x003e6',  U'\x00000',  U'\x003e8',  U'\x00000',  U'\x003ea',  U'\x00000',  U'\x003ec',
  U'\x00000',  U'\x003ee',  U'\x0039a',  U'\x003a1',  U'\x003f9',  U'\x0037f',  U'\x00000',
  U'\x00395',  U'\x00000',  U'\x00000',  U'\x003f7',  U'\x00000',  U'\x00000',  U'\x003fa'
};

static constexpr const char32_t UPPER_CASE_0x004[]
{
  U'\x000ff',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00410',
  U'\x00411',  U'\x00412',  U'\x00413',  U'\x00414',  U'\x00415',  U'\x00416',  U'\x00417',
  U'\x00418',  U'\x00419',  U'\x0041a',  U'\x0041b',  U'\x0041c',  U'\x0041d',  U'\x0041e',
  U'\x0041f',  U'\x00420',  U'\x00421',  U'\x00422',  U'\x00423',  U'\x00424',  U'\x00425',
  U'\x00426',  U'\x00427',  U'\x00428',  U'\x00429',  U'\x0042a',  U'\x0042b',  U'\x0042c',
  U'\x0042d',  U'\x0042e',  U'\x0042f',  U'\x00400',  U'\x00401',  U'\x00402',  U'\x00403',
  U'\x00404',  U'\x00405',  U'\x00406',  U'\x00407',  U'\x00408',  U'\x00409',  U'\x0040a',
  U'\x0040b',  U'\x0040c',  U'\x0040d',  U'\x0040e',  U'\x0040f',  U'\x00000',  U'\x00460',
  U'\x00000',  U'\x00462',  U'\x00000',  U'\x00464',  U'\x00000',  U'\x00466',  U'\x00000',
  U'\x00468',  U'\x00000',  U'\x0046a',  U'\x00000',  U'\x0046c',  U'\x00000',  U'\x0046e',
  U'\x00000',  U'\x00470',  U'\x00000',  U'\x00472',  U'\x00000',  U'\x00474',  U'\x00000',
  U'\x00476',  U'\x00000',  U'\x00478',  U'\x00000',  U'\x0047a',  U'\x00000',  U'\x0047c',
  U'\x00000',  U'\x0047e',  U'\x00000',  U'\x00480',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0048a',
  U'\x00000',  U'\x0048c',  U'\x00000',  U'\x0048e',  U'\x00000',  U'\x00490',  U'\x00000',
  U'\x00492',  U'\x00000',  U'\x00494',  U'\x00000',  U'\x00496',  U'\x00000',  U'\x00498',
  U'\x00000',  U'\x0049a',  U'\x00000',  U'\x0049c',  U'\x00000',  U'\x0049e',  U'\x00000',
  U'\x004a0',  U'\x00000',  U'\x004a2',  U'\x00000',  U'\x004a4',  U'\x00000',  U'\x004a6',
  U'\x00000',  U'\x004a8',  U'\x00000',  U'\x004aa',  U'\x00000',  U'\x004ac',  U'\x00000',
  U'\x004ae',  U'\x00000',  U'\x004b0',  U'\x00000',  U'\x004b2',  U'\x00000',  U'\x004b4',
  U'\x00000',  U'\x004b6',  U'\x00000',  U'\x004b8',  U'\x00000',  U'\x004ba',  U'\x00000',
  U'\x004bc',  U'\x00000',  U'\x004be',  U'\x00000',  U'\x00000',  U'\x004c1',  U'\x00000',
  U'\x004c3',  U'\x00000',  U'\x004c5',  U'\x00000',  U'\x004c7',  U'\x00000',  U'\x004c9',
  U'\x00000',  U'\x004cb',  U'\x00000',  U'\x004cd',  U'\x004c0',  U'\x00000',  U'\x004d0',
  U'\x00000',  U'\x004d2',  U'\x00000',  U'\x004d4',  U'\x00000',  U'\x004d6',  U'\x00000',
  U'\x004d8',  U'\x00000',  U'\x004da',  U'\x00000',  U'\x004dc',  U'\x00000',  U'\x004de',
  U'\x00000',  U'\x004e0',  U'\x00000',  U'\x004e2',  U'\x00000',  U'\x004e4',  U'\x00000',
  U'\x004e6',  U'\x00000',  U'\x004e8',  U'\x00000',  U'\x004ea',  U'\x00000',  U'\x004ec',
  U'\x00000',  U'\x004ee',  U'\x00000',  U'\x004f0',  U'\x00000',  U'\x004f2',  U'\x00000',
  U'\x004f4',  U'\x00000',  U'\x004f6',  U'\x00000',  U'\x004f8',  U'\x00000',  U'\x004fa',
  U'\x00000',  U'\x004fc',  U'\x00000',  U'\x004fe'
};

static constexpr const char32_t UPPER_CASE_0x005[]
{
  U'\x00086',
  U'\x00000',  U'\x00500',  U'\x00000',  U'\x00502',  U'\x00000',  U'\x00504',  U'\x00000',
  U'\x00506',  U'\x00000',  U'\x00508',  U'\x00000',  U'\x0050a',  U'\x00000',  U'\x0050c',
  U'\x00000',  U'\x0050e',  U'\x00000',  U'\x00510',  U'\x00000',  U'\x00512',  U'\x00000',
  U'\x00514',  U'\x00000',  U'\x00516',  U'\x00000',  U'\x00518',  U'\x00000',  U'\x0051a',
  U'\x00000',  U'\x0051c',  U'\x00000',  U'\x0051e',  U'\x00000',  U'\x00520',  U'\x00000',
  U'\x00522',  U'\x00000',  U'\x00524',  U'\x00000',  U'\x00526',  U'\x00000',  U'\x00528',
  U'\x00000',  U'\x0052a',  U'\x00000',  U'\x0052c',  U'\x00000',  U'\x0052e',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00531',
  U'\x00532',  U'\x00533',  U'\x00534',  U'\x00535',  U'\x00536',  U'\x00537',  U'\x00538',
  U'\x00539',  U'\x0053a',  U'\x0053b',  U'\x0053c',  U'\x0053d',  U'\x0053e',  U'\x0053f',
  U'\x00540',  U'\x00541',  U'\x00542',  U'\x00543',  U'\x00544',  U'\x00545',  U'\x00546',
  U'\x00547',  U'\x00548',  U'\x00549',  U'\x0054a',  U'\x0054b',  U'\x0054c',  U'\x0054d',
  U'\x0054e',  U'\x0054f',  U'\x00550',  U'\x00551',  U'\x00552',  U'\x00553',  U'\x00554',
  U'\x00555',  U'\x00556'
};

static constexpr const char32_t UPPER_CASE_0x010[]
{
  U'\x000ff',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01c90',  U'\x01c91',
  U'\x01c92',  U'\x01c93',  U'\x01c94',  U'\x01c95',  U'\x01c96',  U'\x01c97',  U'\x01c98',
  U'\x01c99',  U'\x01c9a',  U'\x01c9b',  U'\x01c9c',  U'\x01c9d',  U'\x01c9e',  U'\x01c9f',
  U'\x01ca0',  U'\x01ca1',  U'\x01ca2',  U'\x01ca3',  U'\x01ca4',  U'\x01ca5',  U'\x01ca6',
  U'\x01ca7',  U'\x01ca8',  U'\x01ca9',  U'\x01caa',  U'\x01cab',  U'\x01cac',  U'\x01cad',
  U'\x01cae',  U'\x01caf',  U'\x01cb0',  U'\x01cb1',  U'\x01cb2',  U'\x01cb3',  U'\x01cb4',
  U'\x01cb5',  U'\x01cb6',  U'\x01cb7',  U'\x01cb8',  U'\x01cb9',  U'\x01cba',  U'\x00000',
  U'\x00000',  U'\x01cbd',  U'\x01cbe',  U'\x01cbf'
};

static constexpr const char32_t UPPER_CASE_0x013[]
{
  U'\x000fd',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x013f0',  U'\x013f1',  U'\x013f2',  U'\x013f3',
  U'\x013f4',  U'\x013f5'
};

static constexpr const char32_t UPPER_CASE_0x01c[]
{
  U'\x00088',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00412',  U'\x00414',  U'\x0041e',  U'\x00421',  U'\x00422',
  U'\x00422',  U'\x0042a',  U'\x00462',  U'\x0a64a'
};

static constexpr const char32_t UPPER_CASE_0x01d[]
{
  U'\x0008e',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x0a77d',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02c63',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x0a7c6'
};

static constexpr const char32_t UPPER_CASE_0x01e[]
{
  U'\x000ff',
  U'\x00000',  U'\x01e00',  U'\x00000',  U'\x01e02',  U'\x00000',  U'\x01e04',  U'\x00000',
  U'\x01e06',  U'\x00000',  U'\x01e08',  U'\x00000',  U'\x01e0a',  U'\x00000',  U'\x01e0c',
  U'\x00000',  U'\x01e0e',  U'\x00000',  U'\x01e10',  U'\x00000',  U'\x01e12',  U'\x00000',
  U'\x01e14',  U'\x00000',  U'\x01e16',  U'\x00000',  U'\x01e18',  U'\x00000',  U'\x01e1a',
  U'\x00000',  U'\x01e1c',  U'\x00000',  U'\x01e1e',  U'\x00000',  U'\x01e20',  U'\x00000',
  U'\x01e22',  U'\x00000',  U'\x01e24',  U'\x00000',  U'\x01e26',  U'\x00000',  U'\x01e28',
  U'\x00000',  U'\x01e2a',  U'\x00000',  U'\x01e2c',  U'\x00000',  U'\x01e2e',  U'\x00000',
  U'\x01e30',  U'\x00000',  U'\x01e32',  U'\x00000',  U'\x01e34',  U'\x00000',  U'\x01e36',
  U'\x00000',  U'\x01e38',  U'\x00000',  U'\x01e3a',  U'\x00000',  U'\x01e3c',  U'\x00000',
  U'\x01e3e',  U'\x00000',  U'\x01e40',  U'\x00000',  U'\x01e42',  U'\x00000',  U'\x01e44',
  U'\x00000',  U'\x01e46',  U'\x00000',  U'\x01e48',  U'\x00000',  U'\x01e4a',  U'\x00000',
  U'\x01e4c',  U'\x00000',  U'\x01e4e',  U'\x00000',  U'\x01e50',  U'\x00000',  U'\x01e52',
  U'\x00000',  U'\x01e54',  U'\x00000',  U'\x01e56',  U'\x00000',  U'\x01e58',  U'\x00000',
  U'\x01e5a',  U'\x00000',  U'\x01e5c',  U'\x00000',  U'\x01e5e',  U'\x00000',  U'\x01e60',
  U'\x00000',  U'\x01e62',  U'\x00000',  U'\x01e64',  U'\x00000',  U'\x01e66',  U'\x00000',
  U'\x01e68',  U'\x00000',  U'\x01e6a',  U'\x00000',  U'\x01e6c',  U'\x00000',  U'\x01e6e',
  U'\x00000',  U'\x01e70',  U'\x00000',  U'\x01e72',  U'\x00000',  U'\x01e74',  U'\x00000',
  U'\x01e76',  U'\x00000',  U'\x01e78',  U'\x00000',  U'\x01e7a',  U'\x00000',  U'\x01e7c',
  U'\x00000',  U'\x01e7e',  U'\x00000',  U'\x01e80',  U'\x00000',  U'\x01e82',  U'\x00000',
  U'\x01e84',  U'\x00000',  U'\x01e86',  U'\x00000',  U'\x01e88',  U'\x00000',  U'\x01e8a',
  U'\x00000',  U'\x01e8c',  U'\x00000',  U'\x01e8e',  U'\x00000',  U'\x01e90',  U'\x00000',
  U'\x01e92',  U'\x00000',  U'\x01e94',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x01e60',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x01ea0',  U'\x00000',  U'\x01ea2',  U'\x00000',  U'\x01ea4',  U'\x00000',  U'\x01ea6',
  U'\x00000',  U'\x01ea8',  U'\x00000',  U'\x01eaa',  U'\x00000',  U'\x01eac',  U'\x00000',
  U'\x01eae',  U'\x00000',  U'\x01eb0',  U'\x00000',  U'\x01eb2',  U'\x00000',  U'\x01eb4',
  U'\x00000',  U'\x01eb6',  U'\x00000',  U'\x01eb8',  U'\x00000',  U'\x01eba',  U'\x00000',
  U'\x01ebc',  U'\x00000',  U'\x01ebe',  U'\x00000',  U'\x01ec0',  U'\x00000',  U'\x01ec2',
  U'\x00000',  U'\x01ec4',  U'\x00000',  U'\x01ec6',  U'\x00000',  U'\x01ec8',  U'\x00000',
  U'\x01eca',  U'\x00000',  U'\x01ecc',  U'\x00000',  U'\x01ece',  U'\x00000',  U'\x01ed0',
  U'\x00000',  U'\x01ed2',  U'\x00000',  U'\x01ed4',  U'\x00000',  U'\x01ed6',  U'\x00000',
  U'\x01ed8',  U'\x00000',  U'\x01eda',  U'\x00000',  U'\x01edc',  U'\x00000',  U'\x01ede',
  U'\x00000',  U'\x01ee0',  U'\x00000',  U'\x01ee2',  U'\x00000',  U'\x01ee4',  U'\x00000',
  U'\x01ee6',  U'\x00000',  U'\x01ee8',  U'\x00000',  U'\x01eea',  U'\x00000',  U'\x01eec',
  U'\x00000',  U'\x01eee',  U'\x00000',  U'\x01ef0',  U'\x00000',  U'\x01ef2',  U'\x00000',
  U'\x01ef4',  U'\x00000',  U'\x01ef6',  U'\x00000',  U'\x01ef8',  U'\x00000',  U'\x01efa',
  U'\x00000',  U'\x01efc',  U'\x00000',  U'\x01efe'
};

static constexpr const char32_t UPPER_CASE_0x01f[]
{
  U'\x000f3',
  U'\x01f08',  U'\x01f09',  U'\x01f0a',  U'\x01f0b',  U'\x01f0c',  U'\x01f0d',  U'\x01f0e',
  U'\x01f0f',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x01f18',  U'\x01f19',  U'\x01f1a',  U'\x01f1b',  U'\x01f1c',
  U'\x01f1d',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f28',  U'\x01f29',  U'\x01f2a',
  U'\x01f2b',  U'\x01f2c',  U'\x01f2d',  U'\x01f2e',  U'\x01f2f',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f38',
  U'\x01f39',  U'\x01f3a',  U'\x01f3b',  U'\x01f3c',  U'\x01f3d',  U'\x01f3e',  U'\x01f3f',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x01f48',  U'\x01f49',  U'\x01f4a',  U'\x01f4b',  U'\x01f4c',  U'\x01f4d',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f59',  U'\x00000',  U'\x01f5b',
  U'\x00000',  U'\x01f5d',  U'\x00000',  U'\x01f5f',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f68',  U'\x01f69',
  U'\x01f6a',  U'\x01f6b',  U'\x01f6c',  U'\x01f6d',  U'\x01f6e',  U'\x01f6f',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x01fba',  U'\x01fbb',  U'\x01fc8',  U'\x01fc9',  U'\x01fca',  U'\x01fcb',  U'\x01fda',
  U'\x01fdb',  U'\x01ff8',  U'\x01ff9',  U'\x01fea',  U'\x01feb',  U'\x01ffa',  U'\x01ffb',
  U'\x00000',  U'\x00000',  U'\x01f88',  U'\x01f89',  U'\x01f8a',  U'\x01f8b',  U'\x01f8c',
  U'\x01f8d',  U'\x01f8e',  U'\x01f8f',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01f98',  U'\x01f99',  U'\x01f9a',
  U'\x01f9b',  U'\x01f9c',  U'\x01f9d',  U'\x01f9e',  U'\x01f9f',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01fa8',
  U'\x01fa9',  U'\x01faa',  U'\x01fab',  U'\x01fac',  U'\x01fad',  U'\x01fae',  U'\x01faf',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x01fb8',  U'\x01fb9',  U'\x00000',  U'\x01fbc',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00399',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01fcc',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01fd8',  U'\x01fd9',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x01fe8',  U'\x01fe9',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01fec',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x01ffc'
};

static constexpr const char32_t UPPER_CASE_0x021[]
{
  U'\x00084',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x02132',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x02160',  U'\x02161',  U'\x02162',  U'\x02163',  U'\x02164',  U'\x02165',  U'\x02166',
  U'\x02167',  U'\x02168',  U'\x02169',  U'\x0216a',  U'\x0216b',  U'\x0216c',  U'\x0216d',
  U'\x0216e',  U'\x0216f',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02183'
};

static constexpr const char32_t UPPER_CASE_0x024[]
{
  U'\x000e9',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x024b6',  U'\x024b7',
  U'\x024b8',  U'\x024b9',  U'\x024ba',  U'\x024bb',  U'\x024bc',  U'\x024bd',  U'\x024be',
  U'\x024bf',  U'\x024c0',  U'\x024c1',  U'\x024c2',  U'\x024c3',  U'\x024c4',  U'\x024c5',
  U'\x024c6',  U'\x024c7',  U'\x024c8',  U'\x024c9',  U'\x024ca',  U'\x024cb',  U'\x024cc',
  U'\x024cd',  U'\x024ce',  U'\x024cf'
};

static constexpr const char32_t UPPER_CASE_0x02c[]
{
  U'\x000f3',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02c00',
  U'\x02c01',  U'\x02c02',  U'\x02c03',  U'\x02c04',  U'\x02c05',  U'\x02c06',  U'\x02c07',
  U'\x02c08',  U'\x02c09',  U'\x02c0a',  U'\x02c0b',  U'\x02c0c',  U'\x02c0d',  U'\x02c0e',
  U'\x02c0f',  U'\x02c10',  U'\x02c11',  U'\x02c12',  U'\x02c13',  U'\x02c14',  U'\x02c15',
  U'\x02c16',  U'\x02c17',  U'\x02c18',  U'\x02c19',  U'\x02c1a',  U'\x02c1b',  U'\x02c1c',
  U'\x02c1d',  U'\x02c1e',  U'\x02c1f',  U'\x02c20',  U'\x02c21',  U'\x02c22',  U'\x02c23',
  U'\x02c24',  U'\x02c25',  U'\x02c26',  U'\x02c27',  U'\x02c28',  U'\x02c29',  U'\x02c2a',
  U'\x02c2b',  U'\x02c2c',  U'\x02c2d',  U'\x02c2e',  U'\x02c2f',  U'\x00000',  U'\x02c60',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0023a',  U'\x0023e',  U'\x00000',  U'\x02c67',
  U'\x00000',  U'\x02c69',  U'\x00000',  U'\x02c6b',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02c72',  U'\x00000',  U'\x00000',  U'\x02c75',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02c80',  U'\x00000',  U'\x02c82',  U'\x00000',
  U'\x02c84',  U'\x00000',  U'\x02c86',  U'\x00000',  U'\x02c88',  U'\x00000',  U'\x02c8a',
  U'\x00000',  U'\x02c8c',  U'\x00000',  U'\x02c8e',  U'\x00000',  U'\x02c90',  U'\x00000',
  U'\x02c92',  U'\x00000',  U'\x02c94',  U'\x00000',  U'\x02c96',  U'\x00000',  U'\x02c98',
  U'\x00000',  U'\x02c9a',  U'\x00000',  U'\x02c9c',  U'\x00000',  U'\x02c9e',  U'\x00000',
  U'\x02ca0',  U'\x00000',  U'\x02ca2',  U'\x00000',  U'\x02ca4',  U'\x00000',  U'\x02ca6',
  U'\x00000',  U'\x02ca8',  U'\x00000',  U'\x02caa',  U'\x00000',  U'\x02cac',  U'\x00000',
  U'\x02cae',  U'\x00000',  U'\x02cb0',  U'\x00000',  U'\x02cb2',  U'\x00000',  U'\x02cb4',
  U'\x00000',  U'\x02cb6',  U'\x00000',  U'\x02cb8',  U'\x00000',  U'\x02cba',  U'\x00000',
  U'\x02cbc',  U'\x00000',  U'\x02cbe',  U'\x00000',  U'\x02cc0',  U'\x00000',  U'\x02cc2',
  U'\x00000',  U'\x02cc4',  U'\x00000',  U'\x02cc6',  U'\x00000',  U'\x02cc8',  U'\x00000',
  U'\x02cca',  U'\x00000',  U'\x02ccc',  U'\x00000',  U'\x02cce',  U'\x00000',  U'\x02cd0',
  U'\x00000',  U'\x02cd2',  U'\x00000',  U'\x02cd4',  U'\x00000',  U'\x02cd6',  U'\x00000',
  U'\x02cd8',  U'\x00000',  U'\x02cda',  U'\x00000',  U'\x02cdc',  U'\x00000',  U'\x02cde',
  U'\x00000',  U'\x02ce0',  U'\x00000',  U'\x02ce2',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02ceb',  U'\x00000',
  U'\x02ced',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x02cf2'
};

static constexpr const char32_t UPPER_CASE_0x02d[]
{
  U'\x0002d',
  U'\x010a0',  U'\x010a1',  U'\x010a2',  U'\x010a3',  U'\x010a4',  U'\x010a5',  U'\x010a6',
  U'\x010a7',  U'\x010a8',  U'\x010a9',  U'\x010aa',  U'\x010ab',  U'\x010ac',  U'\x010ad',
  U'\x010ae',  U'\x010af',  U'\x010b0',  U'\x010b1',  U'\x010b2',  U'\x010b3',  U'\x010b4',
  U'\x010b5',  U'\x010b6',  U'\x010b7',  U'\x010b8',  U'\x010b9',  U'\x010ba',  U'\x010bb',
  U'\x010bc',  U'\x010bd',  U'\x010be',  U'\x010bf',  U'\x010c0',  U'\x010c1',  U'\x010c2',
  U'\x010c3',  U'\x010c4',  U'\x010c5',  U'\x00000',  U'\x010c7',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x010cd'
};

static constexpr const char32_t UPPER_CASE_0x0a6[]
{
  U'\x0009b',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x0a640',  U'\x00000',  U'\x0a642',  U'\x00000',  U'\x0a644',
  U'\x00000',  U'\x0a646',  U'\x00000',  U'\x0a648',  U'\x00000',  U'\x0a64a',  U'\x00000',
  U'\x0a64c',  U'\x00000',  U'\x0a64e',  U'\x00000',  U'\x0a650',  U'\x00000',  U'\x0a652',
  U'\x00000',  U'\x0a654',  U'\x00000',  U'\x0a656',  U'\x00000',  U'\x0a658',  U'\x00000',
  U'\x0a65a',  U'\x00000',  U'\x0a65c',  U'\x00000',  U'\x0a65e',  U'\x00000',  U'\x0a660',
  U'\x00000',  U'\x0a662',  U'\x00000',  U'\x0a664',  U'\x00000',  U'\x0a666',  U'\x00000',
  U'\x0a668',  U'\x00000',  U'\x0a66a',  U'\x00000',  U'\x0a66c',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a680',  U'\x00000',  U'\x0a682',  U'\x00000',
  U'\x0a684',  U'\x00000',  U'\x0a686',  U'\x00000',  U'\x0a688',  U'\x00000',  U'\x0a68a',
  U'\x00000',  U'\x0a68c',  U'\x00000',  U'\x0a68e',  U'\x00000',  U'\x0a690',  U'\x00000',
  U'\x0a692',  U'\x00000',  U'\x0a694',  U'\x00000',  U'\x0a696',  U'\x00000',  U'\x0a698',
  U'\x00000',  U'\x0a69a'
};

static constexpr const char32_t UPPER_CASE_0x0a7[]
{
  U'\x000f6',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x0a722',  U'\x00000',  U'\x0a724',  U'\x00000',  U'\x0a726',  U'\x00000',  U'\x0a728',
  U'\x00000',  U'\x0a72a',  U'\x00000',  U'\x0a72c',  U'\x00000',  U'\x0a72e',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x0a732',  U'\x00000',  U'\x0a734',  U'\x00000',  U'\x0a736',
  U'\x00000',  U'\x0a738',  U'\x00000',  U'\x0a73a',  U'\x00000',  U'\x0a73c',  U'\x00000',
  U'\x0a73e',  U'\x00000',  U'\x0a740',  U'\x00000',  U'\x0a742',  U'\x00000',  U'\x0a744',
  U'\x00000',  U'\x0a746',  U'\x00000',  U'\x0a748',  U'\x00000',  U'\x0a74a',  U'\x00000',
  U'\x0a74c',  U'\x00000',  U'\x0a74e',  U'\x00000',  U'\x0a750',  U'\x00000',  U'\x0a752',
  U'\x00000',  U'\x0a754',  U'\x00000',  U'\x0a756',  U'\x00000',  U'\x0a758',  U'\x00000',
  U'\x0a75a',  U'\x00000',  U'\x0a75c',  U'\x00000',  U'\x0a75e',  U'\x00000',  U'\x0a760',
  U'\x00000',  U'\x0a762',  U'\x00000',  U'\x0a764',  U'\x00000',  U'\x0a766',  U'\x00000',
  U'\x0a768',  U'\x00000',  U'\x0a76a',  U'\x00000',  U'\x0a76c',  U'\x00000',  U'\x0a76e',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a779',  U'\x00000',  U'\x0a77b',  U'\x00000',
  U'\x00000',  U'\x0a77e',  U'\x00000',  U'\x0a780',  U'\x00000',  U'\x0a782',  U'\x00000',
  U'\x0a784',  U'\x00000',  U'\x0a786',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x0a78b',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a790',  U'\x00000',
  U'\x0a792',  U'\x0a7c4',  U'\x00000',  U'\x00000',  U'\x0a796',  U'\x00000',  U'\x0a798',
  U'\x00000',  U'\x0a79a',  U'\x00000',  U'\x0a79c',  U'\x00000',  U'\x0a79e',  U'\x00000',
  U'\x0a7a0',  U'\x00000',  U'\x0a7a2',  U'\x00000',  U'\x0a7a4',  U'\x00000',  U'\x0a7a6',
  U'\x00000',  U'\x0a7a8',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7b4',
  U'\x00000',  U'\x0a7b6',  U'\x00000',  U'\x0a7b8',  U'\x00000',  U'\x0a7ba',  U'\x00000',
  U'\x0a7bc',  U'\x00000',  U'\x0a7be',  U'\x00000',  U'\x0a7c0',  U'\x00000',  U'\x0a7c2',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7c7',  U'\x00000',  U'\x0a7c9',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7d0',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7d6',  U'\x00000',
  U'\x0a7d8',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x0a7f5'
};

static constexpr const char32_t UPPER_CASE_0x0ab[]
{
  U'\x000bf',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x0a7b3',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x013a0',  U'\x013a1',  U'\x013a2',  U'\x013a3',  U'\x013a4',  U'\x013a5',  U'\x013a6',
  U'\x013a7',  U'\x013a8',  U'\x013a9',  U'\x013aa',  U'\x013ab',  U'\x013ac',  U'\x013ad',
  U'\x013ae',  U'\x013af',  U'\x013b0',  U'\x013b1',  U'\x013b2',  U'\x013b3',  U'\x013b4',
  U'\x013b5',  U'\x013b6',  U'\x013b7',  U'\x013b8',  U'\x013b9',  U'\x013ba',  U'\x013bb',
  U'\x013bc',  U'\x013bd',  U'\x013be',  U'\x013bf',  U'\x013c0',  U'\x013c1',  U'\x013c2',
  U'\x013c3',  U'\x013c4',  U'\x013c5',  U'\x013c6',  U'\x013c7',  U'\x013c8',  U'\x013c9',
  U'\x013ca',  U'\x013cb',  U'\x013cc',  U'\x013cd',  U'\x013ce',  U'\x013cf',  U'\x013d0',
  U'\x013d1',  U'\x013d2',  U'\x013d3',  U'\x013d4',  U'\x013d5',  U'\x013d6',  U'\x013d7',
  U'\x013d8',  U'\x013d9',  U'\x013da',  U'\x013db',  U'\x013dc',  U'\x013dd',  U'\x013de',
  U'\x013df',  U'\x013e0',  U'\x013e1',  U'\x013e2',  U'\x013e3',  U'\x013e4',  U'\x013e5',
  U'\x013e6',  U'\x013e7',  U'\x013e8',  U'\x013e9',  U'\x013ea',  U'\x013eb',  U'\x013ec',
  U'\x013ed',  U'\x013ee',  U'\x013ef'
};

static constexpr const char32_t UPPER_CASE_0x0ff[]
{
  U'\x0005a',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x0ff21',  U'\x0ff22',  U'\x0ff23',  U'\x0ff24',  U'\x0ff25',
  U'\x0ff26',  U'\x0ff27',  U'\x0ff28',  U'\x0ff29',  U'\x0ff2a',  U'\x0ff2b',  U'\x0ff2c',
  U'\x0ff2d',  U'\x0ff2e',  U'\x0ff2f',  U'\x0ff30',  U'\x0ff31',  U'\x0ff32',  U'\x0ff33',
  U'\x0ff34',  U'\x0ff35',  U'\x0ff36',  U'\x0ff37',  U'\x0ff38',  U'\x0ff39',  U'\x0ff3a'
};

static constexpr const char32_t UPPER_CASE_0x104[]
{
  U'\x000fb',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x10400',  U'\x10401',
  U'\x10402',  U'\x10403',  U'\x10404',  U'\x10405',  U'\x10406',  U'\x10407',  U'\x10408',
  U'\x10409',  U'\x1040a',  U'\x1040b',  U'\x1040c',  U'\x1040d',  U'\x1040e',  U'\x1040f',
  U'\x10410',  U'\x10411',  U'\x10412',  U'\x10413',  U'\x10414',  U'\x10415',  U'\x10416',
  U'\x10417',  U'\x10418',  U'\x10419',  U'\x1041a',  U'\x1041b',  U'\x1041c',  U'\x1041d',
  U'\x1041e',  U'\x1041f',  U'\x10420',  U'\x10421',  U'\x10422',  U'\x10423',  U'\x10424',
  U'\x10425',  U'\x10426',  U'\x10427',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x104b0',
  U'\x104b1',  U'\x104b2',  U'\x104b3',  U'\x104b4',  U'\x104b5',  U'\x104b6',  U'\x104b7',
  U'\x104b8',  U'\x104b9',  U'\x104ba',  U'\x104bb',  U'\x104bc',  U'\x104bd',  U'\x104be',
  U'\x104bf',  U'\x104c0',  U'\x104c1',  U'\x104c2',  U'\x104c3',  U'\x104c4',  U'\x104c5',
  U'\x104c6',  U'\x104c7',  U'\x104c8',  U'\x104c9',  U'\x104ca',  U'\x104cb',  U'\x104cc',
  U'\x104cd',  U'\x104ce',  U'\x104cf',  U'\x104d0',  U'\x104d1',  U'\x104d2',  U'\x104d3'
};

static constexpr const char32_t UPPER_CASE_0x105[]
{
  U'\x000bc',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x10570',  U'\x10571',  U'\x10572',
  U'\x10573',  U'\x10574',  U'\x10575',  U'\x10576',  U'\x10577',  U'\x10578',  U'\x10579',
  U'\x1057a',  U'\x00000',  U'\x1057c',  U'\x1057d',  U'\x1057e',  U'\x1057f',  U'\x10580',
  U'\x10581',  U'\x10582',  U'\x10583',  U'\x10584',  U'\x10585',  U'\x10586',  U'\x10587',
  U'\x10588',  U'\x10589',  U'\x1058a',  U'\x00000',  U'\x1058c',  U'\x1058d',  U'\x1058e',
  U'\x1058f',  U'\x10590',  U'\x10591',  U'\x10592',  U'\x00000',  U'\x10594',  U'\x10595'
};

static constexpr const char32_t UPPER_CASE_0x10c[]
{
  U'\x000f2',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x10c80',  U'\x10c81',  U'\x10c82',  U'\x10c83',
  U'\x10c84',  U'\x10c85',  U'\x10c86',  U'\x10c87',  U'\x10c88',  U'\x10c89',  U'\x10c8a',
  U'\x10c8b',  U'\x10c8c',  U'\x10c8d',  U'\x10c8e',  U'\x10c8f',  U'\x10c90',  U'\x10c91',
  U'\x10c92',  U'\x10c93',  U'\x10c94',  U'\x10c95',  U'\x10c96',  U'\x10c97',  U'\x10c98',
  U'\x10c99',  U'\x10c9a',  U'\x10c9b',  U'\x10c9c',  U'\x10c9d',  U'\x10c9e',  U'\x10c9f',
  U'\x10ca0',  U'\x10ca1',  U'\x10ca2',  U'\x10ca3',  U'\x10ca4',  U'\x10ca5',  U'\x10ca6',
  U'\x10ca7',  U'\x10ca8',  U'\x10ca9',  U'\x10caa',  U'\x10cab',  U'\x10cac',  U'\x10cad',
  U'\x10cae',  U'\x10caf',  U'\x10cb0',  U'\x10cb1',  U'\x10cb2'
};

static constexpr const char32_t UPPER_CASE_0x118[]
{
  U'\x000df',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x118a0',  U'\x118a1',  U'\x118a2',  U'\x118a3',
  U'\x118a4',  U'\x118a5',  U'\x118a6',  U'\x118a7',  U'\x118a8',  U'\x118a9',  U'\x118aa',
  U'\x118ab',  U'\x118ac',  U'\x118ad',  U'\x118ae',  U'\x118af',  U'\x118b0',  U'\x118b1',
  U'\x118b2',  U'\x118b3',  U'\x118b4',  U'\x118b5',  U'\x118b6',  U'\x118b7',  U'\x118b8',
  U'\x118b9',  U'\x118ba',  U'\x118bb',  U'\x118bc',  U'\x118bd',  U'\x118be',  U'\x118bf'
};

static constexpr const char32_t UPPER_CASE_0x16e[]
{
  U'\x0007f',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x16e40',  U'\x16e41',
  U'\x16e42',  U'\x16e43',  U'\x16e44',  U'\x16e45',  U'\x16e46',  U'\x16e47',  U'\x16e48',
  U'\x16e49',  U'\x16e4a',  U'\x16e4b',  U'\x16e4c',  U'\x16e4d',  U'\x16e4e',  U'\x16e4f',
  U'\x16e50',  U'\x16e51',  U'\x16e52',  U'\x16e53',  U'\x16e54',  U'\x16e55',  U'\x16e56',
  U'\x16e57',  U'\x16e58',  U'\x16e59',  U'\x16e5a',  U'\x16e5b',  U'\x16e5c',  U'\x16e5d',
  U'\x16e5e',  U'\x16e5f'
};

static constexpr const char32_t UPPER_CASE_0x1e9[]
{
  U'\x00043',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',
  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x00000',  U'\x1e900',
  U'\x1e901',  U'\x1e902',  U'\x1e903',  U'\x1e904',  U'\x1e905',  U'\x1e906',  U'\x1e907',
  U'\x1e908',  U'\x1e909',  U'\x1e90a',  U'\x1e90b',  U'\x1e90c',  U'\x1e90d',  U'\x1e90e',
  U'\x1e90f',  U'\x1e910',  U'\x1e911',  U'\x1e912',  U'\x1e913',  U'\x1e914',  U'\x1e915',
  U'\x1e916',  U'\x1e917',  U'\x1e918',  U'\x1e919',  U'\x1e91a',  U'\x1e91b',  U'\x1e91c',
  U'\x1e91d',  U'\x1e91e',  U'\x1e91f',  U'\x1e920',  U'\x1e921'
};

static constexpr const char32_t * const UPPER_CASE_INDEX[]
{
  UPPER_CASE_0x000,  UPPER_CASE_0x001,  UPPER_CASE_0x002,  UPPER_CASE_0x003,  UPPER_CASE_0x004,
  UPPER_CASE_0x005,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  UPPER_CASE_0x010,  0x0,  0x0,  UPPER_CASE_0x013,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  UPPER_CASE_0x01c,  UPPER_CASE_0x01d,
  UPPER_CASE_0x01e,  UPPER_CASE_0x01f,  0x0,  UPPER_CASE_0x021,  0x0,
  0x0,  UPPER_CASE_0x024,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  UPPER_CASE_0x02c,
  UPPER_CASE_0x02d,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  UPPER_CASE_0x0a6,  UPPER_CASE_0x0a7,  0x0,  0x0,
  0x0,  UPPER_CASE_0x0ab,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  UPPER_CASE_0x0ff,  0x0,  0x0,  0x0,  0x0,
  UPPER_CASE_0x104,  UPPER_CASE_0x105,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  UPPER_CASE_0x10c,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  UPPER_CASE_0x118,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  UPPER_CASE_0x16e,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  0x0,
  0x0,  0x0,  0x0,  0x0,  UPPER_CASE_0x1e9
};

// clang-format on

static constexpr const char32_t MAX_FOLD_HIGH_BYTES =
    (sizeof(FOLDCASE_INDEX) / sizeof(char32_t*)) - 1;

/*!
 * \brief Folds the case of the given Unicode (32-bit) character
 *
 * Performs "simple" case folding using data from Unicode Inc.'
 * Read the description preceding the tables below (FOLDCASE_0000)
 *
 * \param c UTF32 codepoint to fold
 * \return folded UTF32 codepoint
 */

char32_t FoldCaseChar(const char32_t c)
{
  const char32_t high_bytes = c >> 8;
  if (high_bytes > MAX_FOLD_HIGH_BYTES)
    return c;
  const char32_t* table = FOLDCASE_INDEX[high_bytes];
  if (table == 0) // Not case folded
    return c;

  const uint16_t low_byte = c & 0xFF;
  // First table entry is # entries.
  // Entries are in table[1...n + 1], NOT [0..n]
  if (low_byte >= table[0])
    return c; // Not case folded

  const char32_t foldedChar = table[low_byte + 1];
  if (foldedChar == 0)
    return c; // Not case folded

  return foldedChar;
}

static constexpr size_t LOWER_CASE_INDEX_LENGTH =
    (sizeof(LOWER_CASE_INDEX) / sizeof(char32_t*)) - 1;
static constexpr size_t UPPER_CASE_INDEX_LENGTH =
    (sizeof(UPPER_CASE_INDEX) / sizeof(char32_t*)) - 1;

char32_t ToUpperUnicode(const char32_t c)
{
  const char32_t high_bytes = c >> 8;
  if (high_bytes > UPPER_CASE_INDEX_LENGTH)
    return c;

  const char32_t* table = UPPER_CASE_INDEX[high_bytes];
  if (table == 0) // Not case folded
    return c;

  const uint16_t low_byte = c & 0xFF;
  // First table entry is # entries.
  // Entries are in table[1...n + 1], NOT [0..n]
  if (low_byte >= table[0])
    return c; // Not case folded

  const char32_t upperChar = table[low_byte + 1];
  if (upperChar == 0)
    return c; // No change in case

  return upperChar;
}

char32_t ToLowerUnicode(const char32_t c)
{
  const char32_t high_bytes = c >> 8;
  if (high_bytes > LOWER_CASE_INDEX_LENGTH)
    return c;

  const char32_t* table = LOWER_CASE_INDEX[high_bytes];
  if (table == 0) // Not case folded
    return c;

  const uint16_t low_byte = c & 0xFF;
  // First table entry is # entries.
  // Entries are in table[1...n + 1], NOT [0..n]
  if (low_byte >= table[0])
    return c; // Not case folded

  const char32_t lowerChar = table[low_byte + 1];
  if (lowerChar == 0)
    return c; // No change in case

  return lowerChar;
}
} // namespace
