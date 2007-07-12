#include "include.h"
#ifdef HAS_XPR_FONTS
#include "GUIFontXPR.h"


CGUIFontXPR::CGUIFontXPR(const CStdString& strFileName)
  : CGUIFontBase(strFileName)
{}

CGUIFontXPR::~CGUIFontXPR(void)
{}

boolean CGUIFontXPR::Load(const CStdString& strFilename)
{
  try
  {
    CLog::Log(LOGINFO, "Load font:%s path:%s", m_strFileName.c_str(), strFilename.c_str());
    bool bResult = (m_font.Create(strFilename.c_str()) == S_OK);
    if (!bResult)
    {
      CLog::Log(LOGERROR, "failed to load Load font:%s path:%s", m_strFileName.c_str(), strFilename.c_str());
    }
    else
    {
      float fTextHeight, fTextWidth;
      GetTextExtentInternal( L"W", &fTextWidth, &fTextHeight);
      m_iMaxCharWidth = (unsigned int)fTextWidth;
    }

    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "failed to load font:%s file:%s", m_strFileName.c_str(), strFilename.c_str());
  }

  return false;
}

void CGUIFontXPR::Begin()
{
  m_font.Begin();
}

void CGUIFontXPR::End()
{
  m_font.End();
}

void CGUIFontXPR::DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                               const WCHAR* strText, DWORD cchText, DWORD dwFlags,
                               FLOAT fMaxPixelWidth)
{
  m_font.DrawTextEx(fOriginX, fOriginY, dwColor, strText, cchText>2048?2048:cchText, dwFlags, fMaxPixelWidth);
}

void CGUIFontXPR::GetTextExtentInternal(const WCHAR* strText, FLOAT* pWidth,
                                FLOAT* pHeight, BOOL bFirstLineOnly)
{
  m_font.GetTextExtent(strText, pWidth, pHeight, bFirstLineOnly);
}

void CGUIFontXPR::DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
                                     const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
                                     FLOAT fMaxPixelWidth)
{
  m_font.DrawColourText(fOriginX, fOriginY, pdw256ColorPalette, strText, pbColours, cchText>2048?2048:cchText, dwFlags, fMaxPixelWidth);
}

D3DTexture* CGUIFontXPR::CreateTexture( const WCHAR* strText,
                                        D3DCOLOR dwBackgroundColor,
                                        D3DCOLOR dwTextColor,
                                        D3DFORMAT d3dFormat)
{
  return m_font.CreateTexture(strText, dwBackgroundColor, dwTextColor, d3dFormat);
}

void CGUIFontXPR::CreditsGetTextExtent(const WCHAR* strText, FLOAT* pWidth,
                                FLOAT* pHeight, BOOL bFirstLineOnly)
{
  m_font.GetTextExtent(strText, pWidth, pHeight, bFirstLineOnly);
}
#endif
