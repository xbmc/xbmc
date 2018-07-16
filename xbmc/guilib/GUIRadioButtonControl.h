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

  ~CGUIRadioButtonControl(void) override;
  CGUIRadioButtonControl *Clone() const override { return new CGUIRadioButtonControl(*this); };

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
  bool IsSelected() const { return m_bSelected; };
protected:
  bool UpdateColors() override;
  CGUITexture m_imgRadioOnFocus;
  CGUITexture m_imgRadioOnNoFocus;
  CGUITexture m_imgRadioOffFocus;
  CGUITexture m_imgRadioOffNoFocus;
  CGUITexture m_imgRadioOnDisabled;
  CGUITexture m_imgRadioOffDisabled;
  float m_radioPosX;
  float m_radioPosY;
  INFO::InfoPtr m_toggleSelect;
  bool m_useLabel2;
};
