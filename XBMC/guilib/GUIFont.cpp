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
  return (CXBFont::Create(strPath.c_str())==S_OK);

}

void CGUIFont::DrawShadowText( FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                              const WCHAR* strText, DWORD dwFlags,
                              FLOAT fMaxPixelWidth, 
                              int iShadowWidth, 
                              int iShadowHeight,
                              DWORD dwShadowColor)
{
  for (int x=0; x < iShadowWidth; x++)
  {
    for (int y=0; y < iShadowHeight; y++)
    {
      CXBFont::DrawText(fOriginX+x, fOriginY+y, dwShadowColor,strText,dwFlags,fMaxPixelWidth );
    }
  }
  CXBFont::DrawText(fOriginX, fOriginY, dwColor,strText,dwFlags,fMaxPixelWidth );
}


void CGUIFont::DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                              const WCHAR* strText,float fMaxWidth)
{
  WCHAR wszText[1024];
  wcscpy(wszText,strText);
  
  float fTextHeight,fTextWidth;
  GetTextExtent( wszText, &fTextWidth,&fTextHeight);

  while (fTextWidth >= fMaxWidth)
  {
    wszText[ wcslen(wszText)-1 ] = 0;
    GetTextExtent( wszText, &fTextWidth,&fTextHeight);
  }
  CXBFont::DrawText(fOriginX,fOriginY,dwColor,wszText);
}