#include "stdafx.h"
#include "guifontxpr.h"
#include "graphiccontext.h"

WCHAR*	CGUIFont::m_pwzBuffer	= NULL;
INT		CGUIFont::m_nBufferSize	= 0;

CGUIFont::CGUIFont(const CStdString& strFontName)
{
	m_strFontName = strFontName;
}

CGUIFont::~CGUIFont()
{
}

CStdString& CGUIFont::GetFontName()
{
	return m_strFontName;
}

void CGUIFont::DrawShadowText( FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                              const WCHAR* strText, DWORD dwFlags,
                              FLOAT fMaxPixelWidth, 
                              int iShadowWidth, 
                              int iShadowHeight,
                              DWORD dwShadowColor)
{
  Begin();
	float nw=0.0f,nh=0.0f;
	g_graphicsContext.Correct(fOriginX, fOriginY);

	for (int x=-iShadowWidth; x < iShadowWidth; x++)
	{
		for (int y=-iShadowHeight; y < iShadowHeight; y++)
		{
			DrawTextImpl( (float)x+fOriginX, (float)y+fOriginY, dwShadowColor, strText, wcslen( strText ), dwFlags);	
		}
	}

	DrawTextImpl( fOriginX, fOriginY, dwColor, strText, wcslen( strText ), dwFlags);	
  End();
}


void CGUIFont::DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                              const WCHAR* strText,float fMaxWidth)
{
	float nh=0.0f;
	g_graphicsContext.Correct(fOriginX, fOriginY);

	WCHAR wszText[1024];
	wcscpy(wszText,strText);
  
	float fTextHeight,fTextWidth;
	GetTextExtent( wszText, &fTextWidth,&fTextHeight);
	if (fTextWidth <=fMaxWidth)
	{
		DrawTextImpl( fOriginX, fOriginY, dwColor, wszText, wcslen( wszText ),0, 0.0f);	
		return;
	}

	int iMinCharsLeft;
	int iStrLength = wcslen(wszText);
	while (fTextWidth >= fMaxWidth && fTextWidth > 0)
	{
		iMinCharsLeft = (int)((fTextWidth - fMaxWidth) / m_iMaxCharWidth);
		if (iMinCharsLeft > 5)
		{
			// at least 5 chars are left, strip al remaining characters instead
			// of doing it one by one.
			iStrLength -= iMinCharsLeft;
			wszText[iStrLength] = 0;
			GetTextExtent(wszText, &fTextWidth, &fTextHeight);
		}
		else
		{
			wszText[--iStrLength] = 0;
			GetTextExtent(wszText, &fTextWidth, &fTextHeight);
		}
	}

  DrawTextImpl( fOriginX, fOriginY, dwColor, wszText, wcslen( wszText ),0, 0.0f);	
}

void CGUIFont::DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
                              const WCHAR* strText, BYTE* pbColours, float fMaxWidth)
{

	float nh=0.0f;
	g_graphicsContext.Correct(fOriginX, fOriginY);

	int nStringLength = wcslen(strText);
	if ((nStringLength+1)>m_nBufferSize)
	{
		if (m_pwzBuffer)
		{
			delete[] m_pwzBuffer;
		}
		
		m_nBufferSize = nStringLength + 1;
		m_pwzBuffer = new WCHAR[m_nBufferSize];
	}

	wcscpy(m_pwzBuffer,strText);
  
	float fTextHeight,fTextWidth;
	GetTextExtent( m_pwzBuffer, &fTextWidth,&fTextHeight);
	if (fTextWidth <=fMaxWidth)
	{
		DrawColourTextImpl( fOriginX, fOriginY, pdw256ColorPalette, m_pwzBuffer, pbColours, nStringLength,0, 0.0f);	
		return;
	}

	int iMinCharsLeft;
	while (fTextWidth >= fMaxWidth)
	{
		iMinCharsLeft = (int)((fTextWidth - fMaxWidth) / m_iMaxCharWidth);
		if (iMinCharsLeft > 5)
		{
			// at least 5 chars are left, strip al remaining characters instead
			// of doing it one by one.
			nStringLength -= iMinCharsLeft;
			m_pwzBuffer[ nStringLength ] = 0;
			GetTextExtent( m_pwzBuffer, &fTextWidth,&fTextHeight);
		}
		else
		{
			m_pwzBuffer[ --nStringLength ] = 0;
			GetTextExtent( m_pwzBuffer, &fTextWidth,&fTextHeight);
		}
	}

	DrawColourTextImpl( fOriginX, fOriginY, pdw256ColorPalette, m_pwzBuffer, pbColours, wcslen( m_pwzBuffer ),0, 0.0f);	
}

void CGUIFont::DrawText( FLOAT sx, FLOAT sy, DWORD dwColor, const WCHAR* strText, DWORD dwFlags,FLOAT fMaxPixelWidth)
{
	float nw=0.0f,nh=0.0f;
	g_graphicsContext.Correct(sx, sy);

   DrawTextImpl( sx, sy, dwColor, strText, wcslen( strText ),dwFlags, fMaxPixelWidth );
}
  
FLOAT  CGUIFont::GetTextWidth(const WCHAR* strText)
{
	FLOAT fTextWidth  = 0.0f;
	FLOAT fTextHeight = 0.0f;

	GetTextExtent( strText, &fTextWidth, &fTextHeight );

	return fTextWidth;
}
