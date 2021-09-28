/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsManagerBase.h"

class CSettings;

class CGUIWindowSettingsCategory : public CGUIDialogSettingsManagerBase
{
public:
  CGUIWindowSettingsCategory();
  ~CGUIWindowSettingsCategory() override;

  // specialization of CGUIControl
  bool OnMessage(CGUIMessage &message) override;
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;
  int GetID() const override { return CGUIDialogSettingsManagerBase::GetID() + m_iSection; }

  // specialization of CGUIWindow
  bool IsDialog() const override { return false; }

protected:
  // specialization of CGUIWindow
  void OnWindowLoaded() override;

  // implementation of CGUIDialogSettingsBase
  int GetSettingLevel() const override;
  std::shared_ptr<CSettingSection> GetSection() override;
  bool Save() override;

  // implementation of CGUIDialogSettingsManagerBase
  CSettingsManager* GetSettingsManager() const override;

  /*!
   * Set focus to a category or setting in this window. The setting/category must be active in the
   * current level.
   */
  void FocusElement(const std::string& elementId);

  std::shared_ptr<CSettings> m_settings;
  int m_iSection = 0;
  bool m_returningFromSkinLoad = false; // true if we are returning from loading the skin
};
