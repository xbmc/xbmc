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
                         const CLabelInfo& labelInfo, int scrollTime)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , CGUITextLayout(labelInfo.font, true)
    , m_upDown(dwControlId, CONTROL_UPDOWN, 0, 0, spinWidth, spinHeight, textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, SPIN_CONTROL_TYPE_INT)
{
  m_upDown.SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
  m_offset = 0;
  m_scrollOffset = 0;
  m_itemsPerPage = 10;
  m_itemHeight = 10;
  m_spinPosX = spinX;
  m_spinPosY = spinY;
  m_upDown.SetShowRange(true); // show the range by default
  ControlType = GUICONTROL_TEXTBOX;
  m_pageControl = 0;
  m_singleInfo = 0;
  m_renderTime = 0;
  m_lastRenderTime = 0;
  m_scrollTime = scrollTime;
  m_autoScrollCondition = 0;
  m_autoScrollTime = 0;
  m_autoScrollDelay = 3000;
  m_autoScrollDelayTime = 0;
  m_autoScrollRepeatAnim = NULL;
  m_label = labelInfo;
  m_textColor = m_label.textColor;
}

CGUITextBox::~CGUITextBox(void)
{
  if (m_autoScrollRepeatAnim)
    delete m_autoScrollRepeatAnim;
  m_autoScrollRepeatAnim = NULL;
}

void CGUITextBox::DoRender(DWORD currentTime)
{
  m_renderTime = currentTime;

  // render the repeat anim as appropriate
  if (m_autoScrollRepeatAnim)
  {
    m_autoScrollRepeatAnim->Animate(m_renderTime, true);
    TransformMatrix matrix;
    m_autoScrollRepeatAnim->RenderAnimation(matrix);
    g_graphicsContext.AddTransform(matrix);
  }

  CGUIControl::DoRender(currentTime);
  // if not visible, we reset the autoscroll timer and positioning
  if (!IsVisible() && m_autoScrollTime)
  {
    ResetAutoScrolling();
    m_lastRenderTime = 0;
    m_offset = 0;
    m_scrollOffset = 0;
    m_scrollSpeed = 0;
  }
  if (m_autoScrollRepeatAnim)
    g_graphicsContext.RemoveTransform();
}

void CGUITextBox::Render()
{
  CStdString renderLabel;
	if (m_singleInfo)
		renderLabel = g_infoManager.GetLabel(m_singleInfo, m_dwParentID);
	else
    renderLabel = g_infoManager.GetMultiInfo(m_multiInfo, m_dwParentID);

  if (CGUITextLayout::Update(renderLabel, m_width))
  { // needed update, so reset to the top of the textbox and update our sizing/page control
    m_offset = 0;
    m_scrollOffset = 0;
    ResetAutoScrolling();

    m_itemHeight = m_font->GetTextHeight(1);   // note: GetLineHeight() is more correct, but seems a bit too much spacing
    float fTotalHeight = m_height;
    if (!m_pageControl)
      fTotalHeight -=  m_upDown.GetHeight() + 5;
    m_itemsPerPage = (unsigned int)(fTotalHeight / m_itemHeight);

    UpdatePageControl();
  }

  // update our auto-scrolling as necessary
  if (m_autoScrollTime && m_lines.size() > m_itemsPerPage)
  {
    if (!m_autoScrollCondition || g_infoManager.GetBool(m_autoScrollCondition, m_dwParentID))
    {
      if (m_lastRenderTime)
        m_autoScrollDelayTime += m_renderTime - m_lastRenderTime;
      if (m_autoScrollDelayTime > (unsigned int)m_autoScrollDelay && m_scrollSpeed == 0)
      { // delay is finished - start scrolling
        if (m_offset < (int)m_lines.size() - m_itemsPerPage)
          ScrollToOffset(m_offset + 1, true);
        else
        { // at the end, run a delay and restart
          if (m_autoScrollRepeatAnim)
          {
            if (m_autoScrollRepeatAnim->GetState() == ANIM_STATE_NONE)
              m_autoScrollRepeatAnim->QueueAnimation(ANIM_PROCESS_NORMAL);
            else if (m_autoScrollRepeatAnim->GetState() == ANIM_STATE_APPLIED)
            { // reset to the start of the list and start the scrolling again
              m_offset = 0;
              m_scrollOffset = 0;
              ResetAutoScrolling();
            }
          }
        }
      }
    }
    else if (m_autoScrollCondition)
      ResetAutoScrolling();  // conditional is false, so reset the autoscrolling
  }

  // update our scroll position as necessary
  if (m_lastRenderTime)
    m_scrollOffset += m_scrollSpeed * (m_renderTime - m_lastRenderTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset * m_itemHeight) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset * m_itemHeight))
  {
    m_scrollOffset = m_offset * m_itemHeight;
    m_scrollSpeed = 0;
  }
  m_lastRenderTime = m_renderTime;

  int offset = (int)(m_scrollOffset / m_itemHeight);

  g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height);

  // we offset our draw position to take into account scrolling and whether or not our focused
  // item is offscreen "above" the list.
  float posX = m_posX;
  float posY = m_posY + offset * m_itemHeight - m_scrollOffset;

  // alignment correction
  if (m_label.align & XBFONT_CENTER_X)
    posX += m_width * 0.5f;
  if (m_label.align & XBFONT_RIGHT)
    posX += m_width;

  if (m_font)
  {
    m_font->Begin();
    int current = offset;
    while (posY < m_posY + m_height && current < (int)m_lines.size())
    {
      DWORD align = m_label.align;
      if (m_lines[current].m_text.size() && m_lines[current].m_carriageReturn)
        align &= ~XBFONT_JUSTIFIED; // last line of a paragraph shouldn't be justified
      m_font->DrawText(posX, posY + 2, m_colors, m_label.shadowColor, m_lines[current].m_text, align, m_width);
      posY += m_itemHeight;
      current++;
    }
    m_font->End();
  }

  g_graphicsContext.RestoreClipRegion();

  if (!m_pageControl)
  {
    m_upDown.SetPosition(m_posX + m_spinPosX, m_posY + m_spinPosY);
    m_upDown.Render();
  }
  else
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, offset);
    SendWindowMessage(msg);
  }
  CGUIControl::Render();
}

bool CGUITextBox::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    OnPageUp();
    ResetAutoScrolling();
    return true;
    break;

  case ACTION_PAGE_DOWN:
    OnPageDown();
    ResetAutoScrolling();
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
        ResetAutoScrolling();
        if (m_upDown.GetValue() >= 1)
          ScrollToOffset((m_upDown.GetValue() - 1) * m_itemsPerPage);
      }
    }
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_offset = 0;
      m_scrollOffset = 0;
      ResetAutoScrolling();
      CGUITextLayout::Reset();
      m_upDown.SetRange(1, 1);
      m_upDown.SetValue(1);

      SetLabel( message.GetLabel() );
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_offset = 0;
      m_scrollOffset = 0;
      ResetAutoScrolling();
      CGUITextLayout::Reset();
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
      ResetAutoScrolling();
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS)
    {
      m_upDown.SetFocus(false);
    }

    if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl)
      { // update our page
        ResetAutoScrolling();
        ScrollToOffset(message.GetParam1());
        return true;
      }
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUITextBox::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_upDown.PreAllocResources();
}

void CGUITextBox::AllocResources()
{
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
    ScrollToOffset((m_upDown.GetValue() - 1) * m_itemsPerPage);
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
    ScrollToOffset((m_upDown.GetValue() - 1) * m_itemsPerPage);
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
    int offset = m_offset - wheel;
    // check that we are within the correct bounds.
    if (offset + m_itemsPerPage > (int)m_lines.size())
      offset = (m_lines.size() >= m_itemsPerPage) ? m_lines.size() - m_itemsPerPage : 0;
    ScrollToOffset(offset);
    // update the page control...
    int iPage = offset / m_itemsPerPage + 1;
    // last page??
    if (offset + m_itemsPerPage == m_lines.size())
      iPage = m_upDown.GetMaximum();
    m_upDown.SetValue(iPage);
    ResetAutoScrolling();
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

void CGUITextBox::ScrollToOffset(int offset, bool autoScroll)
{
  float size = m_itemHeight;
  m_scrollOffset = m_offset * m_itemHeight;
  int timeToScroll = autoScroll ? m_autoScrollTime : m_scrollTime;
  m_scrollSpeed = (offset * m_itemHeight - m_scrollOffset) / timeToScroll;
  m_offset = offset;
}

void CGUITextBox::SetAutoScrolling(const TiXmlNode *node)
{
  if (!node) return;
  const TiXmlElement *scroll = node->FirstChildElement("autoscroll");
  if (scroll)
  {
    scroll->Attribute("delay", &m_autoScrollDelay);
    scroll->Attribute("time", &m_autoScrollTime);
    if (scroll->FirstChild())
      m_autoScrollCondition = g_infoManager.TranslateString(scroll->FirstChild()->ValueStr());
    int repeatTime;
    if (scroll->Attribute("repeat", &repeatTime))
      m_autoScrollRepeatAnim = CAnimation::CreateFader(100, 0, repeatTime, 1000);
  }
}

void CGUITextBox::ResetAutoScrolling()
{
  m_autoScrollDelayTime = 0;
  if (m_autoScrollRepeatAnim)
    m_autoScrollRepeatAnim->ResetAnimation();
}
