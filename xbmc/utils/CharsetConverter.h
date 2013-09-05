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

  void utf8ToW(const std::string& utf8String, std::wstring& wString, bool bVisualBiDiFlip = true, bool forceLTRReadingOrder = false, bool* bWasFlipped = NULL);

  void utf16LEtoW(const std::u16string& utf16String, std::wstring& wString);

  void subtitleCharsetToW(const std::string& strSource, std::wstring& strDest);

  void utf8ToStringCharset(const std::string& strSource, std::string& strDest);

  void utf8ToStringCharset(std::string& strSourceDest);
  void utf8ToSystem(std::string& strSourceDest);

  void utf8To(const std::string& strDestCharset, const std::string& strSource, std::string& strDest);
  void utf8To(const std::string& strDestCharset, const std::string& strSource, std::u16string& strDest);
  void utf8To(const std::string& strDestCharset, const std::string& strSource, std::u32string& strDest);

  void stringCharsetToUtf8(const std::string& strSourceCharset, const std::string& strSource, std::string& strDest);

  bool isValidUtf8(const std::string& str);

  bool isValidUtf8(const char* buf, unsigned int len);

  void ucs2CharsetToStringCharset(const std::wstring& strSource, std::string& strDest, bool swap = false);

  void wToUTF8(const std::wstring& strSource, std::string& strDest);
  void utf16BEtoUTF8(const std::u16string& strSource, std::string& strDest);
  void utf16LEtoUTF8(const std::u16string& strSource, std::string& strDest);
  void ucs2ToUTF8(const std::u16string& strSource, std::string& strDest);

  void utf8logicalToVisualBiDi(const std::string& strSource, std::string& strDest);

  void utf32ToStringCharset(const unsigned long* strSource, std::string& strDest);

  std::vector<std::string> getCharsetLabels();
  std::string getCharsetLabelByName(const std::string& charsetName);
  std::string getCharsetNameByLabel(const std::string& charsetLabel);
  bool isBidiCharset(const std::string& charset);

  void unknownToUTF8(std::string& sourceDest);
  void unknownToUTF8(const std::string& source, std::string& dest);

  void toW(const std::string& source, std::wstring& dest, const std::string& enc);
  void fromW(const std::wstring& source, std::string& dest, const std::string& enc);

  static void SettingOptionsCharsetsFiller(const CSetting* setting, std::vector< std::pair<std::string, std::string> >& list, std::string& current);
};

XBMC_GLOBAL(CCharsetConverter,g_charsetConverter);

size_t iconv_const (void* cd, const char** inbuf, size_t* inbytesleft, char** outbuf, size_t* outbytesleft);

#endif
