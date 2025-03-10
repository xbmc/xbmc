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

#include <chrono>
#include <locale>
#include <span>
#include <sstream>
#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <vector>

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

namespace KODI::UTILS
{

template<typename T, std::enable_if_t<!std::is_enum_v<T>, int> = 0>
constexpr decltype(auto) EnumToInt(T&& arg) noexcept
{
  return std::forward<T>(arg);
}
template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr decltype(auto) EnumToInt(T&& arg) noexcept
{
  return fmt::underlying(std::forward<T>(arg));
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
  static constexpr std::string Format(std::string_view format, Args&&... args)
  {
    // coverity[fun_call_w_exception : FALSE]
    return fmt::format(fmt::runtime(format), EnumToInt(std::forward<Args>(args))...);
  }
  template<typename... Args>
  static constexpr std::wstring Format(std::wstring_view format, Args&&... args)
  {
    // coverity[fun_call_w_exception : FALSE]
    return fmt::format(fmt::runtime(format), EnumToInt(std::forward<Args>(args))...);
  }

  [[nodiscard]] static std::string FormatV(PRINTF_FORMAT_STRING const char* fmt, va_list args);
  [[nodiscard]] static std::wstring FormatV(PRINTF_FORMAT_STRING const wchar_t* fmt, va_list args);
  [[nodiscard]] static std::string ToUpper(std::string_view str);
  [[nodiscard]] static std::wstring ToUpper(std::wstring_view str);
  static void ToUpper(std::string& str) noexcept;
  static void ToUpper(std::wstring& str) noexcept;
  [[nodiscard]] static std::string ToLower(std::string_view str);
  [[nodiscard]] static std::wstring ToLower(std::wstring_view str);
  static void ToLower(std::string& str) noexcept;
  static void ToLower(std::wstring& str) noexcept;
  static void ToCapitalize(std::string& str) noexcept;
  static void ToCapitalize(std::wstring& str) noexcept;
  [[nodiscard]] static bool EqualsNoCase(std::string_view str1, std::string_view str2) noexcept;
  [[nodiscard]] static int CompareNoCase(std::string_view str1,
                                         std::string_view str2,
                                         size_t n = 0) noexcept;
  [[nodiscard]] static int ReturnDigits(std::string_view str) noexcept;
  [[nodiscard]] static std::string Left(std::string_view str, size_t count);
  [[nodiscard]] static std::string Mid(std::string_view str,
                                       size_t first,
                                       size_t count = std::string_view::npos);
  [[nodiscard]] static std::string Right(std::string_view str, size_t count);
  static std::string& Trim(std::string& str) noexcept;
  static std::string& Trim(std::string& str, std::string_view chars) noexcept;
  static std::string& TrimLeft(std::string& str) noexcept;
  static std::string& TrimLeft(std::string& str, std::string_view chars) noexcept;
  static std::string& TrimRight(std::string& str) noexcept;
  static std::string& TrimRight(std::string& str, std::string_view chars) noexcept;
  static std::string& RemoveDuplicatedSpacesAndTabs(std::string& str) noexcept;

  /*! \brief Check if the character is a special character.

   A special character is not an alphanumeric character, and is not useful to provide information

   \param c Input character to be checked
   */
  [[nodiscard]] static bool IsSpecialCharacter(char c) noexcept;

  [[nodiscard]] static std::string ReplaceSpecialCharactersWithSpace(std::string_view str);
  static int Replace(std::string& str, char oldChar, char newChar) noexcept;
  static int Replace(std::string& str, std::string_view oldStr, std::string_view newStr);
  static int Replace(std::wstring& str, std::wstring_view oldStr, std::wstring_view newStr);
  [[nodiscard]] static bool StartsWith(std::string_view str1, std::string_view str2) noexcept;
  [[nodiscard]] static bool StartsWithNoCase(std::string_view str1, std::string_view str2) noexcept;
  [[nodiscard]] static bool EndsWith(std::string_view str1, std::string_view str2) noexcept;
  [[nodiscard]] static bool EndsWithNoCase(std::string_view str1, std::string_view str2) noexcept;

  template<typename CONTAINER>
  [[nodiscard]] static std::string Join(const CONTAINER& strings, std::string_view delimiter)
  {
    std::string result;
    for (const auto& str : strings)
    {
      result += str;
      result += delimiter;
    }

    if (!result.empty())
      result.erase(result.size() - delimiter.size());
    return result;
  }

  /*! \brief Splits the given input string using the given delimiter into separate strings.

   If the given input string is empty the result will be an empty array (not
   an array containing an empty string).

   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of split strings
   */
  [[nodiscard]] static std::vector<std::string> Split(std::string_view input,
                                                      std::string_view delimiter,
                                                      unsigned int iMaxStrings = 0);
  [[nodiscard]] static std::vector<std::string> Split(std::string_view input,
                                                      char delimiter,
                                                      size_t iMaxStrings = 0);
  [[nodiscard]] static std::vector<std::string> Split(std::string_view input,
                                                      std::span<const std::string> delimiters);
  [[nodiscard]] static std::vector<std::string> Split(std::string_view input,
                                                      std::span<const std::string_view> delimiters);
  /*! \brief Splits the given input string using the given delimiter into separate strings.

   If the given input string is empty nothing will be put into the target iterator.

   \param d_first the beginning of the destination range
   \param input Input string to be split
   \param delimiter Delimiter to be used to split the input string
   \param iMaxStrings (optional) Maximum number of split strings
   \return output iterator to the element in the destination range, one past the last element
   *       that was put there
   */
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first,
                          std::string_view input,
                          std::string_view delimiter,
                          unsigned int iMaxStrings = 0)
  {
    OutputIt dest = d_first;

    if (input.empty())
      return dest;
    if (delimiter.empty())
    {
      *d_first++ = std::string(input);
      return dest;
    }

    const size_t delimLen = delimiter.length();
    size_t nextDelim;
    size_t textPos = 0;
    do
    {
      if (--iMaxStrings == 0)
      {
        *dest++ = std::string(input.substr(textPos));
        break;
      }
      nextDelim = input.find(delimiter, textPos);
      *dest++ = std::string(input.substr(textPos, nextDelim - textPos));
      textPos = nextDelim + delimLen;
    } while (nextDelim != std::string::npos);

    return dest;
  }
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first,
                          std::string_view input,
                          char delimiter,
                          size_t iMaxStrings = 0)
  {
    return SplitTo(d_first, input, std::string_view(&delimiter, 1), iMaxStrings);
  }
  template<typename OutputIt, typename StringLike>
  static OutputIt SplitTo(OutputIt d_first,
                          std::string_view input,
                          std::span<StringLike> delimiters)
  {
    OutputIt dest = d_first;
    if (input.empty())
      return dest;

    if (delimiters.empty())
    {
      *dest++ = std::string(input);
      return dest;
    }
    std::string str(input);
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
  [[nodiscard]] static std::vector<std::string> SplitMulti(std::span<const std::string> input,
                                                           std::span<const std::string> delimiters,
                                                           size_t iMaxStrings = 0);
  [[nodiscard]] static std::vector<std::string> SplitMulti(
      std::span<const std::string_view> input,
      std::span<const std::string_view> delimiters,
      size_t iMaxStrings = 0);
  [[nodiscard]] static std::vector<std::string> SplitMulti(
      std::span<const std::string> input,
      std::span<const std::string_view> delimiters,
      size_t iMaxStrings = 0);
  [[nodiscard]] static std::vector<std::string> SplitMulti(std::span<const std::string_view> input,
                                                           std::span<const std::string> delimiters,
                                                           size_t iMaxStrings = 0);
  [[nodiscard]] static int FindNumber(std::string_view strInput, std::string_view strFind) noexcept;
  [[nodiscard]] static int64_t AlphaNumericCompare(std::wstring_view left,
                                                   std::wstring_view right) noexcept;
  [[nodiscard]] static int AlphaNumericCollation(int nKey1,
                                                 const void* pKey1,
                                                 int nKey2,
                                                 const void* pKey2) noexcept;
  [[nodiscard]] static long TimeStringToSeconds(std::string_view timeString);
  static void RemoveCRLF(std::string& strLine) noexcept;

  /*! \brief utf8 version of strlen - skips any non-starting bytes in the count, thus returning the number of utf8 characters
   \param s c-string to find the length of.
   \return the number of utf8 characters in the string.
   */
  [[nodiscard]] static size_t utf8_strlen(std::string_view s) noexcept;

  /*! \brief convert a time in seconds to a string based on the given time format
   \param seconds time in seconds
   \param format the format we want the time in.
   \return the formatted time
   \sa TIME_FORMAT
   */
  [[nodiscard]] static std::string SecondsToTimeString(long seconds,
                                                       TIME_FORMAT format = TIME_FORMAT_GUESS);

  /*! \brief convert a milliseconds value to a time string in the TIME_FORMAT_HH_MM_SS format
   \param milliSeconds time in milliseconds
   \return the formatted time
   \sa TIME_FORMAT
   */
  [[nodiscard]] static std::string MillisecondsToTimeString(std::chrono::milliseconds milliSeconds);

  /*! \brief check whether a string is a natural number.
   Matches [ \t]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is a natural number, false otherwise.
   */
  [[nodiscard]] static bool IsNaturalNumber(std::string_view str) noexcept;

  /*! \brief check whether a string is an integer.
   Matches [ \t]*[\-]*[0-9]+[ \t]*
   \param str the string to check
   \return true if the string is an integer, false otherwise.
   */
  [[nodiscard]] static bool IsInteger(std::string_view str) noexcept;

  /* The next several isasciiXX and asciiXXvalue functions are locale independent (US-ASCII only),
   * as opposed to standard ::isXX (::isalpha, ::isdigit...) which are locale dependent.
   * Next functions get parameter as char and don't need double cast ((int)(unsigned char) is required for standard functions). */
  [[nodiscard]] inline static bool isasciidigit(char chr) noexcept // locale independent
  {
    return chr >= '0' && chr <= '9';
  }
  [[nodiscard]] inline static bool isasciixdigit(char chr) noexcept // locale independent
  {
    return (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') || (chr >= 'A' && chr <= 'F');
  }
  [[nodiscard]] static int asciidigitvalue(char chr) noexcept; // locale independent
  [[nodiscard]] static int asciixdigitvalue(char chr) noexcept; // locale independent
  [[nodiscard]] inline static bool isasciiuppercaseletter(char chr) noexcept // locale independent
  {
    return (chr >= 'A' && chr <= 'Z');
  }
  [[nodiscard]] inline static bool isasciilowercaseletter(char chr) noexcept // locale independent
  {
    return (chr >= 'a' && chr <= 'z');
  }
  [[nodiscard]] inline static bool isasciiletter(char chr) noexcept // locale independent
  {
    return isasciiuppercaseletter(chr) || isasciilowercaseletter(chr);
  }
  [[nodiscard]] inline static bool isasciialphanum(char chr) noexcept // locale independent
  {
    return isasciiletter(chr) || isasciidigit(chr);
  }
  [[nodiscard]] static std::string SizeToString(int64_t size);
  static const std::string Empty;
  [[nodiscard]] static size_t FindWords(std::string_view str,
                                        std::string_view wordLowerCase) noexcept;
  [[nodiscard]] static int FindEndBracket(std::string_view str,
                                          char opener,
                                          char closer,
                                          int startPos = 0) noexcept;
  [[nodiscard]] static int DateStringToYYYYMMDD(std::string_view dateString);
  [[nodiscard]] static std::string ISODateToLocalizedDate(std::string_view strIsoDate);
  static void WordToDigits(std::string& word) noexcept;
  [[nodiscard]] static std::string CreateUUID();
  [[nodiscard]] static bool ValidateUUID(const std::string& uuid); // NB only validates syntax
  [[nodiscard]] static double CompareFuzzy(std::string_view left, std::string_view right) noexcept;
  [[nodiscard]] static int FindBestMatch(std::string_view str,
                                         std::span<const std::string_view> strings,
                                         double& matchscore) noexcept;
  [[nodiscard]] static int FindBestMatch(std::string_view str,
                                         std::span<const std::string> strings,
                                         double& matchscore) noexcept;
  [[nodiscard]] static bool ContainsKeyword(std::string_view str,
                                            std::span<const std::string_view> keywords) noexcept;
  [[nodiscard]] static bool ContainsKeyword(std::string_view str,
                                            std::span<const std::string> keywords) noexcept;

  /*! \brief Convert the string of binary chars to the actual string.

  Convert the string representation of binary chars to the actual string.
  For example \1\2\3 is converted to a string with binary char \1, \2 and \3

  \param param String to convert
  \return Converted string
  */
  [[nodiscard]] static std::string BinaryStringToString(std::string_view in);
  /**
   * Convert each character in the string to its hexadecimal
   * representation and return the concatenated result
   *
   * example: "abc\n" -> "6162630a"
   */
  [[nodiscard]] static std::string ToHexadecimal(std::string_view in);
  /*! \brief Format the string with locale separators.

  Format the string with locale separators.
  For example 10000.57 in en-us is '10,000.57' but in italian is '10.000,57'

  \param param String to format
  \return Formatted string
  */
  template<typename T>
  [[nodiscard]] static std::string FormatNumber(T num)
  {
    std::stringstream ss;
// ifdef is needed because when you set _ITERATOR_DEBUG_LEVEL=0 and you use custom numpunct you will get runtime error in debug mode
// for more info https://connect.microsoft.com/VisualStudio/feedback/details/2655363
#if !(defined(_DEBUG) && defined(TARGET_WINDOWS))
    ss.imbue(GetOriginalLocale());
#endif
    ss.precision(1);
    ss << std::fixed << num;
    return std::move(ss).str();
  }

  /*! \brief Escapes the given string to be able to be used as a parameter.

   Escapes backslashes and double-quotes with an additional backslash and
   adds double-quotes around the whole string.

   \param param String to escape/paramify
   \return Escaped/Paramified string
   */
  [[nodiscard]] static std::string Paramify(std::string param);

  /*! \brief Unescapes the given string.

   Unescapes backslashes and double-quotes and removes double-quotes around the whole string.

   \param param String to unescape/deparamify
   \return Unescaped/Deparamified string
   */
  [[nodiscard]] static std::string DeParamify(std::string param);

  /*! \brief Split a string by the specified delimiters.
   Splits a string using one or more delimiting characters, ignoring empty tokens.
   Differs from Split() in two ways:
    1. The delimiters are treated as individual characters, rather than a single delimiting string.
    2. Empty tokens are ignored.
   \return a vector of tokens
   */
  [[nodiscard]] static std::vector<std::string> Tokenize(std::string_view input,
                                                         std::string_view delimiters);
  static void Tokenize(std::string_view input,
                       std::vector<std::string>& tokens,
                       std::string_view delimiters);
  [[nodiscard]] static std::vector<std::string> Tokenize(std::string_view input,
                                                         const char delimiter);
  static void Tokenize(std::string_view input,
                       std::vector<std::string>& tokens,
                       const char delimiter);

  /*!
   * \brief Converts a string to a unsigned int number.
   * \param str The string to convert
   * \param fallback [OPT] The number to return when the conversion fails
   * \return The converted number, otherwise fallback if conversion fails
   */
  [[nodiscard]] static uint32_t ToUint32(std::string_view str, uint32_t fallback = 0);

  /*!
   * \brief Converts a string to a unsigned long long number.
   * \param str The string to convert
   * \param fallback [OPT] The number to return when the conversion fails
   * \return The converted number, otherwise fallback if conversion fails
   */
  [[nodiscard]] static uint64_t ToUint64(std::string_view str, uint64_t fallback = 0);

  /*!
   * \brief Converts a string to a float number.
   * \param str The string to convert
   * \param fallback [OPT] The number to return when the conversion fails
   * \return The converted number, otherwise fallback if conversion fails
   */
  [[nodiscard]] static float ToFloat(std::string_view str, float fallback = 0.0f);

  /*!
   * Returns bytes in a human readable format using the smallest unit that will fit `bytes` in at
   * most three digits. The number of decimals are adjusted with significance such that 'small'
   * numbers will have more decimals than larger ones.
   *
   * For example: 1024 bytes will be formatted as "1.00kB", 10240 bytes as "10.0kB" and
   * 102400 bytes as "100kB". See TestStringUtils for more examples.
   */
  [[nodiscard]] static std::string FormatFileSize(uint64_t bytes);

  /*! \brief Converts a cstring pointer (const char*) to a std::string.
             In case nullptr is passed the result is an empty string.
      \param cstr the const pointer to char
      \return the resulting std::string or ""
   */
  [[nodiscard]] static std::string CreateFromCString(const char* cstr);

  /*!
   * \brief Check if a keyword string is contained on another string.
   * \param str The string in which to search for the keyword
   * \param keyword The string to search for
   * \return True if the keyword if found.
   */
  [[nodiscard]] static bool Contains(std::string_view str,
                                     std::string_view keyword,
                                     bool isCaseInsensitive = true) noexcept;

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
  bool operator()(std::string_view strItem1, std::string_view strItem2) const
  {
    return StringUtils::CompareNoCase(strItem1, strItem2) < 0;
  }
};

} // namespace KODI::UTILS

// We make these utilities globally available by default, as it would be a massive
// change to the codebase to refer to them by their fully qualified names or import
// them everywhere. However, it is important that the symbols are declared inside the
// KODI::UTILS namespace in order to avoid a name clash with p8platform. p8platform
// declares StringUtils in the global namespace. If we declare StringUtils without a
// namespace as well, this is a violation of the One Definition Rule and therefore
// undefined behavior. With the switch to C++20 in Kodi and recent versions of the clang compiler,
// this can cause crashes due to the `Empty` static constant being present in different ELF
// sections depending on the compiler mode.
// See also: https://github.com/llvm/llvm-project/issues/72361
using KODI::UTILS::sortstringbyname;
using KODI::UTILS::StringUtils;
