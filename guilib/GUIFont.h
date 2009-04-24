/*!
\file GUIFont.h
\brief
*/

#ifndef CGUILIB_GUIFONT_H
#define CGUILIB_GUIFONT_H
#pragma once

#include "GraphicContext.h"
#include "utils/SingleLock.h"

class CGUIFontTTF;

// flags for alignment
#define XBFONT_LEFT       0x00000000
#define XBFONT_RIGHT      0x00000001
#define XBFONT_CENTER_X   0x00000002
#define XBFONT_CENTER_Y   0x00000004
#define XBFONT_TRUNCATED  0x00000008
#define XBFONT_JUSTIFIED  0x00000010

#define FONT_STYLE_NORMAL       0
#define FONT_STYLE_BOLD         1
#define FONT_STYLE_ITALICS      2
#define FONT_STYLE_BOLD_ITALICS 3


class CScrollInfo
{
public:
  CScrollInfo(unsigned int wait = 50, float pos = 0, int speed = defaultSpeed, const CStdStringW &scrollSuffix = L" | ")
  {
    initialWait = wait;
    initialPos = pos;
    SetSpeed(speed);
    suffix = scrollSuffix;
    Reset();
  };
  void SetSpeed(int speed)
  {
    pixelSpeed = speed * 0.001f;
  }
  void Reset()
  {
    waitTime = initialWait;
    characterPos = 0;
    // pixelPos is where we start the current letter, so is measured
    // to the left of the text rendering's left edge.  Thus, a negative
    // value will mean the text starts to the right
    pixelPos = -initialPos;
    // privates:
    m_averageFrameTime = 1000.f / abs(defaultSpeed);
    m_lastFrameTime = 0;
  }
  DWORD GetCurrentChar(const std::vector<DWORD> &text) const
  {
    assert(text.size());
    if (characterPos < text.size())
      return text[characterPos];
    else if (characterPos < text.size() + suffix.size())
      return suffix[characterPos - text.size()];
    return text[0];
  }
  float GetPixelsPerFrame();

  float pixelPos;
  float pixelSpeed;
  unsigned int waitTime;
  unsigned int characterPos;
  unsigned int initialWait;
  float initialPos;
  CStdStringW suffix;

  static const int defaultSpeed = 60;
private:
  float m_averageFrameTime;
  DWORD m_lastFrameTime;
};

/*!
 \ingroup textures
 \brief
 */
class CGUIFont
{
public:
  CGUIFont(const CStdString& strFontName, DWORD style, DWORD textColor, DWORD shadowColor, float lineSpacing, CGUIFontTTF *font);
  virtual ~CGUIFont();

  CStdString& GetFontName();

  void DrawText( float x, float y, DWORD color, DWORD shadowColor,
                 const std::vector<DWORD> &text, DWORD alignment, float maxPixelWidth)
  {
    std::vector<DWORD> colors;
    colors.push_back(color);
    DrawText(x, y, colors, shadowColor, text, alignment, maxPixelWidth);
  };

  void DrawText( float x, float y, const std::vector<DWORD> &colors, DWORD shadowColor,
                 const std::vector<DWORD> &text, DWORD alignment, float maxPixelWidth);

  void DrawScrollingText( float x, float y, const std::vector<DWORD> &colors, DWORD shadowColor,
                 const std::vector<DWORD> &text, DWORD alignment, float maxPixelWidth, CScrollInfo &scrollInfo);

  float GetTextWidth( const std::vector<DWORD> &text );
  float GetCharWidth( DWORD ch );
  float GetTextHeight(int numLines) const;
  float GetLineHeight() const;

  void Begin();
  void End();

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
  float m_lineSpacing;
  CGUIFontTTF *m_font; // the font object has the size information

private:
  bool ClippedRegionIsEmpty(float x, float y, float width, DWORD alignment) const;
};

#endif
