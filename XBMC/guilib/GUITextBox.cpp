#include "stdafx.h"
#include "guitextbox.h"
#include "guifontmanager.h"

#define CONTROL_LIST		0
#define CONTROL_UPDOWN	1
CGUITextBox::CGUITextBox(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, 
                                 const CStdString& strFontName, 
                                 DWORD dwSpinWidth,DWORD dwSpinHeight,
                                 const CStdString& strUp, const CStdString& strDown, 
                                 const CStdString& strUpFocus, const CStdString& strDownFocus, 
                                 DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                                 const CStdString& strFont, DWORD dwTextColor)
:CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
,m_upDown(dwControlId, 0, dwSpinX, dwSpinY, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, strFont, dwSpinColor, SPIN_CONTROL_TYPE_INT)
{
  m_iOffset=0;
  m_iItemsPerPage=10;
  m_iItemHeight=10;
	m_iMaxPages=50;
  m_pFont=g_fontManager.GetFont(strFontName);
  m_dwTextColor=dwTextColor;
	ControlType = GUICONTROL_TEXTBOX;
}

CGUITextBox::~CGUITextBox(void)
{
}

void CGUITextBox::Render()
{
  if (!m_pFont) return;
  if (!IsVisible()) return;
  CGUIControl::Render();
  int iPosY=m_iPosY;
	
  for (int i=0; i < m_iItemsPerPage; i++)
  {
		int iPosX=m_iPosX;
    if (i+m_iOffset < (int)m_vecItems.size() )
    {
      // render item
			CGUIListItem& item=m_vecItems[i+m_iOffset];
			CStdString strLabel1=item.GetLabel();
			CStdString strLabel2=item.GetLabel2();

			WCHAR wszText1[1024];
			swprintf(wszText1,L"%S", strLabel1.c_str() );
			DWORD dMaxWidth=m_dwWidth+16;
			if (strLabel2.size())
			{
				WCHAR wszText2[1024];
				float fTextWidth,fTextHeight;
				swprintf(wszText2,L"%S", strLabel2.c_str() );
				m_pFont->GetTextExtent( wszText2, &fTextWidth,&fTextHeight);
				dMaxWidth -= (DWORD)(fTextWidth);

				m_pFont->DrawTextWidth((float)iPosX+dMaxWidth, (float)iPosY+2, m_dwTextColor,wszText2,(float)fTextWidth);
			}
			m_pFont->DrawTextWidth((float)iPosX, (float)iPosY+2, m_dwTextColor,wszText1,(float)dMaxWidth);
      iPosY += (DWORD)m_iItemHeight;
    }
  }
	m_upDown.Render();
}

void CGUITextBox::OnAction(const CAction &action)
{
  switch (action.wID)
  {
    case ACTION_PAGE_UP:
		OnPageUp();
    break;

    case ACTION_PAGE_DOWN:
		OnPageDown();
    break;

	case ACTION_MOVE_UP:
	case ACTION_MOVE_DOWN:
	case ACTION_MOVE_LEFT:
	case ACTION_MOVE_RIGHT:
		CGUIControl::OnAction(action);
	break;

	default:
        m_upDown.OnAction(action);
  }
}

bool CGUITextBox::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId()==0)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
      }
    }
    if (message.GetMessage() == GUI_MSG_LABEL2_SET)
    {
			int iItem=message.GetParam1();
			if (iItem >=0 && iItem < (int)m_vecItems.size())
			{
				CGUIListItem& item=m_vecItems[iItem];
				item.SetLabel2( message.GetLabel() );
			}
		}

    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_iOffset=0;
      m_vecItems.erase(m_vecItems.begin(),m_vecItems.end());
      m_upDown.SetRange(1,1);
      m_upDown.SetValue(1);

			// set max pages (param1)
			if (message.GetParam1() > 0) m_iMaxPages = message.GetParam1();
			SetText( message.GetLabel() );
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_iOffset=0;
      m_vecItems.erase(m_vecItems.begin(),m_vecItems.end());
      m_upDown.SetRange(1,1);
      m_upDown.SetValue(1);
    }

		if (message.GetMessage() == GUI_MSG_SETFOCUS)
		{
			m_upDown.SetFocus(true);
		}
		if (message.GetMessage() == GUI_MSG_LOSTFOCUS)
		{
			m_upDown.SetFocus(false);
		}
  }

  if ( CGUIControl::OnMessage(message) ) return true;

  return false;
}

void CGUITextBox::PreAllocResources()
{
	if (!m_pFont) return;
	CGUIControl::PreAllocResources();
	m_upDown.PreAllocResources();
}

void CGUITextBox::AllocResources()
{
  if (!m_pFont) return;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();
  float fWidth,fHeight;
  
  m_pFont->GetTextExtent( L"y", &fWidth,&fHeight);
  //fHeight+=10.0f;

  //fHeight+=2;
  m_iItemHeight			= (int)fHeight;
  float fTotalHeight= (float)(m_dwHeight-m_upDown.GetHeight()-5);
  m_iItemsPerPage		= (int)(fTotalHeight / fHeight);

  int iPages=m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage) iPages++;
  m_upDown.SetRange(1,iPages);
  m_upDown.SetValue(1);

}

void CGUITextBox::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
}

void CGUITextBox::OnRight()
{
  m_upDown.OnRight();
  if (!m_upDown.HasFocus()) 
  {
    CGUIControl::OnRight();
  }
}

void CGUITextBox::OnLeft()
{
  m_upDown.OnLeft();
  if (!m_upDown.HasFocus()) 
  {
    CGUIControl::OnLeft();
  }
}

void CGUITextBox::OnUp()
{
  m_upDown.OnUp();
  if (!m_upDown.HasFocus()) 
  {
    CGUIControl::OnUp();
  }
}

void CGUITextBox::OnDown()
{
  m_upDown.OnDown();
  if (!m_upDown.HasFocus()) 
  {
    CGUIControl::OnDown();
  }  
}

void CGUITextBox::OnPageUp()
{
  int iPage = m_upDown.GetValue();
  if (iPage > 1)
  {
    iPage--;
    m_upDown.SetValue(iPage);
    m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
  }
}

void CGUITextBox::OnPageDown()
{
  int iPages=m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage) iPages++;

  int iPage = m_upDown.GetValue();
  if (iPage+1 <= iPages)
  {
    iPage++;
    m_upDown.SetValue(iPage);
    m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
  }
}
void CGUITextBox::SetText(const wstring &strText)
{
	m_vecItems.erase(m_vecItems.begin(),m_vecItems.end());
	// start wordwrapping
  // Set a flag so we can determine initial justification effects
  BOOL bStartingNewLine = TRUE;
	BOOL bBreakAtSpace = FALSE;
  int pos=0;
	int iTextSize = (int)strText.size();
	int lpos=0;
	int iLastSpace=-1;
	int iLastSpaceInLine=-1;
	char szLine[1024];
	WCHAR wsTmp[1024];
	int iTotalLines = 0;
  while(pos < iTextSize)
  {
    // Get the current letter in the string
    char letter = (char)strText[pos];

		// break if we get more pages then maxpages
		if (((iTotalLines + 1) / m_iItemsPerPage) > (m_iMaxPages - 1)) break;

    // Handle the newline character
    if (letter == '\n' )
    {
			CGUIListItem item(szLine);
			m_vecItems.push_back(item);
			iTotalLines++;
			iLastSpace=-1;
			iLastSpaceInLine=-1;
			lpos=0;
    }
		else
		{
			if (letter==' ') 
			{
				iLastSpace=pos;
				iLastSpaceInLine=lpos;
			}

			if (lpos < 0 || lpos >1023)
			{
				OutputDebugString("CGUITextBox::SetText -> ERRROR\n");
			}
			szLine[lpos]=letter;
			szLine[lpos+1]=0;

			FLOAT fwidth,fheight;
			swprintf(wsTmp,L"%S",szLine);
			m_pFont->GetTextExtent(wsTmp,&fwidth,&fheight);
			if (fwidth > m_dwWidth)
			{
				if (iLastSpace > 0 && iLastSpaceInLine != lpos)
				{
					szLine[iLastSpaceInLine]=0;
					pos=iLastSpace;
				}
				CGUIListItem item(szLine);
				m_vecItems.push_back(item);
				iTotalLines++;
				iLastSpaceInLine=-1;
				iLastSpace=-1;
				lpos=0;
			}
			else
			{
				lpos++;
			}
		}
		pos++;
	}

	if (lpos > 0)
	{
		CGUIListItem item(szLine);
		m_vecItems.push_back(item);
		iTotalLines++;
	}

  int iPages=m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage) iPages++;
  m_upDown.SetRange(1,iPages);
  m_upDown.SetValue(1);

}

bool CGUITextBox::HitTest(int iPosX, int iPosY) const
{
	if (m_upDown.HitTest(iPosX, iPosY)) return true;
	return CGUIControl::HitTest(iPosX, iPosY);
}

void CGUITextBox::OnMouseOver()
{
	if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
		m_upDown.OnMouseOver();
	CGUIControl::OnMouseOver();
}

void CGUITextBox::OnMouseClick(DWORD dwButton)
{
	if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
		m_upDown.OnMouseClick(dwButton);
}

void CGUITextBox::OnMouseWheel()
{
	if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
	{
		m_upDown.OnMouseWheel();
	}
	else
	{	// increase or decrease our offset by the appropriate amount.
		m_iOffset -= g_Mouse.cWheel;
		// check that we are within the correct bounds.
		if (m_iOffset + m_iItemsPerPage > (int)m_vecItems.size())
			m_iOffset = m_vecItems.size() - m_iItemsPerPage;
		if (m_iOffset<0) m_iOffset = 0;
		// update the page control...
		int iPage=m_iOffset / m_iItemsPerPage + 1;
		// last page??
		if (m_iOffset + m_iItemsPerPage == (int)m_vecItems.size())
			iPage = m_upDown.GetMaximum();
		m_upDown.SetValue(iPage);
	}
}
