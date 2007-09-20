#include "include.h"
#include "GUIListLabel.h"
#include "../xbmc/utils/CharsetConverter.h"

CGUIListLabel::CGUIListLabel(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_selected = false;
  m_scrolling = false;
  m_label = labelInfo;
}

CGUIListLabel::~CGUIListLabel(void)
{
}

void CGUIListLabel::SetScrolling(bool scrolling)
{
  m_scrolling = scrolling;
  if (!scrolling)
    m_scrollInfo.Reset();
}

void CGUIListLabel::SetSelected(bool selected)
{
  m_selected = selected;
}

void CGUIListLabel::Render()
{
  if (m_label.font && !m_text.IsEmpty())
  {
    DWORD color = m_selected ? m_label.selectedColor : m_label.textColor;
    if (m_scrolling && m_renderRect.x2 < m_textWidth + m_renderRect.x1)
      m_label.font->DrawScrollingText(m_renderRect.x1, m_renderRect.y1, &color, 1,
                                  m_label.shadowColor, m_text, m_renderRect.Width(), m_scrollInfo);
    else
      m_label.font->DrawTextWidth(m_renderRect.x1, m_renderRect.y1, m_label.angle, color,
                                  m_label.shadowColor, m_text, m_renderRect.Width());
  }
  CGUIControl::Render();
}

void CGUIListLabel::SetLabel(const CStdString &label)
{
  CStdStringW newText;
  g_charsetConverter.utf8ToUTF16(label, newText);
  if (newText != m_text)
  { // changed label - reset scrolling
    m_scrollInfo.Reset();
    m_text = newText;
  }
  // and recalculate our text
  if (m_label.font)
  {
    float width, height;
    m_label.font->GetTextExtent(m_text, &m_textWidth, &height);
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
