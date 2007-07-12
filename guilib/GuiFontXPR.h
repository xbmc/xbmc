/*!
\file GUIFont.h
\brief 
*/
#ifdef HAS_XPR_FONTS
#ifndef CGUILIB_GUIFONTXPR_H
#define CGUILIB_GUIFONTXPR_H
#pragma once

#include "GUIFontBase.h"
#include "common/xbfont.h"

/*!
 \ingroup textures
 \brief 
 */
class CGUIFontXPR: public CGUIFontBase
{
public:
  CGUIFontXPR(const CStdString& strFileName);

  boolean Load(const CStdString& strFileName);

  virtual ~CGUIFontXPR(void);

 
  D3DTexture* CreateTexture( const WCHAR* strText,
                             D3DCOLOR dwBackgroundColor = 0x00000000,
                             D3DCOLOR dwTextColor = 0xffffffff,
                             D3DFORMAT d3dFormat = D3DFMT_LIN_A8R8G8B8 );
  virtual void Begin();
  virtual void End();

  void CreditsGetTextExtent( const WCHAR* strText, FLOAT* pWidth, FLOAT* pHeight, BOOL bFirstLineOnly = FALSE);

protected:
  virtual void GetTextExtentInternal(const WCHAR* strText, FLOAT* pWidth,
                             FLOAT* pHeight, BOOL bFirstLineOnly = FALSE);


  virtual void DrawTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                            const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
                            FLOAT fMaxPixelWidth = 0.0f);

  virtual void DrawColourTextImpl(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette,
                                  const WCHAR* strText, BYTE* pbColours, DWORD cchText, DWORD dwFlags,
                                  FLOAT fMaxPixelWidth);

  CXBFont m_font;
};

#endif
#endif
