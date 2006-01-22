#pragma once

// REMOVE ME WHEN WE SWITCH TO SKIN VERSION 2.0
#define PRE_SKIN_VERSION_2_0_COMPATIBILITY 1
// REMOVE ME WHEN WE SWITCH TO SKIN VERSION 2.0

#include <xvoice.h>
#include "Profile.h"
#include "settings/VideoSettings.h"
#include "../xbmc/StringUtils.h"
#include "GUISettings.h"
#include "fileitem.h"
#include "GUIViewState.h" // for the VIEW_METHOD enum type

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
  CShare() { m_iBufferSize=0; m_iDepthSize=0; m_iDriveType=SHARE_TYPE_UNKNOWN; m_iLockMode=LOCK_MODE_EVERYONE; m_iBadPwdCount=0; };
  virtual ~CShare() {};
  CStdString strName; ///< Name of the share, can be choosen freely.
  CStdString strPath; ///< Path of the share, eg. iso9660:// or F:
  CStdString strEntryPoint; ///< entry point of shares, used with archives
  int m_iBufferSize;   ///< Cachesize of the share
  int m_iDepthSize;    ///< Depth for My Programs

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
  int m_iBadPwdCount; ///< Number of wrong passwords user has entered since share was last unlocked

  CStdString m_strThumbnailImage; ///< Path to a thumbnail image for the share, or blank for default

  vector<CStdString> vecPaths;
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

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
class CButtonScrollerSettings
{
public:
  CButtonScrollerSettings() {};
  ~CButtonScrollerSettings()
  {
    Clear();
  }
  void Clear()
  {
    for (unsigned int i = 0; i < m_vecButtons.size(); i++)
    {
      CButton *pButton = m_vecButtons[i];
      if (pButton)
        delete pButton;
    }
    m_vecButtons.clear();
  };
  class CButton
  {
  public:
    CButton(const wstring strLabel, const CStdString &strExecute, const int iIcon)
    {
      m_dwLabel = -1;
      m_strLabel = strLabel;
      m_strExecute = strExecute;
      m_iIcon = iIcon;
    };
    CButton(DWORD dwLabel, const CStdString &strExecute, const int iIcon)
    {
      m_dwLabel = dwLabel;
      m_strExecute = strExecute;
      m_iIcon = iIcon;
    };
    DWORD m_dwLabel;
    wstring m_strLabel;
    CStdString m_strExecute;
    int m_iIcon;
  };
  std::vector<CButton *> m_vecButtons;
  int m_iDefaultButton;
};
#endif

class CSettings
{
public:
  CSettings(void);
  virtual ~CSettings(void);

  bool Load(bool& bXboxMediacenter, bool& bSettings);
  bool QuickXMLLoad(CStdString strElement);
  void Save() const;

  void Clear();

  bool LoadProfile(int index);
  bool SaveSettingsToProfile(int index);
  void DeleteProfile(int index);

  bool UpdateBookmark(const CStdString &strType, const CStdString &strOldName, const CStdString &strUpdateChild, const CStdString &strUpdateValue);
  bool DeleteBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath);
  bool AddBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath);
  bool AddBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath, const int iDepth);
  bool SetBookmarkLocks(const CStdString& strType, bool bEngageLocks);

  bool LoadFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders);
  bool SaveFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders);

  bool UpDateXbmcXML(const CStdString &strFirstChild, const CStdString &strChild, const CStdString &strChildValue);
  bool UpDateXbmcXML(const CStdString &strFirstChild, const CStdString &strFirstChildValue);

  bool GetSkinSetting(const char *setting) const;
  void ToggleSkinSetting(const char *setting);

  struct AdvancedSettings
  {
public:

    int m_audioHeadRoom;

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
  };
  struct stSettings
  {
public:
    unsigned long dwFileVersion;
    char szHomeDir[1024];


    char szDashboard[1024];
    int m_iStartupWindow;

    char szThumbnailsDirectory[1024];
    char m_szMyPicturesExtensions[512];
    char m_szMyMusicExtensions[1024];
    char m_szMyVideoExtensions[512];
    char m_szShortcutDirectory[256];
    char m_szAlbumDirectory[256];
    char m_szScreenshotsDirectory[256];
    char m_szCacheDirectory[256];
    char m_szPlaylistsDirectory[256];
    char m_szTrainerDirectory[256];

    /*
    bool m_bMyFilesSourceViewMethod;
    bool m_bMyFilesSourceRootViewMethod;
    bool m_bMyFilesDestViewMethod;
    bool m_bMyFilesDestRootViewMethod;
    int m_iMyFilesSourceSortMethod;
    int m_iMyFilesSourceRootSortMethod;
    bool m_bMyFilesSourceSortOrder;
    bool m_bMyFilesSourceRootSortOrder;
    int m_iMyFilesDestSortMethod;
    int m_iMyFilesDestRootSortMethod;
    bool m_bMyFilesDestSortOrder;
    bool m_bMyFilesDestRootSortOrder;
    */

    int m_iRepeatDelayIR;
    int m_iMoveDelayController;
    int m_iRepeatDelayController;
    float m_fAnalogDeadzoneController;

    int m_iLogLevel;
    char m_szlogpath[128];
    bool m_bDisplayRemoteCodes;
    bool m_bShowFreeMem;
    bool m_bUnhandledExceptionToFatalError;

    char m_szDefaultPrograms[128];
    char m_szDefaultMusic[128];
    char m_szDefaultPictures[128];
    char m_szDefaultFiles[128];
    char m_szDefaultVideos[128];

    char m_szCDDBIpAdres[128];
    char m_szIMDBurl[128];

    char m_strRipPath[MAX_PATH + 1];
    char m_szMusicRecordingDirectory[128];
   
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

    // for scanning
    bool m_bMyMusicIsScanning;
    bool m_bMyMusicOldUseTags;
    bool m_bMyMusicOldFindThumbs;

    CVideoSettings m_defaultVideoSettings;
    CVideoSettings m_currentVideoSettings;

    float m_fZoomAmount;      // current zoom amount
    float m_fPixelRatio;      // current pixel ratio

    int m_iMyVideoTitleShowMode;
    int m_iMyVideoGenreShowMode;
    int m_iMyVideoActorShowMode;
    int m_iMyVideoYearShowMode;

    VIEW_METHOD m_MyVideoViewMethod;
    VIEW_METHOD m_MyVideoRootViewMethod;
    SORT_METHOD m_MyVideoSortMethod;
    SORT_METHOD m_MyVideoRootSortMethod;
    SORT_ORDER m_MyVideoSortOrder;
    SORT_ORDER m_MyVideoRootSortOrder;

    VIEW_METHOD m_MyVideoGenreViewMethod;
    VIEW_METHOD m_MyVideoGenreRootViewMethod;
    SORT_METHOD m_MyVideoGenreSortMethod;
    SORT_METHOD m_MyVideoGenreRootSortMethod;
    SORT_ORDER m_MyVideoGenreSortOrder;
    SORT_ORDER m_MyVideoGenreRootSortOrder;

    VIEW_METHOD m_MyVideoActorViewMethod;
    VIEW_METHOD m_MyVideoActorRootViewMethod;
    SORT_METHOD m_MyVideoActorSortMethod;
    SORT_METHOD m_MyVideoActorRootSortMethod;
    SORT_ORDER m_MyVideoActorSortOrder;
    SORT_ORDER m_MyVideoActorRootSortOrder;

    VIEW_METHOD m_MyVideoYearViewMethod;
    VIEW_METHOD m_MyVideoYearRootViewMethod;
    SORT_METHOD m_MyVideoYearSortMethod;
    SORT_METHOD m_MyVideoYearRootSortMethod;
    SORT_ORDER m_MyVideoYearSortOrder;
    SORT_ORDER m_MyVideoYearRootSortOrder;

    VIEW_METHOD m_MyVideoTitleViewMethod;
    SORT_METHOD m_MyVideoTitleSortMethod;
    SORT_ORDER m_MyVideoTitleSortOrder;

    VIEW_METHOD m_MyVideoPlaylistViewMethod;

    bool m_bMyVideoPlaylistRepeat;
    bool m_bMyVideoPlaylistShuffle;

    int m_iVideoStartWindow;

    int m_iMyVideoStack;
    bool m_bMyVideoCleanTitles;
    char m_szMyVideoCleanTokens[256];
    char m_szMyVideoCleanSeparators[32];

    bool m_bAutoDetectFG;
    bool m_bUseFDrive;
    bool m_bUseGDrive;
    bool m_bUsePCDVDROM;
    bool m_bDetectAsIso;

    char m_szAlternateSubtitleDirectory[128];

    char m_szExternalDVDPlayer[128];
    char m_szExternalCDDAPlayer[128];

    char szOnlineArenaPassword[32]; // private arena password
    char szOnlineArenaDescription[64]; // private arena description


    int m_iMasterLockMaxRetry;        // maximum # of password retries a user gets for all locked shares
    int m_iMasterLockEnableShutdown;  // allows XBMC Master Lock to shut off XBOX if true
    int m_iMasterLockProtectShares;   // prompts for mastercode when editing shares with context menu if true
    int m_iMasterLockMode;            // determines the type of master lock UI to present to the user, if any
    CStdString m_masterLockCode;      // password to check for on startup
    int m_iMasterLockStartupLock;     // prompts user for szMasterLockCode on startup if true
    int m_iMasterLockFilemanager;     // prompts user for MasterLockCode on Click Filemanager! 
    int m_iMasterLockSettings;        // prompts user for MasterLockCode on Click Settings!
    int m_iMasterLockHomeMedia;       // prompts user for MasterLockCode on Click Media [Video/Picture/Musik/Programs]!
    
    int m_iSambaDebugLevel;
    char m_strSambaWorkgroup[128];
    char m_strSambaIPAdress[32];
    char m_strSambaShareName[32];
    char m_strSambaDefaultUserName[128];
    char m_strSambaDefaultPassword[128];
    char m_strSambaWinsServer[32];

    int m_nVolumeLevel;       // measured in 100th's of a dB.  0dB is max, -60.00dB is min
    int m_iPreMuteVolumeLevel;    // save the m_nVolumeLevel for proper restore
    bool m_bMute;
    int m_iSystemTimeTotalUp;    // Uptime in minutes!
    char szScreenSaverSlideShowPath[1024];

    XVOICE_MASK m_karaokeVoiceMask[4];
  };

  CStdStringArray m_MyVideoStackRegExps;
  CStdStringArray m_vecPathSubstitutions;

  std::map<int,std::pair<std::vector<int>,std::vector<wstring> > > m_mapRssUrls;
  std::map<CStdString, bool> m_skinSettings;

  // cache copies of these parsed values, to avoid re-parsing over and over
  CStdString m_szMyVideoStackSeparatorsString;
  CStdStringArray m_szMyVideoStackTokensArray;
  CStdString m_szMyVideoCleanSeparatorsString;
  CStdStringArray m_szMyVideoCleanTokensArray;

  VECSHARES m_vecMyProgramsBookmarks;
  VECSHARES m_vecMyPictureShares;
  VECSHARES m_vecMyFilesShares;
  VECSHARES m_vecMyMusicShares;
  VECSHARES m_vecMyVideoShares;
  VECSHARES m_vecSambeShres;
  VECFILETYPEICONS m_vecIcons;
  VECPROFILES m_vecProfiles;
  int m_iLastLoadedProfileIndex;
  RESOLUTION_INFO m_ResInfo[10];
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  CButtonScrollerSettings m_buttonSettings;
#endif
protected:
  void GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName, bool& bValue);
  void GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  void GetFloat(const TiXmlElement* pRootElement, const CStdString& strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, CStdString& strValue, const CStdString& strDefaultValue);
  void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char *szValue, const CStdString& strDefaultValue);
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

  bool LoadSettings(const CStdString& strSettingsFile, const bool loadprofiles);
  bool SaveSettings(const CStdString& strSettingsFile, const bool saveprofiles) const;

  bool LoadProfiles(const TiXmlElement* pRootElement, const CStdString& strSettingsFile);
  bool SaveProfiles(TiXmlNode* pRootElement) const;

  bool LoadXml();
  void CloseXml();
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  void LoadHomeButtons(TiXmlElement* pRootElement);
#endif
  // skin activated settings
  void LoadSkinSettings(const TiXmlElement* pElement);
  void SaveSkinSettings(TiXmlNode *pElement) const;

  TiXmlDocument xbmcXml;  // for editing the xml file from within XBMC
  bool xbmcXmlLoaded;
};

extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;
extern struct CSettings::AdvancedSettings g_advancedSettings;