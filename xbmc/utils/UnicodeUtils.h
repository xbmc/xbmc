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
//  File:      UnicodeUtils.h
//
//  Purpose:   Unicode oriented string functions
//  Author:    Frank Feuerbacher
//
//  Derived from StringUtils.h, but put in separate class to emphasize
//  both the Unicode orientation as well as to point out that the
//  functions are as close to the original StringUtils functions, but NOT
//  identical. Function name, parameter and behavioral differences exist.
//
//  This class relies heavily on icu4c Unicode library. The Unicode class
//  handles the low level interaction with ICU while this provides the
//  functions which Kodi code should normally use.
//
//------------------------------------------------------------------------

// TODO: Move to make/cmake file
// #define USE_ICU_COLLATOR chooses between ICU and legacy Kodi collation

#define USE_ICU_COLLATOR 1

/*
 * #define USE_TO_TITLE_FOR_CAPITALIZE controls whether to use legacy Kodi ToCapitalize
 * or to use ICU's TitleCase.
 * ToCapitalize algorithm:
 *   Upper case first letter of every "word"; Punctuation as well as spaces
 *   separates words. "n.y-c you pig" results in "N.Y-C You Pig". No characters
 *   lower cased. Only uppercases first codepoint of word, potentially producing
 *   bad Unicode when multiple codepoints are used to represent a letter.
 *   May be difficult to correct with exposed APIs.
 *
 * TitleCase algorithm:
 *   Imperfect, but uses Locale rules to Title Case. Takes context into account.
 *   Does NOT generally use a dictionary of words to NOT uppercases (i.e. to, the...)
 *   Would titlecase "n.y.c you pig" as "N.y.c You Pig", but "n-y-c" becomes "N-Y-C"
 *   (the same as Capitalize). Behavior can be modified, including adding code to
 *   perform dictionary lookups.
 *
 *   Properly handles any change of string (byte) lengths during the capitalization.
 *
 * Limitations: Uses the current user's locale. Call ToTitle directly to be able
 *   to specify locale.
 */
#undef USE_TO_TITLE_FOR_CAPITALIZE

#include <climits>
#include <iostream>
#include <sstream>
#include <stdarg.h> // others depend on this
#include <string>

// workaround for broken [[deprecated]] in coverity
#if defined(__COVERITY__)
#undef FMT_DEPRECATED
#define FMT_DEPRECATED
#endif

#if FMT_VERSION >= 80000
#include <fmt/xchar.h>
#endif

#include "LangInfo.h"
#include "XBDateTime.h"
#include "utils/Unicode.h"
#include "utils/params_check_macros.h"

class UnicodeUtils
{
public:
  /**
   *  A Brief description of some of the Unicode issues:
   *
   *  Working with Unicode and multiple languages greatly complicates
   *  string operations. ToLower prompted the creation of this class. Turkic
   *  has 4 versions of the letter I (two of which are the Roman ones "I"
   *  and "i". A number of languages do something like this and it is not normally
   *  a huge deal. However, in Turkic, changing the case of the Roman "I" (same
   *  as English I) results in the Turkic, lower case dotless I, "ı". This
   *  broke tons of code and prevented Kodi from woking at all in Turkic and
   *  a similar language. Other problems were discovered that were more subtle.
   *
   *  Changing the case of strings can change their byte AND character lengths,
   *  depending upon locale. Example, German sharp-s 'ß' is equivalent to two
   *  lower-case 's' characters. UpperCasing a sharp-s becomes 'SS' because
   *  there is no upper-case sharp-S.
   *
   *  FoldCase is normally what you want to use instead of ToLower (when you
   *  want to 'normalize' a string for use as an index, etc.). (This resolved
   *  the Turkic-I problem mentioned above.)
   *
   *  UTF-8, UTF-16, etc. are all encodings for 4-byte Unicode codepoints.
   *  A codepoint is frequently a 'character' but not necessarily. Codepoints
   *  can represent character variations, such as accent marks. Emojiis are
   *  also included in the Unicode standard. (The flag of Scotland (🏴󠁧󠁢󠁳󠁣󠁴󠁿) requires
   *  seven-codepoint sequence)). Unicode frequently uses the terms "character"
   *  and "Grapheme" interchangeably, if not a bit loosely.
   *
   *  In addition to some characters being composed of multiple codepoints, the
   *  order of all but the first codepoint can get scrambled a bit during
   *  processing. They visually look the same (who cares if the left upper dot
   *  is specified before the right upper dot), they can wreak havoc with the
   *  code. To solve this are various Normalizations that can be applied. Fortunately
   *  Normalization is not required in many cases. The only problem is, I haven't
   *  figured out the rules yet. Normalization may be required before & after a
   *  FoldCase operation, for example. When possible, this function does this
   *  for you.
   *
   *  C++ has some rudimentary Unicode support but has gaping holes:
   *  Many g++ systems define wchar_t as unsigned 32 bits, which maps
   *  nicely to a Unicode codepoint. However, the standard does NOT specify
   *  a length for wchar_t. Newer C++ versions adds several new character
   *  types that move in the right direction, but there is no support for
   *  Normalizing, working with multi-codepoint "characters" and many other
   *  capabilities.
   *
   *  Implications of C++ limitations:
   *  * You can't compare two strings one byte or even codepoint at a time
   *  * Two strings of different, non-zero lengths does not tell you if
   *    strings are not-equal! (Not without knowing that the strings are Normalized
   *    for the same locale).
   *  * When iterating the characters of a string, you frequently have
   *    to know where the character boundaries are, which there is no support
   *    for
   *  * You may have to copy or alter strings simply to compare them
   *
   * ICU library
   *  * The ICU library, while complicated and pretty heavy, can reliably
   *    handle many localization and Unicode issues.
   *  * Frequently strings are Normalized. Normalizing isn't that expensive.
   *  * While it is simpler/faster to keep everything in Unicode (32-bit),
   *    it is not overly expensive to use utf-8, utf-16.
   *  * ICU tries to be as efficient as possible, assumes the most common,
   *    cheaper case first, resorting to more expensive solutions only when
   *    needed.
   *
   * The string methods here are definitely more expensive than those in StringUtils.
   * There is a lot of opportunity for improvement:
   *  * This initial release focused more on correct function than speed.
   *    Numerous improvements can be made
   *  * Database Sorting is a good opportunity for improvement. There are
   *    multiple character conversions performed for EACH comparison
   *    (UTF-8? -> wchar (Unicode) -> UTF-16 (ICU native encoding)).
   *    Database encodings are configurable. I don't know how Kodi configures
   *    the databases.
   *  * Many operations use heap memory for temp objects. Can change to
   *    stack allocated memory (requires a bit more boilerplate code)
   *  * For future release, convert everything to one of ICUs preferred
   *    representations and avoid all of the temp conversions.
   */

  /*!
   * \brief Converts a string to Upper case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param str string to change case on in-place
   * \param locale the underlying icu::Locale is created using the language,
   *        country, etc. from this locale
   */
  static void ToUpper(std::string& str, const std::locale& locale);

  /*!
   * \brief Converts a string to Upper case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on in-place
   * \param locale controls the conversion rules
   */
  static void ToUpper(std::string& str, const icu::Locale& locale);

  /*!
   * \brief Converts a string to Upper case using LangInfo::GetSystemLocale
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on in-place
   */
  static void ToUpper(std::string& str);

  /*!
   * \brief Converts a wstring to Upper case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param str string to change case on in-place
   * \param locale the underlying icu::Locale is created using the language,
   *        country, etc. from this locale
   */
  static void ToUpper(std::wstring& str, const std::locale& locale);

  /*!
   * \brief Converts a wstring to Upper case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on in-place
   * \param locale controls the conversion rules
   */
  static void ToUpper(std::wstring& str, const icu::Locale& locale);

  /*!
   * \brief Converts a wstring to Upper case using LangInfo::GetSystemLocale
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on in-place
   */
  static void ToUpper(std::wstring& str);

  /*!
   * \brief Converts a string to Lower case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param str string to change case on
   * \param locale the underlying icu::Locale is created using the language,
   *        country, etc. from this locale
   */
  static void ToLower(std::string& str, const std::locale& locale);

  /*!
   * \brief Converts a string to Lower case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on in-place
   * \param locale controls the conversion rules
   */
  static void ToLower(std::string& str, const icu::Locale& locale);

  /*!
   * \brief Converts a string to Lower case using LangInfo::GetSystemLocale
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on in-place
   */
  static void ToLower(std::string& str);

  /*!
   * \brief Converts a wstring to Lower case according to locale.
   *
   * Note: The length of the string can change, depending upon the underlying
   * icu::locale.
   *
   * \param str string to change case on in-place
   * \param locale the underlying icu::Locale is created using the language,
   *        country, etc. from this locale
   */
  static void ToLower(std::wstring& str, const std::locale& locale);

  /*!
   * \brief Converts a wstring to Lower case according to locale.
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on in-place
   * \param locale controls the conversion rules
   */
  static void ToLower(std::wstring& str, const icu::Locale& locale);

  /*!
   * \brief Converts a wstring to Lower case using LangInfo::GetSystemLocale
   *
   * Note: the length of the string can change, depending upon locale.
   *
   * \param str string to change case on in-place
   */
  static void ToLower(std::wstring& str);

  /*!
   *  \brief Folds the case of a string. Locale Independent.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * In particular, when FOLD_CASE_DEFAULT is used, the Turkic Dotted I and Dotless
   * i follow the "en" locale rules for ToLower.
   *
   * DEVELOPERS who use non-ASCII keywords that will use FoldCase
   * should be aware that it may not always work as expected. Testing is important.
   * Changes will have to be made to keywords that don't work as expected. One solution is
   * to try to always use lower-case in the first place.
   *
   * \param str string to fold in place
   * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
   *            default value, FOLD_CASE_DEFAULT
   *
   * Note: This function serves a similar purpose that "ToLower/ToUpper" is
   *       frequently used for ASCII maps indexed by keyword. ToLower/ToUpper does
   *       NOT work to 'Normalize' Unicode to a consistent value regardless of
   *       the case variations of the input string. A particular problem is the behavior
   *       of "The Turkic I". FOLD_CASE_DEFAULT is effective at
   *       eliminating this problem. Below are the four "I" characters in
   *       Turkic and the result of FoldCase for each:
   *
   * Locale                    Unicode                                      Unicode
   *                           codepoint                                    (hex 32-bit codepoint(s))
   * en_US I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
   * tr_TR I (Dotless I)       \u0049 -> ı (Dotless small I)                \u0131
   * FOLD1 I (Dotless I)       \u0049 -> i (Dotted small I)                 \u0069
   * FOLD2 I (Dotless I)       \u0049 -> ı (Dotless small I)                \u0131
   *
   * en_US i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   * tr_TR i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   * FOLD1 i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   * FOLD2 i (Dotted small I)  \u0069 -> i (Dotted small I)                 \u0069
   *
   * en_US İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * tr_TR İ (Dotted I)        \u0130 -> i (Dotted small I)                 \u0069
   * FOLD1 İ (Dotted I)        \u0130 -> i̇ (Dotted small I + Combining dot) \u0069 \u0307
   * FOLD2 İ (Dotted I)        \u0130 -> i (Dotted small I)                 \u0069
   *
   * en_US ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   * tr_TR ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   * FOLD1 ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   * FOLD2 ı (Dotless small I) \u0131 -> ı (Dotless small I)                \u0131
   *
   * FOLD_CASE_DEFAULT causes FoldCase to behave similar to ToLower for the "en" locale
   * FOLD_CASE_SPECIAL_I causes FoldCase to behave similar to ToLower for the "tr_TR" locale.
   *
   * Case folding also ignores insignificant differences in strings (some accent marks,
   * etc.).
   *
   * TODO: Modify to return str (see StartsWithNoCase)
   */
  static void FoldCase(std::wstring& str,
                       const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   *  \brief Folds the case of a string.
   *
   * Similar to ToLower except in addition, insignificant accents are stripped
   * and other transformations are made (such as German sharp-S is converted to ss).
   * The transformation is independent of locale.
   *
   * \param str string to fold
   * \param opt StringOptions to fine-tune behavior. For most purposes, leave at
   *            default value, FOLD_CASE_DEFAULT
   *
   * Note: For more details, see
   *       FoldCase(std::wstring &str, const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);
   *
   *
   * TODO: Modify to return str (see StartsWithNoCase)
   */
  static void FoldCase(std::string& str,
                       const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   *  \brief Capitalizes a wstring using Legacy Kodi rules
   *
   * Uses a simplistic approach familiar to English speakers.
   * See TitleCase for a more locale aware solution.
   *
   * \param str string to capitalize in place
   * \return str capitalized
   */
  static void ToCapitalize(std::wstring& str);

  /*!
   *  \brief Capitalizes a string using Legacy Kodi rules
   *
   * Uses a simplistic approach familiar to English speakers.
   * See TitleCase for a more locale aware solution.
   *
   * \param str string to capitalize in place
   * \return str capitalized
   */
  static void ToCapitalize(std::string& str);

  /*!
   *  \brief TitleCase a wstring using locale.
   *
   *  Similar too, but more language friendly version of ToCapitalize. Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param str string to TitleCase
   *  \param locale
   *  \return str in TitleCase
   */
  static std::wstring TitleCase(const std::wstring& str, const std::locale& locale);

  /*!
   *  \brief TitleCase a wstring using LangInfo::GetSystemLocale.
   *
   *  Similar too, but more language friendly version of ToCapitalize.
   *  Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param str string to TitleCase
   *  \param locale
   *  \return str in TitleCase
   */
  static std::wstring TitleCase(const std::wstring& str);

  /*!
   *  \brief TitleCase a wstring using locale.
   *
   *  Similar too, but more language friendly version of ToCapitalize.
   *  Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param str string to TitleCase
   *  \param locale
   *  \return str in TitleCase
   */
  static std::string TitleCase(const std::string& str, const std::locale& locale);

  /*!
   *  \brief TitleCase a string using LangInfo::GetSystemLocale.
   *
   *  Similar too, but more language friendly version of ToCapitalize.
   *  Uses ICU library.
   *
   *  Best results are when a complete sentence/paragraph is TitleCased rather than
   *  individual words.
   *
   *  \param str string to TitleCase
   *  \param locale
   *  \return str in TitleCase
   */
  static std::string TitleCase(const std::string& str);

  /*!
   *  \brief Determines if two strings are identical in content.
   *
   * Performs a bitwise comparison of the two strings. Locale is
   * not considered.
   *
   * \param str1 one of the strings to compare
   * \param str2 the other string to compare
   * \return true if both strings are identical, otherwise false
   */

  static bool Equals(const std::string& str1, const std::string& str2);

  /*!
   * \brief determines if two wstrings are identical in content.
   *
   * Performs a bitwise comparison of the two strings. Locale is
   * not considered.
   *
   * \param str1 one of the wstrings to compare
   * \param str2 the other wstring to compare
   * \return true if both wstrings are identical, otherwise false
   */
  static bool Equals(const std::wstring& str1, const std::wstring& str2);

  // TODO: Add wstring version of EqualsNoCase
  // TODO: Give guidance on when and what type of Normalization to use.

  /*!
   * \brief Determines if two strings are the same, after case folding each.
   *
   * Logically equivalent to Equals(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if Normalize == true: Equals(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are Normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using Normalize
   * should yield better results for those cases where it is required.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \param opt StringOptions to apply. Generally leave at default value.
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return true if both strings compare after case folding, otherwise false
   */
  static bool EqualsNoCase(const std::string& str1,
                           const std::string& str2,
                           const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                           const bool Normalize = false);

  /*!
   * \brief Determines if two strings are the same, after case folding each.
   *
   * Logically equivalent to Equals(FoldCase(str1, opt)), FoldCase(s2, opt))
   * or, if Normalize == true: Equals(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(s2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are Normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using Normalize
   * should yield better results for those cases where it is required.
   *
   * \param str1 one of the strings to compare
   * \param s2 one of the (c-style) strings to compare
   * \param opt StringOptions to apply. Generally leave at default value.
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return true if both strings compare after case folding, otherwise false
   */
  static bool EqualsNoCase(const std::string& str1,
                           const char* s2,
                           const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                           const bool Normalize = false);

  /*!
   * \brief Determines if two strings are the same, after case folding each.
   *
   * Logically equivalent to Equals(FoldCase(s1, opt)), FoldCase(s2, opt))
   * or, if Normalize == true: Equals(NFD(FoldCase(NFD(s1))), NFD(FoldCase(NFD(s2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are Normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using Normalize
   * should yield better results for those cases where it is required.
   *
   * \param s1 one of the (c-style) strings to compare
   * \param s2 one of the (c-style) strings to compare
   * \param opt StringOptions to apply. Generally leave at default value.
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return true if both strings compare after case folding, otherwise false
   */
  static bool EqualsNoCase(const char* s1,
                           const char* s2,
                           StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                           const bool Normalize = false);

  /*!
   * \brief Compares two wstrings using codepoint order. Locale does not matter.
   *
   * DO NOT USE for collation
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \return <0 or 0 or >0 as usual for string comparisons
   */
  static int Compare(const std::wstring& str1, const std::wstring& str2);

  /*!
   * \brief Compares two strings using codepoint order. Locale does not matter.
   *
   * DO NOT USE for collation
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \return <0 or 0 or >0 as usual for string comparisons
   */
  static int Compare(const std::string& str1, const std::string& str2);

  /*!
   * \brief Performs a bit-wise comparison of two wstrings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if Normalize == true: Compare(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are Normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using Normalize
   * may yield better results.
   *
   * \param str1 one of the wstrings to compare
   * \param str2 one of the wstrings to compare
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters str1 are bitwise less than the characters in str2,
   * = 0 if str1 contains the same characters as str2,
   * > 0 if the characters in str1 are bitwise greater than the characters in str2.
   */
  static int CompareNoCase(const std::wstring& str1,
                           const std::wstring& str2,
                           StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                           const bool Normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if Normalize == true: Compare(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are Normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using Normalize
   * may yield better results.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters str1 are bitwise less than the characters in str2,
   * = 0 if str1 contains the same characters as str2,
   * > 0 if the characters in str1 are bitwise greater than the characters in str2.
   */
  static int CompareNoCase(const std::string& str1,
                           const std::string& str2,
                           StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                           const bool Normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(s1, opt)), FoldCase(s2, opt))
   * or, if Normalize == true: Compare(NFD(FoldCase(NFD(s1))), NFD(FoldCase(NFD(s2))))
   * (NFD is a type of normalization)
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are Normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using Normalize
   * may yield better results.
   *
   * \param s1 one of the strings to compare
   * \param s2 one of the strings to compare
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters s1 are bitwise less than the characters in s2,
   * = 0 if s1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */
  static int CompareNoCase(const char* s1,
                           const char* s2,
                           StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                           const bool Normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(str1, opt)), FoldCase(str2, opt))
   * or, if Normalize == true: Compare(NFD(FoldCase(NFD(str1))), NFD(FoldCase(NFD(str2))))
   * (NFD is a type of normalization)
   *
   * Note: Use of the byte-length argument n is STRONGLY discouraged since
   * it can easily result in malformed Unicode. Further, byte-length does not
   * correlate to character length in multi-byte languages.
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are Normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using Normalize
   * may yield better results.
   *
   * \param str1 one of the strings to compare
   * \param str2 one of the strings to compare
   * \param n maximum number of bytes to compare. A value of 0 means no limit
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters str1 are bitwise less than the characters in str2,
   * = 0 if str1 contains the same characters as str2,
   * > 0 if the characters in str1 are bitwise greater than the characters in str2.
   */
  [[deprecated("StartsWith/EndsWith may be better choices. Multibyte characters, case folding and "
               "byte lengths don't mix.")]] static int
  CompareNoCase(const std::string& str1,
                const std::string& str2,
                size_t n,
                StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                const bool Normalize = false);

  /*!
   * \brief Performs a bit-wise comparison of two strings, after case folding each.
   *
   * Logically equivalent to Compare(FoldCase(s1, opt)), FoldCase(s2, opt))
   * or, if Normalize == true: Compare(NFD(FoldCase(NFD(s1))), NFD(FoldCase(NFD(s2))))
   * (NFD is a type of normalization)
   *
   * NOTE: Limiting the number of bytes to compare via the option n may produce
   *       unexpected results for multi-byte characters.
   *
   * Note: When normalization = true, the string comparison is done incrementally
   * as the strings are Normalized and folded. Otherwise, case folding is applied
   * to the entire string first.
   *
   * Note In most cases normalization should not be required, using Normalize
   * may yield better results.
   *
   * \param s1 one of the strings to compare
   * \param s2 one of the strings to compare
   * \param n maximum number of bytes to compare.  A value of 0 means no limit
   * \param opt StringOptions to apply. Generally leave at the default value
   * \param Normalize Controls whether normalization is performed before and after
   *        case folding
   * \return The result of bitwise character comparison:
   * < 0 if the characters s1 are bitwise less than the characters in s2,
   * = 0 if s1 contains the same characters as s2,
   * > 0 if the characters in s1 are bitwise greater than the characters in s2.
   */

  [[deprecated("StartsWith/EndsWith may be better choices. Multibyte characters, case folding and "
               "byte lengths don't mix.")]] static int
  CompareNoCase(const char* s1,
                const char* s2,
                size_t n,
                StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                const bool Normalize = false);

  /*!
   * \brief Get the leftmost side of a UTF-8 string, using the character
   *        boundary rules from the current icu Locale.
   *
   * Unicode characters are of variable length. This function's
   * parameters are based on characters and NOT bytes. Byte-length can change during
   * processing (from normalization).
   *
   * \param str to get a substring of
   * \param charCount if keepLeft: charCount is number of characters to
   *                  copy from left end (limited by str length)
   *                  if ! keepLeft: number of characters to omit from right end
   * \param keepLeft controls how charCount is interpreted
   * \return leftmost characters of string, length determined by charCount
   *
   * Ex: Copy all but the rightmost two characters from str:
   *
   * std::string x = Left(str, 2, false);
   */
  static std::string Left(const std::string& str,
                          const size_t charCount,
                          const bool keepLeft = true);

  /*!
   * \brief Get the leftmost side of a UTF-8 string, using character boundary
   * rules defined by the given locale.
   *
   * Unicode characters are of variable length. This function's
   * parameters are based on characters and NOT bytes. Byte-length can change during
   * processing (from normalization).
   *
   * \param str to get a substring of
   * \param charCount if keepLeft: charCount is number of characters to
   *                  copy from left end (limited by str length)
   *                  if ! keepLeft: number of characters to omit from right end
   * \param icuLocale determines where the character breaks are
   * \param keepLeft controls how charCount is interpreted
   * \return leftmost characters of string, length determined by charCount
   *
   * Ex: Copy all but the rightmost two characters from str:
   *
   * std::string x = Left(str, 2, false, Unicode::GetDefaultICULocale());
   */

  static std::string Left(const std::string& str,
                          const size_t charCount,
                          const icu::Locale& icuLocale,
                          const bool keepLeft = true);

  /*!
   * \brief Get a substring of a UTF-8 string using character boundary rules
   * defined by the current icu::Locale.
   *
   * Unicode characters may consist of multiple codepoints. This function's
   * parameters are based on characters and NOT bytes. Due to normalization,
   * the byte-length of the strings may change, although the character counts
   * will not.
   *
   * \param str string to extract substring from
   * \param startChar the leftmost n-th character (0-based) in str to include in substring
   * \param charCount number of characters to include in substring (the actual number
   *                  of characters copied is limited by the length of str)
   * \return substring of str, beginning with character 'startChar',
   *         length determined by charCount
   */
  static std::string Mid(const std::string& str,
                         const size_t startChar,
                         const size_t charCount = std::string::npos);

  /*!
   * \brief Get the rightmost end of a UTF-8 string, using character boundary
   * rules defined by the current icu Locale.
   *
   * Unicode characters may consist of multiple codepoints. This function's
   * parameters are based on characters and NOT bytes. Due to normalization,
   * the byte-length of the strings may change, although the character counts
   * will not.
   *
   * \param str to get a substring of
   * \param charCount if keepRight: charCount is number of characters to
   *                  copy from right end (limited by str length)
   *                  if ! keepRight: charCount number of characters to omit from right end
   * \param keepRight controls how charCount is interpreted
   * \return rightmost characters of string, length determined by charCount
   *
   * Ex: Copy all but the leftmost two characters from str:
   *
   * std::string x = Right(str, 2, false);
   */
  static std::string Right(const std::string& str, const size_t charCount, bool keepRight = true);

  /*!
   * \brief Get the rightmost end of a UTF-8 string, using character boundary
   * rules defined by the given locale.
   *
   * Unicode characters may consist of multiple codepoints. This function's
   * parameters are based on characters and NOT bytes. Due to normalization,
   * the byte-length of the strings may change, although the character counts
   * will not.
   *
   * \param str to get a substring of
   * \param charCount if keepRight: charCount is number of characters to
   *                  copy from right end (limited by str length)
   *                  if ! keepRight: charCount number of characters to omit from right end
   * \param icuLocale determines character boundaries
   * \param keepRight controls how charCount is interpreted
   * \return rightmost characters of string, length determined by charCount
   *
   * Ex: Copy all but the leftmost two characters from str:
   *
   * std::string x = Right(str, 2, false, Unicode::GetDefaultICULocale());
   */
  static std::string Right(const std::string& str,
                           const size_t charCount,
                           const icu::Locale& icuLocale,
                           bool keepRight = true);

  /*!
   * \brief Gets the byte-offset of a Unicode character relative to a reference
   *
   * This function is primarily used by Left, Right and Mid. See comment at end for details on use.
   *
   *  Unicode characters may consist of multiple codepoints. This function's parameters
   * are based on characters NOT bytes.
   *
   * \param str UTF-8 string to get index from
   * \param charCount number of characters from reference point to get byte index for
   * \param left + keepLeft define how character index is measured. See comment below
   * \param keepLeft + left define how character index is measured. See comment below
   * \param icuLocale fine-tunes character boundary rules
   * \return code-unit index, relative to str for the given character count
   *                  Unicode::BEFORE_START or Unicode::AFTER::END is returned if
   *                  charCount exceeds the string's length. std::string::npos is
   *                  returned on other errors.
   *
   * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
   * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
   * left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
   * left=false keepLeft=false  Returns offset of first byte of nth char from right end.
   *                            Character 0 is AFTER the last character.  Used by Right(x)
   */
  static size_t GetCharPosition(const std::string& str,
                                size_t charCount,
                                const bool left,
                                const bool keepLeft,
                                const icu::Locale& icuLocale);
  /*!
   * \brief Gets the byte-offset of a Unicode character relative to a reference
   *
   * This function is primarily used by Left, Right and Mid. See comment at end for details on use.
   *
   * Unicode characters may consist of multiple codepoints. This function's parameters
   * are based on characters NOT bytes.
   *
   * \param str UTF-8 string to get index from
   * \param charCount number of characters from reference point to get byte index for
   * \param left + keepLeft define how character index is measured. See comment below
   * \param keepLeft + left define how character index is measured. See comment below
   *                   std::string::npos is returned if charCount is outside of the string
   * \return code-unit index, relative to str for the given character count
   *                  Unicode::BEFORE_START or Unicode::AFTER::END is returned if
   *                  charCount exceeds the string's length. std::string::npos is
   *                  returned on other errors.
   *
   * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
   * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
   * left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
   * left=false keepLeft=false  Returns offset of first byte of nth char from right end.
   *                            Character 0 is AFTER the last character.  Used by Right(x)
   */
  static size_t GetCharPosition(const std::string& str,
                                size_t charCount,
                                const bool left,
                                const bool keepLeft,
                                const std::locale& locale);

  /*!
   * \brief Gets the byte-offset of a Unicode character relative to a reference
   *
   * This function is primarily used by Left, Right and Mid. See comment at end for details on use.
   * The currently configured locale is used to tweak character boundaries.
   *
   * Unicode characters may consist of multiple codepoints. This function's parameters
   * are based on characters NOT bytes.
   *
   * \param str UTF-8 string to get index from
   * \param charCount number of characters from reference point to get byte index for
   * \param left + keepLeft define how character index is measured. See comment below
   * \param keepLeft + left define how character index is measured. See comment below
   * \return code-unit index, relative to str for the given character count
   *                  Unicode::BEFORE_START or Unicode::AFTER::END is returned if
   *                  charCount exceeds the string's length. std::string::npos is
   *                  returned on other errors.
   *
   * left=true  keepLeft=true   Returns offset of last byte of nth character (0-n). Used by Left.
   * left=true  keepLeft=false  Returns offset of last byte of nth character from right end (0-n). Used by Left(x, false)
   * left=false keepLeft=true   Returns offset of first byte of nth character (0-n). Used by Right(x, false)
   * left=false keepLeft=false  Returns offset of first byte of nth char from right end.
   *                            Character 0 is AFTER the last character.  Used by Right(x)
   */
  static size_t GetCharPosition(const std::string& str,
                                size_t charCount,
                                const bool left,
                                const bool keepLeft);

  /*!
   * \brief Get the rightmost side of a UTF-8 string, using character boundary
   * rules defined by the given locale.
   *
   * Unicode characters may consist of multiple codepoints. This function's
   * parameters are based on characters and NOT bytes. Due to normalization,
   * the byte-length of the strings may change, although the character counts
   * will not.
   *
   * \param str to get a substring of
   * \param charCount if rightReference: charCount is number of characters to
   *                  copy from right end (limited by str length)
   *                  if ! rightReference: number of characters to omit from left end
   * \param rightReference controls how charCount is interpreted
   * \param icuLocale determines how character breaks are made
   * \return rightmost characters of string, length determined by charCount
   *
   * Ex: Copy all but the leftmost two characters from str:
   *
   * std::string x = Right(str, 2, false, Unicode::GetDefaultICULocale());
   */
  static std::string Right(const std::string& str,
                           const size_t charCount,
                           bool rightReference,
                           const icu::Locale& icuLocale);

  /*!
   * \brief Remove all whitespace from beginning and end of str in-place
   *
   * \param str to trim
   *
   * Whitespace is defined for Unicode as:  [\t\n\f\r\p{Z}] where \p{Z} means marked as white space
   * which includes ASCII space, plus a number of Unicode space characters. See Unicode::Trim for
   * a complete list.
   *
   * \return trimmed string, same as str argument.
   *
   * Note: Prior to Kodi 20 Trim defined whitespace as: isspace() which is [ \t\n\v\f\r] and
   *       as well as other characters, depending upon locale.
   */
  static std::string& Trim(std::string& str);

  /*!
   * \brief Remove a set of characters from beginning and end of str in-place
   *
   *  Remove any leading or trailing characters from the set chars from str.
   *
   *  Ex: Trim("abc1234bxa", "acb") ==> "1234bx"
   *
   * \param str to trim
   * \param chars characters to remove from str
   * \return trimmed string, same as str argument.
   *
   * Note: Prior algorithm only supported chars containing ASCII characters.
   * This implementation allows for chars to be any utf-8 characters. (Does NOT Normalize).
   */
  static std::string& Trim(std::string& str, const char* const chars);

  /*!
   *  \brief Remove all whitespace from beginning of str in-place
   *
   *  See UnicodeUtils::Trim(str) for a description of whitespace characters
   *
   * \param str to trim
   * \return trimmed string, same as str argument.
   */
  static std::string& TrimLeft(std::string& str);

  /*!
   * \brief Remove a set of characters from beginning of str in-place
   *
   *  Remove any leading characters from the set chars from str.
   *
   *  Ex: TrimLeft("abc1234bxa", "acb") ==> "1234bxa"
   *
   * \param str to trim
   * \param chars (characters) to remove from str
   * \return trimmed string, same as str argument.
   *
   * Note: Prior algorithm only supported chars containing ASCII characters.
   * This implementation allows for chars to be any utf-8 characters. (Does NOT Normalize).
   */
  static std::string& TrimLeft(std::string& str, const char* const chars);

  /*!
   * \brief Remove all whitespace from end of str in-place
   *
   * See Trim(str) for information about what characters are considered whitespace
   *
   * \param str to trim
   * \return trimmed string, same as str argument.
   */
  static std::string& TrimRight(std::string& str);

  /*!
   *  \brief Remove trailing characters from the set of chars from str in-place
   *
   *  Ex: TrimRight("abc1234bxa", "acb") ==> "abc1234bx"
   *
   * \param str to trim
   * \param chars (characters) to remove from str
   * \return trimmed string, same as str argument.
   *
   * Note: Prior algorithm only supported chars containing ASCII characters.
   * This implementation allows for chars to be any utf-8 characters. (Does NOT Normalize).
   */
  static std::string& TrimRight(std::string& str, const char* const chars);

  /*!
   * \brief Replaces every occurrence of a char in string.
   *
   * Somewhat less efficient than FindAndReplace because this one returns a count
   * of the number of changes.
   *
   * \param str String to make changes to in-place
   * \param oldChar character to be replaced
   * \param newChar character to replace with
   * \return Count of the number of changes
   */
  [[deprecated("FindAndReplace is faster, returned count not used.")]] static int Replace(
      std::string& str, char oldChar, char newChar);

  /*!
   * \brief Replaces every occurrence of a string within another string.
   *
   * Somewhat less efficient than FindAndReplace because this one returns a count
   * of the number of changes.
   *
   * \param str String to make changes to in-place
   * \param oldStr string to be replaced
   * \param newStr string to replace with
   * \return Count of the number of changes
   */
  [[deprecated("FindAndReplace is faster, returned count not used.")]] static int Replace(
      std::string& str, const std::string& oldStr, const std::string& newStr);

  /*!
   * \brief Replaces every occurrence of a wstring within another wstring in-place
   *
   *  Somewhat less efficient than FindAndReplace because this one returns a count
   *  of the number of changes.
   *
   * \param str String to make changes to
   * \param oldStr string to be replaced
   * \parm newStr string to replace with
   * \return Count of the number of changes
   */
  [[deprecated("FindAndReplace is faster, returned count not used.")]] static int Replace(
      std::wstring& str, const std::wstring& oldStr, const std::wstring& newStr);

  /*!
   * \brief Replaces every occurrence of a string within another string
   *
   * Should be more efficient than Replace since it directly uses an icu library
   * routine and does not have to count changes.
   *
   * \param str String to make changes to
   * \param oldStr string to be replaced
   * \parm newStr string to replace with
   * \return the modified string.
   */
  static std::string FindAndReplace(const std::string& str,
                                    const std::string oldText,
                                    const std::string newText);

  /*!
   * \brief Replaces every occurrence of a string within another string.
   *
   * Should be more efficient than Replace since it directly uses an icu library
   * routine and does not have to count changes.
   *
   * \param str String to make changes to
   * \param oldStr string to be replaced
   * \parm newStr string to replace with
   * \return the modified string
   */
  static std::string FindAndReplace(const std::string& str,
                                    const char* oldText,
                                    const char* newText);

  /*!
   * \brief Replaces every occurrence of a regex pattern with a string in another string.
   *
   * Regex based version of Replace. See:
   * https://unicode-org.github.io/icu/userguide/strings/regexp.html
   *
   * \param str string being altered
   * \param pattern regular expression pattern
   * \param newStr new value to replace with
   * \param flags controls behavior of regular expression engine
   * \return result of regular expression
   */
  std::string RegexReplaceAll(const std::string& str,
                              const std::string pattern,
                              const std::string newStr,
                              const int flags);

  /*!
   * \brief Determines if a string begins with another string
   *
   * \param str1 string to be searched
   * \param str2 string to find at beginning of str1
   * \return true if str1 starts with str2, otherwise false
   *
   * Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
   */
  static bool StartsWith(const std::string& str1, const std::string& str2);

  /*!
   * \brief Determines if a string begins with another string
   *
   * \param str1 string to be searched
   * \param s2 string to find at beginning of str1
   * \return true if str1 starts with s2, otherwise false
   *
   *  Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
   */
  static bool StartsWith(const std::string& str1, const char* s2);

  /*!
   * \brief Determines if a string begins with another string
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of str1
   * \return true if s1 starts with s2, otherwise false
   *
   *  Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
   *
   */
  static bool StartsWith(const char* s1, const char* s2);

  /*!
   * \brief Determines if a string begins with another string, ignoring their case
   *
   * Equivalent to StartsWith(FoldCase(str1), FoldCase(str2))
   *
   * \param str1 string to be searched
   * \param str2 string to find at beginning of str1
   * \param opt controls behavior of case folding, normally leave at default
   * \return true if str1 starts with str2, otherwise false
   *
   * Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
   */
  static bool StartsWithNoCase(const std::string& str1,
                               const std::string& str2,
                               StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   * \brief Determines if a string begins with another string, ignoring their case
   *
   * Equivalent to StartsWith(FoldCase(str1), FoldCase(s2))
   *
   * \param str1 string to be searched
   * \param s2 string to find at beginning of str1
   * \param opt controls behavior of case folding, normally leave at default
   * \return true if str1 starts with s2, otherwise false
   *
   * Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
   */
  static bool StartsWithNoCase(const std::string& str1,
                               const char* s2,
                               StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   * \brief Determines if a string begins with another string, ignoring their case
   *
   * Equivalent to StartsWith(FoldCase(s1), FoldCase(s2))
   *
   * \param s1 string to be searched
   * \param s2 string to find at beginning of s1
   * \param opt controls behavior of case folding, normally leave at default
   * \return true if s1 starts with s2, otherwise false
   */
  static bool StartsWithNoCase(const char* s1,
                               const char* s2,
                               StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   * \brief Determines if a string ends with another string
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \return true if str1 ends with str2, otherwise false
   *
   * Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
   */
  static bool EndsWith(const std::string& str1, const std::string& str2);

  /*!
   * \brief Determines if a string ends with another string
   *
   * \param str1 string to be searched
   * \param s2 string to find at end of str1
   * \return true if str1 ends with s2, otherwise false
   *
   * Note: Embedded nulls in str1 or str2 behaves as a null-terminated string behaves
   */
  static bool EndsWith(const std::string& str1, const char* s2);

  /*! \brief Determines if a string ends with another string while ignoring case
   *
   * \param str1 string to be searched
   * \param str2 string to find at end of str1
   * \param opt controls behavior of case folding
   * \return true if str1 ends with str2, otherwise false
   */
  static bool EndsWithNoCase(const std::string& str1,
                             const std::string& str2,
                             StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   * \brief Determines if a string begins with another string while ignoring case
   *
   * \param str1 string to be searched
   * \param s2 string to find at beginning of str1
   * \param opt controls behavior of case folding, normally leave at default
   * \return true if str1 starts with s2, otherwise false
   */
  static bool EndsWithNoCase(const std::string& str1,
                             const char* s2,
                             StringOptions opt = StringOptions::FOLD_CASE_DEFAULT);

  /*!
   *  \brief Normalizes a wstring. Not expected to be used outside of UnicodeUtils.
   *
   *  Made public to facilitate testing.
   *
   *  There are multiple Normalizations that can be performed on Unicode. Fortunately
   *  normalization is not needed in many situations. An introduction can be found
   *  at: https://unicode-org.github.io/icu/userguide/transforms/normalization/
   *
   *  \param str string to Normalize.
   *  \param options fine tunes behavior. See StringOptions. Frequently can leave
   *         at default value.
   *  \param NormalizerType select the appropriate Normalizer for the job
   *  \return Normalized string
   */

  static const std::wstring Normalize(const std::wstring& src,
                                      const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                                      const NormalizerType NormalizerType = NormalizerType::NFKC);

  /*!
   *  \brief Normalizes a string. Not expected to be used outside of UnicodeUtils.
   *
   *  Made public to facilitate testing.
   *
   * There are multiple Normalizations that can be performed on Unicode. Fortunately
   * normalization is not needed in many situations. An introduction can be found
   * at: https://unicode-org.github.io/icu/userguide/transforms/normalization/
   *
   * \param str string to Normalize.
   * \param options fine tunes behavior. See StringOptions. Frequently can leave
   *        at default value.
   * \param NormalizerType select the appropriate Normalizer for the job
   * \return Normalized string
   */

  static const std::string Normalize(const std::string& src,
                                     const StringOptions opt = StringOptions::FOLD_CASE_DEFAULT,
                                     const NormalizerType NormalizerType = NormalizerType::NFKC);

  /*!
   * \brief Initializes the Collator for this thread, such as before sorting a
   * table.
   *
   * Assumes that all collation will occur in this thread.
   *
   * Note: Only has an impact if icu collation is configured instead of legacy
   *       AlphaNumericCompare.
   *
   *       Also starts the elapsed-time timer for the sort. See SortCompleted.
   *
   * \param icuLocale Collation order will be based on the given locale.
   * \param Normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true if initialization was successful, otherwise false.
   */
  static bool InitializeCollator(const icu::Locale& icuLocale, bool Normalize = false);

  /*!
   * \brief Initializes the Collator for this thread, such as before sorting a
   * table.
   *
   * Assumes that all collation will occur in this thread.
   *
   * Note: Only has an impact if icu collation is configured instead of legacy
   *       AlphaNumericCompare.
   *
   *       Also starts the elapsed-time timer for the sort. See SortCompleted.
   *
   * \param locale Collation order will be based on the given locale.
   * \param Normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true if initialization was successful, otherwise false.
   */
  static bool InitializeCollator(const std::locale& locale, bool Normalize = false);

  /*!
   * \brief Initializes the Collator for this thread using LangInfo::GetSystemLocale,
   * such as before sorting a table.
   *
   * Assumes that all collation will occur in this thread.
   *
   *
   * Note: Only has an impact if icu collation is configured instead of legacy
   *       AlphaNumericCompare.
   *
   *       Also starts the elapsed-time timer for the sort. See SortCompleted.
   *
   * \param Normalize Controls whether normalization is performed prior to collation.
   *                  Frequently not required. Some free normalization always occurs.
   * \return true of initialization was successful, otherwise false.
   */
  static bool InitializeCollator(bool Normalize = false);

  /*!
   * \brief Provides the ability to collect basic performance info for the previous sort
   *
   * Must be run in the same thread that InitializeCollator was run. May require some setting
   * or #define to be set to enable recording of data in log.
   *
   * \param sortItems simple count of how many items sorted.
   */
  static void SortCompleted(int sortItems);

  /*!
   * \brief Performs locale sensitive string comparison.
   *
   * Must be run in the same thread that InitializeCollator that configured the Collator
   * for this was run.
   *
   * \param left string to compare
   * \param right string to compare
   * \return  < 0 if left collates < right
   *         == 0 if left collates the same as right
   *          > 0 if left collates > right
   */

  static int32_t Collate(const std::wstring& left, const std::wstring& right);

  /*!
   * \brief Performs locale sensitive wchar_t* comparison.
   *
   * Must be run in the same thread that InitializeCollator that configured the Collator
   * for this was run.
   *
   * \param left string to compare
   * \param right string to compare
   * \return  < 0 if left collates < right
   *         == 0 if left collates the same as right
   *          > 0 if left collates > right
   */
  static int32_t Collate(const wchar_t* left, const wchar_t* right)
  {
    return UnicodeUtils::Collate(std::wstring(left), std::wstring(right));
  }

  /*!
   * \brief Splits the given input string into separate strings using the given delimiter.
   *
   * If the given input string is empty the result will be an empty vector (not
   * a vector containing an empty string).
   *
   * \param input string to be split
   * \param delimiter used to split the input string
   * \param iMaxStrings (optional) Maximum number of generated split strings
   */
  static std::vector<std::string> Split(const std::string& input,
                                        const std::string& delimiter,
                                        size_t iMaxStrings = 0);

  /*!
   *  \brief Splits the given input string into separate strings using the given delimiter.
   *
   * If the given input string is empty the result will be an empty vector (not
   * an vector containing an empty string).
   *
   * If iMaxStrings limit is reached, the unprocessed input is returned along with the
   * incomplete split strings.
   *
   *  Ex: input = "a/b#c/d/e/f/g/h"
   *     delimiter = "/"
   *     iMaxStrings = 5
   *     returned = {"a", "b#c", "d", "e", "f", "/g/h"}
   *
   * \param input string to be split
   * \param delimiter used to split the input string
   * \param iMaxStrings Maximum number of generated split strings. The default value
   *        of 0 places no limit on the generated strings.
   */
  static std::vector<std::string> Split(const std::string& input,
                                        const char delimiter,
                                        size_t iMaxStrings = 0);

  /*!
   * \brief Splits the given input string into separate strings using the given delimiters.
   *
   * \param input string to be split
   * \param delimiters used to split the input string as described above
   * \return a Vector of substrings
   */
  static std::vector<std::string> Split(const std::string& input,
                                        const std::vector<std::string>& delimiters);

  /*!
   * \brief Splits each input string with each delimiter string producing a vector of split strings
   * omitting any null strings.
   *
   * \param input vector of strings to be split
   * \param delimiters when found in a string, a delimiter splits a string into two strings
   * \param iMaxStrings (optional) Maximum number of resulting split strings
   * \result the accumulation of all split strings (and any unfinished splits, due to iMaxStrings
   *          being exceeded) omitting null strings
   *
   *
   * SplitMulti is essentially equivalent to running Split(string, vector<delimiters>, maxstrings) over multiple
   * strings with the same delimiters and returning the aggregate results. Null strings are not returned.
   *
   * There are some significant differences when maxstrings alters the process. Here are the boring details:
   *
   * For each delimiter, Split(string<n> input, delimiter, maxstrings) is called for each input string.
   * The results are appended to results <vector<string>>
   *
   * After a delimiter has been applied to all input strings, the process is repeated with the
   * next delimiter, but this time with the vector<string> input being replaced with the
   * results of the previous pass.
   *
   * If the maxstrings limit is not reached, then, as stated above, the results are similar to
   * running Split(string, vector<delimiters> maxstrings) over multiple strings. But when the limit is reached
   * differences emerge.
   *
   * Before a delimiter is applied to a string a check is made to see if maxstrings is exceeded. If so,
   * then splitting stops and all split string results are returned, including any strings that have not
   * been split by as many delimiters as others, leaving the delimiters in the results.
   *
   * Differences between current behavior and prior versions: Earlier versions removed most empty strings,
   * but a few slipped past. Now, all empty strings are removed. This means not as many empty strings
   * will count against the maxstrings limit. This change should cause no harm since there is no reliable
   * way to correlate a result with an input; they all get thrown in together.
   *
   * If an input vector element is empty the result will be an empty vector element (not
   * an empty string).
   *
   * Examples:
   *
   * Delimiter strings are applied in order, so once iMaxStrings
   * items is produced no other delimiters are applied. This produces different results
   * than applying all delimiters at once:
   *
   * Ex: input = {"a/b#c/d/e/foo/g::h/", "#p/q/r:s/x&extraNarfy"}
   *     delimiters = {"/", "#", ":", "Narf"}
   *     if iMaxStrings=7
   *        return value = {"a", "b#c", "d" "e", "foo", "/g::h/", "p", "q", "/r:s/x&extraNarfy}"
   *
   *     if iMaxStrings=0
   *        return value = {"a", "b", "c", "d", "e", "f", "g", "", "h", "", "", "p", "q", "r", "s",
   *                        "x&extra", "y"}
   *
   * e.g. "a/b#c/d" becomes "a", "b#c", "d" rather
   * than "a", "b", "c/d"
   *
   * \param input vector of strings each to be split
   * \param delimiters strings to be used to split the input strings
   * \param iMaxStrings limits number of resulting split strings. A value of 0
   *        means no limit.
   * \return vector of split strings
   */
  static std::vector<std::string> SplitMulti(const std::vector<std::string>& input,
                                             const std::vector<std::string>& delimiters,
                                             size_t iMaxStrings = 0);
  /*! \brief Counts the occurrences of strFind in strInput
   *
   * \param strInput string to be searched
   * \param strFind string to count occurrences in strInput
   * \return count of the number of occurrences found
   */
  static int FindNumber(const std::string& strInput, const std::string& strFind);

  /*! \brief Compares two strings based on the rules of the given locale
   *
   *  TODO: DRAFT
   *
   *  This is a complex comparison. Broadly speaking, it tries to compare
   *  numbers using math rules and Alphabetic characters as words in a caseless
   *  manner. Punctuation characters, etc. are also included in comparison.
   *
   * \param left string to compare
   * \param right other string to compare
   * \param locale supplies rules for comparison
   * \return < 0 if left < right based upon comparison based on comparison rules
   */
  static int64_t AlphaNumericCompare(const wchar_t* left,
                                     const wchar_t* right,
                                     const std::locale& locale);

  /*! \brief Compares two strings based on the rules of LocaleInfo.GestSystemLocale
   *
   *  TODO: DRAFT
   *
   *  This is a complex comparison. Broadly speaking, it tries to compare
   *  numbers using math rules and Alphabetic characters as words in a caseless
   *  manner. Punctuation characters, etc. are also included in comparison.
   *
   * \param left string to compare
   * \param right other string to compare
   * \param locale supplies rules for comparison
   * \return < 0 if left < right based upon comparison based on comparison rules
   */
  static int64_t AlphaNumericCompare(const wchar_t* left, const wchar_t* right);

  /*!
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
   * \brief converts timeString (hh:mm:ss or nnn min) to seconds.
   *
   * \param timeString string to convert to seconds may be in "hh:mm:ss" or "nnn min" format
   *                   missing values are assumed zero. Whitespace is trimmed first.
   * \return parsed value in seconds
   *
   *   ex: " 14:57 " or "  23 min"
   */
  static long TimeStringToSeconds(const std::string& timeString);

  /*!
   * \brief Strip any trailing \n and \r characters.
   *
   * \param strLine input string to have consecutive trailing \n and \r characters removed in-place
   */
  static void RemoveCRLF(std::string& strLine);

  /*!
   * \brief detects when a string contains non-ASCII to aide in debugging or error reporting
   *
   * \param str String to be examined for non-ASCII
   * \return true if non-ASCII characters found, otherwise false
   */
  inline static bool ContainsNonAscii(std::string str)
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
  inline static bool ContainsNonAscii(std::wstring str)
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
   * \brief Determine if "word" is present in string
   *
   * \param str string to search
   * \param word to search for in str
   * \return true if word found, otherwise false
   *
   * Search algorithm:
   *   Both str and word are case-folded (see FoldCase)
   *   For each character in str
   *     Return false if word not found in str
   *     Return true if word found starting at beginning of substring
   *     If partial match found:
   *      If non-matching character is a digit, then skip past every
   *      digit in str. Same for Latin letters. Otherwise, skip one character
   *      Skip any whitespace characters
   */

  static bool FindWord(const std::string& str, const std::string& word);

  /*!
   * \brief Converts a date string into an integer format
   *
   * \param dateString to be converted. See note
   * \return integer format of dateString. See note
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
};

struct sortstringbyname
{
  bool operator()(const std::string& strItem1, const std::string& strItem2) const
  {
    return UnicodeUtils::CompareNoCase(strItem1, strItem2) < 0;
  }
};
