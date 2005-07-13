#include "include.h"
#include "GUISelectButtonControl.h"
#include "GUIWindowManager.h"
#include "../xbmc/utils/CharsetConverter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGUISelectButtonControl::CGUISelectButtonControl(DWORD dwParentID, DWORD dwControlId,
    int iPosX, int iPosY,
    DWORD dwWidth, DWORD dwHeight,
    const CStdString& strButtonFocus,
    const CStdString& strButton,
    DWORD dwTextOffsetX,
    DWORD dwTextOffsetY,
    DWORD dwTextAlign,
    const CStdString& strSelectBackground,
    const CStdString& strSelectArrowLeft,
    const CStdString& strSelectArrowLeftFocus,
    const CStdString& strSelectArrowRight,
    const CStdString& strSelectArrowRightFocus
                                                )
    : CGUIButtonControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strButtonFocus, strButton, dwTextOffsetX, dwTextOffsetY, dwTextAlign)
    , m_imgBackground(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strSelectBackground)
    , m_imgLeft(dwParentID, dwControlId, iPosX, iPosY, 16, 16, strSelectArrowLeft)
    , m_imgLeftFocus(dwParentID, dwControlId, iPosX, iPosY, 16, 16, strSelectArrowLeftFocus)
    , m_imgRight(dwParentID, dwControlId, iPosX, iPosY, 16, 16, strSelectArrowRight)
    , m_imgRightFocus(dwParentID, dwControlId, iPosX, iPosY, 16, 16, strSelectArrowRightFocus)
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
  if (!UpdateVisibility() ) return ;


  // Are we in selection mode
  if (m_bShowSelect)
  {
    // Yes, render the select control

    // render background, left and right arrow

    m_imgBackground.Render();

    D3DCOLOR dwTextColor = m_dwTextColor;

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
      dwTextColor = m_dwDisabledColor;
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
      dwTextColor = m_dwDisabledColor;
    }

    // Render arrow
    if (m_bRightSelected || m_bMovedRight)
      m_imgRightFocus.Render();
    else
      m_imgRight.Render();


    // Render text if a current item is available
    if (m_iCurrentItem >= 0 && m_pFont && (unsigned)m_iCurrentItem < m_vecItems.size())
    {
      if (m_vecItems[m_iCurrentItem].size())
      {
        CStdStringW itemStrUnicode;
        g_charsetConverter.stringCharsetToFontCharset(m_vecItems[m_iCurrentItem].c_str(), itemStrUnicode);

        DWORD dwAlign = m_dwTextAlignment | XBFONT_CENTER_X;
        float fPosY = (float)m_iPosY + m_dwTextOffsetY;
        if (m_dwTextAlignment & XBFONT_CENTER_Y)
          fPosY = (float)m_iPosY + m_imgBackground.GetHeight() / 2;
        m_pFont->DrawText( (float)m_iPosX + GetWidth()/2, fPosY, dwTextColor, itemStrUnicode.c_str(), dwAlign);
      }
    }


    // Select current item, if user doesn't
    // move left or right for 1.5 sec.
    DWORD dwTicksSpan = timeGetTime() - m_dwTicks;
    if ((float)(dwTicksSpan / 1000) > 1.5f)
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
    }
    else if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
      m_iCurrentItem = -1;
      m_iDefaultItem = -1;
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_iCurrentItem);
      message.SetLabel(m_vecItems[m_iCurrentItem]);
    }
    else if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      m_iDefaultItem = m_iCurrentItem = message.GetParam1();
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
      g_graphicsContext.SendMessage(message);
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
  int iPosX = (m_iPosX + m_dwWidth - 8) - 16;
  int iPosY = m_iPosY + (m_dwHeight - 16) / 2;
  m_imgRight.SetPosition(iPosX, iPosY);
  m_imgRightFocus.SetPosition(iPosX, iPosY);

  // Position left arrow
  iPosX = m_iPosX + 8;
  m_imgLeft.SetPosition(iPosX, iPosY);
  m_imgLeftFocus.SetPosition(iPosX, iPosY);
}

void CGUISelectButtonControl::Update()
{
  CGUIButtonControl::Update();

  m_imgBackground.SetWidth(m_dwWidth);
  m_imgBackground.SetHeight(m_dwHeight);
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
        m_iCurrentItem = m_vecItems.size() - 1;
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

void CGUISelectButtonControl::OnMouseOver()
{
  CGUIControl::OnMouseOver();
  m_bLeftSelected = false;
  m_bRightSelected = false;
  if (m_imgLeft.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  { // highlight the left control, but don't start moving until we have clicked
    m_bLeftSelected = true;
  }
  if (m_imgRight.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  { // highlight the right control, but don't start moving until we have clicked
    m_bRightSelected = true;
  }
  // reset ticks
  m_dwTicks = timeGetTime();
}

void CGUISelectButtonControl::OnMouseClick(DWORD dwButton)
{ // only left click handled
  if (dwButton != MOUSE_LEFT_BUTTON) return ;
  if (m_bShowSelect && m_imgLeft.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  { // move left
    OnLeft();
  }
  else if (m_bShowSelect && m_imgRight.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  { // move right
    OnRight();
  }
  else
  { // normal select
    CGUIButtonControl::OnMouseClick(dwButton);
  }
}

void CGUISelectButtonControl::OnMouseWheel()
{
  if (g_Mouse.cWheel > 0)
    OnLeft();
  else
    OnRight();
}

void CGUISelectButtonControl::SetPosition(int iPosX, int iPosY)
{
  int iLeftOffX = m_imgLeft.GetXPosition() - m_iPosX;
  int iLeftOffY = m_imgLeft.GetYPosition() - m_iPosY;
  int iRightOffX = m_imgRight.GetXPosition() - m_iPosX;
  int iRightOffY = m_imgRight.GetYPosition() - m_iPosY;
  int iBackOffX = m_imgBackground.GetXPosition() - m_iPosX;
  int iBackOffY = m_imgBackground.GetYPosition() - m_iPosY;
  CGUIButtonControl::SetPosition(iPosX, iPosY);
  m_imgLeft.SetPosition(iPosX + iLeftOffX, iPosY + iLeftOffY);
  m_imgLeftFocus.SetPosition(iPosX + iLeftOffX, iPosY + iLeftOffY);
  m_imgRight.SetPosition(iPosX + iRightOffX, iPosY + iRightOffY);
  m_imgRightFocus.SetPosition(iPosX + iRightOffX, iPosY + iRightOffY);
  m_imgBackground.SetPosition(iPosX + iBackOffX, iPosY + iBackOffY);
}