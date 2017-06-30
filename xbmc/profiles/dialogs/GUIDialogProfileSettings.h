#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include <string>

#include "profiles/Profile.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"

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
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

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

  bool m_needsSaving;
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
