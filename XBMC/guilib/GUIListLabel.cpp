#include "include.h"
#include "GUIListLabel.h"
#include "../xbmc/utils/CharsetConverter.h"

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
  if (m_scrolling && m_renderRect.x2 < m_textWidth + m_renderRect.x1)
    m_textLayout.RenderScrolling(m_renderRect.x1, m_renderRect.y1, m_label.angle, color, m_label.shadowColor, 0, m_renderRect.Width(), m_scrollInfo);
  else
    m_textLayout.Render(m_renderRect.x1, m_renderRect.y1, m_label.angle, color, m_label.shadowColor, 0, m_renderRect.Width());
  CGUIControl::Render();
}

void CGUIListLabel::SetLabel(const CStdString &label)
{
  if (m_textLayout.Update(label))
  { // needed an update - reset scrolling
    m_scrollInfo.Reset();
  }
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
