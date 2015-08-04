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

#include "GUIRadioButtonControl.h"
#include "GUIInfoManager.h"
#include "LocalizeStrings.h"
#include "input/Key.h"

CGUIRadioButtonControl::CGUIRadioButtonControl(int parentID, int controlID, float posX, float posY, float width, float height,
    const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus,
    const CLabelInfo& labelInfo,
    const CTextureInfo& radioOnFocus, const CTextureInfo& radioOnNoFocus,
    const CTextureInfo& radioOffFocus, const CTextureInfo& radioOffNoFocus, 
    const CTextureInfo& radioOnDisabled, const CTextureInfo& radioOffDisabled)
    : CGUIButtonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
    , m_imgRadioOnFocus(posX, posY, 16, 16, radioOnFocus)
    , m_imgRadioOnNoFocus(posX, posY, 16, 16, radioOnNoFocus)
    , m_imgRadioOffFocus(posX, posY, 16, 16, radioOffFocus)
    , m_imgRadioOffNoFocus(posX, posY, 16, 16, radioOffNoFocus)
    , m_imgRadioOnDisabled(posX, posY, 16, 16, radioOnDisabled)
    , m_imgRadioOffDisabled(posX, posY, 16, 16, radioOffDisabled)
{
  m_radioPosX = 0;
  m_radioPosY = 0;
  m_imgRadioOnFocus.SetAspectRatio(CAspectRatio::AR_KEEP);
  m_imgRadioOnNoFocus.SetAspectRatio(CAspectRatio::AR_KEEP);
  m_imgRadioOffFocus.SetAspectRatio(CAspectRatio::AR_KEEP);
  m_imgRadioOffNoFocus.SetAspectRatio(CAspectRatio::AR_KEEP);
  m_imgRadioOnDisabled.SetAspectRatio(CAspectRatio::AR_KEEP);
  m_imgRadioOffDisabled.SetAspectRatio(CAspectRatio::AR_KEEP);
  ControlType = GUICONTROL_RADIO;
  m_useLabel2 = false;
}

CGUIRadioButtonControl::~CGUIRadioButtonControl(void)
{}

void CGUIRadioButtonControl::Render()
{
  CGUIButtonControl::Render();

  if ( IsSelected() && !IsDisabled() )
  {
    if (HasFocus())
      m_imgRadioOnFocus.Render();
    else
      m_imgRadioOnNoFocus.Render();
  }
  else if ( !IsSelected() && !IsDisabled() )
  {
    if (HasFocus())
      m_imgRadioOffFocus.Render();
    else
      m_imgRadioOffNoFocus.Render();
  }
  else if ( IsSelected() && IsDisabled() )
    m_imgRadioOnDisabled.Render();
  else
    m_imgRadioOffDisabled.Render();
}

void CGUIRadioButtonControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_toggleSelect)
  {
    // ask our infoManager whether we are selected or not...
    bool selected = m_toggleSelect->Get();

    if (selected != m_bSelected)
    {
      MarkDirtyRegion();
      m_bSelected = selected;
    }
  }

  m_imgRadioOnFocus.Process(currentTime);
  m_imgRadioOnNoFocus.Process(currentTime);
  m_imgRadioOffFocus.Process(currentTime);
  m_imgRadioOffNoFocus.Process(currentTime);
  m_imgRadioOnDisabled.Process(currentTime);
  m_imgRadioOffDisabled.Process(currentTime);

  if (m_useLabel2)
    SetLabel2(g_localizeStrings.Get(m_bSelected ? 16041 : 351));

  CGUIButtonControl::Process(currentTime, dirtyregions);
}

bool CGUIRadioButtonControl::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    m_bSelected = !m_bSelected;
    MarkDirtyRegion();
  }
  return CGUIButtonControl::OnAction(action);
}

bool CGUIRadioButtonControl::OnMessage(CGUIMessage& message)
{
  return CGUIButtonControl::OnMessage(message);
}

void CGUIRadioButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();
  m_imgRadioOnFocus.AllocResources();
  m_imgRadioOnNoFocus.AllocResources();
  m_imgRadioOffFocus.AllocResources();
  m_imgRadioOffNoFocus.AllocResources();
  m_imgRadioOnDisabled.AllocResources();
  m_imgRadioOffDisabled.AllocResources();
  SetPosition(m_posX, m_posY);
}

void CGUIRadioButtonControl::FreeResources(bool immediately)
{
  CGUIButtonControl::FreeResources(immediately);
  m_imgRadioOnFocus.FreeResources(immediately);
  m_imgRadioOnNoFocus.FreeResources(immediately);
  m_imgRadioOffFocus.FreeResources(immediately);
  m_imgRadioOffNoFocus.FreeResources(immediately);
  m_imgRadioOnDisabled.FreeResources(immediately);
  m_imgRadioOffDisabled.FreeResources(immediately);
}

void CGUIRadioButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgRadioOnFocus.DynamicResourceAlloc(bOnOff);
  m_imgRadioOnNoFocus.DynamicResourceAlloc(bOnOff);
  m_imgRadioOffFocus.DynamicResourceAlloc(bOnOff);
  m_imgRadioOffNoFocus.DynamicResourceAlloc(bOnOff);
  m_imgRadioOnDisabled.DynamicResourceAlloc(bOnOff);
  m_imgRadioOffDisabled.DynamicResourceAlloc(bOnOff);
}

void CGUIRadioButtonControl::SetInvalid()
{
  CGUIButtonControl::SetInvalid();
  m_imgRadioOnFocus.SetInvalid();
  m_imgRadioOnNoFocus.SetInvalid();
  m_imgRadioOffFocus.SetInvalid();
  m_imgRadioOffNoFocus.SetInvalid();
  m_imgRadioOnDisabled.SetInvalid();
  m_imgRadioOffDisabled.SetInvalid();
}

void CGUIRadioButtonControl::SetPosition(float posX, float posY)
{
  CGUIButtonControl::SetPosition(posX, posY);
  float radioPosX = m_radioPosX ? m_posX + m_radioPosX : (m_posX + m_width - 8) - m_imgRadioOnFocus.GetWidth();
  float radioPosY = m_radioPosY ? m_posY + m_radioPosY : m_posY + (m_height - m_imgRadioOnFocus.GetHeight()) / 2;
  m_imgRadioOnFocus.SetPosition(radioPosX, radioPosY);
  m_imgRadioOnNoFocus.SetPosition(radioPosX, radioPosY);
  m_imgRadioOffFocus.SetPosition(radioPosX, radioPosY);
  m_imgRadioOffNoFocus.SetPosition(radioPosX, radioPosY);
  m_imgRadioOnDisabled.SetPosition(radioPosX, radioPosY);
  m_imgRadioOffDisabled.SetPosition(radioPosX, radioPosY);
}

void CGUIRadioButtonControl::SetRadioDimensions(float posX, float posY, float width, float height)
{
  m_radioPosX = posX;
  m_radioPosY = posY;
  if (width)
  {
    m_imgRadioOnFocus.SetWidth(width);
    m_imgRadioOnNoFocus.SetWidth(width);
    m_imgRadioOffFocus.SetWidth(width);
    m_imgRadioOffNoFocus.SetWidth(width);
    m_imgRadioOnDisabled.SetWidth(width);
    m_imgRadioOffDisabled.SetWidth(width);
  }
  if (height)
  {
    m_imgRadioOnFocus.SetHeight(height);
    m_imgRadioOnNoFocus.SetHeight(height);
    m_imgRadioOffFocus.SetHeight(height);
    m_imgRadioOffNoFocus.SetHeight(height);
    m_imgRadioOnDisabled.SetHeight(height);
    m_imgRadioOffDisabled.SetHeight(height);
  }

  // use label2 to display the button value in case no
  // dimensions were specified and there's no label2 yet.
  if (GetLabel2().empty() && !width && !height)
    m_useLabel2 = true;

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

std::string CGUIRadioButtonControl::GetDescription() const
{
  std::string strLabel = CGUIButtonControl::GetDescription();
  if (m_bSelected)
    strLabel += " (*)";
  else
    strLabel += " ( )";
  return strLabel;
}

bool CGUIRadioButtonControl::UpdateColors()
{
  bool changed = CGUIButtonControl::UpdateColors();
  changed |= m_imgRadioOnFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgRadioOnNoFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgRadioOffFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgRadioOffNoFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgRadioOnDisabled.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgRadioOffDisabled.SetDiffuseColor(m_diffuseColor);
  return changed;
}

void CGUIRadioButtonControl::SetToggleSelect(const std::string &toggleSelect)
{
  m_toggleSelect = g_infoManager.Register(toggleSelect, GetParentID());
}
