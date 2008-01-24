#ifndef CCHARSET_CONVERTER
#define CCHARSET_CONVERTER

#pragma once

#include "../lib/libiconv/iconv.h"
#include "../lib/libfribidi/fribidi.h"

#include <vector>

class CCharsetConverter
{
public:
  CCharsetConverter();

  void reset();

  void clear();

  void utf8ToUTF16(const CStdStringA& utf8String, CStdStringW &utf16String, bool bVisualBiDiFlip=true);

  void subtitleCharsetToUTF16(const CStdStringA& strSource, CStdStringW& strDest);

  void utf8ToStringCharset(const CStdStringA& strSource, CStdStringA& strDest);

  void utf8ToStringCharset(CStdStringA& strSourceDest);

  void stringCharsetToUtf8(const CStdStringA& strSource, CStdStringA& strDest);

  void stringCharsetToUtf8(const CStdStringA& strSourceCharset, const CStdStringA& strSource, CStdStringA& strDest);

  void stringCharsetToUtf8(CStdStringA& strSourceDest);

  bool isValidUtf8(const CStdString& str);

  bool isValidUtf8(const char *buf, unsigned int len);

  void ucs2CharsetToStringCharset(const CStdStringW& strSource, CStdStringA& strDest, bool swap = false);

  void utf16toUTF8(const CStdStringW& strSource, CStdStringA &strDest);
  void utf16BEtoUTF8(const CStdStringW& strSource, CStdStringA &strDest);

  void utf32ToStringCharset(const unsigned long* strSource, CStdStringA& strDest);

  std::vector<CStdString> getCharsetLabels();
  CStdString& getCharsetLabelByName(const CStdString& charsetName);
  CStdString& getCharsetNameByLabel(const CStdString& charsetLabel);
  boolean isBidiCharset(const CStdString& charset);

  void logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest, FriBidiCharSet fribidiCharset, FriBidiCharType base = FRIBIDI_TYPE_LTR);
  void logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest, CStdStringA& charset, FriBidiCharType base = FRIBIDI_TYPE_LTR);

private:
  std::vector<CStdString> m_vecCharsetNames;
  std::vector<CStdString> m_vecCharsetLabels;
  std::vector<CStdString> m_vecBidiCharsetNames;
  std::vector<FriBidiCharSet> m_vecBidiCharsets;

  iconv_t m_iconvStringCharsetToFontCharset;
  iconv_t m_iconvSubtitleCharsetToUtf16;
  iconv_t m_iconvUtf8ToStringCharset;
  iconv_t m_iconvStringCharsetToUtf8;
  iconv_t m_iconvUcs2CharsetToStringCharset;
  iconv_t m_iconvUtf32ToStringCharset;
  iconv_t m_iconvUtf16toUtf8;
  iconv_t m_iconvUtf16BEtoUtf8;
  iconv_t m_iconvUtf8toUtf16;

  FriBidiCharSet m_stringFribidiCharset;

  CStdString EMPTY;
};

extern CCharsetConverter g_charsetConverter;

#endif
