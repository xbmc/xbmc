#include "guifont.h"

CGUIFont::CGUIFont(void)
{
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
  CStdString strPath=g_graphicsContext.GetMediaDir();
	strPath+="\\fonts\\";
  strPath+=strFilename;
  m_strFontName=strFontName;
  bool bResult= (CXBFont::Create(strPath.c_str())==S_OK);
	if (!bResult)
	{
		OutputDebugString("Unable to load font:");
		OutputDebugString(strFilename.c_str());
		OutputDebugString("\n");
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

	for (int x=0; x < iShadowWidth; x++)
  {
    for (int y=0; y < iShadowHeight; y++)
    {
			CXBFont::DrawTextEx( fOriginX, fOriginY, dwColor, strText, wcslen( strText ));	
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

  while (fTextWidth >= fMaxWidth)
  {
    wszText[ wcslen(wszText)-1 ] = 0;
    GetTextExtent( wszText, &fTextWidth,&fTextHeight);
  }

  CXBFont::DrawTextEx( fOriginX, fOriginY, dwColor, wszText, wcslen( wszText ),0, 0.0f);	
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
