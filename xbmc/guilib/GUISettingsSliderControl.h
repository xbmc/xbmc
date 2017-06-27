/*!
\file GUISliderControl.h
\brief
*/

#ifndef GUILIB_GUISettingsSliderCONTROL_H
#define GUILIB_GUISettingsSliderCONTROL_H

#pragma once

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

#include "GUISliderControl.h"
#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUISettingsSliderControl :
      public CGUISliderControl
{
public:
  CGUISettingsSliderControl(int parentID, int controlID, float posX, float posY, float width, float height, float sliderWidth, float sliderHeight, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus, const CTextureInfo& backGroundTexture, const CTextureInfo& nibTexture, const CTextureInfo& nibTextureFocus, const CLabelInfo &labelInfo, int iType);
  ~CGUISettingsSliderControl(void) override;
  CGUISettingsSliderControl *Clone() const override { return new CGUISettingsSliderControl(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnAction(const CAction &action) override;
  void OnUnFocus() override;
  EVENT_RESULT OnMouseEvent(const CPoint& point, const CMouseEvent& event) override;
  void SetActive();
  bool IsActive() const override { return m_active; };
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

  void SetText(const std::string &label) {m_buttonControl.SetLabel(label);};
  float GetXPosition() const override { return m_buttonControl.GetXPosition();};
  float GetYPosition() const override { return m_buttonControl.GetYPosition();};
  std::string GetDescription() const override;
  bool HitTest(const CPoint &point) const override { return m_buttonControl.HitTest(point); };

protected:
  bool UpdateColors() override;
  virtual void ProcessText();

private:
  CGUIButtonControl m_buttonControl;
  CGUILabel m_label;
  bool m_active; ///< Whether the slider has been activated by a click.
};
#endif
