#include "stdafx.h"
#include "guiCheckMarkControl.h"
#include "guifontmanager.h"
#include "../xbmc/utils/CharsetConverter.h"

CGUICheckMarkControl::CGUICheckMarkControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureCheckMark,const CStdString& strTextureCheckMarkNF,DWORD dwCheckWidth, DWORD dwCheckHeight,DWORD dwAlign)
:CGUIControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight)
,m_imgCheckMark(dwParentID, dwControlId, iPosX, iPosY,dwCheckWidth, dwCheckHeight, strTextureCheckMark)
,m_imgCheckMarkNoFocus(dwParentID, dwControlId, iPosX, iPosY,dwCheckWidth, dwCheckHeight, strTextureCheckMarkNF)
{
	m_strLabel=L"";
	m_dwTextColor	= 0xFFFFFFFF; 
	m_dwDisabledColor	= 0xFF606060; 
	m_pFont=NULL;
  m_bSelected=false;
	m_dwAlign=dwAlign;
  m_bShadow=false;
	ControlType = GUICONTROL_CHECKMARK;
}

CGUICheckMarkControl::~CGUICheckMarkControl(void)
{
}

void CGUICheckMarkControl::Render()
{
  if (!IsVisible()) return;
	int iTextPosX=m_iPosX;
	int iTextPosY=m_iPosY;
	int iCheckMarkPosX=m_iPosX;
  if (m_pFont) 
  {
	  CStdStringW strLabelUnicode;
	  g_charsetConverter.stringCharsetToFontCharset(m_strLabel, strLabelUnicode);

	  float fTextHeight,fTextWidth;
	  m_pFont->GetTextExtent( strLabelUnicode.c_str(), &fTextWidth, &fTextHeight);
	  m_dwWidth = (DWORD)fTextWidth+5+m_imgCheckMark.GetWidth();
	  m_dwHeight = m_imgCheckMark.GetHeight();

	  if (fTextHeight < m_imgCheckMark.GetHeight())
	  {
		  iTextPosY += (m_imgCheckMark.GetHeight() - (int)fTextHeight) / 2;
	  }

	  if (m_dwAlign==XBFONT_LEFT)
	  {
		  iCheckMarkPosX += ( (DWORD)(fTextWidth)+5);
	  }
	  else
	  {
		  iTextPosX += m_imgCheckMark.GetWidth() +5;
	  }

    if (IsDisabled() )
    {
      m_pFont->DrawText((float)iTextPosX, (float)iTextPosY, m_dwDisabledColor, strLabelUnicode.c_str());
    }
    else
    {
      if (HasFocus())
      {
        if (m_bShadow)
          m_pFont->DrawShadowText((float)iTextPosX, (float)iTextPosY, m_dwTextColor, strLabelUnicode.c_str());
        else
          m_pFont->DrawText((float)iTextPosX, (float)iTextPosY, m_dwTextColor, strLabelUnicode.c_str());
      }
      else
      {
        if (m_bShadow)
          m_pFont->DrawShadowText((float)iTextPosX, (float)iTextPosY, m_dwDisabledColor, strLabelUnicode.c_str());
        else
          m_pFont->DrawText((float)iTextPosX, (float)iTextPosY, m_dwDisabledColor, strLabelUnicode.c_str());
      }
    }
  }
  if (m_bSelected)
  {
    m_imgCheckMark.SetPosition(iCheckMarkPosX, m_iPosY); 
    m_imgCheckMark.Render();
  }
  else
  {
    m_imgCheckMarkNoFocus.SetPosition(iCheckMarkPosX, m_iPosY); 
    m_imgCheckMarkNoFocus.Render();
  }
}

void CGUICheckMarkControl::OnAction(const CAction &action) 
{
	CGUIControl::OnAction(action);
	if (action.wID == ACTION_SELECT_ITEM)
	{
		m_bSelected=!m_bSelected;
		CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.wID);
		g_graphicsContext.SendMessage(msg);
    }
}

bool CGUICheckMarkControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId()==GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
	  m_strLabel=message.GetLabel();
      return true;
    }
  }
  if (CGUIControl::OnMessage(message)) return true;
  return false;
}

void CGUICheckMarkControl::PreAllocResources()
{
	CGUIControl::PreAllocResources();
	m_imgCheckMark.PreAllocResources();
	m_imgCheckMarkNoFocus.PreAllocResources();
}

void CGUICheckMarkControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_imgCheckMark.AllocResources();
  m_imgCheckMarkNoFocus.AllocResources();
}

void CGUICheckMarkControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgCheckMark.FreeResources();
  m_imgCheckMarkNoFocus.FreeResources();
}


void CGUICheckMarkControl::SetLabel(const CStdString& strFontName,const CStdString& strLabel,D3DCOLOR dwColor)
{
  WCHAR wszText[1024];
  swprintf(wszText,L"%S",strLabel.c_str());
	m_strLabel=wszText;
	m_dwTextColor=dwColor;
	m_pFont=g_fontManager.GetFont(strFontName);
}

void CGUICheckMarkControl::SetLabel(const CStdString& strFontName,const wstring& strLabel,D3DCOLOR dwColor)
{
	m_strLabel=strLabel;
	m_dwTextColor=dwColor;
	m_pFont=g_fontManager.GetFont(strFontName);
}

void CGUICheckMarkControl::SetDisabledColor(D3DCOLOR color)
{
  m_dwDisabledColor=color;
}

void CGUICheckMarkControl::SetSelected(bool bOnOff)
{
  m_bSelected=bOnOff;
}


bool CGUICheckMarkControl::GetSelected() const
{
  return m_bSelected;
}

bool CGUICheckMarkControl::GetShadow() const
{
  return m_bShadow;
}

void CGUICheckMarkControl::SetShadow(bool bOnOff)
{
  m_bShadow=bOnOff;
}

void CGUICheckMarkControl::OnMouseClick(DWORD dwButton)
{
	if (dwButton != MOUSE_LEFT_BUTTON) return;
	g_Mouse.SetState(MOUSE_STATE_CLICK);
	CAction action;
	action.wID = ACTION_SELECT_ITEM;
	OnAction(action);
}