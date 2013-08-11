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

  void utf8ToW(const std::string& utf8StringSrc, std::wstring& wStringDst, bool bVisualBiDiFlip = true, bool forceLTRReadingOrder = false, bool* bWasFlipped = NULL);

  void utf16LEtoW(const std::u16string& utf16String, std::wstring& wString);

  void subtitleCharsetToW(const std::string& stringSrc, std::wstring& wStringDst);

  void utf8ToStringCharset(const std::string& utf8StringSrc, std::string& stringDst);

  void utf8ToStringCharset(std::string& stringSrcDst);
  void utf8ToSystem(std::string& stringSrcDst);

  void utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::string& stringDst);
  void utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u16string& utf16StringDst);
  void utf8To(const std::string& strDestCharset, const std::string& utf8StringSrc, std::u32string& utf32StringDst);

  void stringCharsetToUtf8(const std::string& strSourceCharset, const std::string& stringSrc, std::string& utf8StringDst);

  bool isValidUtf8(const std::string& str);

  bool isValidUtf8(const char* buf, unsigned int len);

  void ucs2CharsetToStringCharset(const std::u16string& ucs2StringSrc, std::string& stringDst, bool swap = false);

  void wToUTF8(const std::wstring& wStringSrc, std::string& utf8StringDst);
  void utf16BEtoUTF8(const std::u16string& utf16StringSrc, std::string& utf8StringDst);
  void utf16LEtoUTF8(const std::u16string& utf16StringSrc, std::string& utf8StringDst);
  void ucs2ToUTF8(const std::u16string& ucs2StringSrc, std::string& utf8StringDst);

  void utf8logicalToVisualBiDi(const std::string& utf8StringSrc, std::string& utf8StringDst);

  void utf32ToStringCharset(const unsigned long* utf32StringSrc, std::string& stringDst);

  std::vector<std::string> getCharsetLabels();
  std::string getCharsetLabelByName(const std::string& charsetName);
  std::string getCharsetNameByLabel(const std::string& charsetLabel);
  bool isBidiCharset(const std::string& charset);

  void unknownToUTF8(std::string& stringSrcDst);
  void unknownToUTF8(const std::string& stringSrc, std::string& utf8StringDst);

  void toW(const std::string& stringSrc, std::wstring& wStringDst, const std::string& enc);
  void fromW(const std::wstring& wStringSrc, std::string& stringDst, const std::string& enc);

  static void SettingOptionsCharsetsFiller(const CSetting* setting, std::vector< std::pair<std::string, std::string> >& list, std::string& current);
};

XBMC_GLOBAL(CCharsetConverter,g_charsetConverter);

size_t iconv_const (void* cd, const char** inbuf, size_t* inbytesleft, char** outbuf, size_t* outbytesleft);

#endif
