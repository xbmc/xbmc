#include "include.h"
#include "GUIListLabel.h"
#include "../xbmc/utils/CharsetConverter.h"
#include <limits>

CGUIListLabel::CGUIListLabel(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool alwaysScroll)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_textLayout(labelInfo.font, false)
{
  m_selected = false;
  m_scrolling = m_alwaysScroll = alwaysScroll;
  m_label = labelInfo;
}

CGUIListLabel::~CGUIListLabel(void)
{
}

void CGUIListLabel::SetScrolling(bool scrolling)
{
  m_scrolling = m_alwaysScroll ? true : scrolling;
  if (!m_scrolling)
    m_scrollInfo.Reset();
}

void CGUIListLabel::SetSelected(bool selected)
{
  m_selected = selected;
}

void CGUIListLabel::Render()
{
  DWORD color = m_selected ? m_label.selectedColor : m_label.textColor;
  bool needsToScroll(m_renderRect.Width() + 0.5f < m_textWidth); // 0.5f to deal with floating point rounding issues
  if (m_scrolling && needsToScroll)
    m_textLayout.RenderScrolling(m_renderRect.x1, m_renderRect.y1, m_label.angle, color, m_label.shadowColor, 0, m_renderRect.Width(), m_scrollInfo);
  else
  {
    float posX = m_renderRect.x1;
    DWORD align = 0;
    if (!needsToScroll)
    { // hack for right and centered multiline text, as GUITextLayout::Render() treats posX as the right hand
      // or center edge of the text (see GUIFontTTF::DrawTextInternal), and this has already been taken care of
      // in SetLabel(), but we wish to still pass the horizontal alignment info through (so that multiline text
      // is aligned correctly), so we must undo the SetLabel() changes for horizontal alignment.
      if (m_label.align & XBFONT_RIGHT)
        posX += m_renderRect.Width();
      else if (m_label.align & XBFONT_CENTER_X)
        posX += m_renderRect.Width() * 0.5f;
      align = m_label.align & ~XBFONT_CENTER_Y;  // ignore vertical alignment
    }
    m_textLayout.Render(posX, m_renderRect.y1, m_label.angle, color, m_label.shadowColor, align, m_renderRect.Width());
  }
  CGUIControl::Render();
}

void CGUIListLabel::SetLabel(const CStdString &label)
{
  if (m_textLayout.Update(label))
  { // needed an update - reset scrolling
    m_scrollInfo.Reset();
    // recalculate our text layout
    float width, height;
    m_textLayout.GetTextExtent(m_textWidth, height);
    width = min(m_textWidth, m_width);
    if (m_label.align & XBFONT_CENTER_Y)
      m_renderRect.y1 = m_posY + (m_height - height) * 0.5f;
    else
      m_renderRect.y1 = m_posY;
    if (m_label.align & XBFONT_RIGHT)
      m_renderRect.x1 = m_posX - width;
    else if (m_label.align & XBFONT_CENTER_X)
      m_renderRect.x1 = m_posX - width * 0.5f;
    else
      m_renderRect.x1 = m_posX;
    m_renderRect.x2 = m_renderRect.x1 + width;
    m_renderRect.y2 = m_renderRect.y1 + height;
  }
}
