#include "include.h"
#include "GUISelectButtonControl.h"
#include "GUIWindowManager.h"
#include "../xbmc/utils/CharsetConverter.h"


CGUISelectButtonControl::CGUISelectButtonControl(DWORD dwParentID, DWORD dwControlId,
    float posX, float posY,
    float width, float height,
    const CImage& buttonFocus,
    const CImage& button,
    const CLabelInfo& labelInfo,
    const CImage& selectBackground,
    const CImage& selectArrowLeft,
    const CImage& selectArrowLeftFocus,
    const CImage& selectArrowRight,
    const CImage& selectArrowRightFocus
                                                )
    : CGUIButtonControl(dwParentID, dwControlId, posX, posY, width, height, buttonFocus, button, labelInfo)
    , m_imgBackground(dwParentID, dwControlId, posX, posY, width, height, selectBackground)
    , m_imgLeft(dwParentID, dwControlId, posX, posY, 16, 16, selectArrowLeft)
    , m_imgLeftFocus(dwParentID, dwControlId, posX, posY, 16, 16, selectArrowLeftFocus)
    , m_imgRight(dwParentID, dwControlId, posX, posY, 16, 16, selectArrowRight)
    , m_imgRightFocus(dwParentID, dwControlId, posX, posY, 16, 16, selectArrowRightFocus)
{
  m_bShowSelect = false;
  m_iCurrentItem = -1;
  m_iDefaultItem = -1;
  m_iStartFrame = 0;
  m_bLeftSelected = false;
  m_bRightSelected = false;
  m_bMovedLeft = false;
  m_bMovedRight = false;
  m_dwTicks = 0;
  ControlType = GUICONTROL_SELECTBUTTON;
}

CGUISelectButtonControl::~CGUISelectButtonControl(void)
{}

void CGUISelectButtonControl::Render()
{
  // Are we in selection mode
  if (m_bShowSelect)
  {
    // render background, left and right arrow
    m_imgBackground.Render();

    D3DCOLOR dwTextColor = m_label.textColor;

    // User has moved left...
    if (m_bMovedLeft)
    {
      m_iStartFrame++;
      if (m_iStartFrame >= 10)
      {
        m_iStartFrame = 0;
        m_bMovedLeft = false;
      }
      // If we are moving left
      // render item text as disabled
      dwTextColor = m_label.disabledColor;
    }

    // Render arrow
    if (m_bLeftSelected || m_bMovedLeft)
      m_imgLeftFocus.Render();
    else
      m_imgLeft.Render();

    // User has moved right...
    if (m_bMovedRight)
    {
      m_iStartFrame++;
      if (m_iStartFrame >= 10)
      {
        m_iStartFrame = 0;
        m_bMovedRight = false;
      }
      // If we are moving right
      // render item text as disabled
      dwTextColor = m_label.disabledColor;
    }

    // Render arrow
    if (m_bRightSelected || m_bMovedRight)
      m_imgRightFocus.Render();
    else
      m_imgRight.Render();

    // Render text if a current item is available
    if (m_iCurrentItem >= 0 && (unsigned)m_iCurrentItem < m_vecItems.size())
    {
      m_textLayout.Update(m_vecItems[m_iCurrentItem]);
      DWORD dwAlign = m_label.align | XBFONT_CENTER_X;
      float fPosY = m_posY + m_label.offsetY;
      if (m_label.align & XBFONT_CENTER_Y)
        fPosY = m_posY + m_imgBackground.GetHeight()*0.5f;
      m_textLayout.Render(m_posX + GetWidth()*0.5f, fPosY, 0, dwTextColor, m_label.shadowColor, dwAlign, m_label.width);
    }

    // Select current item, if user doesn't
    // move left or right for 1.5 sec.
    DWORD dwTicksSpan = timeGetTime() - m_dwTicks;
    if (dwTicksSpan > 1500)
    {
      // User hasn't moved disable selection mode...
      m_bShowSelect = false;

      // ...and send a thread message.
      // (Sending a message with SendMessage
      // can result in a GPF.)
      CGUIMessage message(GUI_MSG_CLICKED, GetID(), GetParentID() );
      m_gWindowManager.SendThreadMessage(message);
    }
  } // if (m_bShowSelect)
  else
  {
    // No, render a normal button
    CGUIButtonControl::Render();
  }
  CGUIControl::Render();
}

bool CGUISelectButtonControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      if (m_vecItems.size() <= 0)
      {
        m_iCurrentItem = 0;
        m_iDefaultItem = 0;
      }
      m_vecItems.push_back(message.GetLabel());
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
      m_iCurrentItem = -1;
      m_iDefaultItem = -1;
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_iCurrentItem);
      if (m_iCurrentItem >= 0 && m_iCurrentItem < (int)m_vecItems.size())
        message.SetLabel(m_vecItems[m_iCurrentItem]);
      return true;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      m_iDefaultItem = m_iCurrentItem = message.GetParam1();
      return true;
    }
  }

  return CGUIButtonControl::OnMessage(message);
}

bool CGUISelectButtonControl::OnAction(const CAction &action)
{
  if (!m_bShowSelect)
  {
    if (action.wID == ACTION_SELECT_ITEM)
    {
      // Enter selection mode
      m_bShowSelect = true;

      // Start timer, if user doesn't select an item
      // or moves left/right. The control will
      // automatically select the current item.
      m_dwTicks = timeGetTime();
      return true;
    }
    else
      return CGUIButtonControl::OnAction(action);
  }
  else
  {
    if (action.wID == ACTION_SELECT_ITEM)
    {
      // User has selected an item, disable selection mode...
      m_bShowSelect = false;

      // ...and send a message.
      CGUIMessage message(GUI_MSG_CLICKED, GetID(), GetParentID() );
      SendWindowMessage(message);
      return true;
    }
    if (action.wID == ACTION_MOVE_UP || action.wID == ACTION_MOVE_DOWN )
    {
      // Disable selection mode when moving up or down
      m_bShowSelect = false;
      m_iCurrentItem = m_iDefaultItem;
    }
    // call the base class
    return CGUIButtonControl::OnAction(action);
  }
}

void CGUISelectButtonControl::FreeResources()
{
  CGUIButtonControl::FreeResources();

  m_imgBackground.FreeResources();

  m_imgLeft.FreeResources();
  m_imgLeftFocus.FreeResources();

  m_imgRight.FreeResources();
  m_imgRightFocus.FreeResources();

  m_bShowSelect = false;
}

void CGUISelectButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);

  m_imgBackground.DynamicResourceAlloc(bOnOff);

  m_imgLeft.DynamicResourceAlloc(bOnOff);
  m_imgLeftFocus.DynamicResourceAlloc(bOnOff);

  m_imgRight.DynamicResourceAlloc(bOnOff);
  m_imgRightFocus.DynamicResourceAlloc(bOnOff);
}

void CGUISelectButtonControl::PreAllocResources()
{
  CGUIButtonControl::PreAllocResources();

  m_imgBackground.PreAllocResources();

  m_imgLeft.PreAllocResources();
  m_imgLeftFocus.PreAllocResources();

  m_imgRight.PreAllocResources();
  m_imgRightFocus.PreAllocResources();
}

void CGUISelectButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();

  m_imgBackground.AllocResources();

  m_imgLeft.AllocResources();
  m_imgLeftFocus.AllocResources();

  m_imgRight.AllocResources();
  m_imgRightFocus.AllocResources();

  // Position right arrow
  float posX = (m_posX + m_width - 8) - 16;
  float posY = m_posY + (m_height - 16) / 2;
  m_imgRight.SetPosition(posX, posY);
  m_imgRightFocus.SetPosition(posX, posY);

  // Position left arrow
  posX = m_posX + 8;
  m_imgLeft.SetPosition(posX, posY);
  m_imgLeftFocus.SetPosition(posX, posY);
}

void CGUISelectButtonControl::Update()
{
  CGUIButtonControl::Update();

  m_imgBackground.SetWidth(m_width);
  m_imgBackground.SetHeight(m_height);
}

void CGUISelectButtonControl::OnLeft()
{
  if (m_bShowSelect)
  {
    // Set for visual feedback
    m_bMovedLeft = true;
    m_iStartFrame = 0;

    // Reset timer for automatically selecting
    // the current item.
    m_dwTicks = timeGetTime();

    // Switch to previous item
    if (m_vecItems.size() > 0)
    {
      m_iCurrentItem--;
      if (m_iCurrentItem < 0)
        m_iCurrentItem = (int)m_vecItems.size() - 1;
    }
  }
  else
  { // use the base class
    CGUIButtonControl::OnLeft();
  }
}

void CGUISelectButtonControl::OnRight()
{
  if (m_bShowSelect)
  {
    // Set for visual feedback
    m_bMovedRight = true;
    m_iStartFrame = 0;

    // Reset timer for automatically selecting
    // the current item.
    m_dwTicks = timeGetTime();

    // Switch to next item
    if (m_vecItems.size() > 0)
    {
      m_iCurrentItem++;
      if (m_iCurrentItem >= (int)m_vecItems.size())
        m_iCurrentItem = 0;
    }
  }
  else
  { // use the base class
    CGUIButtonControl::OnRight();
  }
}

bool CGUISelectButtonControl::OnMouseOver(const CPoint &point)
{
  bool ret = CGUIControl::OnMouseOver(point);
  m_bLeftSelected = false;
  m_bRightSelected = false;
  if (m_imgLeft.HitTest(point))
  { // highlight the left control, but don't start moving until we have clicked
    m_bLeftSelected = true;
  }
  if (m_imgRight.HitTest(point))
  { // highlight the right control, but don't start moving until we have clicked
    m_bRightSelected = true;
  }
  // reset ticks
  m_dwTicks = timeGetTime();
  return ret;
}

bool CGUISelectButtonControl::OnMouseClick(DWORD dwButton, const CPoint &point)
{ // only left click handled
  if (dwButton != MOUSE_LEFT_BUTTON) return false;
  if (m_bShowSelect && m_imgLeft.HitTest(point))
  { // move left
    OnLeft();
  }
  else if (m_bShowSelect && m_imgRight.HitTest(point))
  { // move right
    OnRight();
  }
  else
  { // normal select
    CGUIButtonControl::OnMouseClick(dwButton, point);
  }
  return true;
}

bool CGUISelectButtonControl::OnMouseWheel(char wheel, const CPoint &point)
{
  if (wheel > 0)
    OnLeft();
  else
    OnRight();
  return true;
}

void CGUISelectButtonControl::SetPosition(float posX, float posY)
{
  float leftOffX = m_imgLeft.GetXPosition() - m_posX;
  float leftOffY = m_imgLeft.GetYPosition() - m_posY;
  float rightOffX = m_imgRight.GetXPosition() - m_posX;
  float rightOffY = m_imgRight.GetYPosition() - m_posY;
  float backOffX = m_imgBackground.GetXPosition() - m_posX;
  float backOffY = m_imgBackground.GetYPosition() - m_posY;
  CGUIButtonControl::SetPosition(posX, posY);
  m_imgLeft.SetPosition(posX + leftOffX, posY + leftOffY);
  m_imgLeftFocus.SetPosition(posX + leftOffX, posY + leftOffY);
  m_imgRight.SetPosition(posX + rightOffX, posY + rightOffY);
  m_imgRightFocus.SetPosition(posX + rightOffX, posY + rightOffY);
  m_imgBackground.SetPosition(posX + backOffX, posY + backOffY);
}

void CGUISelectButtonControl::SetColorDiffuse(D3DCOLOR color)
{
  CGUIButtonControl::SetColorDiffuse(color);
  m_imgLeft.SetColorDiffuse(color);
  m_imgLeftFocus.SetColorDiffuse(color);
  m_imgRight.SetColorDiffuse(color);
  m_imgRightFocus.SetColorDiffuse(color);
  m_imgBackground.SetColorDiffuse(color);
}