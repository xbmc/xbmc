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

class CShare
{
	public:
    CShare(){};
    virtual ~CShare(){};
		CStdString strName;
		CStdString strPath;
		int        m_iBufferSize;
		int        m_iDriveType;

};
typedef vector<CShare> VECSHARES;
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

	bool	Load();
	void	Save() const;

  struct stSettings
  {
  public:
	  unsigned long dwFileVersion;
    char	    szDefaultSkin[1024];
  
	  int				m_iMyProgramsSortMethod;
	  bool			m_bMyProgramsSortAscending;
	  bool			m_bMyProgramsViewAsIcons;
	  bool			m_bMyProgramsFlatten;
		
    char	    szThumbnailsDirectory[1024];
    char      szDashboard[1024];
    int       m_iStartupWindow;
    

    char      m_strLocalIPAdres[32];
    char      m_strLocalNetmask[32];
    char      m_strGateway[32];
    char      m_strNameServer[128];
    char      m_strTimeServer[128];

	  bool			m_bMyPicturesViewAsIcons;
	  bool			m_bMyPicturesRootViewAsIcons;
	  int				m_iMyPicturesSortMethod;
	  bool			m_bMyPicturesSortAscending;

		char      m_szMyPicturesExtensions[256];
		char      m_szMyMusicExtensions[256];
		char      m_szMyVideoExtensions[256];
    char      m_szShortcutDirectory[256];
    char      m_szIMDBDirectory[256];
		char      m_szAlbumDirectory[256];

	  bool			m_bMyFilesSourceViewAsIcons;
	  bool			m_bMyFilesSourceRootViewAsIcons;
	  bool			m_bMyFilesDestViewAsIcons;
	  bool			m_bMyFilesDestRootViewAsIcons;
	  int				m_iMyFilesSortMethod;
	  bool			m_bMyFilesSortAscending;

		bool			m_bMyMusicSongsRootViewAsIcons;
		bool			m_bMyMusicSongsViewAsIcons;
	  bool			m_bMyMusicSongsRootSortAscending;
	  bool			m_bMyMusicSongsSortAscending;
	  int				m_iMyMusicSongsSortMethod;
		int				m_iMyMusicSongsRootSortMethod;
		bool			m_bMyMusicAlbumRootViewAsIcons;
		bool			m_bMyMusicAlbumViewAsIcons;
	  bool			m_bMyMusicAlbumRootSortAscending;
	  bool			m_bMyMusicAlbumSortAscending;
	  int				m_iMyMusicAlbumSortMethod;
	  int				m_iMyMusicAlbumRootSortMethod;
		bool			m_bMyMusicArtistsRootViewAsIcons;
		bool			m_bMyMusicArtistsViewAsIcons;
	  bool			m_bMyMusicArtistsRootSortAscending;
	  bool			m_bMyMusicArtistsSortAscending;
	  int				m_iMyMusicArtistsSortMethod;
	  int				m_iMyMusicArtistsRootSortMethod;
		bool			m_bMyMusicGenresRootViewAsIcons;
		bool			m_bMyMusicGenresViewAsIcons;
	  bool			m_bMyMusicGenresRootSortAscending;
	  bool			m_bMyMusicGenresSortAscending;
	  int				m_iMyMusicGenresSortMethod;
	  int				m_iMyMusicGenresRootSortMethod;
		bool			m_bMyMusicPlaylistViewAsIcons;
		bool			m_bMyMusicTop100ViewAsIcons;
		int				m_iMyMusicStartWindow;

		bool			m_bMyVideoViewAsIcons;
		bool			m_bMyVideoRootViewAsIcons;
	  int				m_iMyVideoSortMethod;
	  bool			m_bMyVideoSortAscending;

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
		RESOLUTION		m_ScreenResolution;
		int			  m_iHTTPProxyPort;
		char		  m_szHTTPProxy[128];

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
		bool			m_bAllowVideoSwitching;
		bool			m_bAllowPAL60;

    bool			m_bAutoShufflePlaylist;
		int			  m_iHDSpinDownTime;
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
		bool			m_bAudioOnAllSpeakers;
		int				m_iChannels;
		bool			m_bAC3PassThru;
		bool			m_bUseID3;
		char			m_szMusicRecordingDirectory[128];
    char      m_szAlternateSubtitleDirectory[128];
    bool      m_bPostProcessing;
    bool      m_bDeInterlace;
    char      m_szSubtitleFont[40];
    int       m_iSubtitleHeight;
    float     m_fVolumeAmplification;
    bool      m_bNonInterleaved;
    bool      m_bPPAuto;
    bool      m_bPPVertical;
    bool      m_bPPHorizontal;
    bool      m_bPPAutoLevels;
    int       m_iPPHorizontal;
    int       m_iPPVertical;
    bool      m_bPPdering;
    bool      m_bFrameRateConversions;
  };

  VECSHARES					m_vecMyProgramsBookmarks;
	VECSHARES					m_vecMyPictureShares;
  VECSHARES					m_vecMyFilesShares;
  VECSHARES					m_vecMyMusicShares;
  VECSHARES					m_vecMyVideoShares;
  VECFILETYPEICONS	m_vecIcons;
	RESOLUTION_INFO			m_ResInfo[10];
protected:
	void GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName, bool& bValue);
	void GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue);
	void GetFloat(const TiXmlElement* pRootElement, const CStdString& strTagName, float& fValue); 
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