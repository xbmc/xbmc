#ifndef CCHARSET_CONVERTER
#define CCHARSET_CONVERTER

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef _LINUX
#include "lib/libiconv/iconv.h"
#include "lib/libfribidi/fribidi.h"
#else
#include <iconv.h>
#include <fribidi/fribidi.h>
#include <fribidi/fribidi_char_sets.h>
#endif

#include <vector>

size_t iconv_const (iconv_t cd, const char** inbuf, size_t *inbytesleft, 
		    char* * outbuf, size_t *outbytesleft);

#ifdef __APPLE__
  #define WCHAR_CHARSET "UTF-32LE"
  #define UTF8_SOURCE "UTF-8-MAC" 
#elif defined(_XBOX) || defined(WIN32)
  #define WCHAR_CHARSET "UTF-16LE"
  #define UTF8_SOURCE "UTF-8"
#else
  #define WCHAR_CHARSET "WCHAR_T"
  #define UTF8_SOURCE "UTF-8"
#endif

class CCharsetConverter
{
public:
  CCharsetConverter();

  void reset();

  void clear();
  template<class INPUT,class OUTPUT>
  void convert(iconv_t& type, int multiplier, const CStdString& strFromCharset, const CStdString& strToCharset, const INPUT& strSource,  OUTPUT& strDest)
  {
    if (type == (iconv_t) - 1)
    {
      type = iconv_open(strToCharset.c_str(), strFromCharset.c_str());
    }

    if (type != (iconv_t) - 1)
    {
      size_t inBytes  = (strSource.length() + 1)*sizeof(strSource[0]);
      size_t outBytes = (strSource.length() + 1)*multiplier;
      const char *src = (const char*)strSource.c_str();
      char       *dst = (char*)strDest.GetBuffer(outBytes);

      if (iconv_const(type, &src, &inBytes, &dst, &outBytes) == (size_t)-1)
      {
        CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
        strDest.ReleaseBuffer();
        strDest = strSource;
        return;
      }

      if (iconv_const(type, NULL, NULL, &dst, &outBytes) == (size_t)-1)
      {
        CLog::Log(LOGERROR, "%s failed cleanup", __FUNCTION__);
        strDest.ReleaseBuffer();
        strDest = strSource;
        return;
      }

      strDest.ReleaseBuffer();
    }
  }

  void utf8ToW(const CStdStringA& utf8String, CStdStringW &utf16String, bool bVisualBiDiFlip=true);

  void utf16LEtoW(const char* utf16String, CStdStringW &wString);

  void subtitleCharsetToW(const CStdStringA& strSource, CStdStringW& strDest);

  void utf8ToStringCharset(const CStdStringA& strSource, CStdStringA& strDest);

  void utf8ToStringCharset(CStdStringA& strSourceDest);

  void stringCharsetToUtf8(const CStdStringA& strSource, CStdStringA& strDest);

  void stringCharsetToUtf8(const CStdStringA& strSourceCharset, const CStdStringA& strSource, CStdStringA& strDest);
  void stringCharsetTo(const CStdStringA& strDestCharset, const CStdStringA& strSource, CStdStringA& strDest);

  void stringCharsetToUtf8(CStdStringA& strSourceDest);

  bool isValidUtf8(const CStdString& str);

  bool isValidUtf8(const char *buf, unsigned int len);

  void ucs2CharsetToStringCharset(const CStdStringW& strSource, CStdStringA& strDest, bool swap = false);

  void wToUTF8(const CStdStringW& strSource, CStdStringA &strDest);
  void utf16BEtoUTF8(const CStdStringW& strSource, CStdStringA &strDest);
  void utf16LEtoUTF8(const CStdStringW& strSource, CStdStringA &strDest);
  void ucs2ToUTF8(const CStdStringW& strSource, CStdStringA& strDest);

  void utf32ToStringCharset(const unsigned long* strSource, CStdStringA& strDest);

  std::vector<CStdString> getCharsetLabels();
  CStdString& getCharsetLabelByName(const CStdString& charsetName);
  CStdString& getCharsetNameByLabel(const CStdString& charsetLabel);
  bool isBidiCharset(const CStdString& charset);

  void logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest, FriBidiCharSet fribidiCharset, FriBidiCharType base = FRIBIDI_TYPE_LTR);
  void logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest, CStdStringA& charset, FriBidiCharType base = FRIBIDI_TYPE_LTR);

private:
  std::vector<CStdString> m_vecCharsetNames;
  std::vector<CStdString> m_vecCharsetLabels;
  std::vector<CStdString> m_vecBidiCharsetNames;
  std::vector<FriBidiCharSet> m_vecBidiCharsets;

  iconv_t m_iconvStringCharsetToFontCharset;
  iconv_t m_iconvSubtitleCharsetToW;
  iconv_t m_iconvUtf8ToStringCharset;
  iconv_t m_iconvStringCharsetToUtf8;
  iconv_t m_iconvUcs2CharsetToStringCharset;
  iconv_t m_iconvUtf32ToStringCharset;
  iconv_t m_iconvWtoUtf8;
  iconv_t m_iconvUtf16LEtoW;
  iconv_t m_iconvUtf16BEtoUtf8;
  iconv_t m_iconvUtf16LEtoUtf8;
  iconv_t m_iconvUtf8toW;
  iconv_t m_iconvUcs2CharsetToUtf8;
  
  FriBidiCharSet m_stringFribidiCharset;

  CStdString EMPTY;
};

extern CCharsetConverter g_charsetConverter;

#endif
