#include "settings.h"
#include "util.h"
#include "utils/log.h"
#include "localizestrings.h"
#include "stdstring.h"
#include "GraphicContext.h"
#include "GUIWindowMusicBase.h"
using namespace std;

class CSettings g_settings;
struct CSettings::stSettings g_stSettings;

CSettings::CSettings(void)
{
  strcpy(g_stSettings.m_szExternalDVDPlayer,"");
  g_stSettings.m_bMyVideoVideoStack =false;
  g_stSettings.m_bMyVideoActorStack =false;
  g_stSettings.m_bMyVideoGenreStack =false;
  g_stSettings.m_bMyVideoYearStack =false;


  g_stSettings.m_bPPAuto=true;
  g_stSettings.m_bPPVertical=false;
  g_stSettings.m_bPPHorizontal=false;
  g_stSettings.m_bPPAutoLevels=false;
  g_stSettings.m_bPPdering=false;
  g_stSettings.m_bFrameRateConversions=false;
  g_stSettings.m_bUseDigitalOutput=false;

  strcpy(g_stSettings.m_szSubtitleFont,"arial-iso-8859-1");
  g_stSettings.m_bPostProcessing=true;
  g_stSettings.m_bDeInterlace=false;
  g_stSettings.m_bNonInterleaved=false;
	g_stSettings.m_bAudioOnAllSpeakers=false;
	g_stSettings.m_bUseID3=true;
	g_stSettings.m_bDD_DTSMultiChannelPassThrough=false;
	g_stSettings.m_bDDStereoPassThrough=false;
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
	g_stSettings.m_bAllowPAL60=true;
	g_stSettings.m_bAutoShufflePlaylist=true;
	g_stSettings.dwFileVersion =CONFIG_VERSION;
	g_stSettings.m_bMyProgramsViewAsIcons=false;
	g_stSettings.m_bMyProgramsSortAscending=true;
	g_stSettings.m_bMyProgramsFlatten=false;
	g_stSettings.m_bMyProgramsDefaultXBE=false;
  strcpy(g_stSettings.szDashboard,"C:\\xboxdash.xbe");
  strcpy(g_stSettings.m_szAlternateSubtitleDirectory,"");
  strcpy(g_stSettings.m_strLocalIPAdres,"");
  strcpy(g_stSettings.m_strLocalNetmask,"");
  strcpy(g_stSettings.m_strGateway,"");
	strcpy(g_stSettings.m_strNameServer,"");
	strcpy(g_stSettings.m_strTimeServer,"");
	g_stSettings.m_bTimeServerEnabled=false;
	g_stSettings.m_bFTPServerEnabled=false;
	g_stSettings.m_bHTTPServerEnabled=false;
	strcpy(g_stSettings.m_szHTTPProxy,"");
  strcpy(g_stSettings.szDefaultSkin,"MediaCenter");
	strcpy(g_stSettings.szHomeDir,"");
	g_stSettings.m_bMyPicturesSortAscending=true;

	strcpy(g_stSettings.m_szMyPicturesExtensions,".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");
	strcpy(g_stSettings.m_szMyMusicExtensions,".ac3|.aac|.pls|.rm|.sc|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u");
	strcpy(g_stSettings.m_szMyVideoExtensions,".nfo|.rm|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli");
	
	strcpy( g_stSettings.m_szDefaultPrograms, "Q:\\shortcuts");
	strcpy( g_stSettings.m_szDefaultMusic, "");	
	strcpy( g_stSettings.m_szDefaultPictures, "");	
	strcpy( g_stSettings.m_szDefaultFiles, "");	
	strcpy( g_stSettings.m_szDefaultVideos, "");	
	strcpy( g_stSettings.m_szCDDBIpAdres,"");
	strcpy (g_stSettings.m_szMusicRecordingDirectory,"");
	g_stSettings.m_bUseCDDB=false;

	g_stSettings.m_bMyMusicSongsRootSortAscending=true;
	g_stSettings.m_bMyMusicSongsSortAscending=true;

	g_stSettings.m_bMyMusicAlbumRootSortAscending=true;
	g_stSettings.m_bMyMusicAlbumSortAscending=true;
	g_stSettings.m_bMyMusicAlbumShowRecent=false;

	g_stSettings.m_bMyMusicArtistsRootSortAscending=true;
	g_stSettings.m_bMyMusicArtistsSortAscending=true;

	g_stSettings.m_bMyMusicGenresRootSortAscending=true;
	g_stSettings.m_bMyMusicGenresSortAscending=true;

	g_stSettings.m_bMyVideoSortAscending=true;

  g_stSettings.m_bMyVideoGenreSortAscending=true;

  g_stSettings.m_bMyVideoActorSortAscending=true;

  g_stSettings.m_bMyVideoYearSortAscending=true;

  g_stSettings.m_bMyVideoTitleSortAscending=true;

	g_stSettings.m_bMyFilesSourceViewAsIcons=false;
	g_stSettings.m_bMyFilesSourceRootViewAsIcons=true;
	g_stSettings.m_bMyFilesDestViewAsIcons=false;
	g_stSettings.m_bMyFilesDestRootViewAsIcons=true;

	g_stSettings.m_bScriptsViewAsIcons = false;
	g_stSettings.m_bScriptsRootViewAsIcons = false;
	g_stSettings.m_bScriptsSortAscending = true;

	g_stSettings.m_bMyFilesSortAscending=true;
	g_stSettings.m_bSoften=false;
	g_stSettings.m_bZoom=false;
	g_stSettings.m_bStretch=false;

	g_stSettings.m_bAllowVideoSwitching=true;
	strcpy(g_stSettings.m_szWeatherArea, "UKXX0085");	//default WEATHER to London for no good reason
	strcpy(g_stSettings.m_szWeatherFTemp, "C");			//default WEATHER temp units 
	strcpy(g_stSettings.m_szWeatherFSpeed, "K");		//default WEATHER speed units
	g_stSettings.m_iWeatherRefresh = 1;
}

CSettings::~CSettings(void)
{
}


void CSettings::Save() const
{
	if (!SaveSettings("T:\\settings.xml"))
	{
    CLog::Log("Unable to T:\\settings.xml");
	}
	if (!SaveCalibration("T:\\calibration.xml"))
	{
		CLog::Log("Unable to T:\\calibration.xml");
	}
}

bool CSettings::Load()
{
	// load settings file...
  CLog::Log("loading T:\\settings.xml");
	if (!LoadSettings("T:\\settings.xml"))
	{
    CLog::Log("Unable to load T:\\settings.xml");
	}
	// load calibration file...
  CLog::Log("loading T:\\calibration.xml");
	if (!LoadCalibration("T:\\calibration.xml"))
	{
		CLog::Log("Unable to load T:\\calibration.xml");
	}

	// load xml file...

  CLog::Log("loading Q:\\XboxMediaCenter.xml");
	CStdString strXMLFile = "Q:\\XboxMediaCenter.xml";
	TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( strXMLFile.c_str() ) ) 
	{
    CLog::Log("unable to load:%s",strXMLFile.c_str());
		return false;
	}

	TiXmlElement* pRootElement =xmlDoc.RootElement();
  CStdString strValue=pRootElement->Value();
	if ( strValue != "xboxmediacenter") 
  {
    CLog::Log("%s doesnt start with <xboxmediacenter>",strXMLFile.c_str());
    return false;
  }

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
			GetInteger(pRemoteDelays, "move", g_stSettings.m_iMoveDelayIR,220,1,INT_MAX);
			GetInteger(pRemoteDelays, "repeat", g_stSettings.m_iRepeatDelayIR,220,1,INT_MAX);
		}

		if (pControllerDelays)
		{
			GetInteger(pControllerDelays, "move", g_stSettings.m_iMoveDelayController,220,1,INT_MAX);
			GetInteger(pControllerDelays, "repeat", g_stSettings.m_iRepeatDelayController,220,1,INT_MAX);
		}
	}

  //GetString(pRootElement, "skin", g_stSettings.szDefaultSkin,"MediaCenter");
	GetString(pRootElement, "home", g_stSettings.szHomeDir, "");
	GetString(pRootElement, "dashboard", g_stSettings.szDashboard,"C:\\xboxdash.xbe");
	
	GetString(pRootElement, "CDDBIpAdres", g_stSettings.m_szCDDBIpAdres,"194.97.4.18");
	if (g_stSettings.m_szCDDBIpAdres == "194.97.4.18")
		GetString(pRootElement, "CDDBIpAddress", g_stSettings.m_szCDDBIpAdres,"194.97.4.18");
	//g_stSettings.m_bUseCDDB=GetBoolean(pRootElement, "CDDBEnabled");

	GetString(pRootElement, "ipadres", g_stSettings.m_strLocalIPAdres,"");
	if (g_stSettings.m_strLocalIPAdres == "")
		GetString(pRootElement, "ip", g_stSettings.m_strLocalIPAdres,"");
	GetString(pRootElement, "netmask", g_stSettings.m_strLocalNetmask,"");
	GetString(pRootElement, "defaultgateway", g_stSettings.m_strGateway,"");
	GetString(pRootElement, "nameserver", g_stSettings.m_strNameServer,"");
	GetString(pRootElement, "timeserver", g_stSettings.m_strTimeServer,"207.46.248.43");
  GetString(pRootElement, "thumbnails",g_stSettings.szThumbnailsDirectory,"");
  GetString(pRootElement, "shortcuts", g_stSettings.m_szShortcutDirectory,"");
	GetString(pRootElement, "recordings", g_stSettings.m_szMusicRecordingDirectory,"");
	GetString(pRootElement, "httpproxy", g_stSettings.m_szHTTPProxy,"");
	
	GetString(pRootElement, "albums", g_stSettings.m_szAlbumDirectory,"");
  GetString(pRootElement, "subtitles", g_stSettings.m_szAlternateSubtitleDirectory,"");

	GetString(pRootElement, "pictureextensions", g_stSettings.m_szMyPicturesExtensions,".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");
  
	GetString(pRootElement, "musicextensions", g_stSettings.m_szMyMusicExtensions,".ac3|.aac|.pls|.rm|.sc|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u");
	GetString(pRootElement, "videoextensions", g_stSettings.m_szMyVideoExtensions,".nfo|.rm|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli");

	GetInteger(pRootElement, "startwindow", g_stSettings.m_iStartupWindow,0,0,INT_MAX);
	GetInteger(pRootElement, "httpproxyport", g_stSettings.m_iHTTPProxyPort,0,0,INT_MAX);

	GetBoolean(pRootElement, "useFDrive", g_stSettings.m_bUseFDrive);
	GetBoolean(pRootElement, "useGDrive", g_stSettings.m_bUseGDrive);

  GetString(pRootElement, "dvdplayer", g_stSettings.m_szExternalDVDPlayer,"");

  CStdString strDir;
  strDir=g_stSettings.m_szShortcutDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szShortcutDirectory, strDir.c_str() );

  strDir=g_stSettings.szThumbnailsDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.szThumbnailsDirectory, strDir.c_str() );


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
	if (strDefault.size())
		strcpy( g_stSettings.m_szDefaultPrograms, strDefault.c_str());

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
	TiXmlElement* pWeatherElement =pRootElement->FirstChildElement("weather");	//grab WEATHER settings
	if (pWeatherElement)
	{
		GetString(pWeatherElement, "areacode", g_stSettings.m_szWeatherArea, "UKXX0085");
		GetString(pWeatherElement, "unitT", g_stSettings.m_szWeatherFTemp, "C");
		//do some validation
		if(g_stSettings.m_szWeatherFTemp[0] != 'F' && g_stSettings.m_szWeatherFTemp[0] != 'C')
			g_stSettings.m_szWeatherFTemp[0] = 'C';

		GetString(pWeatherElement, "unitS", g_stSettings.m_szWeatherFSpeed, "K");
		if(g_stSettings.m_szWeatherFSpeed[0] != 'M' && g_stSettings.m_szWeatherFSpeed[0] != 'K')
			g_stSettings.m_szWeatherFSpeed[0] = 'K';

		GetInteger(pWeatherElement, "refresh", g_stSettings.m_iWeatherRefresh,1,1,60);
	}

  return true;
}

void CSettings::ConvertHomeVar(CStdString& strText)
{
	// Replaces first occurence of $HOME with the home directory.
	// "$HOME\bookmarks" becomes for instance "e:\apps\xbmp\bookmarks"

	char szText[1024];
  char szTemp[1024];
  char *pReplace,*pReplace2;

	CStdString strHomePath = "Q:";
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
					CStdString strPath=share.strPath;
					strPath.ToUpper();
					if (strPath.Left(4)=="UDF:")
					{
						share.m_iDriveType=SHARE_TYPE_VIRTUAL_DVD;
						share.strPath="D:\\";
					}
					else if (CUtil::IsISO9660(share.strPath))
						share.m_iDriveType=SHARE_TYPE_VIRTUAL_DVD;
					else if (CUtil::IsDVD(share.strPath))
						share.m_iDriveType = SHARE_TYPE_DVD;
					else if (CUtil::IsRemote(share.strPath))
						share.m_iDriveType = SHARE_TYPE_REMOTE;
					else if (CUtil::IsHD(share.strPath))
						share.m_iDriveType = SHARE_TYPE_LOCAL;
					else
						share.m_iDriveType = SHARE_TYPE_UNKNOWN;

          
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

void CSettings::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue, const int iDefault, const int iMin, const int iMax)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		iValue = atoi( pChild->FirstChild()->Value() );
		if ((iValue<iMin) || (iValue>iMax)) iValue=iDefault;
	} 
	else
		iValue=iDefault;
}

void CSettings::GetFloat(const TiXmlElement* pRootElement, const CStdString& strTagName, float& fValue, const float fDefault, const float fMin, const float fMax)
{
	const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
	if (pChild)
	{
		fValue = (float)atof( pChild->FirstChild()->Value() );
		if ((fValue<fMin) || (fValue>fMax)) fValue=fDefault;
	} 
	else
		fValue=fDefault;
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
		int iRes;
		GetInteger(pResolution, "id", iRes, (int)PAL_4x3, HDTV_1080i, PAL60_16x9); //PAL4x3 as default data
		GetString(pResolution, "description", m_ResInfo[iRes].strMode, m_ResInfo[iRes].strMode);
		GetInteger(pResolution, "width", m_ResInfo[iRes].iWidth,720,640,10000);
		GetInteger(pResolution, "height", m_ResInfo[iRes].iHeight,576,400,10000);
		GetInteger(pResolution, "subtitles", m_ResInfo[iRes].iSubtitles,576,m_ResInfo[iRes].iHeight-128,m_ResInfo[iRes].iHeight);
		GetInteger(pResolution, "flags", (int &)m_ResInfo[iRes].dwFlags,0,INT_MIN,INT_MAX);
		GetFloat(pResolution, "pixelratio", m_ResInfo[iRes].fPixelRatio,128.0f/117.0f,0.5f,2.0f);

		// get the overscan info		
		TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
		if (pOverscan)
		{
			GetInteger(pOverscan, "left", m_ResInfo[iRes].Overscan.left,0,0,128);
			GetInteger(pOverscan, "top", m_ResInfo[iRes].Overscan.top,0,0,128);
			GetInteger(pOverscan, "width", m_ResInfo[iRes].Overscan.width,720,640,10000);
			GetInteger(pOverscan, "height", m_ResInfo[iRes].Overscan.height,576,400,10000);
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
    CLog::Log("Unable to load:%s",strSettingsFile.c_str());
		return false;
	}
	TiXmlElement *pRootElement = xmlDoc.RootElement();
	if (CUtil::cmpnocase(pRootElement->Value(),"settings")!=0)
	{
    CLog::Log("%s doesnt contain <settings>",strSettingsFile.c_str());
		return false;
	}
	// mypictures
	TiXmlElement *pElement = pRootElement->FirstChildElement("mypictures");
	if (pElement)
	{
		GetInteger(pElement, "picturesviewicons", g_stSettings.m_iMyPicturesViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "picturesrooticons", g_stSettings.m_iMyPicturesRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "picturessortmethod",g_stSettings.m_iMyPicturesSortMethod,0,0,2);
		GetBoolean(pElement, "picturessortascending", g_stSettings.m_bMyPicturesSortAscending);
	}
	// myfiles
	pElement = pRootElement->FirstChildElement("myfiles");
	if (pElement)
	{
		TiXmlElement *pChild = pElement->FirstChildElement("source");
		if (pChild)
		{
			GetBoolean(pChild, "srcfilesviewicons", g_stSettings.m_bMyFilesSourceViewAsIcons);
			GetBoolean(pChild, "srcfilesrooticons", g_stSettings.m_bMyFilesSourceRootViewAsIcons);
		}
		pChild = pElement->FirstChildElement("dest");
		if (pChild)
		{
			GetBoolean(pChild, "dstfilesviewicons", g_stSettings.m_bMyFilesDestViewAsIcons);
			GetBoolean(pChild, "dstfilesrooticons", g_stSettings.m_bMyFilesDestRootViewAsIcons);
		}
		GetInteger(pElement, "filessortmethod",g_stSettings.m_iMyFilesSortMethod,0,0,2);
		GetBoolean(pElement, "filessortascending", g_stSettings.m_bMyFilesSortAscending);
	}

  // mymusic settings
	pElement = pRootElement->FirstChildElement("mymusic");
	if (pElement)
	{
		TiXmlElement *pChild = pElement->FirstChildElement("songs");
		if (pChild)
		{
			GetInteger(pChild, "songsviewicons", g_stSettings.m_iMyMusicSongsViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "songsrooticons", g_stSettings.m_iMyMusicSongsRootViewAsIcons,VIEW_AS_ICONS,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "songssortmethod",g_stSettings.m_iMyMusicSongsSortMethod,0,0,2);
			GetInteger(pChild, "songssortmethodroot",g_stSettings.m_iMyMusicSongsRootSortMethod,0,0,2);
			GetBoolean(pChild, "songssortascending",g_stSettings.m_bMyMusicSongsSortAscending);
			GetBoolean(pChild, "songssortascendingroot",g_stSettings.m_bMyMusicSongsRootSortAscending);
		}
		pChild = pElement->FirstChildElement("album");
		if (pChild)
		{
			GetInteger(pChild, "albumviewicons", g_stSettings.m_iMyMusicAlbumViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_ICONS);
			GetInteger(pChild, "albumrooticons", g_stSettings.m_iMyMusicAlbumRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_ICONS);
			GetInteger(pChild, "albumsortmethod",g_stSettings.m_iMyMusicAlbumSortMethod,3,3,5);
			GetInteger(pChild, "albumsortmethodroot",g_stSettings.m_iMyMusicAlbumRootSortMethod,7,6,7);
			GetBoolean(pChild, "albumsortascending",g_stSettings.m_bMyMusicAlbumSortAscending);
			GetBoolean(pChild, "albumsortascendingroot",g_stSettings.m_bMyMusicAlbumRootSortAscending);
			GetBoolean(pChild, "albumshowrecentalbums",g_stSettings.m_bMyMusicAlbumShowRecent);
		}
		pChild = pElement->FirstChildElement("artist");
		if (pChild)
		{
			GetInteger(pChild, "artistviewicons", g_stSettings.m_iMyMusicArtistsViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "artistrooticons", g_stSettings.m_iMyMusicArtistsRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "artistsortmethod",g_stSettings.m_iMyMusicArtistsSortMethod,4,5,5); //title
			GetInteger(pChild, "artistsortmethodroot",g_stSettings.m_iMyMusicArtistsRootSortMethod,0,0,0); //Only name (??)
			GetBoolean(pChild, "artistsortascending",g_stSettings.m_bMyMusicArtistsSortAscending);
			GetBoolean(pChild, "artistsortascendingroot",g_stSettings.m_bMyMusicArtistsRootSortAscending);
		}
		pChild = pElement->FirstChildElement("genre");
		if (pChild)
		{
			GetInteger(pChild, "genreviewicons", g_stSettings.m_iMyMusicGenresViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "genrerooticons", g_stSettings.m_iMyMusicGenresRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
			GetInteger(pChild, "genresortmethod",g_stSettings.m_iMyMusicGenresSortMethod,5,4,5);	//	titel
			GetInteger(pChild, "genresortmethodroot",g_stSettings.m_iMyMusicGenresRootSortMethod,0,0,0); // Only name (??)
			GetBoolean(pChild, "genresortascending",g_stSettings.m_bMyMusicGenresSortAscending);
			GetBoolean(pChild, "genresortascendingroot",g_stSettings.m_bMyMusicGenresRootSortAscending);
		}
		pChild = pElement->FirstChildElement("top100");
		if (pChild)
		{
			GetInteger(pChild, "top100rooticons", g_stSettings.m_iMyMusicTop100ViewAsIcons,VIEW_AS_LIST,VIEW_AS_ICONS,VIEW_AS_ICONS);
		}
		pChild = pElement->FirstChildElement("playlist");
		if (pChild)
		{
			GetInteger(pChild, "playlistrooticons", g_stSettings.m_iMyMusicPlaylistViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		}
		GetInteger(pElement, "startwindow",g_stSettings.m_iMyMusicStartWindow,WINDOW_MUSIC_FILES,WINDOW_MUSIC_FILES,WINDOW_MUSIC_TOP100);//501; view songs
	}
	// myvideos settings
	pElement = pRootElement->FirstChildElement("myvideos");
	if (pElement)
	{ 
    GetInteger(pElement, "startwindow",g_stSettings.m_iVideoStartWindow,0,0,INT_MAX);
    GetBoolean(pElement, "stackvideo", g_stSettings.m_bMyVideoVideoStack);
    GetBoolean(pElement, "stackgenre", g_stSettings.m_bMyVideoGenreStack);
    GetBoolean(pElement, "stackactor", g_stSettings.m_bMyVideoActorStack);
    GetBoolean(pElement, "stackyear", g_stSettings.m_bMyVideoYearStack);
    
		GetInteger(pElement, "videoviewicons", g_stSettings.m_iMyVideoViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "videorooticons", g_stSettings.m_iMyVideoRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "videosortmethod",g_stSettings.m_iMyVideoSortMethod,0,0,2);
		GetBoolean(pElement, "videosortascending", g_stSettings.m_bMyVideoSortAscending);

		GetInteger(pElement, "genreviewicons", g_stSettings.m_iMyVideoGenreViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "genrerooticons", g_stSettings.m_iMyVideoGenreRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "genresortmethod",g_stSettings.m_iMyVideoGenreSortMethod,0,0,2);
		GetBoolean(pElement, "genresortascending", g_stSettings.m_bMyVideoGenreSortAscending);

		GetInteger(pElement, "actorviewicons", g_stSettings.m_iMyVideoActorViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "actorrooticons", g_stSettings.m_iMyVideoActorRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "actorsortmethod",g_stSettings.m_iMyVideoActorSortMethod,0,0,2);
		GetBoolean(pElement, "actorsortascending", g_stSettings.m_bMyVideoActorSortAscending);
    
		GetInteger(pElement, "yearviewicons", g_stSettings.m_iMyVideoYearViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "yearrooticons", g_stSettings.m_iMyVideoYearRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "yearsortmethod",g_stSettings.m_iMyVideoYearSortMethod,0,0,2);
		GetBoolean(pElement, "yearsortascending", g_stSettings.m_bMyVideoYearSortAscending);

    GetInteger(pElement, "titleviewicons", g_stSettings.m_iMyVideoTitleViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "titlerooticons", g_stSettings.m_iMyVideoTitleRootViewAsIcons,VIEW_AS_LIST,VIEW_AS_LIST,VIEW_AS_LARGEICONS);
		GetInteger(pElement, "titlesortmethod",g_stSettings.m_iMyVideoTitleSortMethod,0,0,3);
		GetBoolean(pElement, "titlesortascending", g_stSettings.m_bMyVideoTitleSortAscending);

		GetBoolean(pElement, "postprocessing", g_stSettings.m_bPostProcessing);
		GetBoolean(pElement, "deinterlace", g_stSettings.m_bDeInterlace);
      
		GetInteger(pElement, "subtitleheight",g_stSettings.m_iSubtitleHeight,28,1,128);
		GetString(pElement, "subtitlefont", g_stSettings.m_szSubtitleFont,"arial-iso-8859-1");
	}
	// myscripts settings
	pElement = pRootElement->FirstChildElement("myscripts");
	if (pElement)
	{
		GetBoolean(pElement, "scriptsviewicons", g_stSettings.m_bScriptsViewAsIcons);
		GetBoolean(pElement, "scriptsrooticons", g_stSettings.m_bScriptsRootViewAsIcons);
		GetInteger(pElement, "scriptssortmethod",g_stSettings.m_iScriptsSortMethod,0,0,2);
		GetBoolean(pElement, "scriptssortascending", g_stSettings.m_bScriptsSortAscending);
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
		GetInteger(pElement, "hdspindowntime", g_stSettings.m_iHDSpinDownTime,5,0,INT_MAX);
		GetBoolean(pElement, "autorundvd", g_stSettings.m_bAutorunDVD);
		GetBoolean(pElement, "autorunvcd", g_stSettings.m_bAutorunVCD);
		GetBoolean(pElement, "autoruncdda", g_stSettings.m_bAutorunCdda);
		GetBoolean(pElement, "autorunxbox", g_stSettings.m_bAutorunXbox);
		GetBoolean(pElement, "autorunmusic", g_stSettings.m_bAutorunMusic);
		GetBoolean(pElement, "autorunvideo", g_stSettings.m_bAutorunVideo);
		GetBoolean(pElement, "autorunpictures", g_stSettings.m_bAutorunPictures);
		GetString(pElement, "language", g_stSettings.szDefaultLanguage, g_stSettings.szDefaultLanguage);
		GetInteger(pElement, "shutdowntime", g_stSettings.m_iShutdownTime,0,0,INT_MAX);
		GetInteger(pElement, "screensavertime", g_stSettings.m_iScreenSaverTime,3,1,INT_MAX);	// CB: SCREENSAVER PATCH
		GetInteger(pElement, "screensavermode", g_stSettings.m_iScreenSaverMode,1,0,3);	// 0=Off, 1=Fade to dim, 2=Fade to black, 3=Matrix Trails
		GetInteger(pElement, "screensaverfade", g_stSettings.m_iScreenSaverFadeLevel,20,1,100);	// default to 20%
		GetInteger(pElement, "audiostream",g_stSettings.m_iAudioStream,0,0,INT_MAX);
	}
	// slideshow settings
	pElement = pRootElement->FirstChildElement("slideshow");
	if (pElement)
	{
		GetInteger(pElement, "transistionframes", g_stSettings.m_iSlideShowTransistionFrames,25,1,INT_MAX);
		GetInteger(pElement, "staytime", g_stSettings.m_iSlideShowStayTime,3000,500,INT_MAX);
	}
	// screen settings
	pElement = pRootElement->FirstChildElement("screen");
	if (pElement)
	{
		GetInteger(pElement, "resolution",(int &)g_stSettings.m_ScreenResolution,(int)PAL_4x3,(int)HDTV_1080i,(int)PAL60_16x9);
		GetInteger(pElement, "uioffsetx",g_stSettings.m_iUIOffsetX,0,INT_MIN,INT_MAX);
		GetInteger(pElement, "uioffsety",g_stSettings.m_iUIOffsetY,0,INT_MIN,INT_MAX);
		GetBoolean(pElement, "soften", g_stSettings.m_bSoften);
    GetBoolean(pElement, "framerateconversion", g_stSettings.m_bFrameRateConversions);
		GetBoolean(pElement, "zoom", g_stSettings.m_bZoom);
		GetBoolean(pElement, "stretch", g_stSettings.m_bStretch);
		GetBoolean(pElement, "allowswitching", g_stSettings.m_bAllowVideoSwitching);
		GetBoolean(pElement, "allowpal60", g_stSettings.m_bAllowPAL60);
		GetInteger(pElement, "minfilter", (int &)g_stSettings.m_minFilter,D3DTEXF_LINEAR,INT_MIN,INT_MAX);
		GetInteger(pElement, "maxfilter", (int &)g_stSettings.m_maxFilter,D3DTEXF_LINEAR,INT_MIN,INT_MAX);
	}
	// audio settings
	pElement = pRootElement->FirstChildElement("audio");
	if (pElement)
	{
		GetBoolean(pElement, "audioonallspeakers", g_stSettings.m_bAudioOnAllSpeakers);
		GetInteger(pElement, "channels",g_stSettings.m_iChannels,2,0,INT_MAX);
		GetBoolean(pElement, "ac3passthru51", g_stSettings.m_bDD_DTSMultiChannelPassThrough);
		GetBoolean(pElement, "ac3passthru21", g_stSettings.m_bDDStereoPassThrough);
		GetBoolean(pElement, "useid3", g_stSettings.m_bUseID3);
		GetString(pElement, "visualisation", g_stSettings.szDefaultVisualisation, g_stSettings.szDefaultVisualisation);
		GetBoolean(pElement, "autoshuffleplaylist", g_stSettings.m_bAutoShufflePlaylist);
    GetFloat(pElement, "volumeamp", g_stSettings.m_fVolumeAmplification,0.0f,-200.0f,60.0f);

    GetBoolean(pElement, "UseDigitalOutput", g_stSettings.m_bUseDigitalOutput);
	}

  // post processing
  pElement = pRootElement->FirstChildElement("PostProcessing");
	if (pElement)
	{
	  GetBoolean(pElement, "PPAuto", g_stSettings.m_bPPAuto);
	  GetBoolean(pElement, "PPVertical", g_stSettings.m_bPPVertical);
	  GetBoolean(pElement, "PPHorizontal", g_stSettings.m_bPPHorizontal);
	  GetBoolean(pElement, "PPAutoLevels", g_stSettings.m_bPPAutoLevels);
	  GetBoolean(pElement, "PPdering", g_stSettings.m_bPPdering);
	  GetInteger(pElement, "PPHorizontalVal",g_stSettings.m_iPPHorizontal,0,INT_MIN,INT_MAX);
	  GetInteger(pElement, "PPVerticalVal",g_stSettings.m_iPPVertical,0,INT_MIN,INT_MAX);

  g_stSettings.m_iPPHorizontal=0;
  g_stSettings.m_iPPVertical=0;
	
}

  // my programs
  pElement = pRootElement->FirstChildElement("myprograms");
  if (pElement)
  {
	GetBoolean(pElement, "programsviewicons", g_stSettings.m_bMyProgramsViewAsIcons);
	GetInteger(pElement, "programssortmethod", g_stSettings.m_iMyProgramsSortMethod,0,0,2);
	GetBoolean(pElement, "programssortascending", g_stSettings.m_bMyProgramsSortAscending);
	GetInteger(pElement, "selecteditem",g_stSettings.m_iMyProgramsSelectedItem,0,0,INT_MAX);

	  GetBoolean(pElement, "flatten", g_stSettings.m_bMyProgramsFlatten);
   	GetBoolean(pElement, "defaultxbe", g_stSettings.m_bMyProgramsDefaultXBE);
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
	SetBoolean(pNode, "programsviewicons", g_stSettings.m_bMyProgramsViewAsIcons);
	SetInteger(pNode, "programssortmethod", g_stSettings.m_iMyProgramsSortMethod);
	SetBoolean(pNode, "programssortascending", g_stSettings.m_bMyProgramsSortAscending);
	SetBoolean(pNode, "flatten", g_stSettings.m_bMyProgramsFlatten);
	SetBoolean(pNode, "defaultxbe", g_stSettings.m_bMyProgramsDefaultXBE);
  SetInteger(pNode, "selecteditem",g_stSettings.m_iMyProgramsSelectedItem);

	// mypictures settings
	TiXmlElement picturesNode("mypictures");
	pNode = pRoot->InsertEndChild(picturesNode);
	if (!pNode) return false;
	SetInteger(pNode, "picturesviewicons", g_stSettings.m_iMyPicturesViewAsIcons);
	SetInteger(pNode, "picturesrooticons", g_stSettings.m_iMyPicturesRootViewAsIcons);
	SetInteger(pNode, "picturessortmethod",g_stSettings.m_iMyPicturesSortMethod);
	SetBoolean(pNode, "picturessortascending", g_stSettings.m_bMyPicturesSortAscending);

	// myfiles settings
	TiXmlElement filesNode("myfiles");
	pNode = pRoot->InsertEndChild(filesNode);
	if (!pNode) return false;
	{
		TiXmlElement childNode("source");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "srcfilesviewicons", g_stSettings.m_bMyFilesSourceViewAsIcons);
		SetBoolean(pChild, "srcfilesrooticons", g_stSettings.m_bMyFilesSourceRootViewAsIcons);
	}
	{
		TiXmlElement childNode("dest");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetBoolean(pChild, "dstfilesviewicons", g_stSettings.m_bMyFilesDestViewAsIcons);
		SetBoolean(pChild, "dstfilesrooticons", g_stSettings.m_bMyFilesDestRootViewAsIcons);
	}
	SetInteger(pNode, "filessortmethod",g_stSettings.m_iMyFilesSortMethod);
	SetBoolean(pNode, "filessortascending", g_stSettings.m_bMyFilesSortAscending);

	// mymusic settings
	TiXmlElement musicNode("mymusic");
	pNode = pRoot->InsertEndChild(musicNode);
	if (!pNode) return false;
	{
		TiXmlElement childNode("songs");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "songsviewicons", g_stSettings.m_iMyMusicSongsViewAsIcons);
		SetInteger(pChild, "songsrooticons", g_stSettings.m_iMyMusicSongsRootViewAsIcons);
		SetInteger(pChild, "songssortmethod",g_stSettings.m_iMyMusicSongsSortMethod);
		SetInteger(pChild, "songssortmethodroot",g_stSettings.m_iMyMusicSongsRootSortMethod);
		SetBoolean(pChild, "songssortascending",g_stSettings.m_bMyMusicSongsSortAscending);
		SetBoolean(pChild, "songssortascendingroot",g_stSettings.m_bMyMusicSongsRootSortAscending);
	}
	{
		TiXmlElement childNode("album");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "albumviewicons", g_stSettings.m_iMyMusicAlbumViewAsIcons);
		SetInteger(pChild, "albumrooticons", g_stSettings.m_iMyMusicAlbumRootViewAsIcons);
		SetInteger(pChild, "albumsortmethod",g_stSettings.m_iMyMusicAlbumSortMethod);
		SetInteger(pChild, "albumsortmethodroot",g_stSettings.m_iMyMusicAlbumRootSortMethod);
		SetBoolean(pChild, "albumsortascending",g_stSettings.m_bMyMusicAlbumSortAscending);
		SetBoolean(pChild, "albumsortascendingroot",g_stSettings.m_bMyMusicAlbumRootSortAscending);
		SetBoolean(pChild, "albumshowrecentalbums",g_stSettings.m_bMyMusicAlbumShowRecent);
	}
	{
		TiXmlElement childNode("artist");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "artistviewicons", g_stSettings.m_iMyMusicArtistsViewAsIcons);
		SetInteger(pChild, "artistrooticons", g_stSettings.m_iMyMusicArtistsRootViewAsIcons);
		SetInteger(pChild, "artistsortmethod",g_stSettings.m_iMyMusicArtistsSortMethod);
		SetInteger(pChild, "artistsortmethodroot",g_stSettings.m_iMyMusicArtistsRootSortMethod);
		SetBoolean(pChild, "artistsortascending",g_stSettings.m_bMyMusicArtistsSortAscending);
		SetBoolean(pChild, "artistsortascendingroot",g_stSettings.m_bMyMusicArtistsRootSortAscending);
	}
	{
		TiXmlElement childNode("genre");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "genreviewicons", g_stSettings.m_iMyMusicGenresViewAsIcons);
		SetInteger(pChild, "genrerooticons", g_stSettings.m_iMyMusicGenresRootViewAsIcons);
		SetInteger(pChild, "genresortmethod",g_stSettings.m_iMyMusicGenresSortMethod);
		SetInteger(pChild, "genresortmethodroot",g_stSettings.m_iMyMusicGenresRootSortMethod);
		SetBoolean(pChild, "genresortascending",g_stSettings.m_bMyMusicGenresSortAscending);
		SetBoolean(pChild, "genresortascendingroot",g_stSettings.m_bMyMusicGenresRootSortAscending);
	}
	{
		TiXmlElement childNode("top100");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "top100rooticons", g_stSettings.m_iMyMusicTop100ViewAsIcons);
	}
	{
		TiXmlElement childNode("playlist");
		TiXmlNode *pChild = pNode->InsertEndChild(childNode);
		if (!pChild) return false;
		SetInteger(pChild, "playlistrooticons", g_stSettings.m_iMyMusicPlaylistViewAsIcons);
	}

	SetInteger(pNode, "startwindow",g_stSettings.m_iMyMusicStartWindow);

	// myvideos settings
	TiXmlElement videosNode("myvideos");
	pNode = pRoot->InsertEndChild(videosNode);
	if (!pNode) return false;

  SetInteger(pNode, "startwindow",g_stSettings.m_iVideoStartWindow);
  SetBoolean(pNode, "stackvideo", g_stSettings.m_bMyVideoVideoStack);
  SetBoolean(pNode, "stackgenre", g_stSettings.m_bMyVideoGenreStack);
  SetBoolean(pNode, "stackactor", g_stSettings.m_bMyVideoActorStack);
  SetBoolean(pNode, "stackyear", g_stSettings.m_bMyVideoYearStack);

	SetInteger(pNode, "videoviewicons", g_stSettings.m_iMyVideoViewAsIcons);
	SetInteger(pNode, "videorooticons", g_stSettings.m_iMyVideoRootViewAsIcons);
	SetInteger(pNode, "videosortmethod",g_stSettings.m_iMyVideoSortMethod);
	SetBoolean(pNode, "videosortascending", g_stSettings.m_bMyVideoSortAscending);
  
	SetInteger(pNode, "genreviewicons", g_stSettings.m_iMyVideoGenreViewAsIcons);
	SetInteger(pNode, "genrerooticons", g_stSettings.m_iMyVideoGenreRootViewAsIcons);
	SetInteger(pNode, "genresortmethod",g_stSettings.m_iMyVideoGenreSortMethod);
	SetBoolean(pNode, "genresortascending", g_stSettings.m_bMyVideoGenreSortAscending);

	SetInteger(pNode, "actorviewicons", g_stSettings.m_iMyVideoActorViewAsIcons);
	SetInteger(pNode, "actorrooticons", g_stSettings.m_iMyVideoActorRootViewAsIcons);
	SetInteger(pNode, "actorsortmethod",g_stSettings.m_iMyVideoActorSortMethod);
	SetBoolean(pNode, "actorsortascending", g_stSettings.m_bMyVideoActorSortAscending);

	SetInteger(pNode, "yearviewicons", g_stSettings.m_iMyVideoYearViewAsIcons);
	SetInteger(pNode, "yearrooticons", g_stSettings.m_iMyVideoYearRootViewAsIcons);
	SetInteger(pNode, "yearsortmethod",g_stSettings.m_iMyVideoYearSortMethod);
	SetBoolean(pNode, "yearsortascending", g_stSettings.m_bMyVideoYearSortAscending);

	SetInteger(pNode, "titleviewicons", g_stSettings.m_iMyVideoTitleViewAsIcons);
	SetInteger(pNode, "titlerooticons", g_stSettings.m_iMyVideoTitleRootViewAsIcons);
	SetInteger(pNode, "titlesortmethod",g_stSettings.m_iMyVideoTitleSortMethod);
	SetBoolean(pNode, "titlesortascending", g_stSettings.m_bMyVideoTitleSortAscending);

	SetBoolean(pNode, "postprocessing", g_stSettings.m_bPostProcessing);
	SetBoolean(pNode, "deinterlace", g_stSettings.m_bDeInterlace);

	SetInteger(pNode, "subtitleheight",g_stSettings.m_iSubtitleHeight);
	SetString(pNode, "subtitlefont", g_stSettings.m_szSubtitleFont);

	// myscripts settings
	TiXmlElement scriptsNode("myscripts");
	pNode = pRoot->InsertEndChild(scriptsNode);
	if (!pNode) return false;
	SetBoolean(pNode, "scriptsviewicons", g_stSettings.m_bScriptsViewAsIcons);
	SetBoolean(pNode, "scriptsrooticons", g_stSettings.m_bScriptsRootViewAsIcons);
	SetInteger(pNode, "scriptssortmethod",g_stSettings.m_iScriptsSortMethod);
	SetBoolean(pNode, "scriptssortascending", g_stSettings.m_bScriptsSortAscending);

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
	SetInteger(pNode, "shutdowntime", g_stSettings.m_iShutdownTime);
	SetInteger(pNode, "screensavertime", g_stSettings.m_iScreenSaverTime);	// CB: SCREENSAVER PATCH
	SetInteger(pNode, "screensavermode", g_stSettings.m_iScreenSaverMode);	// CB: SCREENSAVER PATCH
	SetInteger(pNode, "screensaverfade", g_stSettings.m_iScreenSaverFadeLevel);
	SetInteger(pNode, "audiostream",g_stSettings.m_iAudioStream);

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
  SetBoolean(pNode, "framerateconversion", g_stSettings.m_bFrameRateConversions);
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
	SetBoolean(pNode, "ac3passthru51", g_stSettings.m_bDD_DTSMultiChannelPassThrough);
	SetBoolean(pNode, "ac3passthru21", g_stSettings.m_bDDStereoPassThrough);
	SetBoolean(pNode, "useid3", g_stSettings.m_bUseID3);
	SetString(pNode, "visualisation", g_stSettings.szDefaultVisualisation);
  SetBoolean(pNode, "autoshuffleplaylist", g_stSettings.m_bAutoShufflePlaylist);
  SetFloat(pNode, "volumeamp", g_stSettings.m_fVolumeAmplification);
  SetBoolean(pNode, "UseDigitalOutput", g_stSettings.m_bUseDigitalOutput);

	TiXmlElement postprocNode("PostProcessing");
	pNode = pRoot->InsertEndChild(postprocNode);
	if (!pNode) return false;
	SetBoolean(pNode, "PPAuto", g_stSettings.m_bPPAuto);
	SetBoolean(pNode, "PPVertical", g_stSettings.m_bPPVertical);
	SetBoolean(pNode, "PPHorizontal", g_stSettings.m_bPPHorizontal);
	SetBoolean(pNode, "PPAutoLevels", g_stSettings.m_bPPAutoLevels);
	SetBoolean(pNode, "PPdering", g_stSettings.m_bPPdering);
	SetInteger(pNode, "PPHorizontalVal",g_stSettings.m_iPPHorizontal);
	SetInteger(pNode, "PPVerticalVal",g_stSettings.m_iPPVertical);


  
	// save the file
	return xmlDoc.SaveFile(strSettingsFile);
}
