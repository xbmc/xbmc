#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#define PRE_SKIN_VERSION_9_10_COMPATIBILITY 1
#define PRE_SKIN_VERSION_11_COMPATIBILITY 1

//FIXME - after eden - make that one nicer somehow...
#if defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV2)
#include "system.h" //for HAS_SKIN_TOUCHED
#endif

#if defined(HAS_SKIN_TOUCHED) && defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_IOS_ATV2)
#define DEFAULT_SKIN          "skin.touched"
#else
#define DEFAULT_SKIN          "skin.confluence"
#endif
#define DEFAULT_WEB_INTERFACE "webinterface.default"
#ifdef MID
#define DEFAULT_VSYNC       VSYNC_DISABLED
#else  // MID
#if defined(TARGET_DARWIN) || defined(_WIN32)
#define DEFAULT_VSYNC       VSYNC_ALWAYS
#else
#define DEFAULT_VSYNC       VSYNC_DRIVER
#endif
#endif // MID

#include "settings/VideoSettings.h"
#include "Profile.h"
#include "ViewState.h"
#include "guilib/Resolution.h"
#include "guilib/GraphicContext.h"

#include <vector>
#include <map>

#define CACHE_AUDIO 0
#define CACHE_VIDEO 1
#define CACHE_VOB   2

#define VOLUME_MINIMUM 0.0f        // -60dB
#define VOLUME_MAXIMUM 1.0f        // 0dB
#define VOLUME_DYNAMIC_RANGE 90.0f // 60dB
#define VOLUME_CONTROL_STEPS 90    // 90 steps
#define VOLUME_DRC_MINIMUM 0    // 0dB
#define VOLUME_DRC_MAXIMUM 6000 // 60dB

#define VIEW_MODE_NORMAL        0
#define VIEW_MODE_ZOOM          1
#define VIEW_MODE_STRETCH_4x3   2
#define VIEW_MODE_WIDE_ZOOM    3
#define VIEW_MODE_STRETCH_16x9  4
#define VIEW_MODE_ORIGINAL      5
#define VIEW_MODE_CUSTOM        6

#define VIDEO_SHOW_ALL 0
#define VIDEO_SHOW_UNWATCHED 1
#define VIDEO_SHOW_WATCHED 2

/* FIXME: eventually the profile should dictate where special://masterprofile/ is but for now it
   makes sense to leave all the profile settings in a user writeable location
   like special://masterprofile/ */
#define PROFILES_FILE "special://masterprofile/profiles.xml"

class CSkinString
{
public:
  CStdString name;
  CStdString value;
};

class CSkinBool
{
public:
  CSkinBool() : value(false) {};
  CStdString name;
  bool value;
};

class CGUISettings;
class TiXmlElement;
class TiXmlNode;
class CMediaSource;

class CSettings
{
public:
  CSettings(void);
  virtual ~CSettings(void);

  void Initialize();

  bool Load();
  void Save() const;
  bool Reset();

  void Clear();

  bool LoadProfile(unsigned int index);
  bool DeleteProfile(unsigned int index);
  void CreateProfileFolders();

  VECSOURCES *GetSourcesFromType(const CStdString &type);
  CStdString GetDefaultSourceFromType(const CStdString &type);

  bool UpdateSource(const CStdString &strType, const CStdString strOldName, const CStdString &strUpdateChild, const CStdString &strUpdateValue);
  bool DeleteSource(const CStdString &strType, const CStdString strName, const CStdString strPath, bool virtualSource = false);
  bool UpdateShare(const CStdString &type, const CStdString oldName, const CMediaSource &share);
  bool AddShare(const CStdString &type, const CMediaSource &share);

  int TranslateSkinString(const CStdString &setting);
  const CStdString &GetSkinString(int setting) const;
  void SetSkinString(int setting, const CStdString &label);

  int TranslateSkinBool(const CStdString &setting);
  bool GetSkinBool(int setting) const;
  void SetSkinBool(int setting, bool set);

  /*! \brief Retreive the watched mode for the given content type
   \param content Current content type
   \return the current watch mode for this content type, WATCH_MODE_ALL if the content type is unknown.
   \sa SetWatchMode, IncrementWatchMode
   */
  int GetWatchMode(const CStdString& content) const;

  /*! \brief Set the watched mode for the given content type
   \param content Current content type
   \param value Watched mode to set
   \sa GetWatchMode, IncrementWatchMode
   */
  void SetWatchMode(const CStdString& content, int value);

  /*! \brief Cycle the watched mode for the given content type
   \param content Current content type
   \sa GetWatchMode, SetWatchMode
   */
  void CycleWatchMode(const CStdString& content);

  void ResetSkinSetting(const CStdString &setting);
  void ResetSkinSettings();

  CStdString m_pictureExtensions;
  CStdString m_musicExtensions;
  CStdString m_videoExtensions;
  CStdString m_discStubExtensions;

  CStdString m_logFolder;

  bool m_bMyMusicSongInfoInVis;
  bool m_bMyMusicSongThumbInVis;

  CViewState m_viewStateMusicNavArtists;
  CViewState m_viewStateMusicNavAlbums;
  CViewState m_viewStateMusicNavSongs;
  CViewState m_viewStateMusicLastFM;
  CViewState m_viewStateVideoNavActors;
  CViewState m_viewStateVideoNavYears;
  CViewState m_viewStateVideoNavGenres;
  CViewState m_viewStateVideoNavTitles;
  CViewState m_viewStateVideoNavEpisodes;
  CViewState m_viewStateVideoNavSeasons;
  CViewState m_viewStateVideoNavTvShows;
  CViewState m_viewStateVideoNavMusicVideos;

  CViewState m_viewStatePrograms;
  CViewState m_viewStatePictures;
  CViewState m_viewStateMusicFiles;
  CViewState m_viewStateVideoFiles;

  bool m_bMyMusicPlaylistRepeat;
  bool m_bMyMusicPlaylistShuffle;
  int m_iMyMusicStartWindow;

  // for scanning
  bool m_bMyMusicIsScanning;

  CVideoSettings m_defaultVideoSettings;
  CVideoSettings m_currentVideoSettings;

  float m_fZoomAmount;      // current zoom amount
  float m_fPixelRatio;      // current pixel ratio
  float m_fVerticalShift;   // current vertical shift
  bool  m_bNonLinStretch;   // current non-linear stretch

  bool m_bMyVideoPlaylistRepeat;
  bool m_bMyVideoPlaylistShuffle;
  bool m_bMyVideoNavFlatten;
  bool m_bStartVideoWindowed;
  bool m_bAddonAutoUpdate;
  bool m_bAddonNotifications;
  bool m_bAddonForeignFilter;

  int m_iVideoStartWindow;

  bool m_videoStacking;

  int iAdditionalSubtitleDirectoryChecked;

  int m_HttpApiBroadcastPort;
  int m_HttpApiBroadcastLevel;
  float m_fVolumeLevel;        // float 0.0 - 1.0 range
  bool m_bMute;
  int m_iSystemTimeTotalUp;    // Uptime in minutes!

  CStdString m_userAgent;

  struct RssSet
  {
    bool rtl;
    std::vector<int> interval;
    std::vector<std::string> url;
  };

  std::map<int,RssSet> m_mapRssUrls;
  std::map<int, CSkinString> m_skinStrings;
  std::map<int, CSkinBool> m_skinBools;

  VECSOURCES m_programSources;
  VECSOURCES m_pictureSources;
  VECSOURCES m_fileSources;
  VECSOURCES m_musicSources;
  VECSOURCES m_videoSources;

  CStdString m_defaultProgramSource;
  CStdString m_defaultMusicSource;
  CStdString m_defaultPictureSource;
  CStdString m_defaultFileSource;
  CStdString m_defaultMusicLibSource;

  CStdString m_UPnPUUIDServer;
  int        m_UPnPPortServer;
  int        m_UPnPMaxReturnedItems;
  CStdString m_UPnPUUIDRenderer;
  int        m_UPnPPortRenderer;

  int        m_musicNeedsUpdate; ///< if a database update means an update is required (set to the version number of the db)
  int        m_videoNeedsUpdate; ///< if a database update means an update is required (set to the version number of the db)

  /*! \brief Retrieve the master profile
   \return const reference to the master profile
   */
  const CProfile &GetMasterProfile() const;

  /*! \brief Retreive the current profile
   \return const reference to the current profile
   */
  const CProfile &GetCurrentProfile() const;

  /*! \brief Retreive the profile from an index
   \param unsigned index of the profile to retrieve
   \return const pointer to the profile, NULL if the index is invalid
   */
  const CProfile *GetProfile(unsigned int index) const;

  /*! \brief Retreive the profile from an index
   \param unsigned index of the profile to retrieve
   \return pointer to the profile, NULL if the index is invalid
   */
  CProfile *GetProfile(unsigned int index);

  /*! \brief Retreive index of a particular profile by name
   \param name name of the profile index to retrieve
   \return index of this profile, -1 if invalid.
   */
  int GetProfileIndex(const CStdString &name) const;

  /*! \brief Retrieve the number of profiles
   \return number of profiles
   */
  unsigned int GetNumProfiles() const;

  /*! \brief Add a new profile
   \param profile CProfile to add
   */
  void AddProfile(const CProfile &profile);

  /*! \brief Are we using the login screen?
   \return true if we're using the login screen, false otherwise
   */
  bool UsingLoginScreen() const { return m_usingLoginScreen; };

  /*! \brief Toggle login screen use on and off
   Toggles the login screen state
   */
  void ToggleLoginScreen() { m_usingLoginScreen = !m_usingLoginScreen; };

  /*! \brief Are we the master user?
   \return true if the current profile is the master user, false otherwise
   */
  bool IsMasterUser() const { return 0 == m_currentProfile; };

  /*! \brief Update the date of the current profile
   */
  void UpdateCurrentProfileDate();

  /*! \brief Load the master user for the purposes of logging in
   Loads the master user.  Identical to LoadProfile(0) but doesn't update the last logged in details
   */
  void LoadMasterForLogin();

  /*! \brief Retreive the last used profile index
   \return the last used profile that logged in.  Does not count the master user during login.
   */
  unsigned int GetLastUsedProfileIndex() const { return m_lastUsedProfile; };

  /*! \brief Retrieve the current profile index
   \return the index of the currently logged in profile.
   */
  unsigned int GetCurrentProfileIndex() const { return m_currentProfile; };

  /*! \brief Retrieve the next id to use for a new profile
   \return the unique <id> to be used when creating a new profile
   */
  int GetNextProfileId() const { return m_nextIdProfile; }; // used to get the value of m_nextIdProfile for use in new profile creation

  int GetCurrentProfileId() const;

  std::vector<RESOLUTION_INFO> m_ResInfo;
  std::vector<RESOLUTION_INFO> m_Calibrations;

  // utility functions for user data folders

  //uses HasSlashAtEnd to determine if a directory or file was meant
  CStdString GetUserDataItem(const CStdString& strFile) const;
  CStdString GetProfileUserDataFolder() const;
  CStdString GetUserDataFolder() const;
  CStdString GetDatabaseFolder() const;
  CStdString GetCDDBFolder() const;
  CStdString GetThumbnailsFolder() const;
  CStdString GetVideoThumbFolder() const;
  CStdString GetBookmarksThumbFolder() const;
  CStdString GetSourcesFile() const;

  CStdString GetSettingsFile() const;

  bool LoadUPnPXml(const CStdString& strSettingsFile);
  bool SaveUPnPXml(const CStdString& strSettingsFile) const;

  /*! \brief Load the user profile information from disk
   Loads the profiles.xml file and creates the list of profiles. If no profiles
   exist, a master user is created.  Should be called after special://masterprofile/
   has been defined.
   \param profilesFile XML file to load.
   */
  void LoadProfiles(const CStdString& profilesFile);

  /*! \brief Save the user profile information to disk
   Saves the list of profiles to the profiles.xml file.
   \param profilesFile XML file to save.
   \return true on success, false on failure to save
   */
  bool SaveProfiles(const CStdString& profilesFile) const;

  bool SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings = NULL) const;

  void LoadSources();
  bool SaveSources();

  void LoadRSSFeeds();
  bool GetInteger(const TiXmlElement* pRootElement, const char *strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  bool GetFloat(const TiXmlElement* pRootElement, const char *strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  static bool GetPath(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue);
  static bool GetString(const TiXmlElement* pRootElement, const char *strTagName, CStdString& strValue, const CStdString& strDefaultValue);
  bool GetString(const TiXmlElement* pRootElement, const char *strTagName, char *szValue, const CStdString& strDefaultValue);
  bool GetSource(const CStdString &category, const TiXmlNode *source, CMediaSource &share);

  void ApplyCalibrations();
  void UpdateCalibrations();
protected:
  void GetSources(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSOURCES& items, CStdString& strDefault);
  bool SetSources(TiXmlNode *root, const char *section, const VECSOURCES &shares, const char *defaultPath);
  void GetViewState(const TiXmlElement* pRootElement, const CStdString& strTagName, CViewState &viewState, SORT_METHOD defaultSort = SORT_METHOD_LABEL, int defaultView = DEFAULT_VIEW_LIST);

  // functions for writing xml files
  void SetViewState(TiXmlNode* pRootNode, const CStdString& strTagName, const CViewState &viewState) const;

  bool LoadCalibration(const TiXmlElement* pElement, const CStdString& strSettingsFile);
  bool SaveCalibration(TiXmlNode* pRootNode) const;

  bool LoadSettings(const CStdString& strSettingsFile);
//  bool SaveSettings(const CStdString& strSettingsFile) const;

  bool LoadPlayerCoreFactorySettings(const CStdString& fileStr, bool clear);

  // skin activated settings
  void LoadSkinSettings(const TiXmlElement* pElement);
  void SaveSkinSettings(TiXmlNode *pElement) const;

  void LoadUserFolderLayout();

private:
  std::vector<CProfile> m_vecProfiles;
  std::map<CStdString, int> m_watchMode;
  bool m_usingLoginScreen;
  unsigned int m_lastUsedProfile;
  unsigned int m_currentProfile;
  int m_nextIdProfile; // for tracking the next available id to give to a new profile to ensure id's are not re-used
};

extern class CSettings g_settings;
