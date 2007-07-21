#include "include.h"
#include "GUIRadioButtonControl.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIRadioButtonControl::CGUIRadioButtonControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
    const CImage& textureFocus, const CImage& textureNoFocus,
    const CLabelInfo& labelInfo,
    const CImage& radioFocus, const CImage& radioNoFocus)
    : CGUIButtonControl(dwParentID, dwControlId, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
    , m_imgRadioFocus(dwParentID, dwControlId, posX, posY, 16, 16, radioFocus)
    , m_imgRadioNoFocus(dwParentID, dwControlId, posX, posY, 16, 16, radioNoFocus)
{
  m_radioPosX = 0;
  m_radioPosY = 0;
  m_toggleSelect = 0;
  m_imgRadioFocus.SetAspectRatio(CGUIImage::ASPECT_RATIO_KEEP);
  m_imgRadioNoFocus.SetAspectRatio(CGUIImage::ASPECT_RATIO_KEEP);
  ControlType = GUICONTROL_RADIO;
}

CGUIRadioButtonControl::~CGUIRadioButtonControl(void)
{}


void CGUIRadioButtonControl::Render()
{
  CGUIButtonControl::Render();

  // ask our infoManager whether we are selected or not...
  if (m_toggleSelect)
    m_bSelected = g_infoManager.GetBool(m_toggleSelect, m_dwParentID);

  if ( IsSelected() && !IsDisabled() )
    m_imgRadioFocus.Render();
  else
    m_imgRadioNoFocus.Render();
}

bool CGUIRadioButtonControl::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    m_bSelected = !m_bSelected;
  }
  return CGUIButtonControl::OnAction(action);
}

bool CGUIRadioButtonControl::OnMessage(CGUIMessage& message)
{
  return CGUIButtonControl::OnMessage(message);
}

void CGUIRadioButtonControl::PreAllocResources()
{
  CGUIButtonControl::PreAllocResources();
  m_imgRadioFocus.PreAllocResources();
  m_imgRadioNoFocus.PreAllocResources();
}

void CGUIRadioButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();
  m_imgRadioFocus.AllocResources();
  m_imgRadioNoFocus.AllocResources();

  SetPosition(m_posX, m_posY);
}

void CGUIRadioButtonControl::FreeResources()
{
  CGUIButtonControl::FreeResources();
  m_imgRadioFocus.FreeResources();
  m_imgRadioNoFocus.FreeResources();
}

void CGUIRadioButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgRadioFocus.DynamicResourceAlloc(bOnOff);
  m_imgRadioNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIRadioButtonControl::Update()
{
  CGUIButtonControl::Update();

}

void CGUIRadioButtonControl::SetPosition(float posX, float posY)
{
  CGUIButtonControl::SetPosition(posX, posY);
  float radioPosX = m_radioPosX ? m_radioPosX : (m_posX + m_width - 8) - m_imgRadioFocus.GetWidth();
  float radioPosY = m_radioPosY ? m_radioPosY : m_posY + (m_height - m_imgRadioFocus.GetHeight()) / 2;
  m_imgRadioFocus.SetPosition(radioPosX, radioPosY);
  m_imgRadioNoFocus.SetPosition(radioPosX, radioPosY);
}

void CGUIRadioButtonControl::SetRadioDimensions(float posX, float posY, float width, float height)
{
  m_radioPosX = posX;
  m_radioPosY = posY;
  if (width)
  {
    m_imgRadioFocus.SetWidth(width);
    m_imgRadioNoFocus.SetWidth(width);
  }
  if (height)
  {
    m_imgRadioFocus.SetHeight(height);
    m_imgRadioNoFocus.SetHeight(height);
  }
  SetPosition(GetXPosition(), GetYPosition());
}

void CGUIRadioButtonControl::SetWidth(float width)
{
  CGUIButtonControl::SetWidth(width);
  SetPosition(GetXPosition(), GetYPosition());
}

void CGUIRadioButtonControl::SetHeight(float height)
{
  CGUIButtonControl::SetHeight(height);
  SetPosition(GetXPosition(), GetYPosition());
}

CStdString CGUIRadioButtonControl::GetDescription() const
{
  CStdString strLabel = CGUIButtonControl::GetDescription();
  if (m_bSelected)
    strLabel += " (*)";
  else
    strLabel += " ( )";
  return strLabel;
}

void CGUIRadioButtonControl::SetColorDiffuse(D3DCOLOR color)
{
  CGUIButtonControl::SetColorDiffuse(color);
  m_imgRadioFocus.SetColorDiffuse(color);
  m_imgRadioNoFocus.SetColorDiffuse(color);
}

