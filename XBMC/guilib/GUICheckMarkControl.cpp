#include "guiCheckMarkControl.h"
#include "guifontmanager.h"


CGUICheckMarkControl::CGUICheckMarkControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureCheckMark,DWORD dwCheckWidth, DWORD dwCheckHeight,DWORD dwAlign)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
,m_imgCheckMark(dwParentID, dwControlId, dwPosX, dwPosY,dwCheckWidth, dwCheckHeight, strTextureCheckMark)
{
	m_strLabel=L""; 
	m_dwTextColor	= 0xFFFFFFFF; 
	m_dwDisabledColor	= 0xFF606060; 
	m_pFont=NULL;
  m_bSelected=false;
	m_dwAlign=dwAlign;
}

CGUICheckMarkControl::~CGUICheckMarkControl(void)
{
}

void CGUICheckMarkControl::Render()
{
  if (!m_pFont) return;
  if (!IsVisible()) return;
	DWORD dwTextPosX=m_dwPosX;
	DWORD dwCheckMarkPosX=m_dwPosX;
	if (m_dwAlign==XBFONT_LEFT)
	{
		float fTextHeight,fTextWidth;
		m_pFont->GetTextExtent( m_strLabel.c_str(), &fTextWidth,&fTextHeight);
		dwCheckMarkPosX += ( (DWORD)(fTextWidth)+5);
	}
	else
	{
		dwTextPosX += m_imgCheckMark.GetWidth() +5;
	}

  if (IsDisabled() )
  {
    m_pFont->DrawText((float)dwTextPosX, (float)m_dwPosY, m_dwDisabledColor, m_strLabel.c_str());
  }
  else
  {
    if (HasFocus())
    {
      m_pFont->DrawShadowText((float)dwTextPosX, (float)m_dwPosY, m_dwTextColor, m_strLabel.c_str());
    }
    else
    {
      m_pFont->DrawText((float)dwTextPosX, (float)m_dwPosY, m_dwDisabledColor, m_strLabel.c_str());
    }
  }
  if (m_bSelected)
  {
    m_imgCheckMark.SetPosition(dwCheckMarkPosX, m_dwPosY); 
    m_imgCheckMark.Render();
  }
}

void CGUICheckMarkControl::OnKey(const CKey& key) 
{
  CGUIControl::OnKey(key);
  if (key.IsButton())
  {
    if (key.GetButtonCode() ==KEY_BUTTON_A || key.GetButtonCode() == KEY_REMOTE_SELECT)
    {
      m_bSelected=!m_bSelected;
			CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(),key.GetButtonCode());
			g_graphicsContext.SendMessage(msg);

    }
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

void CGUICheckMarkControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_imgCheckMark.AllocResources();
}

void CGUICheckMarkControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgCheckMark.FreeResources();
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
