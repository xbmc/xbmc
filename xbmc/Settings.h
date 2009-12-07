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

#define DEFAULT_SKIN        "Confluence"
#ifdef MID
#define DEFAULT_VSYNC       VSYNC_DISABLED
#define DEFAULT_THUMB_SIZE  256
#else  // MID
#if defined(__APPLE__) || defined(_WIN32)
#define DEFAULT_VSYNC       VSYNC_ALWAYS
#else
#define DEFAULT_VSYNC       VSYNC_DRIVER
#endif
#define DEFAULT_THUMB_SIZE  512
#endif // MID

#include "settings/VideoSettings.h"
#include "Profile.h"
#include "ViewState.h"
#include "Resolution.h"
#include "GraphicContext.h" //TODO: only due to VIEW_TYPE

#include <vector>
#include <map>

#define CACHE_AUDIO 0
#define CACHE_VIDEO 1
#define CACHE_VOB   2

#define VOLUME_MINIMUM -6000  // -60dB
#define VOLUME_MAXIMUM 0      // 0dB
#define VOLUME_DRC_MINIMUM 0    // 0dB
#define VOLUME_DRC_MAXIMUM 3000 // 30dB

#define VIEW_MODE_NORMAL        0
#define VIEW_MODE_ZOOM          1
#define VIEW_MODE_STRETCH_4x3   2
#define VIEW_MODE_STRETCH_14x9  3
#define VIEW_MODE_STRETCH_16x9  4
#define VIEW_MODE_ORIGINAL      5
#define VIEW_MODE_CUSTOM        6

#define STACK_NONE          0
#define STACK_SIMPLE        1

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

struct VOICE_MASK {
  float energy;
  float pitch;
  float robotic;
  float whisper;
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

  bool Load(bool& bXboxMediacenter, bool& bSettings);
  void Save() const;
  bool Reset();

  void Clear();

  bool LoadProfile(int index);
  bool SaveSettingsToProfile(int index);
  bool DeleteProfile(int index);
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

  void ResetSkinSetting(const CStdString &setting);
  void ResetSkinSettings();

  struct stSettings
  {
public:
    CStdString m_pictureExtensions;
    CStdString m_musicExtensions;
    CStdString m_videoExtensions;

    CStdString m_logFolder;

    bool m_bMyMusicSongInfoInVis;
    bool m_bMyMusicSongThumbInVis;

    CViewState m_viewStateMusicNavArtists;
    CViewState m_viewStateMusicNavAlbums;
    CViewState m_viewStateMusicNavSongs;
    CViewState m_viewStateMusicShoutcast;
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

    int m_iMyVideoWatchMode;

    bool m_bMyVideoPlaylistRepeat;
    bool m_bMyVideoPlaylistShuffle;
    bool m_bMyVideoNavFlatten;
    bool m_bStartVideoWindowed;

    int m_iVideoStartWindow;

    int m_iMyVideoStack;

    int iAdditionalSubtitleDirectoryChecked;

    int m_HttpApiBroadcastPort;
    int m_HttpApiBroadcastLevel;
    int m_nVolumeLevel;                     // measured in milliBels -60dB -> 0dB range.
    int m_dynamicRangeCompressionLevel;     // measured in milliBels  0dB -> 30dB range.
    int m_iPreMuteVolumeLevel;    // save the m_nVolumeLevel for proper restore
    bool m_bMute;
    int m_iSystemTimeTotalUp;    // Uptime in minutes!

    VOICE_MASK m_karaokeVoiceMask[4];
  };

  stSettings m_stSettings;

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
  CStdString m_defaultVideoSource;
  CStdString m_defaultMusicLibSource;
  CStdString m_defaultVideoLibSource;

  CStdString m_UPnPUUIDServer;
  int        m_UPnPPortServer;
  int        m_UPnPMaxReturnedItems;
  CStdString m_UPnPUUIDRenderer;
  int        m_UPnPPortRenderer;

  //VECFILETYPEICONS m_vecIcons;
  VECPROFILES m_vecProfiles;
  int m_iLastLoadedProfileIndex;
  int m_iLastUsedProfileIndex;
  bool bUseLoginScreen;
  std::vector<RESOLUTION_INFO> m_ResInfo;

  // utility functions for user data folders

  //uses HasSlashAtEnd to determine if a directory or file was meant
  CStdString GetUserDataItem(const CStdString& strFile) const;
  CStdString GetProfileUserDataFolder() const;
  CStdString GetUserDataFolder() const;
  CStdString GetDatabaseFolder() const;
  CStdString GetCDDBFolder() const;
  CStdString GetThumbnailsFolder() const;
  CStdString GetMusicThumbFolder() const;
  CStdString GetLastFMThumbFolder() const;
  CStdString GetMusicArtistThumbFolder() const;
  CStdString GetVideoThumbFolder() const;
  CStdString GetBookmarksThumbFolder() const;
  CStdString GetPicturesThumbFolder() const;
  CStdString GetProgramsThumbFolder() const;
  CStdString GetGameSaveThumbFolder() const;
  CStdString GetProfilesThumbFolder() const;
  CStdString GetSourcesFile() const;
  CStdString GetSkinFolder() const;
  CStdString GetSkinFolder(const CStdString& skinName) const;
  CStdString GetScriptsFolder() const;
  CStdString GetVideoFanartFolder() const;
  CStdString GetMusicFanartFolder() const;

  CStdString GetSettingsFile() const;

  bool LoadUPnPXml(const CStdString& strSettingsFile);
  bool SaveUPnPXml(const CStdString& strSettingsFile) const;

  bool LoadProfiles(const CStdString& strSettingsFile);
  bool SaveProfiles(const CStdString& strSettingsFile) const;

  bool SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings = NULL) const;

  bool SaveSources();

  void LoadRSSFeeds();
  bool GetInteger(const TiXmlElement* pRootElement, const char *strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  bool GetFloat(const TiXmlElement* pRootElement, const char *strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  static bool GetPath(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue);
  static bool GetString(const TiXmlElement* pRootElement, const char *strTagName, CStdString& strValue, const CStdString& strDefaultValue);
  bool GetString(const TiXmlElement* pRootElement, const char *strTagName, char *szValue, const CStdString& strDefaultValue);
  bool GetSource(const CStdString &category, const TiXmlNode *source, CMediaSource &share);
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
};

extern class CSettings g_settings;
extern struct CSettings::stSettings& g_stSettings;
