#include "include.h"
#include "GUISpinControlEx.h"
#include "../xbmc/Util.h"


CGUISpinControlEx::CGUISpinControlEx(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float spinWidth, float spinHeight, const CLabelInfo& spinInfo, const CImage &textureFocus, const CImage &textureNoFocus, const CImage& textureUp, const CImage& textureDown, const CImage& textureUpFocus, const CImage& textureDownFocus, const CLabelInfo& labelInfo, int iType)
    : CGUISpinControl(dwParentID, dwControlId, posX, posY, spinWidth, spinHeight, textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, iType)
    , m_buttonControl(dwParentID, dwControlId, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
{
  ControlType = GUICONTROL_SPINEX;
  m_spinPosX = 0;
}

CGUISpinControlEx::~CGUISpinControlEx(void)
{
}

void CGUISpinControlEx::PreAllocResources()
{
  CGUISpinControl::PreAllocResources();
  m_buttonControl.PreAllocResources();
}

void CGUISpinControlEx::AllocResources()
{
  // Correct alignment - we always align the spincontrol on the right,
  // and we always use a negative offsetX
  m_label.align = (m_label.align & 4) | XBFONT_RIGHT;
  if (m_label.offsetX > 0)
    m_label.offsetX = -m_label.offsetX;
  CGUISpinControl::AllocResources();
  m_buttonControl.AllocResources();
  SetPosition(GetXPosition(), GetYPosition());
  if (m_height == 0)
    m_height = GetSpinHeight();
}

void CGUISpinControlEx::FreeResources()
{
  CGUISpinControl::FreeResources();
  m_buttonControl.FreeResources();
}

void CGUISpinControlEx::DynamicResourceAlloc(bool bOnOff)
{
  CGUISpinControl::DynamicResourceAlloc(bOnOff);
  m_buttonControl.DynamicResourceAlloc(bOnOff);
}

void CGUISpinControlEx::Render()
{
  // make sure the button has focus if it should have...
  m_buttonControl.SetFocus(HasFocus());
  m_buttonControl.Render();
  CGUISpinControl::Render();
}

void CGUISpinControlEx::SetPosition(float posX, float posY)
{
  m_buttonControl.SetPosition(posX, posY);
  float spinPosX = posX + m_buttonControl.GetWidth() - GetSpinWidth() * 2 - (m_spinPosX ? m_spinPosX : m_buttonControl.GetLabelInfo().offsetX);
  float spinPosY = posY + (m_buttonControl.GetHeight() - GetSpinHeight()) * 0.5f;
  CGUISpinControl::SetPosition(spinPosX, spinPosY);
}

void CGUISpinControlEx::SetWidth(float width)
{
  m_buttonControl.SetWidth(width);
  SetPosition(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition());
}

void CGUISpinControlEx::SetHeight(float height)
{
  m_buttonControl.SetHeight(height);
  SetPosition(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition());
}

void CGUISpinControlEx::SetVisible(bool bVisible)
{
  m_buttonControl.SetVisible(bVisible);
  CGUISpinControl::SetVisible(bVisible);
}

void CGUISpinControlEx::SetColorDiffuse(D3DCOLOR color)
{
  m_buttonControl.SetColorDiffuse(color);
  CGUISpinControl::SetColorDiffuse(color);
}

void CGUISpinControlEx::SetEnabled(bool bEnable)
{
  m_buttonControl.SetEnabled(bEnable);
  CGUISpinControl::SetEnabled(bEnable);
}

const CStdString CGUISpinControlEx::GetCurrentLabel() const
{
  return CGUISpinControl::GetLabel();
}

CStdString CGUISpinControlEx::GetDescription() const
{
  CStdString strLabel;
  strLabel.Format("%s (%s)", m_buttonControl.GetDescription(), GetLabel());
  return strLabel;
}

void CGUISpinControlEx::SettingsCategorySetSpinTextColor(D3DCOLOR color)
{
  m_label.textColor = color;
  m_label.focusedColor = color;
}

void CGUISpinControlEx::SetSpinPosition(float spinPosX)
{
  m_spinPosX = spinPosX;
  SetPosition(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition());
}
