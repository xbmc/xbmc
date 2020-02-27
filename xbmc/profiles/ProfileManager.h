/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

#include "profiles/Profile.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"

class CEventLog;
class CEventLogManager;
class CSettings;
class TiXmlNode;

class CProfileManager : protected ISettingsHandler,
                         protected ISettingCallback
{
public:
  CProfileManager();
  CProfileManager(const CProfileManager&) = delete;
  CProfileManager& operator=(CProfileManager const&) = delete;
  ~CProfileManager() override;

  void Initialize(const std::shared_ptr<CSettings>& settings);
  void Uninitialize();

  void OnSettingsLoaded() override;
  void OnSettingsSaved() const override;
  void OnSettingsCleared() override;

  bool Load();
  /*! \brief Load the user profile information from disk
    Loads the profiles.xml file and creates the list of profiles.
    If no profiles exist, a master user is created. Should be called
    after special://masterprofile/ has been defined.
    \param file XML file to load.
    */

  bool Save() const;
  /*! \brief Save the user profile information to disk
    Saves the list of profiles to the profiles.xml file.
    \param file XML file to save.
    \return true on success, false on failure to save
    */

  void Clear();

  bool LoadProfile(unsigned int index);
  void LogOff();

  bool DeleteProfile(unsigned int index);

  void CreateProfileFolders();

  /*! \brief Retrieve the master profile
    \return const reference to the master profile
    */
  const CProfile& GetMasterProfile() const;

  /*! \brief Retrieve the current profile
    \return const reference to the current profile
    */
  const CProfile& GetCurrentProfile() const;

  /*! \brief Retrieve the profile from an index
    \param unsigned index of the profile to retrieve
    \return const pointer to the profile, NULL if the index is invalid
    */
  const CProfile* GetProfile(unsigned int index) const;

  /*! \brief Retrieve the profile from an index
    \param unsigned index of the profile to retrieve
    \return pointer to the profile, NULL if the index is invalid
    */
  CProfile* GetProfile(unsigned int index);

  /*! \brief Retrieve index of a particular profile by name
    \param name name of the profile index to retrieve
    \return index of this profile, -1 if invalid.
    */
  int GetProfileIndex(const std::string &name) const;

  /*! \brief Retrieve the number of profiles
    \return number of profiles
    */
  size_t GetNumberOfProfiles() const { return m_profiles.size(); }

  /*! \brief Add a new profile
    \param profile CProfile to add
    */
  void AddProfile(const CProfile &profile);

  /*! \brief Are we using the login screen?
    \return true if we're using the login screen, false otherwise
    */
  bool UsingLoginScreen() const { return m_usingLoginScreen; }

  /*! \brief Toggle login screen use on and off
    Toggles the login screen state
    */
  void ToggleLoginScreen()
  {
    m_usingLoginScreen = !m_usingLoginScreen;
    Save();
  }

  /*! \brief Are we the master user?
    \return true if the current profile is the master user, false otherwise
    */
  bool IsMasterProfile() const { return m_currentProfile == 0; }

  /*! \brief Update the date of the current profile
    */
  void UpdateCurrentProfileDate();

  /*! \brief Load the master user for the purposes of logging in
    Loads the master user.  Identical to LoadProfile(0) but doesn't
    update the last logged in details
    */
  void LoadMasterProfileForLogin();

  /*! \brief Retrieve the last used profile index
    \return the last used profile that logged in.  Does not count the
    master user during login.
    */
  uint32_t GetLastUsedProfileIndex() const { return m_lastUsedProfile; }

  /*! \brief Retrieve the current profile index
    \return the index of the currently logged in profile.
    */
  uint32_t GetCurrentProfileIndex() const { return m_currentProfile; }

  /*! \brief Retrieve the next id to use for a new profile
    \return the unique <id> to be used when creating a new profile
    */
  int GetNextProfileId() const { return m_nextProfileId; }

  int GetCurrentProfileId() const { return GetCurrentProfile().getId(); }

  /*! \brief Retrieve the autologin profile id
    Retrieves the autologin profile id. When set to -1, then the last
    used profile will be loaded
    \return the id to the autologin profile
    */
  int GetAutoLoginProfileId() const { return m_autoLoginProfile; }

  /*! \brief Retrieve the autologin profile id
    Retrieves the autologin profile id. When set to -1, then the last
    used profile will be loaded
    \return the id to the autologin profile
    */
  void SetAutoLoginProfileId(const int profileId)
  {
    m_autoLoginProfile = profileId;
    Save();
  }

  /*! \brief Retrieve the name of a particular profile by index
    \param profileId profile index for which to retrieve the name
    \param name will hold the name of the profile when a valid profile index has been provided
    \return false if profileId is an invalid index, true if the name parameter is set
    */
  bool GetProfileName(const unsigned int profileId, std::string& name) const;

  std::string GetUserDataFolder() const;
  std::string GetProfileUserDataFolder() const;
  std::string GetDatabaseFolder() const;
  std::string GetCDDBFolder() const;
  std::string GetThumbnailsFolder() const;
  std::string GetVideoThumbFolder() const;
  std::string GetBookmarksThumbFolder() const;
  std::string GetLibraryFolder() const;
  std::string GetSavestatesFolder() const;
  std::string GetSettingsFile() const;

  // uses HasSlashAtEnd to determine if a directory or file was meant
  std::string GetUserDataItem(const std::string& strFile) const;

  // Event log access
  CEventLog &GetEventLog();

protected:
  // implementation of ISettingCallback
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

private:
  /*! \brief Set the current profile id and update the special://profile path
    \param profileId profile index
    */
  void SetCurrentProfileId(unsigned int profileId);

  void PrepareLoadProfile(unsigned int profileIndex);
  void FinalizeLoadProfile();

  // Construction parameters
  std::shared_ptr<CSettings> m_settings;

  std::vector<CProfile> m_profiles;
  bool m_usingLoginScreen;
  bool m_profileLoadedForLogin;
  bool m_previousProfileLoadedForLogin;
  int m_autoLoginProfile;
  unsigned int m_lastUsedProfile;
  unsigned int m_currentProfile; // do not modify directly, use SetCurrentProfileId() function instead
  int m_nextProfileId; // for tracking the next available id to give to a new profile to ensure id's are not re-used
  mutable CCriticalSection m_critical;

  // Event properties
  std::unique_ptr<CEventLogManager> m_eventLogs;
};
