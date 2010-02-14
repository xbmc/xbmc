/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "GUILabel.h"
#include "utils/CharsetConverter.h"
#include <limits>

CGUILabel::CGUILabel(float posX, float posY, float width, float height, const CLabelInfo& labelInfo, CGUILabel::OVER_FLOW overflow, int scrollSpeed)
    : m_textLayout(labelInfo.font, overflow == OVER_FLOW_WRAP, height)
    , m_scrollInfo(50, 0, scrollSpeed)
    , m_maxRect(posX, posY, posX + width, posY + height)
{
  m_selected = false;
  m_overflowType = overflow;
  m_scrolling = (overflow == OVER_FLOW_SCROLL);
  m_label = labelInfo;
  m_invalid = true;
}

CGUILabel::~CGUILabel(void)
{
}

void CGUILabel::SetScrolling(bool scrolling, int scrollSpeed)
{
  m_scrolling = scrolling;
  if (scrollSpeed)
    m_scrollInfo.SetSpeed(scrollSpeed);
  if (!m_scrolling)
    m_scrollInfo.Reset();
}

void CGUILabel::SetColor(CGUILabel::COLOR color)
{
  m_color = color;
}

color_t CGUILabel::GetColor() const
{
  switch (m_color)
  {
    case COLOR_SELECTED:
      return m_label.selectedColor;
    case COLOR_DISABLED:
      return m_label.disabledColor;
    case COLOR_FOCUSED:
      return m_label.focusedColor ? m_label.focusedColor : m_label.textColor;
    default:
      break;
  }
  return m_label.textColor;
}

void CGUILabel::Render()
{
  color_t color = GetColor();
  bool renderSolid = (m_color == COLOR_DISABLED);
  bool overFlows = (m_renderRect.Width() + 0.5f < m_textLayout.GetTextWidth()); // 0.5f to deal with floating point rounding issues
  if (overFlows && m_scrolling && !renderSolid)
    m_textLayout.RenderScrolling(m_renderRect.x1, m_renderRect.y1, m_label.angle, color, m_label.shadowColor, 0, m_renderRect.Width(), m_scrollInfo);
  else
  {
    float posX = m_renderRect.x1;
    uint32_t align = 0;
    if (!overFlows)
    { // hack for right and centered multiline text, as GUITextLayout::Render() treats posX as the right hand
      // or center edge of the text (see GUIFontTTF::DrawTextInternal), and this has already been taken care of
      // in UpdateRenderRect(), but we wish to still pass the horizontal alignment info through (so that multiline text
      // is aligned correctly), so we must undo the UpdateRenderRect() changes for horizontal alignment.
      if (m_label.align & XBFONT_RIGHT)
        posX += m_renderRect.Width();
      else if (m_label.align & XBFONT_CENTER_X)
        posX += m_renderRect.Width() * 0.5f;
      align = m_label.align & ~XBFONT_CENTER_Y;  // ignore vertical alignment
    }
    m_textLayout.Render(posX, m_renderRect.y1, m_label.angle, color, m_label.shadowColor, align, m_renderRect.Width(), renderSolid);
  }
}

void CGUILabel::SetInvalid()
{
  m_invalid = true;
}

void CGUILabel::UpdateColors()
{
  m_label.UpdateColors();
}

void CGUILabel::SetMaxRect(float x, float y, float w, float h)
{
  m_maxRect.SetRect(x + m_label.offsetX, y + m_label.offsetY, x + w - m_label.offsetX, y + h - m_label.offsetY);
  UpdateRenderRect();
}

void CGUILabel::SetAlign(uint32_t align)
{
  m_label.align = align;
  UpdateRenderRect();
}

void CGUILabel::SetText(const CStdString &label)
{
  if (m_textLayout.Update(label, m_maxRect.Width(), m_invalid))
  { // needed an update - reset scrolling and update our text layout
    m_scrollInfo.Reset();
    UpdateRenderRect();
    m_invalid = false;
  }
}

void CGUILabel::SetTextW(const CStdStringW &label)
{
  m_textLayout.SetText(label);
  m_scrollInfo.Reset();
  UpdateRenderRect();
  m_invalid = false;
}

void CGUILabel::UpdateRenderRect()
{
  // recalculate our text layout
  float width, height;
  m_textLayout.GetTextExtent(width, height);
  width = std::min(width, m_maxRect.Width());
  if (m_label.align & XBFONT_CENTER_Y)
    m_renderRect.y1 = m_maxRect.y1 + (m_maxRect.Height() - height) * 0.5f;
  else
    m_renderRect.y1 = m_maxRect.y1;
  if (m_label.align & XBFONT_RIGHT)
    m_renderRect.x1 = m_maxRect.x2 - width;
  else if (m_label.align & XBFONT_CENTER_X)
    m_renderRect.x1 = m_maxRect.x1 + (m_maxRect.Width() - width) * 0.5f;
  else
    m_renderRect.x1 = m_maxRect.x1;
  m_renderRect.x2 = m_renderRect.x1 + width;
  m_renderRect.y2 = m_renderRect.y1 + height;
}
