#include "stdafx.h"
#include "guilistcontrol.h"
#include "guifontmanager.h"

#define CONTROL_LIST		0
#define CONTROL_UPDOWN	1
CGUIListControl::CGUIListControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                                 const CStdString& strFontName, 
                                 DWORD dwSpinWidth,DWORD dwSpinHeight,
                                 const CStdString& strUp, const CStdString& strDown, 
                                 const CStdString& strUpFocus, const CStdString& strDownFocus, 
                                 DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                                 const CStdString& strFont, DWORD dwTextColor,DWORD dwSelectedColor,
                                 const CStdString& strButton, const CStdString& strButtonFocus,
								 DWORD dwItemTextOffsetX, DWORD dwItemTextOffsetY)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight)
,m_upDown(dwControlId, 0, dwSpinX, dwSpinY, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, strFont, dwSpinColor, SPIN_CONTROL_TYPE_INT)
,m_imgButton(dwControlId, 0, dwPosX, dwPosY, dwWidth, dwHeight, strButtonFocus,strButton, dwItemTextOffsetX, dwItemTextOffsetY)
{
  m_dwSelectedColor=dwSelectedColor;
  m_iOffset=0;
  m_iItemsPerPage=10;
  m_iItemHeight=10;
  m_pFont=g_fontManager.GetFont(strFontName);
  m_pFont2=g_fontManager.GetFont(strFontName);
  m_iSelect=CONTROL_LIST;  
	m_iCursorY=0;
  m_dwTextColor=dwTextColor;
  m_strSuffix=L"|";
	m_iTextOffsetX=0;
	m_iTextOffsetY=0;
	m_iImageWidth=16;
	m_iImageHeight=16;
	m_iSpaceBetweenItems=4;
	m_iTextOffsetX2=0;
	m_iTextOffsetY2=0;
	m_bUpDownVisible = true;	// show the spin control by default
	
	m_dwTextColor2=dwTextColor;
	m_dwSelectedColor2=dwSelectedColor;
	ControlType = GUICONTROL_LIST;
}

CGUIListControl::~CGUIListControl(void)
{
}

void CGUIListControl::Render()
{
  if (!m_pFont) return;
  if (!IsVisible()) return;
  CGUIControl::Render();
  WCHAR wszText[1024];
  DWORD dwPosY=m_dwPosY;
	
  for (int i=0; i < m_iItemsPerPage; i++)
  {
		DWORD dwPosX=m_dwPosX;
    if (i+m_iOffset < (int)m_vecItems.size() )
    {
      // render item
      CGUIListItem *pItem=m_vecItems[i+m_iOffset];
      if (i == m_iCursorY && HasFocus() && m_iSelect== CONTROL_LIST)
			{
				// render focused line
        m_imgButton.SetFocus(true);
			}
			else
			{
				// render no-focused line
        m_imgButton.SetFocus(false);
			}
      m_imgButton.SetPosition(m_dwPosX, dwPosY);	
      m_imgButton.Render();
      
			// render the icon
			if (pItem->HasIcon() )
      {
        // show icon
				CGUIImage* pImage=pItem->GetIcon();
				if (!pImage)
				{
					pImage=new CGUIImage(0,0,0,0,m_iImageWidth,m_iImageHeight,pItem->GetIconImage(),0x0);
					pImage->AllocResources();
					pItem->SetIcon(pImage);
					
				}
				pImage->SetWidth(m_iImageWidth);
				pImage->SetHeight(m_iImageHeight);
				// center vertically
				pImage->SetPosition(dwPosX+8, dwPosY+(m_iItemHeight-m_iImageHeight)/2);
				pImage->Render();
      }
			dwPosX+=(m_iImageWidth+10);

			// render the text
      DWORD dwColor=m_dwTextColor;
			if (pItem->IsSelected())
			{
        dwColor=m_dwSelectedColor;
			}
      
			dwPosX +=m_iTextOffsetX;
      bool bSelected(false);
      if (i == m_iCursorY && HasFocus() && m_iSelect== CONTROL_LIST)
			{
        bSelected=true;
			}

			CStdString strLabel2=pItem->GetLabel2();
      
			DWORD dMaxWidth=(m_dwWidth-m_iImageWidth-16);
      if ( strLabel2.size() > 0 )
      {
				if ( m_iTextOffsetY == m_iTextOffsetY2 ) 
				{
					float fTextHeight,fTextWidth;
					swprintf(wszText,L"%S", strLabel2.c_str() );
					m_pFont2->GetTextExtent( wszText, &fTextWidth,&fTextHeight);
					dMaxWidth -= (DWORD)(fTextWidth+20);
				}
			}

			swprintf(wszText,L"%S", pItem->GetLabel().c_str() );
			RenderText((float)dwPosX, (float)dwPosY+2+m_iTextOffsetY, (FLOAT)dMaxWidth, dwColor, wszText,bSelected);
      
      if (strLabel2.size()>0)
      {
				dwColor=m_dwTextColor2;
				if (pItem->IsSelected())
				{
					dwColor=m_dwSelectedColor2;
				}
				if (!m_iTextOffsetX2)
					dwPosX=m_dwPosX+m_dwWidth-16;
				else
					dwPosX=m_dwPosX+m_iTextOffsetX2;

        swprintf(wszText,L"%S", strLabel2.c_str() );
        m_pFont2->DrawText((float)dwPosX, (float)dwPosY+2+m_iTextOffsetY2,dwColor,wszText,XBFONT_RIGHT); 
      }	
      dwPosY += (DWORD)(m_iItemHeight+m_iSpaceBetweenItems);
    }
  }
	if (m_bUpDownVisible) m_upDown.Render();
}

void CGUIListControl::RenderText(float fPosX, float fPosY, float fMaxWidth,DWORD dwTextColor, WCHAR* wszText,bool bScroll )
{
	static int scroll_pos = 0;
	static int iScrollX=0;
	static int iLastItem=-1;
	static int iFrames=0;
	static int iStartFrame=0;

  float fTextHeight,fTextWidth;
  m_pFont->GetTextExtent( wszText, &fTextWidth,&fTextHeight);

	float fPosCX=fPosX;
	float fPosCY=fPosY;
	g_graphicsContext.Correct(fPosCX, fPosCY);
	if (fPosCX <0) fPosCX=0.0f;
	if (fPosCY <0) fPosCY=0.0f;
	if (fPosCY >g_graphicsContext.GetHeight()) fPosCY=(float)g_graphicsContext.GetHeight();
	float fHeight=60.0f;
	if (fHeight+fPosCY >= g_graphicsContext.GetHeight() )
		fHeight = g_graphicsContext.GetHeight() - fPosCY -1;
	if (fHeight <= 0) return ;

	float fwidth=fMaxWidth-5.0f;

	D3DVIEWPORT8 newviewport,oldviewport;
	g_graphicsContext.Get3DDevice()->GetViewport(&oldviewport);
	newviewport.X      = (DWORD)fPosCX;
	newviewport.Y			 = (DWORD)fPosCY;
	newviewport.Width  = (DWORD)(fwidth);
	newviewport.Height = (DWORD)(fHeight);
	newviewport.MinZ   = 0.0f;
	newviewport.MaxZ   = 1.0f;
	g_graphicsContext.Get3DDevice()->SetViewport(&newviewport);

  if (!bScroll)
  {
    m_pFont->DrawTextWidth(fPosX,fPosY,dwTextColor,wszText,fMaxWidth);
		g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);
    return;
  }
  else
  {
	  if (fTextWidth <= fMaxWidth)
	  {	// don't need to scroll
		m_pFont->DrawTextWidth(fPosX,fPosY,dwTextColor,wszText,fMaxWidth);
		g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);
		iLastItem = -1; // reset scroller
		return;
	  }
    // scroll
    int iItem=m_iCursorY+m_iOffset;
    WCHAR wszOrgText[1024];
    wcscpy(wszOrgText, wszText);
		wcscat(wszOrgText, L" ");
    wcscat(wszOrgText, m_strSuffix.c_str());
    m_pFont->GetTextExtent( wszOrgText, &fTextWidth,&fTextHeight);

    if (fTextWidth > fMaxWidth)
    {
				fMaxWidth+=50.0f;
        WCHAR szText[1024];
				if (iLastItem != iItem)
				{
					scroll_pos=0;
					iLastItem=iItem;
					iStartFrame=0;
					iScrollX=1;
				}
        if (iStartFrame > 25)
				{
						WCHAR wTmp[3];
						if (scroll_pos >= (int)wcslen(wszOrgText) )
							wTmp[0]=L' ';
						else
							wTmp[0]=wszOrgText[scroll_pos];
						wTmp[1]=0;
            float fWidth,fHeight;
						m_pFont->GetTextExtent(wTmp,&fWidth,&fHeight);
						if ( iScrollX >= fWidth)
						{
							++scroll_pos;
							if (scroll_pos > (int)wcslen(wszOrgText) )
								scroll_pos = 0;
							iFrames=0;
							iScrollX=1;
						}
						else iScrollX++;
					
						int ipos=0;
						for (int i=0; i < (int)wcslen(wszOrgText); i++)
						{
							if (i+scroll_pos < (int)wcslen(wszOrgText))
								szText[i]=wszOrgText[i+scroll_pos];
							else
							{
								if (ipos==0) szText[i]=L' ';
								else szText[i]=wszOrgText[ipos-1];
								ipos++;
							}
							szText[i+1]=0;
						}
						if (fPosY >=0.0)
              m_pFont->DrawTextWidth(fPosX-iScrollX,fPosY,dwTextColor,szText,fMaxWidth);
						
					}
					else
					{
						iStartFrame++;
						if (fPosY >=0.0)
              m_pFont->DrawTextWidth(fPosX,fPosY,dwTextColor,wszText,fMaxWidth);
					}
    }
  }
	g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);
}

void CGUIListControl::OnAction(const CAction &action)
{
  switch (action.wID)
  {
    case ACTION_PAGE_UP:
      OnPageUp();
    break;

    case ACTION_PAGE_DOWN:
      OnPageDown();
    break;

    case ACTION_MOVE_DOWN:
    {
      OnDown();
    }
    break;
    
     case ACTION_MOVE_UP:
    {
      OnUp();
    }
    break;

    case ACTION_MOVE_LEFT:
    {
      OnLeft();
    }
    break;

    case ACTION_MOVE_RIGHT:
    {
      OnRight();
    }
    break;

    default:
    {
      if (m_iSelect==CONTROL_LIST)
      {
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.wID);
          g_graphicsContext.SendMessage(msg);
      }
      else
      {
        m_upDown.OnAction(action);
      }
    }
  }
}

bool CGUIListControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId()==0)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
				while (m_iOffset+m_iCursorY >= (int)m_vecItems.size()) m_iCursorY--;

				//	moving to the last page
				if (m_iOffset+m_iItemsPerPage>(int)m_vecItems.size() && (int)m_vecItems.size()-1>m_iItemsPerPage)
				{
					m_iOffset=m_vecItems.size()-m_iItemsPerPage;
					m_iCursorY=m_iItemsPerPage-1;
				}
      }
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS ||
        message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_iSelect=CONTROL_LIST;
    }
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      CGUIListItem* pItem=(CGUIListItem*)message.GetLPVOID();
      m_vecItems.push_back( pItem);
      int iPages=m_vecItems.size() / m_iItemsPerPage;
      if (m_vecItems.size() % m_iItemsPerPage) iPages++;
      m_upDown.SetRange(1,iPages);
      m_upDown.SetValue(1);
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_iCursorY=0;
      m_iOffset=0;
      m_vecItems.erase(m_vecItems.begin(),m_vecItems.end());
      m_upDown.SetRange(1,1);
      m_upDown.SetValue(1);
    }

    if (message.GetMessage()==GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_iCursorY+m_iOffset);
    }
		if (message.GetMessage()==GUI_MSG_ITEM_SELECT)
		{
			if (message.GetParam1() >=0 && message.GetParam1() < (int)m_vecItems.size())
			{
				int iPage=1;
				m_iOffset=0;
				m_iCursorY=message.GetParam1();
				while (m_iCursorY >= m_iItemsPerPage)
				{
					iPage++;
					m_iOffset+=m_iItemsPerPage;
					m_iCursorY -=m_iItemsPerPage;
				}
				//	moving to the last item, make sure the whole page is filled
				if (message.GetParam1() == (int)m_vecItems.size()-1 && (int)m_vecItems.size()-1>m_iItemsPerPage )
				{
					m_iOffset=m_vecItems.size()-m_iItemsPerPage;
					m_iCursorY=m_iItemsPerPage-1;
				}
				m_upDown.SetValue(iPage);
			}
		}
  }

  if ( CGUIControl::OnMessage(message) ) return true;

  return false;
}

void CGUIListControl::PreAllocResources()
{
	if (!m_pFont) return;
	CGUIControl::PreAllocResources();
	m_upDown.PreAllocResources();
	m_imgButton.PreAllocResources();
}

void CGUIListControl::AllocResources()
{
  if (!m_pFont) return;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();
  
  m_imgButton.AllocResources();

  
  m_imgButton.SetWidth(m_dwWidth);
  m_imgButton.SetHeight(m_iItemHeight);

	float fHeight=(float)m_iItemHeight + (float)m_iSpaceBetweenItems;
  float fTotalHeight= (float)(m_dwHeight-m_upDown.GetHeight()-5);
  m_iItemsPerPage		= (int)(fTotalHeight / fHeight );

  int iPages=m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage) iPages++;
  m_upDown.SetRange(1,iPages);
  m_upDown.SetValue(1);

}

void CGUIListControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
  m_imgButton.FreeResources();
}

void CGUIListControl::OnRight()
{
  CKey key(KEY_BUTTON_DPAD_RIGHT);
  CAction action;
  action.wID = ACTION_MOVE_RIGHT;
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_upDown.GetMaximum() > 1)
    {
      m_iSelect=CONTROL_UPDOWN;
      m_upDown.SetFocus(true);
      if (!m_upDown.HasFocus()) 
      {
        m_iSelect=CONTROL_LIST;
      }
    }
  }
  else
  {
    m_upDown.OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }
  }
}

void CGUIListControl::OnLeft()
{
  CKey key(KEY_BUTTON_DPAD_LEFT);
  CAction action;
  action.wID = ACTION_MOVE_LEFT;
  if (m_iSelect==CONTROL_LIST) 
  {
    CGUIControl::OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }
  }
  else
  {
    m_upDown.OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }
  }
}

void CGUIListControl::OnUp()
{
  CKey key(KEY_BUTTON_DPAD_UP);
  CAction action;
  action.wID = ACTION_MOVE_UP;
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorY > 0) 
    {
      m_iCursorY--;
    }
    else if (m_iCursorY ==0 && m_iOffset)
    {
      m_iOffset--;
    }
    else
    {
			// move 2 last item in list
			CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), m_vecItems.size() -1); 
			OnMessage(msg);
    }
  }
  else
  {
    m_upDown.OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }  
  }
}

void CGUIListControl::OnDown()
{
  CKey key(KEY_BUTTON_DPAD_DOWN);
  CAction action;
  action.wID = ACTION_MOVE_DOWN;
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorY+1 < m_iItemsPerPage)
    {
			if (m_iOffset+1+m_iCursorY <  (int)m_vecItems.size())
			{
				m_iCursorY++;
			}
			else
			{
				// move first item in list
				CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), 0); 
				OnMessage(msg);
			}
    }
    else 
    {
			if (m_iOffset+1+m_iCursorY <  (int)m_vecItems.size())
			{
				m_iOffset++;

				int iPage=1;
				int iSel=m_iOffset+m_iCursorY;
				while (iSel >= m_iItemsPerPage)
				{
					iPage++;
					iSel -= m_iItemsPerPage;
				}
				m_upDown.SetValue(iPage);
			}
			else
			{
				// move first item in list
				CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), 0); 
				OnMessage(msg);
			}
    }
  }
  else
  {
    m_upDown.OnAction(action);
    if (!m_upDown.HasFocus()) 
    {
      CGUIControl::OnAction(action);
    }  
  }
}

void CGUIListControl::SetScrollySuffix(const CStdString& wstrSuffix)
{
  WCHAR wsSuffix[128];
  swprintf(wsSuffix,L"%S", wstrSuffix.c_str());
  m_strSuffix=wsSuffix;
}


void CGUIListControl::OnPageUp()
{
  int iPage = m_upDown.GetValue();
  if (iPage > 1)
  {
    iPage--;
    m_upDown.SetValue(iPage);
    m_iOffset=(m_upDown.GetValue()-1)*m_iItemsPerPage;
  }
	else 
	{
		// already on page 1, then select the 1st item
		m_iCursorY=0;
	}
}

void CGUIListControl::OnPageDown()
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
	else
	{
		// already on last page, move 2 last item in list
		CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), m_vecItems.size() -1); 
		OnMessage(msg);
	}
  if (m_iOffset+m_iCursorY >= (int)m_vecItems.size() )
  {
    m_iCursorY = (m_vecItems.size()-m_iOffset)-1;
  }
}


void CGUIListControl::SetTextOffsets(int iXoffset, int iYOffset,int iXoffset2, int iYOffset2)
{
	m_iTextOffsetX = iXoffset;
	m_iTextOffsetY = iYOffset;
	m_iTextOffsetX2 = iXoffset2;
	m_iTextOffsetY2 = iYOffset2;
}

void CGUIListControl::SetImageDimensions(int iWidth, int iHeight)
{
	m_iImageWidth  = iWidth;
	m_iImageHeight = iHeight;
}

void CGUIListControl::SetItemHeight(int iHeight)
{
	m_iItemHeight=iHeight;
}
void CGUIListControl::SetSpace(int iHeight)
{
	m_iSpaceBetweenItems=iHeight;
}

void CGUIListControl::SetFont2(const CStdString& strFont)
{
	if (strFont != "")
	{
		m_pFont2=g_fontManager.GetFont(strFont);
	}
}
void CGUIListControl::SetColors2(DWORD dwTextColor, DWORD dwSelectedColor)
{
	m_dwTextColor2=dwTextColor;
	m_dwSelectedColor2=dwSelectedColor;
}

int CGUIListControl::GetSelectedItem(CStdString& strLabel)
{
  strLabel="";
  int iItem=m_iCursorY+m_iOffset;
  if (iItem >=0 && iItem < (int)m_vecItems.size())
  {
   CGUIListItem *pItem=m_vecItems[iItem];
   strLabel=pItem->GetLabel();
   if (pItem->m_bIsFolder)
   {
     strLabel.Format("[%s]", pItem->GetLabel().c_str());
   }
  }
  return iItem;
}

void CGUIListControl::SetPageControlVisible(bool bVisible)
{
	m_bUpDownVisible = bVisible;
	return;
}