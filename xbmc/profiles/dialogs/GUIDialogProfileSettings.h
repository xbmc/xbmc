/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "profiles/Profile.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

#include <string>

class CGUIDialogProfileSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogProfileSettings();
  ~CGUIDialogProfileSettings() override;

  static bool ShowForProfile(unsigned int iProfile, bool firstLogin = false);

protected:
  // specializations of CGUIWindow
  void OnWindowLoaded() override;

  // implementations of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  void Save() override { }
  void OnCancel() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

  /*! \brief Prompt for a change in profile path
   \param directory Current directory for the profile, new profile directory will be returned here
   \param isDefault whether this is the default profile or not
   \return true if the profile path has been changed, false otherwise.
   */
  static bool GetProfilePath(std::string &directory, bool isDefault);

  void UpdateProfileImage();
  void updateProfileDirectory();

  bool m_needsSaving = false;
  std::string m_name;
  std::string m_thumb;
  std::string m_directory;
  int m_sourcesMode;
  int m_dbMode;
  bool m_isDefault;
  bool m_isNewUser;
  bool m_showDetails;

  CProfile::CLock m_locks;
};
