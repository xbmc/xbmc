#pragma once

// REMOVE ME WHEN WE SWITCH TO SKIN VERSION 2.1
#define PRE_SKIN_VERSION_2_1_COMPATIBILITY 1
// REMOVE ME WHEN WE SWITCH TO SKIN VERSION 2.1

class CProfile;
#include "settings/VideoSettings.h"
#include "../xbmc/StringUtils.h"
#include "GUISettings.h"
#include "FileItem.h"
#include "XBVideoConfig.h"

#include <vector>

#define SHARE_TYPE_UNKNOWN      0
#define SHARE_TYPE_LOCAL        1
#define SHARE_TYPE_DVD          2
#define SHARE_TYPE_VIRTUAL_DVD  3
#define SHARE_TYPE_REMOTE       4
#define SHARE_TYPE_VPATH        5

#define LOCK_MODE_UNKNOWN            -1
#define LOCK_MODE_EVERYONE            0
#define LOCK_MODE_NUMERIC             1
#define LOCK_MODE_GAMEPAD             2
#define LOCK_MODE_QWERTY              3
#define LOCK_MODE_SAMBA               4
#define LOCK_MODE_EEPROM_PARENTAL     5

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
#define STACK_FUZZY         2
#define STACK_UNAVAILABLE   4

#define VIDEO_SHOW_ALL 0
#define VIDEO_SHOW_UNWATCHED 1
#define VIDEO_SHOW_WATCHED 2

#define DEFAULT_VIEW_AUTO (VIEW_TYPE_AUTO << 16)
#define DEFAULT_VIEW_LIST (VIEW_TYPE_LIST << 16)
#define DEFAULT_VIEW_ICONS (VIEW_TYPE_ICON << 16)
#define DEFAULT_VIEW_BIG_ICONS (VIEW_TYPE_BIG_ICON << 16)
#define DEFAULT_VIEW_MAX (((VIEW_TYPE_MAX - 1) << 16) | 60)

class CViewState
{
public:
  CViewState(int viewMode, SORT_METHOD sortMethod, SORT_ORDER sortOrder)
  {
    m_viewMode = viewMode;
    m_sortMethod = sortMethod;
    m_sortOrder = sortOrder;
  };
  CViewState()
  {
    m_viewMode = 0;
    m_sortMethod = SORT_METHOD_LABEL;
    m_sortOrder = SORT_ORDER_ASC;
  };

  int m_viewMode;
  SORT_METHOD m_sortMethod;
  SORT_ORDER m_sortOrder;
};

/*!
\ingroup windows
\brief Represents a share.
\sa VECSHARES, IVECSHARES
*/
class CShare
{
public:
  CShare() { m_iDriveType=SHARE_TYPE_UNKNOWN; m_iLockMode=LOCK_MODE_EVERYONE; m_iBadPwdCount=0; m_iHasLock=0; m_ignore=false; };
  virtual ~CShare() {};

  void FromNameAndPaths(const CStdString &category, const CStdString &name, const vector<CStdString> &paths);
  bool isWritable() const;
  CStdString strName; ///< Name of the share, can be choosen freely.
  CStdString strStatus; ///< Status of the share (eg has disk etc.)
  CStdString strPath; ///< Path of the share, eg. iso9660:// or F:

  /*!
  \brief The type of the share.

  Value can be:
  - SHARE_TYPE_UNKNOWN \n
  Unknown share, maybe a wrong path.
  - SHARE_TYPE_LOCAL \n
  Harddisk share.
  - SHARE_TYPE_DVD \n
  DVD-ROM share of the build in drive, strPath may vary.
  - SHARE_TYPE_VIRTUAL_DVD \n
  DVD-ROM share, strPath is fix.
  - SHARE_TYPE_REMOTE \n
  Network share.
  */
  int m_iDriveType;

  /*!
  \brief The type of Lock UI to show when accessing the share.

  Value can be:
  - LOCK_MODE_EVERYONE \n
  Default value.  No lock UI is shown, user can freely access the share.
  - LOCK_MODE_NUMERIC \n
  Lock code is entered via OSD numpad or IrDA remote buttons.
  - LOCK_MODE_GAMEPAD \n
  Lock code is entered via XBOX gamepad buttons.
  - LOCK_MODE_QWERTY \n
  Lock code is entered via OSD keyboard or PC USB keyboard.
  - LOCK_MODE_SAMBA \n
  Lock code is entered via OSD keyboard or PC USB keyboard and passed directly to SMB for authentication.
  - LOCK_MODE_EEPROM_PARENTAL \n
  Lock code is retrieved from XBOX EEPROM and entered via XBOX gamepad or remote.
  - LOCK_MODE_UNKNOWN \n
  Value is unknown or unspecified.
  */
  int m_iLockMode;
  CStdString m_strLockCode;  ///< Input code for Lock UI to verify, can be chosen freely.
  int m_iHasLock;
  int m_iBadPwdCount; ///< Number of wrong passwords user has entered since share was last unlocked

  CStdString m_strThumbnailImage; ///< Path to a thumbnail image for the share, or blank for default

  vector<CStdString> vecPaths;
  bool m_ignore; /// <Do not store in xml
};

class CSkinString
{
public:
  CStdString name;
  CStdString value;
};

class CSkinBool
{
public:
  CStdString name;
  bool value;
};

/*!
\ingroup windows
\brief A vector to hold CShare objects.
\sa CShare, IVECSHARES
*/
typedef std::vector<CShare> VECSHARES;

/*!
\ingroup windows
\brief Iterator of VECSHARES.
\sa CShare, VECSHARES
*/
typedef std::vector<CShare>::iterator IVECSHARES;

typedef std::vector<CProfile> VECPROFILES;
typedef std::vector<CProfile>::iterator IVECPROFILES;

struct VOICE_MASK {
  float energy;
  float pitch;
  float robotic;
  float whisper;
};

class CSettings
{
public:
  CSettings(void);
  virtual ~CSettings(void);

  bool Load(bool& bXboxMediacenter, bool& bSettings);
  void Save() const;
  bool Reset();

  void Clear();

  bool LoadProfile(int index);
  bool SaveSettingsToProfile(int index);
  bool DeleteProfile(int index);

  VECSHARES *GetSharesFromType(const CStdString &type);
  CStdString GetDefaultShareFromType(const CStdString &type);

  bool UpdateSource(const CStdString &strType, const CStdString strOldName, const CStdString &strUpdateChild, const CStdString &strUpdateValue);
  bool DeleteSource(const CStdString &strType, const CStdString strName, const CStdString strPath);
  bool UpdateShare(const CStdString &type, const CStdString oldName, const CShare &share);
  bool AddShare(const CStdString &type, const CShare &share);

  int TranslateSkinString(const CStdString &setting);
  const CStdString &GetSkinString(int setting) const;
  void SetSkinString(int setting, const CStdString &label);

  int TranslateSkinBool(const CStdString &setting);
  bool GetSkinBool(int setting) const;
  void SetSkinBool(int setting, bool set);

  void ResetSkinSetting(const CStdString &setting);
  void ResetSkinSettings();

  struct AdvancedSettings
  {
public:
    // multipath testing
    bool m_useMultipaths;
    bool m_DisableModChipDetection;

    int m_audioHeadRoom;
    float m_karaokeSyncDelay;

    float m_videoSubsDelayRange;
    float m_videoAudioDelayRange;
    int m_videoSmallStepBackSeconds;
    int m_videoSmallStepBackTries;
    int m_videoSmallStepBackDelay;
    bool m_videoUseTimeSeeking;
    int m_videoTimeSeekForward;
    int m_videoTimeSeekBackward;
    int m_videoTimeSeekForwardBig;
    int m_videoTimeSeekBackwardBig;
    int m_videoPercentSeekForward;
    int m_videoPercentSeekBackward;
    int m_videoPercentSeekForwardBig;
    int m_videoPercentSeekBackwardBig;
    int m_videoBlackBarColour;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_lcdRows;
    int m_lcdColumns;
    int m_lcdAddress1;
    int m_lcdAddress2;
    int m_lcdAddress3;
    int m_lcdAddress4;

    int m_autoDetectPingTime;
    float m_playCountMinimumPercent;

    int m_songInfoDuration;
    int m_busyDialogDelay;
    int m_logLevel;
    CStdString m_cddbAddress;
    bool m_usePCDVDROM;
    bool m_noDVDROM;
    CStdString m_cachePath;
    bool m_displayRemoteCodes;
    CStdStringArray m_videoStackRegExps;
    CStdStringArray m_tvshowStackRegExps;
    CStdStringArray m_tvshowTwoPartStackRegExps;
    CStdStringArray m_pathSubstitutions;
    int m_remoteRepeat;
    float m_controllerDeadzone;
    bool m_FTPShowCache;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    int m_thumbSize;

    int m_sambaclienttimeout;
    CStdString m_sambadoscodepage;
    CStdString m_musicThumbs;
    CStdString m_dvdThumbs;

    bool m_bMusicLibraryHideAllItems;
    bool m_bMusicLibraryAllItemsOnBottom;
    bool m_bMusicLibraryHideCompilationArtists;
    bool m_bMusicLibraryAlbumsSortByArtistThenYear;
    CStdString m_strMusicLibraryAlbumFormat;
    CStdString m_strMusicLibraryAlbumFormatRight;
    bool m_prioritiseAPEv2tags;
    CStdString m_musicItemSeparator;
    CStdString m_videoItemSeparator;

    bool m_bVideoLibraryHideAllItems;
    bool m_bVideoLibraryAllItemsOnBottom;
    bool m_bVideoLibraryHideRecentlyAddedItems;

    bool m_bUseEvilB;
    std::vector<CStdString> m_vecTokens; // cleaning strings tied to language
    //TuxBox
    bool m_bTuxBoxSubMenuSelection;
    int m_iTuxBoxDefaultSubMenu;
    int m_iTuxBoxDefaultRootMenu;
    bool m_bTuxBoxAudioChannelSelection;
    bool m_bTuxBoxPictureIcon;
    int m_iTuxBoxEpgRequestTime;
    int m_iTuxBoxZapWaitTime;
    bool m_bTuxBoxSendAllAPids;

    int m_curlclienttimeout;
    
#ifdef HAS_SDL
    bool m_fullScreen;
#endif
  };
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

    int m_iVideoStartWindow;

    int m_iMyVideoStack;
    bool m_bMyVideoCleanTitles;
    char m_szMyVideoCleanTokens[256];
    char m_szMyVideoCleanSeparators[32];

    int iAdditionalSubtitleDirectoryChecked;

    char szOnlineArenaPassword[32]; // private arena password
    char szOnlineArenaDescription[64]; // private arena description

	  int m_HttpApiBroadcastPort;
	  int m_HttpApiBroadcastLevel;
    int m_nVolumeLevel;                     // measured in milliBels -60dB -> 0dB range.
    int m_dynamicRangeCompressionLevel;     // measured in milliBels  0dB -> 30dB range.
    int m_iPreMuteVolumeLevel;    // save the m_nVolumeLevel for proper restore
    bool m_bMute;
    int m_iSystemTimeTotalUp;    // Uptime in minutes!

    VOICE_MASK m_karaokeVoiceMask[4];
  };

  std::map<int,std::pair<std::vector<int>,std::vector<string> > > m_mapRssUrls;
  std::map<int, CSkinString> m_skinStrings;
  std::map<int, CSkinBool> m_skinBools;

  // cache copies of these parsed values, to avoid re-parsing over and over
  CStdString m_szMyVideoStackSeparatorsString;
  CStdStringArray m_szMyVideoStackTokensArray;
  CStdString m_szMyVideoCleanSeparatorsString;
  CStdStringArray m_szMyVideoCleanTokensArray;

  VECSHARES m_programSources;
  VECSHARES m_pictureSources;
  VECSHARES m_fileSources;
  VECSHARES m_musicSources;
  VECSHARES m_videoSources;

  CStdString m_defaultProgramSource;
  CStdString m_defaultMusicSource;
  CStdString m_defaultPictureSource;
  CStdString m_defaultFileSource;
  CStdString m_defaultVideoSource;
  CStdString m_defaultMusicLibSource;
  CStdString m_defaultVideoLibSource;

  VECSHARES m_UPnPMusicSources;
  VECSHARES m_UPnPVideoSources;
  VECSHARES m_UPnPPictureSources;

  CStdString m_UPnPUUID;
  CStdString m_UPnPUUIDRenderer;

  //VECFILETYPEICONS m_vecIcons;
  VECPROFILES m_vecProfiles;
  int m_iLastLoadedProfileIndex;
  int m_iLastUsedProfileIndex;
  bool bUseLoginScreen;
  RESOLUTION_INFO m_ResInfo[CUSTOM+MAX_RESOLUTIONS];

  // utility functions for user data folders
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
  CStdString GetXLinkKaiThumbFolder() const;
  CStdString GetProfilesThumbFolder() const;
  CStdString GetSourcesFile() const;
  CStdString GetSkinFolder() const;

  CStdString GetSettingsFile() const;
  CStdString GetAvpackSettingsFile() const;

  bool LoadUPnPXml(const CStdString& strSettingsFile);
  bool SaveUPnPXml(const CStdString& strSettingsFile) const;
  
  bool LoadProfiles(const CStdString& strSettingsFile);
  bool SaveProfiles(const CStdString& strSettingsFile) const;

  bool SaveSettings(const CStdString& strSettingsFile) const;

  bool SaveSources();

protected:
  void GetInteger(const TiXmlElement* pRootElement, const char *strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  void GetFloat(const TiXmlElement* pRootElement, const char *strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  void GetString(const TiXmlElement* pRootElement, const char *strTagName, CStdString& strValue, const CStdString& strDefaultValue);
  void GetString(const TiXmlElement* pRootElement, const char *strTagName, char *szValue, const CStdString& strDefaultValue);
  bool GetShare(const CStdString &category, const TiXmlNode *source, CShare &share);
  void GetShares(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSHARES& items, CStdString& strDefault);
  bool SetShares(TiXmlNode *root, const char *section, const VECSHARES &shares, const char *defaultPath);
  void GetViewState(const TiXmlElement* pRootElement, const CStdString& strTagName, CViewState &viewState);

  void ConvertHomeVar(CStdString& strText);
  // functions for writing xml files
  void SetString(TiXmlNode* pRootNode, const CStdString& strTagName, const CStdString& strValue) const;
  void SetInteger(TiXmlNode* pRootNode, const CStdString& strTagName, int iValue) const;
  void SetFloat(TiXmlNode* pRootNode, const CStdString& strTagName, float fValue) const;
  void SetBoolean(TiXmlNode* pRootNode, const CStdString& strTagName, bool bValue) const;
  void SetHex(TiXmlNode* pRootNode, const CStdString& strTagName, DWORD dwHexValue) const;
  void SetViewState(TiXmlNode* pRootNode, const CStdString& strTagName, const CViewState &viewState) const;

  bool LoadCalibration(const TiXmlElement* pElement, const CStdString& strSettingsFile);
  bool SaveCalibration(TiXmlNode* pRootNode) const;

  bool LoadSettings(const CStdString& strSettingsFile);
//  bool SaveSettings(const CStdString& strSettingsFile) const;

  // skin activated settings
  void LoadSkinSettings(const TiXmlElement* pElement);
  void SaveSkinSettings(TiXmlNode *pElement) const;

  // Advanced settings
  void LoadAdvancedSettings();

  void LoadUserFolderLayout();

  void LoadRSSFeeds();

  bool SaveAvpackXML() const;
  bool SaveNewAvpackXML() const;
  bool SaveAvpackSettings(TiXmlNode *io_pRoot) const;
  bool LoadAvpackXML();

};

extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;
extern struct CSettings::AdvancedSettings g_advancedSettings;
