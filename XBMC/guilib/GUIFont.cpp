#include "stdafx.h"
#include "guifont.h"
#include "../xbmc/utils/log.h"

WCHAR*	CGUIFont::m_pwzBuffer	= NULL;
INT		CGUIFont::m_nBufferSize	= 0;

CGUIFont::CGUIFont(void)
{
	m_iMaxCharWidth = 0;
}

CGUIFont::~CGUIFont(void)
{
}


const CStdString& CGUIFont::GetFontName() const
{
  return m_strFontName;
}

bool CGUIFont::Load(const CStdString& strFontName,const CStdString& strFilename)
{
  CStdString strPath;
	if (strFilename[1] != ':')
	{
		strPath=g_graphicsContext.GetMediaDir();
		strPath+="\\fonts\\";
		strPath+=strFilename;
	}
	else
		strPath=strFilename;
  m_strFontName=strFontName;
  CLog::Log("Load font:%s path:%s", m_strFontName.c_str(), strPath.c_str());
  bool bResult= (CXBFont::Create(strPath.c_str())==S_OK);
	if (!bResult)
	{
    CLog::Log("failed to load Load font:%s path:%s", m_strFontName.c_str(), strPath.c_str());
	}
	else
	{
		float fTextHeight;
		GetTextExtent( L"W", &m_iMaxCharWidth,&fTextHeight);
	}
		
	return bResult;
}

void CGUIFont::DrawShadowText( FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                              const WCHAR* strText, DWORD dwFlags,
                              FLOAT fMaxPixelWidth, 
                              int iShadowWidth, 
                              int iShadowHeight,
                              DWORD dwShadowColor)
{
	float nw=0.0f,nh=0.0f;
	g_graphicsContext.Correct(fOriginX, fOriginY);

	for (int x=-iShadowWidth; x < iShadowWidth; x++)
  {
    for (int y=-iShadowHeight; y < iShadowHeight; y++)
    {
			CXBFont::DrawTextEx( (float)x+fOriginX, (float)y+fOriginY, dwShadowColor, strText, wcslen( strText ));	
    }
  }
	CXBFont::DrawTextEx( fOriginX, fOriginY, dwColor, strText, wcslen( strText ));	
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
		CXBFont::DrawTextEx( fOriginX, fOriginY, dwColor, wszText, wcslen( wszText ),0, 0.0f);	
		return;
	}

	int iMinCharsLeft;
	int iStrLength = wcslen(wszText);
	while (fTextWidth >= fMaxWidth)
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

  CXBFont::DrawTextEx( fOriginX, fOriginY, dwColor, wszText, wcslen( wszText ),0, 0.0f);	
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
			delete m_pwzBuffer;
		}
		
		m_nBufferSize = nStringLength + 1;
		m_pwzBuffer = new WCHAR[m_nBufferSize];
	}

	wcscpy(m_pwzBuffer,strText);
  
	float fTextHeight,fTextWidth;
	GetTextExtent( m_pwzBuffer, &fTextWidth,&fTextHeight);
	if (fTextWidth <=fMaxWidth)
	{
		CXBFont::DrawColourText( fOriginX, fOriginY, pdw256ColorPalette, m_pwzBuffer, pbColours, nStringLength,0, 0.0f);	
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

	CXBFont::DrawColourText( fOriginX, fOriginY, pdw256ColorPalette, m_pwzBuffer, pbColours, wcslen( m_pwzBuffer ),0, 0.0f);	
}

HRESULT CGUIFont::DrawText( FLOAT sx, FLOAT sy, DWORD dwColor, const WCHAR* strText, DWORD dwFlags,FLOAT fMaxPixelWidth)
{
	float nw=0.0f,nh=0.0f;
	g_graphicsContext.Correct(sx, sy);

  return CXBFont::DrawTextEx( sx, sy, dwColor, strText, wcslen( strText ),dwFlags, fMaxPixelWidth );

}
HRESULT CGUIFont::DrawTextEx( FLOAT sx, FLOAT sy, DWORD dwColor, const WCHAR* strText, DWORD cchText, DWORD dwFlags,FLOAT fMaxPixelWidth )
{
	float nw=0.0f,nh=0.0f;
	g_graphicsContext.Correct(sx, sy);
	return CXBFont::DrawTextEx( sx, sy, dwColor, strText, cchText, dwFlags,fMaxPixelWidth );
}
