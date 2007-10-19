#include "include.h"
#include "GUITextBox.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/StringUtils.h"
#include "GUILabelControl.h"
#include "../xbmc/utils/GUIInfoManager.h"

#define CONTROL_LIST  0
#define CONTROL_UPDOWN 9998
CGUITextBox::CGUITextBox(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                         float spinWidth, float spinHeight,
                         const CImage& textureUp, const CImage& textureDown,
                         const CImage& textureUpFocus, const CImage& textureDownFocus,
                         const CLabelInfo& spinInfo, float spinX, float spinY,
                         const CLabelInfo& labelInfo)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_upDown(dwControlId, CONTROL_UPDOWN, 0, 0, spinWidth, spinHeight, textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, SPIN_CONTROL_TYPE_INT)
{
  m_upDown.SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
  m_offset = 0;
  m_label = labelInfo;
  m_itemsPerPage = 10;
  m_itemHeight = 10;
  m_spinPosX = spinX;
  m_spinPosY = spinY;
  m_upDown.SetShowRange(true); // show the range by default
  ControlType = GUICONTROL_TEXTBOX;
  m_pageControl = 0;
  m_singleInfo = 0;
}

CGUITextBox::~CGUITextBox(void)
{
}

void CGUITextBox::Render()
{
  CStdString renderLabel;
	if (m_singleInfo)
		renderLabel = g_infoManager.GetLabel(m_singleInfo, m_dwParentID);
	else
    renderLabel = g_infoManager.GetMultiInfo(m_multiInfo, m_dwParentID);

  // need to check the last one
  if (m_renderLabel != renderLabel)
  { // different, so reset to the top of the textbox and invalidate.  The page control will update itself below
    m_renderLabel = renderLabel;
    m_offset = 0;
    m_bInvalidated = true;
  }

  if (m_bInvalidated)
  { 
    // first correct any sizing we need to do
    float fWidth, fHeight;
    m_label.font->GetTextExtent( L"y", &fWidth, &fHeight);
    m_itemHeight = fHeight;
    float fTotalHeight = m_height;
    if (!m_pageControl)
      fTotalHeight -=  m_upDown.GetHeight() + 5;
    m_itemsPerPage = (unsigned int)(fTotalHeight / fHeight);

    // we have all the sizing correct so do any wordwrapping
    CStdStringW utf16Text;
    g_charsetConverter.utf8ToUTF16(m_renderLabel, utf16Text);
    CGUILabelControl::WrapText(utf16Text, m_label.font, m_width, m_lines);

    // disable all second label information
    m_lines2.clear();
    UpdatePageControl();
  }

  float posY = m_posY;

  if (m_label.font)
  {
    m_label.font->Begin();
    for (unsigned int i = 0; i < m_itemsPerPage; i++)
    {
      float posX = m_posX;
      if (i + m_offset < m_lines.size() )
      {
        // render item
        float maxWidth = m_width + 16;
        if (i + m_offset < m_lines2.size())
        {
          float fTextWidth, fTextHeight;
          m_label.font->GetTextExtent( m_lines2[i + m_offset].c_str(), &fTextWidth, &fTextHeight);
          maxWidth -= fTextWidth;

          m_label.font->DrawTextWidth(posX + maxWidth, posY + 2, m_label.textColor, m_label.shadowColor, m_lines2[i + m_offset].c_str(), fTextWidth);
        }
        m_label.font->DrawTextWidth(posX, posY + 2, m_label.textColor, m_label.shadowColor, m_lines[i + m_offset].c_str(), maxWidth);
        posY += m_itemHeight;
      }
    }
    m_label.font->End();
  }
  if (!m_pageControl)
  {
    m_upDown.SetPosition(m_posX + m_spinPosX, m_posY + m_spinPosY);
    m_upDown.Render();
  }
  CGUIControl::Render();
}

bool CGUITextBox::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    OnPageUp();
    return true;
    break;

  case ACTION_PAGE_DOWN:
    OnPageDown();
    return true;
    break;

  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
    return CGUIControl::OnAction(action);
    break;

  default:
    return m_upDown.OnAction(action);
  }
}

bool CGUITextBox::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId() == CONTROL_UPDOWN)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        if (m_upDown.GetValue() >= 1)
          m_offset = (m_upDown.GetValue() - 1) * m_itemsPerPage;
      }
    }
    if (message.GetMessage() == GUI_MSG_LABEL_BIND)
    { // send parameter is a link to a vector of CGUIListItem's
      vector<CGUIListItem> *items = (vector<CGUIListItem> *)message.GetLPVOID();
      if (items)
      {
        m_offset = 0;
        m_lines.clear();
        m_lines2.clear();
        for (unsigned int i = 0; i < items->size(); ++i)
        {
          CStdStringW utf16Label;
          CGUIListItem &item = items->at(i);
          g_charsetConverter.utf8ToUTF16(item.GetLabel(), utf16Label);
          m_lines.push_back(utf16Label);
          g_charsetConverter.utf8ToUTF16(item.GetLabel2(), utf16Label);
          m_lines2.push_back(utf16Label);
        }
        UpdatePageControl();
      }
    }
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_offset = 0;
      m_lines.clear();
      m_lines2.clear();
      m_upDown.SetRange(1, 1);
      m_upDown.SetValue(1);

      SetLabel( message.GetLabel() );
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_offset = 0;
      m_lines.clear();
      m_lines2.clear();
      m_upDown.SetRange(1, 1);
      m_upDown.SetValue(1);
      if (m_pageControl)
      {
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_lines.size());
        SendWindowMessage(msg);
      }
    }

    if (message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_upDown.SetFocus(true);
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS)
    {
      m_upDown.SetFocus(false);
    }

    if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl)
      { // update our page
        m_offset = message.GetParam1();
        return true;
      }
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUITextBox::PreAllocResources()
{
  if (!m_label.font) return;
  CGUIControl::PreAllocResources();
  m_upDown.PreAllocResources();
}

void CGUITextBox::AllocResources()
{
  if (!m_label.font) return;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();

  SetHeight(m_height);
}

void CGUITextBox::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
}

void CGUITextBox::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_upDown.DynamicResourceAlloc(bOnOff);
}

void CGUITextBox::OnRight()
{
  if (!m_pageControl && !m_upDown.IsFocusedOnUp())
    m_upDown.OnRight();
  else
    CGUIControl::OnRight();
}

void CGUITextBox::OnLeft()
{
  if (!m_pageControl && m_upDown.IsFocusedOnUp())
    m_upDown.OnLeft();
  else
    CGUIControl::OnLeft();
}

void CGUITextBox::OnPageUp()
{
  int iPage = m_upDown.GetValue();
  if (iPage > 1)
  {
    iPage--;
    m_upDown.SetValue(iPage);
    m_offset = (m_upDown.GetValue() - 1) * m_itemsPerPage;
  }
  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, m_offset);
    SendWindowMessage(msg);
  }
}

void CGUITextBox::OnPageDown()
{
  int iPages = m_lines.size() / m_itemsPerPage;
  if (m_lines.size() % m_itemsPerPage) iPages++;

  int iPage = m_upDown.GetValue();
  if (iPage + 1 <= iPages)
  {
    iPage++;
    m_upDown.SetValue(iPage);
    m_offset = (m_upDown.GetValue() - 1) * m_itemsPerPage;
  }
  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, m_offset);
    SendWindowMessage(msg);
  }
}

void CGUITextBox::SetLabel(const string &strText)
{
  g_infoManager.ParseLabel(strText, m_multiInfo);
  m_bInvalidated = true;
}

void CGUITextBox::UpdatePageControl()
{
  // and update our page control
  int iPages = m_lines.size() / m_itemsPerPage;
  if (m_lines.size() % m_itemsPerPage || !iPages) iPages++;
  m_upDown.SetRange(1, iPages);
  m_upDown.SetValue(1);
  if (m_pageControl)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_itemsPerPage, m_lines.size());
    SendWindowMessage(msg);
  }
}

bool CGUITextBox::HitTest(const CPoint &point) const
{
  if (m_upDown.HitTest(point)) return true;
  return CGUIControl::HitTest(point);
}

bool CGUITextBox::CanFocus() const
{
  if (m_pageControl) return false;
  return CGUIControl::CanFocus();
}

bool CGUITextBox::OnMouseOver(const CPoint &point)
{
  if (m_upDown.HitTest(point))
    m_upDown.OnMouseOver(point);
  return CGUIControl::OnMouseOver(point);
}

bool CGUITextBox::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  if (m_upDown.HitTest(point))
    return m_upDown.OnMouseClick(dwButton, point);
  return false;
}

bool CGUITextBox::OnMouseWheel(char wheel, const CPoint &point)
{
  if (m_upDown.HitTest(point))
  {
    return m_upDown.OnMouseWheel(wheel, point);
  }
  else
  { // increase or decrease our offset by the appropriate amount.
    m_offset -= wheel;
    // check that we are within the correct bounds.
    if (m_offset + m_itemsPerPage > (int)m_lines.size())
      m_offset = (m_lines.size() >= m_itemsPerPage) ? m_lines.size() - m_itemsPerPage : 0;
    // update the page control...
    int iPage = m_offset / m_itemsPerPage + 1;
    // last page??
    if (m_offset + m_itemsPerPage == m_lines.size())
      iPage = m_upDown.GetMaximum();
    m_upDown.SetValue(iPage);
  }
  return true;
}

void CGUITextBox::SetPosition(float posX, float posY)
{
  // offset our spin control by the appropriate amount
  float spinOffsetX = m_upDown.GetXPosition() - GetXPosition();
  float spinOffsetY = m_upDown.GetYPosition() - GetYPosition();
  CGUIControl::SetPosition(posX, posY);
  m_upDown.SetPosition(GetXPosition() + spinOffsetX, GetYPosition() + spinOffsetY);
}

void CGUITextBox::SetWidth(float width)
{
  float spinOffsetX = m_upDown.GetXPosition() - GetXPosition() - GetWidth();
  CGUIControl::SetWidth(width);
  m_upDown.SetPosition(GetXPosition() + GetWidth() + spinOffsetX, m_upDown.GetYPosition());
}

void CGUITextBox::SetHeight(float height)
{
  float spinOffsetY = m_upDown.GetYPosition() - GetYPosition() - GetHeight();
  CGUIControl::SetHeight(height);
  m_upDown.SetPosition(m_upDown.GetXPosition(), GetYPosition() + GetHeight() + spinOffsetY);
}

void CGUITextBox::SetPulseOnSelect(bool pulse)
{
  m_upDown.SetPulseOnSelect(pulse);
  CGUIControl::SetPulseOnSelect(pulse);
}

void CGUITextBox::SetNavigation(DWORD up, DWORD down, DWORD left, DWORD right)
{
  CGUIControl::SetNavigation(up, down, left, right);
  m_upDown.SetNavigation(up, down, left, right);
}

void CGUITextBox::SetPageControl(DWORD pageControl)
{
  m_pageControl = pageControl;
}

void CGUITextBox::SetInfo(int singleInfo)
{
  m_singleInfo = singleInfo;
}

void CGUITextBox::SetColorDiffuse(D3DCOLOR color)
{
  CGUIControl::SetColorDiffuse(color);
  m_upDown.SetColorDiffuse(color);
}