#include "stdafx.h"
#define XFONT_TRUETYPE 
#include "guifontttf.h"
#include "../xbmc/utils/log.h"

CGUIFontTTF::CGUIFontTTF(const CStdString& strFontName) : CGUIFont(strFontName)
{
	m_pTrueTypeFont = NULL;
}

CGUIFontTTF::~CGUIFontTTF(void)
{
	m_usedTTFRefCount[m_strFilename]--;

	if (m_usedTTFRefCount[m_strFilename] == 0)
	{
		m_usedTTFRefCount.erase(m_strFilename);
		m_usedTTFs.erase(m_strFilename);

		XFONT_Release(m_pTrueTypeFont);
	}
}

// Change font style: XFONT_NORMAL, XFONT_BOLD, XFONT_ITALICS, XFONT_BOLDITALICS
bool CGUIFontTTF::Load(const CStdString& strFilename, int iHeight, int iStyle)
{
	m_iHeight = iHeight;
	m_iStyle = iStyle;

    // size of the font cache in bytes
    DWORD dwFontCacheSize = 64 * 1024;

	m_strFilename = strFilename;

	WCHAR wszFilename[256];
	swprintf(wszFilename, L"%S", strFilename.c_str());

	if (m_usedTTFs[strFilename] == NULL)
	{
		if( FAILED( XFONT_OpenTrueTypeFont ( wszFilename,
											dwFontCacheSize,&m_pTrueTypeFont ) ) )
			return false;

		m_usedTTFs[strFilename] = m_pTrueTypeFont;
		m_usedTTFRefCount[strFilename] = 1;
	}
	else
	{
		m_pTrueTypeFont = m_usedTTFs[strFilename];
		m_usedTTFRefCount[strFilename]++;
	}

	m_pTrueTypeFont->SetTextHeight( iHeight );
	m_pTrueTypeFont->SetTextStyle( iStyle );
	// Anti-Alias the font -- 0 for no anti-alias, 2 for some, 4 for MAX!
	m_pTrueTypeFont->SetTextAntialiasLevel( 2 );
	m_pTrueTypeFont->SetTextAlignment(XFONT_CENTER);

	return true;
}


void CGUIFontTTF::DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
								  const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
								  FLOAT fMaxPixelWidth)
{
	// There is no real way to draw text as textured multi-colored polygons (maybe there is but I 
	// don't know how to do it)
	DrawTextImpl(fOriginX, fOriginY, pdw256ColorPalette[0], strText, cchText, dwFlags, fMaxPixelWidth);
}

void CGUIFontTTF::DrawTextImpl( FLOAT sx, FLOAT sy, DWORD dwColor, const WCHAR* strText, DWORD cchText, DWORD dwFlags,FLOAT fMaxPixelWidth )
{
	if (dwFlags & XBFONT_CENTER_Y)
	{
	    FLOAT w, h;
		GetTextExtent( strText, &w, &h );
		sy = floorf( sy - h/2 );
	}

	m_pTrueTypeFont->SetTextColor(dwColor);

	// Now draw the text line by line
	WCHAR buf[1024];
	int len = cchText;
	int i = 0,j = 0;
	unsigned lineHeight = m_pTrueTypeFont->GetTextHeight();
	unsigned lineWidth;
	while (i < len)
	{
		// find a carrage return
		for(j = i; j < len; j++)
		{
			if (strText[j] == L'\n')
				break;
		}

		// copy text up until the carriage return into a buffer
		wcsncpy(buf, strText+i, j - i);
		buf[j-i] = L'\0';

		// determine the position of the line
		m_pTrueTypeFont->GetTextExtent(buf, -1, &lineWidth);
		int ll = wcslen(buf);
		while (fMaxPixelWidth > 0 && ll >= 2 && lineWidth > fMaxPixelWidth + 60)
		{
			buf[ll-2] = L'.';
			buf[ll-1] = L'\0';
			ll--;
			m_pTrueTypeFont->GetTextExtent(buf, -1, &lineWidth);
		}

		DWORD flags = (dwFlags & XBFONT_LEFT ? XFONT_LEFT : 0) |
			(dwFlags & XBFONT_CENTER_X ? XFONT_CENTER : 0) |
			(dwFlags & XBFONT_RIGHT ? XFONT_RIGHT : 0) |
			XFONT_TOP;

		m_pTrueTypeFont->SetTextAlignment(flags);

		DrawTrueType((long)sx, (long)sy, (WCHAR *)buf, wcslen(strText));

		sy += lineHeight;

		// continue from next character
		i = j + 1;
	}
}

void CGUIFontTTF::GetTextExtent( const WCHAR* strText, FLOAT* pWidth, 
                           FLOAT* pHeight, BOOL bFirstLineOnly)
{
	unsigned width, lineHeight;
	// First let's calculate width
	WCHAR buf[1024];
	int len = wcslen(strText);
	int i = 0,j = 0;
	*pWidth = *pHeight = 0.0f;
	lineHeight = m_pTrueTypeFont->GetTextHeight();

	while (j < len) {

		for(j = i; j < len; j++)
			if (strText[j] == L'\n')
				break;

		wcsncpy(buf, strText+i, j - i);
		buf[j-i] = L'\0';
		m_pTrueTypeFont->GetTextExtent(strText, -1, &width);
		if (width > *pWidth)
			*pWidth = (float) width;
		*pHeight += lineHeight;

/*
	this was originally from XBMP - is this required in XBMC?

		if (g_graphicsContext.IsCorrectionEnabled())
		{	
			*pWidth  /= WIDE_SCREEN_COMPENSATIONX;
			*pHeight /= WIDE_SCREEN_COMPENSATIONY;
		}
*/
		if (bFirstLineOnly)
			return;
	}

	return;
}


void CGUIFontTTF::DrawTrueType(LONG nPosX, LONG nPosY, WCHAR* text, int len)
{
	if (m_pTrueTypeFont==NULL)
		return;

	LPDIRECT3DSURFACE8 pSurface;

	// draw to back buffer
	D3DDevice::GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO, &pSurface);

	int count = 0;
		
	for (int i = 0; i <= len; i++)
	{
		if (i == len || (text[i]==0x0A) || (text[i]==0x0D))
		{
			if (count == 0)
				continue;

			m_pTrueTypeFont->TextOut( pSurface, &text[i-count], count, nPosX, nPosY );

			count = 0;

			if (i + 1 < len && text[i+1] == 0x0A)
				i++;		// Windows LF

			nPosY += m_iHeight;
			continue;
		}

		count++;
	}

	pSurface->Release();
}

stdext::hash_map<string, XFONT*> CGUIFontTTF::m_usedTTFs;
stdext::hash_map<string, int>	 CGUIFontTTF::m_usedTTFRefCount;
