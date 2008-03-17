#ifndef CCHARSET_CONVERTER
#define CCHARSET_CONVERTER

#pragma once

#ifndef _LINUX
#include "../lib/libiconv/iconv.h"
#include "../lib/libfribidi/fribidi.h"
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
#elif defined(_XBOX) || defined(WIN32)
  #define WCHAR_CHARSET "UTF-16LE"
#else
  #define WCHAR_CHARSET "WCHAR_T"
#endif

class CCharsetConverter
{
public:
  CCharsetConverter();

  void reset();

  void clear();

  void utf8ToW(const CStdStringA& utf8String, CStdStringW &utf16String, bool bVisualBiDiFlip=true);

  void utf16LEtoW(const char* utf16String, CStdStringW &wString);

  void subtitleCharsetToW(const CStdStringA& strSource, CStdStringW& strDest);

  void utf8ToStringCharset(const CStdStringA& strSource, CStdStringA& strDest);

  void utf8ToStringCharset(CStdStringA& strSourceDest);

  void stringCharsetToUtf8(const CStdStringA& strSource, CStdStringA& strDest);

  void stringCharsetToUtf8(const CStdStringA& strSourceCharset, const CStdStringA& strSource, CStdStringA& strDest);

  void stringCharsetToUtf8(CStdStringA& strSourceDest);

  bool isValidUtf8(const CStdString& str);

  bool isValidUtf8(const char *buf, unsigned int len);

  void ucs2CharsetToStringCharset(const CStdStringW& strSource, CStdStringA& strDest, bool swap = false);

  void wToUTF8(const CStdStringW& strSource, CStdStringA &strDest);
  void utf16BEtoUTF8(const CStdStringW& strSource, CStdStringA &strDest);

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
  iconv_t m_iconvUtf8toW;

  FriBidiCharSet m_stringFribidiCharset;

  CStdString EMPTY;
};

extern CCharsetConverter g_charsetConverter;

#endif
