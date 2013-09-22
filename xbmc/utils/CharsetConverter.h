#ifndef CCHARSET_CONVERTER
#define CCHARSET_CONVERTER

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

#include "settings/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "utils/GlobalsHandling.h"
#include "utils/uXstrings.h"

#include <string>
#include <vector>

class CSetting;

class CCharsetConverter : public ISettingCallback
{
public:
  CCharsetConverter();

  virtual void OnSettingChanged(const CSetting* setting);

  void reset();

  void clear();

  bool utf8ToW(const std::string& utf8StringSrc, std::wstring& wStringDst,
                bool bVisualBiDiFlip = true, bool forceLTRReadingOrder = false,
                bool failOnBadChar = false, bool* bWasFlipped = NULL);

  bool utf16LEtoW(const std::u16string& utf16String, std::wstring& wString);

  bool subtitleCharsetToW(const std::string& stringSrc, std::wstring& wStringDst);

  bool utf8ToStringCharset(const std::string& utf8StringSrc, std::string& stringDst);

  bool utf8ToStringCharset(std::string& stringSrcDst);
  bool utf8ToSystem(std::string& stringSrcDst, bool failOnBadChar = false);

  bool utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::string& stringDst);
  bool utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u16string& utf16StringDst);
  bool utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u32string& utf32StringDst);

  bool ToUtf8(const std::string& strSourceCharset, const std::string& stringSrc, std::string& utf8StringDst);

  bool isValidUtf8(const std::string& str);

  bool isValidUtf8(const char* buf, unsigned int len);

  bool wToUTF8(const std::wstring& wStringSrc, std::string& utf8StringDst, bool failOnBadChar = false);
  bool utf16BEtoUTF8(const std::u16string& utf16StringSrc, std::string& utf8StringDst);
  bool utf16LEtoUTF8(const std::u16string& utf16StringSrc, std::string& utf8StringDst);
  bool ucs2ToUTF8(const std::u16string& ucs2StringSrc, std::string& utf8StringDst);

  bool utf8logicalToVisualBiDi(const std::string& utf8StringSrc, std::string& utf8StringDst);

  bool utf32ToStringCharset(const std::u32string& utf32StringSrc, std::string& stringDst);

  std::vector<std::string> getCharsetLabels();
  std::string getCharsetLabelByName(const std::string& charsetName);
  std::string getCharsetNameByLabel(const std::string& charsetLabel);
  bool isBidiCharset(const std::string& charset);

  bool unknownToUTF8(std::string& stringSrcDst);
  bool unknownToUTF8(const std::string& stringSrc, std::string& utf8StringDst, bool failOnBadChar = false);

  bool toW(const std::string& stringSrc, std::wstring& wStringDst, const std::string& enc);
  bool fromW(const std::wstring& wStringSrc, std::string& stringDst, const std::string& enc);

  static void SettingOptionsCharsetsFiller(const CSetting* setting, std::vector< std::pair<std::string, std::string> >& list, std::string& current);
private:
  static const int m_Utf8CharMinSize, m_Utf8CharMaxSize;
};

XBMC_GLOBAL(CCharsetConverter,g_charsetConverter);

size_t iconv_const (void* cd, const char** inbuf, size_t* inbytesleft, char** outbuf, size_t* outbytesleft);

#endif
