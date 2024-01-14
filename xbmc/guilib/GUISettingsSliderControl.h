/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUISliderControl.h
\brief
*/

#include "GUIButtonControl.h"
#include "GUISliderControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUISettingsSliderControl :
      public CGUISliderControl
{
public:
  CGUISettingsSliderControl(int parentID,
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
                            int iType);
  ~CGUISettingsSliderControl() override = default;
  CGUISettingsSliderControl *Clone() const override { return new CGUISettingsSliderControl(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnAction(const CAction &action) override;
  void OnUnFocus() override;
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;
  void SetActive();
  bool IsActive() const override { return m_active; }
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  void SetPosition(float posX, float posY) override;
  float GetWidth() const override { return m_buttonControl.GetWidth(); }
  void SetWidth(float width) override;
  float GetHeight() const override { return m_buttonControl.GetHeight(); }
  void SetHeight(float height) override;
  void SetEnabled(bool bEnable) override;

  void SetText(const std::string& label) { m_buttonControl.SetLabel(label); }
  float GetXPosition() const override { return m_buttonControl.GetXPosition(); }
  float GetYPosition() const override { return m_buttonControl.GetYPosition(); }
  std::string GetDescription() const override;
  bool HitTest(const CPoint& point) const override { return m_buttonControl.HitTest(point); }

protected:
  bool UpdateColors(const CGUIListItem* item) override;
  virtual void ProcessText();

private:
  CGUISettingsSliderControl(const CGUISettingsSliderControl& control);

  CGUIButtonControl m_buttonControl;
  CGUILabel m_label;
  bool m_active; ///< Whether the slider has been activated by a click.
};

