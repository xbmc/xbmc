#pragma once

#define CONFIG_VERSION 0x000F

#include "Profile.h"
#include "settings/VideoSettings.h"
#include "../xbmc/StringUtils.h"
#include "GUISettings.h"

#define SHARE_TYPE_UNKNOWN      0
#define SHARE_TYPE_LOCAL        1
#define SHARE_TYPE_DVD          2
#define SHARE_TYPE_VIRTUAL_DVD  3
#define SHARE_TYPE_REMOTE       4

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

class CFolderView
{
public:
  CFolderView(CStdString &strPath, int iView, int iSort, bool bSortAscending)
  {
    m_strPath = strPath;
    m_iView = iView;
    m_iSort = iSort;
    m_bSortAscending = bSortAscending;
  };
  ~CFolderView() {};

  CStdString m_strPath;
  int m_iView;
  int m_iSort;
  bool m_bSortAscending;
};

typedef vector<CFolderView*> VECFOLDERVIEWS;

/*!
\ingroup windows
\brief Represents a share.
\sa VECSHARES, IVECSHARES
*/
class CShare
{
public:
  CShare(){};
  virtual ~CShare(){};
  CStdString strName; ///< Name of the share, can be choosen freely.
  CStdString strPath; ///< Path of the share, eg. iso9660:// or F:
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
  CStdStringW m_strLockCode;  ///< Input code for Lock UI to verify, can be chosen freely.
  int m_iBadPwdCount; ///< Number of wrong passwords user has entered since share was last unlocked
};
/*!
\ingroup windows
\brief A vector to hold CShare objects.
\sa CShare, IVECSHARES
*/
typedef vector<CShare> VECSHARES;

/*!
\ingroup windows
\brief Iterator of VECSHARES.
\sa CShare, VECSHARES
*/
typedef vector<CShare>::iterator IVECSHARES;

typedef vector<CProfile> VECPROFILES;
typedef vector<CProfile>::iterator IVECPROFILES;

class CFileTypeIcon
{
public:
  CFileTypeIcon(){};
  virtual ~CFileTypeIcon(){};
  CStdString m_strName;
  CStdString m_strIcon;
};
typedef vector<CFileTypeIcon> VECFILETYPEICONS;
typedef vector<CFileTypeIcon>::iterator IVECFILETYPEICONS;


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
  vector<CButton *> m_vecButtons;
  int m_iDefaultButton;
};

class CSettings
{
public:
  CSettings(void);
  virtual ~CSettings(void);

  bool Load(bool& bXboxMediacenter, bool& bSettings);
  void Save() const;

  bool LoadProfile(int index);
  bool SaveSettingsToProfile(int index);
  void DeleteProfile(int index);

  bool UpdateBookmark(const CStdString &strType, const CStdString &strOldName, const CStdString &strUpdateChild, const CStdString &strUpdateValue);
  bool DeleteBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath);
  bool AddBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath);
  bool AddBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath, const int iDepth);
  bool SetBookmarkLocks(const CStdString& strType, bool bEngageLocks);
  bool SaveHomeButtons();

  bool LoadFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders);
  bool SaveFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders);

  struct stSettings
  {
public:
    unsigned long dwFileVersion;
    char szHomeDir[1024];

    int m_iMyProgramsSortMethod;
    bool m_bMyProgramsSortAscending;
    int m_iMyProgramsViewAsIcons;

    char szDashboard[1024];
    int m_iStartupWindow;

    int m_iMyPicturesSortMethod;
    bool m_bMyPicturesSortAscending;
    int m_iMyPicturesViewAsIcons;
    int m_iMyPicturesRootSortMethod;
    bool m_bMyPicturesRootSortAscending;
    int m_iMyPicturesRootViewAsIcons;

    char szThumbnailsDirectory[1024];
    char m_szMyPicturesExtensions[256];
    char m_szMyMusicExtensions[256];
    char m_szMyVideoExtensions[256];
    char m_szShortcutDirectory[256];
    char m_szAlbumDirectory[256];
    char m_szScreenshotsDirectory[256];

    bool m_bMyFilesSourceViewAsIcons;
    bool m_bMyFilesSourceRootViewAsIcons;
    bool m_bMyFilesDestViewAsIcons;
    bool m_bMyFilesDestRootViewAsIcons;
    int m_iMyFilesSourceSortMethod;
    int m_iMyFilesSourceRootSortMethod;
    bool m_bMyFilesSourceSortAscending;
    bool m_bMyFilesSourceRootSortAscending;
    int m_iMyFilesDestSortMethod;
    int m_iMyFilesDestRootSortMethod;
    bool m_bMyFilesDestSortAscending;
    bool m_bMyFilesDestRootSortAscending;

    int m_iMyVideoViewAsIcons;
    int m_iMyVideoRootViewAsIcons;
    int m_iMyVideoSortMethod;
    int m_iMyVideoRootSortMethod;
    bool m_bMyVideoSortAscending;
    bool m_bMyVideoRootSortAscending;

    bool m_bScriptsViewAsIcons;
    bool m_bScriptsRootViewAsIcons;
    int m_iScriptsSortMethod;
    bool m_bScriptsSortAscending;

    int m_iMoveDelayIR;
    int m_iRepeatDelayIR;
    int m_iMoveDelayController;
    int m_iRepeatDelayController;
    float m_fAnalogDeadzoneController;

    int m_iLogLevel;
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

    int m_iMyMusicSongsRootViewAsIcons;
    int m_iMyMusicSongsViewAsIcons;
    bool m_bMyMusicSongsRootSortAscending;
    bool m_bMyMusicSongsSortAscending;
    int m_iMyMusicSongsSortMethod;
    int m_iMyMusicSongsRootSortMethod;
    int m_iMyMusicAlbumRootViewAsIcons;
    int m_iMyMusicAlbumViewAsIcons;
    bool m_bMyMusicAlbumRootSortAscending;
    bool m_bMyMusicAlbumSortAscending;
    int m_iMyMusicAlbumSortMethod;
    int m_iMyMusicAlbumRootSortMethod;
    bool m_bMyMusicAlbumShowRecent;
    int m_iMyMusicArtistsRootViewAsIcons;
    int m_iMyMusicArtistsAlbumsViewAsIcons;
    int m_iMyMusicArtistsSongsViewAsIcons;
    bool m_bMyMusicArtistsRootSortAscending;
    bool m_bMyMusicArtistsSortAscending;
    bool m_bMyMusicArtistsAlbumsSortAscending;
    bool m_bMyMusicArtistsAlbumSongsSortAscending;
    bool m_bMyMusicArtistsAllSongsSortAscending;
    int m_iMyMusicArtistsRootSortMethod;
    int m_iMyMusicArtistsAllSongsSortMethod;
    int m_iMyMusicArtistsAlbumsSortMethod;
    int m_iMyMusicArtistsAlbumsSongsSortMethod;
    int m_iMyMusicGenresRootViewAsIcons;
    int m_iMyMusicGenresViewAsIcons;
    bool m_bMyMusicGenresRootSortAscending;
    bool m_bMyMusicGenresSortAscending;
    int m_iMyMusicGenresSortMethod;
    int m_iMyMusicGenresRootSortMethod;
    int m_iMyMusicPlaylistViewAsIcons;
    bool m_bMyMusicPlaylistRepeat;
    bool m_bMyMusicPlaylistShuffle;
    int m_iMyMusicTop100ViewAsIcons;
    int m_iMyMusicStartWindow;

    // new settings for the Music Nav Window
    int m_iMyMusicNavRootViewAsIcons;
    int m_iMyMusicNavGenresViewAsIcons;
    int m_iMyMusicNavArtistsViewAsIcons;
    int m_iMyMusicNavAlbumsViewAsIcons;
    int m_iMyMusicNavSongsViewAsIcons;

    int m_iMyMusicNavRootSortMethod;
    int m_iMyMusicNavAlbumsSortMethod;
    int m_iMyMusicNavSongsSortMethod;

    bool m_bMyMusicNavGenresSortAscending;
    bool m_bMyMusicNavArtistsSortAscending;
    bool m_bMyMusicNavAlbumsSortAscending;
    bool m_bMyMusicNavSongsSortAscending;

    // for scanning
    bool m_bMyMusicIsScanning;
    bool m_bMyMusicOldUseTags;
    bool m_bMyMusicOldFindThumbs;

    int m_iSmallStepBackSeconds;
    int m_iSmallStepBackTries;
    int m_iSmallStepBackDelay;
    float m_fSubsDelayRange;
    float m_fAudioDelayRange;

    CVideoSettings m_defaultVideoSettings;
    CVideoSettings m_currentVideoSettings;

    float m_fZoomAmount;      // current zoom amount
    float m_fPixelRatio;      // current pixel ratio

    int m_iMyVideoGenreViewAsIcons;
    int m_iMyVideoGenreRootViewAsIcons;
    int m_iMyVideoGenreSortMethod;
    int m_iMyVideoGenreRootSortMethod;
    bool m_bMyVideoGenreSortAscending;
    bool m_bMyVideoGenreRootSortAscending;

    int m_iMyVideoActorViewAsIcons;
    int m_iMyVideoActorRootViewAsIcons;
    int m_iMyVideoActorSortMethod;
    int m_iMyVideoActorRootSortMethod;
    bool m_bMyVideoActorSortAscending;
    bool m_bMyVideoActorRootSortAscending;

    int m_iMyVideoYearViewAsIcons;
    int m_iMyVideoYearRootViewAsIcons;
    int m_iMyVideoYearSortMethod;
    int m_iMyVideoYearRootSortMethod;
    bool m_bMyVideoYearSortAscending;
    bool m_bMyVideoYearRootSortAscending;

    int m_iMyVideoTitleViewAsIcons;
    int m_iMyVideoTitleRootViewAsIcons;
    int m_iMyVideoTitleSortMethod;
    bool m_bMyVideoTitleSortAscending;

    int m_iMyVideoPlaylistViewAsIcons;
    bool m_bMyVideoPlaylistRepeat;
    bool m_bMyVideoPlaylistShuffle;

    int m_iVideoStartWindow;

    int m_iMyVideoVideoStack;
    bool m_bMyVideoActorStack;
    bool m_bMyVideoGenreStack;
    bool m_bMyVideoYearStack;

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


    int m_iMasterLockMaxRetry; // maximum # of password retries a user gets for all locked shares
    int m_iMasterLockStartupLock; // prompts user for szMasterLockCode on startup if true
    int m_iMasterLockMode; // determines the type of master lock UI to present to the user, if any
    char szMasterLockCode[128]; // password to check for on startup
    int m_iMasterLockEnableShutdown; // allows XBMC Master Lock to shut off XBOX if true
    int m_iMasterLockProtectShares; // prompts for mastercode when editing shares with context menu if true

    int m_iSambaDebugLevel;
    char m_strSambaWorkgroup[128];
    char m_strSambaWinsServer[32];
    char m_strSambaDefaultUserName[128];
    char m_strSambaDefaultPassword[128];

    int m_nVolumeLevel;       // measured in 100th's of a dB.  0dB is max, -60.00dB is min
    int m_iPreMuteVolumeLevel;    // save the m_nVolumeLevel for proper restore
    bool m_bMute;

  };

  CStdStringArray m_MyVideoStackRegExps;

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
  VECFILETYPEICONS m_vecIcons;
  VECPROFILES m_vecProfiles;
  int m_iLastLoadedProfileIndex;
  RESOLUTION_INFO m_ResInfo[10];
  CButtonScrollerSettings m_buttonSettings;
protected:
  void GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName, bool& bValue);
  void GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  void GetFloat(const TiXmlElement* pRootElement, const CStdString& strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue);
  void GetShares(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSHARES& items, CStdString& strDefault);
  void GetHex(const TiXmlNode* pRootElement, const CStdString& strTagName, DWORD& dwHexValue, DWORD dwDefaultValue);
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
  void LoadHomeButtons(TiXmlElement* pRootElement);

  TiXmlDocument xbmcXml;  // for editing the xml file from within XBMC
  bool xbmcXmlLoaded;
};

extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;
