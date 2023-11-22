/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//-----------------------------------------------------------------------
//
//  File:      StringUtils.h
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to support J O'Leary's std::string class by kraqh3d
//
//------------------------------------------------------------------------

#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <sstream>
#include <locale>

// workaround for broken [[deprecated]] in coverity
#if defined(__COVERITY__)
#undef FMT_DEPRECATED
#define FMT_DEPRECATED
#endif
#include "utils/TimeFormat.h"
#include "utils/params_check_macros.h"

#include <fmt/format.h>
#if FMT_VERSION >= 80000
#include <fmt/xchar.h>
#endif

/*! \brief  C-processor Token stringification

The following macros can be used to stringify definitions to
C style strings.

Example:

#define foo 4
DEF_TO_STR_NAME(foo)  // outputs "foo"
DEF_TO_STR_VALUE(foo) // outputs "4"

*/

#define DEF_TO_STR_NAME(x) #x
#define DEF_TO_STR_VALUE(x) DEF_TO_STR_NAME(x)

template<typename T, std::enable_if_t<!std::is_enum<T>::value, int> = 0>
constexpr auto&& EnumToInt(T&& arg) noexcept
{
  return arg;
}
template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
constexpr auto EnumToInt(T&& arg) noexcept
{
  return static_cast<int>(arg);
}

class StringUtils
{
public:
  /*! \brief Get a formatted string similar to sprintf

  \param fmt Format of the resulting string
  \param ... variable number of value type arguments
  \return Formatted string
  */
  template<typename... Args>
  static std::string Format(const std::string& fmt, Args&&... args)
  {
    // coverity[fun_call_w_exception : FALSE]
    return ::fmt::format(fmt, EnumToInt(std::forward<Args>(args))...);
  }
  template<typename... Args>
  static std::wstring Format(const std::wstring& fmt, Args&&... args)
  {
    // coverity[fun_call_w_exception : FALSE]
    return ::fmt::format(fmt, EnumToInt(std::forward<Args>(args))...);
  }

  static std::string FormatV(PRINTF_FORMAT_STRING const char *fmt, va_list args);
  static std::wstring FormatV(PRINTF_FORMAT_STRING const wchar_t *fmt, va_list args);
  static std::string ToUpper(const std::string& str);
  static std::wstring ToUpper(const std::wstring& str);
  static void ToUpper(std::string &str);
  static void ToUpper(std::wstring &str);
  static std::string ToLower(const std::string& str);
  static std::wstring ToLower(const std::wstring& str);
  static void ToLower(std::string &str);
  static void ToLower(std::wstring &str);
  static void ToCapitalize(std::string &str);
  static void ToCapitalize(std::wstring &str);
  static bool EqualsNoCase(const std::string &str1, const std::string &str2);
  static bool EqualsNoCase(const std::string &str1, const char *s2);
  static bool EqualsNoCase(const char *s1, const char *s2);
  static int CompareNoCase(const std::string& str1, const std::string& str2, size_t n = 0);
  static int CompareNoCase(const char* s1, const char* s2, size_t n = 0);
  static int ReturnDigits(const std::string &str);
  static std::string Left(const std::string &str, size_t count);
  static std::string Mid(const std::string &str, size_t first, size_t count = std::string::npos);
  static std::string Right(const std::string &str, size_t count);
  static std::string& Trim(std::string &str);
  static std::string& Trim(std::string &str, const char* const chars);
  static std::string& TrimLeft(std::string &str);
  static std::string& TrimLeft(std::string &str, const char* const chars);
  static std::string& TrimRight(std::string &str);
  static std::string& TrimRight(std::string &str, const char* const chars);
  static std::string& RemoveDuplicatedSpacesAndTabs(std::string& str);

  /*! \brief Check if the character is a special character.

   A special character is not an alphanumeric character, and is not useful to provide information

   \param c Input character to be checked
   */
  static bool IsSpecialCharacter(char c);

  static std::string ReplaceSpecialCharactersWithSpace(const std::string& str);
  static int Replace(std::string &str, char oldChar, char newChar);
  static int Replace(std::string &str, const std::string &oldStr, const std::string &newStr);
  static int Replace(std::wstring &str, const std::wstring &oldStr, const std::wstring &newStr);
  static bool StartsWith(const std::string &str1, const std::string &str2);
  static bool StartsWith(const std::string &str1, const char *s2);
  static bool StartsWith(const char *s1, const char *s2);
  static bool StartsWithNoCase(const std::string &str1, const std::string &str2);
  static bool StartsWithNoCase(const std::string &str1, const char *s2);
  static bool StartsWithNoCase(const char *s1, const char *s2);
  static bool EndsWith(const std::string &str1, const std::string &str2);
  static bool EndsWith(const std::string &str1, const char *s2);
  static bool EndsWithNoCase(const std::string &str1, const std::string &str2);
  static bool EndsWithNoCase(const std::string &str1, const char *s2);

  template<typename CONTAINER>
  static std::string Join(const CONTAINER &strings, const std::string& delimiter)
  {
    std::string result;
    for (const auto& str : strings)
      result += str + delimiter;

    if (!result.empty())
      result.erase(result.size() - delimiter.size());
    return result;
  }

  /*! \brief Splits the given input string using the given delimiter into separate strings.

   If the given input string is empty the result will be an empty array (not
   an array containing an empty string).

   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of splitted strings
   */
  static std::vector<std::string> Split(const std::string& input, const std::string& delimiter, unsigned int iMaxStrings = 0);
  static std::vector<std::string> Split(const std::string& input, const char delimiter, size_t iMaxStrings = 0);
  static std::vector<std::string> Split(const std::string& input, const std::vector<std::string> &delimiters);
  /*! \brief Splits the given input string using the given delimiter into separate strings.

   If the given input string is empty nothing will be put into the target iterator.

   \param d_first the beginning of the destination range
   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of splitted strings
   \return output iterator to the element in the destination range, one past the last element
   *       that was put there
   */
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first, const std::string& input, const std::string& delimiter, unsigned int iMaxStrings = 0)
  {
    OutputIt dest = d_first;

    if (input.empty())
      return dest;
    if (delimiter.empty())
    {
      *d_first++ = input;
      return dest;
    }

    const size_t delimLen = delimiter.length();
    size_t nextDelim;
    size_t textPos = 0;
    do
    {
      if (--iMaxStrings == 0)
      {
        *dest++ = input.substr(textPos);
        break;
      }
      nextDelim = input.find(delimiter, textPos);
      *dest++ = input.substr(textPos, nextDelim - textPos);
      textPos = nextDelim + delimLen;
    } while (nextDelim != std::string::npos);

    return dest;
  }
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first, const std::string& input, const char delimiter, size_t iMaxStrings = 0)
  {
    return SplitTo(d_first, input, std::string(1, delimiter), iMaxStrings);
  }
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first, const std::string& input, const std::vector<std::string> &delimiters)
  {
    OutputIt dest = d_first;
    if (input.empty())
      return dest;

    if (delimiters.empty())
    {
      *dest++ = input;
      return dest;
    }
    std::string str = input;
    for (size_t di = 1; di < delimiters.size(); di++)
      StringUtils::Replace(str, delimiters[di], delimiters[0]);
    return SplitTo(dest, str, delimiters[0]);
  }

  /*! \brief Splits the given input strings using the given delimiters into further separate strings.

  If the given input string vector is empty the result will be an empty array (not
  an array containing an empty string).

  Delimiter strings are applied in order, so once the (optional) maximum number of
  items is produced no other delimiters are applied. This produces different results
  to applying all delimiters at once e.g. "a/b#c/d" becomes "a", "b#c", "d" rather
  than "a", "b", "c/d"

  \param input Input vector of strings each to be split
  \param delimiters Delimiter strings to be used to split the input strings
  \param iMaxStrings (optional) Maximum number of resulting split strings
  */
  static std::vector<std::string> SplitMulti(const std::vector<std::string>& input,
                                             const std::vector<std::string>& delimiters,
                                             size_t iMaxStrings = 0);
  static int FindNumber(const std::string& strInput, const std::string &strFind);
  static int64_t AlphaNumericCompare(const wchar_t *left, const wchar_t *right);
  static int AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2);
  static long TimeStringToSeconds(const std::string &timeString);
  static void RemoveCRLF(std::string& strLine);

  /*! \brief utf8 version of strlen - skips any non-starting bytes in the count, thus returning the number of utf8 characters
   \param s c-string to find the length of.
   \return the number of utf8 characters in the string.
   */
  static size_t utf8_strlen(const char *s);

  /*! \brief convert a time in seconds to a string based on the given time format
   \param seconds time in seconds
   \param format the format we want the time in.
   \return the formatted time
   \sa TIME_FORMAT
   */
  static std::string SecondsToTimeString(long seconds, TIME_FORMAT format = TIME_FORMAT_GUESS);

  /*! \brief check whether a string is a natural number.
   Matches [ \t]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is a natural number, false otherwise.
   */
  static bool IsNaturalNumber(const std::string& str);

  /*! \brief check whether a string is an integer.
   Matches [ \t]*[\-]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is an integer, false otherwise.
   */
  static bool IsInteger(const std::string& str);

  /* The next several isasciiXX and asciiXXvalue functions are locale independent (US-ASCII only),
   * as opposed to standard ::isXX (::isalpha, ::isdigit...) which are locale dependent.
   * Next functions get parameter as char and don't need double cast ((int)(unsigned char) is required for standard functions). */
  inline static bool isasciidigit(char chr) // locale independent
  {
    return chr >= '0' && chr <= '9';
  }
  inline static bool isasciixdigit(char chr) // locale independent
  {
    return (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') || (chr >= 'A' && chr <= 'F');
  }
  static int asciidigitvalue(char chr); // locale independent
  static int asciixdigitvalue(char chr); // locale independent
  inline static bool isasciiuppercaseletter(char chr) // locale independent
  {
    return (chr >= 'A' && chr <= 'Z');
  }
  inline static bool isasciilowercaseletter(char chr) // locale independent
  {
    return (chr >= 'a' && chr <= 'z');
  }
  inline static bool isasciialphanum(char chr) // locale independent
  {
    return isasciiuppercaseletter(chr) || isasciilowercaseletter(chr) || isasciidigit(chr);
  }
  static std::string SizeToString(int64_t size);
  static const std::string Empty;
  static size_t FindWords(const char *str, const char *wordLowerCase);
  static int FindEndBracket(const std::string &str, char opener, char closer, int startPos = 0);
  static int DateStringToYYYYMMDD(const std::string &dateString);
  static std::string ISODateToLocalizedDate (const std::string& strIsoDate);
  static void WordToDigits(std::string &word);
  static std::string CreateUUID();
  static bool ValidateUUID(const std::string &uuid); // NB only validates syntax
  static double CompareFuzzy(const std::string &left, const std::string &right);
  static int FindBestMatch(const std::string &str, const std::vector<std::string> &strings, double &matchscore);
  static bool ContainsKeyword(const std::string &str, const std::vector<std::string> &keywords);

  /*! \brief Convert the string of binary chars to the actual string.

  Convert the string representation of binary chars to the actual string.
  For example \1\2\3 is converted to a string with binary char \1, \2 and \3

  \param param String to convert
  \return Converted string
  */
  static std::string BinaryStringToString(const std::string& in);
  /**
   * Convert each character in the string to its hexadecimal
   * representation and return the concatenated result
   *
   * example: "abc\n" -> "6162630a"
   */
  static std::string ToHexadecimal(const std::string& in);
  /*! \brief Format the string with locale separators.

  Format the string with locale separators.
  For example 10000.57 in en-us is '10,000.57' but in italian is '10.000,57'

  \param param String to format
  \return Formatted string
  */
  template<typename T>
  static std::string FormatNumber(T num)
  {
    std::stringstream ss;
// ifdef is needed because when you set _ITERATOR_DEBUG_LEVEL=0 and you use custom numpunct you will get runtime error in debug mode
// for more info https://connect.microsoft.com/VisualStudio/feedback/details/2655363
#if !(defined(_DEBUG) && defined(TARGET_WINDOWS))
    ss.imbue(GetOriginalLocale());
#endif
    ss.precision(1);
    ss << std::fixed << num;
    return ss.str();
  }

  /*! \brief Escapes the given string to be able to be used as a parameter.

   Escapes backslashes and double-quotes with an additional backslash and
   adds double-quotes around the whole string.

   \param param String to escape/paramify
   \return Escaped/Paramified string
   */
  static std::string Paramify(const std::string &param);

  /*! \brief Unescapes the given string.

   Unescapes backslashes and double-quotes and removes double-quotes around the whole string.

   \param param String to unescape/deparamify
   \return Unescaped/Deparamified string
   */
  static std::string DeParamify(const std::string& param);

  /*! \brief Split a string by the specified delimiters.
   Splits a string using one or more delimiting characters, ignoring empty tokens.
   Differs from Split() in two ways:
    1. The delimiters are treated as individual characters, rather than a single delimiting string.
    2. Empty tokens are ignored.
   \return a vector of tokens
   */
  static std::vector<std::string> Tokenize(const std::string& input, const std::string& delimiters);
  static void Tokenize(const std::string& input, std::vector<std::string>& tokens, const std::string& delimiters);
  static std::vector<std::string> Tokenize(const std::string& input, const char delimiter);
  static void Tokenize(const std::string& input, std::vector<std::string>& tokens, const char delimiter);

  /*!
   * \brief Converts a string to a unsigned int number.
   * \param str The string to convert
   * \param fallback [OPT] The number to return when the conversion fails
   * \return The converted number, otherwise fallback if conversion fails
   */
  static uint32_t ToUint32(std::string_view str, uint32_t fallback = 0) noexcept;

  /*!
   * \brief Converts a string to a unsigned long long number.
   * \param str The string to convert
   * \param fallback [OPT] The number to return when the conversion fails
   * \return The converted number, otherwise fallback if conversion fails
   */
  static uint64_t ToUint64(std::string_view str, uint64_t fallback = 0) noexcept;

  /*!
   * \brief Converts a string to a float number.
   * \param str The string to convert
   * \param fallback [OPT] The number to return when the conversion fails
   * \return The converted number, otherwise fallback if conversion fails
   */
  static float ToFloat(std::string_view str, float fallback = 0.0f) noexcept;

  /*!
   * Returns bytes in a human readable format using the smallest unit that will fit `bytes` in at
   * most three digits. The number of decimals are adjusted with significance such that 'small'
   * numbers will have more decimals than larger ones.
   *
   * For example: 1024 bytes will be formatted as "1.00kB", 10240 bytes as "10.0kB" and
   * 102400 bytes as "100kB". See TestStringUtils for more examples.
   */
  static std::string FormatFileSize(uint64_t bytes);

  /*! \brief Converts a cstring pointer (const char*) to a std::string.
             In case nullptr is passed the result is an empty string.
      \param cstr the const pointer to char
      \return the resulting std::string or ""
   */
  static std::string CreateFromCString(const char* cstr);

  /*!
   * \brief Check if a keyword string is contained on another string.
   * \param str The string in which to search for the keyword
   * \param keyword The string to search for
   * \return True if the keyword if found.
   */
  static bool Contains(std::string_view str,
                       std::string_view keyword,
                       bool isCaseInsensitive = true);

private:
  /*!
   * Wrapper for CLangInfo::GetOriginalLocale() which allows us to
   * avoid including LangInfo.h from this header.
   */
  static const std::locale& GetOriginalLocale() noexcept;
};

struct sortstringbyname
{
  bool operator()(const std::string& strItem1, const std::string& strItem2) const
  {
    return StringUtils::CompareNoCase(strItem1, strItem2) < 0;
  }
};
