#pragma once

#include "tinyxml/tinyxml.h"
#define CONFIG_VERSION 0x0003

#include <xtl.h>
#include "stdstring.h"
#include <vector>
using namespace std;

class CShare
{
	public:
    CShare(){};
    virtual ~CShare(){};
		CStdString strName;
		CStdString strIcon;
		CStdString strPath;
		int        m_iBufferSize;

};
typedef vector<CShare> VECSHARES;
typedef vector<CShare>::iterator IVECSHARES;

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
    bool      m_bFTPServer;

    char      m_strLocalIPAdres[32];
    char      m_strLocalNetmask[32];
    char      m_strGateway[32];
    char      m_strNameServer[128];
    char      m_strTimeServer[128];

	  bool			m_bMyPicturesViewAsIcons;
	  int				m_bMyPicturesSortMethod;
	  bool			m_bMyPicturesSortAscending;

		char      m_szMyPicturesExtensions[256];
    char      m_szShortcutDirectory[256];

	  bool			m_bMyFilesViewAsIcons;
	  int				m_bMyFilesSortMethod;
	  bool			m_bMyFilesSortAscending;

		bool			m_bMyMusicViewAsIcons;
	  int				m_bMyMusicSortMethod;
	  bool			m_bMyMusicSortAscending;

  };
  int       m_iSlideShowTransistionFrames;
  int       m_iSlideShowStayTime;
  VECSHARES	m_vecMyProgramsBookmarks;
	VECSHARES	m_vecMyPictureShares;
  VECSHARES	m_vecMyFilesShares;

protected:
	bool GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName);
	int	 GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName);
	void GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue);
	void GetShares(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSHARES& items);
	void ConvertHomeVar(CStdString& strText);
};

extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;