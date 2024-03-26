/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUILabel.h"

#include <limits>

CGUILabel::CGUILabel(float posX,
                     float posY,
                     float width,
                     float height,
                     const CLabelInfo& labelInfo,
                     CGUILabel::OVER_FLOW overflow)
  : m_label(labelInfo),
    m_textLayout(labelInfo.font, overflow == OVER_FLOW_WRAP, height),
    m_scrolling(overflow == OVER_FLOW_SCROLL),
    m_overflowType(overflow),
    m_scrollInfo(50, 0, labelInfo.scrollSpeed, labelInfo.scrollSuffix),
    m_renderRect(),
    m_maxRect(posX, posY, posX + width, posY + height)
{
}

CGUILabel::CGUILabel(const CGUILabel& label)
  : m_label(label.m_label),
    m_textLayout(label.m_textLayout),
    m_scrolling(label.m_scrolling),
    m_overflowType(label.m_overflowType),
    m_scrollInfo(label.m_scrollInfo),
    m_renderRect(label.m_renderRect),
    m_maxRect(label.m_maxRect),
    m_invalid(label.m_invalid),
    m_color(label.m_color),
    m_maxScrollLoops(label.m_maxScrollLoops)
{
}

bool CGUILabel::SetScrolling(bool scrolling)
{
  bool changed = m_scrolling != scrolling;

  m_scrolling = scrolling;
  if (changed)
    m_scrollInfo.Reset();

  return changed;
}

bool CGUILabel::SetOverflow(OVER_FLOW overflow)
{
  bool changed = m_overflowType != overflow;

  m_overflowType = overflow;

  return changed;
}

bool CGUILabel::SetColor(CGUILabel::COLOR color)
{
  bool changed = m_color != color;

  m_color = color;

  return changed;
}

UTILS::COLOR::Color CGUILabel::GetColor() const
{
  switch (m_color)
  {
    case COLOR_SELECTED:
      return m_label.selectedColor;
    case COLOR_DISABLED:
      return m_label.disabledColor;
    case COLOR_FOCUSED:
      return m_label.focusedColor ? m_label.focusedColor : m_label.textColor;
    case COLOR_INVALID:
      return m_label.invalidColor ? m_label.invalidColor : m_label.textColor;
    default:
      break;
  }
  return m_label.textColor;
}

bool CGUILabel::Process(unsigned int currentTime)
{
  //! @todo Add the correct processing

  bool overFlows = (m_renderRect.Width() + 0.5f < m_textLayout.GetTextWidth()); // 0.5f to deal with floating point rounding issues
  bool renderSolid = (m_color == COLOR_DISABLED);

  if (overFlows && m_scrolling && !renderSolid)
  {
    if (m_maxScrollLoops < m_scrollInfo.m_loopCount)
      SetScrolling(false);
    else
      return m_textLayout.UpdateScrollinfo(m_scrollInfo);
  }

  return false;
}

void CGUILabel::Render()
{
  UTILS::COLOR::Color color = GetColor();
  bool renderSolid = (m_color == COLOR_DISABLED);
  bool overFlows = (m_renderRect.Width() + 0.5f < m_textLayout.GetTextWidth()); // 0.5f to deal with floating point rounding issues
  if (overFlows && m_scrolling && !renderSolid)
    m_textLayout.RenderScrolling(m_renderRect.x1, m_renderRect.y1, m_label.angle, color, m_label.shadowColor, 0, m_renderRect.Width(), m_scrollInfo);
  else
  {
    float posX = m_renderRect.x1;
    float posY = m_renderRect.y1;
    uint32_t align = 0;
    if (!overFlows)
    { // hack for centered multiline text, as GUITextLayout::Render() treats posX as
      // center edge of the text (see GUIFontTTF::DrawTextInternal), and this has already been taken care of
      // in UpdateRenderRect(), but we wish to still pass the horizontal alignment info through (so that multiline text
      // is aligned correctly), so we must undo the UpdateRenderRect() changes for horizontal alignment.
      if (m_label.align & XBFONT_CENTER_X)
        posX += m_renderRect.Width() * 0.5f;
      if (m_label.align & XBFONT_CENTER_Y) // need to pass a centered Y so that <angle> will rotate around the correct point.
        posY += m_renderRect.Height() * 0.5f;
      align = m_label.align;
    }
    else
    {
      if (m_overflowType == OVER_FLOW_TRUNCATE_LEFT)
        align |= XBFONT_TRUNCATED_LEFT;
      else
        align |= XBFONT_TRUNCATED;

      if (m_label.align & XBFONT_RIGHT)
        align |= XBFONT_RIGHT;

      if (m_label.align & XBFONT_CENTER_X)
      {
        posX += m_renderRect.Width() * 0.5f; // hack for centered multiline text, same of above
        align |= XBFONT_CENTER_X;
      }
    }
    m_textLayout.Render(posX, posY, m_label.angle, color, m_label.shadowColor, align, m_overflowType == OVER_FLOW_CLIP ? m_textLayout.GetTextWidth() : m_renderRect.Width(), renderSolid);
  }
}

void CGUILabel::SetInvalid()
{
  m_invalid = true;
}

bool CGUILabel::UpdateColors()
{
  return m_label.UpdateColors();
}

bool CGUILabel::SetMaxRect(float x, float y, float w, float h)
{
  CRect oldRect = m_maxRect;

  m_maxRect.SetRect(x, y, x + w, y + h);
  UpdateRenderRect();

  return oldRect != m_maxRect;
}

bool CGUILabel::SetAlign(uint32_t align)
{
  bool changed = m_label.align != align;

  m_label.align = align;
  UpdateRenderRect();

  return changed;
}

bool CGUILabel::SetStyledText(const vecText& text, const std::vector<UTILS::COLOR::Color>& colors)
{
  m_textLayout.UpdateStyled(text, colors, m_maxRect.Width());
  m_invalid = false;
  return true;
}

bool CGUILabel::SetText(const std::string &label)
{
  if (m_textLayout.Update(label, m_maxRect.Width(), m_invalid))
  { // needed an update - reset scrolling and update our text layout
    m_scrollInfo.Reset();
    UpdateRenderRect();
    m_invalid = false;
    return true;
  }
  else
    return false;
}

bool CGUILabel::SetTextW(const std::wstring &label)
{
  if (m_textLayout.UpdateW(label, m_maxRect.Width(), m_invalid))
  {
    m_scrollInfo.Reset();
    UpdateRenderRect();
    m_invalid = false;
    return true;
  }
  else
    return false;
}

void CGUILabel::UpdateRenderRect()
{
  // recalculate our text layout
  float width, height;
  m_textLayout.GetTextExtent(width, height);
  width = std::min(width, GetMaxWidth());
  if (m_label.align & XBFONT_CENTER_Y)
    m_renderRect.y1 = m_maxRect.y1 + (m_maxRect.Height() - height) * 0.5f;
  else
    m_renderRect.y1 = m_maxRect.y1 + m_label.offsetY;
  if (m_label.align & XBFONT_RIGHT)
    m_renderRect.x1 = m_maxRect.x2 - width - m_label.offsetX;
  else if (m_label.align & XBFONT_CENTER_X)
    m_renderRect.x1 = m_maxRect.x1 + (m_maxRect.Width() - width) * 0.5f;
  else
    m_renderRect.x1 = m_maxRect.x1 + m_label.offsetX;
  m_renderRect.x2 = m_renderRect.x1 + width;
  m_renderRect.y2 = m_renderRect.y1 + height;
}

float CGUILabel::GetMaxWidth() const
{
  if (m_label.width) return m_label.width;
  return m_maxRect.Width() - 2*m_label.offsetX;
}

bool CGUILabel::CheckAndCorrectOverlap(CGUILabel &label1, CGUILabel &label2)
{
  CRect rect(label1.m_renderRect);
  if (rect.Intersect(label2.m_renderRect).IsEmpty())
    return false; // nothing to do (though it could potentially encroach on the min_space requirement)

  // overlap vertically and horizontally - check alignment
  CGUILabel &left = label1.m_renderRect.x1 <= label2.m_renderRect.x1 ? label1 : label2;
  CGUILabel &right = label1.m_renderRect.x1 <= label2.m_renderRect.x1 ? label2 : label1;
  if ((left.m_label.align & 3) == 0 && right.m_label.align & XBFONT_RIGHT)
  {
    static const float min_space = 10;
    float chopPoint = (left.m_maxRect.x1 + left.GetMaxWidth() + right.m_maxRect.x2 - right.GetMaxWidth()) * 0.5f;
    // [1       [2...[2  1].|..........1]         2]
    // [1       [2.....[2   |      1]..1]         2]
    // [1       [2..........|.[2   1]..1]         2]
    if (right.m_renderRect.x1 > chopPoint)
      chopPoint = right.m_renderRect.x1 - min_space;
    else if (left.m_renderRect.x2 < chopPoint)
      chopPoint = left.m_renderRect.x2 + min_space;
    left.m_renderRect.x2 = chopPoint - min_space;
    right.m_renderRect.x1 = chopPoint + min_space;
    return true;
  }
  return false;
}
