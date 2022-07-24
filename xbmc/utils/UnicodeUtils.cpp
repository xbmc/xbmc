/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
//-----------------------------------------------------------------------
//
//  File:      UnicodeUtils.cpp
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Major modifications by Frank Feuerbacher to utilize icu4c Unicode library
//  to resolve issues discovered in Kodi 19 Matrix
//
//  Modified to use J O'Leary's std::string class by kraqh3d
//
//------------------------------------------------------------------------
#include "../commons/ilog.h"
#include "StringUtils.h"

#include <cctype>
#include <iterator>
#include <locale>
#include <stddef.h>
#include <vector>

#include <bits/std_abs.h>
#include <bits/stdint-intn.h>
#include <unicode/uversion.h>

#ifdef HAVE_NEW_CROSSGUID
#include <crossguid/guid.hpp>
#else
#include <guid.h>
#endif

#if defined(TARGET_ANDROID)
#include <androidjni/JNIThreading.h>
#endif

#include "CharsetConverter.h"
#include "LangInfo.h"
#include "UnicodeUtils.h"
#include "Util.h"

#include <algorithm>
#include <array>
#include <assert.h>
#include <functional>
#include <inttypes.h>
#include <iomanip>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <time.h>
#include <tuple>

#include <fstrcmp.h>
#include <memory.h>
#include <unicode/locid.h>
// #include "unicode/uregex.h"
#include "utils/log.h"

#include <unicode/utf8.h>

// DO NOT MOVE or std functions end up in PCRE namespace
// clang-format off
#include "utils/RegExp.h"
// clang-format on
#include "utils/Unicode.h"

void UnicodeUtils::ToUpper(std::string& str, const icu::Locale& locale)
{
  if (str.length() == 0)
    return;

  std::string upper = Unicode::ToUpper(str, locale);
  str.swap(upper);
}

void UnicodeUtils::ToUpper(std::string& str, const std::locale& locale)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  UnicodeUtils::ToUpper(str, icuLocale);
}

void UnicodeUtils::ToUpper(std::string& str)
{
  if (str.length() == 0)
    return;

  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  UnicodeUtils::ToUpper(str, icuLocale);
}

void UnicodeUtils::ToUpper(std::wstring& str, const std::locale& locale)
{
  if (str.length() == 0)
    return;

  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  UnicodeUtils::ToUpper(str, icuLocale);
}

void UnicodeUtils::ToUpper(std::wstring& str, const icu::Locale& locale)
{
  if (str.length() == 0)
    return;

  std::string str_utf8 = Unicode::WStringToUTF8(str);
  const std::string upper = Unicode::ToUpper(str_utf8, locale);
  std::wstring wUpper = Unicode::UTF8ToWString(upper);
  str.swap(wUpper);
}
void UnicodeUtils::ToUpper(std::wstring& str)
{
  if (str.length() == 0)
    return;

  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  UnicodeUtils::ToUpper(str, icuLocale);
}

void UnicodeUtils::ToLower(std::string& str, const std::locale& locale)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  return UnicodeUtils::ToLower(str, icuLocale);
}

void UnicodeUtils::ToLower(std::string& str, const icu::Locale& locale)
{
  if (str.length() == 0)
    return;

  std::string lower = Unicode::ToLower(str, locale);
  str.swap(lower);
}

void UnicodeUtils::ToLower(std::wstring& str, const std::locale& locale)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  return UnicodeUtils::ToLower(str, icuLocale);
}

void UnicodeUtils::ToLower(std::wstring& str, const icu::Locale& locale)
{
  if (str.length() == 0)
    return;

  std::string str_utf8 = Unicode::WStringToUTF8(str);
  const std::string lower_utf8 = Unicode::ToLower(str_utf8, locale);
  std::wstring wLower = Unicode::UTF8ToWString(lower_utf8);
  str.swap(wLower);
}

void UnicodeUtils::ToLower(std::string& str)
{
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  return UnicodeUtils::ToLower(str, icuLocale);
}

void UnicodeUtils::ToLower(std::wstring& str)
{
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  return UnicodeUtils::ToLower(str, icuLocale);
}

void UnicodeUtils::FoldCase(std::string& str,
                            const StringOptions opt /*  = StringOptions::FOLD_CASE_DEFAULT */)
{
  if (str.length() == 0)
    return;

  std::string result = Unicode::FoldCase(str, opt);
  str.swap(result);
}

void UnicodeUtils::FoldCase(std::wstring& str,
                            const StringOptions opt /*  = StringOptions::FOLD_CASE_DEFAULT */)
{
  if (str.length() == 0)
    return;

  std::wstring result = Unicode::FoldCase(str, opt);
  str.swap(result);
  return;
}

void UnicodeUtils::ToCapitalize(std::wstring& str)
{
  std::wstring result = Unicode::ToCapitalize(str);
  str.swap(result);
  return;
}

void UnicodeUtils::ToCapitalize(std::string& str)
{
  std::string result = Unicode::ToCapitalize(str);
  str.swap(result);
  return;
}

std::wstring UnicodeUtils::TitleCase(const std::wstring_view& str, const std::locale& locale)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  std::wstring result = Unicode::TitleCase(str, icuLocale);
  return result;
}

std::wstring UnicodeUtils::TitleCase(const std::wstring_view& str)
{
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  std::wstring result = Unicode::TitleCase(str, icuLocale);
  return result;
}

std::string UnicodeUtils::TitleCase(const std::string_view& str, const std::locale& locale)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  std::string result = Unicode::TitleCase(str, icuLocale);
  return result;
}

std::string UnicodeUtils::TitleCase(const std::string_view& str)
{
  icu::Locale icuLocale = Unicode::GetDefaultICULocale();
  std::string result = Unicode::TitleCase(str, icuLocale);
  return result;
}

const std::wstring UnicodeUtils::Normalize(
    const std::wstring_view& src,
    const StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
    const NormalizerType NormalizerType)
{
  return Unicode::Normalize(src, opt, NormalizerType);
}

const std::string UnicodeUtils::Normalize(
    const std::string_view& src,
    const StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
    const NormalizerType NormalizerType)
{
  return Unicode::Normalize(src, opt, NormalizerType);
}

bool UnicodeUtils::Equals(const std::string_view& str1, const std::string_view& str2)
{
  int8_t rc = Unicode::StrCmp(str1, 0, str1.length(), str2, 0, str2.length());
  return rc == (int8_t)0;
}

bool UnicodeUtils::Equals(const std::wstring_view& str1, const std::wstring_view& str2)
{
  int8_t rc = Unicode::StrCmp(str1, 0, str1.length(), str2, 0, str2.length());
  return rc == 0;
}

bool UnicodeUtils::EqualsNoCase(const std::string_view& str1,
                                const std::string_view& str2,
                                StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
                                const bool Normalize /* = false */)
{
  // before we do the char-by-char comparison, first compare sizes of both strings.
  // This led to a 33% improvement in benchmarking on average. (size() just returns a member of std::string)
  if (str1.size() == 0 and str2.size() == 0)
    return true;

  if (str1.size() == 0 or str2.size() == 0)
    return false;

  int8_t rc = Unicode::StrCaseCmp(str1, str2, opt, Normalize);

  rc = rc == 0;
  return rc;
}
/*
bool UnicodeUtils::EqualsNoCase(const std::string& str1,
                                const char* s2,
                                StringOptions opt / * StringOptions::FOLD_CASE_DEFAULT * /,
                                const bool Normalize / * = false * /)
{
  std::string str2 = std::string(s2);
  return EqualsNoCase(str1, str2, opt, Normalize);
}

bool UnicodeUtils::EqualsNoCase(const char* s1,
                                const char* s2,
                                StringOptions opt / * = StringOptions::FOLD_CASE_DEFAULT * /,
                                const bool Normalize / * = false * /)
{
  std::string str1 = std::string(s1);
  std::string str2 = std::string(s2);
  return EqualsNoCase(str1, str2, opt, Normalize);
}
*/

int UnicodeUtils::Compare(const std::wstring_view& str1, const std::wstring_view& str2)
{

  int32_t str1_length = str1.length();
  int32_t str2_length = str2.length();
  int32_t str1_start = 0;
  int32_t str2_start = 0;
  int rc = Unicode::StrCmp(str1, str1_start, str1_length, str2, str2_start, str2_length);
  return rc;
}

int UnicodeUtils::Compare(const std::string_view& str1, const std::string_view& str2)
{

  int32_t str1_length = str1.length();
  int32_t str2_length = str2.length();
  int32_t str1_start = 0;
  int32_t str2_start = 0;
  int rc = Unicode::StrCmp(str1, str1_start, str1_length, str2, str2_start, str2_length);
  return rc;
}

int UnicodeUtils::CompareNoCase(const std::wstring_view& str1,
                                const std::wstring_view& str2,
                                StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
                                const bool Normalize /* = false */)
{

  int rc = Unicode::StrCaseCmp(str1, str2, opt, Normalize);
  return rc;
}
int UnicodeUtils::CompareNoCase(const std::string_view& str1,
                                const std::string_view& str2,
                                StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
                                const bool Normalize /* = false */)
{
  int rc = Unicode::StrCaseCmp(str1, str2, opt, Normalize);
  return rc;
}

int UnicodeUtils::CompareNoCase(const char* s1,
                                const char* s2,
                                StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
                                const bool Normalize /* = false */)
{
  std::string str1 = std::string(s1);
  std::string str2 = std::string(s2);
  return UnicodeUtils::CompareNoCase(str1, str2, opt, Normalize);
}

int UnicodeUtils::CompareNoCase(const std::string_view& str1,
                                const std::string_view& str2,
                                size_t n,
                                StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
                                const bool Normalize /* = false */)
{

  // This function is deprecated due to the argument n. Very dubious in a Unicode environment.

  if (n == 0) // Legacy behavior
  {
    n = std::string::npos;
  }
  else
  {
    if (ContainsNonAscii(str1))
    {
      CLog::Log(LOGWARNING, "UnicodeUtils::CompareNoCase str1 contains non-ASCII: {}", str1);
    }
    if (ContainsNonAscii(str2))
    {
      CLog::Log(LOGWARNING, "UnicodeUtils::CompareNoCase str2 contains non-ASCII: {}", str2);
    }
  }

  int rc = Unicode::StrCaseCmp(str1, str2, n, opt, Normalize);
  return rc;
}

int UnicodeUtils::CompareNoCase(const char* s1,
                                const char* s2,
                                size_t n /* = 0 */,
                                StringOptions opt /* = StringOptions::FOLD_CASE_DEFAULT */,
                                const bool Normalize /* = false */)
{

  std::string str1 = std::string(s1);
  std::string str2 = std::string(s2);
  return UnicodeUtils::CompareNoCase(str1, str2, n, opt, Normalize);
}

std::string UnicodeUtils::Left(const std::string_view& str, const size_t charCount, const bool keepLeft)
{
  std::string result = Unicode::Left(str, charCount, Unicode::GetDefaultICULocale(), keepLeft);

  return result;
}

std::string UnicodeUtils::Left(const std::string_view& str,
                               const size_t charCount,
                               const icu::Locale& icuLocale,
                               const bool keepLeft /* = true */)
{
  std::string result = Unicode::Left(str, charCount, icuLocale, keepLeft);

  return result;
}

std::string UnicodeUtils::Mid(const std::string_view& str,
                              const size_t firstCharIndex,
                              const size_t charCount /* = std::string::npos */)
{
  std::string result = Unicode::Mid(str, firstCharIndex, charCount);
  return result;
}

std::string UnicodeUtils::Right(const std::string_view& str,
                                const size_t charCount,
                                bool keepRight /* = true */)
{
  std::string result = Unicode::Right(str, charCount, Unicode::GetDefaultICULocale(), keepRight);
  return result;
}

std::string UnicodeUtils::Right(const std::string_view& str,
                                const size_t charCount,
                                const icu::Locale& icuLocale,
                                bool keepRight /* = true */)
{
  std::string result = Unicode::Right(str, charCount, icuLocale, keepRight);
  return result;
}

size_t UnicodeUtils::GetCharPosition(const std::string_view& str,
                                     size_t charCount,
                                     const bool left,
                                     const bool keepLeft,
                                     const icu::Locale& icuLocale)
{
  size_t byteIndex;
  byteIndex = Unicode::GetCharPosition(str, charCount, left, keepLeft, icuLocale);
  return byteIndex;
}

size_t UnicodeUtils::GetCharPosition(const std::string_view& str,
                                     size_t charCount,
                                     const bool left,
                                     const bool keepLeft,
                                     const std::locale& locale)
{
  icu::Locale icuLocale = Unicode::GetICULocale(locale);
  size_t byteIndex;
  byteIndex = Unicode::GetCharPosition(str, charCount, left, keepLeft, icuLocale);
  return byteIndex;
}

size_t UnicodeUtils::GetCharPosition(const std::string_view& str,
                                     size_t charCount,
                                     const bool left,
                                     const bool keepLeft)
{
  size_t byteIndex;
  byteIndex =
      Unicode::GetCharPosition(str, charCount, left, keepLeft, Unicode::GetDefaultICULocale());
  return byteIndex;
}

std::string& UnicodeUtils::Trim(std::string& str)
{
  std::string result = Unicode::Trim(str);
  str.swap(result);
  return str;
}

std::string& UnicodeUtils::Trim(std::string& str, const char* const chars)
{
  std::string delChars = std::string(chars);
  std::string result = Unicode::Trim(str, delChars, true, true);
  str.swap(result);
  return str;
}

std::string& UnicodeUtils::TrimLeft(std::string& str)
{
  std::string orig = std::string(str);
  std::string result = Unicode::TrimLeft(str);
  str.swap(result);
  return str;
}

std::string& UnicodeUtils::TrimLeft(std::string& str, const char* const chars)
{
  std::string delChars = std::string(chars);
  std::string result = Unicode::Trim(str, delChars, true, false);
  str.swap(result);
  return str;
}

std::string& UnicodeUtils::TrimRight(std::string& str)
{
  std::string result = Unicode::TrimRight(str);
  str.swap(result);
  return str;
}

std::string& UnicodeUtils::TrimRight(std::string& str, const char* const chars)
{
  std::string delChars = std::string(chars);
  std::string result = Unicode::Trim(str, delChars, false, true);
  str.swap(result);
  return str;
}

std::string UnicodeUtils::FindAndReplace(const std::string_view& str,
                                         const std::string_view oldText,
                                         const std::string_view newText)
{
  return Unicode::FindAndReplace(str, oldText, newText);
}

std::string UnicodeUtils::FindAndReplace(const std::string& str,
                                         const char* oldText,
                                         const char* newText)
{
  std::string s_oldText = std::string(oldText);
  std::string s_newText = std::string(newText);
  return UnicodeUtils::FindAndReplace(str, s_oldText, s_newText);
}

std::string UnicodeUtils::RegexReplaceAll(const std::string_view& str,
                                          const std::string_view pattern,
                                          const std::string_view newStr,
                                          const int flags)
{
  std::string result = Unicode::RegexReplaceAll(str, pattern, newStr, flags);
  return result;
}

/**
 * Replaces every occurrence of oldchar with newChar in str.
 *
 */
int UnicodeUtils::Replace(std::string& str, char oldChar, char newChar)
{
  if (not isascii(oldChar))
  {
    CLog::Log(LOGWARNING, "UnicodeUtils::Replace oldChar contains non-ASCII: {}\n", oldChar);
  }
  if (not isascii(newChar))
  {
    CLog::Log(LOGWARNING, "UnicodeUtils::Replace oldChar contains non-ASCII: {}\n", newChar);
  }
  std::string oldStr = std::string(1, oldChar);
  std::string newStr = std::string(1, newChar);
  return UnicodeUtils::Replace(str, oldStr, newStr);
}

/**
 * Replaces every occurrence of oldStr with newStr in str. Not regex based.
 */
int UnicodeUtils::Replace(std::string& str, const std::string_view& oldStr, const std::string_view& newStr)
{
  std::string orig = str;
  if (oldStr.empty() or str.empty())
    return 0;

  std::tuple<std::string, int> result = Unicode::FindCountAndReplace(str, oldStr, newStr);

  std::string resultStr = std::get<0>(result);
  str.swap(resultStr);
  int changes = std::get<1>(result);
  return changes;
}

int UnicodeUtils::Replace(std::wstring& str, const std::wstring_view& oldStr, const std::wstring_view& newStr)
{
  if (oldStr.empty() or str.empty())
    return 0;

  std::string str_utf8 = Unicode::WStringToUTF8(str);
  std::string str_utf8_save = std::string(str_utf8);
  std::string oldStr_utf8 = Unicode::WStringToUTF8(oldStr);
  std::string newStr_utf8 = Unicode::WStringToUTF8(newStr);

  int changes = UnicodeUtils::Replace(str_utf8, oldStr_utf8, newStr_utf8);
  std::wstring result_w = Unicode::UTF8ToWString(str_utf8);
  str.swap(result_w);
  return changes;
}

bool UnicodeUtils::StartsWith(const std::string_view& str1, const std::string_view& str2)
{
  return Unicode::StartsWith(str1, str2);
}

bool UnicodeUtils::StartsWith(const std::string_view& str1, const char* s2)
{
  return StartsWith(str1, std::string(s2));
}

bool UnicodeUtils::StartsWith(const char* s1, const char* s2)
{
  return StartsWith(std::string(s1), std::string(s2));
}

bool UnicodeUtils::StartsWithNoCase(const std::string_view& str1,
                                    const std::string_view& str2,
                                    StringOptions opt)
{
  if (str1.length() == 0 and str2.length() == 0)
    return true;

  if (str1.length() == 0 or str2.length() == 0)
    return false;

  // In general, different lengths don't tell you if Unicode strings are
  // different.

  return Unicode::StartsWithNoCase(str1, str2, opt);
}

bool UnicodeUtils::StartsWithNoCase(const std::string_view& str1, const char* s2, StringOptions opt)
{
  return StartsWithNoCase(str1, std::string(s2), opt);
}

bool UnicodeUtils::StartsWithNoCase(const char* s1, const char* s2, StringOptions opt)
{
  return StartsWithNoCase(std::string(s1), std::string(s2), opt);
}

bool UnicodeUtils::EndsWith(const std::string_view& str1, const std::string_view& str2)
{
  const bool result = Unicode::EndsWith(str1, str2);
  return result;
}

bool UnicodeUtils::EndsWith(const std::string_view& str1, const char* s2)
{
  std::string str2 = std::string(s2);
  return EndsWith(str1, str2);
}

bool UnicodeUtils::EndsWithNoCase(const std::string_view& str1,
                                  const std::string_view& str2,
                                  StringOptions opt)
{
  bool result = Unicode::EndsWithNoCase(str1, str2, opt);
  return result;
}

bool UnicodeUtils::EndsWithNoCase(const std::string_view& str1, const char* s2, StringOptions opt)
{
  std::string str2 = std::string(s2);
  return EndsWithNoCase(str1, str2, opt);
}

std::vector<std::string> UnicodeUtils::Split(const std::string& input,
                                             const std::string_view& delimiter,
                                             const size_t iMaxStrings)
{
  if (ContainsNonAscii(delimiter))
  {
    CLog::Log(LOGWARNING, "UnicodeUtils::Split delimiter contains non-ASCII: {}", delimiter);
  }
  std::vector<std::string> result = std::vector<std::string>();
  if (not(input.empty() or delimiter.empty()))
  {
    Unicode::SplitTo(std::back_inserter(result), input, delimiter, iMaxStrings);
  }
  else
  {
    if (not input.empty())
      result.push_back(input);
  }
  return result;
}

std::vector<std::string> UnicodeUtils::Split(const std::string& input,
                                             const char delimiter,
                                             size_t iMaxStrings)
{
  if (not isascii(delimiter))
  {
    CLog::Log(LOGWARNING, "UnicodeUtils::Split delimiter contains non-ASCII: {}\n", delimiter);
  }
  std::vector<std::string> result;
  std::string sDelimiter = std::string(1, delimiter);
  result = UnicodeUtils::Split(input, sDelimiter, iMaxStrings);

  return result;
}

std::vector<std::string> UnicodeUtils::Split(const std::string& input,
                                             const std::vector<std::string>& delimiters)
{
  // TODO: Need tests for splitting in middle of multi-codepoint characters.
  for (size_t i = 0; i < delimiters.size(); i++)
  {
    if (ContainsNonAscii(delimiters[i]))
    {
      CLog::Log(LOGWARNING, "UnicodeUtils::Split delimiter contains non-ASCII: {}\n",
                delimiters[i]);
    }
  }
  std::vector<std::string> result = std::vector<std::string>();

  if (input.length() == 0)
    return result;

  if (delimiters.size() == 0)
  {
    result.push_back(std::string(input)); // Send back a copy
    return result;
  }

  std::string orig = input;
  Unicode::SplitTo(std::back_inserter(result), input, delimiters, 0);
  return result;
}

std::vector<std::string> UnicodeUtils::SplitMulti(const std::vector<std::string>& input,
                                                  const std::vector<std::string>& delimiters,
                                                  size_t iMaxStrings /* = 0 */)
{
  return Unicode::SplitMulti(input, delimiters, iMaxStrings);
}

// returns the number of occurrences of strFind in strInput.
int UnicodeUtils::FindNumber(const std::string_view& strInput, const std::string_view& strFind)
{
  int numFound =
      Unicode::countOccurances(strInput, strFind, to_underlying(RegexpFlag::UREGEX_LITERAL));
  return numFound;
}

bool UnicodeUtils::InitializeCollator(bool Normalize /* = false */)
{
  return Unicode::InitializeCollator(Unicode::GetDefaultICULocale(), Normalize);
}
bool UnicodeUtils::InitializeCollator(const std::locale& locale, bool Normalize /* = false */)
{
  return Unicode::InitializeCollator(Unicode::GetICULocale(locale), Normalize);
}

bool UnicodeUtils::InitializeCollator(const icu::Locale& icuLocale, bool Normalize /* = false */)
{
  return Unicode::InitializeCollator(icuLocale, Normalize);
}

void UnicodeUtils::SortCompleted(int sortItems)
{
  Unicode::SortCompleted(sortItems);
}

int32_t UnicodeUtils::Collate(const std::wstring_view& left, const std::wstring_view& right)
{
  return Unicode::Collate(left, right);
}

int64_t UnicodeUtils::AlphaNumericCompare(const wchar_t* left, const wchar_t* right)
{
#ifdef USE_ICU_COLLATOR
  int64_t result = UnicodeUtils::Collate(std::wstring_view{left}, std::wstring_view{right});
#else
  int64_t result = StringUtils::AlphaNumericCompare(left, right);
#endif
  return result;
}

int UnicodeUtils::DateStringToYYYYMMDD(const std::string& dateString)
{
  std::vector<std::string> days = UnicodeUtils::Split(dateString, '-');
  if (days.size() == 1)
    return atoi(days[0].c_str());
  else if (days.size() == 2)
    return atoi(days[0].c_str()) * 100 + atoi(days[1].c_str());
  else if (days.size() == 3)
    return atoi(days[0].c_str()) * 10000 + atoi(days[1].c_str()) * 100 + atoi(days[2].c_str());
  else
    return -1;
}

long UnicodeUtils::TimeStringToSeconds(const std::string_view& timeString)
{
  std::string strCopy(timeString);
  UnicodeUtils::Trim(strCopy);
  if (UnicodeUtils::EndsWithNoCase(strCopy, " min"))
  {
    // this is imdb format of "XXX min"
    return 60 * atoi(strCopy.c_str());
  }
  else
  {
    std::vector<std::string> secs = UnicodeUtils::Split(strCopy, ':');
    int timeInSecs = 0;
    for (unsigned int i = 0; i < 3 && i < secs.size(); i++)
    {
      timeInSecs *= 60;
      timeInSecs += atoi(secs[i].c_str());
    }
    return timeInSecs;
  }
}

void UnicodeUtils::RemoveCRLF(std::string& strLine)
{
  UnicodeUtils::TrimRight(strLine, "\n\r");
}

bool UnicodeUtils::FindWord(const std::string_view& str, const std::string_view& word)
{
  return Unicode::FindWord(str, word);
}

std::string UnicodeUtils::Paramify(const std::string_view& param)
{
  std::string result = Unicode::FindAndReplace(param, "\\", "\\\\");

  // escape double quotes
  result = Unicode::FindAndReplace(result, "\"", "\\\"");

  // add double quotes around the whole string
  return "\"" + result + "\"";
}
