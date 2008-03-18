#include "include.h"
#include "GUISettingsSliderControl.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUISettingsSliderControl::CGUISettingsSliderControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float sliderWidth, float sliderHeight, const CImage &textureFocus, const CImage &textureNoFocus, const CImage& backGroundTexture, const CImage& nibTexture, const CImage& nibTextureFocus, const CLabelInfo &labelInfo, int iType)
    : CGUISliderControl(dwParentID, dwControlId, posX, posY, sliderWidth, sliderHeight, backGroundTexture, nibTexture,nibTextureFocus, iType)
    , m_buttonControl(dwParentID, dwControlId, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
    , m_textLayout(labelInfo.font, false)
{
  ControlType = GUICONTROL_SETTINGS_SLIDER;
  m_controlOffsetX = 0;  // no offsets for setting sliders
  m_controlOffsetY = 0;
  m_renderText = false;
}

CGUISettingsSliderControl::~CGUISettingsSliderControl(void)
{
}


void CGUISettingsSliderControl::Render()
{
  if (IsDisabled()) return ;

  // make sure the button has focus if it should have...
  m_buttonControl.SetFocus(HasFocus());
  m_buttonControl.Render();
  CGUISliderControl::Render();

  // now render our text
  m_textLayout.Update(CGUISliderControl::GetDescription());

  float posX = m_posX - m_buttonControl.GetLabelInfo().offsetX;
  float posY = GetYPosition() + GetHeight() * 0.5f;
  if (HasFocus() && m_buttonControl.GetLabelInfo().focusedColor)
    m_textLayout.Render(posX, posY, 0, m_buttonControl.GetLabelInfo().focusedColor, m_buttonControl.GetLabelInfo().shadowColor, XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
  else
    m_textLayout.Render(posX, posY, 0, m_buttonControl.GetLabelInfo().textColor, m_buttonControl.GetLabelInfo().shadowColor, XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
}

bool CGUISettingsSliderControl::OnAction(const CAction &action)
{
  return CGUISliderControl::OnAction(action);
}

void CGUISettingsSliderControl::FreeResources()
{
  CGUISliderControl::FreeResources();
  m_buttonControl.FreeResources();
}

void CGUISettingsSliderControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUISliderControl::DynamicResourceAlloc(bOnOff);
  m_buttonControl.DynamicResourceAlloc(bOnOff);
}

void CGUISettingsSliderControl::PreAllocResources()
{
  CGUISliderControl::PreAllocResources();
  m_buttonControl.PreAllocResources();
}

void CGUISettingsSliderControl::AllocResources()
{
  CGUISliderControl::AllocResources();
  m_buttonControl.AllocResources();
}

void CGUISettingsSliderControl::Update()
{
  CGUISliderControl::Update();
}

void CGUISettingsSliderControl::SetPosition(float posX, float posY)
{
  m_buttonControl.SetPosition(posX, posY);
  float sliderPosX = posX + m_buttonControl.GetWidth() - m_width - m_buttonControl.GetLabelInfo().offsetX;
  float sliderPosY = posY + (m_buttonControl.GetHeight() - m_height) * 0.5f;
  CGUISliderControl::SetPosition(sliderPosX, sliderPosY);
}

void CGUISettingsSliderControl::SetWidth(float width)
{
  m_buttonControl.SetWidth(width);
  SetPosition(GetXPosition(), GetYPosition());
}

void CGUISettingsSliderControl::SetHeight(float height)
{
  m_buttonControl.SetHeight(height);
  SetPosition(GetXPosition(), GetYPosition());
}

void CGUISettingsSliderControl::SetEnabled(bool bEnable)
{
  CGUISliderControl::SetEnabled(bEnable);
  m_buttonControl.SetEnabled(bEnable);
}

CStdString CGUISettingsSliderControl::GetDescription() const
{
  return m_buttonControl.GetDescription() + " " + CGUISliderControl::GetDescription();
}

void CGUISettingsSliderControl::SetColorDiffuse(D3DCOLOR color)
{
  m_buttonControl.SetColorDiffuse(color);
  CGUISliderControl::SetColorDiffuse(color);
}
