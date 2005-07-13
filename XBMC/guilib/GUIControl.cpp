#include "include.h"
#include "GUIControl.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "LocalizeStrings.h"
#include "../xbmc/Util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGUIControl::CGUIControl()
{
  m_bHasFocus = false;
  m_dwControlID = 0;
  m_iGroup = -1;
  m_dwParentID = 0;
  m_bVisible = true;
  m_VisibleCondition = 0;
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
  m_VisibleCondition = 0;
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
          m_fadingPos = (int)(m_fadingPos * (float)message.GetParam1() / m_fadingTime);
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
          m_fadingPos = (int)(m_fadingPos * (float)message.GetParam1() / m_fadingTime);
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
  if (!m_bVisible)
    return false;
  if (m_VisibleCondition && !g_infoManager.GetBool(m_VisibleCondition))
    return false;
  return true;
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

CStdString CGUIControl::ParseLabel(CStdString &strLabel)
{
  CStdString toString = L"";
  CStdString queueString = L"";
  int lastPos = 0;
  bool lastValid = true;

  // first first parseable item
  int t = strLabel.Find('$');
  while ( t >= 0 )
  {
    int skip = 0;
    CStdString checkEscape = "";

    // read next character
		if (t+1 < (int)strLabel.length())
		{
			checkEscape = strLabel.substr(t+1,1);
		}
    bool emptyInfo = false;

    // $ means escape
    if (!checkEscape.Equals("$"))
    {
      //write all text since the last parsable item till this one to the literal text queue
      if (t > lastPos)
      {
        CStdString tempString = strLabel.substr(lastPos,t-lastPos);
        tempString.Replace("$$","$");
        queueString.append(tempString);
      }

      // look for a type delimiter
      CStdString infoString = L"";
      int startData = strLabel.Find('(',t);
      int endData = strLabel.Find(')',t);
      if (startData > t && endData > startData)
      {
        CStdString strType = strLabel.substr(t+1,(startData - t)-1);
        CStdString strValue = strLabel.substr(startData+1,(endData - startData)-1);

        // info item
        if (strType.Equals("INFO"))
        {
          int info = g_infoManager.TranslateString(strValue);
          if (info)
          {
            infoString = g_infoManager.GetLabel(info);
            emptyInfo = (infoString.length() == 0);
          }
          // just skip ahead if the parameter is bad
          lastPos = endData + 1;
          skip = endData - t;
        }

        // localized string
        else if (strType.Equals("LOCALIZE"))
        {
          int localize = atoi(strValue);
          if (localize && CUtil::IsNaturalNumber(strValue))
          {
            CStdString localizeString = g_localizeStrings.Get(localize);
            queueString.append(localizeString);
          }
          // just skip ahead if the parameter is bad
          lastPos = endData + 1;
          skip = endData - t;
        }
      }

      // figure out what to write out
			if (infoString.length() > 0)
			{
				// if the previous info item was valid (or if there was none), and this one also, 1st write all queued literal text
				if (queueString.length() > 0)
				{
					toString.append(queueString);
				}
				// empty literal text queue string
				queueString = L"";

				// now write info string
				toString.append(infoString);
				lastValid = true;
			}
			else if (emptyInfo)
			{
				queueString = L"";
        lastValid = false;
			}
      else  // must have had a $ without the necessary INFO() or LOCALIZE() informati
        queueString = L"";
      /*
			else
			{
				lastValid = false;
			}
      */
    }

    // escape character found, skip ahead one character
    else
    {
      skip = 1;
    }

    // are there anymore characters to test?
    if (t+skip < (int)strLabel.length())
    {
      // find the next parsable item
      t = strLabel.Find('$',t+skip + 1);
    }

    // no more characters
    else
    {
      // exit the while loop
      t = -1;
    }
  }

	// if any text was leftover after the last parsable tag, add it to the literal text queue
  if (lastPos < (int)strLabel.length())
  {
    CStdString tempString = strLabel.substr(lastPos,(int)strLabel.length()-lastPos);
    tempString.Replace("$$","$");
    queueString.append(tempString.c_str());
  }

	//if the last info item was valid, append the literal text queue to the result string
	if (lastValid && queueString.length() > 0)
	{
		toString.append(queueString.c_str());
	}

  return toString;
}

bool CGUIControl::UpdateVisibility()
{
  // update our alpha values if we're fading
  if (m_fadingState == FADING_IN)
  { // doing a fade in
    m_fadingPos--;
    if (!m_fadingPos)
    {
      m_fadingState = FADING_NONE;
    }
    DWORD fadeAmount = (DWORD)(m_fadingPos * 255.0f / m_fadingTime + 0.5f);
    SetAlpha(255 - fadeAmount);
  }
  else if (m_fadingState == FADING_OUT)
  {
    m_fadingPos--;
    if (!m_fadingPos)
    {
      m_fadingState = FADING_NONE;
      m_bVisible = false;
    }
    DWORD fadeAmount = (DWORD)(m_fadingPos * 255.0f / m_fadingTime + 0.5f);
    SetAlpha(fadeAmount);
  }
  return IsVisible();
}