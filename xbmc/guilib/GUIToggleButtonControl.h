/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIToggleButtonControl.h
\brief
*/

#include "GUIButtonControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIToggleButtonControl : public CGUIButtonControl
{
public:
  CGUIToggleButtonControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CTextureInfo& altTextureFocus, const CTextureInfo& altTextureNoFocus, const CLabelInfo &labelInfo, bool wrapMultiline = false);
  ~CGUIToggleButtonControl(void) override;
  CGUIToggleButtonControl* Clone() const override { return new CGUIToggleButtonControl(*this); }

  void DoProcess(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void DoRender() override;
  bool OnAction(const CAction &action) override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  void SetPosition(float posX, float posY) override;
  void SetWidth(float width) override;
  void SetHeight(float height) override;
  void SetMinWidth(float minWidth) override;
  void SetLabel(const std::string& label) override;
  void SetAltLabel(const std::string& label);
  std::string GetDescription() const override;
  void SetToggleSelect(const std::string &toggleSelect);
  void SetAltClickActions(const CGUIAction &clickActions);
  
  void SetPulseOnSelect(bool pulseOnSelect) override;
  void SetEnabled(bool enable) override;
  void SetFocus(bool focus) override;
  void SetVisible(bool visible, bool setVisState = false) override;

protected:
  bool UpdateColors(const CGUIListItem* item) override;
  void OnClick() override;
  CGUIButtonControl m_selectButton;
  INFO::InfoPtr m_toggleSelect;

private:
  void ProcessToggle(unsigned int currentTime);
  std::string m_altLabel;
};

