#include "include.h"
#include "GUIProgressControl.h"
#include "../xbmc/utils/GUIInfoManager.h"



CGUIProgressControl::CGUIProgressControl(DWORD dwParentID, DWORD dwControlId, 
                                         float posX, float posY, float width, 
                                         float height, const CImage& backGroundTexture, 
                                         const CImage& leftTexture, 
                                         const CImage& midTexture, 
                                         const CImage& rightTexture, 
                                         const CImage& overlayTexture, float min, float max)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_guiBackground(dwParentID, dwControlId, posX, posY, width, height, backGroundTexture)
    , m_guiLeft(dwParentID, dwControlId, posX, posY, width, height, leftTexture)
    , m_guiMid(dwParentID, dwControlId, posX, posY, width, height, midTexture)
    , m_guiRight(dwParentID, dwControlId, posX, posY, width, height, rightTexture)
    , m_guiOverlay(dwParentID, dwControlId, posX, posY, width, height, overlayTexture)
{
  m_RangeMin = min;
  m_RangeMax = max;
  m_fPercent = 0;
  m_iInfoCode = 0;
  ControlType = GUICONTROL_PROGRESS;
}

CGUIProgressControl::~CGUIProgressControl(void)
{
}

void CGUIProgressControl::SetPosition(float posX, float posY)
{
  // everything is positioned based on the background image position
  CGUIControl::SetPosition(posX, posY);
  m_guiBackground.SetPosition(posX, posY);
}

void CGUIProgressControl::Render()
{
  if (!IsDisabled())
  {
    if (m_iInfoCode )
    {
      m_fPercent = (float)g_infoManager.GetInt(m_iInfoCode);
      if ((m_RangeMax - m_RangeMin)> 0 && (m_RangeMax != 100 && m_RangeMin != 0) )
      {
        if (m_fPercent > m_RangeMax)
          m_fPercent = m_RangeMax;
        if (m_fPercent < m_RangeMin) 
          m_fPercent = m_RangeMin;
        m_fPercent = ((100*(m_fPercent - m_RangeMin)) / (m_RangeMax - m_RangeMin));
      }
    }
    if (m_fPercent < 0.0f) m_fPercent = 0.0f;
    if (m_fPercent > 100.0f) m_fPercent = 100.0f;

    float fScaleX, fScaleY;
    fScaleY = m_height == 0 ? 1.0f : m_height/(float)m_guiBackground.GetTextureHeight();
    fScaleX = m_width == 0 ? 1.0f : m_width/(float)m_guiBackground.GetTextureWidth();

    m_guiBackground.SetHeight(fScaleY*m_guiBackground.GetTextureHeight());
    m_guiBackground.SetWidth(fScaleX*m_guiBackground.GetTextureWidth());
    m_guiBackground.Render();

    float fWidth = m_fPercent;
    fWidth /= 100.0f;
    fWidth *= m_guiBackground.GetTextureWidth() - m_guiLeft.GetTextureWidth() - m_guiRight.GetTextureWidth();

    float posX = m_guiBackground.GetXPosition();
    float posY = m_guiBackground.GetYPosition();
    float offset = fabs(fScaleY * 0.5f * (m_guiLeft.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
    if (offset > 0)  //  Center texture to the background if necessary
      m_guiLeft.SetPosition(posX, posY + offset);
    else
      m_guiLeft.SetPosition(posX, posY);
    m_guiLeft.SetHeight(fScaleY*m_guiLeft.GetTextureHeight());
    m_guiLeft.SetWidth(fScaleX*m_guiLeft.GetTextureWidth());
    m_guiLeft.Render();

    posX += fScaleX*m_guiLeft.GetTextureWidth();
    if (m_fPercent && (int)fWidth > 1)
    {
      float offset = fabs(fScaleY * 0.5f * (m_guiMid.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
      if (offset > 0)  //  Center texture to the background if necessary
        m_guiMid.SetPosition(posX, posY + offset);
      else
        m_guiMid.SetPosition(posX, posY);
      m_guiMid.SetHeight(fScaleY * m_guiMid.GetTextureHeight());
      m_guiMid.SetWidth(fScaleX * fWidth);
      m_guiMid.Render();
      posX += fWidth * fScaleX;
    }

    offset = fabs(fScaleY * 0.5f * (m_guiRight.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
    if (offset > 0)  //  Center texture to the background if necessary
      m_guiRight.SetPosition(posX, posY + offset);
    else
      m_guiRight.SetPosition(posX, posY);
    m_guiRight.SetHeight(fScaleY * m_guiRight.GetTextureHeight());
    m_guiRight.SetWidth(fScaleX * m_guiRight.GetTextureWidth());
    m_guiRight.Render();

    offset = fabs(fScaleY * 0.5f * (m_guiOverlay.GetTextureHeight() - m_guiBackground.GetTextureHeight()));
    if (offset > 0)  //  Center texture to the background if necessary
      m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(), m_guiBackground.GetYPosition() + offset);
    else
      m_guiOverlay.SetPosition(m_guiBackground.GetXPosition(), m_guiBackground.GetYPosition());
    m_guiOverlay.SetHeight(fScaleY * m_guiOverlay.GetTextureHeight());
    m_guiOverlay.SetWidth(fScaleX * m_guiOverlay.GetTextureWidth());
    m_guiOverlay.Render();
  }
  CGUIControl::Render();
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

void CGUIProgressControl::SetInfo(int iInfo)
{
  m_iInfoCode = iInfo;
}

void CGUIProgressControl::SetColorDiffuse(D3DCOLOR color)
{
  CGUIControl::SetColorDiffuse(color);
  m_guiBackground.SetColorDiffuse(color);
  m_guiRight.SetColorDiffuse(color);
  m_guiLeft.SetColorDiffuse(color);
  m_guiMid.SetColorDiffuse(color);
  m_guiOverlay.SetColorDiffuse(color);
}

