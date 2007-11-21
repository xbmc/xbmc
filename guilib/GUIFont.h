/*!
\file GUIFont.h
\brief 
*/

#ifndef CGUILIB_GUIFONT_H
#define CGUILIB_GUIFONT_H
#pragma once

#include "GUIFontTTF.h"
#include "GraphicContext.h"
#include "../xbmc/utils/SingleLock.h"

class CScrollInfo
{
public:
  CScrollInfo(unsigned int wait = 50, float pos = 0, float speed = 1.0f, const CStdStringW &scrollSuffix = L" | ") { initialWait = wait; initialPos = pos; pixelSpeed = speed; suffix = scrollSuffix; Reset(); };
  void Reset()
  {
    waitTime = initialWait;
    characterPos = 0;
    // pixelPos is where we start the current letter, so is measured
    // to the left of the text rendering's left edge.  Thus, a negative
    // value will mean the text starts to the right
    pixelPos = -initialPos;
  }
  float pixelPos;
  float pixelSpeed;
  unsigned int waitTime;
  unsigned int characterPos;
  unsigned int initialWait;
  float initialPos;
  CStdStringW suffix;
};

/*!
 \ingroup textures
 \brief 
 */
class CGUIFont
{
public:
  CGUIFont(const CStdString& strFontName, DWORD textColor, DWORD shadowColor, CGUIFontTTF *font);
  virtual ~CGUIFont();

  CStdString& GetFontName();

  void DrawOutlineText(float x, float y, DWORD color, DWORD outlineColor, int outlineWidth, const WCHAR *text, DWORD flags = 0L, float maxWidth = 0.0f);

  void DrawText( float x, float y, float angle, DWORD dwColor, DWORD dwShadowColor,
                 const WCHAR* strText, DWORD dwFlags = 0L,
                 FLOAT fMaxPixelWidth = 0.0f);

  void DrawTextWidth(float x, float y, float angle, DWORD dwColor, DWORD dwShadowColor,
                     const WCHAR* strText, float fMaxWidth);

  void DrawScrollingText(float x, float y, float angle, DWORD* color, int numColors,
                         DWORD dwShadowColor, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette = NULL);

  void DrawColourTextWidth(float x, float y, float angle, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                           const WCHAR* strText, BYTE* pbColours, float fMaxWidth);

  void DrawText( float x, float y, DWORD dwColor, DWORD dwShadowColor,
                 const WCHAR* strText, DWORD dwFlags = 0L,
                 FLOAT fMaxPixelWidth = 0.0f );

  void DrawTextWidth(float x, float y, DWORD dwColor, DWORD dwShadowColor,
                     const WCHAR* strText, float fMaxWidth);

  void DrawColourTextWidth(float x, float y, DWORD* pdw256ColorPalette, int numColors, DWORD dwShadowColor,
                           const WCHAR* strText, BYTE* pbColours, float fMaxWidth);

  void DrawScrollingText(float x, float y, DWORD* color, int numColors, DWORD dwShadowColor, const CStdStringW &text, float w, CScrollInfo &scrollInfo, BYTE *pPalette = NULL);

  void GetTextExtent( const WCHAR* strText, FLOAT* pWidth, FLOAT* pHeight, BOOL bFirstLineOnly = FALSE)
  {
    if (!m_font) return;
    CSingleLock lock(g_graphicsContext);
    m_font->GetTextExtentInternal(strText, pWidth, pHeight, bFirstLineOnly);
    *pWidth *= g_graphicsContext.GetGUIScaleX();
    *pHeight *= g_graphicsContext.GetGUIScaleY();
  }

  float GetTextWidth( const WCHAR* strText );
  float GetTextHeight( const WCHAR* strText );

  void Begin() { if (m_font) m_font->Begin(); };
  void End() { if (m_font) m_font->End(); };

  static SHORT RemapGlyph(SHORT letter);
protected:
  CStdString m_strFontName;
  // for shadowed text
  DWORD m_shadowColor;
  DWORD m_textColor;
  CGUIFontTTF *m_font;
private:
  void SetRotation(float sx, float sy, float angle);
};

#endif
