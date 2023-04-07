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
//  File:      StringUtils.cpp
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Modified to use J O'Leary's std::string class by kraqh3d
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
#include "utils/TimeFormat.h"
#include "utils/params_check_macros.h"

#include <fmt/format.h>
#if FMT_VERSION >= 80000
#include <fmt/xchar.h>
#endif

/*!
 * \brief  C-processor Token stringification
 *
 * The following macros can be used to stringify definitions to
 * C style strings.
 *
 *  Example:
 *
 *   #define foo 4
 *   DEF_TO_STR_NAME(foo)  // outputs "foo"
 *   DEF_TO_STR_VALUE(foo) // outputs "4"
 *
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

  static std::string FormatV(PRINTF_FORMAT_STRING const char* fmt, va_list args);
  static std::wstring FormatV(PRINTF_FORMAT_STRING const wchar_t* fmt, va_list args);

  /*!
   * \brief Converts a u32string_view to a string
   *
   * \param str a u32string_view in UTF32 format to be converted to a string in
   *        UTF8 format
   * \return a string containing the result
   */
  static std::string ToUtf8(std::u32string_view str);

  /*!
   * \brief Converts a wstring_view to a string
   *
   * \param str a wstring_view in a variant of UTF-32 or UTF-16 (Windows)
   *            format to be converted to string in UTF-8
   * \return a string containing the result
   */
  static std::string ToUtf8(std::wstring_view str);

  /*!
   * \brief Converts a string to a u32string
   *
   * \param str_string a string_view in UTF8 format to be converted to u32string
   * \return a u32string containing the result
   */
  static std::u32string ToUtf32(std::string_view str);

  /*!
   * \brief Converts a wstring to a u32string
   *
   * \param str a wstring_view in to be converted to a u32string
   * \return a u32string containing the result
   */
  static std::u32string ToUtf32(std::wstring_view str);

  /*!
   * \brief Converts a string to a wstring
   *
   * \param str a u32string_view to be converted to wstring
   * \return a wstring containing the result
   */
  static std::wstring ToWstring(std::string_view str);

  /*!
   * \brief Converts a u32string to a wstring
   *
   * \param str a u32string_view to be converted to wstring
   * \return a wstring containing the result
   */
  static std::wstring ToWstring(std::u32string_view str);

  /*!
   * \brief Returns the C Locale, primarily for the use of ToLower/ToUpper.
   *
   * The C locale can be very useful for reducing undesirable side effects when
   * using ToLower/ToUpper for 'id' processing.
   *
   * Use FoldCase, if possible, to create caseless keys for Kodi internal use.
   * Otherwise, if a keyword is ASCII, then specifying the C locale will greatly
   * reduce surprises, such as the "Turkic-I" problem.
   *
   * If the keyword can not be constrained to ASCII, then English will likely work
   * (but not guaranteed for all cases). Beyond that you are own your own.
   *
   * \return a cached instance of the Clocale. Caching eliminates the overhead of creating
   * the locale instance.
   */
  static std::locale GetCLocale();

public:
  /*
   *        IMPLEMENTATION NOTES FOR ToUpper, ToLower and FoldCase
   *
   * FoldCase
   *
   * C++ does not provide a means to do case folding.
   * This implementation uses case folding tables from ICU4C. For more info
   * see utils/unicode_tools. This implementation ONLY DOES SIMPLE case folding.
   * Simple case folding always produces strings of the same byte length
   * as the unfolded strings. Full case folding can produce strings of
   * different lengths. For example, the German 'ß' folds to 'ss'. Other,
   * more complex examples exist. Fortunately, there is not a large
   * number of these characters and not so likely to occur
   * in the contexts that folding is used in Kodi, but it would be more
   * bullet-proof to have full case folding. The implementation here
   * can easily be modified to use what is provided by ICU4C, should
   * Kodi incorporate it in the future.
   *
   * Case mapping (ToLower/ToUpper)
   *
   * C++ provides case mapping via toupper/tolower. Because
   * toupper/tolower operate on one code-unit at a time, it
   * works fairly well using UTF-8 with "western" languages based
   * on the "Latin" alphabet since many of these take up one byte of
   * UTF-8. However for many other languages, and even Latin-based
   * ones with accents, more than one-byte of UTF-8 is required
   * per character (codepoint) and the quality of tolower/toupper
   * decreases. It is best to use with at least UTF-16 and preferably
   * UTF-32.
   *
   * An alternative UTF-32 implementation is provided here which is based
   * on case mapping tables from ICU4C, similar to the case folding
   * tables. An advantage to using this alternative implementation
   * is that behavior is consistent with case-folding as well as being
   * consistent across platforms.
   *
   * This implementation ONLY DOES SIMPLE case mapping and is
   * insensitive to Locale (except "C" locale). For the C locale the
   * native C++ toupper/tolower is used. This is because the "C"
   * locale only modifies ASCII characters, which is useful in
   * certain situations.
   *
   * At some point the implementation can be changed to support full,
   * or nearly fully case mapping, or changed to use ICU4C.
   */

  /*!
   * \brief Changes lower case letters to upper case.
   *
   *        Note: Only simple case mapping is supported
   *
   * \param str string to change to upper-case
   * \param locale Specifies the language rules to follow
   * \return upper cased string
   */
  [[nodiscard]] static std::u32string ToUpper(std::u32string_view str,
                                              const std::locale locale = GetSystemLocale());

  /*!
   * \brief Changes lower case letters to upper case.
   *
   *        Note: Only simple case mapping is supported
   *
   * \param str string to change to upper-case
   * \param locale Specifies the language rules to follow
   * \return upper cased string
   */
  [[nodiscard]] static std::string ToUpper(std::string_view str,
                                           const std::locale locale = GetSystemLocale());

  /*!
   * \brief Changes lower case letters to upper case.
   *
   *        Note: Only simple case mapping is supported
   *
   * \param str string to change to upper-case
   * \param locale Specifies the language rules to follow
   * \return upper cased string
   */
  [[nodiscard]] static std::wstring ToUpper(std::wstring_view str,
                                            const std::locale locale = GetSystemLocale());

  /*!
   * \brief Changes upper case letters to lower-case case.
   *
   *        Note: Only simple case mapping is supported
   *
   * \param str string to change to upper-case
   * \param locale Specifies the language rules to follow
   * \return lower cased string
   */
  [[nodiscard]] static std::u32string ToLower(std::u32string_view str,
                                              const std::locale locale = GetSystemLocale());

  /*!
   * \brief Changes upper case letters to lower case.
   *
   *        Note: Only simple case mapping is supported
   *
   * \param str string to change to lower-case
   * \param locale Specifies the language rules to follow
   * \return lower cased string
   */
  [[nodiscard]] static std::string ToLower(std::string_view str,
                                           const std::locale locale = GetSystemLocale());

  /*!
   * \brief Changes upper case letters to lower case.
   *
   *        Note: Only simple case mapping is supported
   *
   * \param str string to change to lower-case
   * \param locale Specifies the language rules to follow
   * \return lower cased string
   */
  [[nodiscard]] static std::wstring ToLower(std::wstring_view str,
                                            const std::locale locale = GetSystemLocale());

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
   * To ensure consistent results regardless of string type (string, wstring or u32string)
   * all conversions occur using u32string.
   *
   * This API does NOT implement "Full case folding" which requires a
   * more advanced library, such as ICU4C.
   *
   * \param str u32string to fold
   * \return Case folded version of str (all lower-case)
   */
  [[nodiscard]] static std::u32string FoldCase(std::u32string_view str);

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

  [[nodiscard]] static std::string FoldCase(std::string_view str);

  /*!
   * \brief Folds the case of a string using a simple algorithm.
   *
   * This is similar to ToLower, but is meant to 'normalize' a string for use as a
   * unique-id. Essentially, most characters with case are changed to lower case.
   * Further, in some cases unimportant accent info is also removed. Locale is
   * ignored. Character case mapping tables derived from ICU4C are used to implement
   * this.
   *
   * Results are consistent with wstring_view by converting to u32string for
   * the fold operation.
   *
   * This API does NOT implement "Full case folding" which requires a
   * more advanced library, such as ICU4C.
   *
   * \param str string to fold
   * \return Case folded version of str (all lower-case)
   */
  [[nodiscard]] static std::wstring FoldCase(std::wstring_view str);

private:
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
  static bool FoldAndCompareStart(std::u32string_view str1, std::u32string_view str2);

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
  static bool FoldAndCompareEnd(std::u32string_view str1, std::u32string_view str2);

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
   * Used by EqualsNoCase
   *
   * \param str1 u32string to fold and compare
   * \param str2 u32string to fold and compare
   * \return true if both folded strings are identical,
   *         false otherwise
   */
  static bool FoldAndEquals(std::u32string_view str1, std::u32string_view str2);

public:
  static void ToCapitalize(std::string& str);
  static void ToCapitalize(std::wstring& str);

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
  static bool Equals(std::string_view str1, std::string_view str2);

  /*
   *
   * \brief Determines if two strings are the same.
   *
   * A bit faster than Compare since length check can be done before compare.
   * Also compare does not require any conversion.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \return true if both strings are identical, otherwise false
   */
  static bool Equals(std::wstring_view str1, std::wstring_view str2);

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
  static bool Equals(std::u32string_view str1, std::u32string_view str2);

  /*!
   * \brief Determines if two strings are the same, after case folding each.
   *
   * Logically equivalent to Equals(FoldCase(str1), FoldCase(str2))
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \return true if both strings compare after case folding, otherwise false
   */

  static bool EqualsNoCase(std::string_view str1, std::string_view str2);

  /*!
   * \brief Compares two strings, ignoring case, using lexicographic order.
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
   */

  static int CompareNoCase(std::string_view str1, std::string_view str2, const size_t n = 0);

  /*!
   * \brief Compares two wstrings, ignoring case, using codepoint order.
   *        Locale does not matter.
   *
   * DO NOT USE for collation
   * Embedded NULLS do NOT terminate string
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
   */
  static int CompareNoCase(std::wstring_view str1, std::wstring_view str2, const size_t n = 0);
  /*!
   * \brief Returns the int value of the first series of digits found in the string
   *
   *  Ignores any non-digits in string
   *
   * \param str to extract number from
   * \return int value of found string
   */
  static int ReturnDigits(const std::string& str);

  /*!
   * \brief gets the leftmost segment of a string
   *
   *  NOT MULTIBYTE SAFE
   *
   * \param str   string to get segment from
   * \param count how many bytes to get from leftmost end of str
   * \return string containing the first count bytes from str,
   *         or the entire str if count > str.length()
   */
  static std::string Left(const std::string& str, size_t count);

  /*!
   * \brief gets a middle segment of a string
   *
   *  NOT MULTIBYTE SAFE
   *
   * \param str   string to get segment from
   * \param first byte offset from start of str where segment starts
   * \param count how many bytes to get from str
   * \return string containing the segment first .. count bytes from str
   *         count is automatically reduced if str is not long enough
   *         An empty string is returned if first > length of str
   */
  static std::string Mid(const std::string& str, size_t first, size_t count = std::string::npos);

  /*!
   * \brief gets the rightmost segment of a string
   *
   *  NOT MULTIBYTE SAFE
   *
   * \param str   string to get segment from
   * \param count how many bytes to get from rightmost end of str
   * \return string containing the first count bytes from str,
   *         or the entire str if count > str.length()
   */
  static std::string Right(const std::string& str, size_t count);

  static std::string& Trim(std::string& str);
  static std::string& Trim(std::string& str, const char* const chars);
  static std::string& TrimLeft(std::string& str);
  static std::string& TrimLeft(std::string& str, const char* const chars);
  static std::string& TrimRight(std::string& str);
  static std::string& TrimRight(std::string& str, const char* const chars);

  /*!
   * \brief Converts tabs to spaces and then removes duplicate space characters
   *        from str in-place
   *
   * \param str to modify
   * \return trimmed string, same as str argument.
   */
  static std::string& RemoveDuplicatedSpacesAndTabs(std::string& str);
  static int Replace(std::string& str, char oldChar, char newChar);
  static int Replace(std::string& str, const std::string& oldStr, const std::string& newStr);
  static int Replace(std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr);

  /*!
   * \brief Determines if a string begins with another string
   *
   *        Note: No character conversions required
   *
   * \param str1 string to be searched
   * \param str2 string to find at beginning of str1
   * \return true if str1 starts with str2, otherwise false
   */
  static bool StartsWith(std::string_view str1, std::string_view str2);

  /*!
   * \brief Determines if a string begins with another string, ignoring case
   *
   *        Equivalent to StartsWith(FoldCase(str1), FoldCase(str2))
   *
   * \param str1 string to be searched
   * \param str2 string to find at beginning of str1
   * \return true if folded str1 starts with folded str2, otherwise false
   */
  static bool StartsWithNoCase(std::string_view str1, std::string_view str2);

  /*!
   * \brief Determines if a string ends with another string
   *
   * Note: no character conversion required
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \return true if str1 ends with str2, otherwise false
   */
  static bool EndsWith(std::string_view str1, std::string_view str2);

  /*!
   *  \brief Determines if a string ends with another string while ignoring case
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \return true if str1 ends with str2, otherwise false
   */
  static bool EndsWithNoCase(std::string_view str1, std::string_view str2);

  /*!
   *  \brief Builds a string by appending every string from a container,
   *         separated by a delimiter
   *
   * \param strings a container of a number of strings
   * \param delimiter will separate each member of strings
   * \return the concatenation of every string in the container, separated by the delimiter
   */
  template<typename CONTAINER>
  static std::string Join(const CONTAINER& strings, const std::string& delimiter)
  {
    if (strings.empty())
      return std::string();

    size_t size{0};
    size_t delimSize = delimiter.size();
    for (const auto& str : strings)
      size += str.size() + delimSize;

    std::string result;
    size -= delimSize;

    result.reserve(size);

    // Avoid erase

    bool appendDelimiter = false;
    for (const auto& str : strings)
    {
      if (appendDelimiter)
        result += delimiter;
      else
        appendDelimiter = true;

      result += str;
    }

    return result;
  }

  /*!
   * \brief Splits the given input string using the given delimiter into separate strings.
   *
   * If the given input string is empty the result will be an empty array (not
   * an array containing an empty string).
   *
   * \param input Input string to be split
   * \param delimiter Delimiter to be used to split the input string
   * \param iMaxStrings (optional) Maximum number of splitted strings
   */
  static std::vector<std::string> Split(const std::string& input,
                                        const std::string& delimiter,
                                        unsigned int iMaxStrings = 0);
  static std::vector<std::string> Split(const std::string& input,
                                        const char delimiter,
                                        size_t iMaxStrings = 0);
  static std::vector<std::string> Split(const std::string& input,
                                        const std::vector<std::string>& delimiters);

  /*!
   *  \brief Splits the given input string using the given delimiter into separate strings.
   *
   * If the given input string is empty nothing will be put into the target iterator.
   *
   * \param d_first the beginning of the destination range
   * \param input Input string to be split
   * \param delimiter Delimiter to be used to split the input string
   * \param iMaxStrings (optional) Maximum number of splitted strings
   * \return output iterator to the element in the destination range, one past the last element
   *       that was put there
   */
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first,
                          const std::string& input,
                          const std::string& delimiter,
                          unsigned int iMaxStrings = 0)
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
  static OutputIt SplitTo(OutputIt d_first,
                          const std::string& input,
                          const char delimiter,
                          size_t iMaxStrings = 0)
  {
    return SplitTo(d_first, input, std::string(1, delimiter), iMaxStrings);
  }
  template<typename OutputIt>
  static OutputIt SplitTo(OutputIt d_first,
                          const std::string& input,
                          const std::vector<std::string>& delimiters)
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

  /*! \brief Splits the given input strings using the given delimiters into
   *         further separate strings.
   *
   * If the given input string vector is empty the result will be an empty array (not
   * an array containing an empty string).
   *
   * Delimiter strings are applied in order, so once the (optional) maximum number of
   * items is produced no other delimiters are applied. This produces different results
   * to applying all delimiters at once e.g. "a/b#c/d" becomes "a", "b#c", "d" rather
   * than "a", "b", "c/d"
   *
   * \param input Input vector of strings each to be split
   * \param delimiters Delimiter strings to be used to split the input strings
   * \param iMaxStrings (optional) Maximum number of resulting split strings
   *
   * \return vector of the split strings
   */
  static std::vector<std::string> SplitMulti(const std::vector<std::string>& input,
                                             const std::vector<std::string>& delimiters,
                                             size_t iMaxStrings = 0);
  static int FindNumber(const std::string& strInput, const std::string& strFind);
  static int64_t AlphaNumericCompare(const wchar_t* left, const wchar_t* right);
  static int AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2);
  static long TimeStringToSeconds(const std::string& timeString);
  static void RemoveCRLF(std::string& strLine);
  /*!
   * \brief utf8 version of strlen
   *
   *       - skips any non-starting bytes in the count, thus returning the
   *         number of unicode codepoints.
   *       - it takes between one and four UTF8 code-units (bytes) to
   *         represent one Unicode code-point
   *       - a codepoint is NOT a character, although it frequently is.
   *         a codepoint may be just a graphical element of a character/grapheme
   *       - Unicode is based on 32-bit (well, 21-bits, but contained in 4 bytes)
   *         codepoints.
   *
   * \param s c-string to find the length of.
   * \return the number of utf8 characters in the string.
   */
  static size_t utf8_strlen(const char* s);

  /*!
   *  \brief convert a time in seconds into to a string based on the given time format
   *
   *  \param seconds time in seconds
   *  \param format the format we want the time in.
   *  \return the formatted time
   */
  static std::string SecondsToTimeString(long seconds, TIME_FORMAT format = TIME_FORMAT_GUESS);

  /*! \brief check whether a string is a natural number.
   *         Matches [ \t]*[0-9]+[ \t]*
   *
   * \param str the string to check
   * \return true if the string is a natural number, false otherwise.
   */
  static bool IsNaturalNumber(const std::string& str);

  /*!
   * \brief check whether a string is a natural number.
   *
   *        Matches [ \t]*[0-9]+[ \t]*
   *
   * \param str the string to check
   * \return true if the string is a natural number, false otherwise.
   */
  static bool IsInteger(const std::string& str);

  /* The next several isasciiXX and asciiXXvalue functions are locale independent
   * (US-ASCII only), as opposed to standard ::isXX (::isalpha, ::isdigit...)
   * which are locale dependent.
   *
   * Next functions get parameter as char and don't need double cast
   * ((int)(unsigned char) is required for standard functions).
   */

  /*!
   * \brief Determines whether the given character is an ASCII digit or not
   *
   * Locale independent, safe to use with UTF-8
   *
   * \param chr C-char (byte) to examine
   * \return true if char matches [0-9], else false
   */
  inline static bool isasciidigit(char chr) // locale independent
  {
    return chr >= '0' && chr <= '9';
  }

  /*!
   * \brief Determines whether the given character is an ASCII hexadecimal digit or not
   *
   * Locale independent, safe to use with UTF-8
   *
   * \param chr C-char (byte) to examine
   * \return true if char matches [0-9a-fA-F], otherwise false
   */
  inline static bool isasciixdigit(char chr) // locale independent
  {
    return (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') || (chr >= 'A' && chr <= 'F');
  }

  /*!
   * \brief Converts the given ASCII digit to its numeric value.
   *
   *        Locale independent, safe to use with UTF-8
   *
   * \param chr a C-char (byte)
   * \return -1 if ! isasciidigit(chr), otherwise the integer value represented by chr
   */
  static int asciidigitvalue(char chr); // locale independent

  /*!
   * \brief Converts the given ASCII hexadecimal digit to its numeric value.
   *
   *        Locale independent, safe to use with UTF-8
   *
   * \param chr a C-char (byte)
   * \return -1 if ! isasciixdigit(chr), otherwise the integer value represented by
   *         hexadecimal digit chr (character case does not matter)
   */
  static int asciixdigitvalue(char chr); // locale independent

  /*!
   * \brief Determines whether the given character is an ASCII uppercase letter or not
   *
   *        Locale independent, safe to use with UTF-8
   *
   * \param chr C-char (byte) to examine
   * \return true if char matches [A-Z],
   *        false otherwise
   */
  inline static bool isasciiuppercaseletter(char chr) // locale independent
  {
    return (chr >= 'A' && chr <= 'Z');
  }

  /*!
   * \brief Determines whether the given character is an ASCII lowercase letter or not
   *
   *        Locale independent, safe to use with UTF-8
   *
   * \param chr C-char (byte) to examine
   * \return true if char matches regex [a-z], otherwise false
   */
  inline static bool isasciilowercaseletter(char chr) // locale independent
  {
    return (chr >= 'a' && chr <= 'z');
  }

  /*!
   * \brief Determines whether the given character is an ASCII
   *        alphanumeric character or not
   *
   *        Locale independent, safe to use with UTF-8
   *
   * \param chr C-char (byte) to examine
   * \return true if char matches regex [0-9a-zA-Z], otherwise false
   */
  inline static bool isasciialphanum(char chr) // locale independent
  {
    return isasciiuppercaseletter(chr) || isasciilowercaseletter(chr) || isasciidigit(chr);
  }

  /*!
   * \brief converts the given number into a human-friendly string
   *        representing a size in bytes.
   *
   *        The returned string is a power of two:
   *           2147483647 => "2.00 GB"
   *
   * \param size
   * \return human friendly value for size
   */
  static std::string SizeToString(int64_t size);

  static const std::string Empty;

  /*!
   * \brief Determines if "word" begins on a boundary in a string
   *
   *        Designed to work with ASCII, may work with single-byte
   *        characters
   *
   *        Uses caseless comparison
   *
   * \param str string to search
   * \param wordLowerCase to search for in str
   * \return true if word found, otherwise false
   *
   * Search algorithm:
   *   If wordLowerCase matches the start of a boundary, then return true
   *   If there are no more boundaries, then return false
   *
   *   The first boundary is the start of str
   *   Starting at the current boundary, each subsequent boundary is found by
   *   skipping over one of: all letters or digits, or a single character
   *   Then skipping over all spaces
   */
  static size_t FindWords(const char* str, const char* wordLowerCase);

  /*!
   * \brief Starting at a point after an opening bracket, scans a string
   *        for it's matching close bracket.
   *
   * Note: While the string can be utf-8, the open & close brackets must be ASCII.
   *
   * \param str string to scan for 'brackets'
   * \param opener The 'open-bracket' ASCII character used in the string
   * \param closer The 'close-bracket' ASCII character used in the string
   * \param startPos Offset in str, past the open-bracket that the function is to find
   *        the closing bracket for.
   *
   * \return the index of the matching close-bracket, or std::string::npos if not found.
   */
  static int FindEndBracket(const std::string& str, char opener, char closer, int startPos = 0);

  /*!
   * \brief Converts a date string into an integer format
   *
   * \param dateString to be converted. See note
   * \return integer format of dateString.
   *
   * No validation of dateString is performed. It is assumed to be
   * in one of the following formats:
   *    YYYY-DD-MM, YYYY--DD, YYYY
   *
   *    Examples:
   *      1974-10-18 => 19741018
   *      1974-10    => 197410
   *      1974       => 1974
   */
  static int DateStringToYYYYMMDD(const std::string& dateString);
  static std::string ISODateToLocalizedDate(const std::string& strIsoDate);

  /*!
   * \brief Converts ASCII string to digits using a specialized mapping that
   *        can not be reversed to the original.
   *
   * \param word Converted to digits and spaces in place
   *
   * Convert rules:
   *    lower case all characters
   *    Digits are left alone
   *    Non letters are converted to spaces
   *    letters are converted as follows:
   *
   *       abc def ghi jkl mno pqrs tuv wxyz
   *       222 333 444 555 666 7777 888 9999
   */
  static void WordToDigits(std::string& word);
  static std::string CreateUUID();
  static bool ValidateUUID(const std::string& uuid); // NB only validates syntax

  /*!
   * \brief Performs a fuzzy search using "fstrcmp"
   *
   *       fstrcmp can work with Unicode/multibyte chars, but requires
   *       a different call than we make and uses "currently configured locale" to
   *       to get C_TYPE. Also speaks of using wide chars and not UTF-8 or other
   *       combinations. More investigation needed.
   *
   * \param left
   * \param right
   * \return A value of 0 means no match at all. The larger the value the
   *         stronger the match.
   */
  static double CompareFuzzy(const std::string& left, const std::string& right);

  /*!
   * \brief Determines which string from a list most closely matches another string
   *
   * Uses CompareFuzzy
   *
   * \param str string to find best match for
   * \param strings list of strings that are compared against str using CompareFuzzy
   * \param [OUT] matchscore the best matching score is returned
   * \return Index of the best match found
   */
  static int FindBestMatch(const std::string& str,
                           const std::vector<std::string>& strings,
                           double& matchscore);

  /*!
   * \brief Determines if a str is a substring of any of a list of strings
   *
   * \param str string to find in list
   * \param keywords strings to see if str is a substring of
   * \return true if str is a substring of one of the keywords, else false
   */
  static bool ContainsKeyword(const std::string& str, const std::vector<std::string>& keywords);

  /*!
   *  \brief Convert the string of binary chars to the actual string.
   *
   *         Convert the string representation of binary chars to the actual string.
   *         For example \1\2\3 is converted to a string with binary char \1, \2 and \3
   *
   *  \param param String to convert
   *  \return Converted string
   */
  static std::string BinaryStringToString(const std::string& in);
  /*!
   * \brief Converts a string to Hexadecimal characters without spaces
   *        or decoration
   *
   *        example: "abc\n" -> "6162630a"
   *
   * \param in string to be converted
   *
   * \return hex string version
   */
  static std::string ToHexadecimal(const std::string& in);

  /*!
   * \brief Convert string_view to hex primarily for debugging purposes.
   *
   *        Note: does not reorder hex for endian-ness
   *
   * \param in strin_view to be rendered as hex
   *
   * \return Each character in 'in' represented in hex, separated by space
   */
  static std::string ToHex(std::string_view in);

  /*!
   * \brief Convert wstring_view to hex primarily for debugging purposes.
   *
   *        Note: does not reorder hex for endian-ness
   *
   * \param in wstring_view to be rendered as hex
   *
   * \return Each character in 'in' represented in hex, separated by space
   */
  static std::string ToHex(std::wstring_view in);

  /*!
   * \brief Convert u32string_view to hex primarily for debugging purposes.
   *
   *        Note: does not reorder hex for endian-ness
   *
   * \param in u32string_view to be rendered as hex
   *
   * \return Each character in 'in' represented in hex, separated by space
   *
   */
  static std::string ToHex(std::u32string_view in);

  /*!
   * \brief Formats a string with separators appropriate for the Locale
   *
   *        Example 10000.57 in en-us is '10,000.57' but in italian is '10.000,57'
   *
   * \param param String to format
   * \return Formatted string
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

  /*!
   * \brief Escapes the given string to be able to be used as a parameter.
   *
   * Escapes backslashes and double-quotes with an additional backslash and
   * adds double-quotes around the whole string.
   *
   * \param param String to escape/paramify
   * \return Escaped/Paramified string
   */
  static std::string Paramify(const std::string& param);

  /*!
   *  \brief Unescapes the given string.
   *
   * Unescapes backslashes and double-quotes and removes double-quotes
   * around the whole string.
   *
   * \param param String to unescape/deparamify
   * \return Unescaped/Deparamified string
   */
  static std::string DeParamify(const std::string& param);

  /*!
   * \brief Split a string into tokens by the specified delimiters.
   *
   * Splits a string using one or more delimiting characters, ignoring empty tokens.
   *
   * Differs from Split() in two ways:
   *   1. The delimiters are treated as individual characters, rather than a
   *      single delimiting string.
   *   2. Empty tokens are ignored.
   *
   * \return a vector of tokens
   */
  static std::vector<std::string> Tokenize(const std::string& input, const std::string& delimiters);
  static void Tokenize(const std::string& input,
                       std::vector<std::string>& tokens,
                       const std::string& delimiters);
  static std::vector<std::string> Tokenize(const std::string& input, const char delimiter);
  static void Tokenize(const std::string& input,
                       std::vector<std::string>& tokens,
                       const char delimiter);

  /*!
   * \brief Converts a string to a uint32_t number
   *
   *        Uses istringstream
   *
   * \param str The string to convert
   * \param fallback [OPT] The number to return when the conversion fails
   * \return The converted number, otherwise fallback if conversion fails
   */
  static uint32_t ToUint32(std::string_view str, uint32_t fallback = 0) noexcept;

  /*!
   * \brief Converts a string to a unsigned uint64_t number
   *
   *        Uses istringstream
   *
   * \param str      string to convert
   * \param fallback [OPT] number to return when the conversion fails
   * \return The converted number, otherwise fallback if conversion fails
   */
  static uint64_t ToUint64(std::string_view str, uint64_t fallback = 0) noexcept;

  /*!
   * \brief Converts a string to a float number
   *
   *        Uses istringstream
   *
   * \param str The string to convert
   * \param fallback [OPT] The number to return when the conversion fails
   * \return The converted number, otherwise fallback if conversion fails
   */
  static float ToFloat(std::string_view str, float fallback = 0.0f) noexcept;

  /*!
   * \brief formats a byte length into a human friendly form
   *
   *        Ex: 1024 => "1.00kB", 10240 => "10.0kB" and 102400 => "100kB"
   */
  static std::string FormatFileSize(uint64_t bytes);

  /*!
   * \brief Converts a C-string into a std::string
   *
   * \param cstr a pointer to string to convert
   * \return a c++ string containing a copy of cstr, or an empty string if
   *         cstr is null
   */
  static std::string CreateFromCString(const char* cstr);

private:
  /*!
   * Wrapper for CLangInfo::GetOriginalLocale() which allows us to
   * avoid including LangInfo.h from this header.
   */
  static const std::locale& GetOriginalLocale() noexcept;

  static const std::locale& GetSystemLocale() noexcept;
};

struct sortstringbyname
{
  bool operator()(const std::string& strItem1, const std::string& strItem2) const
  {
    return StringUtils::CompareNoCase(strItem1, strItem2) < 0;
  }
};
