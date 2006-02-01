/*!
\file GUIFont.h
\brief 
*/

#ifndef CGUILIB_GUIFONT_H
#define CGUILIB_GUIFONT_H
#pragma once

#include "GUIFontBase.h"

class CScrollInfo
{
public:
  CScrollInfo(unsigned int wait = 50) { initialWait = wait; Reset(); };
  void Reset()
  {
    waitTime = initialWait;
    pixelPos = characterPos = 0;
  }
  unsigned int waitTime;
  unsigned int pixelPos;
  unsigned int characterPos;
  unsigned int initialWait;
};

/*!
 \ingroup textures
 \brief 
 */
class CGUIFont
{
public:
  CGUIFont(const CStdString& strFontName, DWORD textColor, DWORD shadowColor, CGUIFontBase *font);
  virtual ~CGUIFont();

  CStdString& GetFontName();

  void DrawText( FLOAT sx, FLOAT sy, const CAngle &angle, DWORD dwColor, DWORD dwShadowColor,
                 const WCHAR* strText, DWORD dwFlags = 0L,
                 FLOAT fMaxPixelWidth = 0.0f);

  void DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD dwColor, DWORD dwShadowColor,
                     const WCHAR* strText, float fMaxWidth);

  void DrawScrollingText(float x, float y, const CAngle &angle, DWORD* color, int numColors,
                         DWORD dwShadowColor, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette = NULL);

  void DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, const CAngle &angle, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                           const WCHAR* strText, BYTE* pbColours, float fMaxWidth);

  void DrawText( FLOAT sx, FLOAT sy, DWORD dwColor, DWORD dwShadowColor,
                 const WCHAR* strText, DWORD dwFlags = 0L,
                 FLOAT fMaxPixelWidth = 0.0f );

  void DrawTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor, DWORD dwShadowColor,
                     const WCHAR* strText, float fMaxWidth);

  void DrawColourTextWidth(FLOAT fOriginX, FLOAT fOriginY, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                           const WCHAR* strText, BYTE* pbColours, float fMaxWidth);

  void DrawScrollingText(float x, float y, DWORD* color, int numColors, DWORD dwShadowColor, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette = NULL);

  inline void GetTextExtent( const WCHAR* strText, FLOAT* pWidth, FLOAT* pHeight, BOOL bFirstLineOnly = FALSE);

  FLOAT GetTextWidth( const WCHAR* strText );
  inline void Begin() { if (m_font) m_font->Begin(); };
  inline void End() { if (m_font) m_font->End(); };

protected:
  CStdString m_strFontName;
  // for shadowed text
  DWORD m_shadowColor;
  DWORD m_textColor;
  CGUIFontBase *m_font;
private:
  CAngle Transform(const CAngle &angle);
};

#endif
