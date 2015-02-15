/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "GUISettingsGroupLabelControl.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "GUIFontManager.h"
#include "settings/lib/SettingDefinitions.h"

CGUISettingsGroupLabelControl::CGUISettingsGroupLabelControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, const CLabelInfo& labelInfo)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_image(posX, posY, width, height, texture),
    m_label(posX, posY, width, height, labelInfo),
    m_alpha(255),
    m_imageHeight(height),
    m_normalHeight(height),
    m_hideTexture(false)
{
  ControlType = GUICONTROL_SETTINGS_GROUP_LABEL;
}

CGUISettingsGroupLabelControl::~CGUISettingsGroupLabelControl(void)
{
}

void CGUISettingsGroupLabelControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  {
    m_image.SetWidth(m_width);
    m_image.SetHeight(m_imageHeight);
  }

  m_image.Process(currentTime);

  std::string label(m_info.GetLabel(m_parentID));
  bool changed  = m_label.SetMaxRect(m_posX, m_posY, m_width, m_height);
  changed |= m_label.SetText(label);
  if (!label.empty())
  {
    changed |= m_label.SetColor(GetTextColor());
    changed |= m_label.Process(currentTime);
  }
  if (changed)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUISettingsGroupLabelControl::Render()
{
  m_image.Render();
  m_label.Render();
  CGUIControl::Render();
}

CGUILabel::COLOR CGUISettingsGroupLabelControl::GetTextColor() const
{
  if (IsDisabled())
    return CGUILabel::COLOR_DISABLED;
  return CGUILabel::COLOR_TEXT;
}

bool CGUISettingsGroupLabelControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID())
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      SetLabel(message.GetLabel());
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUISettingsGroupLabelControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_image.AllocResources();
  if (!m_width)
    m_width = m_image.GetWidth();
  if (!m_height)
    m_height = m_image.GetHeight();
}

void CGUISettingsGroupLabelControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_image.FreeResources(immediately);
}

void CGUISettingsGroupLabelControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_image.DynamicResourceAlloc(bOnOff);
}

void CGUISettingsGroupLabelControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_label.SetInvalid();
  m_image.SetInvalid();
}

void CGUISettingsGroupLabelControl::SetLabel(const std::string &label)
{
  m_info.SetLabel(label, "", GetParentID());
  m_height = label.empty() ? m_hideTexture ? 0 : m_imageHeight : m_normalHeight;
  SetInvalid();
}

void CGUISettingsGroupLabelControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);
  m_image.SetPosition(posX, posY + (m_info.GetLabel(m_parentID).empty() ? 0 : m_imagePosY));
}

void CGUISettingsGroupLabelControl::SetAlpha(unsigned char alpha)
{
  if (m_alpha != alpha)
    MarkDirtyRegion();
  m_alpha = alpha;
}

bool CGUISettingsGroupLabelControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();
  changed |= m_image.SetDiffuseColor(m_diffuseColor);

  return changed;
}

CRect CGUISettingsGroupLabelControl::CalcRenderRegion() const
{
  CRect rect = CGUIControl::CalcRenderRegion();
  CRect textRect = m_label.GetRenderRect();
  rect.Union(textRect);
  return rect;
}

std::string CGUISettingsGroupLabelControl::GetDescription() const
{
  std::string strLabel(m_info.GetLabel(m_parentID));
  return strLabel;
}

void CGUISettingsGroupLabelControl::SetAspectRatio(const CAspectRatio &aspect)
{
  m_image.SetAspectRatio(aspect);
}

float CGUISettingsGroupLabelControl::GetHeight() const
{
  return m_height;
}

void CGUISettingsGroupLabelControl::SetTexture(float posY, float height)
{
  if (m_imageHeight != height || m_imagePosY != posY)
  {
    m_imagePosY = posY;
    m_imageHeight = height;
    m_image.SetHeight(m_imageHeight);

    SetInvalid();
  }
}

void CGUISettingsGroupLabelControl::SetHideTexture(bool hide)
{
  if (m_hideTexture != hide)
  {
    m_hideTexture = hide;
    m_image.SetVisible(!hide);

    SetInvalid();
  }
}

bool CGUISettingsGroupLabelControl::TextureAllowedToVisible(bool firstGroup, bool labelEmpty) const
{
  if (firstGroup && labelEmpty)
    return false;

  return true;
}
