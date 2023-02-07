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

#include <locale>
#include <sstream>
#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

// workaround for broken [[deprecated]] in coverity
#if defined(__COVERITY__)
#undef FMT_DEPRECATED
#define FMT_DEPRECATED
#endif
#include "LangInfo.h"
#include "utils/TimeFormat.h"
#include "utils/params_check_macros.h"

#include <fmt/format.h>
#if FMT_VERSION >= 80000
#include <fmt/xchar.h>
#endif

#ifdef _MSC_VER
#define WARN_UNUSED_RESULT
#elif defined(__GNUC__) || defined(__clang__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
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
   *
   * \param fmt Format of the resulting string
   * \param ... variable number of value type arguments
   * \return Formatted string
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

  /*!
   * \brief Converts a u32string_view to a string
   *
   * Note: Used by StringUtils because CharSetConverter may not be initialized
   *       before it is needed.
   *
   * \param str a u32string_view in UTF32 format to be converted to a string in
   *        UTF8 format
   * \return a string containing the result
   */
  static std::string ToUtf8(const std::u32string_view str);

  /*!
   * \brief Converts a wstring_view to a string
   *
   * Note: Used by StringUtils because CharSetConverter may not be initialized
   *       before it is needed.
   *
   * \param str a wstring_view in a variant of UTF-32 or UTF-16 (Windows)
   *            format to be converted to string in UTF-8
   * \return a string containing the result
   */
  static std::string ToUtf8(const std::wstring_view str);

  /*!
   * \brief Converts a string_view to a u32string
   *
   * Note: Used by StringUtils because CharSetConverter may not be initialized
   *       before it is needed.
   *
   * \param str a string_view in UTF8 format to be converted to a u32string
   * \return a u32string containing the result
   */
  static std::u32string ToUtf32(const std::string_view utf8_string);

  static std::u32string ToUtf32(const std::wstring_view utf8_string);

  /*!
   * \brief Converts a string_view to a wstring
   *
   * Note: Used by StringUtils because CharSetConverter may not be initialized
   *       before it is needed.
   *
   * \param str a string_view in UTF8 format to be converted to wstring
   * \return a wstring containing the result
   */
  static std::wstring ToWstring(const std::string_view str);
  static std::wstring ToWstring(const std::u32string_view str);

  /*!
     * \brief Returns the C Locale, primarily for the use of ToLower/ToUpper.
     *
     * The C locale can be very useful for reducing undesirable side effects when
     * using ToLower/ToUpper for 'id' processing.
     *
     * Use FoldCase, if possible, to create caseless keys for Kodi internal use. Otherwise, if a keyword is
     * ASCII, then specifying the C locale will greatly reduce surprises, such as the "Turkic-I" problem.
     *
     * If the keyword can not be constrained to ASCII, then English will likely work (but not guaranteed
     * for all cases). Beyond that you are own your own.
     *
     * \return a cached instance of the Clocale. Caching eliminates the overhead of creating
     * the locale instance.
     */
  static std::locale GetCLocale()
  {
    static std::locale C_LOCALE;
    static bool initialized;

    if (&C_LOCALE == (void*)0)
    {
      initialized = false;
    }
    if (!initialized)
    {
      std::locale x = std::locale("C");

      C_LOCALE = x;
      initialized = true;
    }
    return C_LOCALE;
  }

  /*!
     * \brief Returns the en_GB Locale, primarily for the use of ToLower/ToUpper.
     *
     * The en_GB locale can be very useful for reducing undesirable side effects when
     * using ToLower/ToUpper for 'id' processing.
     *
     * If neither FoldCase nor the use of the C locale aren't adequate, then using the "en_GB" locale when
     * 'normalizing' a keyword for internal use may work. It will certainly be better than using the
     * user's locale, which may cause some nasty surprises, such as the "Turkic-I" problem.
     *
     * \return a cached instance of the en_GB.UTF-8 locale. Caching eliminates the overhead of creating
     * the locale instance.
     */
  static std::locale GetEnglishLocale()
  {
    static std::locale GBEnglishLocale;
    static bool EnglishLocaleInitialized;

    if (&GBEnglishLocale == (void*)0)
    {
      EnglishLocaleInitialized = false;
    }
    if (!EnglishLocaleInitialized)
    {
      std::locale x = std::locale("en_GB.UTF-8");

      GBEnglishLocale = x;
      EnglishLocaleInitialized = true;
    }
    return GBEnglishLocale;
  }

  // private:

  static std::u32string ToUpper(const std::u32string_view str,
                                const std::locale locale = g_langInfo.GetSystemLocale());

public:
  static std::string ToUpper(const std::string_view str,
                             const std::locale locale = g_langInfo.GetSystemLocale())
      WARN_UNUSED_RESULT;
  static std::wstring ToUpper(const std::wstring_view str,
                              const std::locale locale = g_langInfo.GetSystemLocale())
      WARN_UNUSED_RESULT;
  // static void ToUpper(std::string &str);
  // static void ToUpper(std::wstring &str);
  // static std::string ToLower(const std::string& str);
  // static std::wstring ToLower(const std::wstring& str);

  // private:
  static std::u32string ToLower(const std::u32string_view str,
                                const std::locale locale = g_langInfo.GetSystemLocale())
      WARN_UNUSED_RESULT;

public:
  static std::string ToLower(const std::string_view str,
                             const std::locale locale = g_langInfo.GetSystemLocale())
      WARN_UNUSED_RESULT;
  static std::wstring ToLower(const std::wstring_view str,
                              const std::locale locale = g_langInfo.GetSystemLocale())
      WARN_UNUSED_RESULT;

private:
  /*!
   *  \brief Folds the case of a string using a simple algorithm.
   *
   * Backend for FoldCase operations. The underlying tables use 32-bit Unicode
   * codepoints.
   *
   * This is similar to ToLower, but is meant to 'normalize' a string for use as a
   * unique-id. Essentially, most characters with case are changed to lower case.
   * Further, in some cases unimportant accent info is also removed. Locale is
   * ignored. Character case mapping tables derived from ICU4C are used to implement
   * this.
   *
   * Results are consistent with wstring_view by both converting to u32string for
   * the fold operation.
   *
   * This API does NOT implement "Full case folding" which requires a
   * more advanced library, such as ICU4C.
   *
   * \param str u32string to fold
   * \return Case folded version of str (all lower-case)
   */
  static std::u32string FoldCase(const std::u32string_view str) WARN_UNUSED_RESULT;

public:
  /*!
   *  \brief Folds the case of a string using a simple algorithm.
   *
   * This is similar to ToLower, but is meant to 'normalize' a string for use as a
   * unique-id. Essentially, most characters with case are changed to lower case.
   * Further, in some cases unimportant accent info is also removed. Locale is
   * ignored. Character case mapping tables derived from ICU4C are used to implement
   * this.
   *
   * Results are consistent with wstring_view by both converting to u32string for
   * the fold operation.
   *
   * This API does NOT implement "Full case folding" which requires a
   * more advanced library, such as ICU4C.
   *
   * \param str string to fold
   * \return Case folded version of str (all lower-case)
   */

  static std::string FoldCase(const std::string_view str) WARN_UNUSED_RESULT;

  /*!
   *  \brief Folds the case of a string using a simple algorithm.
   *
   * This is similar to ToLower, but is meant to 'normalize' a string for use as a
   * unique-id. Essentially, most  characters with case are changed to lower case.
   * Further, in some cases unimportant accent info is also removed. Locale is
   * ignored. Character case mapping tables derived from ICU4C are used to implement
   * this.
   *
   * Results are consistent with wstring_view by both converting to u32string for
   * the fold operation.
   *
   * This API does NOT implement "Full case folding" which requires a
   * more advanced library, such as ICU4C.
   *
   * \param str string to fold
   * \return Case folded version of str (all lower-case)
   */
  static std::wstring FoldCase(const std::wstring_view str) WARN_UNUSED_RESULT;

private:
  /*!
   *  \brief Folds and compares two u32strings
   *
   * By Folding and comparing characters one at a time the performance should be a bit
   * better than Folding both strings and then comparing. Note that these functions take
   * strings that have already been converted to u32string.
   *
   * Compares the codepoint values of each Unicode character in each string until
   * there is a difference, or when all codepoints have been compared.
   *
   * Note: Embedded NULLS are treated as ordinary characters, not as string terminators
   *
   * TODO: Revisit this behavior
   *
   * Used by CompareNoCase
   *
   * \param str1 u32string to fold and compare
   * \param str2 u32string to fold and compare
   * \return 0 if both folded strings are identical
   *         -1 if the first difference is a codepoint in folded str1 which is numerically less
   *            than folded str2 OR if they compare equal, but str1 is shorter.
   *         1  if the first difference is a codepoint in folded str1 which is numerically greater
   *            than folded str2 OR if they compare equal, but str2 is shorter.
   */

  static int FoldAndCompare(const std::u32string_view str1, const std::u32string_view str2);

  /*!
    * \brief Folds and compares two u32strings from the leftmost end. str2 determines the
    *        maximum number of Unicode codepoints (characters) compared.
    *
    * By Folding and comparing characters one at a time the performance should be a bit
    * better than Folding both strings and then comparing. Note that these functions take
    * strings that have already been converted to u32string.
    *
    * Used by StartsWithNoCase
    *
    * \param str1 u32string to fold and compare
    * \param str2 u32string to fold and compare
    * \return true if str1 has at least as many codepoints as str2 and that the leftmost codepoints
    *         of str1 match str2
    *         false otherwise
    */
  static bool FoldAndCompareStart(const std::u32string_view str1, const std::u32string_view str2);

  /*!
   * \brief Folds and compares two u32strings from the rightmost end. str2 determines the
   *        maximum number of Unicode codepoints (characters) compared.
   *
   * By Folding and comparing characters one at a time the performance should be a bit
   * better than Folding both strings and then comparing. Note that these functions take
   * strings that have already been converted to u32string.
   *
   * Used by EndsWithNoCase
   *
   * \param str1 u32string to fold and compare
   * \param str2 u32string to fold and compare
   * \return true if str1 has at least as many codepoints as str2 and that the rightmost codepoints
   *         of str1 match str2
   *         false otherwise
   */
  static bool FoldAndCompareEnd(const std::u32string_view str1, const std::u32string_view str2);

  /*!
    *  \brief Determines if two strings are equal when folded, or not
    *
    * Nearly identical to FoldAndCompare, except that unequal lengths will return
    * false.
    *
    * Compares the codepoint values of each Unicode character in each string until
    * there is a difference, or when all codepoints have been compared.
    *
    * Note: Embedded NULLS are treated as ordinary characters, not as string terminators
    *
    * TODO: Revisit this behavior
    *
    * Used by EqualsNoCase
    *
    * \param str1 u32string to fold and compare
    * \param str2 u32string to fold and compare
    * \return true if both folded strings are identical,
    *         false otherwise
    */
  static bool FoldAndEquals(const std::u32string_view str1, const std::u32string_view str2);

public:
  static void ToCapitalize(std::string &str);
  static void ToCapitalize(std::wstring &str);

  /*!
    * \brief Determines if two strings are the same.
    *
    * A bit faster than Compare since length check can be done before compare.
    * Also compare does not require any conversion.
    *
    * \param str1 one of the strings to compare
    * \param str2 one of the strings to compare
    * \return true if both strings are identical, otherwise false
    */
  static bool Equals(const std::string_view str1, const std::string_view str2);

  /*!
   * \brief Determines if two strings are the same, after case folding each.
   *
   * Logically equivalent to Equals(FoldCase(str1), FoldCase(str2))
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \return true if both strings compare after case folding, otherwise false
   */

  static bool EqualsNoCase(const std::string_view str1, const std::string_view str2);

    /*!
   * \brief Compares two strings using codepoint order. Locale does not matter.
   *
   * DO NOT USE for collation
   *
   * Best to use StartsWith or EndsWith than to specify 'n' since n can
   * be difficult to calculate using multi-byte characters.
   *
   * Equals is faster
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \param n maximum number of Unicode codepoints to compare.
   *
   * \return The result of bitwise character comparison:
   *      < 0 if the characters str1 are bitwise less than the characters in str2,
   *          OR if the str1 is shorter than str2, but compare equal until the end of str1
   *      = 0 if str1 contains the same characters as str2,
   *      > 0 if the characters in str1 are bitwise greater than the characters in str2
   *          OR if str2 is shorter than str1, but compare equal until the end of str2
   *
   * Note: Between one and four UTF8 code units may be required to
   *       represent a single Unicode codepoint (for practical
   *       purposes a character).
   */
  // TODO: Change the calls that use 'n' argument to use StartsWith and EndsWith. Faster
  //       since only equality is checked, not ordering. Equality check does not require
  //       character conversion.
  static int Compare(const std::string_view str1, const std::string_view str2, const size_t n = 0);

  /*!
   * \brief Compares two strings, ignoring case, using codepoint order.
   * Locale does not matter.
   *
   * DO NOT USE for collation
   *
   * Best to use StartsWithNoCase or EndsWithNoCase than to specify 'n' since n can
   * be difficult to calculate using multi-byte characters.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \param n maximum number of Unicode codepoints to compare.
   *
   * \return The result of bitwise character comparison of the folded characters:
   *      < 0 if the folded characters str1 are bitwise less than the folded characters in str2,
   *          OR if the str1 is shorter than str2, but compare equal until the end of str1
   *      = 0 if str1 contains the same folded characters as str2,
   *      > 0 if the folded characters in str1 are bitwise greater than the folded characters in str2
   *          OR if str2 is shorter than str1, but compare equal until the end of str2
   *
   * Note: Between one and four UTF8 code units may be required to
   *       represent a single Unicode codepoint (for practical
   *       purposes a character).
   *
   */
  // TODO: The VAST majority of uses can be changed to use EqualsNoCase, which is faster
  //       since no conversion to Unicode is required. Further, if string length is fixed,
  //       then a simple length check is all that is needed.
  //
  // TODO: For all of the cases that use 'n' they should change to StartsWithNoCase or EndsWithNoCase
  //       because they will be faster and less error-prone.
  static int CompareNoCase(const std::string_view str1,
                           const std::string_view str2,
                           const size_t n = 0);

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
  static int Replace(std::string &str, char oldChar, char newChar);
  static int Replace(std::string &str, const std::string &oldStr, const std::string &newStr);
  static int Replace(std::wstring &str, const std::wstring &oldStr, const std::wstring &newStr);

  /*!
   * \brief Determines if a string begins with another string
   *
   * Note: No character conversions required
   *
   * \param str1 string to be searched
   * \param str2 string to find at beginning of str1
   * \return true if str1 starts with str2, otherwise false
   */
  static bool StartsWith(const std::string_view str1, const std::string_view str2);

  /*!
   * \brief Determines if a string begins with another string, ignoring case
   *
   * Equivalent to StartsWith(FoldCase(str1), FoldCase(str2))
   *
   * \param str1 string to be searched
   * \param str2 string to find at beginning of str1
   * \return true if folded str1 starts with folded str2, otherwise false
   */
  static bool StartsWithNoCase(const std::string_view str1, const std::string_view str2);

  // Changed EndsWith APIs to be the same as EndsWithNoCase
  /*!
   * \brief Determines if a string ends with another string
   *
   * Note: no character conversion required
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \return true if str1 ends with str2, otherwise false
   */
  static bool EndsWith(const std::string_view str1, const std::string_view str2);

  /*!
   *  \brief Determines if a string ends with another string while ignoring case
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \return true if str1 ends with str2, otherwise false
   */
  static bool EndsWithNoCase(const std::string_view str1, const std::string_view str2);

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
  static std::string ToHex(const std::string_view in);
  static std::string ToHex(const std::wstring_view in);
  static std::string ToHex(const std::u32string_view in);

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

private:
  /*!
   * Wrapper for CLangInfo::GetOriginalLocale() which allows us to
   * avoid including LangInfo.h from this header.
   */
  static const std::locale& GetOriginalLocale() noexcept;

public:
  /*!
   * \brief detects when a string contains non-ASCII to aide in debugging or error reporting
   *
   * \param str String to be examined for non-ASCII
   * \return true if non-ASCII characters found, otherwise false
   */
  inline static bool ContainsNonAscii(std::string_view str)
  {
    for (size_t i = 0; i < str.length(); i++)
    {
      if (not isascii(str[i]))
      {
        return true;
      }
    }
    return false;
  }

  /*!
   * \brief detects when a wstring contains non-ASCII to aide in debugging or error reporting
   *
   * \param str String to be examined for non-ASCII
   * \return true if non-ASCII characters found, otherwise false
   */
  inline static bool ContainsNonAscii(std::wstring_view str)
  {
    for (size_t i = 0; i < str.length(); i++)
    {
      if (not isascii(str[i]))
      {
        return true;
      }
    }
    return false;
  }
};

struct sortstringbyname
{
  bool operator()(const std::string& strItem1, const std::string& strItem2) const
  {
    return StringUtils::CompareNoCase(strItem1, strItem2) < 0;
  }
};
