#include "stdafx.h"
#include "GUIProgressControl.h"


CGUIProgressControl::CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, CStdString& strBackGroundTexture, CStdString& strLeftTexture, CStdString& strMidTexture, CStdString& strRightTexture, CStdString& strOverlayTexture)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
    , m_guiBackground(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strBackGroundTexture)
    , m_guiLeft(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strLeftTexture)
    , m_guiMid(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strMidTexture)
    , m_guiRight(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strRightTexture)
    , m_guiOverlay(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strOverlayTexture)
{
  m_fPercent = 0;
  ControlType = GUICONTROL_PROGRESS;
}

CGUIProgressControl::~CGUIProgressControl(void)
{
}

void CGUIProgressControl::SetPosition(int iPosX, int iPosY)
{
  // everything is positioned based on the background image position
  CGUIControl::SetPosition(iPosX, iPosY);
  m_guiBackground.SetPosition(iPosX, iPosY);
}

void CGUIProgressControl::Render()
{
  if (!IsVisible()) return ;
  if (IsDisabled()) return ;
  float fScaleX, fScaleY;
  fScaleY = m_dwHeight == 0 ? 1.0f : m_dwHeight/(float)m_guiBackground.GetTextureHeight();
  fScaleX = m_dwWidth == 0 ? 1.0f : m_dwWidth/(float)m_guiBackground.GetTextureWidth();

  m_guiBackground.SetHeight((int)(fScaleY*m_guiBackground.GetTextureHeight()));
  m_guiBackground.SetWidth((int)(fScaleX*m_guiBackground.GetTextureWidth()));
  m_guiBackground.Render();

  float fWidth = m_fPercent;
  fWidth /= 100.0f;
  fWidth *= (float) (m_guiBackground.GetTextureWidth() - m_guiLeft.GetTextureWidth() - m_guiRight.GetTextureWidth());

  int iXPos = m_guiBackground.GetXPosition();
  int iYPos = m_guiBackground.GetYPosition();
  int iOffset = abs((int)(fScaleY * 0.5f * (m_guiLeft.GetTextureHeight() - m_guiBackground.GetTextureHeight())));
  if (iOffset > 0)  //  Center texture to the background if necessary
    m_guiLeft.SetPosition(iXPos, iYPos + iOffset);
  else
    m_guiLeft.SetPosition(iXPos, iYPos);
  m_guiLeft.SetHeight((int)(fScaleY*m_guiLeft.GetTextureHeight()));
  m_guiLeft.SetWidth((int)(fScaleX*m_guiLeft.GetTextureWidth()));
  m_guiLeft.Render();

  iXPos += (int)(fScaleX*m_guiLeft.GetTextureWidth());
  if (m_fPercent && (int)fWidth > 1)
  {
    int iOffset = abs((int)(fScaleY * 0.5f * (m_guiMid.GetTextureHeight() - m_guiBackground.GetTextureHeight())));
    if (iOffset > 0)  //  Center texture to the background if necessary
      m_guiMid.SetPosition(iXPos, iYPos + iOffset);
    else
      m_guiMid.SetPosition(iXPos, iYPos);
    m_guiMid.SetHeight((int)(fScaleY * m_guiMid.GetTextureHeight()));
    m_guiMid.SetWidth((int)(fScaleX * fWidth));
    m_guiMid.Render();
    iXPos += (int)(fWidth * fScaleY);
  }

  iOffset = abs((int)(fScaleY * 0.5f * (m_guiRight.GetTextureHeight() - m_guiBackground.GetTextureHeight())));
  if (iOffset > 0)  //  Center texture to the background if necessary
    m_guiRight.SetPosition(iXPos, iYPos + iOffset);
  else
    m_guiRight.SetPosition(iXPos, iYPos);
  m_guiRight.SetHeight((int)(fScaleY * m_guiRight.GetTextureHeight()));
  m_guiRight.SetWidth((int)(fScaleX * m_guiRight.GetTextureWidth()));
  m_guiRight.Render();

  iOffset = abs((int)(fScaleY * 0.5f * (m_guiOverlay.GetTextureHeight() - m_guiBackground.GetTextureHeight())));
  if (iOffset > 0)  //  Center texture to the background if necessary
    m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(), m_guiBackground.GetYPosition() + iOffset);
  else
    m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(), m_guiBackground.GetYPosition());
  m_guiOverlay.SetHeight((int)(fScaleY * m_guiOverlay.GetTextureHeight()));
  m_guiOverlay.SetWidth((int)(fScaleX * m_guiOverlay.GetTextureWidth()));
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

void CGUIProgressControl::SetPercentage(float fPercent)
{
  m_fPercent = fPercent;
}

float CGUIProgressControl::GetPercentage() const
{
  return m_fPercent;
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

void CGUIProgressControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_guiBackground.DynamicResourceAlloc(bOnOff);
  m_guiMid.DynamicResourceAlloc(bOnOff);
  m_guiRight.DynamicResourceAlloc(bOnOff);
  m_guiLeft.DynamicResourceAlloc(bOnOff);
  m_guiOverlay.DynamicResourceAlloc(bOnOff);
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
