#include "GUIProgressControl.h"
#include "guifontmanager.h"


CGUIProgressControl::CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, CStdString& strBackGroundTexture,CStdString& strForGroundTexture)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
,m_guiBackground(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight,strBackGroundTexture)
,m_guiForeground(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight,strForGroundTexture)
{
	m_iPercent=0;
}

CGUIProgressControl::~CGUIProgressControl(void)
{
}


void CGUIProgressControl::Render()
{
	if (!IsVisible()) return;
	if (IsDisabled()) return;

	m_guiBackground.Render();
	float fWidth = (float)m_iPercent;
	fWidth/=100.0f;
	fWidth *= (float) m_guiBackground.GetWidth();

	m_guiForeground.SetWidth( (int) fWidth);
	m_guiForeground.Render();
}


bool CGUIProgressControl::CanFocus() const
{
  return false;
}


bool CGUIProgressControl::OnMessage(CGUIMessage& message)
{
  return CGUIControl::OnMessage(message);
}

void CGUIProgressControl::SetPercentage(int iPercent)
{
		m_iPercent=iPercent;
}

int CGUIProgressControl::GetPercentage() const
{
	return m_iPercent;
}