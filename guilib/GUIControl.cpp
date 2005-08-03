#include "include.h"
#include "GUIControl.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "LocalizeStrings.h"
#include "../xbmc/Util.h"


CGUIControl::CGUIControl()
{
  m_bHasFocus = false;
  m_dwControlID = 0;
  m_iGroup = -1;
  m_dwParentID = 0;
  m_bVisible = true;
  m_visibleCondition = 0;
  m_bDisabled = false;
  m_bSelected = false;
  m_bCalibration = true;
  m_colDiffuse = 0xFFFFFFFF;
  m_dwAlpha = 0xFF;
  m_iPosX = 0;
  m_iPosY = 0;
  m_dwControlLeft = 0;
  m_dwControlRight = 0;
  m_dwControlUp = 0;
  m_dwControlDown = 0;
  m_fadingState = FADING_NONE;
  m_fadingTime = 0;
  m_fadingPos = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
}

CGUIControl::CGUIControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight)
{
  m_colDiffuse = 0xFFFFFFFF;
  m_dwAlpha = 0xFF;
  m_iPosX = iPosX;
  m_iPosY = iPosY;
  m_dwWidth = dwWidth;
  m_dwHeight = dwHeight;
  m_bHasFocus = false;
  m_dwControlID = dwControlId;
  m_iGroup = -1;
  m_dwParentID = dwParentID;
  m_bVisible = true;
  m_lastVisible = true;
  m_visibleCondition = 0;
  m_bDisabled = false;
  m_bSelected = false;
  m_bCalibration = true;
  m_dwControlLeft = 0;
  m_dwControlRight = 0;
  m_dwControlUp = 0;
  m_dwControlDown = 0;
  m_fadingState = FADING_NONE;
  m_fadingTime = 0;
  m_fadingPos = 0;
  ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
  m_bAllocated=false;
}


CGUIControl::~CGUIControl(void)
{

}

void CGUIControl::AllocResources()
{
  m_bInvalidated = true;
  m_bAllocated=true;
}

void CGUIControl::FreeResources()
{
  m_bAllocated=false;
}

bool CGUIControl::IsAllocated()
{
  return m_bAllocated;
}

void CGUIControl::DynamicResourceAlloc(bool bOnOff)
{

}

void CGUIControl::Render()
{
  m_bInvalidated = false;
}

bool CGUIControl::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_MOVE_DOWN:
    OnDown();
    return true;
    break;

  case ACTION_MOVE_UP:
    OnUp();
    return true;
    break;

  case ACTION_MOVE_LEFT:
    OnLeft();
    return true;
    break;

  case ACTION_MOVE_RIGHT:
    OnRight();
    return true;
    break;
  }
  return false;
}

// Movement controls (derived classes can override)
void CGUIControl::OnUp()
{
  if (HasFocus())
  {
    SetFocus(false);
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwControlUp, ACTION_MOVE_UP);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIControl::OnDown()
{
  if (HasFocus())
  {
    SetFocus(false);
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwControlDown, ACTION_MOVE_DOWN);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIControl::OnLeft()
{
  if (HasFocus())
  {
    SetFocus(false);
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwControlLeft, ACTION_MOVE_LEFT);
    g_graphicsContext.SendMessage(msg);
  }
}

void CGUIControl::OnRight()
{
  if (HasFocus())
  {
    SetFocus(false);
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_dwControlRight, ACTION_MOVE_RIGHT);
    g_graphicsContext.SendMessage(msg);
  }
}

DWORD CGUIControl::GetID(void) const
{
  return m_dwControlID;
}


DWORD CGUIControl::GetParentID(void) const
{
  return m_dwParentID;
}

bool CGUIControl::HasFocus(void) const
{
  return m_bHasFocus;
}

void CGUIControl::SetFocus(bool bOnOff)
{
  m_bHasFocus = bOnOff;
}

bool CGUIControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    switch (message.GetMessage() )
    {
    case GUI_MSG_SETFOCUS:
      // if control is disabled then move 2 the next control
      if ( IsDisabled() || !IsVisible() || !CanFocus() )
      {
        DWORD dwControl = 0;
        if (message.GetParam1() == ACTION_MOVE_DOWN) dwControl = m_dwControlDown;
        if (message.GetParam1() == ACTION_MOVE_UP) dwControl = m_dwControlUp;
        if (message.GetParam1() == ACTION_MOVE_LEFT) dwControl = m_dwControlLeft;
        if (message.GetParam1() == ACTION_MOVE_RIGHT) dwControl = m_dwControlRight;
        if (GetID() != dwControl)
        {
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), dwControl, message.GetParam1());
          g_graphicsContext.SendMessage(msg);
          return true;
        }
        else
        {
          //ok, the control points to itself, it will do a stackoverflow so try to go back
          DWORD dwReverse = 0;
          if (message.GetParam1() == ACTION_MOVE_DOWN) {dwReverse = ACTION_MOVE_UP; dwControl = m_dwControlUp;}
          if (message.GetParam1() == ACTION_MOVE_UP) {dwReverse = ACTION_MOVE_DOWN; dwControl = m_dwControlDown;}
          if (message.GetParam1() == ACTION_MOVE_LEFT) {dwReverse = ACTION_MOVE_RIGHT; dwControl = m_dwControlRight;}
          if (message.GetParam1() == ACTION_MOVE_RIGHT) {dwReverse = ACTION_MOVE_LEFT; dwControl = m_dwControlLeft;}
          //if the other direction also points to itself it will still stackoverflow but then it's just skinproblem imho
          CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), dwControl, dwReverse);
          g_graphicsContext.SendMessage(msg);
          return true;
        }
      }
      m_bHasFocus = true;
      return true;
      break;

    case GUI_MSG_LOSTFOCUS:
      {
        m_bHasFocus = false;
        return true;
      }
      break;

    case GUI_MSG_VISIBLE:
      if (message.GetParam1())  // fade time
      {
        if (!m_bVisible && m_fadingState != FADING_IN)
        {
          m_fadingState = FADING_IN;
          m_fadingTime = m_fadingPos = message.GetParam1();
        }
        else if (m_bVisible && m_fadingState == FADING_OUT)
        { // turn around direction of fade
          m_fadingState = FADING_IN;
          m_fadingPos = (int)((m_fadingTime - m_fadingPos) * (float)message.GetParam1() / m_fadingTime);
          m_fadingTime = message.GetParam1();
        }
      }
      else
      {
        SetAlpha(255);      // make sure it's fully visible.
      }
      m_bVisible = true;
      return true;
      break;

    case GUI_MSG_HIDDEN:
      if (message.GetParam1())  // fade time
      {
        if (m_bVisible && m_fadingState == FADING_NONE)
        {
          m_fadingState = FADING_OUT;
          m_fadingTime = m_fadingPos = message.GetParam1();
        }
        else if (m_bVisible && m_fadingState == FADING_IN)
        { // turn around direction of fade
          m_fadingState = FADING_OUT;
          m_fadingPos = (int)((m_fadingTime - m_fadingPos) * (float)message.GetParam1() / m_fadingTime);
          m_fadingTime = message.GetParam1();
        }
      }
      else
        m_bVisible = false;
      return true;
      break;

    case GUI_MSG_ENABLED:
      m_bDisabled = false;
      return true;
      break;

    case GUI_MSG_DISABLED:
      m_bDisabled = true;
      return true;
      break;
    case GUI_MSG_SELECTED:
      m_bSelected = true;
      return true;
      break;

    case GUI_MSG_DESELECTED:
      m_bSelected = false;
      return true;
      break;
    }
  }
  return false;
}

bool CGUIControl::CanFocus() const
{
  if (!IsVisible()) return false;
  if (IsDisabled()) return false;
  return true;
}

bool CGUIControl::IsVisible() const
{
  return m_bVisible;
}

bool CGUIControl::IsSelected() const
{
  return m_bSelected;
}

bool CGUIControl::IsDisabled() const
{
  return m_bDisabled;
}

void CGUIControl::SetEnabled(bool bEnable)
{
  m_bDisabled = !bEnable;
}

void CGUIControl::SetPosition(int iPosX, int iPosY)
{
  if ((m_iPosX != iPosX) || (m_iPosY != iPosY))
  {
    m_iPosX = iPosX;
    m_iPosY = iPosY;
    Update();
  }
}

void CGUIControl::SetAlpha(DWORD dwAlpha)
{
  m_dwAlpha = dwAlpha;
  D3DCOLOR colour = (dwAlpha << 24) | 0xFFFFFF;
  return SetColourDiffuse(colour);
}

void CGUIControl::SetColourDiffuse(D3DCOLOR colour)
{
  if (colour != m_colDiffuse)
  {
    m_colDiffuse = colour;
    Update();
  }
}

int CGUIControl::GetXPosition() const
{
  return m_iPosX;
}

int CGUIControl::GetYPosition() const
{
  return m_iPosY;
}

DWORD CGUIControl::GetWidth() const
{
  return m_dwWidth;
}

DWORD CGUIControl::GetHeight() const
{
  return m_dwHeight;
}

void CGUIControl::SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight)
{
  m_dwControlUp = dwUp;
  m_dwControlDown = dwDown;
  m_dwControlLeft = dwLeft;
  m_dwControlRight = dwRight;
}

void CGUIControl::SetWidth(int iWidth)
{
  if (m_dwWidth != iWidth)
  {
    m_dwWidth = iWidth;
    Update();
  }
}

void CGUIControl::SetHeight(int iHeight)
{
  if (m_dwHeight != iHeight)
  {
    m_dwHeight = iHeight;
    Update();
  }
}

void CGUIControl::SetVisible(bool bVisible)
{
  if (m_bVisible != bVisible)
  {
    m_bVisible = bVisible;
    m_bInvalidated = true;
  }
}

void CGUIControl::SetSelected(bool bSelected)
{
  if (m_bSelected != bSelected)
  {
    m_bSelected = bSelected;
    m_bInvalidated = true;
  }
}

bool CGUIControl::HitTest(int iPosX, int iPosY) const
{
  if (iPosX >= (int)m_iPosX && iPosX <= (int)(m_iPosX + m_dwWidth) && iPosY >= (int)m_iPosY && iPosY <= (int)(m_iPosY + m_dwHeight))
    return true;
  return false;
}

// override this function to implement custom mouse behaviour
void CGUIControl::OnMouseOver()
{
  if (g_Mouse.GetState() != MOUSE_STATE_DRAG)
    g_Mouse.SetState(MOUSE_STATE_FOCUS);
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetParentID(), GetID());
  g_graphicsContext.SendMessage(msg);
}

void CGUIControl::SetGroup(int iGroup)
{
  m_iGroup = iGroup;
}

int CGUIControl::GetGroup(void) const
{
  return m_iGroup;
}

CStdString CGUIControl::ParseLabel(CStdString& strLabel)
{
  CStdString strReturn = "";
  int iPos1 = 0;
  int iPos2 = strLabel.Find('$', iPos1);
  bool bDoneSomething = !(iPos1 == iPos2);
  while (iPos2 >= 0)
  {
    if( (iPos2 > iPos1) && bDoneSomething )
    {
      strReturn += strLabel.Mid(iPos1, iPos2 - iPos1);
      bDoneSomething = false;  
    }

    int iPos3 = 0;
    CStdString str;
    CStdString strInfo = "INFO[";
    CStdString strLocalize = "LOCALIZE[";

    // $INFO[something]
    if (strLabel.Mid(iPos2 + 1,strInfo.size()).Equals(strInfo))
    {
      iPos2 += strInfo.size() + 1;
      iPos3 = strLabel.Find(']', iPos2);
      CStdString strValue = strLabel.Mid(iPos2,(iPos3 - iPos2));
      int iInfo = g_infoManager.TranslateString(strValue);
      if (iInfo)
      {
        str = g_infoManager.GetLabel(iInfo);
        if (str.size() > 0)
          bDoneSomething = true;
      }
    }

    // $LOCALIZE[something]
    else if (strLabel.Mid(iPos2 + 1,strLocalize.size()).Equals(strLocalize))
    {
      iPos2 += strLocalize.size() + 1;
      iPos3 = strLabel.Find(']', iPos2);
      CStdString strValue = strLabel.Mid(iPos2,(iPos3 - iPos2));
      if (CUtil::IsNaturalNumber(strValue))
      {
        int iLocalize = atoi(strValue);
        str = g_localizeStrings.Get(iLocalize);
        if (str.size() > 0)
          bDoneSomething = true;
      }
    }

    // $$ prints $
    else if (strLabel[iPos2 + 1] == '$')
    { 
      iPos3 = iPos2 + 1;
      str = '$';
      bDoneSomething = true;
    }

    //Okey, nothing found, just print it right out
    else
    {
      iPos3 = iPos2;
      str = '$';
      bDoneSomething = true;
    }

    strReturn += str;
    iPos1 = iPos3 + 1;
    iPos2 = strLabel.Find('$', iPos1);
  }

  if (iPos1 < (int)strLabel.size())
    strReturn += strLabel.Right(strLabel.size() - iPos1);

  return strReturn;
}


void CGUIControl::UpdateVisibility()
{
  bool bVisible = g_infoManager.GetBool(m_visibleCondition);
  if (!m_lastVisible && bVisible)
  { // automatic change of visibility - fade in
    SET_CONTROL_FADE_IN(GetID(), m_visibleFadeTime)
  }
  else if (m_lastVisible && !bVisible)
  { // automatic change of visibility - fade out
    SET_CONTROL_FADE_OUT(GetID(), m_visibleFadeTime)
  }
  m_lastVisible = bVisible;
}

void CGUIControl::SetInitialVisibility()
{
  if (m_visibleStartState == FADING_NONE)
  {
    m_lastVisible = m_bVisible = g_infoManager.GetBool(m_visibleCondition);
    return;
  }
  m_lastVisible = m_bVisible = (m_visibleStartState == FADING_OUT);
  UpdateVisibility();
}

bool CGUIControl::UpdateFadeState()
{
  if (m_visibleCondition)
    UpdateVisibility();
  // update our alpha values if we're fading
  if (m_fadingState == FADING_IN)
  { // doing a fade in
    if (m_fadingPos)
      m_fadingPos--;
    if (!m_fadingPos)
    {
      m_fadingState = FADING_NONE;
    }
    DWORD fadeAmount = (DWORD)(m_fadingPos * 255.0f / m_fadingTime + 0.5f);
    g_graphicsContext.SetControlAlpha(255 - fadeAmount);
  }
  else if (m_fadingState == FADING_OUT)
  {
    if (m_fadingPos)
      m_fadingPos--;
    if (!m_fadingPos)
    {
      m_fadingState = FADING_NONE;
      m_bVisible = false;
    }
    DWORD fadeAmount = (DWORD)(m_fadingPos * 255.0f / m_fadingTime + 0.5f);
    g_graphicsContext.SetControlAlpha(fadeAmount);
  }
  return IsVisible();
}

void CGUIControl::SetVisibleCondition(int visible, int fadeTime /*= 0*/, FADE_STATE startHidden /*= FADING_NONE*/)
{
  m_visibleCondition = visible;
  m_visibleFadeTime = fadeTime;
  m_visibleStartState = startHidden;
}
