#pragma once

#include "tinyxml/tinyxml.h"
#define CONFIG_VERSION 0x000F

#include <xtl.h>
#include "stdstring.h"
#include "GraphicContext.h"
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

	bool	Load(bool& bXboxMediacenter, bool& bSettings, bool &bCalibration);
	void	Save() const;

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

		char      szThumbnailsDirectory[1024];
		char      szDashboard[1024];
		int       m_iStartupWindow;


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
		int				m_iMyMusicArtistsViewAsIcons;
		bool			m_bMyMusicArtistsRootSortAscending;
		bool			m_bMyMusicArtistsSortAscending;
		int				m_iMyMusicArtistsSortMethod;
		int				m_iMyMusicArtistsRootSortMethod;
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

		bool			m_bTimeServerEnabled;
		bool			m_bFTPServerEnabled;
		bool			m_bHTTPServerEnabled;
		int       m_iSlideShowTransistionFrames;
		int       m_iSlideShowStayTime;
		RESOLUTION		m_GUIResolution;
		int			  m_iHTTPProxyPort;
		char		  m_szHTTPProxy[128];

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
		bool			m_bSoften;
		bool			m_bZoom;
		bool			m_bStretch;
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
		char      szDefaultVisualisation[256];
		bool		  m_bUseFDrive;
		bool		  m_bUseGDrive;
		bool			m_bUsePCDVDROM;
		bool			m_bDetectAsIso;
		bool			m_bAudioOnAllSpeakers;
		int				m_iChannels;
		bool			m_bDD_DTSMultiChannelPassThrough;
		bool			m_bDDStereoPassThrough;
		bool			m_bUseID3;
		char			m_szMusicRecordingDirectory[128];
		char      m_szAlternateSubtitleDirectory[128];
		bool      m_bPostProcessing;
		bool      m_bDeInterlace;
		char      m_szSubtitleFont[40];
		int       m_iSubtitleHeight;
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

		bool			m_bMyVideoVideoStack;
		bool			m_bMyVideoActorStack;
		bool			m_bMyVideoGenreStack;
		bool			m_bMyVideoYearStack;

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

		bool      m_bRipWithTrackNumber;
		int       m_iRipEncoder;
		int       m_iRipQuality;
		int       m_iRipBitRate;
		char      m_strRipPath[MAX_PATH + 1];

		char      m_szFlipBiDiCharset[128];
	};

	VECSHARES					m_vecMyProgramsBookmarks;
	VECSHARES					m_vecMyPictureShares;
	VECSHARES					m_vecMyFilesShares;
	VECSHARES					m_vecMyMusicShares;
	VECSHARES					m_vecMyVideoShares;
	VECFILETYPEICONS	m_vecIcons;
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
	void ConvertHomeVar(CStdString& strText);
	// functions for writing xml files
	void SetString(TiXmlNode* pRootNode, const CStdString& strTagName, const CStdString& strValue) const;
	void SetInteger(TiXmlNode* pRootNode, const CStdString& strTagName, int iValue) const;
	void SetFloat(TiXmlNode* pRootNode, const CStdString& strTagName, float fValue) const;
	void SetBoolean(TiXmlNode* pRootNode, const CStdString& strTagName, bool bValue) const;
	bool LoadCalibration(const CStdString& strCalibrationFile);
	bool SaveCalibration(const CStdString& strCalibrationFile) const;

	bool LoadSettings(const CStdString& strSettingsFile);
	bool SaveSettings(const CStdString& strSettingsFile) const;
};

extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;