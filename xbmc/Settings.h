#pragma once

// REMOVE ME WHEN WE SWITCH TO SKIN VERSION 2.1
#define PRE_SKIN_VERSION_2_1_COMPATIBILITY 1
// REMOVE ME WHEN WE SWITCH TO SKIN VERSION 2.1

class CProfile;
#include "settings/VideoSettings.h"
#include "../xbmc/StringUtils.h"
#include "GUISettings.h"
#include "fileitem.h"

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

#define VIDEO_SHOW_ALL 0
#define VIDEO_SHOW_UNWATCHED 1
#define VIDEO_SHOW_WATCHED 2

class CFolderView
{
public:
  CFolderView(CStdString &strPath, int iView, int iSort, bool bSortOrder)
  {
    m_strPath = strPath;
    m_iView = iView;
    m_iSort = iSort;
    m_bSortOrder = bSortOrder;
  };
  ~CFolderView() {};

  CStdString m_strPath;
  int m_iView;
  int m_iSort;
  bool m_bSortOrder;
};

typedef std::vector<CFolderView*> VECFOLDERVIEWS;

/*!
\ingroup windows
\brief Represents a share.
\sa VECSHARES, IVECSHARES
*/
class CShare
{
public:
  CShare() { m_iBufferSize=0; m_iDriveType=SHARE_TYPE_UNKNOWN; m_iLockMode=LOCK_MODE_EVERYONE; m_iBadPwdCount=0; };
  virtual ~CShare() {};
  bool isWritable()
  {
    if (strPath[1] == ':' && (strPath[0] != 'D' && strPath[0] != 'd'))
      return true; // local disk
    if (strPath.size() > 4)
    {
      if (strPath.substr(0,4) == "smb:")
        return true; // smb path
    }

    return false;
  }
  void FromNameAndPaths(const CStdString &category, const CStdString &name, const vector<CStdString> &paths);

  CStdString strName; ///< Name of the share, can be choosen freely.
  CStdString strStatus; ///< Status of the share (eg has disk etc.)
  CStdString strPath; ///< Path of the share, eg. iso9660:// or F:
  CStdString strEntryPoint; ///< entry point of shares, used with archives
  int m_iBufferSize;   ///< Cachesize of the share

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
/*
class CFileTypeIcon
{
public:
  CFileTypeIcon(){};
  virtual ~CFileTypeIcon(){};
  CStdString m_strName;
  CStdString m_strIcon;
};
typedef std::vector<CFileTypeIcon> VECFILETYPEICONS;
typedef std::vector<CFileTypeIcon>::iterator IVECFILETYPEICONS;
*/
struct VOICE_MASK {
  float energy;
  float pitch;
  float robotic;
  float whisper;
};

#include "GUIViewState.h" // for the VIEW_METHOD enum type

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

  void BeginBookmarkTransaction();
  bool UpdateBookmark(const CStdString &strType, const CStdString strOldName, const CStdString &strUpdateChild, const CStdString &strUpdateValue);
  bool CommitBookmarkTransaction();
  bool DeleteBookmark(const CStdString &strType, const CStdString strName, const CStdString strPath);
  bool UpdateShare(const CStdString &type, const CStdString oldName, const CShare &share);
  bool AddShare(const CStdString &type, const CShare &share);

  bool LoadFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders);
  bool SaveFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders);

  bool UpDateXbmcXML(const CStdString &strFirstChild, const CStdString &strChild, const CStdString &strChildValue);
  bool UpDateXbmcXML(const CStdString &strFirstChild, const CStdString &strFirstChildValue);

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
    int m_logLevel;
    CStdString m_cddbAddress;
    CStdString m_imdbAddress;
    bool m_autoDetectFG;
    bool m_useFDrive;
    bool m_useGDrive;
    bool m_usePCDVDROM;
    CStdString m_cachePath;
    bool m_displayRemoteCodes;
    CStdStringArray m_videoStackRegExps;
    CStdStringArray m_pathSubstitutions;
    int m_remoteRepeat;
    float m_controllerDeadzone;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    int m_thumbSize;

    int m_sambaclienttimeout;
    CStdString m_sambadoscodepage;
    CStdString m_musicThumbs;
  };
  struct stSettings
  {
public:
    CStdString m_pictureExtensions;
    CStdString m_musicExtensions;
    CStdString m_videoExtensions;

    CStdString m_logFolder;

    char m_szDefaultPrograms[128];
    char m_szDefaultMusic[128];
    char m_szDefaultPictures[128];
    char m_szDefaultFiles[128];
    char m_szDefaultVideos[128];

    bool m_bMyMusicSongInfoInVis;
    bool m_bMyMusicSongThumbInVis;

    bool m_bMyMusicPlaylistRepeat;
    bool m_bMyMusicPlaylistShuffle;
    int m_iMyMusicStartWindow;

    VIEW_METHOD m_MyMusicSongsRootViewMethod;
    SORT_METHOD m_MyMusicSongsRootSortMethod;
    SORT_ORDER m_MyMusicSongsRootSortOrder;
    VIEW_METHOD m_MyMusicSongsViewMethod;
    SORT_METHOD m_MyMusicSongsSortMethod;
    SORT_ORDER m_MyMusicSongsSortOrder;

    VIEW_METHOD m_MyMusicPlaylistViewMethod;

    SORT_METHOD m_MyMusicShoutcastSortMethod;
    SORT_ORDER  m_MyMusicShoutcastSortOrder;
    SORT_METHOD m_MyMusicLastFMSortMethod;
    SORT_ORDER  m_MyMusicLastFMSortOrder;

    // new settings for the Music Nav Window
    VIEW_METHOD m_MyMusicNavRootViewMethod;
    VIEW_METHOD m_MyMusicNavGenresViewMethod;
    VIEW_METHOD m_MyMusicNavArtistsViewMethod;
    VIEW_METHOD m_MyMusicNavAlbumsViewMethod;
    VIEW_METHOD m_MyMusicNavSongsViewMethod;
    VIEW_METHOD m_MyMusicNavTopViewMethod;
    VIEW_METHOD m_MyMusicNavPlaylistsViewMethod;

    SORT_METHOD m_MyMusicNavRootSortMethod;
    SORT_METHOD m_MyMusicNavAlbumsSortMethod;
    SORT_METHOD m_MyMusicNavSongsSortMethod;
    SORT_METHOD m_MyMusicNavPlaylistsSortMethod;

    SORT_ORDER m_MyMusicNavGenresSortOrder;
    SORT_ORDER m_MyMusicNavArtistsSortOrder;
    SORT_ORDER m_MyMusicNavAlbumsSortOrder;
    SORT_ORDER m_MyMusicNavSongsSortOrder;
    SORT_ORDER m_MyMusicNavPlaylistsSortOrder;

    VIEW_METHOD m_ScriptsViewMethod;
    SORT_METHOD m_ScriptsSortMethod;
    SORT_ORDER m_ScriptsSortOrder;

    VIEW_METHOD m_MyProgramsViewMethod;
    SORT_METHOD m_MyProgramsSortMethod;
    SORT_ORDER m_MyProgramsSortOrder;

    VIEW_METHOD m_MyPicturesViewMethod;
    SORT_METHOD m_MyPicturesSortMethod;
    SORT_ORDER m_MyPicturesSortOrder;
    VIEW_METHOD m_MyPicturesRootViewMethod;
    SORT_METHOD m_MyPicturesRootSortMethod;
    SORT_ORDER m_MyPicturesRootSortOrder;

    // new settings for the Video Nav Window
    VIEW_METHOD m_MyVideoNavRootViewMethod;
    VIEW_METHOD m_MyVideoNavGenreViewMethod;
    VIEW_METHOD m_MyVideoNavPlaylistsViewMethod;
    VIEW_METHOD m_MyVideoNavTitleViewMethod;
    VIEW_METHOD m_MyVideoNavActorViewMethod;
    VIEW_METHOD m_MyVideoNavYearViewMethod;

    SORT_METHOD m_MyVideoNavGenreSortMethod;
    SORT_METHOD m_MyVideoNavPlaylistsSortMethod;
    SORT_METHOD m_MyVideoNavTitleSortMethod;
    
    SORT_ORDER m_MyVideoNavGenreSortOrder;
    SORT_ORDER m_MyVideoNavPlaylistsSortOrder;
    SORT_ORDER m_MyVideoNavTitleSortOrder;
    SORT_ORDER m_MyVideoNavYearSortOrder;
    SORT_ORDER m_MyVideoNavActorSortOrder;
    
    // for scanning
    bool m_bMyMusicIsScanning;

    CVideoSettings m_defaultVideoSettings;
    CVideoSettings m_currentVideoSettings;

    float m_fZoomAmount;      // current zoom amount
    float m_fPixelRatio;      // current pixel ratio

    int m_iMyVideoWatchMode;

    VIEW_METHOD m_MyVideoViewMethod;
    VIEW_METHOD m_MyVideoRootViewMethod;
    SORT_METHOD m_MyVideoSortMethod;
    SORT_METHOD m_MyVideoRootSortMethod;
    SORT_ORDER m_MyVideoSortOrder;
    SORT_ORDER m_MyVideoRootSortOrder;

    VIEW_METHOD m_MyVideoPlaylistViewMethod;

    bool m_bMyVideoPlaylistRepeat;
    bool m_bMyVideoPlaylistShuffle;

    int m_iVideoStartWindow;

    int m_iMyVideoStack;
    bool m_bMyVideoCleanTitles;
    char m_szMyVideoCleanTokens[256];
    char m_szMyVideoCleanSeparators[32];

    char m_szAlternateSubtitleDirectory[128];

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

  VECSHARES m_vecMyProgramsShares;
  VECSHARES m_vecMyPictureShares;
  VECSHARES m_vecMyFilesShares;
  VECSHARES m_vecMyMusicShares;
  VECSHARES m_vecMyVideoShares;
  VECSHARES m_vecSambeShres;
  //VECFILETYPEICONS m_vecIcons;
  VECPROFILES m_vecProfiles;
  int m_iLastLoadedProfileIndex;
  int m_iLastUsedProfileIndex;
  bool bUseLoginScreen;
  RESOLUTION_INFO m_ResInfo[10];

  // utility functions for user data folders
  CStdString GetUserDataItem(const CStdString& strFile) const;
  CStdString GetProfileUserDataFolder() const;
  CStdString GetUserDataFolder() const;
  CStdString GetDatabaseFolder() const;
  CStdString GetCDDBFolder() const;
  const CStdString GetIMDbFolder() const;
  CStdString GetThumbnailsFolder() const;
  CStdString GetMusicThumbFolder() const;
  CStdString GetMusicArtistThumbFolder() const;
  CStdString GetVideoThumbFolder() const;
  CStdString GetBookmarksThumbFolder() const;
  CStdString GetPicturesThumbFolder() const;
  CStdString GetProgramsThumbFolder() const;
  CStdString GetXLinkKaiThumbFolder() const;
  CStdString GetProfilesThumbFolder() const;
  CStdString GetSourcesFile() const;
  CStdString GetSkinFolder() const;

  CStdString GetSettingsFile() const;
  CStdString GetAvpackSettingsFile() const;

  bool LoadProfiles(const CStdString& strSettingsFile);
  bool SaveProfiles(const CStdString& strSettingsFile) const;

  bool SaveSettings(const CStdString& strSettingsFile) const;
  TiXmlDocument xbmcXml;  // for editing the xml file from within XBMC
  
protected:
  void GetInteger(const TiXmlElement* pRootElement, const char *strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  void GetFloat(const TiXmlElement* pRootElement, const char *strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  void GetString(const TiXmlElement* pRootElement, const char *strTagName, CStdString& strValue, const CStdString& strDefaultValue);
  void GetString(const TiXmlElement* pRootElement, const char *strTagName, char *szValue, const CStdString& strDefaultValue);
  bool GetShare(const CStdString &category, const TiXmlNode *bookmark, CShare &share);
  void GetShares(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSHARES& items, CStdString& strDefault);
  void ConvertHomeVar(CStdString& strText);
  // functions for writing xml files
  void SetString(TiXmlNode* pRootNode, const CStdString& strTagName, const CStdString& strValue) const;
  void SetInteger(TiXmlNode* pRootNode, const CStdString& strTagName, int iValue) const;
  void SetFloat(TiXmlNode* pRootNode, const CStdString& strTagName, float fValue) const;
  void SetBoolean(TiXmlNode* pRootNode, const CStdString& strTagName, bool bValue) const;
  void SetHex(TiXmlNode* pRootNode, const CStdString& strTagName, DWORD dwHexValue) const;

  bool LoadCalibration(const TiXmlElement* pElement, const CStdString& strSettingsFile);
  bool SaveCalibration(TiXmlNode* pRootNode) const;

  bool LoadSettings(const CStdString& strSettingsFile);
//  bool SaveSettings(const CStdString& strSettingsFile) const;

  bool LoadXml();
  void CloseXml();

  // skin activated settings
  void LoadSkinSettings(const TiXmlElement* pElement);
  void SaveSkinSettings(TiXmlNode *pElement) const;

  // Advanced settings
  void LoadAdvancedSettings();

  void LoadUserFolderLayout(const TiXmlElement *pRootElement);

  void LoadRSSFeeds();

  CStdString  GetPluggedAvpack() const;
  bool SaveAvpackXML() const;
  bool SaveNewAvpackXML() const;
  bool SaveAvpackSettings(TiXmlNode *io_pRoot) const;
  bool LoadAvpackXML();

  //TiXmlDocument xbmcXml;  // for editing the xml file from within XBMC
  bool xbmcXmlLoaded;
  bool bTransaction;
  bool bChangedDuringTransaction;
};

extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;
extern struct CSettings::AdvancedSettings g_advancedSettings;
