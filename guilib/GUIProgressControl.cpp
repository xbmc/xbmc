#include "stdafx.h"
#include "GUIProgressControl.h"
#include "guifontmanager.h"


CGUIProgressControl::CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, CStdString& strBackGroundTexture,CStdString& strLeftTexture,CStdString& strMidTexture,CStdString& strRightTexture)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
,m_guiBackground(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight,strBackGroundTexture)
,m_guiLeft(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight,strLeftTexture)
,m_guiMid(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight,strMidTexture)
,m_guiRight(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight,strRightTexture)
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

	int iHeight=m_guiBackground.GetTextureHeight(); //25;
	m_guiBackground.Render();
	m_guiBackground.SetHeight(iHeight);

	//iHeight=20;
	float fWidth = (float)m_iPercent;
	fWidth/=100.0f;
	fWidth *= (float) (m_guiBackground.GetTextureWidth()-24-m_guiLeft.GetTextureWidth()-m_guiRight.GetTextureWidth());

	int iXPos=12+m_guiBackground.GetXPosition();
	//int iYPos=4+m_guiBackground.GetYPosition() ;
	int iYPos= m_guiBackground.GetYPosition() + ((m_guiBackground.GetTextureHeight() / 2) - (m_guiLeft.GetTextureHeight() / 2));
	m_guiLeft.SetPosition(iXPos,iYPos);
	//m_guiLeft.SetHeight(iHeight);
	m_guiLeft.SetHeight(m_guiLeft.GetTextureHeight());
	m_guiLeft.SetWidth(m_guiLeft.GetTextureWidth());
	m_guiLeft.Render();

	iXPos += m_guiLeft.GetTextureWidth();
	if (m_iPercent && (int)fWidth > 1)
	{
		m_guiMid.SetPosition(iXPos,iYPos);
		//m_guiMid.SetHeight(iHeight);
		m_guiMid.SetHeight(m_guiMid.GetTextureHeight());
		m_guiMid.SetWidth((int)fWidth);
		m_guiMid.Render();
		iXPos += (int)fWidth;
	}
	m_guiRight.SetPosition(iXPos,iYPos);
	//m_guiRight.SetHeight(iHeight);
	m_guiRight.SetHeight(m_guiRight.GetTextureHeight());
	m_guiRight.SetWidth(m_guiLeft.GetTextureWidth());
	m_guiRight.Render();
	
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
void CGUIProgressControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_guiBackground.FreeResources();
  m_guiMid.FreeResources();
  m_guiRight.FreeResources();
  m_guiLeft.FreeResources();
}

void CGUIProgressControl::PreAllocResources()
{
	CGUIControl::PreAllocResources();
	m_guiBackground.PreAllocResources();
	m_guiMid.PreAllocResources();
	m_guiRight.PreAllocResources();
	m_guiLeft.PreAllocResources();
}

void CGUIProgressControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground.AllocResources();
  m_guiMid.AllocResources();
  m_guiRight.AllocResources();
  m_guiLeft.AllocResources();
	
	m_guiBackground.SetHeight(25);
	m_guiRight.SetHeight(20);
	m_guiLeft.SetHeight(20);
	m_guiMid.SetHeight(20);
}