/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>

class CUnicodeConverter // : public ISettingCallback
{
  /*!
   * \brief Wrapper for iconv to provide character conversions between several
   *        Unicode encodings: UTF-8, UTF-32, "wstring" (UTF-32, UCS-4 or UTF-16)
   *
   *  Borrows heavily from CharSetConverter, but written to overcome several shortcommings
   *  of CharSetConverter. For details see UnicodeConverter.cpp
   */
public:
  CUnicodeConverter();

  /*!
   * \brief Convert UTF-8 string to UTF-32 string.
   *
   * \param utf8StringSrc       source UTF-8 string to convert
   * \param failOnBadChar       if true then function will fail on invalid character,
   *                            otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are skipped over.
   * \return converted u32string on successful conversion, empty string on any error when
   *         failOnBadChar is true
   */
  static std::u32string Utf8ToUtf32(const std::string_view utf8StringSrc,
                                    const bool failOnBadChar = false,
                                    const bool substituteBadCodepoints = true);

  /*!
   * \brief Convert UTF-32 string to UTF-8 string.
   *
   * \param utf32StringSrc      is source UTF-32 string to convert
   * \param failOnBadChar       if true then function will fail on invalid character,
   *                            otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are skipped over.
   * \return converted string on successful conversion, empty string on any error when
   *         failOnBadChar is true
   */
  static std::string Utf32ToUtf8(const std::u32string_view utf32StringSrc,
                                 const bool failOnBadChar = false,
                                 const bool substituteBadCodepoints = true);

  /*!
   * \brief Convert UTF-32 string to wchar_t string (wstring).
   *
   * \param utf32StringSrc      is source UTF-32 string to convert
   * \param failOnBadChar       if true then function will fail on invalid character,
   *                            otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are skipped over.
   * \return converted wstring on successful conversion, empty string on any error when
   *         failOnBadChar is true
   */
  static std::wstring Utf32ToW(const std::u32string_view utf32StringSrc,
                               const bool failOnBadChar = false,
                               const bool substituteBadCodepoints = true);

  /*!
   * \brief Strictly convert wchar_t string (wstring) to UTF-32 string.
   *
   * \param wStringSrc          is source wchar_t string to convert
   * \param failOnBadChar       if true then function will fail on invalid character,
   *                            otherwise substituteBadCodepoints will control behavior
   * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
   *                                it is replaced with a semi-official substitution
   *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
   *                                Otherwise, when false, then the bad codepoints/codeunits
   *                                are skipped over.
   * \return converted u32string on successful conversion, empty string on any error when
   *         failOnBadChar is true
   */
  static std::u32string WToUtf32(const std::wstring_view wStringSrc,
                                 const bool failOnBadChar = false,
                                 const bool substituteBadCodepoints = true);

  /*!
    * \brief Convert wchar_t string (wstring) to UTF-8 string.
    *
    * \param wStringSrc          source wchar_t string to convert
    * \param failOnBadChar       if true then function will fail on invalid character,
    *                            otherwise substituteBadCodepoints will control behavior
    * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
    *                                it is replaced with a semi-official substitution
    *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
    *                                Otherwise, when false, then the bad codepoints/codeunits
    *                                are skipped over.
    *  \return converted string on successful conversion, empty string on any error when
    *          failOnBadChar is true
    */
  static std::string WToUtf8(const std::wstring_view wStringSrc,
                             const bool failOnBadChar = false,
                             const bool substituteBadCodepoints = true);

  /*!
    * \brief Convert utf8 string (string) to wstring.
    *
    * \param stringSrc           source utf8 string to convert
    * \param failOnBadChar       if true then function will fail on invalid character,
    *                            otherwise substituteBadCodepoints will control behavior
    * \param substituteBadCodepoints If true, then when an invalid codepoint is encountered
    *                                it is replaced with a semi-official substitution
    *                                codepoint:  U"\x0fffd" which is displayed as: '�'.
    *                                Otherwise, when false, then the bad codepoints/codeunits
    *                                are skipped over.
    * \return converted wstring on successful conversion, empty string on any error when
    *         failOnBadChar is true
    */
  static std::wstring Utf8ToW(const std::string_view stringSrc,
                              const bool failOnBadChar = false,
                              const bool substituteBadCodepoints = true);

private:
  static const int m_Utf8CharMinSize;
  static const int m_Utf8CharMaxSize;
  class CInnerUCodeConverter;
};
