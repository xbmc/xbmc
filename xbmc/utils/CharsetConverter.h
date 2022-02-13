/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingCallback.h"
#include "utils/GlobalsHandling.h"

#include <string>
#include <utility>
#include <vector>

class CSetting;
struct StringSettingOption;

class CCharsetConverter : public ISettingCallback
{
public:
  CCharsetConverter();

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  static void reset();
  static void resetSystemCharset();
  static void reinitCharsetsFromSettings(void);

  static void clear();

  /**
   * Convert UTF-8 string to UTF-32 string.
   * No RTL logical-visual transformation is performed.
   * @param utf8StringSrc       is source UTF-8 string to convert
   * @param utf32StringDst      is output UTF-32 string, empty on any error
   * @param failOnBadChar       if set to true function will fail on invalid character,
   *                            otherwise invalid character will be skipped
   * @return true on successful conversion, false on any error
   */
  static bool utf8ToUtf32(const std::string& utf8StringSrc, std::u32string& utf32StringDst, bool failOnBadChar = true);
  /**
   * Convert UTF-8 string to UTF-32 string.
   * No RTL logical-visual transformation is performed.
   * @param utf8StringSrc       is source UTF-8 string to convert
   * @param failOnBadChar       if set to true function will fail on invalid character,
   *                            otherwise invalid character will be skipped
   * @return converted string on successful conversion, empty string on any error
   */
  static std::u32string utf8ToUtf32(const std::string& utf8StringSrc, bool failOnBadChar = true);
  /**
   * Convert UTF-8 string to UTF-32 string.
   * RTL logical-visual transformation is optionally performed.
   * Use it for readable text, GUI strings etc.
   * @param utf8StringSrc       is source UTF-8 string to convert
   * @param utf32StringDst      is output UTF-32 string, empty on any error
   * @param bVisualBiDiFlip     allow RTL visual-logical transformation if set to true, must be set
   *                            to false is logical-visual transformation is already done
   * @param forceLTRReadingOrder        force LTR reading order
   * @param failOnBadChar       if set to true function will fail on invalid character,
   *                            otherwise invalid character will be skipped
   * @return true on successful conversion, false on any error
   */
  static bool utf8ToUtf32Visual(const std::string& utf8StringSrc, std::u32string& utf32StringDst, bool bVisualBiDiFlip = false, bool forceLTRReadingOrder = false, bool failOnBadChar = false);
  /**
   * Convert UTF-32 string to UTF-8 string.
   * No RTL visual-logical transformation is performed.
   * @param utf32StringSrc      is source UTF-32 string to convert
   * @param utf8StringDst       is output UTF-8 string, empty on any error
   * @param failOnBadChar       if set to true function will fail on invalid character,
   *                            otherwise invalid character will be skipped
   * @return true on successful conversion, false on any error
   */
  static bool utf32ToUtf8(const std::u32string& utf32StringSrc, std::string& utf8StringDst, bool failOnBadChar = false);
  /**
   * Convert UTF-32 string to UTF-8 string.
   * No RTL visual-logical transformation is performed.
   * @param utf32StringSrc      is source UTF-32 string to convert
   * @param failOnBadChar       if set to true function will fail on invalid character,
   *                            otherwise invalid character will be skipped
   * @return converted string on successful conversion, empty string on any error
   */
  static std::string utf32ToUtf8(const std::u32string& utf32StringSrc, bool failOnBadChar = false);
  /**
   * Convert UTF-32 string to wchar_t string (wstring).
   * No RTL visual-logical transformation is performed.
   * @param utf32StringSrc      is source UTF-32 string to convert
   * @param wStringDst          is output wchar_t string, empty on any error
   * @param failOnBadChar       if set to true function will fail on invalid character,
   *                            otherwise invalid character will be skipped
   * @return true on successful conversion, false on any error
   */
  static bool utf32ToW(const std::u32string& utf32StringSrc, std::wstring& wStringDst, bool failOnBadChar = false);
  /**
   * Perform logical to visual flip.
   * @param logicalStringSrc    is source string with logical characters order
   * @param visualStringDst     is output string with visual characters order, empty on any error
   * @param forceLTRReadingOrder        force LTR reading order
   * @param visualToLogicalMap    is output mapping of positions in the visual string to the logical string
   * @return true on success, false otherwise
   */
  static bool utf32logicalToVisualBiDi(const std::u32string& logicalStringSrc,
                                       std::u32string& visualStringDst,
                                       bool forceLTRReadingOrder = false,
                                       bool failOnBadString = false,
                                       int* visualToLogicalMap = nullptr);
  /**
   * Strictly convert wchar_t string (wstring) to UTF-32 string.
   * No RTL visual-logical transformation is performed.
   * @param wStringSrc          is source wchar_t string to convert
   * @param utf32StringDst      is output UTF-32 string, empty on any error
   * @param failOnBadChar       if set to true function will fail on invalid character,
   *                            otherwise invalid character will be skipped
   * @return true on successful conversion, false on any error
   */
  static bool wToUtf32(const std::wstring& wStringSrc, std::u32string& utf32StringDst, bool failOnBadChar = false);

  static bool utf8ToW(const std::string& utf8StringSrc, std::wstring& wStringDst,
                bool bVisualBiDiFlip = true, bool forceLTRReadingOrder = false,
                bool failOnBadChar = false);

  static bool utf16LEtoW(const std::u16string& utf16String, std::wstring& wString);

  static bool subtitleCharsetToUtf8(const std::string& stringSrc, std::string& utf8StringDst);

  static bool utf8ToStringCharset(const std::string& utf8StringSrc, std::string& stringDst);

  static bool utf8ToStringCharset(std::string& stringSrcDst);
  static bool utf8ToSystem(std::string& stringSrcDst, bool failOnBadChar = false);
  static bool systemToUtf8(const std::string& sysStringSrc, std::string& utf8StringDst, bool failOnBadChar = false);

  static bool utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::string& stringDst);
  static bool utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u16string& utf16StringDst);
  static bool utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u32string& utf32StringDst);

  static bool ToUtf8(const std::string& strSourceCharset, const std::string& stringSrc, std::string& utf8StringDst, bool failOnBadChar = false);

  static bool wToUTF8(const std::wstring& wStringSrc, std::string& utf8StringDst, bool failOnBadChar = false);

  /*!
   *  \brief Convert UTF-16BE (u16string) string to UTF-8 string.
   *  No RTL visual-logical transformation is performed.
   *  \param utf16StringSrc Is source UTF-16BE u16string string to convert
   *  \param utf8StringDst Is output UTF-8 string, empty on any error
   *  \return True on successful conversion, false on any error
   */
  static bool utf16BEtoUTF8(const std::u16string& utf16StringSrc, std::string& utf8StringDst);

  /*!
   *  \brief Convert UTF-16BE (string) string to UTF-8 string.
   *  No RTL visual-logical transformation is performed.
   *  \param utf16StringSrc Is source UTF-16BE string to convert
   *  \param utf8StringDst Is output UTF-8 string, empty on any error
   *  \return True on successful conversion, false on any error
   */
  static bool utf16BEtoUTF8(const std::string& utf16StringSrc, std::string& utf8StringDst);

  static bool utf16LEtoUTF8(const std::u16string& utf16StringSrc, std::string& utf8StringDst);
  static bool ucs2ToUTF8(const std::u16string& ucs2StringSrc, std::string& utf8StringDst);

  /*!
   *  \brief Convert Macintosh (string) string to UTF-8 string.
   *  No RTL visual-logical transformation is performed.
   *  \param macStringSrc Is source Macintosh string to convert
   *  \param utf8StringDst Is output UTF-8 string, empty on any error
   *  \return True on successful conversion, false on any error
   */
  static bool MacintoshToUTF8(const std::string& macStringSrc, std::string& utf8StringDst);

  static bool utf8logicalToVisualBiDi(const std::string& utf8StringSrc, std::string& utf8StringDst, bool failOnBadString = false);

  /**
   * Check if a string has RTL direction.
   * @param utf8StringSrc the string
   * @return true if the string is RTL, otherwise false
   */
  static bool utf8IsRTLBidiDirection(const std::string& utf8String);

  static bool utf32ToStringCharset(const std::u32string& utf32StringSrc, std::string& stringDst);

  static std::vector<std::string> getCharsetLabels();
  static std::string getCharsetLabelByName(const std::string& charsetName);
  static std::string getCharsetNameByLabel(const std::string& charsetLabel);

  static bool unknownToUTF8(std::string& stringSrcDst);
  static bool unknownToUTF8(const std::string& stringSrc, std::string& utf8StringDst, bool failOnBadChar = false);

  static bool toW(const std::string& stringSrc, std::wstring& wStringDst, const std::string& enc);
  static bool fromW(const std::wstring& wStringSrc, std::string& stringDst, const std::string& enc);

  static void SettingOptionsCharsetsFiller(const std::shared_ptr<const CSetting>& setting,
                                           std::vector<StringSettingOption>& list,
                                           std::string& current,
                                           void* data);

private:
  static void resetUserCharset(void);
  static void resetSubtitleCharset(void);

  static const int m_Utf8CharMinSize, m_Utf8CharMaxSize;
  class CInnerConverter;
};

XBMC_GLOBAL_REF(CCharsetConverter,g_charsetConverter);
#define g_charsetConverter XBMC_GLOBAL_USE(CCharsetConverter)
