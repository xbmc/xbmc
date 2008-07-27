/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "include.h"
#include "GUIFont.h"
#include "GraphicContext.h"

#include "utils/SingleLock.h"
#include <math.h>


#ifndef _LINUX
namespace MathUtils {
  int round_int (double x);
}
#define ROUND(x) (float)(MathUtils::round_int(x))
#else
#define ROUND roundf
#endif

float CScrollInfo::GetPixelsPerFrame()
{
  static const float alphaEMA = 0.01f;

  if (0 == pixelSpeed)
    return 0; // not scrolling
  DWORD currentTime = timeGetTime();
  float delta = m_lastFrameTime ? (float)(currentTime - m_lastFrameTime) : m_averageFrameTime;
  m_lastFrameTime = currentTime;
  // do an exponential moving average of the frame time
  m_averageFrameTime = m_averageFrameTime + (delta - m_averageFrameTime) * alphaEMA;
  // and multiply by pixel speed (per ms) to get number of pixels to move this frame
  return pixelSpeed * m_averageFrameTime;
}

CGUIFont::CGUIFont(const CStdString& strFontName, DWORD style, DWORD textColor, DWORD shadowColor, float lineSpacing, CGUIFontTTF *font)
{
  m_strFontName = strFontName;
  m_style = style & 3;
  m_textColor = textColor;
  m_shadowColor = shadowColor;
  m_lineSpacing = lineSpacing;
  m_font = font;
  
  if (m_font)
    m_font->AddReference();
}

CGUIFont::~CGUIFont()
{
  if (m_font)
    m_font->RemoveReference();
}

CStdString& CGUIFont::GetFontName()
{
  return m_strFontName;
}

void CGUIFont::DrawText( float x, float y, const std::vector<DWORD> &colors, DWORD shadowColor,
                const std::vector<DWORD> &text, DWORD alignment, float maxPixelWidth)
{
  if (!m_font) return;

  bool clip = maxPixelWidth > 0;
  if (clip && ClippedRegionIsEmpty(x, y, maxPixelWidth, alignment))
    return;
      
  maxPixelWidth = ROUND(maxPixelWidth / g_graphicsContext.GetGUIScaleX());
  std::vector<DWORD> renderColors;
  for (unsigned int i = 0; i < colors.size(); i++)
    renderColors.push_back(g_graphicsContext.MergeAlpha(colors[i] ? colors[i] : m_textColor));
  if (!shadowColor) shadowColor = m_shadowColor;
  if (shadowColor)
    m_font->DrawTextInternal(x + 1, y + 1, g_graphicsContext.MergeAlpha(shadowColor), text, alignment, maxPixelWidth);
  m_font->DrawTextInternal( x, y, renderColors, text, alignment, maxPixelWidth );

  if (clip)
    g_graphicsContext.RestoreClipRegion();
}

void CGUIFont::DrawScrollingText(float x, float y, const std::vector<DWORD> &colors, DWORD shadowColor,
                const std::vector<DWORD> &text, DWORD alignment, float maxWidth, CScrollInfo &scrollInfo)
{
  if (!m_font) return;
  if (!shadowColor) shadowColor = m_shadowColor;

  float spaceWidth = GetCharWidth(L' ');
  // max chars on screen + extra margin chars
  std::vector<DWORD>::size_type maxChars =
    std::min<std::vector<DWORD>::size_type>(
      (text.size() + (std::vector<DWORD>::size_type)scrollInfo.suffix.size()),
      (std::vector<DWORD>::size_type)((maxWidth * 1.05f) / spaceWidth));

  if (!text.size() || ClippedRegionIsEmpty(x, y, maxWidth, alignment))
    return; // nothing to render

  maxWidth = ROUND(maxWidth / g_graphicsContext.GetGUIScaleX());

  // draw at our scroll position
  // we handle the scrolling as follows:
  //   We scroll on a per-pixel basis up until we have scrolled the first character outside
  //   of our viewport, whereby we cycle the string around, and reset the scroll position.
  //
  //   pixelPos is the amount in pixels to move the string by.
  //   characterPos is the amount in characters to rotate the string by.
  //
  float offset = 0;
  if (!scrollInfo.waitTime)
  {
    // move along by the appropriate scroll amount
    float scrollAmount = fabs(scrollInfo.GetPixelsPerFrame() * g_graphicsContext.GetGUIScaleX());

    if (scrollInfo.pixelSpeed > 0)
    {
      // we want to move scrollAmount, grab the next character
      float charWidth = GetCharWidth(scrollInfo.GetCurrentChar(text));
      if (scrollInfo.pixelPos + scrollAmount < charWidth)
        scrollInfo.pixelPos += scrollAmount;  // within the current character
      else
      { // past the current character, decrement scrollAmount by the charWidth and move to the next character
        while (scrollInfo.pixelPos + scrollAmount > charWidth)
        {
          scrollAmount -= (charWidth - scrollInfo.pixelPos);
          scrollInfo.pixelPos = 0;
          scrollInfo.characterPos++;
          if (scrollInfo.characterPos >= text.size() + scrollInfo.suffix.size())
          {
            scrollInfo.Reset();
            break;
          }
          charWidth = GetCharWidth(scrollInfo.GetCurrentChar(text));
        }
      }
      offset = scrollInfo.pixelPos;
    }
    else
    { // scrolling backwards
      // we want to move scrollAmount, grab the next character
      float charWidth = GetCharWidth(scrollInfo.GetCurrentChar(text));
      if (scrollInfo.pixelPos + scrollAmount < charWidth)
        scrollInfo.pixelPos += scrollAmount;  // within the current character
      else
      { // past the current character, decrement scrollAmount by the charWidth and move to the next character
        while (scrollInfo.pixelPos + scrollAmount > charWidth)
        {
          scrollAmount -= (charWidth - scrollInfo.pixelPos);
          scrollInfo.pixelPos = 0;
          if (scrollInfo.characterPos == 0)
          {
            scrollInfo.Reset();
            scrollInfo.characterPos = text.size() + scrollInfo.suffix.size() - 1;
            break;
          }
          scrollInfo.characterPos--;
          charWidth = GetCharWidth(scrollInfo.GetCurrentChar(text));
        }
      }
      offset = charWidth - scrollInfo.pixelPos;
    }
  }
  else
    scrollInfo.waitTime--;

  // Now rotate our string as needed, only take a slightly larger then visible part of the text.
  unsigned int pos = scrollInfo.characterPos;
  std::vector<DWORD> renderText;
  renderText.reserve(maxChars);
  for (std::vector<DWORD>::size_type i = 0; i < maxChars; i++)
  {
    if (pos >= text.size() + scrollInfo.suffix.size())
      pos = 0;
    if (pos < text.size())
      renderText.push_back(text[pos]);
    else
      renderText.push_back(scrollInfo.suffix[pos - text.size()]);
    pos++;
  }

  std::vector<DWORD> renderColors;
  for (unsigned int i = 0; i < colors.size(); i++)
    renderColors.push_back(g_graphicsContext.MergeAlpha(colors[i] ? colors[i] : m_textColor));

  if (shadowColor)
    m_font->DrawTextInternal(x - offset + 1, y + 1, g_graphicsContext.MergeAlpha(shadowColor), renderText, alignment, maxWidth + scrollInfo.pixelPos + m_font->GetLineHeight(2.0f));

  m_font->DrawTextInternal(x - offset, y, renderColors, renderText, alignment, maxWidth + scrollInfo.pixelPos + m_font->GetLineHeight(2.0f));

  g_graphicsContext.RestoreClipRegion();
}

// remaps unsupported font glpyhs to other suitable ones
SHORT CGUIFont::RemapGlyph(SHORT letter)
{
  if (letter == 0x2019 || letter == 0x2018) return 0x0027;  // single quotes
  else if (letter == 0x201c || letter == 0x201d) return 0x0022;
  return 0; // no decent character map
}

bool CGUIFont::ClippedRegionIsEmpty(float x, float y, float width, DWORD alignment) const
{
  if (alignment & XBFONT_CENTER_X)
    x -= width * 0.5f;
  else if (alignment & XBFONT_RIGHT)
    x -= width;
  if (alignment & XBFONT_CENTER_Y)
    y -= m_font->GetLineHeight(m_lineSpacing);

  return !g_graphicsContext.SetClipRegion(x, y, width, m_font->GetLineHeight(2.0f));
}

