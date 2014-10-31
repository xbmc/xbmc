#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/GlobalsHandling.h"
#include "utils/uXstrings.h"

#include <string>
#include <vector>

/**
 * \class CCharsetConverter
 *
 * Methods for converting text between different encodings
 *
 * There are a few conventions used in the method names that may be
 * beneficial to know about
 * Methods starting with Try* will try to convert the text and fail on any invalid byte sequence
 *
 * Regular methods will silently ignore invalid byte sequences and skip them,
 * should only fail on errors like out of memory or completely invalid input
 *
 * Methods ending with SystemSafe are meant to be used for file system access,
 * they provide additional checks to make sure that output is in a valid state
 * for the platform. They also fail on invalid byte sequences to avoid passing
 * an unkown filename to the system.
 * Currently the only platform with special needs are OSX and iOS which require
 * a special normalization form for it's file names.
 */
class CCharsetConverter
{
public:

  /** Options to specify BiDi text handling */
  enum BiDiOptions
  {
    NO_BIDI = 0,        /**< No BiDi processing */
    LTR = 1,            /**< text is mainly left-to-right */
    RTL = 2,            /**< text is mainly right-to-left */
    WRITE_REVERSE = 4,  /**< text should be reversed, e.g flip rtl text to ltr */
    REMOVE_CONTROLS = 8 /**< bidi control characters such as LRE, RLE, PDF should be removed from the output*/
  };

  /** Options to specify if any normalization should be performed and how*/
  enum NormalizationOptions
  {
    NO_NORMALIZATION = 0, /**< no normalization should be performed */
    COMPOSE = 1,          /**< text should be normalized to NFC e.g. merging ¨a into a single ä */
    DECOMPOSE = 2,        /**< text should be normalized to NFD e.g. splitting ä into ¨a */
    DECOMPOSE_MAC = 4     /**< text should be normalized to OSX specific NFD excluding certain ranges */
  };

  /**
   * Convert UTF-8 string to UTF-32 string.
   *
   * \param[in]  utf8StringSrc       is source UTF-8 string to convert
   * \param[out] utf32StringDst      is output UTF-32 string, empty on any error
   *
   * \return true on successful conversion, false on any error
   */
  static bool Utf8ToUtf32(const std::string& utf8StringSrc, std::u32string& utf32StringDst);
  
  /**
   * Convert UTF-8 string to UTF-32 string.
   * 
   * \param[in] utf8StringSrc       is source UTF-8 string to convert
   *
   * \return converted string on successful conversion, empty string on any error
   */
  static std::u32string Utf8ToUtf32(const std::string& utf8StringSrc);

  /**
  * Convert UTF-8 string to UTF-16 string.
  * 
  * \param[in]  utf8StringSrc       is source UTF-8 string to convert
  * \param[out] utf16StringDst      is output UTF-16 string, empty on any error
  *
  * \return true on successful conversion, false on any error
  */
  static bool Utf8ToUtf16(const std::string& utf8StringSrc, std::u16string& utf16StringDst);

  /**
  * Try to convert UTF-8 string to UTF-16.
  * Will fail on invalid byte sequences.
  *
  * \param[in]  utf8StringSrc       is source UTF-8 string to convert
  * \param[out] utf16StringDst      is output UTF-16 string, empty on any error
  *
  * \return true on successful conversion, false on any error
  * \sa Utf8ToUtf16
  */
  static bool TryUtf8ToUtf16(const std::string& utf8StringSrc, std::u16string& utf16StringDst);
  
  /**
  * Convert UTF-8 string to UTF-16 string.
  * 
  * \param[in] utf8StringSrc       is source UTF-8 string to convert
  *
  * \return converted string on successful conversion, empty string on any error
  */
  static std::u16string Utf8ToUtf16(const std::string& utf8StringSrc);
  
  /**
   * Convert UTF-8 string to UTF-32 string and perform logical to visual processing
   * on the string.
   * Use it for readable text, GUI strings etc.
   *
   * \param[in]  utf8StringSrc        is source UTF-8 string to convert
   * \param[out] utf32StringDst       is output UTF-32 string, empty on any error
   * \param[in]  bidiOptions          options for logical to visual transformation
   *                                  default is LTR | REMOVE_CONTROLS which should be fine
   *                                  in most cases
   *
   * \return true on successful conversion, false on any error
   * \sa CCharsetConverter::BiDiOptions
   * \sa Utf8ToUtf32
   */
  static bool Utf8ToUtf32LogicalToVisual(const std::string& utf8StringSrc, std::u32string& utf32StringDst, 
                                         uint16_t bidiOptions = LTR | REMOVE_CONTROLS);
  
  /**
   * Convert UTF-32 string to UTF-8 string.
   * 
   * \param[in]  utf32StringSrc      is source UTF-32 string to convert
   * \param[out] utf8StringDst       is output UTF-8 string, empty on any error
   *
   * \return true on successful conversion, false on any error
   * \sa Utf32ToUtf8(std::u32string&)
   */
  static bool Utf32ToUtf8(const std::u32string& utf32StringSrc, std::string& utf8StringDst);
  
  /**
   * Convert UTF-32 string to UTF-8 string.
   * 
   * \param[in] utf32StringSrc      is source UTF-32 string to convert
   *
   * \return converted string on successful conversion, empty string on any error
   * \sa Utf32ToUtf8(std::u32string&, std::string&)
   */
  static std::string Utf32ToUtf8(const std::u32string& utf32StringSrc);
  
  /**
   * Convert UTF-8 string to wide string and perform logical to visual processing
   * on the string.
   * Use it for readable text, GUI strings etc.
   *
   * \param[in]  utf8StringSrc        is source UTF-8 string to convert
   * \param[out] wStringDst           is output wide string, empty on any error
   * \param[in]  bidiOptions          options for logical to visual transformation
   *                                  default is LTR | REMOVE_CONTROLS which should be fine
   *                                  in most cases
   *
   * \return true on successful conversion, false on any error
   * \sa CCharsetConverter::BiDiOptions
   * \sa Utf8ToW
   * \sa Utf8ToUtf32LogicalToVisual
   */
  static bool Utf8ToWLogicalToVisual(const std::string& utf8StringSrc, std::wstring& wStringDst,
                                     uint16_t bidiOptions = LTR | REMOVE_CONTROLS);

  /**
   * Convert UTF-8 string to wide string.
   *
   * \param[in]  utf8StringSrc       is source UTF-8 string to convert
   * \param[out] wStringDst          is output wide string, empty on any error
   *
   * \return true on successful conversion, false on any error
   * \sa Utf8ToWLogicalToVisual
   * \sa Utf8ToWSystemSafe
   */
  static bool Utf8ToW(const std::string& utf8StringSrc, std::wstring& wStringDst);

  /**
  * Convert UTF-8 string to wide string.
  *
  * \param[in]  utf8StringSrc       is source UTF-8 string to convert
  * \param[out] wStringDst          is output wide string, empty on any error
  *
  * \return true on successful conversion, false on any error
  * \sa Utf8ToWLogicalToVisual
  * \sa Utf8ToWSystemSafe
  * \sa Utf8ToW
   */
  static bool TryUtf8ToW(const std::string& utf8StringSrc, std::wstring& wStringDst);

  /**
   * Convert UTF-8 string to wide string and perform extra processing
   * to ensure that the string is valid for file system operations.
   * Fails on invalid byte sequences.
   *
   * \param[in]  utf8StringSrc       is source UTF-8 string to convert
   * \param[out] wStringDst          is output wide string, empty on any error
   *
   * \return true on successful conversion, false on any error
   * \sa Utf8ToWLogicalToVisual
   * \sa Utf8ToW
   */
  static bool Utf8ToWSystemSafe(const std::string& stringSrc, std::wstring& stringDst);

  /**
   * Convert wide string to UTF-8 string.
   *
   * \param[in]  wStringSrc             is source UTF-8 string to convert
   * \param[out] utf8StringDst          is output wide string, empty on any error
   *
   * \return true on successful conversion, false on any error
   * \sa TryWToUtf8
   */
  static bool WToUtf8(const std::wstring& wStringSrc, std::string& utf8StringDst);

  /**
  * Convert wide string to UTF-8 string.
  *
  * \param[in]  wStringSrc             is source UTF-8 string to convert
  * \param[out] utf8StringDst          is output wide string, empty on any error
  *
  * \return true on successful conversion, false on any error
  * \sa WToUtf8
  */
  static bool TryWToUtf8(const std::wstring& wStringSrc, std::string& utf8StringDst);

  /**
   * Convert from the user selected subtitle encoding to UTF-8
   *
   * \param[in]  stringSrc             is source UTF-8 string to convert
   * \param[out] utf8StringDst         is output wide string, empty on any error
   *
   * \return true on successful conversion, false on any error
   */
  static bool SubtitleCharsetToUtf8(const std::string& stringSrc, std::string& utf8StringDst);

  /**
   * Convert UTF-8 string to the user selected GUI character set
   *
   * \param[in]  utf8StringSrc          is source UTF-8 string to convert
   * \param[out] stringDst              is output wide string, empty on any error
   *
   * \return true on successful conversion, false on any error
   */
  static bool Utf8ToStringCharset(const std::string& utf8StringSrc, std::string& stringDst);

  /**
   * Convert UTF-8 string to the user selected GUI character set
   *
   * \param[in,out]  stringSrcDst       is source UTF-8 string to convert
   *                                    Undefined value of stringSrcDst if conversion fails
   *
   * \return true on successful conversion, false on any error
   */
  static bool Utf8ToStringCharset(std::string& stringSrcDst);

  /**
   * Convert UTF-8 string to UTF-16 big endian string.
   *
   * \param[in]  utf8StringSrc          is source UTF-8 string to convert
   * \param[out] utf16StringDst         is output UTF-16 big endian, empty on any error
   *
   * \return true on successful conversion, false on any error
   * \sa Utf8ToUtf16LE
   * \sa Utf8ToUtf16
   */
  static bool Utf8ToUtf16BE(const std::string& utf8StringSrc, std::u16string& utf16StringDst);

  /**
   * Convert UTF-8 string to UTF-16 little endian string.
   * Only for special cases where endianness matters, prefer
   * Utf8ToUtf16 for general use.
   *
   * \param[in]  utf8StringSrc          is source UTF-8 string to convert
   * \param[out] utf16StringDst         is output UTF-16 little endian, empty on any error
   *
   * \return true on successful conversion, false on any error
   * \sa Utf8ToUtf16BE
   * \sa Utf8ToUtf16
   */
  static bool Utf8ToUtf16LE(const std::string& utf8StringSrc, std::u16string& utf16StringDst);

  /**
   * Convert UTF-8 string to system encoding, likely UTF-8 on Linux
   * and Mac but can be just about anything
   *
   * \param[in,out]  utf8StringSrc          is source UTF-8 string to convert
   *                                        Undefined value on errors
   *
   * \return true on successful conversion, false on any error
   * \sa SystemToUtf8
   * \sa Utf8To
   */
  static bool Utf8ToSystem(std::string& stringSrcDst);

  /**
   * Convert string in system encoding to UTF-8
   *
   * \param[in]  sysStringSrc          is source string in system encoding to convert
   * \param[out] utf8StringDst         is output UTF-8 string, empty on any error
   *
   * \return true on successful conversion, false on any error
   * \sa Utf8ToSystem
   * \sa Utf8To
   * \sa TrySystemToUtf8
   */
  static bool SystemToUtf8(const std::string& sysStringSrc, std::string& utf8StringDst);

  /**
   * Try to convert string in system encoding to UTF-8.
   * Will fail on invalid byte sequences.
   *
   * \param[in]  sysStringSrc         is source string in system encoding to convert
   * \param[out] utf8StringDst        is output UTF-8 string, empty on any error
   *
   * \return true on successful conversion, false on any error
   * \sa SystemToUtf8
   * \sa Utf8To
   */
  static bool TrySystemToUtf8(const std::string& sysStringSrc, std::string& utf8StringDst);

  /**
   * Convert UTF-8 string to specified 8-bit encoding
   *
   * \param[in]  strDestCharset         specify destination encoding
   *                                    e.g. US-ASCII or CP-1252
   * \param[in]  utf8StringSrc          is source UTF-8 string to convert
   * \param[out] stringDst              is output string in specified encoding
   *
   * \return true on successful conversion, false on any error
   * \sa Utf8To(const std::string&, const std::string&, std::u16string&)
   * \sa Utf8To(const std::string&, const std::string&, std::u32string&)
   */
  static bool Utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::string& stringDst);

  /**
   * Convert UTF-8 string to specified 16-bit encoding
   *
   * \param[in]  strDestCharset         specify destination encoding
   *                                    e.g. UTF-16 or UTF-16LE
   * \param[in]  utf8StringSrc          is source UTF-8 string to convert
   * \param[out] utf16StringDst         is output string in specified encoding
   *
   * \return true on successful conversion, false on any error
   * \sa Utf8To(const std::string&, const std::string&, std::string&)
   * \sa Utf8To(const std::string&, const std::string&, std::u32string&)
   */
  static bool Utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u16string& utf16StringDst);

  /**
   * Convert UTF-8 string to specified 32-bit encoding
   *
   * \param[in]  strDestCharset         specify destination encoding
   *                                    e.g. UTF-32 or UTF-32LE
   * \param[in]  utf8StringSrc          is source UTF-8 string to convert
   * \param[out] utf32StringDst         is output string in specified encoding
   *
   * \return true on successful conversion, false on any error
   * \sa Utf8To(const std::string&, const std::string&, std::string&)
   * \sa Utf8To(const std::string&, const std::string&, std::u16string&)
   */
  static bool Utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u32string& utf32StringDst);

  /**
   * Convert specified 8-bit encoding to UTF-8
   *
   * \param[in]  strSourceCharset         specify source encoding
   *                                      e.g. US-ASCII or CP-1252
   * \param[in]  stringSrc                is source 8-bit string to convert
   * \param[out] utf8StringDst            is output string in UTF-8
   *
   * \return true on successful conversion, false on any error
   * \sa TryToUtf8
   */
  static bool ToUtf8(const std::string& strSourceCharset, const std::string& stringSrc, std::string& utf8StringDst);

  /**
   * Try to convert specified 8-bit encoding to UTF-8.
   * Fails on any bad byte sequence
   *
   * \param[in]  strSourceCharset         specify source encoding
   *                                      e.g. US-ASCII or CP-1252
   * \param[in]  stringSrc                is source 8-bit string to convert
   * \param[out] utf8StringDst            is output string in UTF-8
   *
   * \return true on successful conversion, false on any error
   * \sa ToUtf8
   */
  static bool TryToUtf8(const std::string& strSourceCharset, const std::string& stringSrc, std::string& utf8StringDst);

  /**
   * Convert UTF-16 big endian to UTF-8
   *
   * \param[in]  utf16StringSrc           is source UTF-16 big endian string to convert
   * \param[out] utf8StringDst            is output string in UTF-8
   *
   * \return true on successful conversion, false on any error
   * \sa Utf16ToUtf8
   * \sa Utf16LEToUtf8
   */
  static bool Utf16BEToUtf8(const std::u16string& utf16StringSrc, std::string& utf8StringDst);

  /**
  * Convert UTF-16 little endian to UTF-8
  *
  * \param[in]  utf16StringSrc           is source UTF-16 little endian string to convert
  * \param[out] utf8StringDst            is output string in UTF-8
  *
  * \return true on successful conversion, false on any error
  * \sa Utf16ToUtf8
  * \sa Utf16BEToUtf8
  */
  static bool Utf16LEToUtf8(const std::u16string& utf16StringSrc, std::string& utf8StringDst);

  /**
  * Convert UTF-16 to UTF-8
  *
  * \param[in]  utf16StringSrc           is source UTF-16 string to convert
  * \param[out] utf8StringDst            is output string in UTF-8
  *
  * \return true on successful conversion, false on any error
  * \sa Utf16ToUtf8
  * \sa Utf16LEToUtf8
  */
  static bool Utf16ToUtf8(const std::u16string& utf16StringSrc, std::string& utf8StringDst);

/**
   * Try to convert string in system encoding to UTF-8.
   * Will fail on invalid byte sequences.
   *
   * \param[in]  utf16StringSrc           is source UTF-16 string to convert
   * \param[out] utf8StringDst            is output string in UTF-8
   *
   * \return true on successful conversion, false on any error
   * \sa Utf16ToUtf8
   * \sa Utf16LEToUtf8
   * \sa Utf16BEToUtf8
   */
  static bool TryUtf16ToUtf8(const std::u16string utf16StringSrc, std::string& utf8StringDst);

  /**
   * Convert UCS-2 to UTF-8
   * This is really another name for Utf16LEToUtf8, technically
   * UCS-2 is only big endian but our use case requires little endian
   * conversion
   *
   * \param[in]  utf16StringSrc           is source UCS-2 little endian string to convert
   * \param[out] utf8StringDst            is output string in UTF-8
   *
   * \return true on successful conversion, false on any error
   * \sa Utf16ToUtf8
   * \sa Utf16LEToUtf8
   * \sa Utf16BEToUtf8
   */
  static bool Ucs2ToUtf8(const std::u16string& ucs2StringSrc, std::string& utf8StringDst);

  /**
   * Perform logical to visual processing on the string.
   * Use it for readable text, GUI strings etc.
   *
   * \param[in]  utf8StringSrc        is source UTF-8 string to process
   * \param[out] utf8StringDst        is output UTF-8 string, empty on any error
   * \param[in]  bidiOptions          options for logical to visual transformation
   *                                  default is LTR | REMOVE_CONTROLS which should be fine
   *                                  in most cases
   *
   * \return true on successful conversion, false on any error
   * \sa CCharsetConverter::BiDiOptions
   * \sa LogicalToVisualBiDi(const std::u16string&, std::u16string&, uint16_t)
   * \sa LogicalToVisualBiDi(const std::wstring&, std::wstring&, uint16_t)
   * \sa LogicalToVisualBiDi(const std::u32string, std::u32string&, uint16_t)
   */
  static bool LogicalToVisualBiDi(const std::string& utf8StringSrc, std::string& utf8StringDst, 
                                  uint16_t bidiOptions = LTR | REMOVE_CONTROLS);

  /**
   * Perform logical to visual processing on the string.
   * Use it for readable text, GUI strings etc.
   *
   * \param[in]  utf16StringSrc       is source UTF-16 string to process
   * \param[out] utf16StringDst       is output UTF-16 string, empty on any error
   * \param[in]  bidiOptions          options for logical to visual transformation
   *                                  default is LTR | REMOVE_CONTROLS which should be fine
   *                                  in most cases
   *
   * \return true on successful conversion, false on any error
   * \sa CCharsetConverter::BiDiOptions
   * \sa LogicalToVisualBiDi(const std::string&, std::string&, uint16_t)
   * \sa LogicalToVisualBiDi(const std::wstring&, std::wstring&, uint16_t)
   * \sa LogicalToVisualBiDi(const std::u32string, std::u32string&, uint16_t)
   */
  static bool LogicalToVisualBiDi(const std::u16string& utf16StringSrc, std::u16string& utf16StringDst,
                                  uint16_t bidiOptions = LTR | REMOVE_CONTROLS);

  /**
   * Perform logical to visual processing on the string.
   * Use it for readable text, GUI strings etc.
   *
   * \param[in]  wStringSrc           is source wide string to process
   * \param[out] wStringDst           is output wide string, empty on any error
   * \param[in]  bidiOptions          options for logical to visual transformation
   *                                  default is LTR | REMOVE_CONTROLS which should be fine
   *                                  in most cases
   *
   * \return true on successful conversion, false on any error
   * \sa CCharsetConverter::BiDiOptions
   * \sa LogicalToVisualBiDi(const std::string&, std::string&, uint16_t)
   * \sa LogicalToVisualBiDi(const std::u16string&, std::u16string&, uint16_t)
   * \sa LogicalToVisualBiDi(const std::u32string, std::u32string&, uint16_t)
   */
  static bool LogicalToVisualBiDi(const std::wstring& wStringSrc, std::wstring& wStringDst,
                                  uint16_t bidiOptions = LTR | REMOVE_CONTROLS);

  /**
   * Perform logical to visual processing on the string.
   * Use it for readable text, GUI strings etc.
   *
   * \param[in]  utf32StringSrc       is source UTF-32 string to process
   * \param[out] utf32StringDst       is output UTF-32 string, empty on any error
   * \param[in]  bidiOptions          options for logical to visual transformation
   *                                  default is LTR | REMOVE_CONTROLS which should be fine
   *                                  in most cases
   *
   * \return true on successful conversion, false on any error
   * \sa CCharsetConverter::BiDiOptions
   * \sa LogicalToVisualBiDi(const std::string&, std::string&, uint16_t)
   * \sa LogicalToVisualBiDi(const std::u16string&, std::u16string&, uint16_t)
   * \sa LogicalToVisualBiDi(const std::wstring&, std::wstring&, uint16_t)
   */
  static bool LogicalToVisualBiDi(const std::u32string& utf32StringSrc, std::u32string& utf32StringDst,
                                  uint16_t bidiOptions = LTR | REMOVE_CONTROLS);

  /**
   * Reverse an RTL string, taking into account encoding
   *
   * \param[in]  utf8StringSrc     RTL string to be reversed in utf8 encoding
   * \param[out] utf8StringDst     Destination for the reversed string
   *
   * \return true in success, false otherwise
   */
  static bool ReverseRTL(const std::string& utf8StringSrc, std::string& utf8StringDst);

  /**
   * Convert from unkown encoding to UTF-8
   *
   * \param[in,out]  stringSrcDst        is source string to convert and destination string
   *                                     Undefined value on error
   *
   * \return true on successful conversion, false on any error
   * \sa unkownToUTF8(const std::string&, std::string&)
   */
  static bool UnknownToUtf8(std::string& stringSrcDst);

  /**
   * Convert from unkown encoding to UTF-8
   *
   * \param[in]  stringSrcDst       is source string to convert and destination string
   * \param[out] utf8StringDst      is output UTF-8 string
   *
   * \return true on successful conversion, false on any error
   * \sa unkownToUTF8(std::string&)
   */
  static bool UnknownToUtf8(const std::string& stringSrc, std::string& utf8StringDst);

  /**
   * Convert UTF-8 string to system encoding and perform extra processing
   * to ensure that the string is valid for file system operations.
   * Fails on invalid byte sequences.
   *
   * \param[in]  stringSrc        is source UTF-8 string to convert
   * \param[out] stringDst        is output string in system encoding
   *
   * \return true on successful conversion, false on any error
   */
  static bool Utf8ToSystemSafe(const std::string& stringSrc, std::string& stringDst);

  /**
   * Convert wide string to UTF-8 string and perform extra processing
   * to ensure that the string is valid for file system operations.
   * Fails on invalid byte sequences.
   *
   * \param[in]  wStringSrc          is source wide string to convert
   * \param[out] utf8StringDst       is output UTF-8 string
   *
   * \return true on successful conversion, false on any error
   */
  static bool WToUtf8SystemSafe(const std::wstring& wStringSrc, std::string& utf8StringDst);

  /**
   * Normalize a string to composed or decomposed form
   *
   * It's also possible to use the special mac decomposed form for any special
   * needs on darwin platforms
   *
   * \param[in]   source        String to normalize
   * \param[out]  destination   String to store the normalized version
   * \param[in]   options       Specify which type of normalization to use
   * \return true on success, false on any failures
   *
   * \sa NormalizationOptions
   * \sa Normalize(const std::u16string&, std::u16string&, uint16_t)
   * \sa Normalize(const std::u32string&, std::u32string&, uint16_t)
   * \sa Normalize(const std::wstring&, std::wstring&, uint16_t)
   */
  static bool Normalize(const std::string& source, std::string& destination, uint16_t options);

  /**
  * Normalize a string to composed or decomposed form
  *
  * It's also possible to use the special mac decomposed form for any special
  * needs on darwin platforms
  *
  * \param[in]   source        String to normalize
  * \param[out]  destination   String to store the normalized version
  * \param[in]   options       Specify which type of normalization to use
  * \return true on success, false on any failures
  *
  * \sa NormalizationOptions
  * \sa Normalize(const std::string&, std::string&, uint16_t)
  * \sa Normalize(const std::u32string&, std::u32string&, uint16_t)
  * \sa Normalize(const std::wstring&, std::wstring&, uint16_t)
  */
  static bool Normalize(const std::u16string& source, std::u16string& destination, uint16_t options);
  
  /**
  * Normalize a string to composed or decomposed form
  *
  * It's also possible to use the special mac decomposed form for any special
  * needs on darwin platforms
  *
  * \param[in]   source        String to normalize
  * \param[out]  destination   String to store the normalized version
  * \param[in]   options       Specify which type of normalization to use
  * \return true on success, false on any failures
  *
  * \sa NormalizationOptions
  * \sa Normalize(const std::string&, std::string&, uint16_t)
  * \sa Normalize(const std::u16string&, std::u16string&, uint16_t)
  * \sa Normalize(const std::wstring&, std::wstring&, uint16_t)
  */
  static bool Normalize(const std::u32string& source, std::u32string& destination, uint16_t options);
  
  /**
  * Normalize a string to composed or decomposed form
  *
  * It's also possible to use the special mac decomposed form for any special
  * needs on darwin platforms
  *
  * \param[in]   source        String to normalize
  * \param[out]  destination   String to store the normalized version
  * \param[in]   options       Specify which type of normalization to use
  * \return true on success, false on any failures
  *
  * \sa NormalizationOptions
  * \sa Normalize(const std::string&, std::string&, uint16_t)
  * \sa Normalize(const std::u16string&, std::u16string&, uint16_t)
  * \sa Normalize(const std::u32string&, std::u32string&, uint16_t)
  */
  static bool Normalize(const std::wstring& source, std::wstring& destination, uint16_t options);
private:

  class CInnerConverter;
};

XBMC_GLOBAL(CCharsetConverter,g_charsetConverter);
