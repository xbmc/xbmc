#include "guithumbnailpanel.h"
#include "guifontmanager.h"

#define CONTROL_LIST		0
#define CONTROL_UPDOWN	1

CGUIThumbnailPanel::CGUIThumbnailPanel(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, 
                                 const string& strFontName, 
                                 const string& strImageIcon,
                                 const string& strImageIconFocus,
                                 DWORD dwitemWidth, DWORD dwitemHeight,
                                 DWORD dwSpinWidth,DWORD dwSpinHeight,
                                 const string& strUp, const string& strDown, 
                                 const string& strUpFocus, const string& strDownFocus, 
                                 DWORD dwSpinColor,DWORD dwSpinX, DWORD dwSpinY,
                                 const string& strFont, DWORD dwTextColor)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY, dwWidth, dwHeight)
,m_imgFolder(dwParentID, dwControlId, dwPosX, dwPosY, dwitemWidth,dwitemHeight,strImageIcon)
,m_imgFolderFocus(dwParentID, dwControlId, dwPosX, dwPosY, dwitemWidth,dwitemHeight,strImageIconFocus)
,m_upDown(dwControlId, 0, dwSpinX, dwSpinY, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, strFont, dwSpinColor, SPIN_CONTROL_TYPE_INT)
{
  m_iItemWidth=dwitemWidth;
  m_iItemHeight=dwitemHeight;
  m_iOffset    = 0; 
  
  m_pFont      = g_fontManager.GetFont(strFontName);
  m_iSelect    = CONTROL_LIST;  
	m_iCursorY   = 0;
  m_iCursorX   = 0;
  m_dwTextColor= dwTextColor;
  m_strSuffix=L"|";  
}

CGUIThumbnailPanel::~CGUIThumbnailPanel(void)
{
}

void CGUIThumbnailPanel::Render()
{
  if (!m_pFont) return;
  if (!IsVisible()) return;
  WCHAR wszText[1024];

  if (!ValidItem(m_iCursorX,m_iCursorY) )
  {
      m_iCursorX=0;
      m_iCursorY=0;
  }
  CGUIControl::Render();
  float fTextHeight,fTextWidth;
  m_pFont->GetTextExtent( L"W", &fTextWidth,&fTextHeight);

  for (int iRow=0; iRow < m_iRows; iRow++)
  {
    DWORD dwPosY=m_dwPosY + iRow*m_iItemHeight;
    for (int iCol=0; iCol < m_iColumns; iCol++)
    {
    
      
      DWORD dwPosX = m_dwPosX + iCol*m_iItemWidth;
      int iItem = iRow*m_iColumns+iCol+m_iOffset;
      if (iItem < (int)m_vecItems.size())
      {
        float fTextPosY =dwPosY+ m_imgFolder.GetHeight()-2*fTextHeight;
        
        CGUIListItem *pItem=m_vecItems[iItem];
        swprintf(wszText,L"%S", pItem->GetLabel().c_str() );

        if (m_iCursorX==iCol && m_iCursorY==iRow && HasFocus()&&m_iSelect==CONTROL_LIST )
        {
          m_imgFolderFocus.SetPosition(dwPosX, dwPosY);
          m_imgFolderFocus.Render();
          
          RenderText((float)dwPosX,(float)fTextPosY,m_dwTextColor,wszText,true);
        }
        else
        {
          m_imgFolder.SetPosition(dwPosX, dwPosY);
          m_imgFolder.Render();
          
          RenderText((float)dwPosX,(float)fTextPosY,m_dwTextColor,wszText,false);
        
        }
        if (pItem->m_strThumbnailImage != "")
        {
          if (!pItem->m_pImage)
          {
            pItem->m_pImage=new CGUIImage(0,0,dwPosX+4,dwPosY+16,64,64,pItem->m_strThumbnailImage,0xffffffff);
            pItem->m_pImage->AllocResources();
          }
          else
          {
            pItem->m_pImage->SetPosition(dwPosX+4,dwPosY+16);
            pItem->m_pImage->Render();
          }
        }
      }
    }
  }
  m_upDown.Render();
}

void CGUIThumbnailPanel::OnKey(const CKey& key)
{
  if (!key.IsButton() ) return;

  switch (key.GetButtonCode())
  {
    case KEY_BUTTON_DPAD_DOWN:
    {
      OnDown();
    }
    break;
    
    case KEY_BUTTON_DPAD_UP:
    {
      OnUp();
    }
    break;

    case KEY_BUTTON_DPAD_LEFT:
    {
      OnLeft();
    }
    break;

    case KEY_BUTTON_DPAD_RIGHT:
    {
      OnRight();
    }
    break;

    default:
    {
      if (m_iSelect==CONTROL_LIST)
      {
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
          g_graphicsContext.SendMessage(msg);
      }
      else
      {
        m_upDown.OnKey(key);
      }
    }
  }
}

bool CGUIThumbnailPanel::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId()==0)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        m_iOffset=(m_upDown.GetValue()-1)*(m_iRows*m_iColumns);
      }
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS ||
        message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_iSelect=CONTROL_LIST;
    }
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      m_vecItems.push_back( (CGUIListItem*) message.GetLPVOID() );
      int iItemsPerPage=m_iRows*m_iColumns;
      int iPages=m_vecItems.size() / iItemsPerPage;
      if (m_vecItems.size() % iItemsPerPage) iPages++;
      m_upDown.SetRange(1,iPages);
      m_upDown.SetValue(1);
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_vecItems.erase(m_vecItems.begin(),m_vecItems.end());
      m_upDown.SetRange(1,1);
      m_upDown.SetValue(1);
      m_iCursorX=m_iCursorY=m_iOffset=0;

    }
    if (message.GetMessage()==GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_iOffset + m_iCursorY*m_iColumns+m_iCursorX);
    }
  }

  if ( CGUIControl::OnMessage(message) ) return true;

  return false;

}

void CGUIThumbnailPanel::AllocResources()
{
  if (!m_pFont) return;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();
  m_imgFolder.AllocResources();
  m_imgFolderFocus.AllocResources();
  float fWidth,fHeight;
  
  // height of 1 item = folder image height + text row height + space in between
  m_pFont->GetTextExtent( L"y", &fWidth,&fHeight);
  
  
  fWidth =(float)m_iItemWidth;
  fHeight=(float)m_iItemHeight;
  float fTotalHeight= (float)(m_dwHeight -5);
  m_iRows		        = (int)(fTotalHeight / fHeight);
    
  
  m_iColumns = (int) (m_dwWidth / fWidth );

  int iItemsPerPage=m_iRows*m_iColumns;
  int iPages=m_vecItems.size() / iItemsPerPage;
  if (m_vecItems.size() % iItemsPerPage) iPages++;
  m_upDown.SetRange(1,iPages);
  m_upDown.SetValue(1);
}

void CGUIThumbnailPanel::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
  m_imgFolder.FreeResources();
  m_imgFolderFocus.FreeResources();
}
bool CGUIThumbnailPanel::ValidItem(int iX, int iY)
{
  if (iX >= m_iColumns) return false;
  if (iY >= m_iRows) return false;
  if (m_iOffset + iY*m_iColumns+iX < (int)m_vecItems.size() ) return true;
  return false;
}
void CGUIThumbnailPanel::OnRight()
{
  CKey key(true,KEY_BUTTON_DPAD_RIGHT);
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorX+1 < m_iColumns && ValidItem(m_iCursorX+1,m_iCursorY) )
    {
      m_iCursorX++;
      return;
    }

    m_iSelect=CONTROL_UPDOWN;
    m_upDown.SetFocus(true);
  }
  else
  {
    m_upDown.OnKey(key);
    if (!m_upDown.HasFocus()) 
    {
      CGUIControl::OnKey(key);
    }
  }
}

void CGUIThumbnailPanel::OnLeft()
{
  CKey key(true,KEY_BUTTON_DPAD_LEFT);
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorX > 0) 
    {
      m_iCursorX--;
      return;
    }
    CGUIControl::OnKey(key);
  }
  else
  {
    m_upDown.OnKey(key);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }
  }
}

void CGUIThumbnailPanel::OnUp()
{
  CKey key(true,KEY_BUTTON_DPAD_UP);
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorY > 0) 
    {
      m_iCursorY--;
    }
    else if (m_iCursorY ==0 && m_iOffset)
    {
      m_iOffset-=m_iColumns;
    }
    else
    {
      return CGUIControl::OnKey(key);
    }
  }
  else
  {
    m_upDown.OnKey(key);
    if (!m_upDown.HasFocus()) 
    {
      m_iSelect=CONTROL_LIST;
    }  
  }
}

void CGUIThumbnailPanel::OnDown()
{
  CKey key(true,KEY_BUTTON_DPAD_DOWN);
  if (m_iSelect==CONTROL_LIST) 
  {
    if (m_iCursorY+1==m_iRows)
    {
      m_iOffset+= m_iColumns;
      if ( !ValidItem(m_iCursorX,m_iCursorY) ) 
        m_iOffset-= m_iColumns;
      return;
    }
    else
    {
      if ( ValidItem(m_iCursorX,m_iCursorY+1) )
      {
        m_iCursorY++;
      }
      else 
      {
        m_upDown.SetFocus(true);
        m_iSelect = CONTROL_UPDOWN;
      }
    }
  }
  else
  {
    m_upDown.OnKey(key);
    if (!m_upDown.HasFocus()) 
    {
      CGUIControl::OnKey(key);
    }  
  }
}

void CGUIThumbnailPanel::RenderText(float fPosX, float fPosY, DWORD dwTextColor, WCHAR* wszText,bool bScroll )
{
	static int scroll_pos = 0;
	static int iScrollX=0;
	static int iLastItem=-1;
	static int iFrames=0;
	static int iStartFrame=0;

  float fTextHeight,fTextWidth;
  m_pFont->GetTextExtent( wszText, &fTextWidth,&fTextHeight);
  float fMaxWidth=m_imgFolder.GetWidth()-m_imgFolder.GetWidth()/10.0f;
  if (!bScroll || fTextWidth <= fMaxWidth)
  {
    m_pFont->DrawTextWidth(fPosX,fPosY,dwTextColor,wszText,fMaxWidth);
    return;
  }
  else
  {
    // scroll
    WCHAR wszOrgText[1024];
    wcscpy(wszOrgText, wszText);
    wcscat(wszOrgText,m_strSuffix.c_str());
    m_pFont->GetTextExtent( wszOrgText, &fTextWidth,&fTextHeight);

    int iItem=m_iCursorX+m_iCursorY*m_iColumns+m_iOffset;
    if (fTextWidth > fMaxWidth)
    {
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
              m_pFont->DrawTextWidth(fPosX-iScrollX,fPosY,m_dwTextColor,szText,fMaxWidth);
						
					}
					else
					{
						iStartFrame++;
						if (fPosY >=0.0)
              m_pFont->DrawTextWidth(fPosX,fPosY,m_dwTextColor,wszText,fMaxWidth);
					}
    }
  }
}

void CGUIThumbnailPanel::SetScrollySuffix(string wstrSuffix)
{
  WCHAR wsSuffix[128];
  swprintf(wsSuffix,L"%S", wstrSuffix.c_str());
  m_strSuffix=wsSuffix;
}