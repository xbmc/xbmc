/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIToggleButtonControl.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "GUIInfoManager.h"
#include "input/Key.h"

CGUIToggleButtonControl::CGUIToggleButtonControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CTextureInfo& altTextureFocus, const CTextureInfo& altTextureNoFocus, const CLabelInfo &labelInfo, bool wrapMultiLine)
    : CGUIButtonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo, wrapMultiLine)
    , m_selectButton(parentID, controlID, posX, posY, width, height, altTextureFocus, altTextureNoFocus, labelInfo, wrapMultiLine)
{
  ControlType = GUICONTROL_TOGGLEBUTTON;
}

CGUIToggleButtonControl::~CGUIToggleButtonControl(void)
{
}

void CGUIToggleButtonControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // ask our infoManager whether we are selected or not...
  if (m_toggleSelect)
    m_bSelected = m_toggleSelect->Get();

  if (m_bSelected)
  {
    // render our Alternate textures...
    m_selectButton.SetFocus(HasFocus());
    m_selectButton.SetVisible(IsVisible());
    m_selectButton.SetEnabled(!IsDisabled());
    m_selectButton.SetPulseOnSelect(m_pulseOnSelect);
    ProcessToggle(currentTime);
    m_selectButton.DoProcess(currentTime, dirtyregions);
  }
  else
    CGUIButtonControl::Process(currentTime, dirtyregions);
}

void CGUIToggleButtonControl::ProcessToggle(unsigned int currentTime)
{
  bool changed = false;

  changed |= m_label.SetMaxRect(m_posX, m_posY, GetWidth(), m_height);
  changed |= m_label.SetText(GetDescription());
  changed |= m_label.SetColor(GetTextColor());
  changed |= m_label.SetScrolling(HasFocus());
  changed |= m_label.Process(currentTime);

  if (changed)
    MarkDirtyRegion();
}

void CGUIToggleButtonControl::Render()
{
  if (m_bSelected)
  {
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
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    m_bSelected = !m_bSelected;
    SetInvalid();
  }
  return CGUIButtonControl::OnAction(action);
}

void CGUIToggleButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();
  m_selectButton.AllocResources();
}

void CGUIToggleButtonControl::FreeResources(bool immediately)
{
  CGUIButtonControl::FreeResources(immediately);
  m_selectButton.FreeResources(immediately);
}

void CGUIToggleButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIButtonControl::DynamicResourceAlloc(bOnOff);
  m_selectButton.DynamicResourceAlloc(bOnOff);
}

void CGUIToggleButtonControl::SetInvalid()
{
  CGUIButtonControl::SetInvalid();
  m_selectButton.SetInvalid();
}

void CGUIToggleButtonControl::SetPosition(float posX, float posY)
{
  CGUIButtonControl::SetPosition(posX, posY);
  m_selectButton.SetPosition(posX, posY);
}

void CGUIToggleButtonControl::SetWidth(float width)
{
  CGUIButtonControl::SetWidth(width);
  m_selectButton.SetWidth(width);
}

void CGUIToggleButtonControl::SetHeight(float height)
{
  CGUIButtonControl::SetHeight(height);
  m_selectButton.SetHeight(height);
}

void CGUIToggleButtonControl::SetMinWidth(float minWidth)
{
  CGUIButtonControl::SetMinWidth(minWidth);
  m_selectButton.SetMinWidth(minWidth);
}

bool CGUIToggleButtonControl::UpdateColors()
{
  bool changed = CGUIButtonControl::UpdateColors();
  changed |= m_selectButton.SetColorDiffuse(m_diffuseColor);
  changed |= m_selectButton.UpdateColors();

  return changed;
}

void CGUIToggleButtonControl::SetLabel(const std::string &label)
{
  CGUIButtonControl::SetLabel(label);
  m_selectButton.SetLabel(label);
}

void CGUIToggleButtonControl::SetAltLabel(const std::string &label)
{
  if (label.size())
    m_selectButton.SetLabel(label);
}

std::string CGUIToggleButtonControl::GetDescription() const
{
  if (m_bSelected)
    return m_selectButton.GetDescription();
  return CGUIButtonControl::GetDescription();
}

void CGUIToggleButtonControl::SetAltClickActions(const CGUIAction &clickActions)
{
  m_selectButton.SetClickActions(clickActions);
}

void CGUIToggleButtonControl::OnClick()
{
  // the ! is here as m_bSelected gets updated before this is called
  if (!m_bSelected && m_selectButton.HasClickActions())
    m_selectButton.OnClick();
  else
    CGUIButtonControl::OnClick();
}

void CGUIToggleButtonControl::SetToggleSelect(const std::string &toggleSelect)
{
  m_toggleSelect = g_infoManager.Register(toggleSelect, GetParentID());
}
