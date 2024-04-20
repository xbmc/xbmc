/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFont.h"

#include "GUIFontTTF.h"
#include "utils/CharsetConverter.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "windowing/GraphicContext.h"

#include <mutex>

#define ROUND(x) (float)(MathUtils::round_int(x))

CScrollInfo::CScrollInfo(unsigned int wait /* = 50 */,
                         float pos /* = 0 */,
                         int speed /* = defaultSpeed */,
                         const std::string& scrollSuffix /* = " | " */)
  : m_initialWait(wait), m_initialPos(pos)

{
  SetSpeed(speed ? speed : defaultSpeed);
  std::wstring wsuffix;
  g_charsetConverter.utf8ToW(scrollSuffix, wsuffix);
  m_suffix.clear();
  m_suffix.reserve(wsuffix.size());
  m_suffix = vecText(wsuffix.begin(), wsuffix.end());
  Reset();
}

float CScrollInfo::GetPixelsPerFrame()
{
  static const float alphaEMA = 0.05f;

  if (0 == m_pixelSpeed)
    return 0; // not scrolling

  unsigned int currentTime = CTimeUtils::GetFrameTime();
  float delta =
      m_lastFrameTime ? static_cast<float>(currentTime - m_lastFrameTime) : m_averageFrameTime;
  if (delta > 100)
    delta = 100; // assume a minimum of 10 fps
  m_lastFrameTime = currentTime;
  // do an exponential moving average of the frame time
  if (delta)
    m_averageFrameTime = m_averageFrameTime + (delta - m_averageFrameTime) * alphaEMA;
  // and multiply by pixel speed (per ms) to get number of pixels to move this frame
  return m_pixelSpeed * m_averageFrameTime;
}

CGUIFont::CGUIFont(const std::string& strFontName,
                   uint32_t style,
                   UTILS::COLOR::Color textColor,
                   UTILS::COLOR::Color shadowColor,
                   float lineSpacing,
                   float origHeight,
                   CGUIFontTTF* font)
  : m_strFontName(strFontName)
{
  m_style = style & FONT_STYLE_MASK;
  m_textColor = textColor;
  m_shadowColor = shadowColor;
  m_lineSpacing = lineSpacing;
  m_origHeight = origHeight;
  m_font = font;

  if (m_font)
    m_font->AddReference();
}

CGUIFont::~CGUIFont()
{
  if (m_font)
    m_font->RemoveReference();
}

std::string& CGUIFont::GetFontName()
{
  return m_strFontName;
}

void CGUIFont::DrawText(float x,
                        float y,
                        const std::vector<UTILS::COLOR::Color>& colors,
                        UTILS::COLOR::Color shadowColor,
                        const vecText& text,
                        uint32_t alignment,
                        float maxPixelWidth)
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!m_font || !winSystem)
    return;

  CGraphicContext& context = winSystem->GetGfxContext();

  bool clip = maxPixelWidth > 0;
  if (clip && ClippedRegionIsEmpty(context, x, y, maxPixelWidth, alignment))
    return;

  maxPixelWidth = ROUND(static_cast<double>(maxPixelWidth / context.GetGUIScaleX()));
  std::vector<UTILS::COLOR::Color> renderColors;
  renderColors.reserve(colors.size());
  for (const auto& color : colors)
    renderColors.emplace_back(context.MergeColor(color ? color : m_textColor));
  if (!shadowColor)
    shadowColor = m_shadowColor;
  if (shadowColor)
  {
    shadowColor = context.MergeColor(shadowColor);
    std::vector<UTILS::COLOR::Color> shadowColors;
    shadowColors.reserve(renderColors.size());
    for (const auto& renderColor : renderColors)
      shadowColors.emplace_back((renderColor & 0xff000000) != 0 ? shadowColor : 0);
    m_font->DrawTextInternal(context, x + 1, y + 1, shadowColors, text, alignment, maxPixelWidth,
                             false);
  }
  m_font->DrawTextInternal(context, x, y, renderColors, text, alignment, maxPixelWidth, false);

  if (clip)
    context.RestoreClipRegion();
}

bool CGUIFont::UpdateScrollInfo(const vecText& text, CScrollInfo& scrollInfo)
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return false;

  // draw at our scroll position
  // we handle the scrolling as follows:
  //   We scroll on a per-pixel basis (eschewing the use of character indices
  //   which were also in use previously). The complete string, including suffix,
  //   is plotted to achieve the desired effect - normally just the one time, but
  //   if there is a wrap point within the viewport then it will be plotted twice.
  //   If the string is smaller than the viewport, then it may be plotted even
  //   more times than that.
  //
  if (scrollInfo.m_waitTime)
  {
    scrollInfo.m_waitTime--;
    return true;
  }

  if (text.empty())
    return false;

  CScrollInfo old(scrollInfo);

  // move along by the appropriate scroll amount
  float scrollAmount =
      fabs(scrollInfo.GetPixelsPerFrame() * winSystem->GetGfxContext().GetGUIScaleX());

  if (!scrollInfo.m_widthValid)
  {
    /* Calculate the pixel width of the complete string */
    scrollInfo.m_textWidth = GetTextWidth(text);
    scrollInfo.m_totalWidth = scrollInfo.m_textWidth + GetTextWidth(scrollInfo.m_suffix);
    scrollInfo.m_widthValid = true;
  }
  scrollInfo.m_pixelPos += scrollAmount;
  assert(scrollInfo.m_totalWidth != 0);
  while (scrollInfo.m_pixelPos >= scrollInfo.m_totalWidth)
    scrollInfo.m_pixelPos -= scrollInfo.m_totalWidth;

  if (scrollInfo.m_pixelPos < old.m_pixelPos)
    ++scrollInfo.m_loopCount;

  if (scrollInfo.m_pixelPos != old.m_pixelPos)
    return true;

  return false;
}

void CGUIFont::DrawScrollingText(float x,
                                 float y,
                                 const std::vector<UTILS::COLOR::Color>& colors,
                                 UTILS::COLOR::Color shadowColor,
                                 const vecText& text,
                                 uint32_t alignment,
                                 float maxWidth,
                                 const CScrollInfo& scrollInfo)
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!m_font || !winSystem)
    return;

  CGraphicContext& context = winSystem->GetGfxContext();

  if (!shadowColor)
    shadowColor = m_shadowColor;

  if (!text.size() || ClippedRegionIsEmpty(context, x, y, maxWidth, alignment))
    return; // nothing to render

  if (!scrollInfo.m_widthValid)
  {
    /* Calculate the pixel width of the complete string */
    scrollInfo.m_textWidth = GetTextWidth(text);
    scrollInfo.m_totalWidth = scrollInfo.m_textWidth + GetTextWidth(scrollInfo.m_suffix);
    scrollInfo.m_widthValid = true;
  }

  assert(scrollInfo.m_totalWidth != 0);

  float textPixelWidth =
      ROUND(static_cast<double>(scrollInfo.m_textWidth / context.GetGUIScaleX()));
  float suffixPixelWidth = ROUND(static_cast<double>(
      (scrollInfo.m_totalWidth - scrollInfo.m_textWidth) / context.GetGUIScaleX()));

  float offset;
  if (scrollInfo.m_pixelSpeed >= 0)
    offset = scrollInfo.m_pixelPos;
  else
    offset = scrollInfo.m_totalWidth - scrollInfo.m_pixelPos;

  std::vector<UTILS::COLOR::Color> renderColors;
  renderColors.reserve(colors.size());
  for (const auto& color : colors)
    renderColors.emplace_back(context.MergeColor(color ? color : m_textColor));

  bool scroll = !scrollInfo.m_waitTime && scrollInfo.m_pixelSpeed;
  if (shadowColor)
  {
    shadowColor = context.MergeColor(shadowColor);
    std::vector<UTILS::COLOR::Color> shadowColors;
    shadowColors.reserve(renderColors.size());
    for (const auto& renderColor : renderColors)
      shadowColors.emplace_back((renderColor & 0xff000000) != 0 ? shadowColor : 0);
    for (float dx = -offset; dx < maxWidth; dx += scrollInfo.m_totalWidth)
    {
      m_font->DrawTextInternal(context, x + 1, y + 1, shadowColors, text, alignment, textPixelWidth,
                               scroll, dx);
      m_font->DrawTextInternal(context, x + scrollInfo.m_textWidth + 1, y + 1, shadowColors,
                               scrollInfo.m_suffix, alignment, suffixPixelWidth, scroll, dx);
    }
  }
  for (float dx = -offset; dx < maxWidth; dx += scrollInfo.m_totalWidth)
  {
    m_font->DrawTextInternal(context, x, y, renderColors, text, alignment, textPixelWidth, scroll,
                             dx);
    m_font->DrawTextInternal(context, x + scrollInfo.m_textWidth, y, renderColors,
                             scrollInfo.m_suffix, alignment, suffixPixelWidth, scroll, dx);
  }

  context.RestoreClipRegion();
}

bool CGUIFont::ClippedRegionIsEmpty(
    CGraphicContext& context, float x, float y, float width, uint32_t alignment) const
{
  if (alignment & XBFONT_CENTER_X)
    x -= width * 0.5f;
  if (alignment & XBFONT_CENTER_Y)
    y -= m_font->GetLineHeight(m_lineSpacing);

  return !context.SetClipRegion(x, y, width, m_font->GetTextHeight(1, 2) * context.GetGUIScaleY());
}

float CGUIFont::GetTextWidth(const vecText& text)
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!m_font || !winSystem)
    return 0;

  CGraphicContext& context = winSystem->GetGfxContext();

  std::unique_lock<CCriticalSection> lock(context);
  return m_font->GetTextWidthInternal(text) * context.GetGUIScaleX();
}

float CGUIFont::GetCharWidth(character_t ch)
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!m_font || !winSystem)
    return 0;

  CGraphicContext& context = winSystem->GetGfxContext();

  std::unique_lock<CCriticalSection> lock(context);
  return m_font->GetCharWidthInternal(ch) * context.GetGUIScaleX();
}

float CGUIFont::GetTextHeight(int numLines) const
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!m_font || !winSystem)
    return 0;

  return m_font->GetTextHeight(m_lineSpacing, numLines) * winSystem->GetGfxContext().GetGUIScaleY();
}

float CGUIFont::GetTextBaseLine() const
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!m_font || !winSystem)
    return 0;

  return m_font->GetTextBaseLine() * winSystem->GetGfxContext().GetGUIScaleY();
}

float CGUIFont::GetLineHeight() const
{
  CWinSystemBase* const winSystem = CServiceBroker::GetWinSystem();
  if (!m_font || !winSystem)
    return 0;

  return m_font->GetLineHeight(m_lineSpacing) * winSystem->GetGfxContext().GetGUIScaleY();
}

float CGUIFont::GetScaleFactor() const
{
  if (!m_font)
    return 1.0f;

  return m_font->GetFontHeight() / m_origHeight;
}

void CGUIFont::Begin()
{
  if (!m_font)
    return;

  m_font->Begin();
}

void CGUIFont::End()
{
  if (!m_font)
    return;

  m_font->End();
}

void CGUIFont::SetFont(CGUIFontTTF* font)
{
  if (m_font == font)
    return; // no need to update the font if we already have it

  if (m_font)
    m_font->RemoveReference();

  m_font = font;
  if (m_font)
    m_font->AddReference();
}
