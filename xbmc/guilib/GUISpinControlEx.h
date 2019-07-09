/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUISpinControlEx.h
\brief
*/

#include "GUIButtonControl.h"
#include "GUISpinControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUISpinControlEx : public CGUISpinControl
{
public:
  CGUISpinControlEx(int parentID, int controlID, float posX, float posY, float width, float height, float spinWidth, float spinHeight, const CLabelInfo& spinInfo, const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus, const CTextureInfo& textureUp, const CTextureInfo& textureDown, const CTextureInfo& textureUpFocus, const CTextureInfo& textureDownFocus, const CTextureInfo& textureUpDisabled, const CTextureInfo& textureDownDisabled, const CLabelInfo& labelInfo, int iType);
  ~CGUISpinControlEx(void) override;
  CGUISpinControlEx *Clone() const override { return new CGUISpinControlEx(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void SetPosition(float posX, float posY) override;
  float GetWidth() const override { return m_buttonControl.GetWidth();};
  void SetWidth(float width) override;
  float GetHeight() const override { return m_buttonControl.GetHeight();};
  void SetHeight(float height) override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  const std::string GetCurrentLabel() const;
  void SetText(const std::string & aLabel) {m_buttonControl.SetLabel(aLabel);};
  void SetEnabled(bool bEnable) override;
  float GetXPosition() const override { return m_buttonControl.GetXPosition();};
  float GetYPosition() const override { return m_buttonControl.GetYPosition();};
  std::string GetDescription() const override;
  bool HitTest(const CPoint &point) const override { return m_buttonControl.HitTest(point); };
  void SetSpinPosition(float spinPosX);

  void SetItemInvalid(bool invalid);
protected:
  void RenderText(float posX, float posY, float width, float height) override;
  bool UpdateColors() override;
  CGUIButtonControl m_buttonControl;
  float m_spinPosX;
};

