#include "include.h"
#include "GUISettingsSliderControl.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUISettingsSliderControl::CGUISettingsSliderControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, DWORD dwSliderWidth, DWORD dwSliderHeight, const CStdString &strFocus, const CStdString &strNoFocus, const CStdString& strBackGroundTexture, const CStdString& strMidTexture, const CStdString& strMidTextureFocus, const CLabelInfo &labelInfo, int iType)
    : CGUISliderControl(dwParentID, dwControlId, iPosX, iPosY, dwSliderWidth, dwSliderHeight, strBackGroundTexture, strMidTexture, strMidTextureFocus, iType)
    , m_buttonControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strFocus, strNoFocus, labelInfo)
{
  ControlType = GUICONTROL_SETTINGS_SLIDER;
  m_iControlOffsetX = 0;  // no offsets for setting sliders
  m_iControlOffsetY = 0;
  m_renderText = false;
}

CGUISettingsSliderControl::~CGUISettingsSliderControl(void)
{
}


void CGUISettingsSliderControl::Render()
{
  if (!UpdateEffectState()) return ;
  if (IsDisabled()) return ;

  // make sure the button has focus if it should have...
  m_buttonControl.SetFocus(HasFocus());
  m_buttonControl.Render();
  CGUISliderControl::Render();

  // now render our text
  CGUIFont *pFont = m_buttonControl.GetLabelInfo().font;
  if (pFont)
  {
    CStdStringW strOut = CGUISliderControl::GetDescription();
    float unneeded, height;
    pFont->GetTextExtent(strOut.c_str(), &unneeded, &height);
    float fPosY = (float)GetYPosition() + ((float)GetHeight() - height) / 2;
    pFont->DrawText((float)m_iPosX - m_buttonControl.GetLabelInfo().offsetX, fPosY, m_buttonControl.GetLabelInfo().textColor, m_buttonControl.GetLabelInfo().shadowColor, strOut.c_str(), XBFONT_RIGHT);
  }
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

void CGUISettingsSliderControl::SetPosition(int iPosX, int iPosY)
{
  m_buttonControl.SetPosition(iPosX, iPosY);
  int iSliderPosX = iPosX + m_buttonControl.GetWidth() - m_dwWidth - m_buttonControl.GetLabelInfo().offsetX;
  int iSliderPosY = iPosY + (m_buttonControl.GetHeight() - m_dwHeight) / 2;
  CGUISliderControl::SetPosition(iSliderPosX, iSliderPosY);
}

void CGUISettingsSliderControl::SetWidth(int iWidth)
{
  m_buttonControl.SetWidth(iWidth);
  SetPosition(GetXPosition(), GetYPosition());
}

void CGUISettingsSliderControl::SetHeight(int iHeight)
{
  m_buttonControl.SetHeight(iHeight);
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
