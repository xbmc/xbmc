#include "settings.h"
#include "util.h"
#include "localizestrings.h"
#include "stdstring.h"
using namespace std;

class CSettings g_settings;
struct CSettings::stSettings g_stSettings;

CSettings::CSettings(void)
{
	g_stSettings.m_bMyMusicTop100ViewAsIcons=false;
	g_stSettings.m_iMyMusicAlbumSortMethod=7;	//	Album
	g_stSettings.m_iMyMusicTracksSortMethod=3;	//	Tracknum
	g_stSettings.m_iMyMusicArtistSortMethod=0;	//	Name
	g_stSettings.m_iMyMusicGenresSortMethod=0;	//	Name
	g_stSettings.m_iMyMusicSongsSortMethod=5;	//	Title
	g_stSettings.m_bAudioOnAllSpeakers=false;
	g_stSettings.m_iChannels=2;
	g_stSettings.m_bUseID3=true;
	g_stSettings.m_bAC3PassThru=false;
	g_stSettings.m_bAutorunPictures=true;
	g_stSettings.m_bAutorunMusic=true;
	g_stSettings.m_bAutorunVideo=true;
	g_stSettings.m_bAutorunDVD=true;
	g_stSettings.m_bAutorunVCD=true;
	g_stSettings.m_bAutorunCdda=true;
	g_stSettings.m_bAutorunXbox=true;
	g_stSettings.m_bUseFDrive=true;
	g_stSettings.m_bUseGDrive=false;
	strcpy(g_stSettings.szDefaultLanguage,"english");
	strcpy(g_stSettings.szDefaultVisualisation,"goom.vis");
	g_stSettings.m_minFilter=D3DTEXF_LINEAR;
	g_stSettings.m_maxFilter=D3DTEXF_LINEAR;
	g_stSettings.m_bAllowPAL60=true;
	g_stSettings.m_iHDSpinDownTime=5; // minutes
	g_stSettings.m_bAutoShufflePlaylist=true;
  g_stSettings.m_iSlideShowTransistionFrames=25;
  g_stSettings.m_iSlideShowStayTime=3000;
	g_stSettings.dwFileVersion =CONFIG_VERSION;
	g_stSettings.m_bMyProgramsViewAsIcons=false;
	g_stSettings.m_bMyProgramsSortAscending=true;
	g_stSettings.m_iMyProgramsSortMethod=0;
	g_stSettings.m_bMyProgramsFlatten=false;
  strcpy(g_stSettings.szDashboard,"C:\\xboxdash.xbe");
  strcpy(g_stSettings.m_szAlternateSubtitleDirectory,"");
  g_stSettings.m_iStartupWindow=0;
  g_stSettings.m_ScreenResolution=PAL_4x3;
  strcpy(g_stSettings.m_strLocalIPAdres,"");
  strcpy(g_stSettings.m_strLocalNetmask,"");
  strcpy(g_stSettings.m_strGateway,"");
	strcpy(g_stSettings.m_strNameServer,"");
	strcpy(g_stSettings.m_strTimeServer,"");
	g_stSettings.m_bTimeServerEnabled=false;
	g_stSettings.m_bFTPServerEnabled=false;
	g_stSettings.m_bHTTPServerEnabled=false;
	g_stSettings.m_iHTTPProxyPort=0;
	strcpy(g_stSettings.m_szHTTPProxy,"");
  strcpy(g_stSettings.szDefaultSkin,"MediaCenter");
	g_stSettings.m_bMyPicturesViewAsIcons=false;
	g_stSettings.m_bMyPicturesRootViewAsIcons=true;
	g_stSettings.m_bMyPicturesSortAscending=true;
	g_stSettings.m_iMyPicturesSortMethod=0;

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
	strcpy (g_stSettings.m_szMusicRecordingDirectory,"");
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

	g_stSettings.m_bScriptsViewAsIcons = false;
	g_stSettings.m_bScriptsRootViewAsIcons = false;
	g_stSettings.m_bScriptsSortAscending = true;
	g_stSettings.m_iScriptsSortMethod = 0;

	g_stSettings.m_bMyFilesSortAscending=true;
	g_stSettings.m_iUIOffsetX=0;
	g_stSettings.m_iUIOffsetY=0;
	g_stSettings.m_bSoften=false;
	g_stSettings.m_bZoom=false;
	g_stSettings.m_bStretch=false;

	g_stSettings.m_bAllowVideoSwitching=true;
}

CSettings::~CSettings(void)
{
}


void CSettings::Save() const
{
	if (!SaveSettings("T:\\settings.xml"))
	{
		OutputDebugString("Unable to save settings file\n");
	}
	if (!SaveCalibration("T:\\calibration.xml"))
	{
		OutputDebugString("Unable to save calibration file\n");
	}
}

void CSettings::Load()
{
	// load settings file...
	if (!LoadSettings("T:\\settings.xml"))
	{
		OutputDebugString("LoadSettings() Failed\n");
	}
	// load calibration file...
	if (!LoadCalibration("T:\\calibration.xml"))
	{
		OutputDebugString("LoadCalibration() Failed\n");
	}

	// load xml file...
	CStdString strXMLFile;
	strXMLFile+="Q:\\XboxMediaCenter.xml";

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
			GetInteger(pRemoteDelays, "move", g_stSettings.m_iMoveDelayIR);
			GetInteger(pRemoteDelays, "repeat", g_stSettings.m_iRepeatDelayIR);
		}

		if (pControllerDelays)
		{
			GetInteger(pControllerDelays, "move", g_stSettings.m_iMoveDelayController);
			GetInteger(pControllerDelays, "repeat", g_stSettings.m_iRepeatDelayController);
		}
	}

  //GetString(pRootElement, "skin", g_stSettings.szDefaultSkin,"MediaCenter");
	GetString(pRootElement, "dashboard", g_stSettings.szDashboard,"C:\\xboxdash.xbe");
	
	
	GetString(pRootElement, "CDDBIpAdres", g_stSettings.m_szCDDBIpAdres,"194.97.4.18");
	//g_stSettings.m_bUseCDDB=GetBoolean(pRootElement, "CDDBEnabled");

	GetString(pRootElement, "ipadres", g_stSettings.m_strLocalIPAdres,"");
	GetString(pRootElement, "netmask", g_stSettings.m_strLocalNetmask,"");
	GetString(pRootElement, "defaultgateway", g_stSettings.m_strGateway,"");
	GetString(pRootElement, "nameserver", g_stSettings.m_strNameServer,"");
	GetString(pRootElement, "timeserver", g_stSettings.m_strTimeServer,"207.46.248.43");
  GetString(pRootElement, "thumbnails",g_stSettings.szThumbnailsDirectory,"");
  GetString(pRootElement, "shortcuts", g_stSettings.m_szShortcutDirectory,"");
	GetString(pRootElement, "recordings", g_stSettings.m_szMusicRecordingDirectory,"");
	GetString(pRootElement, "httpproxy", g_stSettings.m_szHTTPProxy,"");
	
	GetString(pRootElement, "imdb", g_stSettings.m_szIMDBDirectory,"");
	GetString(pRootElement, "albums", g_stSettings.m_szAlbumDirectory,"");
  GetString(pRootElement, "subtitles", g_stSettings.m_szAlternateSubtitleDirectory,"");

	GetString(pRootElement, "pictureextensions", g_stSettings.m_szMyPicturesExtensions,".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");
  
	GetString(pRootElement, "musicextensions", g_stSettings.m_szMyMusicExtensions,".ac3|.aac|.nfo|.pls|.rm|.sc|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u");
	GetString(pRootElement, "videoextensions", g_stSettings.m_szMyVideoExtensions,".nfo|.rm|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli");

	GetInteger(pRootElement, "startwindow", g_stSettings.m_iStartupWindow);
	GetInteger(pRootElement, "httpproxyport", g_stSettings.m_iHTTPProxyPort);

	GetBoolean(pRootElement, "useFDrive", g_stSettings.m_bUseFDrive);
	GetBoolean(pRootElement, "useGDrive", g_stSettings.m_bUseGDrive);

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

  strDir=g_stSettings.m_szMusicRecordingDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szMusicRecordingDirectory, strDir.c_str() );

  if (g_stSettings.m_szShortcutDirectory[0])
  {
    CShare share;
    share.strPath=g_stSettings.m_szShortcutDirectory;
    share.strName="shortcuts";
     m_vecMyProgramsBookmarks.push_back(share);
  }

  strDir=g_stSettings.m_szAlternateSubtitleDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szAlternateSubtitleDirectory, strDir.c_str() );


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

void CSettings::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		iValue = atoi( pChild->FirstChild()->Value() );
	}
}

void CSettings::GetFloat(const TiXmlElement* pRootElement, const CStdString& strTagName, float& fValue)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		fValue = (float)atof( pChild->FirstChild()->Value() );
	}
}

void CSettings::GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName, bool& bValue)
{
	char szString[128];
	GetString(pRootElement,strTagName,szString,"");
	if ( CUtil::cmpnocase(szString,"enabled")==0 ||
			 CUtil::cmpnocase(szString,"yes")==0 ||
			 CUtil::cmpnocase(szString,"on")==0 ||
			 CUtil::cmpnocase(szString,"true")==0 )
	{
		bValue = true;
		return;
	}
	if (strlen(szString)!=0) bValue = false;
}

void CSettings::SetString(TiXmlNode* pRootNode, const CStdString& strTagName, const CStdString& strValue) const
{
	TiXmlElement newElement(strTagName);
	TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
	if (pNewNode)
	{
		TiXmlText value(strValue);
		pNewNode->InsertEndChild(value);
	}
}

void CSettings::SetInteger(TiXmlNode* pRootNode, const CStdString& strTagName, int iValue) const
{
	CStdString strValue;
	strValue.Format("%d",iValue);
	SetString(pRootNode, strTagName, strValue);
}

void CSettings::SetFloat(TiXmlNode* pRootNode, const CStdString& strTagName, float fValue) const
{
	CStdString strValue;
	strValue.Format("%f",fValue);
	SetString(pRootNode, strTagName, strValue);
}

void CSettings::SetBoolean(TiXmlNode* pRootNode, const CStdString& strTagName, bool bValue) const
{
	if (bValue)
		SetString(pRootNode, strTagName, "true");
	else
		SetString(pRootNode, strTagName, "false");
}

bool CSettings::LoadCalibration(const CStdString& strCalibrationFile)
{
	// reset the calibration to the defaults
	for (int i=0; i<10; i++)
		g_graphicsContext.ResetScreenParameters((RESOLUTION)i);
	// now load the xml file
	TiXmlDocument xmlDoc;
	if (!xmlDoc.LoadFile(strCalibrationFile))
	{
		OutputDebugString("Unable to load:");
		OutputDebugString(strCalibrationFile.c_str());
		OutputDebugString("\n");
		return false;
	}
	TiXmlElement *pRootElement = xmlDoc.RootElement();
	if (CUtil::cmpnocase(pRootElement->Value(),"calibration")!=0)
	{
		return false;
	}
	TiXmlElement *pResolution = pRootElement->FirstChildElement("resolution");
	while (pResolution)
	{
		// get the data for this resolution
		int iRes = -1;
		GetInteger(pResolution, "id", iRes);
		if (iRes < 0 || iRes >= 10)//MAX_RESOLUTION)
		{
			return false;
		}

		GetString(pResolution, "description", m_ResInfo[iRes].strMode, m_ResInfo[iRes].strMode);
		GetInteger(pResolution, "width", m_ResInfo[iRes].iWidth);
		GetInteger(pResolution, "height", m_ResInfo[iRes].iHeight);
		GetInteger(pResolution, "subtitles", m_ResInfo[iRes].iSubtitles);
		GetInteger(pResolution, "flags", (int &)m_ResInfo[iRes].dwFlags);
		GetFloat(pResolution, "pixelratio", m_ResInfo[iRes].fPixelRatio);

		// get the overscan info		
		TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
		if (pOverscan)
		{
			GetInteger(pOverscan, "left", m_ResInfo[iRes].Overscan.left);
			GetInteger(pOverscan, "top", m_ResInfo[iRes].Overscan.top);
			GetInteger(pOverscan, "width", m_ResInfo[iRes].Overscan.width);
			GetInteger(pOverscan, "height", m_ResInfo[iRes].Overscan.height);
		}
		// iterate around
		pResolution = pResolution->NextSiblingElement("resolution");
	}
	return true;
}

bool CSettings::SaveCalibration(const CStdString& strCalibrationFile) const
{
	TiXmlDocument xmlDoc;
	TiXmlElement xmlRootElement("calibration");
	TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
	if (!pRoot) return false;
	for (int i=0; i<10; i++)
	{
		// Write the resolution tag
		TiXmlElement resElement("resolution");
		TiXmlNode *pNode = pRoot->InsertEndChild(resElement);
		if (!pNode) return false;
		// Now write each of the pieces of information we need...
		SetString(pNode, "description", m_ResInfo[i].strMode);
		SetInteger(pNode, "id", i);
		SetInteger(pNode, "width", m_ResInfo[i].iWidth);
		SetInteger(pNode, "height", m_ResInfo[i].iHeight);
		SetInteger(pNode, "subtitles", m_ResInfo[i].iSubtitles);
		SetInteger(pNode, "flags", m_ResInfo[i].dwFlags);
		SetFloat(pNode, "pixelratio", m_ResInfo[i].fPixelRatio);
		// create the overscan child
		TiXmlElement overscanElement("overscan");
		TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
		if (!pOverscanNode) return false;
		SetInteger(pOverscanNode, "left", m_ResInfo[i].Overscan.left);
		SetInteger(pOverscanNode, "top", m_ResInfo[i].Overscan.top);
		SetInteger(pOverscanNode, "width", m_ResInfo[i].Overscan.width);
		SetInteger(pOverscanNode, "height", m_ResInfo[i].Overscan.height);
	}
	return xmlDoc.SaveFile(strCalibrationFile);
}

bool CSettings::LoadSettings(const CStdString& strSettingsFile)
{
	// load the xml file
	TiXmlDocument xmlDoc;
	if (!xmlDoc.LoadFile(strSettingsFile))
	{
		OutputDebugString("Unable to load:");
		OutputDebugString(strSettingsFile.c_str());
		OutputDebugString("\n");
		return false;
	}
	TiXmlElement *pRootElement = xmlDoc.RootElement();
	if (CUtil::cmpnocase(pRootElement->Value(),"settings")!=0)
	{
		return false;
	}
	// mypictures
	TiXmlElement *pElement = pRootElement->FirstChildElement("mypictures");
	if (pElement)
	{
		GetBoolean(pElement, "viewicons", g_stSettings.m_bMyPicturesViewAsIcons);
		GetBoolean(pElement, "rooticons", g_stSettings.m_bMyPicturesRootViewAsIcons);
		GetInteger(pElement, "sortmethod",g_stSettings.m_iMyPicturesSortMethod);
		GetBoolean(pElement, "sortascending", g_stSettings.m_bMyPicturesSortAscending);
	}
	// myfiles
	pElement = pRootElement->FirstChildElement("myfiles");
	if (pElement)
	{
		TiXmlElement *pChild = pElement->FirstChildElement("source");
		if (pChild)
		{
			GetBoolean(pChild, "viewicons", g_stSettings.m_bMyFilesSourceViewAsIcons);
			GetBoolean(pChild, "rooticons", g_stSettings.m_bMyFilesSourceRootViewAsIcons);
		}
		pChild = pElement->FirstChildElement("dest");
		if (pChild)
		{
			GetBoolean(pChild, "viewicons", g_stSettings.m_bMyFilesDestViewAsIcons);
			GetBoolean(pChild, "rooticons", g_stSettings.m_bMyFilesDestRootViewAsIcons);
		}
		GetInteger(pElement, "sortmethod",g_stSettings.m_iMyFilesSortMethod);
		GetBoolean(pElement, "sortascending", g_stSettings.m_bMyFilesSortAscending);
	}
	// mymusic settings
	pElement = pRootElement->FirstChildElement("mymusic");
	if (pElement)
	{
		TiXmlElement *pChild = pElement->FirstChildElement("songs");
		if (pChild)
		{
			GetBoolean(pChild, "viewicons", g_stSettings.m_bMyMusicSongsViewAsIcons);
			GetBoolean(pChild, "rooticons", g_stSettings.m_bMyMusicSongsRootViewAsIcons);
			GetInteger(pChild, "sortmethod",g_stSettings.m_iMyMusicSongsSortMethod);
		}
		pChild = pElement->FirstChildElement("album");
		if (pChild)
		{
			GetBoolean(pChild, "viewicons", g_stSettings.m_bMyMusicAlbumViewAsIcons);
			GetBoolean(pChild, "rooticons", g_stSettings.m_bMyMusicAlbumRootViewAsIcons);
			GetInteger(pChild, "sortmethod",g_stSettings.m_iMyMusicAlbumSortMethod);
		}
		pElement = pElement->FirstChildElement("artist");
		if (pChild)
		{
			GetBoolean(pChild, "viewicons", g_stSettings.m_bMyMusicArtistViewAsIcons);
			GetBoolean(pChild, "rooticons", g_stSettings.m_bMyMusicArtistRootViewAsIcons);
			GetInteger(pChild, "sortmethod",g_stSettings.m_iMyMusicArtistSortMethod);
		}
		pChild = pElement->FirstChildElement("genre");
		if (pChild)
		{
			GetBoolean(pChild, "viewicons", g_stSettings.m_bMyMusicGenresViewAsIcons);
			GetBoolean(pChild, "rooticons", g_stSettings.m_bMyMusicGenresRootViewAsIcons);
			GetInteger(pChild, "sortmethod",g_stSettings.m_iMyMusicGenresSortMethod);
		}
		GetBoolean(pElement, "playlistviewicons", g_stSettings.m_bMyMusicPlaylistViewAsIcons);
		GetBoolean(pElement, "top100viewicons", g_stSettings.m_bMyMusicTop100ViewAsIcons);
		GetInteger(pElement, "sortmethod",g_stSettings.m_iMyMusicSortMethod);
		GetInteger(pElement, "trackssortmethod",g_stSettings.m_iMyMusicTracksSortMethod);
		GetBoolean(pElement, "sortascending", g_stSettings.m_bMyMusicSortAscending);
		GetInteger(pElement, "viewmethod",g_stSettings.m_iMyMusicViewMethod);
	}
	// myvideos settings
	pElement = pRootElement->FirstChildElement("myvideos");
	if (pElement)
	{
		GetBoolean(pElement, "viewicons", g_stSettings.m_bMyVideoViewAsIcons);
		GetBoolean(pElement, "rooticons", g_stSettings.m_bMyVideoRootViewAsIcons);
		GetInteger(pElement, "sortmethod",g_stSettings.m_iMyVideoSortMethod);
		GetBoolean(pElement, "sortascending", g_stSettings.m_bMyVideoSortAscending);
	}
	// myscripts settings
	pElement = pRootElement->FirstChildElement("myscripts");
	if (pElement)
	{
		GetBoolean(pElement, "viewicons", g_stSettings.m_bScriptsViewAsIcons);
		GetBoolean(pElement, "rooticons", g_stSettings.m_bScriptsRootViewAsIcons);
		GetInteger(pElement, "sortmethod",g_stSettings.m_iScriptsSortMethod);
		GetBoolean(pElement, "sortascending", g_stSettings.m_bScriptsSortAscending);
	}
	// general settings
	pElement = pRootElement->FirstChildElement("general");
	if (pElement)
	{
		GetString(pElement, "skin", g_stSettings.szDefaultSkin, g_stSettings.szDefaultSkin);
		GetBoolean(pElement, "timeserver", g_stSettings.m_bTimeServerEnabled);
		GetBoolean(pElement, "ftpserver", g_stSettings.m_bFTPServerEnabled);
		GetBoolean(pElement, "httpserver", g_stSettings.m_bHTTPServerEnabled);
		GetBoolean(pElement, "cddb", g_stSettings.m_bUseCDDB);
		GetInteger(pElement, "hdspindowntime", g_stSettings.m_iHDSpinDownTime);
		GetBoolean(pElement, "autorundvd", g_stSettings.m_bAutorunDVD);
		GetBoolean(pElement, "autorunvcd", g_stSettings.m_bAutorunVCD);
		GetBoolean(pElement, "autoruncdda", g_stSettings.m_bAutorunCdda);
		GetBoolean(pElement, "autorunxbox", g_stSettings.m_bAutorunXbox);
		GetBoolean(pElement, "autorunmusic", g_stSettings.m_bAutorunMusic);
		GetBoolean(pElement, "autorunvideo", g_stSettings.m_bAutorunVideo);
		GetBoolean(pElement, "autorunpictures", g_stSettings.m_bAutorunPictures);
		GetString(pElement, "language", g_stSettings.szDefaultLanguage, g_stSettings.szDefaultLanguage);
	}
	// slideshow settings
	pElement = pRootElement->FirstChildElement("slideshow");
	if (pElement)
	{
		GetInteger(pElement, "transistionframes", g_stSettings.m_iSlideShowTransistionFrames);
		GetInteger(pElement, "staytime", g_stSettings.m_iSlideShowStayTime);
	}
	// screen settings
	pElement = pRootElement->FirstChildElement("screen");
	if (pElement)
	{
		int iRes = (int)g_stSettings.m_ScreenResolution;
		GetInteger(pElement, "resolution", iRes);
		if (iRes >=0 || iRes <= 10) g_stSettings.m_ScreenResolution = (RESOLUTION)iRes;
		GetInteger(pElement, "uioffsetx",g_stSettings.m_iUIOffsetX);
		GetInteger(pElement, "uioffsety",g_stSettings.m_iUIOffsetY);
		GetBoolean(pElement, "soften", g_stSettings.m_bSoften);
		GetBoolean(pElement, "zoom", g_stSettings.m_bZoom);
		GetBoolean(pElement, "stretch", g_stSettings.m_bStretch);
		GetBoolean(pElement, "allowswitching", g_stSettings.m_bAllowVideoSwitching);
		GetBoolean(pElement, "allowpal60", g_stSettings.m_bAllowPAL60);
		GetInteger(pElement, "minfilter", (int &)g_stSettings.m_minFilter);
		GetInteger(pElement, "maxfilter", (int &)g_stSettings.m_maxFilter);
	}
	// audio settings
	pElement = pRootElement->FirstChildElement("audio");
	if (pElement)
	{
		GetBoolean(pElement, "audioonallspeakers", g_stSettings.m_bAudioOnAllSpeakers);
		GetInteger(pElement, "channels",g_stSettings.m_iChannels);
		GetBoolean(pElement, "ac3passthru", g_stSettings.m_bAC3PassThru);
		GetBoolean(pElement, "useid3", g_stSettings.m_bUseID3);
		GetString(pElement, "visualisation", g_stSettings.szDefaultVisualisation, g_stSettings.szDefaultVisualisation);
		GetBoolean(pElement, "autoshuffleplaylist", g_stSettings.m_bAutoShufflePlaylist);
	}
	return true;
}

bool CSettings::SaveSettings(const CStdString& strSettingsFile) const
{
	TiXmlDocument xmlDoc;
	TiXmlElement xmlRootElement("settings");
	TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
	if (!pRoot) return false;
	// write our tags one by one - just a big list for now (can be flashed up later)
	// myprograms settings
	TiXmlElement programsNode("myprograms");
	TiXmlNode *pNode = pRoot->InsertEndChild(programsNode);
	if (!pNode) return false;
	SetBoolean(pNode, "viewicons", g_stSettings.m_bMyProgramsViewAsIcons);
	SetInteger(pNode, "sortmethod", g_stSettings.m_iMyProgramsSortMethod);
	SetBoolean(pNode, "sortascending", g_stSettings.m_bMyProgramsSortAscending);
	SetBoolean(pNode, "flatten", g_stSettings.m_bMyProgramsFlatten);
	// mypictures settings
	TiXmlElement picturesNode("mypictures");
	pNode = pRoot->InsertEndChild(picturesNode);
	if (!pNode) return false;
	SetBoolean(pNode, "viewicons", g_stSettings.m_bMyPicturesViewAsIcons);
	SetBoolean(pNode, "rooticons", g_stSettings.m_bMyPicturesRootViewAsIcons);
	SetInteger(pNode, "sortmethod",g_stSettings.m_iMyPicturesSortMethod);
	SetBoolean(pNode, "sortascending", g_stSettings.m_bMyPicturesSortAscending);
	// myfiles settings
	TiXmlElement filesNode("myfiles");
	pNode = pRoot->InsertEndChild(filesNode);
	if (!pNode) return false;
	{
		TiXmlElement childNode("source");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "viewicons", g_stSettings.m_bMyFilesSourceViewAsIcons);
		SetBoolean(pChild, "rooticons", g_stSettings.m_bMyFilesSourceRootViewAsIcons);
	}
	{
		TiXmlElement childNode("dest");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "viewicons", g_stSettings.m_bMyFilesDestViewAsIcons);
		SetBoolean(pChild, "rooticons", g_stSettings.m_bMyFilesDestRootViewAsIcons);
	}
	SetInteger(pNode, "sortmethod",g_stSettings.m_iMyFilesSortMethod);
	SetBoolean(pNode, "sortascending", g_stSettings.m_bMyFilesSortAscending);
	// mymusic settings
	TiXmlElement musicNode("mymusic");
	pNode = pRoot->InsertEndChild(musicNode);
	if (!pNode) return false;
	{
		TiXmlElement childNode("songs");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "viewicons", g_stSettings.m_bMyMusicSongsViewAsIcons);
		SetBoolean(pChild, "rooticons", g_stSettings.m_bMyMusicSongsRootViewAsIcons);
		SetInteger(pChild, "sortmethod",g_stSettings.m_iMyMusicSongsSortMethod);
	}
	{
		TiXmlElement childNode("album");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "viewicons", g_stSettings.m_bMyMusicAlbumViewAsIcons);
		SetBoolean(pChild, "rooticons", g_stSettings.m_bMyMusicAlbumRootViewAsIcons);
		SetInteger(pChild, "sortmethod",g_stSettings.m_iMyMusicAlbumSortMethod);
	}
	{
		TiXmlElement childNode("artist");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "viewicons", g_stSettings.m_bMyMusicArtistViewAsIcons);
		SetBoolean(pChild, "rooticons", g_stSettings.m_bMyMusicArtistRootViewAsIcons);
		SetInteger(pChild, "sortmethod",g_stSettings.m_iMyMusicArtistSortMethod);
	}
	{
		TiXmlElement childNode("genre");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "viewicons", g_stSettings.m_bMyMusicGenresViewAsIcons);
		SetBoolean(pChild, "rooticons", g_stSettings.m_bMyMusicGenresRootViewAsIcons);
		SetInteger(pChild, "sortmethod",g_stSettings.m_iMyMusicGenresSortMethod);
	}

	SetBoolean(pNode, "playlistviewicons", g_stSettings.m_bMyMusicPlaylistViewAsIcons);
	SetBoolean(pNode, "top100viewicons", g_stSettings.m_bMyMusicTop100ViewAsIcons);

	SetInteger(pNode, "sortmethod",g_stSettings.m_iMyMusicSortMethod);
	SetInteger(pNode, "trackssortmethod",g_stSettings.m_iMyMusicTracksSortMethod);
	SetBoolean(pNode, "sortascending", g_stSettings.m_bMyMusicSortAscending);
	SetInteger(pNode, "viewmethod",g_stSettings.m_iMyMusicViewMethod);
	// myvideos settings
	TiXmlElement videosNode("myvideos");
	pNode = pRoot->InsertEndChild(videosNode);
	if (!pNode) return false;
	SetBoolean(pNode, "viewicons", g_stSettings.m_bMyVideoViewAsIcons);
	SetBoolean(pNode, "rooticons", g_stSettings.m_bMyVideoRootViewAsIcons);
	SetInteger(pNode, "sortmethod",g_stSettings.m_iMyVideoSortMethod);
	SetBoolean(pNode, "sortascending", g_stSettings.m_bMyVideoSortAscending);
	// myscripts settings
	TiXmlElement scriptsNode("myscripts");
	pNode = pRoot->InsertEndChild(scriptsNode);
	if (!pNode) return false;
	SetBoolean(pNode, "viewicons", g_stSettings.m_bScriptsViewAsIcons);
	SetBoolean(pNode, "rooticons", g_stSettings.m_bScriptsRootViewAsIcons);
	SetInteger(pNode, "sortmethod",g_stSettings.m_iScriptsSortMethod);
	SetBoolean(pNode, "sortascending", g_stSettings.m_bScriptsSortAscending);
	// general settings
	TiXmlElement generalNode("general");
	pNode = pRoot->InsertEndChild(generalNode);
	if (!pNode) return false;
	SetString(pNode, "skin", g_stSettings.szDefaultSkin);
	SetBoolean(pNode, "timeserver", g_stSettings.m_bTimeServerEnabled);
	SetBoolean(pNode, "ftpserver", g_stSettings.m_bFTPServerEnabled);
	SetBoolean(pNode, "httpserver", g_stSettings.m_bHTTPServerEnabled);
	SetBoolean(pNode, "cddb", g_stSettings.m_bUseCDDB);
	SetInteger(pNode, "hdspindowntime", g_stSettings.m_iHDSpinDownTime);
	SetBoolean(pNode, "autorundvd", g_stSettings.m_bAutorunDVD);
	SetBoolean(pNode, "autorunvcd", g_stSettings.m_bAutorunVCD);
	SetBoolean(pNode, "autoruncdda", g_stSettings.m_bAutorunCdda);
	SetBoolean(pNode, "autorunxbox", g_stSettings.m_bAutorunXbox);
	SetBoolean(pNode, "autorunmusic", g_stSettings.m_bAutorunMusic);
	SetBoolean(pNode, "autorunvideo", g_stSettings.m_bAutorunVideo);
	SetBoolean(pNode, "autorunpictures", g_stSettings.m_bAutorunPictures);
	SetString(pNode, "language", g_stSettings.szDefaultLanguage);
	// slideshow settings
	TiXmlElement slideshowNode("slideshow");
	pNode = pRoot->InsertEndChild(slideshowNode);
	if (!pNode) return false;
	SetInteger(pNode, "transistionframes", g_stSettings.m_iSlideShowTransistionFrames);
	SetInteger(pNode, "staytime", g_stSettings.m_iSlideShowStayTime);
	// screen settings
	TiXmlElement screenNode("screen");
	pNode = pRoot->InsertEndChild(screenNode);
	if (!pNode) return false;
	SetInteger(pNode, "resolution", (int)g_stSettings.m_ScreenResolution);
	SetInteger(pNode, "uioffsetx",g_stSettings.m_iUIOffsetX);
	SetInteger(pNode, "uioffsety",g_stSettings.m_iUIOffsetY);
	SetBoolean(pNode, "soften", g_stSettings.m_bSoften);
	SetBoolean(pNode, "zoom", g_stSettings.m_bZoom);
	SetBoolean(pNode, "stretch", g_stSettings.m_bStretch);
	SetBoolean(pNode, "allowswitching", g_stSettings.m_bAllowVideoSwitching);
	SetBoolean(pNode, "allowpal60", g_stSettings.m_bAllowPAL60);
	SetInteger(pNode, "minfilter", g_stSettings.m_minFilter);
	SetInteger(pNode, "maxfilter", g_stSettings.m_maxFilter);
	// audio settings
	TiXmlElement audioNode("audio");
	pNode = pRoot->InsertEndChild(audioNode);
	if (!pNode) return false;
	SetBoolean(pNode, "audioonallspeakers", g_stSettings.m_bAudioOnAllSpeakers);
	SetInteger(pNode, "channels",g_stSettings.m_iChannels);
	SetBoolean(pNode, "ac3passthru", g_stSettings.m_bAC3PassThru);
	SetBoolean(pNode, "useid3", g_stSettings.m_bUseID3);
	SetString(pNode, "visualisation", g_stSettings.szDefaultVisualisation);
    SetBoolean(pNode, "autoshuffleplaylist", g_stSettings.m_bAutoShufflePlaylist);

	// save the file
	return xmlDoc.SaveFile(strSettingsFile);
}
