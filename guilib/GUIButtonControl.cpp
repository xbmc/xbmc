#include "stdafx.h"
#include "guibuttoncontrol.h"
#include "guifontmanager.h"
#include "guiWindowManager.h"
#include "ActionManager.h"

CGUIButtonControl::CGUIButtonControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus,const CStdString& strTextureNoFocus, DWORD dwTextXOffset, DWORD dwTextYOffset)
:CGUIControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight)
,m_imgFocus(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight, strTextureFocus)
,m_imgNoFocus(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight, strTextureNoFocus)
{
  m_bSelected=false;
  m_dwFrameCounter = 0;
	m_strLabel=L""; 
	m_dwTextColor	= 0xFFFFFFFF; 
	m_dwDisabledColor	= 0xFF606060; 
	m_dwTextOffsetX = dwTextXOffset;
	m_dwTextOffsetY = dwTextYOffset;
	m_pFont=NULL;
  m_lHyperLinkWindowID=WINDOW_INVALID;
	m_strScriptAction="";
	ControlType = GUICONTROL_BUTTON;
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
		{
			m_pFont->DrawText( (float)m_iPosX+m_dwTextOffsetX, (float)m_iPosY+m_dwTextOffsetY,m_dwDisabledColor,m_strLabel.c_str());
		}
		else
		{
			m_pFont->DrawText( (float)m_iPosX+m_dwTextOffsetX, (float)m_iPosY+m_dwTextOffsetY,m_dwTextColor,m_strLabel.c_str());
		}
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

		// button selected.
		// send a message
		SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);

		if (m_lHyperLinkWindowID != WINDOW_INVALID)
		{
			m_gWindowManager.ActivateWindow(m_lHyperLinkWindowID);
		}
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

void CGUIButtonControl::PreAllocResources()
{
	CGUIControl::PreAllocResources();
	m_imgFocus.PreAllocResources();
	m_imgNoFocus.PreAllocResources();
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

void CGUIButtonControl::SetText(CStdString aLabel)
{
	WCHAR wszText[1024];
	swprintf(wszText,L"%S",aLabel.c_str());
	m_strLabel=wszText;
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

void CGUIButtonControl::SetPosition(int iPosX, int iPosY)
{
  CGUIControl::SetPosition(iPosX, iPosY);
  m_imgFocus.SetPosition(iPosX, iPosY);
  m_imgNoFocus.SetPosition(iPosX, iPosY);
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

void CGUIButtonControl::OnMouseClick(DWORD dwButton)
{
	if (dwButton == MOUSE_LEFT_BUTTON)
	{
		CAction action;
		action.wID = ACTION_SELECT_ITEM;
		OnAction(action);
	}
}