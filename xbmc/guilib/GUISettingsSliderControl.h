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
  virtual ~CGUISettingsSliderControl(void);
  virtual CGUISettingsSliderControl *Clone() const override { return new CGUISettingsSliderControl(*this); }

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  virtual void Render() override;
  virtual bool OnAction(const CAction &action) override;
  void OnUnFocus() override;
  EVENT_RESULT OnMouseEvent(const CPoint& point, const CMouseEvent& event) override;
  void SetActive();
  bool IsActive() const override { return m_active; };
  virtual void AllocResources() override;
  virtual void FreeResources(bool immediately = false) override;
  virtual void DynamicResourceAlloc(bool bOnOff) override;
  virtual void SetInvalid() override;
  virtual void SetPosition(float posX, float posY) override;
  virtual float GetWidth() const override { return m_buttonControl.GetWidth(); }
  virtual void SetWidth(float width) override;
  virtual float GetHeight() const override { return m_buttonControl.GetHeight(); }
  virtual void SetHeight(float height) override;
  virtual void SetEnabled(bool bEnable) override;

  void SetText(const std::string &label) {m_buttonControl.SetLabel(label);};
  virtual float GetXPosition() const override { return m_buttonControl.GetXPosition();};
  virtual float GetYPosition() const override { return m_buttonControl.GetYPosition();};
  virtual std::string GetDescription() const override;
  virtual bool HitTest(const CPoint &point) const override { return m_buttonControl.HitTest(point); };

protected:
  virtual bool UpdateColors() override;
  virtual void ProcessText();

private:
  CGUIButtonControl m_buttonControl;
  CGUILabel m_label;
  bool m_active; ///< Whether the slider has been activated by a click.
};
#endif
