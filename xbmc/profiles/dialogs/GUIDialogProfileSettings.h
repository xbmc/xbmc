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
  virtual ~CGUIDialogProfileSettings();

  static bool ShowForProfile(unsigned int iProfile, bool firstLogin = false);

protected:
  // specializations of CGUIWindow
  virtual void OnWindowLoaded();

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save() { }
  virtual void OnCancel();
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  /*! \brief Prompt for a change in profile path
   \param directory Current directory for the profile, new profile directory will be returned here
   \param isDefault whether this is the default profile or not
   \return true if the profile path has been changed, false otherwise.
   */
  static bool GetProfilePath(std::string &directory, bool isDefault);

  void updateProfileName();
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
  std::string m_defaultImage;
};
