#include "guispincontrol.h"
#include "guifontmanager.h"

#define SPIN_BUTTON_DOWN 0
#define SPIN_BUTTON_UP   1
CGUISpinControl::CGUISpinControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strUp, const CStdString& strDown, const CStdString& strUpFocus, const CStdString& strDownFocus, const CStdString& strFont, DWORD dwTextColor, int iType, DWORD dwAlign)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight)
,m_imgspinUp(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strUp)
,m_imgspinDown(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strDown)
,m_imgspinUpFocus(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strUpFocus)
,m_imgspinDownFocus(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight,strDownFocus)
{
	m_bReverse=false;
  m_iStart=0;
  m_iEnd=100;
  m_fStart=0.0f;
  m_fEnd=1.0f;
  m_fInterval=0.1f;
  m_iValue=0;
	m_dwAlign=dwAlign;
  m_fValue=0.0;
  m_strFont=strFont;
  m_pFont=NULL;
  m_dwTextColor=dwTextColor;
  m_iType=iType;
  m_iSelect=SPIN_BUTTON_DOWN;
  m_bShowRange=false;
}

CGUISpinControl::~CGUISpinControl(void)
{
}


void CGUISpinControl::OnAction(const CAction &action)
{
    if (action.wID == ACTION_MOVE_LEFT)
    {
		if (m_iSelect==SPIN_BUTTON_UP) 
		{
					if (m_bReverse)
					{
						if (CanMoveUp() )
							m_iSelect=SPIN_BUTTON_DOWN;
					}
					else
					{
						if (CanMoveDown() )
							m_iSelect=SPIN_BUTTON_DOWN;
					}
			return;
		}
    }
    if (action.wID == ACTION_MOVE_RIGHT)
    {
		if (m_iSelect==SPIN_BUTTON_DOWN) 
		{
					if (m_bReverse)
					{
						if (CanMoveDown() )
							m_iSelect=SPIN_BUTTON_UP;
					}
					else
					{
						if (CanMoveUp() )
							m_iSelect=SPIN_BUTTON_UP;
					}
			return;
		}
    }
    if (action.wID == ACTION_SELECT_ITEM)
    {
		if (m_iSelect==SPIN_BUTTON_UP)
		{
			if (m_bReverse)
				MoveDown();
			else
				MoveUp();
			return;
		}
		if (m_iSelect==SPIN_BUTTON_DOWN)
		{
			if (m_bReverse)
				MoveUp();
			else
				MoveDown();
			return;
		}
    }
	CGUIControl::OnAction(action);
}


bool CGUISpinControl::OnMessage(CGUIMessage& message)
{

	if (CGUIControl::OnMessage(message) ) 
  {
    if (!HasFocus())
			m_iSelect=SPIN_BUTTON_DOWN;
		else
			m_iSelect = message.GetParam1() == SPIN_BUTTON_UP ? SPIN_BUTTON_UP : SPIN_BUTTON_DOWN;
    return true;
  }
  if (message.GetControlId() == GetID() )
  {
    switch (message.GetMessage())
    {
			case GUI_MSG_ITEM_SELECT:
				SetValue( message.GetParam1());
				return true;
			break;
			
			case GUI_MSG_LABEL_RESET:
			{
				m_vecLabels.erase(m_vecLabels.begin(),m_vecLabels.end());
				m_vecValues.erase(m_vecValues.begin(),m_vecValues.end());
				SetValue(0);
        return true;
			}
      break;

      case GUI_MSG_SHOWRANGE:
        if (message.GetParam1() ) m_bShowRange=true;
        else  m_bShowRange=false;
      break;

      case GUI_MSG_LABEL_ADD:
      {
        AddLabel(message.GetLabel(), message.GetParam1());
        return true;
      }
      break;

			case GUI_MSG_ITEM_SELECTED:
			{
				message.SetParam1( GetValue() );

				if (m_iType==SPIN_CONTROL_TYPE_TEXT)
				{
					if ( m_iValue>= 0 && m_iValue < (int)m_vecLabels.size() )
						message.SetLabel( m_vecLabels[m_iValue]);
					
					if ( m_iValue>= 0 && m_iValue < (int)m_vecValues.size() )
						message.SetParam1(m_vecValues[m_iValue]);
				}
				return true;
			}


    }
  }
	return false;
}


void CGUISpinControl::AllocResources()
{
	CGUIControl::AllocResources();
	m_imgspinUp.AllocResources();
	m_imgspinUpFocus.AllocResources();
	m_imgspinDown.AllocResources();
	m_imgspinDownFocus.AllocResources();

  m_pFont=g_fontManager.GetFont(m_strFont);
  SetPosition(m_dwPosX, m_dwPosY);
}

void CGUISpinControl::FreeResources()
{
	CGUIControl::FreeResources();
	m_imgspinUp.FreeResources();
	m_imgspinUpFocus.FreeResources();
	m_imgspinDown.FreeResources();
	m_imgspinDownFocus.FreeResources();
}



void CGUISpinControl::Render()
{
	if (!IsVisible()) return;
	DWORD dwPosX=m_dwPosX;
	WCHAR wszText[1024];
	
	if (m_iType == SPIN_CONTROL_TYPE_INT)
		swprintf(wszText,L"%i/%i",m_iValue, m_iEnd);
	else if (m_iType==SPIN_CONTROL_TYPE_FLOAT)
		swprintf(wszText,L"%02.2f/%02.2f",m_fValue, m_fEnd);
	else
	{
		swprintf(wszText,L"");
		if (m_iValue < (int)m_vecLabels.size() )
    {
      if (m_bShowRange)
      {
        swprintf(wszText,L"(%i/%i) %s", m_iValue+1,(int)m_vecLabels.size(),m_vecLabels[m_iValue].c_str() );
      }
			else
      {
        swprintf(wszText,L"%s", m_vecLabels[m_iValue].c_str() );
      }
    }
	}

	if ( m_dwAlign== XBFONT_LEFT)
	{
		if (m_pFont)
		{
			float fTextHeight,fTextWidth;
			m_pFont->GetTextExtent( wszText, &fTextWidth,&fTextHeight);
			m_imgspinUpFocus.SetPosition((DWORD)fTextWidth + 5+dwPosX+ m_imgspinDown.GetWidth(), m_dwPosY);
			m_imgspinUp.SetPosition((DWORD)fTextWidth + 5+dwPosX+ m_imgspinDown.GetWidth(), m_dwPosY);
			m_imgspinDownFocus.SetPosition((DWORD)fTextWidth + 5+dwPosX, m_dwPosY);
			m_imgspinDown.SetPosition((DWORD)fTextWidth + 5+dwPosX, m_dwPosY);
		}
	}
	if (m_iSelect==SPIN_BUTTON_UP) 
	{
		if (m_bReverse)
		{
			if ( !CanMoveDown() )
				m_iSelect=SPIN_BUTTON_DOWN;
		}
		else
		{
			if ( !CanMoveUp() )
				m_iSelect=SPIN_BUTTON_DOWN;
		}
	}

	if (m_iSelect==SPIN_BUTTON_DOWN) 
	{
		if (m_bReverse)
		{
			if ( !CanMoveUp() )
				m_iSelect=SPIN_BUTTON_UP;
		}
		else
		{
			if ( !CanMoveDown() )
				m_iSelect=SPIN_BUTTON_UP;
		}
	}

	if ( HasFocus() )
	{
		bool bShow=CanMoveUp();
		if (m_bReverse) bShow = CanMoveDown();

		if (m_iSelect==SPIN_BUTTON_UP && bShow ) 
      m_imgspinUpFocus.Render();
    else 
      m_imgspinUp.Render();

		bShow=CanMoveDown();
		if (m_bReverse) bShow = CanMoveUp();
    if (m_iSelect==SPIN_BUTTON_DOWN && bShow) 
		  m_imgspinDownFocus.Render();
    else
      m_imgspinDown.Render();
	}
	else
	{
		m_imgspinUp.Render();
		m_imgspinDown.Render();
	}

	if (m_pFont)
	{
	
    float fWidth,fHeight;
    m_pFont->GetTextExtent( wszText, &fWidth,&fHeight);
    fHeight/=2;
    float fPosY = (float)m_dwHeight/2;
    fPosY-=fHeight;
    fPosY+=(float)m_dwPosY;

    
    m_pFont->DrawText((float)m_dwPosX-3, (float)fPosY,m_dwTextColor,wszText,m_dwAlign);
	}



}



void CGUISpinControl::SetRange(int iStart, int iEnd)
{
  m_iStart=iStart;
  m_iEnd=iEnd;
}


void CGUISpinControl::SetFloatRange(float fStart, float fEnd)
{
  m_fStart=fStart;
  m_fEnd=fEnd;
}

void CGUISpinControl::SetValue(int iValue)
{
  m_iValue=iValue;
}

void CGUISpinControl::SetFloatValue(float fValue)
{
  m_fValue=fValue;
}

int  CGUISpinControl::GetValue() const
{
  return m_iValue;
}

float CGUISpinControl::GetFloatValue() const
{
  return m_fValue;
}


void CGUISpinControl::AddLabel(const wstring& strLabel, int iValue)
{
  m_vecLabels.push_back(strLabel);
	m_vecValues.push_back(iValue);
}

const WCHAR* CGUISpinControl::GetLabel() const
{
	if (m_iValue >=0 && m_iValue < (int)m_vecLabels.size()) return L"";
	const wstring strLabel=m_vecLabels[ m_iValue];
  return strLabel.c_str();
}

void CGUISpinControl::SetPosition(DWORD dwPosX, DWORD dwPosY)
{
  CGUIControl::SetPosition(dwPosX, dwPosY);

  m_imgspinDownFocus.SetPosition(dwPosX, dwPosY);
  m_imgspinDown.SetPosition(dwPosX, dwPosY);

  m_imgspinUp.SetPosition(m_dwPosX + m_imgspinDown.GetWidth(),m_dwPosY);
  m_imgspinUpFocus.SetPosition(m_dwPosX + m_imgspinDownFocus.GetWidth(),m_dwPosY);

}

DWORD CGUISpinControl::GetWidth() const
{
  return m_imgspinDown.GetWidth() * 2 ;
}

void CGUISpinControl::SetFocus(bool bOnOff)
{
  CGUIControl::SetFocus(bOnOff);
  m_iSelect=SPIN_BUTTON_DOWN;
}

bool CGUISpinControl::CanMoveUp()
{
	switch (m_iType)
	{
		case SPIN_CONTROL_TYPE_INT:
		{
			if (m_iValue-1 >= m_iStart) return true;
			return false;
		}
		break;

		case SPIN_CONTROL_TYPE_FLOAT:
		{
			if (m_fValue-m_fInterval >= m_fStart) return true;
			return false;
		}
		break;

		case SPIN_CONTROL_TYPE_TEXT:
		{
			if (m_iValue-1 >= 0) return true;
			return false;
		}
		break;
	}
	return false;
}

bool CGUISpinControl::CanMoveDown()
{
	switch (m_iType)
	{
		case SPIN_CONTROL_TYPE_INT:
		{
			if (m_iValue+1 <= m_iEnd) return true;
			return false;
		}
		break;

	case SPIN_CONTROL_TYPE_FLOAT:
		{
			if (m_fValue+m_fInterval <= m_fEnd) return true;
			return false;
		}
		break;

	case SPIN_CONTROL_TYPE_TEXT:
		{
			if (m_iValue+1 < (int)m_vecLabels.size()) return true;
			return false;
		}
		break;
	}
	return false;
}

void CGUISpinControl::MoveUp()
{
	switch (m_iType)
	{
		case SPIN_CONTROL_TYPE_INT:
		{
			if (m_iValue-1 >= m_iStart)
				m_iValue--;
			CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
			g_graphicsContext.SendMessage(msg);
			return;
		}
		break;

	case SPIN_CONTROL_TYPE_FLOAT:
		{
			if (m_fValue-m_fInterval >= m_fStart)
				m_fValue-=m_fInterval;
			CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
			g_graphicsContext.SendMessage(msg);
			return;
		}
		break;

	case SPIN_CONTROL_TYPE_TEXT:
		{
			if (m_iValue-1 >= 0)
				m_iValue--;
			CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
			g_graphicsContext.SendMessage(msg);
			return;
		}
		break;
	}
}

void CGUISpinControl::MoveDown()
{
	switch (m_iType)
	{
		case SPIN_CONTROL_TYPE_INT:
		{
			if (m_iValue+1 <= m_iEnd)
				m_iValue++;
			CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
			g_graphicsContext.SendMessage(msg);
			return;
		}
		break;

		case SPIN_CONTROL_TYPE_FLOAT:
		{
			if (m_fValue+m_fInterval <= m_fEnd)
				m_fValue+=m_fInterval;
			CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
			g_graphicsContext.SendMessage(msg);
			return;
		}
		break;

		case SPIN_CONTROL_TYPE_TEXT:
		{
				if (m_iValue+1 < (int)m_vecLabels.size() )
					m_iValue++;
			CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
			g_graphicsContext.SendMessage(msg);
			return;
		}
		break;
	}
}
void CGUISpinControl::SetReverse(bool bReverse)
{
	m_bReverse=bReverse;
}

void CGUISpinControl::SetFloatInterval(float fInterval)
{
  m_fInterval=fInterval;
}

float CGUISpinControl::GetFloatInterval() const
{
  return m_fInterval;
}

bool CGUISpinControl::GetShowRange() const
{
  return m_bShowRange;
}
void CGUISpinControl::SetShowRange(bool bOnoff) 
{
  m_bShowRange=bOnoff;
}
