#include "include.h"
#include "GUIToggleButtonControl.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIToggleButtonControl::CGUIToggleButtonControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus, const CStdString& strTextureNoFocus, const CStdString& strAltTextureFocus, const CStdString& strAltTextureNoFocus, const CLabelInfo &labelInfo)
    : CGUIButtonControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureFocus, strTextureNoFocus, labelInfo)
    , m_selectButton(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strAltTextureFocus, strAltTextureNoFocus, labelInfo)
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
  if (!IsVisible()) return;

  // NOTE: The not here is due to the togglebutton being around the wrong way
  //       The alt textures are used normally (when !m_bSelected) from the skin,
  //       though the infomanager stuff <usealttexture> is correctly setup.
  bool useAltTextures = !m_bSelected;

  // ask our infoManager whether we are selected or not...
  if (m_toggleSelect)
    useAltTextures = m_bSelected = g_infoManager.GetBool(m_toggleSelect, m_dwParentID);

  if (useAltTextures)
  {
    // render our Alternate textures...
    m_selectButton.SetFocus(HasFocus());
    m_selectButton.SetVisible(IsVisible());
    m_selectButton.SetEnabled(!IsDisabled());
    m_selectButton.SetPulseOnSelect(GetPulseOnSelect());
    m_selectButton.Render();
    CGUIControl::Render();
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

void CGUIToggleButtonControl::SetColourDiffuse(D3DCOLOR colour)
{
  CGUIButtonControl::SetColourDiffuse(colour);
  m_selectButton.SetColourDiffuse(colour);
}

void CGUIToggleButtonControl::SetLabel(const string &strLabel)
{
  CGUIButtonControl::SetLabel(strLabel);
  m_selectButton.SetLabel(strLabel);
}

void CGUIToggleButtonControl::SetAltLabel(const string &label)
{
  if (label.size())
    m_selectButton.SetLabel(label);
}

const string& CGUIToggleButtonControl::GetLabel() const
{
  if (m_bSelected)
    return m_selectButton.GetLabel();
  return CGUIButtonControl::GetLabel();
}

