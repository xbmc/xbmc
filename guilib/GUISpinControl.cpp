#include "stdafx.h"
#include "guispincontrol.h"
#include "guifontmanager.h"
#include "../xbmc/utils/CharsetConverter.h"

#define SPIN_BUTTON_DOWN 0
#define SPIN_BUTTON_UP   1
CGUISpinControl::CGUISpinControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strUp, const CStdString& strDown, const CStdString& strUpFocus, const CStdString& strDownFocus, const CStdString& strFont, DWORD dwTextColor, int iType, DWORD dwAlign)
        :CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
        ,m_imgspinUp(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight,strUp)
        ,m_imgspinDown(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight,strDown)
        ,m_imgspinUpFocus(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight,strUpFocus)
        ,m_imgspinDownFocus(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight,strDownFocus)
{
	m_fMaxTextWidth = 0;
	m_bReverse=false;
    m_iStart=0;
    m_iEnd=100;
    m_fStart=0.0f;
    m_fEnd=1.0f;
    m_fInterval=0.1f;
    m_iValue=0;
    m_dwAlign=dwAlign;
	m_dwAlignY=XBFONT_CENTER_Y;
    m_fValue=0.0;
    m_strFont=strFont;
    m_pFont=NULL;
    m_dwTextColor=dwTextColor;
	m_dwDisabledColor = 0xFF606060; 
    m_iType=iType;
    m_iSelect=SPIN_BUTTON_DOWN;
    m_bShowRange=false;
    m_iTypedPos=0;
    strcpy(m_szTyped,"");
m_dwBuddyControlID = 0;
	m_bBuddyDisabled = false;
	m_lTextOffsetX = 0;
	m_lTextOffsetY = 0;
	ControlType = GUICONTROL_SPIN;
}

void CGUISpinControl::SetNonProportional(bool bOnOff)
{
	m_fMaxTextWidth = 0;

	if ((bOnOff) && (m_pFont))
	{
		m_bShowRange = false;

	    CStdStringW strLabelUnicode;
        float fTextHeight,fTextWidth;

		for(int i=0;i<(int)m_vecLabels.size();i++)
		{
			g_charsetConverter.stringCharsetToFontCharset(m_vecLabels[m_iValue].c_str(), strLabelUnicode);
			m_pFont->GetTextExtent( strLabelUnicode.c_str(), &fTextWidth,&fTextHeight);
			
			if (fTextWidth>m_fMaxTextWidth)
			{
				m_fMaxTextWidth = fTextWidth;
			}
		}
	}
}

CGUISpinControl::~CGUISpinControl(void)
{}


void CGUISpinControl::OnAction(const CAction &action)
{
  switch (action.wID)
  {
    case REMOTE_0:
    case REMOTE_1:
    case REMOTE_2:
    case REMOTE_3:
    case REMOTE_4:
    case REMOTE_5:
    case REMOTE_6:
    case REMOTE_7:
    case REMOTE_8:
    case REMOTE_9:
    {
      if (strlen(m_szTyped) >= 3)
      {
        m_iTypedPos=0;
        strcpy(m_szTyped,"");
      }
      int iNumber = action.wID - REMOTE_0;
     
      m_szTyped[m_iTypedPos]=iNumber+'0';
      m_iTypedPos++;
      m_szTyped[m_iTypedPos]=0;
      int iValue;
      sscanf(m_szTyped,"%i", &iValue);
      switch (m_iType)
      {
        case SPIN_CONTROL_TYPE_INT:
        {
          if (iValue < m_iStart || iValue > m_iEnd)
          {
            m_iTypedPos=0;
            m_szTyped[m_iTypedPos]=iNumber+'0';
            m_iTypedPos++;
            m_szTyped[m_iTypedPos]=0;
            sscanf(m_szTyped,"%i", &iValue);
            if (iValue < m_iStart || iValue > m_iEnd)
            {
              m_iTypedPos=0;
              strcpy(m_szTyped,"");
              return;
            }
          }
          m_iValue=iValue;
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
          g_graphicsContext.SendMessage(msg);
        }  
        break;

        case SPIN_CONTROL_TYPE_TEXT:
        {
          if (iValue < 0|| iValue >= (int)m_vecLabels.size())
          {
            m_iTypedPos=0;
            m_szTyped[m_iTypedPos]=iNumber+'0';
            m_iTypedPos++;
            m_szTyped[m_iTypedPos]=0;
            sscanf(m_szTyped,"%i", &iValue);
            if (iValue < 0|| iValue >= (int)m_vecLabels.size())
            {
              m_iTypedPos=0;
              strcpy(m_szTyped,"");
              return;
            }
          }
          m_iValue=iValue;
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
          g_graphicsContext.SendMessage(msg);
        }  
        break;

      }
    }
    break;
  }
    if (action.wID == ACTION_PAGE_UP)
    {
      if (!m_bReverse)
          PageDown();
      else
          PageUp();
      return;
    }
    if (action.wID == ACTION_PAGE_DOWN)
    {
      if (!m_bReverse)
          PageUp();
      else
          PageDown();
      return;
    }
    if (action.wID == ACTION_SELECT_ITEM)
    {
        if (m_iSelect==SPIN_BUTTON_UP)
        {
            MoveUp();
            return;
        }
        if (m_iSelect==SPIN_BUTTON_DOWN)
        {
            MoveDown();
            return;
        }
    }
    CGUIControl::OnAction(action);
}

void CGUISpinControl::OnLeft()
{
    if (m_iSelect==SPIN_BUTTON_UP)
    {
		if (CanMoveDown())
		{	// select the down button
            m_iSelect=SPIN_BUTTON_DOWN;
		}
    }
	else
	{	// base class
		CGUIControl::OnLeft();
	}
}
void CGUISpinControl::OnRight()
{
	if (m_iSelect==SPIN_BUTTON_DOWN)
    {
        if (CanMoveUp())
        {	// select the up button
            m_iSelect=SPIN_BUTTON_UP;
		}
	}
	else
	{	// base class
		CGUIControl::OnRight();
	}
}

void CGUISpinControl::Clear()
{
    m_vecLabels.erase(m_vecLabels.begin(),m_vecLabels.end());
	m_vecValues.erase(m_vecValues.begin(),m_vecValues.end());
	SetValue(0);
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
				Clear();
                return true;
            }
            break;

        case GUI_MSG_SHOWRANGE:
            if (message.GetParam1() )
                m_bShowRange=true;
            else
                m_bShowRange=false;
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
                message.SetParam2(m_iSelect);

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

void CGUISpinControl::PreAllocResources()
{
	CGUIControl::PreAllocResources();
	m_imgspinUp.PreAllocResources();
	m_imgspinUpFocus.PreAllocResources();
	m_imgspinDown.PreAllocResources();
	m_imgspinDownFocus.PreAllocResources();
}

void CGUISpinControl::AllocResources()
{
    CGUIControl::AllocResources();
    m_imgspinUp.AllocResources();
    m_imgspinUpFocus.AllocResources();
    m_imgspinDown.AllocResources();
    m_imgspinDownFocus.AllocResources();

    m_pFont=g_fontManager.GetFont(m_strFont);
    SetPosition(m_iPosX, m_iPosY);

	if (m_dwBuddyControlID)	// do we have an associated label control?
	{
		// set it to disabled by default (i.e. no focus)
		CGUIMessage msg(GUI_MSG_DISABLED, GetID(), m_dwBuddyControlID, 0);
		g_graphicsContext.SendMessage(msg);
		m_bBuddyDisabled = true;
	}
}

void CGUISpinControl::FreeResources()
{
    CGUIControl::FreeResources();
    m_imgspinUp.FreeResources();
    m_imgspinUpFocus.FreeResources();
    m_imgspinDown.FreeResources();
    m_imgspinDownFocus.FreeResources();
    m_iTypedPos=0;
    strcpy(m_szTyped,"");
}



void CGUISpinControl::Render()
{
    if (!IsVisible())
    {
      m_iTypedPos=0;
      strcpy(m_szTyped,"");
      return;
    }
    if (!HasFocus())
    {
      m_iTypedPos=0;
      strcpy(m_szTyped,"");
    }

	if (m_dwBuddyControlID)		// do we have an associated label control?
	{
		if (HasFocus())			// we currently have focus
		{
			if (m_bBuddyDisabled)	// our associated label is currently disabled
			{
				// make it enabled
				CGUIMessage msg(GUI_MSG_ENABLED, GetID(), m_dwBuddyControlID, 0);
				g_graphicsContext.SendMessage(msg);
				m_bBuddyDisabled = false;
			}
		}
		else					// we do not have focus
		{
			if (!m_bBuddyDisabled)	// our associated label is current enabled
			{
				// make it disabled
				CGUIMessage msg(GUI_MSG_DISABLED, GetID(), m_dwBuddyControlID, 0);
				g_graphicsContext.SendMessage(msg);
				m_bBuddyDisabled = true;
			}
		}
	}

    int iPosX=m_iPosX;
    WCHAR wszText[1024];
	CStdStringW strTextUnicode;

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
        else swprintf(wszText,L"?%i?",m_iValue);
        
    }

	g_charsetConverter.stringCharsetToFontCharset(wszText, strTextUnicode);

	// Calculate the size of our text (for use in HitTest)
	float fTextWidth, fTextHeight;
	if (m_pFont)
		m_pFont->GetTextExtent( strTextUnicode.c_str(), &fTextWidth, &fTextHeight);
	// Position the arrows
	if (m_fMaxTextWidth>0)
	{
		m_imgspinUpFocus.SetPosition((int)m_fMaxTextWidth + 5+iPosX+ m_imgspinDown.GetWidth(), m_iPosY);
		m_imgspinUp.SetPosition((int)m_fMaxTextWidth + 5+iPosX+ m_imgspinDown.GetWidth(), m_iPosY);
		m_imgspinDownFocus.SetPosition((int)m_fMaxTextWidth + 5+iPosX, m_iPosY);
		m_imgspinDown.SetPosition((int)m_fMaxTextWidth + 5+iPosX, m_iPosY);
	}
  else if (( m_dwAlign== XBFONT_LEFT) && (m_pFont))
  {
		m_imgspinUpFocus.SetPosition((int)fTextWidth + 5+iPosX+ m_imgspinDown.GetWidth(), m_iPosY);
		m_imgspinUp.SetPosition((int)fTextWidth + 5+iPosX+ m_imgspinDown.GetWidth(), m_iPosY);
		m_imgspinDownFocus.SetPosition((int)fTextWidth + 5+iPosX, m_iPosY);
		m_imgspinDown.SetPosition((int)fTextWidth + 5+iPosX, m_iPosY);
  }

  if (m_iSelect==SPIN_BUTTON_UP && !CanMoveUp())
  {
		m_iSelect=SPIN_BUTTON_DOWN;
  }

  if (m_iSelect==SPIN_BUTTON_DOWN && !CanMoveDown())
  {
		m_iSelect=SPIN_BUTTON_UP;
  }

  if ( HasFocus() )
  {
		if (m_iSelect==SPIN_BUTTON_UP && CanMoveUp())
			m_imgspinUpFocus.Render();
		else
			m_imgspinUp.Render();

		if (m_iSelect==SPIN_BUTTON_DOWN && CanMoveDown())
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
		float fPosY;
		if (m_dwAlignY==XBFONT_CENTER_Y)
		{
			fPosY = ((float)m_dwHeight)/2.0f;
			fPosY-= fTextHeight/2.0f;
			fPosY+=(float)m_iPosY;
		}
		else
		{
			fPosY=(float)(m_iPosY+m_lTextOffsetY);
		}

		float fPosX = (float)(m_iPosX+m_lTextOffsetX) -3;
		if ( HasFocus() )
		{
			m_pFont->DrawText(fPosX, fPosY, m_dwTextColor,strTextUnicode.c_str(),m_dwAlign);
		}
		else
		{
			m_pFont->DrawText(fPosX, fPosY, m_dwDisabledColor,strTextUnicode.c_str(),m_dwAlign);
		}
		// set our hit rectangle for MouseOver events
		if (m_dwAlign & XBFONT_LEFT)
			m_rectHit.SetRect((int)fPosX, (int)fPosY, (int) fTextWidth, (int) fTextHeight);
		else
			m_rectHit.SetRect((int)(fPosX-fTextWidth), (int)fPosY, (int) fTextWidth, (int) fTextHeight);
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

void CGUISpinControl::AddLabel(CStdString aLabel, int iValue)
{
    WCHAR wszText[1024];
	swprintf(wszText,L"%S",aLabel.c_str());	
	wstring strLabel = wszText;
	m_vecLabels.push_back(strLabel);
    m_vecValues.push_back(iValue);
 }


const WCHAR* CGUISpinControl::GetLabel() const
{
    if (m_iValue >=0 && m_iValue < (int)m_vecLabels.size())
        return L"";
    const wstring strLabel=m_vecLabels[ m_iValue];
    return strLabel.c_str();
}

void CGUISpinControl::SetPosition(int iPosX, int iPosY)
{
    CGUIControl::SetPosition(iPosX, iPosY);

    m_imgspinDownFocus.SetPosition(iPosX, iPosY);
    m_imgspinDown.SetPosition(iPosX, iPosY);

    m_imgspinUp.SetPosition(m_iPosX + m_imgspinDown.GetWidth(),m_iPosY);
    m_imgspinUpFocus.SetPosition(m_iPosX + m_imgspinDownFocus.GetWidth(),m_iPosY);

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

bool CGUISpinControl::CanMoveUp(bool bTestReverse)
{
	// test for reverse...
	if (bTestReverse && m_bReverse) return CanMoveDown(false);

    switch (m_iType)
    {
    case SPIN_CONTROL_TYPE_INT:
        {
            if (m_iValue-1 >= m_iStart)
                return true;
            return false;
        }
        break;

    case SPIN_CONTROL_TYPE_FLOAT:
        {
            if (m_fValue-m_fInterval >= m_fStart)
                return true;
            return false;
        }
        break;

    case SPIN_CONTROL_TYPE_TEXT:
        {
            if (m_iValue-1 >= 0)
                return true;
            return false;
        }
        break;
    }
    return false;
}

bool CGUISpinControl::CanMoveDown(bool bTestReverse)
{
	// test for reverse...
	if (bTestReverse && m_bReverse) return CanMoveUp(false);
    switch (m_iType)
    {
    case SPIN_CONTROL_TYPE_INT:
        {
            if (m_iValue+1 <= m_iEnd)
                return true;
            return false;
        }
        break;

    case SPIN_CONTROL_TYPE_FLOAT:
        {
            if (m_fValue+m_fInterval <= m_fEnd)
                return true;
            return false;
        }
        break;

    case SPIN_CONTROL_TYPE_TEXT:
        {
            if (m_iValue+1 < (int)m_vecLabels.size())
                return true;
            return false;
        }
        break;
    }
    return false;
}

void CGUISpinControl::PageUp()
{
    switch (m_iType)
    {
        case SPIN_CONTROL_TYPE_INT:
        {
            if (m_iValue-10 >= m_iStart)
                m_iValue-=10;
            CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
            g_graphicsContext.SendMessage(msg);
            return;
        }
        break;
    case SPIN_CONTROL_TYPE_TEXT:
        {
            if (m_iValue-10 >= 0)
                m_iValue-=10;
            CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
            g_graphicsContext.SendMessage(msg);
            return;
        }
        break;
    }

}

void CGUISpinControl::PageDown()
{
    switch (m_iType)
    {
        case SPIN_CONTROL_TYPE_INT:
        {
            if (m_iValue+10 <= m_iEnd)
                m_iValue+=10;
            CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
            g_graphicsContext.SendMessage(msg);
            return;
        }
        break;
        case SPIN_CONTROL_TYPE_TEXT:
        {
            if (m_iValue+10 < (int)m_vecLabels.size() )
                m_iValue+=10;
            CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
            g_graphicsContext.SendMessage(msg);
            return;
        }
        break;
    }
}

void CGUISpinControl::MoveUp(bool bTestReverse)
{
	if (bTestReverse && m_bReverse)
	{	// actually should move down.
		MoveDown(false);
		return;
	}
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

void CGUISpinControl::MoveDown(bool bTestReverse)
{
	if (bTestReverse && m_bReverse)
	{	// actually should move up.
		MoveUp(false);
		return;
	}
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

void CGUISpinControl::SetDisabledColor(D3DCOLOR color)
{
	m_dwDisabledColor=color;
}

int CGUISpinControl::GetMinimum() const
{
  switch (m_iType)
  {
    case SPIN_CONTROL_TYPE_INT:
      return m_iStart;
    break;
    
    case SPIN_CONTROL_TYPE_TEXT:
      return 1;
    break;

    case SPIN_CONTROL_TYPE_FLOAT:
      return (int)(m_fStart*10.0f);
    break;
  }
  return 0;
}

int CGUISpinControl::GetMaximum() const
{
  switch (m_iType)
  {
    case SPIN_CONTROL_TYPE_INT:
      return m_iEnd;
    break;
    
    case SPIN_CONTROL_TYPE_TEXT:
      return (int)m_vecLabels.size();
    break;

    case SPIN_CONTROL_TYPE_FLOAT:
      return (int)(m_fEnd*10.0f);
    break;
  }
  return 100;
}
void CGUISpinControl::SetBuddyControlID(DWORD dwBuddyControlID)
{
	m_dwBuddyControlID = dwBuddyControlID;
	return;
}

bool CGUISpinControl::HitTest(int iPosX, int iPosY) const
{
	if (m_imgspinUpFocus.HitTest(iPosX, iPosY) || m_imgspinDownFocus.HitTest(iPosX, iPosY))
		return true;
	// check if we have the text bit selected...
	return m_rectHit.PtInRect(iPosX, iPosY);
}

void CGUISpinControl::OnMouseOver()
{
	if (m_imgspinUpFocus.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
	{
		if (CanMoveUp()) CGUIControl::OnMouseOver();
		m_iSelect = SPIN_BUTTON_UP;
	}
	else if (m_imgspinDownFocus.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
	{
		if (CanMoveDown()) CGUIControl::OnMouseOver();
		m_iSelect = SPIN_BUTTON_DOWN;
	}
	else
	{
		CGUIControl::OnMouseOver();
		m_iSelect = SPIN_BUTTON_UP;
	}
}

void CGUISpinControl::OnMouseClick(DWORD dwButton)
{	// only left button handled
	if (dwButton != MOUSE_LEFT_BUTTON) return;
	if (m_imgspinUpFocus.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
	{
		MoveUp();
	}
	if (m_imgspinDownFocus.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
	{
		MoveDown();
	}
}

void CGUISpinControl::OnMouseWheel()
{
	for (int i=0; i<abs(g_Mouse.cWheel); i++)
	{
		if (g_Mouse.cWheel > 0)
		{
			MoveUp();
		}
		else
		{
			MoveDown();
		}
	}
}