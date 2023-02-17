/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>

class CUnicodeConverter
{
  /*!
   * \brief Wrapper for iconv to provide character conversions between several
   *        Unicode encodings: UTF-8, UTF-32, "wstring" (UTF-32, UCS-4)
   *
   *  Borrows from CharSetConverter, but written to overcome several shortcomings
   *  of CharSetConverter. For details see UnicodeConverter.cpp
   */
public:
  CUnicodeConverter() = delete;

  /*!
   * \brief Convert UTF-8 string to UTF-32 string.
   *
   * \param utf8StringSrc           source UTF-8 string to convert
   * \param failOnInvalidChar       if true then conversion will fail on invalid character,
   *                                otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are silently omitted.
   * \return converted u32string on successful conversion, empty string on any error when
   *         failOnInvalidChar is true
   */
  static std::u32string Utf8ToUtf32(std::string_view utf8StringSrc,
                                    bool failOnInvalidChar = false,
                                    bool substituteBadCodepoints = true);

  /*!
   * \brief Convert UTF-32 string to UTF-8 string.
   *
   * \param utf32StringSrc          is source UTF-32 string to convert
   * \param failOnInvalidChar       if true then conversion will fail on invalid character,
   *                                otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are silently omitted.
   * \return converted string on successful conversion, empty string on any error when
   *         failOnInvalidChar is true
   */
  static std::string Utf32ToUtf8(std::u32string_view utf32StringSrc,
                                 bool failOnInvalidChar = false,
                                 bool substituteBadCodepoints = true);

  /*!
   * \brief Convert UTF-32 string to wchar_t string (wstring).
   *
   * \param utf32StringSrc          is source UTF-32 string to convert
   * \param failOnInvalidChar       if true then conversion will fail on invalid character,
   *                                otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are silently omitted.
   * \return converted wstring on successful conversion, empty string on any error when
   *         failOnInvalidChar is true
   */
  static std::wstring Utf32ToW(std::u32string_view utf32StringSrc,
                               bool failOnInvalidChar = false,
                               bool substituteBadCodepoints = true);

  /*!
   * \brief Convert wchar_t string (wstring) to UTF-32 string.
   *
   * \param wStringSrc              is source wchar_t string to convert
   * \param failOnInvalidChar       if true then conversion will fail on invalid character,
   *                                otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are silently omitted.
   * \return converted u32string on successful conversion, empty string on any error when
   *         failOnInvalidChar is true
   */
  static std::u32string WToUtf32(std::wstring_view wStringSrc,
                                 bool failOnInvalidChar = false,
                                 bool substituteBadCodepoints = true);

  /*!
   * \brief Convert wchar_t string (wstring) to UTF-8 string.
   *
   * \param wStringSrc              source wchar_t string to convert
   * \param failOnInvalidChar       if true then conversion will fail on invalid character,
   *                                otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are silently omitted.
   *  \return converted string on successful conversion, empty string on any error when
   *          failOnInvalidChar is true
   */
  static std::string WToUtf8(std::wstring_view wStringSrc,
                             bool failOnInvalidChar = false,
                             bool substituteBadCodepoints = true);

  /*!
   * \brief Convert utf8 string to wstring.
   *
   * \param stringSrc               source utf8 string to convert
   * \param failOnInvalidChar       if true then conversion will fail on invalid character,
   *                                otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are silently omitted.
   * \return converted wstring on successful conversion, empty string on any error when
   *         failOnInvalidChar is true
   */
  static std::wstring Utf8ToW(std::string_view stringSrc,
                              bool failOnInvalidChar = false,
                              bool substituteBadCodepoints = true);
};
