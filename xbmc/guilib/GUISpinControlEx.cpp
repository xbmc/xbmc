/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUISpinControlEx.h"

#include "utils/StringUtils.h"

CGUISpinControlEx::CGUISpinControlEx(int parentID, int controlID, float posX, float posY, float width, float height, float spinWidth, float spinHeight, const CLabelInfo& spinInfo, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus, const CTextureInfo& textureUp, const CTextureInfo& textureDown, const CTextureInfo& textureUpFocus, const CTextureInfo& textureDownFocus, const CTextureInfo& textureUpDisabled, const CTextureInfo& textureDownDisabled, const CLabelInfo& labelInfo, int iType)
    : CGUISpinControl(parentID, controlID, posX, posY, spinWidth, spinHeight, textureUp, textureDown, textureUpFocus, textureDownFocus, textureUpDisabled, textureDownDisabled, spinInfo, iType)
    , m_buttonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
{
  ControlType = GUICONTROL_SPINEX;
  m_spinPosX = 0;
}

CGUISpinControlEx::~CGUISpinControlEx(void) = default;

void CGUISpinControlEx::AllocResources()
{
  // Correct alignment - we always align the spincontrol on the right
  m_label.GetLabelInfo().align = (m_label.GetLabelInfo().align & XBFONT_CENTER_Y) | XBFONT_RIGHT;
  CGUISpinControl::AllocResources();
  m_buttonControl.AllocResources();
  if (m_height == 0)
    m_height = GetSpinHeight();
}

void CGUISpinControlEx::FreeResources(bool immediately)
{
  CGUISpinControl::FreeResources(immediately);
  m_buttonControl.FreeResources(immediately);
}

void CGUISpinControlEx::DynamicResourceAlloc(bool bOnOff)
{
  CGUISpinControl::DynamicResourceAlloc(bOnOff);
  m_buttonControl.DynamicResourceAlloc(bOnOff);
}


void CGUISpinControlEx::SetInvalid()
{
  CGUISpinControl::SetInvalid();
  m_buttonControl.SetInvalid();
}

void CGUISpinControlEx::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  // make sure the button has focus if it should have...
  m_buttonControl.SetFocus(HasFocus());
  m_buttonControl.SetPulseOnSelect(m_pulseOnSelect);
  m_buttonControl.SetEnabled(m_enabled);
  if (m_bInvalidated)
  {
    float spinPosX = m_buttonControl.GetXPosition() + m_buttonControl.GetWidth() - GetSpinWidth() * 2 - (m_spinPosX ? m_spinPosX : m_buttonControl.GetLabelInfo().offsetX);
    float spinPosY = m_buttonControl.GetYPosition() + (m_buttonControl.GetHeight() - GetSpinHeight()) * 0.5f;
    CGUISpinControl::SetPosition(spinPosX, spinPosY);
  }
  m_buttonControl.DoProcess(currentTime, dirtyregions);
  CGUISpinControl::Process(currentTime, dirtyregions);
}

void CGUISpinControlEx::Render()
{
  m_buttonControl.Render();
  CGUISpinControl::Render();
}

void CGUISpinControlEx::SetPosition(float posX, float posY)
{
  m_buttonControl.SetPosition(posX, posY);
  CGUISpinControl::SetInvalid();
}

void CGUISpinControlEx::SetWidth(float width)
{
  m_buttonControl.SetWidth(width);
  CGUISpinControl::SetInvalid();
}

void CGUISpinControlEx::SetHeight(float height)
{
  m_buttonControl.SetHeight(height);
  CGUISpinControl::SetInvalid();
}

bool CGUISpinControlEx::UpdateColors(const CGUIListItem* item)
{
  bool changed = CGUISpinControl::UpdateColors(nullptr);
  changed |= m_buttonControl.SetColorDiffuse(m_diffuseColor);
  changed |= m_buttonControl.UpdateColors(nullptr);

  return changed;
}

void CGUISpinControlEx::SetEnabled(bool bEnable)
{
  m_buttonControl.SetEnabled(bEnable);
  CGUISpinControl::SetEnabled(bEnable);
}

const std::string CGUISpinControlEx::GetCurrentLabel() const
{
  return CGUISpinControl::GetLabel();
}

std::string CGUISpinControlEx::GetDescription() const
{
  return StringUtils::Format("{} ({})", m_buttonControl.GetDescription(), GetLabel());
}

void CGUISpinControlEx::SetItemInvalid(bool invalid)
{
  if (invalid)
  {
    m_label.GetLabelInfo().textColor = m_buttonControl.GetLabelInfo().disabledColor;
    m_label.GetLabelInfo().focusedColor = m_buttonControl.GetLabelInfo().disabledColor;
  }
  else
  {
    m_label.GetLabelInfo().textColor = m_buttonControl.GetLabelInfo().textColor;
    m_label.GetLabelInfo().focusedColor = m_buttonControl.GetLabelInfo().focusedColor;
  }
}

void CGUISpinControlEx::SetSpinPosition(float spinPosX)
{
  m_spinPosX = spinPosX;
  SetPosition(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition());
}

void CGUISpinControlEx::RenderText(float posX, float posY, float width, float height)
{
  const float spaceWidth = 10;
  // check our limits from the button control
  float x = std::max(m_buttonControl.m_label.GetRenderRect().x2 + spaceWidth, posX);
  m_label.SetScrolling(HasFocus());
  CGUISpinControl::RenderText(x, m_buttonControl.GetYPosition(), width + posX - x, m_buttonControl.GetHeight());
}
