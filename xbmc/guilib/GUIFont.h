/*
 *  Copyright (C) 2003-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIFont.h
\brief
*/

#include <assert.h>
#include <math.h>
#include <string>
#include <stdint.h>
#include <vector>

#include "utils/Color.h"

typedef uint32_t character_t;
typedef std::vector<character_t> vecText;

class CGUIFontTTFBase;

///
/// \defgroup kodi_gui_font_alignment Font alignment flags
/// \ingroup python_xbmcgui_control_radiobutton
/// @{
/// @brief Flags for alignment
///
/// Flags are used as bits to have several together, e.g. `XBFONT_LEFT | XBFONT_CENTER_Y`
///
#define XBFONT_LEFT       0x00000000 ///< Align X left
#define XBFONT_RIGHT      0x00000001 ///< Align X right
#define XBFONT_CENTER_X   0x00000002 ///< Align X center
#define XBFONT_CENTER_Y   0x00000004 ///< Align Y center
#define XBFONT_TRUNCATED  0x00000008 ///< Truncated text
#define XBFONT_JUSTIFIED  0x00000010 ///< Justify text
/// @}

// flags for font style. lower 16 bits are the unicode code
// points, 16-24 are color bits and 24-32 are style bits
#define FONT_STYLE_NORMAL       0
#define FONT_STYLE_BOLD         1
#define FONT_STYLE_ITALICS      2
#define FONT_STYLE_LIGHT        4
#define FONT_STYLE_UPPERCASE    8
#define FONT_STYLE_LOWERCASE    16
#define FONT_STYLE_CAPITALIZE   32
#define FONT_STYLE_MASK         0xFF

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
    // pixelPos is where we start the current letter, so is measured
    // to the left of the text rendering's left edge.  Thus, a negative
    // value will mean the text starts to the right
    pixelPos = -initialPos;
    // privates:
    m_averageFrameTime = 1000.f / fabs((float)defaultSpeed);
    m_lastFrameTime = 0;
    m_textWidth = 0;
    m_totalWidth = 0;
    m_widthValid = false;
    m_loopCount = 0;
  }
  float GetPixelsPerFrame();

  float pixelPos;
  float pixelSpeed;
  unsigned int waitTime;
  unsigned int initialWait;
  float initialPos;
  vecText suffix;
  mutable float m_textWidth;
  mutable float m_totalWidth;
  mutable bool m_widthValid;

  unsigned int m_loopCount;

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
  CGUIFont(const std::string& strFontName, uint32_t style, UTILS::Color textColor,
	   UTILS::Color shadowColor, float lineSpacing, float origHeight, CGUIFontTTFBase *font);
  virtual ~CGUIFont();

  std::string& GetFontName();

  void DrawText( float x, float y, UTILS::Color color, UTILS::Color shadowColor,
                 const vecText &text, uint32_t alignment, float maxPixelWidth)
  {
    std::vector<UTILS::Color> colors;
    colors.push_back(color);
    DrawText(x, y, colors, shadowColor, text, alignment, maxPixelWidth);
  };

  void DrawText( float x, float y, const std::vector<UTILS::Color> &colors, UTILS::Color shadowColor,
                 const vecText &text, uint32_t alignment, float maxPixelWidth);

  void DrawScrollingText( float x, float y, const std::vector<UTILS::Color> &colors, UTILS::Color shadowColor,
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
  UTILS::Color m_shadowColor;
  UTILS::Color m_textColor;
  float m_lineSpacing;
  float m_origHeight;
  CGUIFontTTFBase *m_font; // the font object has the size information

private:
  bool ClippedRegionIsEmpty(float x, float y, float width, uint32_t alignment) const;
};

