#include "GUISliderControl.h"
#include "guifontmanager.h"


CGUISliderControl::CGUISliderControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strBackGroundTexture,const CStdString& strMidTexture)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
,m_guiBackground(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight,strBackGroundTexture)
,m_guiMid(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight,strMidTexture)
{
	m_iPercent=0;
}

CGUISliderControl::~CGUISliderControl(void)
{
}


void CGUISliderControl::Render()
{
	if (!IsVisible()) return;
	if (IsDisabled()) return;

	int iHeight=25;
	m_guiBackground.Render();
	m_guiBackground.SetHeight(iHeight);

  float fWidth=(float)m_guiBackground.GetTextureWidth();
  
  float fPos = (float)m_iPercent;
  fPos /=100.0f;
  fPos *= fWidth;
  fPos += (float) m_guiBackground.GetXPosition();
	if ((int)fWidth > 1)
	{
		m_guiMid.SetPosition((int)fPos, m_guiBackground.GetYPosition() );
		m_guiMid.Render();
	}
}


bool CGUISliderControl::CanFocus() const
{
  return false;
}


bool CGUISliderControl::OnMessage(CGUIMessage& message)
{
  return CGUIControl::OnMessage(message);
}

void CGUISliderControl::SetPercentage(int iPercent)
{
		m_iPercent=iPercent;
}

int CGUISliderControl::GetPercentage() const
{
	return m_iPercent;
}
void CGUISliderControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_guiBackground.FreeResources();
  m_guiMid.FreeResources();
}

void CGUISliderControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground.AllocResources();
  m_guiMid.AllocResources();
	m_guiBackground.SetHeight(25);
	m_guiMid.SetHeight(20);
}

void CGUISliderControl::Update()
{
  m_guiBackground.SetPosition( GetXPosition(), GetYPosition());
}