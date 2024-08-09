/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIToggleButtonControl.h"

#include "GUIDialog.h"
#include "GUIInfoManager.h"
#include "GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

CGUIToggleButtonControl::CGUIToggleButtonControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CTextureInfo& altTextureFocus, const CTextureInfo& altTextureNoFocus, const CLabelInfo &labelInfo, bool wrapMultiLine)
    : CGUIButtonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo, wrapMultiLine)
    , m_selectButton(parentID, controlID, posX, posY, width, height, altTextureFocus, altTextureNoFocus, labelInfo, wrapMultiLine)
{
  ControlType = GUICONTROL_TOGGLEBUTTON;
  // set parent, so that the dirty state can propagade
  m_selectButton.SetParentControl(GetParentControl());
}

CGUIToggleButtonControl::~CGUIToggleButtonControl(void) = default;

void CGUIToggleButtonControl::DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions)
{
  // ask our infoManager whether we are selected or not...
  if (m_toggleSelect)
    m_bSelected = m_toggleSelect->Get(INFO::DEFAULT_CONTEXT);

  if (m_bSelected)
    m_selectButton.DoProcess(currentTime, dirtyregions);
  else
    CGUIButtonControl::DoProcess(currentTime, dirtyregions);
}

void CGUIToggleButtonControl::DoRender()
{
  if (m_bSelected)
    m_selectButton.DoRender();
  else
    CGUIButtonControl::DoRender();
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

bool CGUIToggleButtonControl::UpdateColors(const CGUIListItem* item)
{
  bool changed = CGUIButtonControl::UpdateColors(nullptr);
  changed |= m_selectButton.SetColorDiffuse(m_diffuseColor);
  changed |= m_selectButton.UpdateColors(nullptr);

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

void CGUIToggleButtonControl::SetPulseOnSelect(bool pulseOnSelect)
{
  CGUIButtonControl::SetPulseOnSelect(pulseOnSelect);
  m_selectButton.SetPulseOnSelect(pulseOnSelect);
}

void CGUIToggleButtonControl::SetEnabled(bool enabled)
{
  CGUIButtonControl::SetEnabled(!enabled);
  m_selectButton.SetEnabled(enabled);
}

void CGUIToggleButtonControl::SetFocus(bool focus)
{
  CGUIButtonControl::SetFocus(focus);
  m_selectButton.SetFocus(focus);
}

void CGUIToggleButtonControl::SetVisible(bool visible, bool setVisState)
{
  CGUIButtonControl::SetVisible(visible, setVisState);
  m_selectButton.SetVisible(visible, setVisState);
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
  m_toggleSelect = CServiceBroker::GetGUI()->GetInfoManager().Register(toggleSelect, GetParentID());
}
