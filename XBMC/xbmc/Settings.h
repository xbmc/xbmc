#pragma once

#include "tinyxml/tinyxml.h"
#define CONFIG_VERSION 0x0005

#include <xtl.h>
#include "stdstring.h"
#include <vector>
using namespace std;

#define SHARE_TYPE_UNKNOWN	0
#define SHARE_TYPE_DVD		1
#define SHARE_TYPE_REMOTE	2
#define SHARE_TYPE_LOCAL	3

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

	void	Load();
	void	Save() const;

  struct stSettings
  {
  public:
	  unsigned long dwFileVersion;
    char	    szDefaultSkin[1024];
  
	  int				m_bMyProgramsSortMethod;
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
	  int				m_bMyPicturesSortMethod;
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
	  int				m_bMyFilesSortMethod;
	  bool			m_bMyFilesSortAscending;

		//bool			m_bMyMusicViewAsIcons;
		bool			m_bMyMusicSongsRootViewAsIcons;
		bool			m_bMyMusicSongsViewAsIcons;
		bool			m_bMyMusicAlbumRootViewAsIcons;
		bool			m_bMyMusicAlbumViewAsIcons;
		bool			m_bMyMusicArtistRootViewAsIcons;
		bool			m_bMyMusicArtistViewAsIcons;
		bool			m_bMyMusicGenresRootViewAsIcons;
		bool			m_bMyMusicGenresViewAsIcons;
		bool			m_bMyMusicPlaylistViewAsIcons;
	  int				m_bMyMusicSortMethod;
	  bool			m_bMyMusicSortAscending;
		int				m_iMyMusicViewMethod;

		bool			m_bMyVideoViewAsIcons;
		bool			m_bMyVideoRootViewAsIcons;
	  int				m_bMyVideoSortMethod;
	  bool			m_bMyVideoSortAscending;

		int				m_iMoveDelayIR;
		int				m_iRepeatDelayIR;

		int				m_iMoveDelayController;
		int				m_iRepeatDelayController;
			
		bool			m_bTimeServerEnabled;
		bool			m_bFTPServerEnabled;
		int       m_iSlideShowTransistionFrames;
		int       m_iSlideShowStayTime;
		int				m_iScreenResolution;
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
		RECT			m_rectMovieCalibration[20];
		bool			m_bAutoShufflePlaylist;
  };

  VECSHARES					m_vecMyProgramsBookmarks;
	VECSHARES					m_vecMyPictureShares;
  VECSHARES					m_vecMyFilesShares;
  VECSHARES					m_vecMyMusicShares;
  VECSHARES					m_vecMyVideoShares;
  VECFILETYPEICONS	m_vecIcons;

protected:
	bool GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName);
	int	 GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName);
	void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue);
	void GetShares(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSHARES& items, CStdString& strDefault);
	void ConvertHomeVar(CStdString& strText);
};

extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;