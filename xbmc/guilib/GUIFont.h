/*!
\file GUIFont.h
\brief
*/

#ifndef CGUILIB_GUIFONT_H
#define CGUILIB_GUIFONT_H
#pragma once

/*
 *      Copyright (C) 2003-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <assert.h>
#include <math.h>
#include <string>
#include <stdint.h>
#include <vector>

typedef uint32_t character_t;
typedef uint32_t color_t;
typedef std::vector<character_t> vecText;
typedef std::vector<color_t> vecColors;

class CGUIFontTTFBase;

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
#define FONT_STYLE_UPPERCASE    4
#define FONT_STYLE_LOWERCASE    8
#define FONT_STYLE_MASK       0xF

class CScrollInfo
{
public:
  CScrollInfo(unsigned int wait = 50, float pos = 0, int speed = defaultSpeed, const std::string &scrollSuffix = " | ");

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
    m_averageFrameTime = 1000.f / fabs((float)defaultSpeed);
    m_lastFrameTime = 0;
  }
  uint32_t GetCurrentChar(const vecText &text) const
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
  std::wstring suffix;

  static const int defaultSpeed = 60;
private:
  float m_averageFrameTime;
  uint32_t m_lastFrameTime;
};

/*!
 \ingroup textures
 \brief
 */
class CGUIFont
{
public:
  CGUIFont(const std::string& strFontName, uint32_t style, color_t textColor,
	   color_t shadowColor, float lineSpacing, float origHeight, CGUIFontTTFBase *font);
  virtual ~CGUIFont();

  std::string& GetFontName();

  void DrawText( float x, float y, color_t color, color_t shadowColor,
                 const vecText &text, uint32_t alignment, float maxPixelWidth)
  {
    vecColors colors;
    colors.push_back(color);
    DrawText(x, y, colors, shadowColor, text, alignment, maxPixelWidth);
  };

  void DrawText( float x, float y, const vecColors &colors, color_t shadowColor,
                 const vecText &text, uint32_t alignment, float maxPixelWidth);

  void DrawScrollingText( float x, float y, const vecColors &colors, color_t shadowColor,
                 const vecText &text, uint32_t alignment, float maxPixelWidth, const CScrollInfo &scrollInfo);

  bool UpdateScrollInfo(const vecText &text, CScrollInfo &scrollInfo);

  float GetTextWidth( const vecText &text );
  float GetCharWidth( character_t ch );
  float GetTextHeight(int numLines) const;
  float GetTextBaseLine() const;
  float GetLineHeight() const;

  //! get font scale factor (rendered height / original height)
  float GetScaleFactor() const;

  void Begin();
  void End();

  uint32_t GetStyle() const { return m_style; };

  static wchar_t RemapGlyph(wchar_t letter);

  CGUIFontTTFBase* GetFont() const
  {
    return m_font;
  }

  void SetFont(CGUIFontTTFBase* font);

protected:
  std::string m_strFontName;
  uint32_t m_style;
  color_t m_shadowColor;
  color_t m_textColor;
  float m_lineSpacing;
  float m_origHeight;
  CGUIFontTTFBase *m_font; // the font object has the size information

private:
  bool ClippedRegionIsEmpty(float x, float y, float width, uint32_t alignment) const;
};

#endif
