#include "stdafx.h"
#include "GUIProgressControl.h"


CGUIProgressControl::CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, CStdString& strBackGroundTexture,CStdString& strLeftTexture,CStdString& strMidTexture,CStdString& strRightTexture, CStdString& strOverlayTexture)
:CGUIControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight)
,m_guiBackground(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight,strBackGroundTexture)
,m_guiLeft(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight,strLeftTexture)
,m_guiMid(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight,strMidTexture)
,m_guiRight(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight,strRightTexture)
,m_guiOverlay(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight,strOverlayTexture)
{
	m_iPercent=0;
	ControlType = GUICONTROL_PROGRESS;
}

CGUIProgressControl::~CGUIProgressControl(void)
{
}


void CGUIProgressControl::Render()
{
	if (!IsVisible()) return;
	if (IsDisabled()) return;

	int iHeight=m_guiBackground.GetTextureHeight();
	m_guiBackground.Render();
	m_guiBackground.SetHeight(iHeight);
  
	float fWidth = (float)m_iPercent;
	fWidth/=100.0f;
	fWidth *= (float) (m_guiBackground.GetTextureWidth() - m_guiLeft.GetTextureWidth() - m_guiRight.GetTextureWidth());

	int iXPos=m_guiBackground.GetXPosition();
	int iYPos=m_guiBackground.GetYPosition();
  int iOffset=abs((m_guiLeft.GetTextureHeight()-m_guiBackground.GetTextureHeight())/2);
  if (iOffset>0)  //  Center texture to the background if necessary
    m_guiLeft.SetPosition(iXPos,iYPos+iOffset);
  else
    m_guiLeft.SetPosition(iXPos,iYPos);
  m_guiLeft.SetHeight(m_guiLeft.GetTextureHeight());
	m_guiLeft.SetWidth(m_guiLeft.GetTextureWidth());
	m_guiLeft.Render();

	iXPos += m_guiLeft.GetTextureWidth();
	if (m_iPercent && (int)fWidth > 1)
	{
    int iOffset=abs((m_guiMid.GetTextureHeight()-m_guiBackground.GetTextureHeight())/2);
    if (iOffset>0)  //  Center texture to the background if necessary
      m_guiMid.SetPosition(iXPos,iYPos+iOffset);
    else
      m_guiMid.SetPosition(iXPos,iYPos);
		m_guiMid.SetHeight(m_guiMid.GetTextureHeight());
		m_guiMid.SetWidth((int)fWidth);
		m_guiMid.Render();
		iXPos += (int)fWidth;
	}

  iOffset=abs((m_guiRight.GetTextureHeight()-m_guiBackground.GetTextureHeight())/2);
  if (iOffset>0)  //  Center texture to the background if necessary
    m_guiRight.SetPosition(iXPos,iYPos+iOffset);
  else
    m_guiRight.SetPosition(iXPos,iYPos);
	m_guiRight.SetHeight(m_guiRight.GetTextureHeight());
	m_guiRight.SetWidth(m_guiLeft.GetTextureWidth());
	m_guiRight.Render();
	
  iOffset=abs((m_guiOverlay.GetTextureHeight()-m_guiBackground.GetTextureHeight())/2);
  if (iOffset>0)  //  Center texture to the background if necessary
    m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(),m_guiBackground.GetYPosition()+iOffset);
  else
    m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(),m_guiBackground.GetYPosition());
  m_guiOverlay.SetHeight(m_guiOverlay.GetTextureHeight());
	m_guiOverlay.SetWidth(m_guiOverlay.GetTextureWidth());
	m_guiOverlay.Render();
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
  m_guiOverlay.FreeResources();
}

void CGUIProgressControl::PreAllocResources()
{
	CGUIControl::PreAllocResources();
	m_guiBackground.PreAllocResources();
	m_guiMid.PreAllocResources();
	m_guiRight.PreAllocResources();
	m_guiLeft.PreAllocResources();
  m_guiOverlay.PreAllocResources();
}

void CGUIProgressControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground.AllocResources();
  m_guiMid.AllocResources();
  m_guiRight.AllocResources();
  m_guiLeft.AllocResources();
  m_guiOverlay.AllocResources();
	
	m_guiBackground.SetHeight(25);
	m_guiRight.SetHeight(20);
	m_guiLeft.SetHeight(20);
	m_guiMid.SetHeight(20);
	m_guiOverlay.SetHeight(20);
}