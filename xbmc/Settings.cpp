#include "settings.h"
#include "util.h"
#include "localizestrings.h"
#include "stdstring.h"
using namespace std;

class CSettings g_settings;
struct CSettings::stSettings g_stSettings;

CSettings::CSettings(void)
{
  g_stSettings.m_iSlideShowTransistionFrames=25;
  g_stSettings.m_iSlideShowStayTime=3000;
	g_stSettings.dwFileVersion =CONFIG_VERSION;
	g_stSettings.m_bMyProgramsViewAsIcons=false;
	g_stSettings.m_bMyProgramsSortAscending=true;
	g_stSettings.m_bMyProgramsSortMethod=0;
	g_stSettings.m_bMyProgramsFlatten=false;
  strcpy(g_stSettings.szDashboard,"C:\\xboxdash.xbe");
  g_stSettings.m_iStartupWindow=0;
  g_stSettings.m_iScreenResolution=0;
  strcpy(g_stSettings.m_strLocalIPAdres,"");
  strcpy(g_stSettings.m_strLocalNetmask,"");
  strcpy(g_stSettings.m_strGateway,"");
	strcpy(g_stSettings.m_strNameServer,"");
	strcpy(g_stSettings.m_strTimeServer,"");
	g_stSettings.m_bTimeServerEnabled=false;
	g_stSettings.m_bFTPServerEnabled=false;
	g_stSettings.m_iHTTPProxyPort=0;
	strcpy(g_stSettings.m_szHTTPProxy,"");
  strcpy(g_stSettings.szDefaultSkin,"MediaCenter");
	g_stSettings.m_bMyPicturesViewAsIcons=false;
	g_stSettings.m_bMyPicturesRootViewAsIcons=true;
	g_stSettings.m_bMyPicturesSortAscending=true;
	g_stSettings.m_bMyPicturesSortMethod=0;

	g_stSettings.m_iMoveDelayIR=220;
	g_stSettings.m_iRepeatDelayIR=220;
	g_stSettings.m_iMoveDelayController=220;
	g_stSettings.m_iRepeatDelayController=220;
	strcpy(g_stSettings.m_szMyPicturesExtensions,".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");
	strcpy(g_stSettings.m_szMyMusicExtensions,".ac3|.aac|.nfo|.pls|.rm|.sc|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u");
	strcpy(g_stSettings.m_szMyVideoExtensions,".nfo|.rm|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli");
	
	strcpy( g_stSettings.m_szDefaultMusic, "");	
	strcpy( g_stSettings.m_szDefaultPictures, "");	
	strcpy( g_stSettings.m_szDefaultFiles, "");	
	strcpy( g_stSettings.m_szDefaultVideos, "");	
	strcpy( g_stSettings.m_szCDDBIpAdres,"");
	g_stSettings.m_bUseCDDB=false;
	g_stSettings.m_iMyMusicViewMethod=2;//view songs
	g_stSettings.m_bMyMusicSongsRootViewAsIcons= true; //thumbs
	g_stSettings.m_bMyMusicSongsViewAsIcons = false;
	g_stSettings.m_bMyMusicAlbumRootViewAsIcons= true;
	g_stSettings.m_bMyMusicAlbumViewAsIcons = false;
	g_stSettings.m_bMyMusicArtistRootViewAsIcons = false;
	g_stSettings.m_bMyMusicArtistViewAsIcons = false;
	g_stSettings.m_bMyMusicGenresRootViewAsIcons = false;
	g_stSettings.m_bMyMusicGenresViewAsIcons = false;
	g_stSettings.m_bMyMusicPlaylistViewAsIcons = false;
	g_stSettings.m_bMyMusicSortAscending=true;

	g_stSettings.m_bMyVideoViewAsIcons=false;
	g_stSettings.m_bMyVideoRootViewAsIcons=true;
	g_stSettings.m_bMyVideoSortAscending=true;
	g_stSettings.m_bMyFilesSourceViewAsIcons=false;
	g_stSettings.m_bMyFilesSourceRootViewAsIcons=true;
	g_stSettings.m_bMyFilesDestViewAsIcons=false;
	g_stSettings.m_bMyFilesDestRootViewAsIcons=true;

	g_stSettings.m_bMyFilesSortAscending=true;
	g_stSettings.m_iUIOffsetX=0;
	g_stSettings.m_iUIOffsetY=0;
	g_stSettings.m_bSoften=false;
	g_stSettings.m_bZoom=false;
	g_stSettings.m_bStretch=false;

	g_stSettings.m_iMoviesOffsetX1=0;
	g_stSettings.m_iMoviesOffsetY1=0;
	g_stSettings.m_iMoviesOffsetX2=0;
	g_stSettings.m_iMoviesOffsetY2=0;
	g_stSettings.m_bAllowVideoSwitching=true;
}

CSettings::~CSettings(void)
{
}


void CSettings::Save() const
{
	FILE* systemSettings = fopen("T:\\system.bin","wb+");
	if (systemSettings!=NULL)
	{
		fwrite(&g_stSettings,sizeof(g_stSettings),1,systemSettings);
		fclose(systemSettings);
	}
}

void CSettings::Load()
{
  struct CSettings::stSettings settings;
	FILE* systemSettings = fopen("T:\\system.bin","rb");
	if (systemSettings!=NULL)
	{
		OutputDebugString("found system.bin\n");
		fread(&settings,sizeof(settings),1,systemSettings);
		fclose(systemSettings);
		if (settings.dwFileVersion==CONFIG_VERSION) 
    {
			OutputDebugString("version is ok\n");
		  FILE* systemSettings = fopen("T:\\system.bin","rb");
		  fread(&g_stSettings,sizeof(g_stSettings),1,systemSettings);
		  fclose(systemSettings);
    }
		else
		{
			OutputDebugString("version is wrong\n");
		}
	}
	else
	{
		OutputDebugString("settings not found\n");
	}

	// load xml file...
	CStdString strXMLFile;
	CUtil::GetHomePath(strXMLFile);
	strXMLFile+="\\XboxMediaCenter.xml";

	TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( strXMLFile.c_str() ) ) 
	{
		OutputDebugString("unable to load:");
		OutputDebugString(strXMLFile.c_str());
		OutputDebugString("\n");
		return;
	}

	TiXmlElement* pRootElement =xmlDoc.RootElement();
  CStdString strValue=pRootElement->Value();
	if ( strValue != "xboxmediacenter") return ;

	TiXmlElement* pFileTypeIcons =pRootElement->FirstChildElement("filetypeicons");
	TiXmlNode* pFileType=pFileTypeIcons->FirstChild();
	while (pFileType)
	{
		CFileTypeIcon icon;
		icon.m_strName=".";
		icon.m_strName+=pFileType->Value();
		icon.m_strIcon=pFileType->FirstChild()->Value();
		m_vecIcons.push_back(icon);
		pFileType=pFileType->NextSibling();
	}

	TiXmlElement* pDelaysElement =pRootElement->FirstChildElement("delays");
	if (pDelaysElement)
	{
		TiXmlElement* pRemoteDelays			=pDelaysElement->FirstChildElement("remote");
		TiXmlElement* pControllerDelays =pDelaysElement->FirstChildElement("controller");
		if (pRemoteDelays)
		{
			g_stSettings.m_iMoveDelayIR=GetInteger(pRemoteDelays, "move");
			g_stSettings.m_iRepeatDelayIR=GetInteger(pRemoteDelays, "repeat");
		}

		if (pControllerDelays)
		{
			g_stSettings.m_iMoveDelayController=GetInteger(pControllerDelays, "move");
			g_stSettings.m_iRepeatDelayController=GetInteger(pControllerDelays, "repeat");
		}
	}

  GetString(pRootElement, "skin", g_stSettings.szDefaultSkin,"MediaCenter");
	GetString(pRootElement, "dashboard", g_stSettings.szDashboard,"C:\\xboxdash.xbe");
	
	
	GetString(pRootElement, "CDDBIpAdres", g_stSettings.m_szCDDBIpAdres,"194.97.4.18");
	g_stSettings.m_bUseCDDB=GetBoolean(pRootElement, "CDDBEnabled");

	GetString(pRootElement, "ipadres", g_stSettings.m_strLocalIPAdres,"");
	GetString(pRootElement, "netmask", g_stSettings.m_strLocalNetmask,"");
	GetString(pRootElement, "defaultgateway", g_stSettings.m_strGateway,"");
	GetString(pRootElement, "nameserver", g_stSettings.m_strNameServer,"");
	GetString(pRootElement, "timeserver", g_stSettings.m_strTimeServer,"207.46.248.43");
  GetString(pRootElement, "thumbnails",g_stSettings.szThumbnailsDirectory,"");
  GetString(pRootElement, "shortcuts", g_stSettings.m_szShortcutDirectory,"");
	GetString(pRootElement, "httpproxy", g_stSettings.m_szHTTPProxy,"");
	
	GetString(pRootElement, "imdb", g_stSettings.m_szIMDBDirectory,"");
	GetString(pRootElement, "albums", g_stSettings.m_szAlbumDirectory,"");

	GetString(pRootElement, "pictureextensions", g_stSettings.m_szMyPicturesExtensions,".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");
  
	GetString(pRootElement, "musicextensions", g_stSettings.m_szMyMusicExtensions,".ac3|.aac|.nfo|.pls|.rm|.sc|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u");
	GetString(pRootElement, "videoextensions", g_stSettings.m_szMyVideoExtensions,".nfo|.rm|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli");

	g_stSettings.m_iStartupWindow=GetInteger(pRootElement, "startwindow");
	g_stSettings.m_iHTTPProxyPort=GetInteger(pRootElement, "httpproxyport");
	

  CStdString strDir;
  strDir=g_stSettings.m_szShortcutDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szShortcutDirectory, strDir.c_str() );

  strDir=g_stSettings.szThumbnailsDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.szThumbnailsDirectory, strDir.c_str() );


  strDir=g_stSettings.m_szIMDBDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szIMDBDirectory, strDir.c_str() );

  strDir=g_stSettings.m_szAlbumDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szAlbumDirectory, strDir.c_str() );

  if (g_stSettings.m_szShortcutDirectory[0])
  {
		const WCHAR* szText;
    szText=g_localizeStrings.Get(111).c_str();
    CShare share;
    share.strPath=g_stSettings.m_szShortcutDirectory;
    CUtil::Unicode2Ansi(szText,share.strName);
     m_vecMyProgramsBookmarks.push_back(share);
  }

  // parse my programs bookmarks...
	CStdString strDefault;
	GetShares(pRootElement,"myprograms",m_vecMyProgramsBookmarks,strDefault);
	

	GetShares(pRootElement,"pictures",m_vecMyPictureShares,strDefault);
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultPictures, strDefault.c_str());

  GetShares(pRootElement,"files",m_vecMyFilesShares,strDefault);
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultFiles, strDefault.c_str());

  GetShares(pRootElement,"music",m_vecMyMusicShares,strDefault);
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultMusic, strDefault.c_str());	

  GetShares(pRootElement,"video",m_vecMyVideoShares,strDefault);
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultVideos, strDefault.c_str());	

}

void CSettings::ConvertHomeVar(CStdString& strText)
{
	// Replaces first occurence of $HOME with the home directory.
	// "$HOME\bookmarks" becomes for instance "e:\apps\xbmp\bookmarks"

	char szText[1024];
  char szTemp[1024];
  char *pReplace,*pReplace2;

	CStdString strHomePath;
	CUtil::GetHomePath(strHomePath);
	strcpy(szText,strText.c_str());

  pReplace = strstr(szText, "$HOME");

  if (pReplace!=NULL) 
	{
	  pReplace2 = pReplace + sizeof("$HOME")-1;
	  strncpy(szTemp, pReplace2, sizeof("$HOME") - (szText-pReplace2) );
	  strcpy(pReplace, strHomePath.c_str() );
	  strcat(szText, szTemp);
  }
	strText=szText;
}

void CSettings::GetShares(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSHARES& items,CStdString& strDefault)
{
	strDefault="";
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		pChild = pChild->FirstChild();
		while (pChild>0)
		{
			CStdString strValue=pChild->Value();
			if (strValue=="bookmark")
			{
				const TiXmlNode *pNodeName=pChild->FirstChild("name");
				const TiXmlNode *pPathName=pChild->FirstChild("path");
        const TiXmlNode *pCacheNode=pChild->FirstChild("cache");
				if (pNodeName && pPathName)
				{
          const char* szName=pNodeName->FirstChild()->Value();
          const char* szPath=pPathName->FirstChild()->Value();

					CShare share;
					share.strName=szName;
					share.strPath=szPath;
					share.m_iBufferSize=0;
					if ( CUtil::IsDVD( share.strPath ) )
						share.m_iDriveType = SHARE_TYPE_DVD;
					else if ( CUtil::IsRemote( share.strPath ) )
						share.m_iDriveType = SHARE_TYPE_REMOTE;
					else 
						share.m_iDriveType = SHARE_TYPE_LOCAL;

          
          if (pCacheNode)
          {
            share.m_iBufferSize=atoi( pCacheNode->FirstChild()->Value() );
          }

          
          
					ConvertHomeVar(share.strPath);

					items.push_back(share);
				}
			}
			if (strValue=="default")
			{
				const TiXmlNode *pValueNode=pChild->FirstChild();
				if (pValueNode)
				{
					const char* pszText=pChild->FirstChild()->Value();
					if (strlen(pszText) > 0)
						strDefault=pszText;
				}
			}
		  pChild=pChild->NextSibling();
		}
	}
}

void CSettings::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char* szValue, const CStdString& strDefaultValue)
{
	strcpy(szValue,"");
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		CStdString strValue=pChild->FirstChild()->Value();
		if (strValue.size() )
		{
			if (strValue !="-")
				strcpy(szValue,strValue.c_str());
		}
	}
	if (strlen(szValue)==0)
	{
		strcpy(szValue,strDefaultValue.c_str());
	}
}

int CSettings::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		return atoi( pChild->FirstChild()->Value() );
	}
	return 0;
}

bool CSettings::GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName)
{
	char szString[128];
	GetString(pRootElement,strTagName,szString,"");
	if ( CUtil::cmpnocase(szString,"enabled")==0 ||
			 CUtil::cmpnocase(szString,"yes")==0 ||
			 CUtil::cmpnocase(szString,"on")==0 ||
			 CUtil::cmpnocase(szString,"true") ) return true;

	return false;
}