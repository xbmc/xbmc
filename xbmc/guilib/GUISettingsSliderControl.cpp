/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUISettingsSliderControl.h"

#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"

using namespace KODI;

CGUISettingsSliderControl::CGUISettingsSliderControl(int parentID,
                                                     int controlID,
                                                     float posX,
                                                     float posY,
                                                     float width,
                                                     float height,
                                                     float sliderWidth,
                                                     float sliderHeight,
                                                     const CTextureInfo& textureFocus,
                                                     const CTextureInfo& textureNoFocus,
                                                     const CTextureInfo& backGroundTexture,
                                                     const CTextureInfo& backGroundTextureDisabled,
                                                     const CTextureInfo& nibTexture,
                                                     const CTextureInfo& nibTextureFocus,
                                                     const CTextureInfo& nibTextureDisabled,
                                                     const CLabelInfo& labelInfo,
                                                     int iType)
  : CGUISliderControl(parentID,
                      controlID,
                      posX,
                      posY,
                      sliderWidth,
                      sliderHeight,
                      backGroundTexture,
                      backGroundTextureDisabled,
                      nibTexture,
                      nibTextureFocus,
                      nibTextureDisabled,
                      iType,
                      HORIZONTAL),
    m_buttonControl(
        parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo),
    m_label(posX, posY, width, height, labelInfo)
{
  m_label.SetAlign((labelInfo.align & XBFONT_CENTER_Y) | XBFONT_RIGHT);
  ControlType = GUICONTROL_SETTINGS_SLIDER;
  m_active = false;
}

CGUISettingsSliderControl::CGUISettingsSliderControl(const CGUISettingsSliderControl& control)
  : CGUISliderControl(control),
    m_buttonControl(control.m_buttonControl),
    m_label(control.m_label),
    m_active(control.m_active)
{
}

void CGUISettingsSliderControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  {
    float sliderPosX = m_buttonControl.GetXPosition() + m_buttonControl.GetWidth() - m_width - m_buttonControl.GetLabelInfo().offsetX;
    float sliderPosY = m_buttonControl.GetYPosition() + (m_buttonControl.GetHeight() - m_height) * 0.5f;
    CGUISliderControl::SetPosition(sliderPosX, sliderPosY);
  }
  m_buttonControl.SetFocus(HasFocus());
  m_buttonControl.SetPulseOnSelect(m_pulseOnSelect);
  m_buttonControl.SetEnabled(m_enabled);
  m_buttonControl.DoProcess(currentTime, dirtyregions);
  ProcessText();
  CGUISliderControl::Process(currentTime, dirtyregions);
}

void CGUISettingsSliderControl::Render()
{
  m_buttonControl.Render();
  CGUISliderControl::Render();
  if (CServiceBroker::GetWinSystem()->GetGfxContext().GetRenderOrder() ==
      RENDER_ORDER_FRONT_TO_BACK)
    return;
  m_label.Render();
}

void CGUISettingsSliderControl::ProcessText()
{
  bool changed = false;

  changed |= m_label.SetMaxRect(m_buttonControl.GetXPosition(), m_buttonControl.GetYPosition(), m_posX - m_buttonControl.GetXPosition(), m_buttonControl.GetHeight());
  changed |= m_label.SetText(CGUISliderControl::GetDescription());
  if (IsDisabled())
    changed |= m_label.SetColor(CGUILabel::COLOR_DISABLED);
  else if (HasFocus())
    changed |= m_label.SetColor(CGUILabel::COLOR_FOCUSED);
  else
    changed |= m_label.SetColor(CGUILabel::COLOR_TEXT);

  if (changed)
    MarkDirtyRegion();
}

bool CGUISettingsSliderControl::OnAction(const CAction &action)
{
  // intercept ACTION_SELECT_ITEM because onclick functionality is different from base class
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    if (!IsActive())
      m_active = true;
     // switch between the two sliders
    else if (m_rangeSelection && m_currentSelector == RangeSelectorLower)
      SwitchRangeSelector();
    else
    {
      m_active = false;
      if (m_rangeSelection)
        SwitchRangeSelector();
    }
    return true;
  }
  return CGUISliderControl::OnAction(action);
}

void CGUISettingsSliderControl::OnUnFocus()
{
  m_active = false;
}

EVENT_RESULT CGUISettingsSliderControl::OnMouseEvent(const CPoint& point,
                                                     const MOUSE::CMouseEvent& event)
{
  SetActive();
  return CGUISliderControl::OnMouseEvent(point, event);
}

void CGUISettingsSliderControl::SetActive()
{
  m_active = true;
}

void CGUISettingsSliderControl::FreeResources(bool immediately)
{
  CGUISliderControl::FreeResources(immediately);
  m_buttonControl.FreeResources(immediately);
}

void CGUISettingsSliderControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUISliderControl::DynamicResourceAlloc(bOnOff);
  m_buttonControl.DynamicResourceAlloc(bOnOff);
}

void CGUISettingsSliderControl::AllocResources()
{
  CGUISliderControl::AllocResources();
  m_buttonControl.AllocResources();
}

void CGUISettingsSliderControl::SetInvalid()
{
  CGUISliderControl::SetInvalid();
  m_buttonControl.SetInvalid();
}

void CGUISettingsSliderControl::SetPosition(float posX, float posY)
{
  m_buttonControl.SetPosition(posX, posY);
  CGUISliderControl::SetInvalid();
}

void CGUISettingsSliderControl::SetWidth(float width)
{
  m_buttonControl.SetWidth(width);
  CGUISliderControl::SetInvalid();
}

void CGUISettingsSliderControl::SetHeight(float height)
{
  m_buttonControl.SetHeight(height);
  CGUISliderControl::SetInvalid();
}

void CGUISettingsSliderControl::SetEnabled(bool bEnable)
{
  CGUISliderControl::SetEnabled(bEnable);
  m_buttonControl.SetEnabled(bEnable);
}

std::string CGUISettingsSliderControl::GetDescription() const
{
  return m_buttonControl.GetDescription() + " " + CGUISliderControl::GetDescription();
}

bool CGUISettingsSliderControl::UpdateColors(const CGUIListItem* item)
{
  bool changed = CGUISliderControl::UpdateColors(nullptr);
  changed |= m_buttonControl.SetColorDiffuse(m_diffuseColor);
  changed |= m_buttonControl.UpdateColors(nullptr);
  changed |= m_label.UpdateColors();

  return changed;
}
