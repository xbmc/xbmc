/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <fmt/core.h>
#include <stddef.h>
#include <locale>
#include <type_traits>
#include <vector>

//-----------------------------------------------------------------------
//
//  File:      StringUtils.h
//
//  Purpose:   ATL split string utility
//  Author:    Paul J. Weiss
//
//  Major modifications by Frank Feuerbacher to utilize icu4c Unicode library
//  to resolve issues discovered in Kodi 19 Matrix (Unicode and Python 3 support)
//
//  Modified to use J O'Leary's std::string class by kraqh3d
//
//------------------------------------------------------------------------


// TODO: Move to make/cmake file
// USE_ICU_COLLATOR chooses between ICU and legacy Kodi collation

#define USE_ICU_COLLATOR 1

/*
 *
 * USE_TO_TITLE_FOR_CAPITALIZE controls whether to use legacy Kodi ToCapitalize
 * algorithm:
 *   Upper case first letter of every "word"; Punctuation as well as spaces
 *   separates words. "n.y-c you pig" results in "N.Y-C You Pig". No characters
 *   lower cased. Only uppercases first codepoint of word, potentially producing
 *   bad Unicode when multiple codepoints are used to represent a letter. Probably
 *   not a severe error. May be difficult to correct with exposed APIs.
 * New Algorithm is to use ICU toTitle. This is locale dependent.
 *   Imperfect, but uses Locale rules to Title Case. Takes context into account.
 *   Does NOT generally use a dictionary of words to NOT uppercases (i.e. to, the...)
 *   Would titlecase "n.y.c you pig" as "N.y.c You Pig", but "n-y-c" becomes "N-Y-C"
 *   (the same as Capitalize).
 *
 *   Properly handles any change of string lengths.
 */
#undef USE_TO_TITLE_FOR_CAPITALIZE

/*
 * TODO: Fix, or remove code for USE_FINDWORD_REGEX
 */
#undef USE_FINDWORD_REGEX

#include <stdarg.h>   // others depend on this
#include <climits>
#include <string>
#include <sstream>
#include <iostream>

// workaround for broken [[deprecated]] in coverity
#if defined(__COVERITY__)
#undef FMT_DEPRECATED
#define FMT_DEPRECATED
#endif
#include <fmt/format.h>

#if FMT_VERSION >= 80000
#include <fmt/xchar.h>
#endif

#include "LangInfo.h"
#include "utils/params_check_macros.h"
// #include "Unicode.h"
#include "XBDateTime.h"

/*!
 * \brief  C-processor Token string-ification
 *
 * The following macros can be used to stringify definitions to
 * C style strings.
 *
 * Example:
 *
 * #define foo 4
 * DEF_TO_STR_NAME(foo)  // outputs "foo"
 * DEF_TO_STR_VALUE(foo) // outputs "4"
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

	/**
	 *  A Brief description of some of the Unicode issues: (Work in Progress)
	 *  DO NOT take this as definitive! It is from my current understanding.
	 *
	 *  Working with Unicode and multiple languages greatly complicates
	 *  string operations.
	 *
	 *
   */

  /*!
   * \brief Get a formatted string similar to sprintf
   *
   * \param fmt Format of the resulting string
   * \param ... variable number of value type arguments
   * \return Formatted string
   */
  template<typename... Args>
  static std::string Format(const std::string& fmt, Args&&... args)
  {
  	// Note that there is an ICU version of Format

    // coverity[fun_call_w_exception : FALSE]

    return ::fmt::format(fmt, EnumToInt(std::forward<Args>(args))...);
     // Can NOT call CLog::Log from here due to recursion

  }

  /*!
   *  \brief Get a formatted wstring similar to sprintf
   *
   * \param fmt Format of the resulting string
   * \param ... variable number of value type arguments
   * \return Formatted string
   */
  template<typename... Args>
  static std::wstring Format(const std::wstring& fmt, Args&&... args)
  {
    // coverity[fun_call_w_exception : FALSE]

  	// TODO: Unicode- Which Locale is used? Unicode safe?

    return ::fmt::format(fmt, EnumToInt(std::forward<Args>(args))...);
    // Can NOT call CLog::Log due to recursion!
  }

  static std::string FormatV(PRINTF_FORMAT_STRING const char *fmt, va_list args);
  static std::wstring FormatV(PRINTF_FORMAT_STRING const wchar_t *fmt, va_list args);

  // TODO: consider renaming to ParseInt

  /*!
   * \brief Returns the int value of the first series of digits found in the string
   *
   *  Ignores any non-digits in string
   *
   * \param str to extract number from
   * \return int value of found string
   */
  static int ReturnDigits(const std::string &str);

  /*!
    * \brief Converts tabs to spaces and then removes duplicate space characters
    * from str in-place
    *
    * \param str to modify
    * \return trimmed string, same as str argument.
    */
   static std::string& RemoveDuplicatedSpacesAndTabs(std::string& str);

  /*! \brief Builds a string by appending every string from a container, separated by a delimiter
   *
   * \param strings a container of a number of strings
   * \param delimiter will separate each member of strings
   * \return the concatenation of every string in the container, separated by the delimiter
   */
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

  static int64_t AlphaNumericCompare(const wchar_t *left, const wchar_t *right);

  /*!
   * TODO: Unicode is this in use? Should this be modified or moved to UnicodeUtils?
   *
     * SQLite collating function, see sqlite3_create_collation
     * The equivalent of AlphaNumericCompare() but for comparing UTF8 encoded data using
     * LangInfo::GetSystemLocale
     *
     * This only processes enough data to find a difference, and avoids expensive data conversions.
     * When sorting in memory item data is converted once to wstring in advance prior to sorting, the
     * SQLite callback function can not do that kind of preparation. Instead, in order to use
     * AlphaNumericCompare(), it would have to repeatedly convert the full input data to wstring for
     * every pair comparison made. That approach was found to be 10 times slower than using this
     * separate routine.
     *
     * /param nKey1 byte-length of first UTF-8 string
     * /param pKey1 pointer to byte array for the first UTF-8 string to compare
     * /param nKey2 byte-length of second UTF-8 string
  	 * /param pKey2 pointer to byte array for the second UTF-8 string to compare
  	 * /return 0 if strings are the same,
  	 *       < 0 if first string should come before the second
  	 *       > 0 if the first string should come after the second
     */
  static int AlphaNumericCollation(int nKey1, const void* pKey1, int nKey2, const void* pKey2);

  /*!
   * \brief utf8 version of strlen - skips any non-starting bytes in the count,
   * thus returning the number of unicode codepoints. Note that a codepoint is NOT
   * a character, although it frequently is. It is, however, Unicode is based on
   * 32-bit (well, 21-bits, but contained in 4 bytes) codepoints. It is likely that
   * in the usecase for SQL, that this is what is needed.
   *
   * TODO: WARNING: Does NOT return # of Unicode "characters" More study is needed
   * to determine what is needed. Only used by SQL query with substring.
   *
   * \param s c-string to find the length of.
   * \return the number of utf8 characters in the string.
   */
  static size_t utf8_strlen(const char *s);

  /*!
   * \brief convert a time in seconds to a string based on the given time format
   *
   * \param seconds time in seconds
   * \param format the format to use to convert seconds to a time
   * \return the formatted time
   */
  static std::string SecondsToTimeString(long seconds, TIME_FORMAT format = TIME_FORMAT_GUESS);

  /*!
   * \brief check whether a string is a natural number.
   *
   * Matches [ \t]*[0-9]+[ \t]*
   *
   * \param str the string to check
   * \return true if the string is a natural number, false otherwise.
   */
  static bool IsNaturalNumber(const std::string& str);

  /*!
   * \brief check whether a string is an integer.
   *
   * Matches [ \t]*[\-]*[0-9]+[ \t]*
   *
   * \param str the string to check
   * \return true if the string is an integer, false otherwise.
   */
  static bool IsInteger(const std::string& str);

  /*!
   * \brief Determines whether the given character is an ASCII digit or not
   *
   * Locale independent, safe to use with UTF-8
   *
   * \param chr C-char (byte) to examine
   * \return true if char matches regex [0-9], else false
   */
  inline static bool isasciidigit(char chr)
  {
    return chr >= '0' && chr <= '9';
  }

  /*!
   * \brief Determines whether the given character is an ASCII hexadecimal digit or not
   *
   * Locale independent, safe to use with UTF-8
   *
   * \param chr C-char (byte) to examine
   * \return true if char matches regex [0-9a-fA-F], otherwise false
   *
   */
  inline static bool isasciixdigit(char chr)
  {
    return (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') || (chr >= 'A' && chr <= 'F');
  }

  /*!
   * \brief Converts the given ASCII digit to its numeric value.
   *
   * Locale independent, safe to use with UTF-8
   *
   * \param chr a C-char (byte)
   * \return -1 if ! isasciidigit(chr), otherwise the integer value represented by chr
   */
  static int asciidigitvalue(char chr); // locale independent

  /*!
   * \brief Converts the given ASCII hexadecimal digit to its numeric value.
   *
   * Locale independent, safe to use with UTF-8
   *
   * \param chr a C-char (byte)
   * \return -1 if ! isasciixdigit(chr), otherwise the integer value represented by
   *         hexadecimal digit chr (character case does not matter)
   */
  static int asciixdigitvalue(char chr);

  /*!
   * \brief Determines whether the given character is an ASCII uppercase letter or not
   *
   * Locale independent, safe to use with UTF-8
   *
   * \param chr C-char (byte) to examine
   * \return true if char matches regex [A-Z], otherwise false
   */
  inline static bool isasciiuppercaseletter(char chr) // locale independent
  {
    return (chr >= 'A' && chr <= 'Z');
  }

  /*!
    * \brief Determines whether the given character is an ASCII lowercase letter or not
    *
    * Locale independent, safe to use with UTF-8
    *
    * \param chr C-char (byte) to examine
    * \return true if char matches regex [a-z], otherwise false
    */
  inline static bool isasciilowercaseletter(char chr) // locale independent
  {
    return (chr >= 'a' && chr <= 'z');
  }

  /*!
    * \brief Determines whether the given character is an ASCII alphanumeric character or not
    *
    * Locale independent, safe to use with UTF-8
    *
    * \param chr C-char (byte) to examine
    * \return true if char matches regex [0-9a-zA-Z], otherwise false
    */
  inline static bool isasciialphanum(char chr) // locale independent
  {
    return isasciiuppercaseletter(chr) || isasciilowercaseletter(chr) || isasciidigit(chr);
  }

 /*!
  * \brief converts the given number into a human-friendly string representing a size in bytes.
  *
  * The returned string is a power of two:
  *   2147483647 => "2.00 GB"
  *
  * \param size
  * \return human friendly value for size
  */
  static std::string SizeToString(int64_t size);

  /*!
   * \brief A constant value for an empty string. Can be used when a constant return
   *        value is allowed
   */
  static const std::string Empty;

  /*!
   * \brief Starting at a point after an opening bracket, scans a string for it's matching
   * close bracket.
   *
   * Note: While the string can be utf-8, the open & close brackets must be ASCII.
   *
   * \param str A utf-8 string to scan for 'brackets'
   * \param opener The 'open-bracket' ASCII character used in the string
   * \param closer The 'close-bracket' ASCII character used in the string
   * \param startPos Offset in str, past the open-bracket that the function is to find
   * the closing bracket for.
   *
   * \return the index of the matching close-bracket, or std::string::npos if not found.
   *
   */
  static int FindEndBracket(const std::string &str, char opener, char closer, int startPos = 0);

  static std::string ISODateToLocalizedDate (const std::string& strIsoDate);

  /*!
   * \brief Converts ASCII string to digits using a specialized mapping that
   * can not be reversed to the original.
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
    ss.imbue(g_langInfo.GetSystemLocale());
#endif
    ss.precision(1);
    ss << std::fixed << num;
    return ss.str();
  }

  // TODO: Could rewrite to use Unicode delimiters using icu::UnicodeSet::spanUTF8(), and friends
  /*!
   *  \brief Splits a string using one or more delimiting ASCII characters,
   *  ignoring empty tokens.
   *
   *  Missing delimiters results in a match of entire string.
   *
   *  Differs from Split() in two ways:
   *    1. The delimiters are treated as individual ASCII characters, rather than a single
   *       delimiting string.
   *    2. Empty tokens are ignored.
   *
   * \param input string to split into tokens
   * \param delimiters one or more ASCII characters to use as token separators
   * \return a vector of non-empty tokens.
   *
   */
  static std::vector<std::string> Tokenize(const std::string& input, const std::string& delimiters);

  /*!
   *  \brief Splits a string using one or more delimiting ASCII characters,
   *  ignoring empty tokens.
   *
   *  Instead of returning the tokens as a return value, they are returned via the first
   *  argument, 'tokens'.
   *
   * \param tokens Vector that found, non-empty tokens are returned by
   * \param input string to split into tokens
   * \param delimiters one or more ASCII characters to use as token separators
   *
   */
  static void Tokenize(const std::string& input, std::vector<std::string>& tokens, const std::string& delimiters);

  /*!
   * \brief Splits a string into tokens delimited by a single ASCII character.
   *
   * \param input string to split into tokens
   * \param delimiter an ASCII character to use as a token separator
   * \return a vector of non-empty tokens.
   *
   */
  static std::vector<std::string> Tokenize(const std::string& input, const char delimiter);

  /*!
   * \brief Splits a string into tokens delimited by a single ASCII character.
   *
   * \param tokens Vector that found, non-empty tokens are returned by
   * \param input string to split into tokens
   * \param delimiter an ASCII character to use as a token separator
   *
   */
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
   * \brief Formats a file-size into human a human-friendly form
   * Returns bytes in a human readable format using the smallest unit that will fit `bytes` in at
   * most three digits. The number of decimals are adjusted with significance such that 'small'
   * numbers will have more decimals than larger ones.
   *
   * For example: 1024 bytes will be formatted as "1.00kB", 10240 bytes as "10.0kB" and
   * 102400 bytes as "100kB". See TestStringUtils for more examples.
   */
  static std::string FormatFileSize(uint64_t bytes);

  /*!
   * \brief Creates a std::string from a char*. A null pointer results in an empty string.
   *
   * \param cstr pointer to C null-terminated byte array
   * \return the resulting std::string or ""
   */
  static std::string CreateFromCString(const char* cstr);

private:

};
