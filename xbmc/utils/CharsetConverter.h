#ifndef CCHARSET_CONVERTER
#define CCHARSET_CONVERTER

#pragma once

#include <vector>
#include "stdstring.h"
#include "../lib/libiconv/iconv.h"
#include "../lib/libfribidi/fribidi.h"

class CCharsetConverter
{
public:
  CCharsetConverter();

  void reset();

  void stringCharsetToFontCharset(const CStdStringA& strSource, CStdStringW& strDest);

  void utf8ToStringCharset(const CStdStringA& strSource, CStdStringA& strDest);

  void ucs2CharsetToStringCharset(const CStdStringW& strSource, CStdStringA& strDest);

  vector<CStdString> getCharsetLabels();
  CStdString& getCharsetLabelByName(const CStdString& charsetName);
  CStdString& getCharsetNameByLabel(const CStdString& charsetLabel);
  boolean isBidiCharset(const CStdString& charset);

    void logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest);

private:
  
  vector<CStdString> m_vecCharsetNames;
  vector<CStdString> m_vecCharsetLabels;
  vector<CStdString> m_vecBidiCharsetNames;
  vector<FriBidiCharSet> m_vecBidiCharsets;

  iconv_t m_iconvStringCharsetToFontCharset;
  iconv_t m_iconvUtf8ToStringCharset;
  iconv_t m_iconvUcs2CharsetToStringCharset;

  FriBidiCharSet m_fribidiCharset;

  CStdString EMPTY;
};

extern CCharsetConverter g_charsetConverter;

#endif