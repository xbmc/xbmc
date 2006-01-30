#include "include.h"
#include "GUISpinControlEx.h"
#include "../xbmc/Util.h"


CGUISpinControlEx::CGUISpinControlEx(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, DWORD dwSpinWidth, DWORD dwSpinHeight, DWORD dwSpinColor, const CStdString &strFocus, const CStdString &strNoFocus, const CStdString& strUp, const CStdString& strDown, const CStdString& strUpFocus, const CStdString& strDownFocus, const CLabelInfo& labelInfo, int iType)
    : CGUISpinControl(dwParentID, dwControlId, iPosX, iPosY, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, labelInfo, dwSpinColor, iType)
    , m_buttonControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strFocus, strNoFocus, labelInfo)
{
  ControlType = GUICONTROL_SPINEX;
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
  if (m_dwHeight == 0)
    m_dwHeight = GetSpinHeight();
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
  if (!IsVisible()) return;

  // make sure the button has focus if it should have...
  m_buttonControl.SetFocus(HasFocus());
  m_buttonControl.Render();
  CGUISpinControl::Render();
}

void CGUISpinControlEx::SetPosition(int iPosX, int iPosY)
{
  m_buttonControl.SetPosition(iPosX, iPosY);
  int iSpinPosX = iPosX + (int)m_buttonControl.GetWidth() - (int)GetSpinWidth() * 2 - (int)m_buttonControl.GetLabelInfo().offsetX;
  int iSpinPosY = iPosY + ((int)m_buttonControl.GetHeight() - (int)GetSpinHeight()) / 2;
  CGUISpinControl::SetPosition(iSpinPosX, iSpinPosY);
}

void CGUISpinControlEx::SetWidth(int iWidth)
{
  m_buttonControl.SetWidth(iWidth);
  SetPosition(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition());
}

void CGUISpinControlEx::SetHeight(int iHeight)
{
  m_buttonControl.SetHeight(iHeight);
  SetPosition(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition());
}

void CGUISpinControlEx::SetVisible(bool bVisible)
{
  m_buttonControl.SetVisible(bVisible);
  CGUISpinControl::SetVisible(bVisible);
}

void CGUISpinControlEx::SetColourDiffuse(D3DCOLOR color)
{
  m_buttonControl.SetColourDiffuse(color);
  CGUISpinControl::SetColourDiffuse(color);
}

void CGUISpinControlEx::SetEnabled(bool bEnable)
{
  m_buttonControl.SetEnabled(bEnable);
  CGUISpinControl::SetEnabled(bEnable);
}

const CStdString CGUISpinControlEx::GetCurrentLabel() const
{
  CStdString strLabel;
  CUtil::Unicode2Ansi(CGUISpinControl::GetLabel(), strLabel);
  return strLabel;
}

CStdString CGUISpinControlEx::GetDescription() const
{
  CStdString strLabel;
  strLabel.Format("%s (%s)", m_buttonControl.GetDescription(), GetCurrentLabel());
  return strLabel;
}

void CGUISpinControlEx::SettingsCategorySetSpinTextColor(D3DCOLOR color)
{
  m_label.textColor = color;
}
