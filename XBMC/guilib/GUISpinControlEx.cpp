#include "stdafx.h"
#include "GUISpinControlEx.h"
#include "../xbmc/Util.h"

CGUISpinControlEx::CGUISpinControlEx(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, DWORD dwSpinWidth, DWORD dwSpinHeight, const CStdString &strFocus, const CStdString &strNoFocus, const CStdString& strUp, const CStdString& strDown, const CStdString& strUpFocus, const CStdString& strDownFocus, const CStdString& strFont, DWORD dwTextColor, int iTextXOffset, int iTextYOffset, DWORD dwAlign, int iType)
:CGUISpinControl(dwParentID, dwControlId, iPosX, iPosY, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, strFont, dwTextColor, iType, XBFONT_RIGHT)
,m_buttonControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strFocus, strNoFocus, iTextXOffset, iTextYOffset, dwAlign)
{
	m_buttonControl.SetLabel(strFont, CStdString(""), dwTextColor);
	SetTextOffsetX(-iTextXOffset);	// offsets are always added for the spincontrol - even if it's right aligned!
	SetTextOffsetY(iTextYOffset);
	SetAlignmentY(dwAlign & XBFONT_CENTER_Y);
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

void CGUISpinControlEx::Render()
{
	// make sure the button has focus if it should have...
	m_buttonControl.SetFocus(HasFocus());
	m_buttonControl.Render();
	CGUISpinControl::Render();
}

void CGUISpinControlEx::SetPosition(int iPosX, int iPosY)
{
	m_buttonControl.SetPosition(iPosX, iPosY);
	int iSpinPosX = iPosX + (int)m_buttonControl.GetWidth() - (int)GetSpinWidth()*2 - (int)m_buttonControl.GetTextOffsetX();
	int iSpinPosY = iPosY + ((int)m_buttonControl.GetHeight() - (int)GetSpinHeight())/2;
  CGUISpinControl::SetPosition(iSpinPosX, iSpinPosY);
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

void CGUISpinControlEx::SetSpinTextColor(D3DCOLOR color)
{
	CGUISpinControl::SetTextColor(color);
}

void CGUISpinControlEx::SetDisabledColor(D3DCOLOR color)
{
	m_buttonControl.SetDisabledColor(color);
	CGUISpinControl::SetDisabledColor(color);
}

void CGUISpinControlEx::SetEnabled(bool bEnable)
{
	m_buttonControl.SetEnabled(bEnable);
/*	if (bEnable)
		CGUISpinControl::SetDisabledColor(GetTextColor());
	else
		CGUISpinControl::SetDisabledColor(m_buttonControl.GetDisabledColor());*/
	CGUISpinControl::SetEnabled(bEnable);
}

const CStdString CGUISpinControlEx::GetCurrentLabel() const
{
	CStdString strLabel;
	CUtil::Unicode2Ansi(CGUISpinControl::GetLabel(), strLabel);
	return strLabel;
}
