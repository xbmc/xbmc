#include "stdafx.h"
#include "../settings.h"
#include "CharsetConverter.h"

CCharsetConverter g_charsetConverter;

CCharsetConverter::CCharsetConverter()
{
	m_vecCharsetNames.push_back("ISO-8859-1");
	m_vecCharsetLabels.push_back("Western Europe (ISO)");
	m_vecCharsetNames.push_back("ISO-8859-2");
	m_vecCharsetLabels.push_back("Central Europe (ISO)");
	m_vecCharsetNames.push_back("ISO-8859-3");
	m_vecCharsetLabels.push_back("South Europe (ISO)");
	m_vecCharsetNames.push_back("ISO-8859-4");
	m_vecCharsetLabels.push_back("Baltic (ISO)");
	m_vecCharsetNames.push_back("ISO-8859-5");
	m_vecCharsetLabels.push_back("Cyrillic (ISO)");
	m_vecCharsetNames.push_back("ISO-8859-6");
	m_vecCharsetLabels.push_back("Arabic (ISO)");
	m_vecCharsetNames.push_back("ISO-8859-7");
	m_vecCharsetLabels.push_back("Greek (ISO)");
	m_vecCharsetNames.push_back("ISO-8859-8");
	m_vecCharsetLabels.push_back("Hebrew (ISO)");
	m_vecCharsetNames.push_back("ISO-8859-9");
	m_vecCharsetLabels.push_back("Turkish (ISO)");

	m_vecCharsetNames.push_back("CP1250");
	m_vecCharsetLabels.push_back("Central Europe (Windows)");
	m_vecCharsetNames.push_back("CP1251");
	m_vecCharsetLabels.push_back("Cyrillic (Windows)");
	m_vecCharsetNames.push_back("CP1252");
	m_vecCharsetLabels.push_back("Western Europe (Windows)");
	m_vecCharsetNames.push_back("CP1253");
	m_vecCharsetLabels.push_back("Greek (Windows)");
	m_vecCharsetNames.push_back("CP1254");
	m_vecCharsetLabels.push_back("Turkish (Windows)");
	m_vecCharsetNames.push_back("CP1255");
	m_vecCharsetLabels.push_back("Hebrew (Windows)");
	m_vecCharsetNames.push_back("CP1256");
	m_vecCharsetLabels.push_back("Arabic (Windows)");
	m_vecCharsetNames.push_back("CP1257");
	m_vecCharsetLabels.push_back("Baltic (Windows)");
	m_vecCharsetNames.push_back("CP1258");
	m_vecCharsetLabels.push_back("Vietnamesse (Windows)");
	m_vecCharsetNames.push_back("CP874");
	m_vecCharsetLabels.push_back("Thai (Windows)");

	m_vecCharsetNames.push_back("BIG5");
	m_vecCharsetLabels.push_back("Chinese Traditional (Big5)");
	m_vecCharsetNames.push_back("GBK");
	m_vecCharsetLabels.push_back("Chinese Simplified (GBK)");
	m_vecCharsetNames.push_back("SHIFT_JIS");
	m_vecCharsetLabels.push_back("Japanese (Shift-JIS)");
	m_vecCharsetNames.push_back("CP949");
	m_vecCharsetLabels.push_back("Korean");
	m_vecCharsetNames.push_back("BIG5-HKSCS");
	m_vecCharsetLabels.push_back("Hong Kong (Big5-HKSCS)");

	m_vecBidiCharsetNames.push_back("ISO-8859-6");
	m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_ISO8859_6);
	m_vecBidiCharsetNames.push_back("ISO-8859-8");
	m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_ISO8859_8);
	m_vecBidiCharsetNames.push_back("CP1255");
	m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_CP1255);
	m_vecBidiCharsetNames.push_back("CP1256");
	m_vecBidiCharsets.push_back(FRIBIDI_CHARSET_CP1256);

	reset();
}

vector<CStdString> CCharsetConverter::getCharsetLabels()
{
	return m_vecCharsetLabels;
}

CStdString& CCharsetConverter::getCharsetLabelByName(const CStdString& charsetName)
{
	for (unsigned int i = 0; i < m_vecCharsetNames.size(); i++)
	{
		if (m_vecCharsetNames[i] == charsetName)
		{
			return m_vecCharsetLabels[i];
		}
	}

	return EMPTY;
}

CStdString& CCharsetConverter::getCharsetNameByLabel(const CStdString& charsetLabel)
{
	for (unsigned int i = 0; i < m_vecCharsetLabels.size(); i++)
	{
		if (m_vecCharsetLabels[i] == charsetLabel)
		{
			return m_vecCharsetNames[i];
		}
	}

	return EMPTY;
}

boolean CCharsetConverter::isBidiCharset(const CStdString& charset)
{
	for (unsigned int i = 0; i < m_vecBidiCharsetNames.size(); i++)
	{
		if (m_vecBidiCharsetNames[i] == charset)
		{
			return true;
		}
	}

	return false;
}

void CCharsetConverter::reset(void)
{
	m_iconvStringCharsetToFontCharset = (iconv_t) -1;
	m_iconvUtf8ToStringCharset = (iconv_t) -1;
	m_iconvUcs2CharsetToStringCharset  = (iconv_t) -1;
	m_fribidiCharset = FRIBIDI_CHARSET_NOT_FOUND;

	for (unsigned int i = 0; i < m_vecBidiCharsetNames.size(); i++)
	{
		if (m_vecBidiCharsetNames[i] == g_stSettings.m_szStringCharset)
		{
			m_fribidiCharset = m_vecBidiCharsets[i];
		}
	}
}

void CCharsetConverter::stringCharsetToFontCharset(const CStdStringA& strSource, CStdStringW& strDest)
{
	CStdStringA strFlipped;

	const char* src;
	size_t inBytes;

	// If this is hebrew/arabic, flip the characters, if required
	if (g_stSettings.m_bFlipBiDiCharset && m_fribidiCharset != FRIBIDI_CHARSET_NOT_FOUND)
	{   
		logicalToVisualBiDi(strSource, strFlipped);
		src = strFlipped.c_str();
		inBytes = strFlipped.length() + 1;
	}
	else
	{
		src = strSource.c_str();
		inBytes = strSource.length() + 1;
	}

	if (m_iconvStringCharsetToFontCharset == (iconv_t) -1)
	{
		m_iconvStringCharsetToFontCharset = iconv_open("UTF-16", g_stSettings.m_szStringCharset);
	}

	if (m_iconvStringCharsetToFontCharset != (iconv_t) -1)
	{
		char *dst = (char*) strDest.SetBuf(inBytes * 2);
		size_t outBytes = inBytes * 2;

		iconv(m_iconvStringCharsetToFontCharset, &src, &inBytes, &dst, &outBytes);

		// Swap bytes since XBOX doesn't know how to handle the endien marker
		char* s = (char*) strDest.c_str();
		while (*s || *(s+1))
		{
			char c = *s;
			*s = *(s+1);
			*(s+1) = c;

			s++;
			s++;
		}
	}
}

void CCharsetConverter::logicalToVisualBiDi(const CStdStringA& strSource, CStdStringA& strDest)
{
	int sourceLen = strlen(strSource.c_str());
	FriBidiChar* logical = (FriBidiChar*) malloc((sourceLen+1) * sizeof(FriBidiChar)); 
	FriBidiChar* visual = (FriBidiChar*) malloc((sourceLen+1) * sizeof(FriBidiChar)); 
	// Convert from the selected charset to Unicode
	int len = fribidi_charset_to_unicode(m_fribidiCharset, (char*) strSource.c_str(), sourceLen, logical);

	// Convert from logical to visual
	FriBidiCharType base = FRIBIDI_TYPE_L; // Right-to-left paragraph

	if (fribidi_log2vis(logical, len, &base, visual, NULL, NULL, NULL))
	{
 		// Removes bidirectional marks
 		//len = fribidi_remove_bidi_marks(visual, len, NULL, NULL, NULL);

		char* result = strDest.SetBuf(sourceLen+1);

		// Convert back from Unicode to the charset 
 		fribidi_unicode_to_charset(m_fribidiCharset, visual, len, result);
	}

	free(logical);
	free(visual);
}

void CCharsetConverter::utf8ToStringCharset(const CStdStringA& strSource, CStdStringA& strDest)
{
	if (m_iconvUtf8ToStringCharset == (iconv_t) -1)
	{
		m_iconvUtf8ToStringCharset = iconv_open(g_stSettings.m_szStringCharset, "UTF-8");
	}

	if (m_iconvUtf8ToStringCharset != (iconv_t) -1)
	{
		const char* src = strSource.c_str();
		size_t inBytes = strSource.length() + 1;

		char *dst = strDest.SetBuf(inBytes);
		size_t outBytes = inBytes;

		if (iconv(m_iconvUtf8ToStringCharset, &src, &inBytes, &dst, &outBytes) == -1)
		{
			// For some reason it failed (maybe wrong charset?). Nothing to do but
			// return the original..
			strDest = strSource;
		}
	}
}

void CCharsetConverter::ucs2CharsetToStringCharset(const CStdStringW& strSource, CStdStringA& strDest)
{
	if (m_iconvUcs2CharsetToStringCharset == (iconv_t) -1)
	{
		m_iconvUcs2CharsetToStringCharset = iconv_open(g_stSettings.m_szStringCharset, "UCS-2");
	}

	if (m_iconvUcs2CharsetToStringCharset != (iconv_t) -1)
	{
		const char* src = (const char*) strSource.c_str();
		size_t inBytes = (strSource.length() + 1) * 2;
		
		// Swap bytes since XBOX doesn't know how to handle the endien marker
		char* s = (char*) src;
		while (*s || *(s+1))
		{
			char c = *s;
			*s = *(s+1);
			*(s+1) = c;

			s++;
			s++;
		}

		char *dst = strDest.SetBuf(inBytes);
		size_t outBytes = inBytes;

		iconv(m_iconvUcs2CharsetToStringCharset, &src, &inBytes, &dst, &outBytes);
	}
}

