#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include <stdint.h>
#include <vector>

#include "profiles/Profile.h"
#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"

class TiXmlNode;

class CProfilesManager : public ISettingsHandler
{
public:
  static CProfilesManager& Get();

  virtual void OnSettingsLoaded();
  virtual void OnSettingsSaved();
  virtual void OnSettingsCleared();

  bool Load();
  /*! \brief Load the user profile information from disk
    Loads the profiles.xml file and creates the list of profiles.
    If no profiles exist, a master user is created. Should be called
    after special://masterprofile/ has been defined.
    \param file XML file to load.
    */
  bool Load(const std::string &file);

  bool Save();
  /*! \brief Save the user profile information to disk
    Saves the list of profiles to the profiles.xml file.
    \param file XML file to save.
    \return true on success, false on failure to save
    */
  bool Save(const std::string &file) const;

  void Clear();

  bool LoadProfile(size_t index);
  bool DeleteProfile(size_t index);

  void CreateProfileFolders();

  /*! \brief Retrieve the master profile
    \return const reference to the master profile
    */
  const CProfile& GetMasterProfile() const;

  /*! \brief Retreive the current profile
    \return const reference to the current profile
    */
  const CProfile& GetCurrentProfile() const;

  /*! \brief Retreive the profile from an index
    \param unsigned index of the profile to retrieve
    \return const pointer to the profile, NULL if the index is invalid
    */
  const CProfile* GetProfile(size_t index) const;

  /*! \brief Retreive the profile from an index
    \param unsigned index of the profile to retrieve
    \return pointer to the profile, NULL if the index is invalid
    */
  CProfile* GetProfile(size_t index);

  /*! \brief Retreive index of a particular profile by name
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
  void ToggleLoginScreen() { m_usingLoginScreen = !m_usingLoginScreen; }

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

  /*! \brief Retreive the last used profile index
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
  void SetAutoLoginProfileId(const int profileId) { m_autoLoginProfile = profileId; }

  /*! \brief Retrieve the name of a particular profile by index
    \param profileId profile index for which to retrieve the name
    \param name will hold the name of the profile when a valid profile index has been provided
    \return false if profileId is an invalid index, true if the name parameter is set
    */
  bool GetProfileName(const size_t profileId, std::string& name) const;

  std::string GetUserDataFolder() const;
  std::string GetProfileUserDataFolder() const;
  std::string GetDatabaseFolder() const;
  std::string GetCDDBFolder() const;
  std::string GetThumbnailsFolder() const;
  std::string GetVideoThumbFolder() const;
  std::string GetBookmarksThumbFolder() const;
  std::string GetLibraryFolder() const;
  std::string GetSettingsFile() const;

  // uses HasSlashAtEnd to determine if a directory or file was meant
  std::string GetUserDataItem(const std::string& strFile) const;

protected:
  CProfilesManager();
  CProfilesManager(const CProfilesManager&);
  CProfilesManager const& operator=(CProfilesManager const&);
  virtual ~CProfilesManager();

private:
  /*! \brief Set the current profile id and update the special://profile path
    \param profileId profile index
    */
  void SetCurrentProfileId(size_t profileId);

  std::vector<CProfile> m_profiles;
  bool m_usingLoginScreen;
  int m_autoLoginProfile;
  uint32_t m_lastUsedProfile;
  uint32_t m_currentProfile; // do not modify directly, use SetCurrentProfileId() function instead
  int m_nextProfileId; // for tracking the next available id to give to a new profile to ensure id's are not re-used
  CCriticalSection m_critical;
};
