#pragma once

#include "tinyxml/tinyxml.h"
#define CONFIG_VERSION 0x000F

#include <xtl.h>
#include "stdstring.h"
#include "StringUtils.h"
#include "GraphicContext.h"
#include "Profile.h"
#include <vector>
using namespace std;

#define SHARE_TYPE_UNKNOWN			0
#define SHARE_TYPE_LOCAL				1
#define SHARE_TYPE_DVD					2
#define SHARE_TYPE_VIRTUAL_DVD	3
#define SHARE_TYPE_REMOTE				4


#define LCD_MODE_NORMAL   0
#define LCD_MODE_NOTV     1

#define LCD_MODE_TYPE_LCD 0
#define LCD_MODE_TYPE_VFD 1

#define MODCHIP_SMARTXX   0
#define MODCHIP_XENIUM    1
#define MODCHIP_XECUTER3  2

#define CACHE_AUDIO 0
#define CACHE_VIDEO 1
#define CACHE_VOB   2

#define CDDARIP_ENCODER_LAME     0
#define CDDARIP_ENCODER_VORBIS   1
#define CDDARIP_ENCODER_WAV      2

#define CDDARIP_QUALITY_CBR      0
#define CDDARIP_QUALITY_MEDIUM   1
#define CDDARIP_QUALITY_STANDARD 2
#define CDDARIP_QUALITY_EXTREME  3

#define VOLUME_MINIMUM -6000	// -60dB
#define VOLUME_MAXIMUM 0		// 0dB

#define VIEW_MODE_NORMAL				0
#define VIEW_MODE_ZOOM					1
#define VIEW_MODE_STRETCH_4x3		2
#define VIEW_MODE_STRETCH_14x9	3
#define VIEW_MODE_STRETCH_16x9	4
#define VIEW_MODE_ORIGINAL			5
#define VIEW_MODE_CUSTOM				6

#define STACK_NONE					0
#define STACK_SIMPLE				1
#define STACK_FUZZY					2

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
	CStdString strName;	///< Name of the share, can be choosen freely.
	CStdString strPath;	///< Path of the share, eg. iso9660:// or F:
	int        m_iBufferSize;		///< Cachesize of the share
	int		   m_iDepthSize;		///< Depth for My Programs

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
	int        m_iDriveType;

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

class CSettings
{
public:
	CSettings(void);
	virtual ~CSettings(void);

	bool	Load(bool& bXboxMediacenter, bool& bSettings);
	void	Save() const;

  bool LoadProfile(int index);
  bool SaveSettingsToProfile(int index);
  void DeleteProfile(int index);

	bool UpdateBookmark(const CStdString &strType, const CStdString &strOldName, const CStdString &strName, const CStdString &strPath);
	bool DeleteBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath);
	bool AddBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath);

	struct stSettings
	{
	public:
		unsigned long dwFileVersion;
		char	    szDefaultSkin[1024];
		char			szHomeDir[1024];

		int				m_iMyProgramsSortMethod;
		bool			m_bMyProgramsSortAscending;
		int       m_iMyProgramsViewAsIcons;
		bool			m_bMyProgramsFlatten;
		bool			m_bMyProgramsDefaultXBE;
		bool			m_bMyProgramsDirectoryName;
		bool			m_bMyProgramsNoShortcuts;

		char      szThumbnailsDirectory[1024];
		char      szDashboard[1024];
		int       m_iStartupWindow;

		char	  m_strIPAssignment[10];
		char      m_strLocalIPAdres[32];
		char      m_strLocalNetmask[32];
		char      m_strGateway[32];
		char      m_strNameServer[128];
		char      m_strTimeServer[128];

		int       m_iMyPicturesViewAsIcons;
		int       m_iMyPicturesRootViewAsIcons;
		int				m_iMyPicturesSortMethod;
		int				m_iMyPicturesRootSortMethod;
		bool			m_bMyPicturesSortAscending;
		bool			m_bMyPicturesRootSortAscending;

		char      m_szMyPicturesExtensions[256];
		char      m_szMyMusicExtensions[256];
		char      m_szMyVideoExtensions[256];
		char      m_szShortcutDirectory[256];
		char      m_szAlbumDirectory[256];
		char	  m_szScreenshotsDirectory[256];

		bool			m_bMyFilesSourceViewAsIcons;
		bool			m_bMyFilesSourceRootViewAsIcons;
		bool			m_bMyFilesDestViewAsIcons;
		bool			m_bMyFilesDestRootViewAsIcons;
		int				m_iMyFilesSourceSortMethod;
		int				m_iMyFilesSourceRootSortMethod;
		bool			m_bMyFilesSourceSortAscending;
		bool			m_bMyFilesSourceRootSortAscending;
		int				m_iMyFilesDestSortMethod;
		int				m_iMyFilesDestRootSortMethod;
		bool			m_bMyFilesDestSortAscending;
		bool			m_bMyFilesDestRootSortAscending;

		int				m_iMyMusicSongsRootViewAsIcons;
		int				m_iMyMusicSongsViewAsIcons;
		bool			m_bMyMusicSongsRootSortAscending;
		bool			m_bMyMusicSongsSortAscending;
		int				m_iMyMusicSongsSortMethod;
		int				m_iMyMusicSongsRootSortMethod;
		bool			m_bMyMusicSongsUsePlaylist;
		bool			m_bMyMusicSongsAutoSwitchThumbsList;
		bool			m_bMyMusicSongsAutoSwitchBigThumbs;
		int				m_iMyMusicAlbumRootViewAsIcons;
		int				m_iMyMusicAlbumViewAsIcons;
		bool			m_bMyMusicAlbumRootSortAscending;
		bool			m_bMyMusicAlbumSortAscending;
		int				m_iMyMusicAlbumSortMethod;
		int				m_iMyMusicAlbumRootSortMethod;
		bool			m_bMyMusicAlbumShowRecent;
		int				m_iMyMusicArtistsRootViewAsIcons;
		int				m_iMyMusicArtistsAlbumsViewAsIcons;
		int				m_iMyMusicArtistsSongsViewAsIcons;
		bool			m_bMyMusicArtistsRootSortAscending;
		bool			m_bMyMusicArtistsSortAscending;
		bool			m_bMyMusicArtistsAlbumsSortAscending;
		bool			m_bMyMusicArtistsAlbumSongsSortAscending;
		bool			m_bMyMusicArtistsAllSongsSortAscending;
		int				m_iMyMusicArtistsRootSortMethod;
		int				m_iMyMusicArtistsAllSongsSortMethod;
		int				m_iMyMusicArtistsAlbumsSortMethod;
		int				m_iMyMusicArtistsAlbumsSongsSortMethod;
		int				m_iMyMusicGenresRootViewAsIcons;
		int				m_iMyMusicGenresViewAsIcons;
		bool			m_bMyMusicGenresRootSortAscending;
		bool			m_bMyMusicGenresSortAscending;
		int				m_iMyMusicGenresSortMethod;
		int				m_iMyMusicGenresRootSortMethod;
		int				m_iMyMusicPlaylistViewAsIcons;
		bool			m_bMyMusicPlaylistRepeat;
		int				m_iMyMusicTop100ViewAsIcons;
		int				m_iMyMusicStartWindow;
		bool			m_bMyMusicRepeat;
		bool			m_bMyMusicSongInfoInVis;
    bool      m_bMyMusicSongThumbInVis;
		bool			m_bMyMusicHideTrackNumber;
		int 			m_iMyVideoViewAsIcons;
		int 			m_iMyVideoRootViewAsIcons;
		int				m_iMyVideoSortMethod;
		int				m_iMyVideoRootSortMethod;
		bool			m_bMyVideoSortAscending;
		bool			m_bMyVideoRootSortAscending;

		bool			m_bScriptsViewAsIcons;
		bool			m_bScriptsRootViewAsIcons;
		int				m_iScriptsSortMethod;
		bool			m_bScriptsSortAscending;

		int				m_iMoveDelayIR;
		int				m_iRepeatDelayIR;

		int				m_iMoveDelayController;
		int				m_iRepeatDelayController;
		float			m_fAnalogDeadzoneController;

		bool			m_bTimeServerEnabled;
		bool			m_bFTPServerEnabled;
		bool			m_bHTTPServerEnabled;
		int       m_iSlideShowTransistionTime;
		int       m_iSlideShowStayTime;
		float			m_fSlideShowMoveAmount;
		float			m_fSlideShowZoomAmount;
		float			m_fSlideShowBlackBarCompensation;
		bool			m_bSlideShowShuffle;
		RESOLUTION		m_GUIResolution;
		bool		  m_bHTTPProxyEnabled;
		int			  m_iHTTPProxyPort;
		char		  m_szHTTPProxy[128];
		int			  m_iWebServerPort;
		int			  m_iLogLevel;

    bool  m_bAutoTemperature;
    int   m_iTargetTemperature;
    bool  m_bFanSpeedControl;
    int   m_iFanSpeed;

		int		m_iShutdownTime;
		int		m_iScreenSaverTime;		// CB: SCREENSAVER PATCH
		int		m_iScreenSaverMode;		// CB: SCREENSAVER
		int		m_iScreenSaverFadeLevel;

		char			m_szDefaultPrograms[128];
		char			m_szDefaultMusic[128];
		char			m_szDefaultPictures[128];
		char			m_szDefaultFiles[128];
		char			m_szDefaultVideos[128];
		char			m_szCDDBIpAdres[128];
		bool			m_bUseCDDB;
		int				m_iUIOffsetX;
		int				m_iUIOffsetY;
    int       m_iFlickerFilterVideo; // 0..5
    int       m_iFlickerFilterUI; // 0..5
		bool			m_bSoftenVideo;
		bool			m_bSoftenUI;
		int				m_iViewMode;			// current view mode
		float			m_fZoomAmount;			// current zoom amount
		float			m_fPixelRatio;			// current pixel ratio
		float			m_fCustomZoomAmount;	// custom setting zoom amount
		float			m_fCustomPixelRatio;	// custom setting pixel ratio
		bool			m_bAutoWidescreenSwitching;
		bool			m_bUpsampleVideo;
		bool			m_bAllowPAL60;

		bool			m_bAutoShufflePlaylist;
		int			  m_iHDSpinDownTime;
    bool      m_bHDRemoteplaySpinDownAudio;
    bool      m_bHDRemoteplaySpinDownVideo;
    int       m_iHDRemoteplaySpinDownTime; //seconds
    int       m_iHDRemoteplaySpinDownMinDuration; //minutes
		DWORD     m_minFilter ;
		DWORD     m_maxFilter ;
		bool			m_bAutorunDVD;
		bool			m_bAutorunVCD;
		bool			m_bAutorunCdda;
		bool			m_bAutorunXbox;
		bool			m_bAutorunMusic;
		bool			m_bAutorunVideo;
		bool			m_bAutorunPictures;
		char      szDefaultLanguage[256];
		char			m_szSkinFontSet[256];
		char      szDefaultVisualisation[256];
		bool		  m_bUseFDrive;
		bool		  m_bUseGDrive;
		bool			m_bUsePCDVDROM;
		bool			m_bDetectAsIso;
		bool			m_bAudioOnAllSpeakers;
		int				m_iChannels;
		bool			m_bUseID3;
		char			m_szMusicRecordingDirectory[128];
		char      m_szAlternateSubtitleDirectory[128];
		bool      m_bPostProcessing;
		bool      m_bDeInterlace;
		char      m_szSubtitleFont[40];
		char	  m_szStringCharset[40];
		int       m_iSubtitleHeight;
		int		  m_iSubtitleTTFStyle;
		DWORD	  m_iSubtitleTTFColor;
		char	  m_szSubtitleCharset[40];
		int       m_iEnlargeSubtitlePercent;
		float     m_fVolumeAmplification;
		float     m_fVolumeHeadroom;
		bool      m_bNonInterleaved;
		bool      m_bPPAuto;
		bool      m_bPPVertical;
		bool      m_bPPHorizontal;
		bool      m_bPPAutoLevels;
		int       m_iPPHorizontal;
		int       m_iPPVertical;
		bool      m_bPPdering;
		bool      m_bFrameRateConversions;
		bool      m_bUseDigitalOutput;
		int       m_iAudioStream;
		int       m_iSubtitleStream;

		int			m_iMyVideoGenreViewAsIcons;
		int			m_iMyVideoGenreRootViewAsIcons;
		int				m_iMyVideoGenreSortMethod;
		int				m_iMyVideoGenreRootSortMethod;
		bool			m_bMyVideoGenreSortAscending;
		bool			m_bMyVideoGenreRootSortAscending;

		int			m_iMyVideoActorViewAsIcons;
		int			m_iMyVideoActorRootViewAsIcons;
		int				m_iMyVideoActorSortMethod;
		int				m_iMyVideoActorRootSortMethod;
		bool			m_bMyVideoActorSortAscending;
		bool			m_bMyVideoActorRootSortAscending;

		int			m_iMyVideoYearViewAsIcons;
		int			m_iMyVideoYearRootViewAsIcons;
		int				m_iMyVideoYearSortMethod;
		int				m_iMyVideoYearRootSortMethod;
		bool			m_bMyVideoYearSortAscending;
		bool			m_bMyVideoYearRootSortAscending;

		int			m_iMyVideoTitleViewAsIcons;
		int			m_iMyVideoTitleRootViewAsIcons;
		int				m_iMyVideoTitleSortMethod;
		bool			m_bMyVideoTitleSortAscending;

		int				m_iMyVideoVideoStack;
		bool			m_bMyVideoActorStack;
		bool			m_bMyVideoGenreStack;
		bool			m_bMyVideoYearStack;
		char			m_szMyVideoStackTokens[128];
		char			m_szMyVideoStackSeparators[32];

		bool			m_bMyVideoCleanTitles;
		char			m_szMyVideoCleanTokens[256];
		char			m_szMyVideoCleanSeparators[32];

		int       m_iVideoStartWindow;
		char			m_szWeatherArea[3][10];	//WEATHER
		char			m_szWeatherFTemp[2];	//WEATHER
		char			m_szWeatherFSpeed[2];	//WEATHER
		int				m_iWeatherRefresh;		//WEATHER
		char			m_szExternalDVDPlayer[128];
		char			m_szExternalCDDAPlayer[128];
		bool      m_bNoCache;
		int       m_iSmallStepBackSeconds;
		int       m_iSmallStepBackTries;
		int       m_iSmallStepBackDelay;
		int       m_iCacheSizeHD[3];
		int       m_iCacheSizeUDF[3];
		int       m_iCacheSizeISO[3];
		int       m_iCacheSizeLAN[3];
		int       m_iCacheSizeInternet[3];
		int       m_iMyVideoPlaylistViewAsIcons;
		bool			m_bMyVideoPlaylistRepeat;
		bool      m_bLCDUsed;
		int       m_iLCDColumns;
		int       m_iLCDRows;
		int       m_iLCDAdress[4];
		int       m_iLCDMode;
		int       m_iLCDBackLight;
        int       m_iLCDContrast;
		int       m_iLCDType;
		int       m_iLCDBrightness;
		bool			m_bDisplayRemoteCodes;	// Remote code debug info
		bool			m_bResampleMusicAudio;	// resample using SSRC
		bool			m_bResampleVideoAudio;	// separate from music, as it causes a CPU hit
		int       m_iLCDModChip;
		int       m_iOSDTimeout;		// OSD timeout in seconds
		char      szOnlineUsername[32]; // KAITAG (username)
		char      szOnlinePassword[32]; // corresponding password
		bool      m_mplayerDebug;
		int       m_iSambaDebugLevel;
		char      m_strSambaWorkgroup[128];
		char      m_strSambaWinsServer[32];
		char      m_strSambaDefaultUserName[128];
		char      m_strSambaDefaultPassword[128];

		bool      m_bHideExtensions;
		bool			m_bHideParentDirItems;

		bool      m_bRipWithTrackNumber;
		int       m_iRipEncoder;
		int       m_iRipQuality;
		int       m_iRipBitRate;
		char      m_strRipPath[MAX_PATH + 1];

		bool      m_bFlipBiDiCharset;
		int		  m_nVolumeLevel;				// measured in 100th's of a dB.  0dB is max, -60.00dB is min

		bool			m_bEnableRSS; // disable RSS feeds?
		bool			m_bShowFreeMem;
		int				m_iMusicOSDTimeout;  // music OSD timeout

		bool		m_bIsCdgEnabled;
		int		m_iCdgBgAlpha;
		int		m_iCdgFgAlpha;
		float		m_fCdgAVDelay;
	};

	// cache copies of these parsed values, to avoid re-parsing over and over
	CStdString m_szMyVideoStackSeparatorsString;
	CStdStringArray m_szMyVideoStackTokensArray;
	CStdString m_szMyVideoCleanSeparatorsString;
	CStdStringArray m_szMyVideoCleanTokensArray;

	VECSHARES					m_vecMyProgramsBookmarks;
	VECSHARES					m_vecMyPictureShares;
	VECSHARES					m_vecMyFilesShares;
	VECSHARES					m_vecMyMusicShares;
	VECSHARES					m_vecMyVideoShares;
	VECFILETYPEICONS	m_vecIcons;
  VECPROFILES       m_vecProfiles;
  int               m_iLastLoadedProfileIndex;
	RESOLUTION_INFO			m_ResInfo[10];
	int               m_iBrightness;
	int               m_iContrast;
	int               m_iGamma;
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

	TiXmlDocument	xbmcXml;	// for editing the xml file from within XBMC
	bool					xbmcXmlLoaded;
};

extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;