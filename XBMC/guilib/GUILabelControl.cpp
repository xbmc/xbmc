#include "guilabelcontrol.h"
#include "guifontmanager.h"


CGUILabelControl::CGUILabelControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strFont,const wstring& strLabel, DWORD dwTextColor, DWORD dwTextAlign)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
{
  m_strLabel=strLabel;
  m_pFont=g_fontManager.GetFont(strFont);
  m_dwTextColor=dwTextColor;
  m_dwdwTextAlign=dwTextAlign;
}

CGUILabelControl::~CGUILabelControl(void)
{
}


void CGUILabelControl::Render()
{
	if (!IsVisible() ) return;
  if (m_pFont)
  {
    m_pFont->DrawText((float)m_dwPosX, (float)m_dwPosY,m_dwTextColor,m_strLabel.c_str(),m_dwdwTextAlign); 
  }
}


bool CGUILabelControl::CanFocus() const
{
  return false;
}


bool CGUILabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId()==GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_strLabel = message.GetLabel();
      return true;
    }
  }
  return CGUIControl::OnMessage(message);
}