#include "include.h"
#include "GUIToggleButtonControl.h"
#include "GUIFontManager.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "GUIFontManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// NB: This is backwards.  Really, the Alt textures should be rendered when selected.
// Currently they are rendered when NOT selected.  (Normal textures rendered when selected)
CGUIToggleButtonControl::CGUIToggleButtonControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus, const CStdString& strTextureNoFocus, const CStdString& strAltTextureFocus, const CStdString& strAltTextureNoFocus, DWORD dwTextXOffset, DWORD dwTextYOffset, DWORD dwAlign)
    : CGUIButtonControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strAltTextureFocus, strAltTextureNoFocus, dwTextXOffset, dwTextYOffset, dwAlign)
    , m_selectButton(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureFocus, strTextureNoFocus, dwTextXOffset, dwTextYOffset, dwAlign)
{
  m_bSelected = false;
  m_toggleSelect = 0;
  ControlType = GUICONTROL_TOGGLEBUTTON;
}

CGUIToggleButtonControl::~CGUIToggleButtonControl(void)
{
}

void CGUIToggleButtonControl::Render()
{
  if (!UpdateVisibility() ) return ;

  // ask our infoManager whether we are selected or not...
  if (m_toggleSelect)
    m_bSelected = !g_infoManager.GetBool(m_toggleSelect);

  if (m_bSelected)
  { // render our Alt textures...
    m_selectButton.SetFocus(HasFocus());
    m_selectButton.SetVisible(IsVisible());
    m_selectButton.SetEnabled(!IsDisabled());
    m_selectButton.SetPulseOnSelect(GetPulseOnSelect());
    m_selectButton.Render();
  }
  else
  { // render our Normal textures...
    CGUIButtonControl::Render();
  }
}

bool CGUIToggleButtonControl::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    m_bSelected = !m_bSelected;
  }
  return CGUIButtonControl::OnAction(action);
}

void CGUIToggleButtonControl::PreAllocResources()
{
  CGUIButtonControl::PreAllocResources();
  m_selectButton.PreAllocResources();
}

void CGUIToggleButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();
  m_selectButton.AllocResources();
}

void CGUIToggleButtonControl::FreeResources()
{
  CGUIButtonControl::FreeResources();
  m_selectButton.FreeResources();
}

void CGUIToggleButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIButtonControl::DynamicResourceAlloc(bOnOff);
  m_selectButton.DynamicResourceAlloc(bOnOff);
}

void CGUIToggleButtonControl::Update()
{
  CGUIButtonControl::Update();
  m_selectButton.Update();
}

void CGUIToggleButtonControl::SetPosition(int iPosX, int iPosY)
{
  CGUIButtonControl::SetPosition(iPosX, iPosY);
  m_selectButton.SetPosition(iPosX, iPosY);
}

void CGUIToggleButtonControl::SetWidth(int iWidth)
{
  CGUIButtonControl::SetWidth(iWidth);
  m_selectButton.SetWidth(iWidth);
}

void CGUIToggleButtonControl::SetHeight(int iHeight)
{
  CGUIButtonControl::SetHeight(iHeight);
  m_selectButton.SetHeight(iHeight);
}

void CGUIToggleButtonControl::SetAlpha(DWORD dwAlpha)
{
  CGUIButtonControl::SetAlpha(dwAlpha);
  m_selectButton.SetAlpha(dwAlpha);
}

void CGUIToggleButtonControl::SetColourDiffuse(D3DCOLOR colour)
{
  CGUIButtonControl::SetColourDiffuse(colour);
  m_selectButton.SetColourDiffuse(colour);
}

void CGUIToggleButtonControl::SetDisabledColor(D3DCOLOR color)
{
  CGUIButtonControl::SetDisabledColor(color);
  m_selectButton.SetDisabledColor(color);
}

void CGUIToggleButtonControl::SetLabel(const CStdString& strFontName, const CStdString& strLabel, D3DCOLOR dwColor)
{
  CGUIButtonControl::SetLabel(strFontName, strLabel, dwColor);
  m_selectButton.SetLabel(strFontName, strLabel, dwColor);
}

void CGUIToggleButtonControl::SetLabel(const CStdString& strFontName, const wstring& strLabel, D3DCOLOR dwColor)
{
  CGUIButtonControl::SetLabel(strFontName, strLabel, dwColor);
  m_selectButton.SetLabel(strFontName, strLabel, dwColor);
}
