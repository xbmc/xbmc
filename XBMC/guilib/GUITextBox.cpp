#include "guitextbox.h"
#include "guifontmanager.h"

#define CONTROL_LIST		0
#define CONTROL_UPDOWN	1
CGUITextBox::CGUITextBox(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                                 const CStdString& strFontName, 
                                 DWORD dwSpinWidth,DWORD dwSpinHeight,
                                 const CStdString& strUp, const CStdString& strDown, 
                                 const CStdString& strUpFocus, const CStdString& strDownFocus, 
                                 DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                                 const CStdString& strFont, DWORD dwTextColor)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight)
,m_upDown(dwControlId, 0, dwSpinX, dwSpinY, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, strFont, dwSpinColor, SPIN_CONTROL_TYPE_INT)
{
  m_iOffset=0;
  m_iItemsPerPage=10;
  m_iItemHeight=10;
  m_pFont=g_fontManager.GetFont(strFontName);
  m_dwTextColor=dwTextColor;
}

CGUITextBox::~CGUITextBox(void)
{
}

void CGUITextBox::Render()
{
  if (!m_pFont) return;
  if (!IsVisible()) return;
  CGUIControl::Render();
  DWORD dwPosY=m_dwPosY;
	
  for (int i=0; i < m_iItemsPerPage; i++)
  {
		DWORD dwPosX=m_dwPosX;
    if (i+m_iOffset < (int)m_vecItems.size() )
    {
      // render item
      wstring strLabel=m_vecItems[i+m_iOffset];
      
			DWORD dMaxWidth=m_dwWidth+16;
			m_pFont->DrawTextWidth((float)dwPosX, (float)dwPosY+2, m_dwTextColor,strLabel.c_str(),(float)dMaxWidth);
      dwPosY += (DWORD)m_iItemHeight;
    }
  }
	m_upDown.Render();
}

void CGUITextBox::OnKey(const CKey& key)
{
  if (!key.IsButton() ) return;

  switch (key.GetButtonCode())
  {
    case KEY_REMOTE_REVERSE:
    case KEY_BUTTON_LEFT_TRIGGER:
      OnPageUp();
    break;

    case KEY_REMOTE_FORWARD:
    case KEY_BUTTON_RIGHT_TRIGGER:
      OnPageDown();
    break;

    case KEY_REMOTE_DOWN:
    case KEY_BUTTON_DPAD_DOWN:
    {
      OnDown();
    }
    break;
    
    case KEY_REMOTE_UP:
    case KEY_BUTTON_DPAD_UP:
    {
      OnUp();
    }
    break;

    case KEY_REMOTE_LEFT:
    case KEY_BUTTON_DPAD_LEFT:
    {
      OnLeft();
    }
    break;

    case KEY_REMOTE_RIGHT:
    case KEY_BUTTON_DPAD_RIGHT:
    {
      OnRight();
    }
    break;

    default:
    {
        m_upDown.OnKey(key);
    }
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

    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_iOffset=0;
      m_vecItems.erase(m_vecItems.begin(),m_vecItems.end());
      m_upDown.SetRange(1,1);
      m_upDown.SetValue(1);
      const WCHAR* wstrLabel=(const WCHAR*)message.GetLPVOID();
			SetText(wstrLabel);
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
  CKey key(true,KEY_BUTTON_DPAD_RIGHT);
  m_upDown.OnKey(key);
  if (!m_upDown.HasFocus()) 
  {
    CGUIControl::OnKey(key);
  }
}

void CGUITextBox::OnLeft()
{
  CKey key(true,KEY_BUTTON_DPAD_LEFT);
  m_upDown.OnKey(key);
  if (!m_upDown.HasFocus()) 
  {
    //m_iSelect=CONTROL_LIST;
  }
}

void CGUITextBox::OnUp()
{
  CKey key(true,KEY_BUTTON_DPAD_UP);
  m_upDown.OnKey(key);
  if (!m_upDown.HasFocus()) 
  {
    //m_iSelect=CONTROL_LIST;
  }
}

void CGUITextBox::OnDown()
{
  CKey key(true,KEY_BUTTON_DPAD_DOWN);
  m_upDown.OnKey(key);
  if (!m_upDown.HasFocus()) 
  {
    CGUIControl::OnKey(key);
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
	int lpos=0;
	int iLastSpace=-1;
	int iLastSpaceInLine=-1;
	WCHAR szLine[1024];
  while( pos < (int)strText.size() )
  {
    // Get the current letter in the string
    WCHAR letter = strText[pos];

    // Handle the newline character
    if (letter == L'\n' )
    {
			m_vecItems.push_back(szLine);
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
				OutputDebugString("ERRROR\n");
			}
			szLine[lpos]=letter;
			szLine[lpos+1]=0;

			FLOAT fwidth,fheight;
			m_pFont->GetTextExtent(szLine,&fwidth,&fheight);
			if (fwidth > m_dwWidth)
			{
				if (iLastSpace > 0 && iLastSpaceInLine != lpos)
				{
					szLine[iLastSpaceInLine]=0;
					pos=iLastSpace;
				}
				wstring strwLine=szLine;
				m_vecItems.push_back(strwLine);
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
		wstring strwLine=szLine;
		m_vecItems.push_back(strwLine);
	}


  int iPages=m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage) iPages++;
  m_upDown.SetRange(1,iPages);
  m_upDown.SetValue(1);

}
