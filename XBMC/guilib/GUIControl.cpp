#include "stdafx.h"
#include "GUIControl.h"

CGUIControl::CGUIControl()
{
  m_bHasFocus=false;
  m_dwControlID = 0;
  m_iGroup=-1;
  m_dwParentID = 0;
	m_bVisible=true;
	m_bDisabled=false;
  m_bSelected=false;
	m_bCalibration=true;
  m_colDiffuse	= 0xFFFFFFFF;
  m_dwAlpha = 0xFF;
  m_iPosX=0;
  m_iPosY=0;
  m_dwControlLeft=0;
  m_dwControlRight=0;
  m_dwControlUp=0;
  m_dwControlDown=0;
	ControlType = GUICONTROL_UNKNOWN;
  m_bInvalidated = true;
}

CGUIControl::CGUIControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight)
{
  m_colDiffuse	= 0xFFFFFFFF;  
  m_dwAlpha = 0xFF;
  m_iPosX=iPosX;
  m_iPosY=iPosY;
  m_dwWidth=dwWidth;
  m_dwHeight=dwHeight;
  m_bHasFocus=false;
  m_dwControlID = dwControlId;
  m_iGroup=-1;
  m_dwParentID = dwParentID;
	m_bVisible=true;
	m_bDisabled=false;
  m_bSelected=false;
	m_bCalibration=true;
  m_dwControlLeft=0;
  m_dwControlRight=0;
  m_dwControlUp=0;
  m_dwControlDown=0;
	ControlType = GUICONTROL_UNKNOWN;
}


CGUIControl::~CGUIControl(void)
{
}

void CGUIControl::AllocResources()
{
  m_bInvalidated = true;
}

void CGUIControl::Render()
{
  m_bInvalidated = false;
}

void CGUIControl::OnAction(const CAction &action) 
{
	switch (action.wID)
	{
		case ACTION_MOVE_DOWN:
			OnDown();
		break;
	    
		case ACTION_MOVE_UP:
			OnUp();
		break;
	    
		case ACTION_MOVE_LEFT:
			OnLeft();
		break;

		case ACTION_MOVE_RIGHT:
			OnRight();
		break;
	}
}

// Movement controls (derived classes can override)
void CGUIControl::OnUp()
{
	if (HasFocus())
    {
		SetFocus(false);
		CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwControlUp, ACTION_MOVE_UP);
		g_graphicsContext.SendMessage(msg);
    }
}
void CGUIControl::OnDown()
{
	if (HasFocus())
    {
		SetFocus(false);
		CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwControlDown, ACTION_MOVE_DOWN);
		g_graphicsContext.SendMessage(msg);
    }
}
void CGUIControl::OnLeft()
{
	if (HasFocus())
    {
		SetFocus(false);
		CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwControlLeft, ACTION_MOVE_LEFT);
		g_graphicsContext.SendMessage(msg);
    }
}
void CGUIControl::OnRight()
{
	if (HasFocus())
    {
		SetFocus(false);
		CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwControlRight, ACTION_MOVE_RIGHT);
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
  m_bHasFocus=bOnOff;
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
          DWORD dwControl=0;
          if (message.GetParam1()==ACTION_MOVE_DOWN)  dwControl = m_dwControlDown;
          if (message.GetParam1()==ACTION_MOVE_UP)    dwControl = m_dwControlUp;
          if (message.GetParam1()==ACTION_MOVE_LEFT)  dwControl = m_dwControlLeft;
          if (message.GetParam1()==ACTION_MOVE_RIGHT) dwControl = m_dwControlRight;
          if (GetID() != dwControl)
          {
            CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), dwControl, message.GetParam1());
            g_graphicsContext.SendMessage(msg);
            return true;
          }
          else
          {
            //ok, the control points to itself, it will do a stackoverflow so try to go back
            DWORD dwReverse = 0;
            if (message.GetParam1() == ACTION_MOVE_DOWN)  {dwReverse = ACTION_MOVE_UP;    dwControl = m_dwControlUp;}
            if (message.GetParam1() == ACTION_MOVE_UP)    {dwReverse = ACTION_MOVE_DOWN;  dwControl = m_dwControlDown;}
            if (message.GetParam1() == ACTION_MOVE_LEFT)  {dwReverse = ACTION_MOVE_RIGHT; dwControl = m_dwControlRight;}
            if (message.GetParam1() == ACTION_MOVE_RIGHT) {dwReverse = ACTION_MOVE_LEFT;  dwControl = m_dwControlLeft;}
            //if the other direction also points to itself it will still stackoverflow but then it's just skinproblem imho
            CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), dwControl, dwReverse);
            g_graphicsContext.SendMessage(msg);
            return true;
          }
        }
        m_bHasFocus=true;
        return true;
      break;
      
      case GUI_MSG_LOSTFOCUS:
			{
				m_bHasFocus=false;
				return true;
			}
      break;

      case GUI_MSG_VISIBLE:
        m_bVisible=true;
        return true;
      break;
      
      case GUI_MSG_HIDDEN:
        m_bVisible=false;
        return true;
      break;

      case GUI_MSG_ENABLED:
        m_bDisabled=false;
        return true;
      break;
      
      case GUI_MSG_DISABLED:
        m_bDisabled=true;
        return true;
      break;
      case GUI_MSG_SELECTED:
        m_bSelected=true;
        return true;
      break;

      case GUI_MSG_DESELECTED:
        m_bSelected=false;
        return true;
      break;
    }    
  }
  return false;
}


bool CGUIControl::CanFocus()  const
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
    m_iPosX=iPosX;
    m_iPosY=iPosY;
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
  m_dwControlUp=dwUp;
  m_dwControlDown=dwDown;
  m_dwControlLeft=dwLeft;
  m_dwControlRight=dwRight;
}


void CGUIControl::SetWidth(int iWidth)
{
  if (m_dwWidth != iWidth)
  {
    m_dwWidth=iWidth;
    Update();
  }
}

void CGUIControl::SetHeight(int iHeight)
{
  if (m_dwHeight != iHeight)
  {
    m_dwHeight=iHeight;
    Update();
  }
}

void CGUIControl::SetVisible(bool bVisible)
{
  if (m_bVisible != bVisible)
  {
    m_bVisible=bVisible;
    m_bInvalidated = true;
  }
}
void CGUIControl::SetSelected(bool bSelected)
{
  if (m_bSelected != bSelected)
  {
    m_bSelected=bSelected;
    m_bInvalidated = true;
  }
}
void CGUIControl::EnableCalibration(bool bOnOff)
{
  if (m_bCalibration != bOnOff)
  {
	  m_bCalibration = bOnOff;
    m_bInvalidated = true;
  }
}
bool CGUIControl::CalibrationEnabled() const
{
	return m_bCalibration;
}

bool CGUIControl::HitTest(int iPosX, int iPosY) const
{
	if (!CalibrationEnabled()) g_graphicsContext.Correct(iPosX, iPosY);
	if (iPosX >= (int)m_iPosX && iPosX <= (int)(m_iPosX+m_dwWidth) && iPosY >= (int)m_iPosY && iPosY <= (int)(m_iPosY+m_dwHeight))
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
  m_iGroup=iGroup;
}

int CGUIControl::GetGroup(void) const
{
  return m_iGroup;
}
