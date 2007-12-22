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
  CGUIFont(const CStdString& strFontName, DWORD style, DWORD textColor, DWORD shadowColor, CGUIFontTTF *font);
  virtual ~CGUIFont();

  CStdString& GetFontName();

  void DrawText( float x, float y, DWORD color, DWORD shadowColor,
                 const vector<DWORD> &text, DWORD alignment, float maxPixelWidth)
  {
    vector<DWORD> colors;
    colors.push_back(color);
    DrawText(x, y, colors, shadowColor, text, alignment, maxPixelWidth);
  };

  void DrawText( float x, float y, const vector<DWORD> &colors, DWORD shadowColor,
                 const vector<DWORD> &text, DWORD alignment, float maxPixelWidth);

  void DrawScrollingText( float x, float y, const vector<DWORD> &colors, DWORD shadowColor,
                 const vector<DWORD> &text, DWORD alignment, float maxPixelWidth, CScrollInfo &scrollInfo);

  float GetTextWidth( const vector<DWORD> &text )
  {
    if (!m_font) return 0;
    CSingleLock lock(g_graphicsContext);
    return m_font->GetTextWidthInternal(text.begin(), text.end()) * g_graphicsContext.GetGUIScaleX();
  };

  float GetCharWidth( DWORD ch )
  {
    if (!m_font) return 0;
    CSingleLock lock(g_graphicsContext);
    return m_font->GetCharWidthInternal(ch) * g_graphicsContext.GetGUIScaleX();
  }

  float GetTextHeight(int numLines) const
  {
    if (!m_font) return 0;
    return m_font->GetTextHeight(numLines) * g_graphicsContext.GetGUIScaleY();
  };

  float GetLineHeight() const
  {
    if (!m_font) return 0;
    return m_font->m_lineHeight * g_graphicsContext.GetGUIScaleY();
  };

  void Begin() { if (m_font) m_font->Begin(); };
  void End() { if (m_font) m_font->End(); };

  DWORD GetStyle() const { return m_style; };

  static SHORT RemapGlyph(SHORT letter);
  
  CGUIFontTTF* GetFont() const
  {
     return m_font;
  }
  
  void SetFont(CGUIFontTTF* font)
  {
     m_font = font;
  }
  
protected:
  CStdString m_strFontName;
  DWORD m_style;
  DWORD m_shadowColor;
  DWORD m_textColor;
  CGUIFontTTF *m_font; // the font object has the size information
  
private:
  bool ClippedRegionIsEmpty(float x, float y, float width, DWORD alignment) const;
};

#endif
