/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "include.h"
#include "GUIRadioButtonControl.h"
#include "utils/GUIInfoManager.h"
#include "GUIFontManager.h"

CGUIRadioButtonControl::CGUIRadioButtonControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
    const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus,
    const CLabelInfo& labelInfo,
    const CTextureInfo& radioFocus, const CTextureInfo& radioNoFocus)
    : CGUIButtonControl(dwParentID, dwControlId, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
    , m_imgRadioFocus(posX, posY, 16, 16, radioFocus)
    , m_imgRadioNoFocus(posX, posY, 16, 16, radioNoFocus)
{
  m_radioPosX = 0;
  m_radioPosY = 0;
  m_toggleSelect = 0;
  m_imgRadioFocus.SetAspectRatio(CAspectRatio::AR_KEEP);\
  m_imgRadioNoFocus.SetAspectRatio(CAspectRatio::AR_KEEP);
  ControlType = GUICONTROL_RADIO;
}

CGUIRadioButtonControl::~CGUIRadioButtonControl(void)
{}


void CGUIRadioButtonControl::Render()
{
  CGUIButtonControl::Render();

  // ask our infoManager whether we are selected or not...
  if (m_toggleSelect)
    m_bSelected = g_infoManager.GetBool(m_toggleSelect, m_dwParentID);

  if ( IsSelected() && !IsDisabled() )
    m_imgRadioFocus.Render();
  else
    m_imgRadioNoFocus.Render();
}

bool CGUIRadioButtonControl::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    m_bSelected = !m_bSelected;
  }
  return CGUIButtonControl::OnAction(action);
}

bool CGUIRadioButtonControl::OnMessage(CGUIMessage& message)
{
  return CGUIButtonControl::OnMessage(message);
}

void CGUIRadioButtonControl::PreAllocResources()
{
  CGUIButtonControl::PreAllocResources();
  m_imgRadioFocus.PreAllocResources();
  m_imgRadioNoFocus.PreAllocResources();
}

void CGUIRadioButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();
  m_imgRadioFocus.AllocResources();
  m_imgRadioNoFocus.AllocResources();

  SetPosition(m_posX, m_posY);
}

void CGUIRadioButtonControl::FreeResources()
{
  CGUIButtonControl::FreeResources();
  m_imgRadioFocus.FreeResources();
  m_imgRadioNoFocus.FreeResources();
}

void CGUIRadioButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgRadioFocus.DynamicResourceAlloc(bOnOff);
  m_imgRadioNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIRadioButtonControl::SetPosition(float posX, float posY)
{
  CGUIButtonControl::SetPosition(posX, posY);
  float radioPosX = m_radioPosX ? m_radioPosX : (m_posX + m_width - 8) - m_imgRadioFocus.GetWidth();
  float radioPosY = m_radioPosY ? m_radioPosY : m_posY + (m_height - m_imgRadioFocus.GetHeight()) / 2;
  m_imgRadioFocus.SetPosition(radioPosX, radioPosY);
  m_imgRadioNoFocus.SetPosition(radioPosX, radioPosY);
}

void CGUIRadioButtonControl::SetRadioDimensions(float posX, float posY, float width, float height)
{
  m_radioPosX = posX;
  m_radioPosY = posY;
  if (width)
  {
    m_imgRadioFocus.SetWidth(width);
    m_imgRadioNoFocus.SetWidth(width);
  }
  if (height)
  {
    m_imgRadioFocus.SetHeight(height);
    m_imgRadioNoFocus.SetHeight(height);
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

void CGUIRadioButtonControl::UpdateColors()
{
  CGUIButtonControl::UpdateColors();
  m_imgRadioFocus.SetDiffuseColor(m_diffuseColor);
  m_imgRadioNoFocus.SetDiffuseColor(m_diffuseColor);
}

