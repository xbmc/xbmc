/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIRadioButtonControl.h
\brief
*/

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIRadioButtonControl :
      public CGUIButtonControl
{
public:
  CGUIRadioButtonControl(int parentID, int controlID,
                         float posX, float posY, float width, float height,
                         const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus,
                         const CLabelInfo& labelInfo,
                         const CTextureInfo& radioOnFocus, const CTextureInfo& radioOnNoFocus,
                         const CTextureInfo& radioOffFocus, const CTextureInfo& radioOffNoFocus,
                         const CTextureInfo& radioOnDisabled, const CTextureInfo& radioOffDisabled);

  ~CGUIRadioButtonControl() override = default;
  CGUIRadioButtonControl* Clone() const override { return new CGUIRadioButtonControl(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnAction(const CAction &action) override ;
  bool OnMessage(CGUIMessage& message) override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  void SetPosition(float posX, float posY) override;
  void SetWidth(float width) override;
  void SetHeight(float height) override;
  std::string GetDescription() const override;
  void SetRadioDimensions(float posX, float posY, float width, float height);
  void SetToggleSelect(const std::string &toggleSelect);
  bool IsSelected() const { return m_bSelected; }

protected:
  bool UpdateColors(const CGUIListItem* item) override;
  std::unique_ptr<CGUITexture> m_imgRadioOnFocus;
  std::unique_ptr<CGUITexture> m_imgRadioOnNoFocus;
  std::unique_ptr<CGUITexture> m_imgRadioOffFocus;
  std::unique_ptr<CGUITexture> m_imgRadioOffNoFocus;
  std::unique_ptr<CGUITexture> m_imgRadioOnDisabled;
  std::unique_ptr<CGUITexture> m_imgRadioOffDisabled;
  float m_radioPosX;
  float m_radioPosY;
  INFO::InfoPtr m_toggleSelect;
  bool m_useLabel2;

private:
  CGUIRadioButtonControl(const CGUIRadioButtonControl& control);
};
