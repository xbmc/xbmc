#include "guibuttoncontrol.h"
#include "guifontmanager.h"
#include "guiWindowManager.h"
#include "ActionManager.h"

CGUIButtonControl::CGUIButtonControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus,const CStdString& strTextureNoFocus)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
,m_imgFocus(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight, strTextureFocus)
,m_imgNoFocus(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight, strTextureNoFocus)
{
  m_bSelected=false;
  m_dwFrameCounter = 0;
	m_strLabel=L""; 
	m_dwTextColor	= 0xFFFFFFFF; 
	m_dwDisabledColor	= 0xFF606060; 
	m_pFont=NULL;
  m_lHyperLinkWindowID=-1;
	m_strScriptAction="";
}

CGUIButtonControl::~CGUIButtonControl(void)
{
}

void CGUIButtonControl::SetDisabledColor(D3DCOLOR color)
{
  m_dwDisabledColor=color;
}
void CGUIButtonControl::Render()
{
  if (!IsVisible() ) return;

  if (HasFocus())
  {
    DWORD dwAlphaCounter = m_dwFrameCounter+2;
    DWORD dwAlphaChannel;
	  if ((dwAlphaCounter%128)>=64)
		  dwAlphaChannel = dwAlphaCounter%64;
	  else
		  dwAlphaChannel = 63-(dwAlphaCounter%64);

	  dwAlphaChannel += 192;
    SetAlpha(dwAlphaChannel );
		m_imgFocus.SetVisible(true);
		m_imgNoFocus.SetVisible(false);
    m_dwFrameCounter++;
  }
  else 
  {
		SetAlpha(0xff);
		m_imgFocus.SetVisible(false);
		m_imgNoFocus.SetVisible(true);
  }
	// render both so the visibility settings cause the frame counter to resetcorrectly
	m_imgFocus.Render();
	m_imgNoFocus.Render();  

	if (m_strLabel.size() > 0 && m_pFont)
	{
    if (IsDisabled() )
      m_pFont->DrawText((float)10+m_dwPosX, (float)2+m_dwPosY,m_dwDisabledColor,m_strLabel.c_str());
    else
      m_pFont->DrawText((float)10+m_dwPosX, (float)2+m_dwPosY,m_dwTextColor,m_strLabel.c_str());
	}

}

void CGUIButtonControl::OnAction(const CAction &action) 
{
	CGUIControl::OnAction(action);
	if (action.wID == ACTION_SELECT_ITEM)
	{
		if (m_strScriptAction.length() > 0)
		{
			CGUIMessage message(GUI_MSG_CLICKED,GetID(), GetParentID());
			message.SetStringParam(m_strScriptAction);
			g_actionManager.CallScriptAction(message);
		}

		if (m_lHyperLinkWindowID >=0)
		{
			m_gWindowManager.ActivateWindow(m_lHyperLinkWindowID);
			return;
		}
		// button selected.
		// send a message
		CGUIMessage message(GUI_MSG_CLICKED,GetID(), GetParentID() );
		g_graphicsContext.SendMessage(message);
    }
}

bool CGUIButtonControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId()==GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
	    m_strLabel=message.GetLabel() ;

      return true;
    }
  }
  if (CGUIControl::OnMessage(message)) return true;
  return false;
}

void CGUIButtonControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_dwFrameCounter=0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  m_dwWidth=m_imgFocus.GetWidth();
  m_dwHeight=m_imgFocus.GetHeight();
}

void CGUIButtonControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgFocus.FreeResources();
  m_imgNoFocus.FreeResources();
}


void CGUIButtonControl::SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor)
{
  WCHAR wszText[1024];
  swprintf(wszText,L"%S",strLabel.c_str());
	m_strLabel=wszText;
	m_dwTextColor=dwColor;
	m_pFont=g_fontManager.GetFont(strFontName);
}

void CGUIButtonControl::SetLabel(const CStdString& strFontName,const wstring& strLabel,D3DCOLOR dwColor)
{
	m_strLabel=strLabel;
	m_dwTextColor=dwColor;
	m_pFont=g_fontManager.GetFont(strFontName);
}


void  CGUIButtonControl::Update() 
{
  CGUIControl::Update();
  
  m_imgFocus.SetWidth(m_dwWidth);
  m_imgFocus.SetHeight(m_dwHeight);

  m_imgNoFocus.SetWidth(m_dwWidth);
  m_imgNoFocus.SetHeight(m_dwHeight);
}

void CGUIButtonControl::SetPosition(DWORD dwPosX, DWORD dwPosY)
{
  CGUIControl::SetPosition(dwPosX, dwPosY);
  m_imgFocus.SetPosition(dwPosX, dwPosY);
  m_imgNoFocus.SetPosition(dwPosX, dwPosY);
}
void CGUIButtonControl::SetAlpha(DWORD dwAlpha)
{
  CGUIControl::SetAlpha(dwAlpha);
  m_imgFocus.SetAlpha(dwAlpha);
  m_imgNoFocus.SetAlpha(dwAlpha);
}

void CGUIButtonControl::SetColourDiffuse(D3DCOLOR colour)
{
  CGUIControl::SetColourDiffuse(colour);
  m_imgFocus.SetColourDiffuse(colour);
  m_imgNoFocus.SetColourDiffuse(colour);
}


void CGUIButtonControl::SetHyperLink(long dwWindowID)
{
  m_lHyperLinkWindowID=dwWindowID;
}

void CGUIButtonControl::SetScriptAction(const CStdString& strScriptAction)
{
	m_strScriptAction = strScriptAction;
}

