#include "guicontrol.h"


CGUIControl::CGUIControl()
{
  m_bHasFocus=false;
  m_dwControlID = 0;
  m_dwParentID = 0;
	m_bVisible=true;
	m_bDisabled=false;
  m_bSelected=false;
	m_bCalibration=true;
  m_colDiffuse	= 0xFFFFFFFF;  
  m_dwPosX=0;
  m_dwPosY=0;
  m_dwControlLeft=0;
  m_dwControlRight=0;
  m_dwControlUp=0;
  m_dwControlDown=0;
}

CGUIControl::CGUIControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight)
{
  m_colDiffuse	= 0xFFFFFFFF;  
  m_dwPosX=dwPosX;
  m_dwPosY=dwPosY;
  m_dwWidth=dwWidth;
  m_dwHeight=dwHeight;
  m_bHasFocus=false;
  m_dwControlID = dwControlId;
  m_dwParentID = dwParentID;
	m_bVisible=true;
	m_bDisabled=false;
  m_bSelected=false;
	m_bCalibration=true;
  m_dwControlLeft=0;
  m_dwControlRight=0;
  m_dwControlUp=0;
  m_dwControlDown=0;
}


CGUIControl::~CGUIControl(void)
{
}

void CGUIControl::Render()
{
}

void CGUIControl::OnAction(const CAction &action) 
{
  switch (action.wID)
  {
    case ACTION_MOVE_DOWN:
    {
      SetFocus(false);
      CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwControlDown, action.wID);
      g_graphicsContext.SendMessage(msg);
    }
    break;
    
    case ACTION_MOVE_UP:
    {
      SetFocus(false);
      CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwControlUp, action.wID);
      g_graphicsContext.SendMessage(msg);
    }
    break;
    
    case ACTION_MOVE_LEFT:
    {
      SetFocus(false);
      CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwControlLeft, action.wID);
      g_graphicsContext.SendMessage(msg);
    }
    break;

    case ACTION_MOVE_RIGHT:
    {
      SetFocus(false);
      CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), m_dwControlRight, action.wID);
      g_graphicsContext.SendMessage(msg);
    }
    break;
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
          if (message.GetParam1()==ACTION_MOVE_DOWN) dwControl = m_dwControlDown;
          if (message.GetParam1()==ACTION_MOVE_UP) dwControl = m_dwControlUp;
          if (message.GetParam1()==ACTION_MOVE_LEFT) dwControl = m_dwControlLeft;
          if (message.GetParam1()==ACTION_MOVE_RIGHT) dwControl = m_dwControlRight;
          CGUIMessage msg(GUI_MSG_SETFOCUS,GetID(), dwControl, message.GetParam1());
          g_graphicsContext.SendMessage(msg);
          return true;
        }
        m_bHasFocus=true;
        return true;
      break;
      
      case GUI_MSG_LOSTFOCUS:
        m_bHasFocus=false;
        return true;
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


void CGUIControl::SetPosition(DWORD dwPosX, DWORD dwPosY)
{
  m_dwPosX=dwPosX;
  m_dwPosY=dwPosY;
  Update();
}


void CGUIControl::SetAlpha(DWORD dwAlpha)
{
	D3DCOLOR colour = (dwAlpha << 24) | 0xFFFFFF;
	return SetColourDiffuse(colour);
}

void CGUIControl::SetColourDiffuse(D3DCOLOR colour)
{
	if (colour!=m_colDiffuse)
	{
		m_colDiffuse = colour;
    Update();
	}
}
DWORD CGUIControl::GetXPosition() const
{
  return m_dwPosX;
}
DWORD CGUIControl::GetYPosition() const
{
  return m_dwPosY;
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
  m_dwWidth=iWidth;
  Update();
}

void CGUIControl::SetHeight(int iHeight)
{
  m_dwHeight=iHeight;
  Update();
}

void CGUIControl::SetVisible(bool bVisible)
{
  m_bVisible=bVisible;
}

void CGUIControl::EnableCalibration(bool bOnOff)
{
	m_bCalibration=bOnOff;
}
bool CGUIControl::CalibrationEnabled() const
{
	return m_bCalibration;
}