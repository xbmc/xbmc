/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "GUIFontManager.h"
#include "Key.h"

CGUIRadioButtonControl::CGUIRadioButtonControl(int parentID, int controlID, float posX, float posY, float width, float height,
    const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus,
    const CLabelInfo& labelInfo,
    const CTextureInfo& radioOn, const CTextureInfo& radioOff)
    : CGUIButtonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
    , m_imgRadioOn(posX, posY, 16, 16, radioOn)
    , m_imgRadioOff(posX, posY, 16, 16, radioOff)
{
  m_radioPosX = 0;
  m_radioPosY = 0;
  m_toggleSelect = 0;
  m_imgRadioOn.SetAspectRatio(CAspectRatio::AR_KEEP);\
  m_imgRadioOff.SetAspectRatio(CAspectRatio::AR_KEEP);
  ControlType = GUICONTROL_RADIO;
}

CGUIRadioButtonControl::~CGUIRadioButtonControl(void)
{}

void CGUIRadioButtonControl::Render()
{
  CGUIButtonControl::Render();

  if ( IsSelected() && !IsDisabled() )
    m_imgRadioOn.Render();
  else
    m_imgRadioOff.Render();
}

void CGUIRadioButtonControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_toggleSelect)
  {
    // ask our infoManager whether we are selected or not...
    bool selected = g_infoManager.GetBoolValue(m_toggleSelect);

    if (selected != m_bSelected)
    {
      MarkDirtyRegion();
      m_bSelected = selected;
    }
  }

  m_imgRadioOn.Process(currentTime);
  m_imgRadioOff.Process(currentTime);

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
  m_imgRadioOn.AllocResources();
  m_imgRadioOff.AllocResources();

  SetPosition(m_posX, m_posY);
}

void CGUIRadioButtonControl::FreeResources(bool immediately)
{
  CGUIButtonControl::FreeResources(immediately);
  m_imgRadioOn.FreeResources(immediately);
  m_imgRadioOff.FreeResources(immediately);
}

void CGUIRadioButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgRadioOn.DynamicResourceAlloc(bOnOff);
  m_imgRadioOff.DynamicResourceAlloc(bOnOff);
}

void CGUIRadioButtonControl::SetInvalid()
{
  CGUIButtonControl::SetInvalid();
  m_imgRadioOn.SetInvalid();
  m_imgRadioOff.SetInvalid();
}

void CGUIRadioButtonControl::SetPosition(float posX, float posY)
{
  CGUIButtonControl::SetPosition(posX, posY);
  float radioPosX = m_radioPosX ? m_posX + m_radioPosX : (m_posX + m_width - 8) - m_imgRadioOn.GetWidth();
  float radioPosY = m_radioPosY ? m_posY + m_radioPosY : m_posY + (m_height - m_imgRadioOn.GetHeight()) / 2;
  m_imgRadioOn.SetPosition(radioPosX, radioPosY);
  m_imgRadioOff.SetPosition(radioPosX, radioPosY);
}

void CGUIRadioButtonControl::SetRadioDimensions(float posX, float posY, float width, float height)
{
  m_radioPosX = posX;
  m_radioPosY = posY;
  if (width)
  {
    m_imgRadioOn.SetWidth(width);
    m_imgRadioOff.SetWidth(width);
  }
  if (height)
  {
    m_imgRadioOn.SetHeight(height);
    m_imgRadioOff.SetHeight(height);
  }
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

CStdString CGUIRadioButtonControl::GetDescription() const
{
  CStdString strLabel = CGUIButtonControl::GetDescription();
  if (m_bSelected)
    strLabel += " (*)";
  else
    strLabel += " ( )";
  return strLabel;
}

bool CGUIRadioButtonControl::UpdateColors()
{
  bool changed = CGUIButtonControl::UpdateColors();
  changed |= m_imgRadioOn.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgRadioOff.SetDiffuseColor(m_diffuseColor);

  return changed;
}

void CGUIRadioButtonControl::SetToggleSelect(const CStdString &toggleSelect)
{
  m_toggleSelect = g_infoManager.Register(toggleSelect, GetParentID());
}
