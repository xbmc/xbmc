#include "stdafx.h"
#include "settings.h"
#include "application.h"
#include "util.h"
#include "GUIWindowMusicBase.h"
#include "utils/FanController.h"
#include "LangCodeExpander.h"
#include "../xbmc/ButtonTranslator.h"

class CSettings g_settings;
struct CSettings::stSettings g_stSettings;

extern CStdString g_LoadErrorStr;

CSettings::CSettings(void)
{
  for (int i = HDTV_1080i; i <= PAL60_16x9; i++)
  {
    g_graphicsContext.ResetScreenParameters((RESOLUTION)i);
    g_graphicsContext.ResetOverscan((RESOLUTION)i, m_ResInfo[i].GUIOverscan);
  }

  strcpy(g_stSettings.m_szExternalDVDPlayer, "");
  strcpy(g_stSettings.m_szExternalCDDAPlayer, "");

  g_stSettings.m_iMyVideoStack = STACK_NONE;

  g_settings.m_MyVideoStackRegExps.push_back("[ _\\.-]*part[ \\.]*([0-9])*");
  g_settings.m_MyVideoStackRegExps.push_back("[ _\\.-]*cd[ \\.]*([0-9])*");

  g_stSettings.m_bMyVideoCleanTitles = false;
  strcpy(g_stSettings.m_szMyVideoCleanTokens, "divx|xvid|3ivx|ac3|ac351|dts|mp3|wma|m4a|mp4|aac|ogg|scr|ts|sharereactor|dvd|dvdrip");
  strcpy(g_stSettings.m_szMyVideoCleanSeparators, "- _.[({+");

  StringUtils::SplitString(g_stSettings.m_szMyVideoCleanTokens, "|", g_settings.m_szMyVideoCleanTokensArray);
  g_settings.m_szMyVideoCleanSeparatorsString = g_stSettings.m_szMyVideoCleanSeparators;

  for (int i = 0; i < (int)g_settings.m_szMyVideoCleanTokensArray.size(); i++)
    g_settings.m_szMyVideoCleanTokensArray[i].MakeLower();

  g_stSettings.m_bAutoDetectFG = true;
  g_stSettings.m_bUseFDrive = true;
  g_stSettings.m_bUseGDrive = false;
  g_stSettings.m_bUsePCDVDROM = false;
  g_stSettings.m_bDetectAsIso = false;
  g_stSettings.dwFileVersion = CONFIG_VERSION;
  g_stSettings.m_MyProgramsViewMethod = VIEW_METHOD_ICONS;
  g_stSettings.m_MyProgramsSortOrder = SORT_ORDER_ASC;
  strcpy(g_stSettings.szDashboard, "C:\\xboxdash.xbe");
  strcpy(g_stSettings.m_szAlternateSubtitleDirectory, "");
  strcpy(g_stSettings.szOnlineArenaPassword, "");
  strcpy(g_stSettings.szOnlineArenaDescription, "It's Good To Play Together!");
  strcpy(g_stSettings.szHomeDir, "");

  strcpy(g_stSettings.m_szMyPicturesExtensions, ".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");
  strcpy(g_stSettings.m_szMyMusicExtensions, ".ac3|.aac|.pls|.strm|.rm|.sc|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u");
  strcpy(g_stSettings.m_szMyVideoExtensions, ".nfo|.rm|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli");

  strcpy( g_stSettings.m_szDefaultPrograms, "");
  strcpy( g_stSettings.m_szDefaultMusic, "");
  strcpy( g_stSettings.m_szDefaultPictures, "");
  strcpy( g_stSettings.m_szDefaultFiles, "");
  strcpy( g_stSettings.m_szDefaultVideos, "");
  strcpy( g_stSettings.m_szCDDBIpAdres, "");
  strcpy( g_stSettings.m_szIMDBurl, "");
  strcpy (g_stSettings.m_szMusicRecordingDirectory, "");
  strcpy(g_stSettings.m_szCacheDirectory,"Z:\\");

  g_stSettings.m_bMyMusicSongInfoInVis = true;    // UNUSED - depreciated.
  g_stSettings.m_bMyMusicSongThumbInVis = false;  // used for music info in vis screen

  g_stSettings.m_MyMusicSongsRootSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyMusicSongsSortOrder = SORT_ORDER_ASC;

  g_stSettings.m_MyMusicNavGenresSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyMusicNavArtistsSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyMusicNavAlbumsSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyMusicNavSongsSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyMusicNavPlaylistsSortOrder = SORT_ORDER_ASC;

  // need defaults for these or the display is
  // incorrect the first time Nav window is used
  g_stSettings.m_MyMusicNavAlbumsSortMethod = SORT_METHOD_ALBUM;
  g_stSettings.m_MyMusicNavSongsSortMethod = SORT_METHOD_TRACKNUM;
  g_stSettings.m_MyMusicNavPlaylistsSortMethod = SORT_METHOD_TRACKNUM;

  g_stSettings.m_MyMusicPlaylistViewMethod = VIEW_METHOD_LIST;
  g_stSettings.m_bMyMusicPlaylistRepeat = false;
  g_stSettings.m_bMyMusicPlaylistShuffle = false;

  g_stSettings.m_MyVideoSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyVideoRootSortOrder = SORT_ORDER_ASC;

  g_stSettings.m_MyVideoGenreSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyVideoGenreRootSortOrder = SORT_ORDER_ASC;

  g_stSettings.m_MyVideoActorSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyVideoActorRootSortOrder = SORT_ORDER_ASC;

  g_stSettings.m_MyVideoYearSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyVideoYearRootSortOrder = SORT_ORDER_ASC;

  g_stSettings.m_MyVideoTitleSortOrder = SORT_ORDER_ASC;

  g_stSettings.m_MyVideoPlaylistViewMethod = VIEW_METHOD_LIST;
  g_stSettings.m_bMyVideoPlaylistRepeat = false;
  g_stSettings.m_bMyVideoPlaylistShuffle = false;

/*  g_stSettings.m_bMyFilesSourceViewMethod = false;
  g_stSettings.m_bMyFilesSourceRootViewMethod = true;
  g_stSettings.m_bMyFilesDestViewMethod = false;
  g_stSettings.m_bMyFilesDestRootViewMethod = true;
  g_stSettings.m_bMyFilesSourceSortOrder = true;
  g_stSettings.m_bMyFilesSourceRootSortOrder = true;
  g_stSettings.m_bMyFilesDestSortOrder = true;
  g_stSettings.m_bMyFilesDestRootSortOrder = true;
*/

  g_stSettings.m_MyPicturesSortOrder = SORT_ORDER_ASC;
  g_stSettings.m_MyPicturesRootSortOrder = SORT_ORDER_ASC;

  g_stSettings.m_ScriptsViewMethod = VIEW_METHOD_LIST;
  g_stSettings.m_ScriptsSortOrder = SORT_ORDER_ASC;

  g_stSettings.m_fZoomAmount = 1.0f;
  g_stSettings.m_fPixelRatio = 1.0f;

  g_stSettings.m_bDisplayRemoteCodes = false;
  g_stSettings.m_iSambaDebugLevel = 0;
  strcpy(g_stSettings.m_strSambaWorkgroup, "WORKGROUP");
  strcpy(g_stSettings.m_strSambaIPAdress, "192.168.0.5");
  strcpy(g_stSettings.m_strSambaShareName, "WORKGROUP (SMB) Network");
  strcpy(g_stSettings.m_strSambaWinsServer, "");

  g_stSettings.m_nVolumeLevel = 0;
  g_stSettings.m_iPreMuteVolumeLevel = 0;
  g_stSettings.m_bMute = false;

  g_stSettings.m_iLogLevel = LOGINFO;
  g_stSettings.m_bUnhandledExceptionToFatalError = false;
  g_stSettings.m_bShowFreeMem = false;

  m_iLastLoadedProfileIndex = -1;

  // defaults for scanning
  g_stSettings.m_bMyMusicIsScanning = false;
  g_stSettings.m_bMyMusicOldUseTags = true;
  g_stSettings.m_bMyMusicOldFindThumbs = true;

  // defaults for seeking
  g_stSettings.m_iMyVideoTimeSeekForward = 30;
  g_stSettings.m_iMyVideoTimeSeekBackward = -30;
  g_stSettings.m_iMyVideoTimeSeekForwardBig = 300;
  g_stSettings.m_iMyVideoTimeSeekBackwardBig = -300;

  g_stSettings.m_iMyVideoPercentSeekForward = 2;
  g_stSettings.m_iMyVideoPercentSeekBackward = -2;
  g_stSettings.m_iMyVideoPercentSeekForwardBig = 10;
  g_stSettings.m_iMyVideoPercentSeekBackwardBig = -10;
  xbmcXmlLoaded = false;
}

CSettings::~CSettings(void)
{
}


void CSettings::Save() const
{
  if (g_application.m_bStop)
  {
    //don't save settings when we're busy stopping the application
    //a lot of screens try to save settings on deinit and deinit is called
    //for every screen when the application is stopping.
    return ;
  }
  if (!SaveSettings("T:\\settings.xml", true))
  {
    CLog::Log(LOGERROR, "Unable to save settings to T:\\settings.xml");
  }
}

bool CSettings::QuickXMLLoad(CStdString strElement)
{
  //
  // load xml file...
  CStdString strXMLFile = "Q:\\XboxMediaCenter.xml";
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( strXMLFile.c_str() ) ) return false;
  CStdString strValue;
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement) strValue = pRootElement->Value();
  if ( strValue != "xboxmediacenter") return false;
  //
  if (strElement == "logpath" )
  {
    GetString(pRootElement, strElement, g_stSettings.m_szlogpath, "");
    if (strlen(g_stSettings.m_szlogpath) > 1) return true;
  }
  else if (strElement == "home")
  {
    GetString(pRootElement, strElement, g_stSettings.szHomeDir, "");
    if (strlen(g_stSettings.szHomeDir) > 1) return true;
  }
  return false;
  
  
  //
  
}
bool CSettings::Load(bool& bXboxMediacenter, bool& bSettings)
{
  // load settings file...
  bXboxMediacenter = bSettings = false;
  CLog::Log(LOGNOTICE, "loading T:\\settings.xml");
  if (!LoadSettings("T:\\settings.xml", true))
  {
    CLog::Log(LOGERROR, "Unable to load T:\\settings.xml, creating new T:\\settings.xml with default values");
    Save();
    if (!(bSettings = LoadSettings("T:\\settings.xml", true)))
      return false;
  }

  // load xml file...
  CLog::Log(LOGNOTICE, "loading Q:\\XboxMediaCenter.xml");
  CStdString strXMLFile = "Q:\\XboxMediaCenter.xml";
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( strXMLFile.c_str() ) )
  {
    g_LoadErrorStr.Format("%s, Line %d\n%s", strXMLFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue;
  if (pRootElement)
    strValue = pRootElement->Value();
  if ( strValue != "xboxmediacenter")
  {
    g_LoadErrorStr.Format("%s Doesn't contain <xboxmediacenter>", strXMLFile.c_str());
    return false;
  }

  GetInteger(pRootElement, "loglevel", g_stSettings.m_iLogLevel, LOGWARNING, LOGDEBUG, LOGNONE);
  GetBoolean(pRootElement, "haltonfatalerrors", g_stSettings.m_bUnhandledExceptionToFatalError);
  GetBoolean(pRootElement, "showfreemem", g_stSettings.m_bShowFreeMem);
  

  TiXmlElement* pFileTypeIcons = pRootElement->FirstChildElement("filetypeicons");
  TiXmlNode* pFileType = pFileTypeIcons->FirstChild();
  while (pFileType)
  {
    CFileTypeIcon icon;
    icon.m_strName = ".";
    icon.m_strName += pFileType->Value();
    icon.m_strIcon = pFileType->FirstChild()->Value();
    m_vecIcons.push_back(icon);
    pFileType = pFileType->NextSibling();
  }

  TiXmlElement* pMasterLockElement = pRootElement->FirstChildElement("masterlock");
  if (pMasterLockElement)
  {
    GetInteger(pMasterLockElement, "maxretry", g_stSettings.m_iMasterLockMaxRetry , 0, 0, 100);
    GetInteger(pMasterLockElement, "enableshutdown", g_stSettings.m_iMasterLockEnableShutdown , 0, 0, 1);
    GetInteger(pMasterLockElement, "protectshares", g_stSettings.m_iMasterLockProtectShares , 0, 0, 1);
    GetInteger(pMasterLockElement, "mastermode", g_stSettings.m_iMasterLockMode , 0, 0, 3);
    GetString(pMasterLockElement, "mastercode", g_stSettings.m_masterLockCode, "");
    GetInteger(pMasterLockElement, "startuplock", g_stSettings.m_iMasterLockStartupLock , 0, 0, 1);
    GetInteger(pMasterLockElement, "LockFilemanager", g_stSettings.m_iMasterLockFilemanager , 0, 0, 1);
    GetInteger(pMasterLockElement, "LockSettings", g_stSettings.m_iMasterLockSettings , 0, 0, 1);
    GetInteger(pMasterLockElement, "LockHomeMedia", g_stSettings.m_iMasterLockHomeMedia , 0, 0, 15);
  }

  TiXmlElement* pSambaElement = pRootElement->FirstChildElement("samba");
  if (pSambaElement)
  {
    GetString(pSambaElement, "workgroup", g_stSettings.m_strSambaWorkgroup, "WORKGROUP");
    GetString(pSambaElement, "smbip", g_stSettings.m_strSambaIPAdress, "192.168.0.5");
    GetString(pSambaElement, "smbsharename", g_stSettings.m_strSambaShareName, "WORKGROUP (SMB) Network");
    
    
    GetString(pSambaElement, "winsserver", g_stSettings.m_strSambaWinsServer, "");
    GetInteger(pSambaElement, "debuglevel", g_stSettings.m_iSambaDebugLevel , 0, 0, 100);
    GetString(pSambaElement, "defaultusername", g_stSettings.m_strSambaDefaultUserName, "");
    GetString(pSambaElement, "defaultpassword", g_stSettings.m_strSambaDefaultPassword, "");

  }

  TiXmlElement* pDelaysElement = pRootElement->FirstChildElement("delays");
  if (pDelaysElement)
  {
    TiXmlElement* pRemoteDelays = pDelaysElement->FirstChildElement("remote");
    TiXmlElement* pControllerDelays = pDelaysElement->FirstChildElement("controller");
    if (pRemoteDelays)
    {
      GetInteger(pRemoteDelays, "repeat", g_stSettings.m_iRepeatDelayIR, 480, 1, INT_MAX);
    }

    if (pControllerDelays)
    {
      GetInteger(pControllerDelays, "move", g_stSettings.m_iMoveDelayController, 220, 1, INT_MAX);
      GetInteger(pControllerDelays, "repeat", g_stSettings.m_iRepeatDelayController, 220, 1, INT_MAX);
      GetFloat(pControllerDelays, "deadzone", g_stSettings.m_fAnalogDeadzoneController, 0.1f, 0.0f, 1.0f);
    }
  }

  GetString(pRootElement, "home", g_stSettings.szHomeDir, "");
  while ( CUtil::HasSlashAtEnd(g_stSettings.szHomeDir) )
  {
    g_stSettings.szHomeDir[strlen(g_stSettings.szHomeDir) - 1] = 0;
  }
  GetString(pRootElement, "dashboard", g_stSettings.szDashboard, "C:\\xboxdash.xbe");

  GetString(pRootElement, "CDDBIpAddress", g_stSettings.m_szCDDBIpAdres, "freedb.freedb.org");
  //g_stSettings.m_bUseCDDB=GetBoolean(pRootElement, "CDDBEnabled");
  GetString(pRootElement, "IMDBAddress", g_stSettings.m_szIMDBurl, "akas.imdb.com");

  GetString(pRootElement, "thumbnails", g_stSettings.szThumbnailsDirectory, "");
  GetString(pRootElement, "shortcuts", g_stSettings.m_szShortcutDirectory, "");
  GetString(pRootElement, "screenshots", g_stSettings.m_szScreenshotsDirectory, "");
  GetString(pRootElement, "recordings", g_stSettings.m_szMusicRecordingDirectory, "");
  GetString(pRootElement, "playlists", g_stSettings.m_szPlaylistsDirectory, "");

  GetString(pRootElement, "albums", g_stSettings.m_szAlbumDirectory, "");
  GetString(pRootElement, "subtitles", g_stSettings.m_szAlternateSubtitleDirectory, "");
  GetString(pRootElement, "cachepath", g_stSettings.m_szCacheDirectory,"Z:\\");

  GetString(pRootElement, "pictureextensions", g_stSettings.m_szMyPicturesExtensions, ".bmp|.jpg|.png|.gif|.pcx|.tif|.jpeg");

  GetString(pRootElement, "musicextensions", g_stSettings.m_szMyMusicExtensions, ".ac3|.aac|.strm|.pls|.rm|.sc|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u");
  //  Internal file-types
  strcat(g_stSettings.m_szMyMusicExtensions, "|.oggstream"); //  bitstreams inside a ogg file
  strcat(g_stSettings.m_szMyMusicExtensions, "|.nsfstream"); //  track inside a nsf file
  strcat(g_stSettings.m_szMyMusicExtensions, "|.sidstream"); //  track inside a sid file

  GetString(pRootElement, "videoextensions", g_stSettings.m_szMyVideoExtensions, ".nfo|.rm|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli");

  // stacking regexps
  TiXmlElement* pVideoStacking = pRootElement->FirstChildElement("videostacking");
  if (pVideoStacking)
  {
    g_settings.m_MyVideoStackRegExps.clear();
    TiXmlNode* pStackRegExp = pVideoStacking->FirstChild("regexp");
    while (pStackRegExp)
    {
      if (pStackRegExp->FirstChild())
      {
        CStdString regExp = pStackRegExp->FirstChild()->Value();
        regExp.MakeLower();
        g_settings.m_MyVideoStackRegExps.push_back(regExp);
      }
      pStackRegExp = pStackRegExp->NextSibling("regexp");
    }
  }

  // path substitutions
  g_settings.m_vecPathSubstitutions.clear();
  TiXmlElement* pPathSubstitution = pRootElement->FirstChildElement("pathsubstitution");
  if (pPathSubstitution)
  {
    CLog::Log(LOGDEBUG,"Configuring path substitions");
    TiXmlNode* pSubstitute = pPathSubstitution->FirstChildElement("substitute");
    while (pSubstitute)
    {
      CStdString strFrom, strTo;
      TiXmlNode* pFrom = pSubstitute->FirstChild("from");
      if (pFrom)
        strFrom = pFrom->FirstChild()->Value();
      TiXmlNode* pTo = pSubstitute->FirstChild("to");
      if (pTo)
        strTo = pTo->FirstChild()->Value();

      if (!strFrom.IsEmpty() && !strTo.IsEmpty())
      {
        CLog::Log(LOGDEBUG,"  Registering substition pair:");
        CLog::Log(LOGDEBUG,"    From: [%s]", strFrom.c_str());
        CLog::Log(LOGDEBUG,"    To:   [%s]", strTo.c_str());
        // keep literal commas since we use comma as a seperator
        strFrom.Replace(",",",,");
        strTo.Replace(",",",,");
        g_settings.m_vecPathSubstitutions.push_back(strFrom + " , " + strTo);
      }
      else
      {
        // error message about missing tag
        if (strFrom.IsEmpty())
          CLog::Log(LOGERROR,"  Missing <from> tag");
        else
          CLog::Log(LOGERROR,"  Missing <to> tag");          
      }

      // get next one
      pSubstitute = pSubstitute->NextSiblingElement("substitute");
    }
  }

  // rss feeds
  TiXmlElement* pRssFeeds = pRootElement->FirstChildElement("rss");
  if (pRssFeeds)
  {
    g_settings.m_mapRssUrls.clear();
    TiXmlElement* pSet = pRssFeeds->FirstChildElement("set");
    while (pSet)
    {
      int iId;
      if (pSet->QueryIntAttribute("id", &iId) == TIXML_SUCCESS)
      {
        std::vector<wstring> vecSet;
        std::vector<int> vecIntervals;
        TiXmlElement* pFeed = pSet->FirstChildElement("feed");
        while (pFeed)
        {
          int iInterval;
          if ( pFeed->QueryIntAttribute("updateinterval",&iInterval) != TIXML_SUCCESS)
          {
            iInterval=30; // default to 30 min
            CLog::Log(LOGDEBUG,"no interval set, default to 30!");
          }
          if (pFeed->FirstChild())
          {
            CStdStringW strUrl = pFeed->FirstChild()->Value();
            vecSet.push_back(strUrl);
            vecIntervals.push_back(iInterval);
          }
          pFeed = pFeed->NextSiblingElement("feed");
        }
        g_settings.m_mapRssUrls.insert(std::make_pair<int,std::pair<std::vector<int>,std::vector<wstring> > >(iId,std::make_pair<std::vector<int>,std::vector<wstring> >(vecIntervals,vecSet)));
      } 
      else 
        CLog::Log(LOGERROR,"found rss url set with no id in XboxMediaCenter.xml, ignored");

      pSet = pSet->NextSiblingElement("set");
    }
  }

  /*
  GetInteger(pRootElement, "startwindow", g_stSettings.m_iStartupWindow, 0, 0, INT_MAX);
  g_stSettings.m_iStartupWindow += WINDOW_HOME; // windows referenced from WINDOW_HOME
  */

  CStdString strWindow;
  GetString(pRootElement, "startwindow", strWindow, "0");
  g_stSettings.m_iStartupWindow = g_buttonTranslator.TranslateWindowString(strWindow.c_str());

  GetBoolean(pRootElement, "autodetectFG", g_stSettings.m_bAutoDetectFG);
  GetBoolean(pRootElement, "useFDrive", g_stSettings.m_bUseFDrive);
  GetBoolean(pRootElement, "useGDrive", g_stSettings.m_bUseGDrive);
  GetBoolean(pRootElement, "usePCDVDROM", g_stSettings.m_bUsePCDVDROM);

  GetBoolean(pRootElement, "detectAsIso", g_stSettings.m_bDetectAsIso);

  GetBoolean(pRootElement, "displayremotecodes", g_stSettings.m_bDisplayRemoteCodes);

  GetString(pRootElement, "dvdplayer", g_stSettings.m_szExternalDVDPlayer, "");
  GetString(pRootElement, "cddaplayer", g_stSettings.m_szExternalCDDAPlayer, "");

  GetString(pRootElement, "CDDARipPath", g_stSettings.m_strRipPath, "");

  CStdString strDir;

  strDir = g_stSettings.m_szShortcutDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szShortcutDirectory, strDir.c_str() );

  strDir = g_stSettings.szThumbnailsDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.szThumbnailsDirectory, strDir.c_str() );

  strDir = g_stSettings.m_szAlbumDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szAlbumDirectory, strDir.c_str() );

  strDir = g_stSettings.m_szMusicRecordingDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szMusicRecordingDirectory, strDir.c_str() );

  strDir = g_stSettings.m_szScreenshotsDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szScreenshotsDirectory, strDir.c_str() );

  strDir = g_stSettings.m_szPlaylistsDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szPlaylistsDirectory, strDir.c_str() );
  
  strDir = g_stSettings.m_szCacheDirectory;
  ConvertHomeVar(strDir);
  CUtil::AddSlashAtEnd(strDir);
  strcpy( g_stSettings.m_szCacheDirectory, strDir.c_str() );

  while ( CUtil::HasSlashAtEnd(g_stSettings.m_szScreenshotsDirectory) )
  {
    g_stSettings.m_szScreenshotsDirectory[strlen(g_stSettings.m_szScreenshotsDirectory) - 1] = 0;
  }

  if (g_stSettings.m_szShortcutDirectory[0])
  {
    CShare share;
    share.strPath = g_stSettings.m_szShortcutDirectory;
    share.strName = "shortcuts";
    m_vecMyProgramsBookmarks.push_back(share);
  }


  strDir = g_stSettings.m_szAlternateSubtitleDirectory;
  ConvertHomeVar(strDir);
  strcpy( g_stSettings.m_szAlternateSubtitleDirectory, strDir.c_str() );

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  // Home page button scroller
  LoadHomeButtons(pRootElement);
#endif

  // parse my programs bookmarks...
  CStdString strDefault;
  GetShares(pRootElement, "myprograms", m_vecMyProgramsBookmarks, strDefault);
  if (strDefault.size())
    strcpy( g_stSettings.m_szDefaultPrograms, strDefault.c_str());

  GetShares(pRootElement, "pictures", m_vecMyPictureShares, strDefault);
  if (strDefault.size())
    strcpy( g_stSettings.m_szDefaultPictures, strDefault.c_str());

  GetShares(pRootElement, "files", m_vecMyFilesShares, strDefault);
  if (strDefault.size())
    strcpy( g_stSettings.m_szDefaultFiles, strDefault.c_str());

  GetShares(pRootElement, "music", m_vecMyMusicShares, strDefault);
  if (strDefault.size())
    strcpy( g_stSettings.m_szDefaultMusic, strDefault.c_str());

  GetShares(pRootElement, "video", m_vecMyVideoShares, strDefault);
  if (strDefault.size())
    strcpy( g_stSettings.m_szDefaultVideos, strDefault.c_str());

  g_LangCodeExpander.LoadUserCodes(pRootElement->FirstChildElement("languagecodes"));

  bXboxMediacenter = true;
  return true;
}

void CSettings::ConvertHomeVar(CStdString& strText)
{
  // Replaces first occurence of $HOME with the home directory.
  // "$HOME\bookmarks" becomes for instance "e:\apps\xbmp\bookmarks"

  char szText[1024];
  char szTemp[1024];
  char *pReplace, *pReplace2;

  CStdString strHomePath = "Q:";
  strcpy(szText, strText.c_str());

  pReplace = strstr(szText, "$HOME");

  if (pReplace != NULL)
  {
    pReplace2 = pReplace + sizeof("$HOME") - 1;
    strcpy(szTemp, pReplace2);
    strcpy(pReplace, strHomePath.c_str() );
    strcat(szText, szTemp);
  }
  strText = szText;
  // unroll any relative paths used
  std::vector<CStdString> token;
  CUtil::Tokenize(strText,token,"\\");
  if (token.size() > 1)
  {
    strText = "";
    for (unsigned int i=0;i<token.size();++i)
      if (token[i] == "..")
      {
        CStdString strParent;
        CUtil::GetParentPath(strText,strParent);
        strText = strParent+"\\";
      }
      else
        strText += token[i]+"\\";
    // remove trailing slash
    if (CUtil::HasSlashAtEnd(strText))
      strText.Delete(strText.size() - 1);
  }
}

void CSettings::GetShares(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSHARES& items, CStdString& strDefault)
{
  CLog::Log(LOGDEBUG, "  Parsing <%s> tag", strTagName.c_str());
  strDefault = "";

  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    pChild = pChild->FirstChild();
    while (pChild > 0)
    {
      CStdString strValue = pChild->Value();
      if (strValue == "bookmark")
      {
        CLog::Log(LOGDEBUG,"    ---- BOOKMARK START ----");
        const TiXmlNode *pNodeName = pChild->FirstChild("name");
        CStdString strName = pNodeName->FirstChild()->Value();
        CLog::Log(LOGDEBUG,"    Found name: %s", strName.c_str());

        // get multiple paths
        vector<CStdString> vecPaths;
        TiXmlNode *pPathName = pChild->FirstChild("path");
        while (pPathName)
        {
          if (pPathName->FirstChild())
          {
            CStdString strPath = pPathName->FirstChild()->Value();
            // make sure there are no virtualpaths or stack paths defined in xboxmediacenter.xml
            CLog::Log(LOGDEBUG,"    Found path: %s", strPath.c_str());
            if (!CUtil::IsVirualPath(strPath) && !CUtil::IsStack(strPath))
            {
              // translate paths
              CStdString strPathOld = strPath;
              ConvertHomeVar(strPath);
              if (!strPath.Equals(strPathOld))
                CLog::Log(LOGDEBUG,"    -> Translated to path: %s", strPath.c_str());

              // translate special tags
              if (strPath.at(0) == '$')
              {
                strPath = CUtil::TranslateSpecialDir(strPath);
                if (!strPath.IsEmpty())
                  CLog::Log(LOGDEBUG,"    -> Translated to path: %s", strPath.c_str());
                else
                {
                  CLog::Log(LOGERROR,"    -> Skipping invalid token: %s", strPathOld.c_str());
                  pPathName = pPathName->NextSibling("path");
                  continue;
                }
              }
              vecPaths.push_back(strPath);
            }
            else
              CLog::Log(LOGERROR,"    Invalid path type (%s) in bookmark", strPath.c_str());
          }
          pPathName = pPathName->NextSibling("path");
        }
        
        const TiXmlNode *pCacheNode = pChild->FirstChild("cache");
        const TiXmlNode *pDepthNode = pChild->FirstChild("depth");
        const TiXmlNode *pLockMode = pChild->FirstChild("lockmode");
        const TiXmlNode *pLockCode = pChild->FirstChild("lockcode");
        const TiXmlNode *pBadPwdCount = pChild->FirstChild("badpwdcount");
        const TiXmlNode *pThumbnailNode = pChild->FirstChild("thumbnail");

        if (!strName.IsEmpty() && vecPaths.size() > 0)
        {
          CShare share;

          CStdString strPath;
          // disallowed for files, or theres only a single path in the vector
          if ((strTagName.Equals("files")) || (vecPaths.size() == 1))
            strPath = vecPaths[0];

          // multiple paths?
          else
          {
            // validate the paths
            for (int j = 0; j < (int)vecPaths.size(); ++j)
            {
              CURL url(vecPaths[j]);
              CStdString protocol = url.GetProtocol();
              bool bIsInvalid = false;

              // for my programs
              if (strTagName.Equals("myprograms"))
              {
                // only allow HD
                if (protocol.IsEmpty())
                  share.vecPaths.push_back(vecPaths[j]);
                else
                  bIsInvalid = true;
              }

              // for others
              else
              {
                // only allow HD, SMB, and XBMS
                if (protocol.IsEmpty() || protocol.Equals("smb") || protocol.Equals("xbms"))
                  share.vecPaths.push_back(vecPaths[j]);
                else
                  bIsInvalid = true;
              }
              
              // error message
              if (bIsInvalid)   
                CLog::Log(LOGERROR,"    Invalid path type (%s) for multipath bookmark", vecPaths[j].c_str());
            }

            // no valid paths? skip to next bookmark
            if (share.vecPaths.size() == 0)
            {
              CLog::Log(LOGERROR,"    Missing or invalid <name> and/or <path> in bookmark");
              pChild = pChild->NextSibling();
            }

            // only one valid path? make it the strPath
            else if (share.vecPaths.size() == 1)
            {
              strPath = share.vecPaths[0];
              share.vecPaths.empty();
            }

            // multiple valid paths?
            else
            {
              // if my programs, make a comma seperated path
              if (strTagName.Equals("myprograms"))
              {
                for (int j = 0; j < (int)share.vecPaths.size(); ++j)
                  strPath += share.vecPaths[j] + ",";
                strPath.Delete(strPath.size()-1);
              }
              // otherwise make a virtualpath path
              else
                strPath.Format("virtualpath://%s/%s", strTagName.c_str(), strName.c_str());
            }
          }

          share.strName = strName;
          share.strPath = strPath;
          share.m_iBufferSize = 0;
          share.m_iDepthSize = 1;
          share.m_iLockMode = LOCK_MODE_EVERYONE;
          share.m_strLockCode = "";
          share.m_iBadPwdCount = 0;

          CLog::Log(LOGDEBUG,"      Adding bookmark:");
          CLog::Log(LOGDEBUG,"        Name: %s", share.strName.c_str());
          if (CUtil::IsVirualPath(share.strPath))
          {
            for (int i = 0; i < (int)share.vecPaths.size(); ++i)
              CLog::Log(LOGDEBUG,"        Path (%02i): %s", i+1, share.vecPaths.at(i).c_str());
          }
          else
            CLog::Log(LOGDEBUG,"        Path: %s", share.strPath.c_str());

          strPath = share.strPath;
          strPath.ToUpper();
          if (strPath.Left(12) == "VIRTUALPATH:")
            share.m_iDriveType = SHARE_TYPE_VPATH;
          else if (strPath.Left(4) == "UDF:")
          {
            share.m_iDriveType = SHARE_TYPE_VIRTUAL_DVD;
            share.strPath = "D:\\";
          }
          else if (strPath.Left(11) == "SOUNDTRACK:")
            share.m_iDriveType = SHARE_TYPE_LOCAL;
          else if (CUtil::IsISO9660(share.strPath))
            share.m_iDriveType = SHARE_TYPE_VIRTUAL_DVD;
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
            share.m_iBufferSize = atoi( pCacheNode->FirstChild()->Value() );
          }

          if (pDepthNode)
          {
            share.m_iDepthSize = atoi( pDepthNode->FirstChild()->Value() );
          }

          if (pLockMode)
          {
            share.m_iLockMode = atoi( pLockMode->FirstChild()->Value() );
          }

          if (pLockCode)
          {
            share.m_strLockCode = pLockCode->FirstChild()->Value();
          }

          if (pBadPwdCount)
          {
            share.m_iBadPwdCount = atoi( pBadPwdCount->FirstChild()->Value() );
          }

          if (pThumbnailNode)
          {
            share.m_strThumbnailImage = pThumbnailNode->FirstChild()->Value();
          }

          // check - convert to url and back again to make sure strPath is accurate
          // in terms of what we expect
          CURL url(share.strPath);
          url.GetURL(share.strPath);
          items.push_back(share);
        }
        else
        {
          CLog::Log(LOGERROR, "    Missing or invalid <name> and/or <path> in bookmark");
        }
      }

      if (strValue == "default")
      {
        const TiXmlNode *pValueNode = pChild->FirstChild();
        if (pValueNode)
        {
          const char* pszText = pChild->FirstChild()->Value();
          if (strlen(pszText) > 0)
            strDefault = pszText;
          CLog::Log(LOGDEBUG, "    Setting <default> share to : %s", strDefault.c_str());
        }
      }
      pChild = pChild->NextSibling();
    }
  }
  else
  {
    CLog::Log(LOGERROR, "  <%s> tag is missing or XboxMediaCenter.xml is malformed", strTagName.c_str());
  }
}

void CSettings::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, CStdString &strValue, const CStdString& strDefaultValue)
{
  strValue.Empty();
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    if (pChild->FirstChild())
    {
      strValue = pChild->FirstChild()->Value();
      if (strValue.Equals("-"))
        strValue.Empty();
    }
  }
  if (strValue.IsEmpty())
    strValue = strDefaultValue;

  //CLog::Log(LOGDEBUG, "  %s: %s", strTagName.c_str(), strValue.c_str());
}

void CSettings::GetString(const TiXmlElement* pRootElement, const CStdString& strTagName, char *szValue, const CStdString& strDefaultValue)
{
  CStdString strValue;
  GetString(pRootElement, strTagName, strValue, strDefaultValue);
  if (szValue)
    strcpy(szValue, strValue.c_str());
}

void CSettings::GetInteger(const TiXmlElement* pRootElement, const CStdString& strTagName, int& iValue, const int iDefault, const int iMin, const int iMax)
{
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    iValue = atoi( pChild->FirstChild()->Value() );
    if ((iValue < iMin) || (iValue > iMax)) iValue = iDefault;
  }
  else
    iValue = iDefault;

  CLog::Log(LOGDEBUG, "  %s: %d", strTagName.c_str(), iValue);
}

void CSettings::GetFloat(const TiXmlElement* pRootElement, const CStdString& strTagName, float& fValue, const float fDefault, const float fMin, const float fMax)
{
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    fValue = (float)atof( pChild->FirstChild()->Value() );
    if ((fValue < fMin) || (fValue > fMax)) fValue = fDefault;
  }
  else
    fValue = fDefault;

  CLog::Log(LOGDEBUG, "  %s: %f", strTagName.c_str(), fValue);
}

void CSettings::GetBoolean(const TiXmlElement* pRootElement, const CStdString& strTagName, bool& bValue)
{
  char szString[128];
  GetString(pRootElement, strTagName, szString, "");
  if ( strcmpi(szString, "enabled") == 0 ||
       strcmpi(szString, "yes") == 0 ||
       strcmpi(szString, "on") == 0 ||
       strcmpi(szString, "true") == 0 )
  {
    bValue = true;
  }
  else if (strlen(szString) != 0)
    bValue = false;
}

void CSettings::GetHex(const TiXmlNode* pRootElement, const CStdString& strTagName, DWORD& dwHexValue, DWORD dwDefaultValue)
{
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    sscanf(pChild->FirstChild()->Value(), "%x", &dwHexValue);
  }
  else
  {
    dwHexValue = dwDefaultValue;
  }
}

void CSettings::SetString(TiXmlNode* pRootNode, const CStdString& strTagName, const CStdString& strValue) const
{
  CStdString strPersistedValue = strValue;
  if (strPersistedValue.length() == 0)
  {
    strPersistedValue = '-';
  }

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
  strValue.Format("%d", iValue);
  SetString(pRootNode, strTagName, strValue);
}

void CSettings::SetFloat(TiXmlNode* pRootNode, const CStdString& strTagName, float fValue) const
{
  CStdString strValue;
  strValue.Format("%f", fValue);
  SetString(pRootNode, strTagName, strValue);
}

void CSettings::SetBoolean(TiXmlNode* pRootNode, const CStdString& strTagName, bool bValue) const
{
  if (bValue)
    SetString(pRootNode, strTagName, "true");
  else
    SetString(pRootNode, strTagName, "false");
}

void CSettings::SetHex(TiXmlNode* pRootNode, const CStdString& strTagName, DWORD dwHexValue) const
{
  CStdString strValue;
  strValue.Format("%x", dwHexValue);
  SetString(pRootNode, strTagName, strValue);
}

bool CSettings::LoadCalibration(const TiXmlElement* pElement, const CStdString& strSettingsFile)
{
  // reset the calibration to the defaults
  //g_graphicsContext.SetD3DParameters(NULL, m_ResInfo);
  //for (int i=0; i<10; i++)
  //  g_graphicsContext.ResetScreenParameters((RESOLUTION)i);

  const TiXmlElement *pRootElement;
  CStdString strTagName = pElement->Value();
  if (!strcmp(strTagName.c_str(), "calibration"))
  {
    pRootElement = pElement;
  }
  else
  {
    pRootElement = pElement->FirstChildElement("calibration");
  }
  if (!pRootElement)
  {
    g_LoadErrorStr.Format("%s Doesn't contain <calibration>", strSettingsFile.c_str());
    //be nice, try to load from "old" calibration.xml file
    if (CFile::Exists("T:\\calibration.xml"))
    {
      TiXmlDocument xmlDoc;
      if (!xmlDoc.LoadFile("T:\\calibration.xml"))
      {
        return false;
      }
      TiXmlElement *pOldConfigRootElement = xmlDoc.RootElement();
      return LoadCalibration(pOldConfigRootElement, "T:\\calibration.xml");
    }
    return false;
  }
  TiXmlElement *pResolution = pRootElement->FirstChildElement("resolution");
  while (pResolution)
  {
    // get the data for this resolution
    int iRes;
    GetInteger(pResolution, "id", iRes, (int)PAL_4x3, HDTV_1080i, PAL60_16x9); //PAL4x3 as default data
    GetString(pResolution, "description", m_ResInfo[iRes].strMode, m_ResInfo[iRes].strMode);
    // get the appropriate "safe graphics area" = 10% for 4x3, 3.5% for 16x9
    float fSafe;
    if (iRes == PAL_4x3 || iRes == NTSC_4x3 || iRes == PAL60_4x3 || iRes == HDTV_480p_4x3)
      fSafe = 0.1f;
    else
      fSafe = 0.035f;
    GetInteger(pResolution, "subtitles", m_ResInfo[iRes].iSubtitles, (int)((1 - fSafe)*m_ResInfo[iRes].iHeight), m_ResInfo[iRes].iHeight / 2, m_ResInfo[iRes].iHeight*5 / 4);
    GetFloat(pResolution, "pixelratio", m_ResInfo[iRes].fPixelRatio, 128.0f / 117.0f, 0.5f, 2.0f);
    GetInteger(pResolution, "osdyoffset", m_ResInfo[iRes].iOSDYOffset, 0, -m_ResInfo[iRes].iHeight, m_ResInfo[iRes].iHeight);

    // get the overscan info
    TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
    if (pOverscan)
    {
      GetInteger(pOverscan, "left", m_ResInfo[iRes].Overscan.left, 0, -m_ResInfo[iRes].iWidth / 4, m_ResInfo[iRes].iWidth / 4);
      GetInteger(pOverscan, "top", m_ResInfo[iRes].Overscan.top, 0, -m_ResInfo[iRes].iHeight / 4, m_ResInfo[iRes].iHeight / 4);
      GetInteger(pOverscan, "right", m_ResInfo[iRes].Overscan.right, m_ResInfo[iRes].iWidth, m_ResInfo[iRes].iWidth / 2, m_ResInfo[iRes].iWidth*3 / 2);
      GetInteger(pOverscan, "bottom", m_ResInfo[iRes].Overscan.bottom, m_ResInfo[iRes].iHeight, m_ResInfo[iRes].iHeight / 2, m_ResInfo[iRes].iHeight*3 / 2);
    }
    // GUI overscan info
    pOverscan = pResolution->FirstChildElement("guioverscan");
    if (pOverscan)
    {
      GetInteger(pOverscan, "left", m_ResInfo[iRes].GUIOverscan.left, 0, -m_ResInfo[iRes].iWidth / 4, m_ResInfo[iRes].iWidth / 4);
      GetInteger(pOverscan, "top", m_ResInfo[iRes].GUIOverscan.top, 0, -m_ResInfo[iRes].iHeight / 4, m_ResInfo[iRes].iHeight / 4);
      GetInteger(pOverscan, "right", m_ResInfo[iRes].GUIOverscan.right, m_ResInfo[iRes].iWidth, m_ResInfo[iRes].iWidth / 2, m_ResInfo[iRes].iWidth * 3 / 2);
      GetInteger(pOverscan, "bottom", m_ResInfo[iRes].GUIOverscan.bottom, m_ResInfo[iRes].iHeight, m_ResInfo[iRes].iHeight / 2, m_ResInfo[iRes].iHeight * 3 / 2);
    }
    CLog::Log(LOGDEBUG, "  calibration for %s %ix%i", m_ResInfo[iRes].strMode, m_ResInfo[iRes].iWidth, m_ResInfo[iRes].iHeight);
    CLog::Log(LOGDEBUG, "    subtitle yposition:%i pixelratio:%03.3f offsets:(%i,%i)->(%i,%i) osdyoffset:%i",
              m_ResInfo[iRes].iSubtitles, m_ResInfo[iRes].fPixelRatio,
              m_ResInfo[iRes].Overscan.left, m_ResInfo[iRes].Overscan.top,
              m_ResInfo[iRes].Overscan.right, m_ResInfo[iRes].Overscan.bottom,
              m_ResInfo[iRes].iOSDYOffset);
    CLog::Log(LOGDEBUG, "    GUI calibration :(%i,%i)->(%i,%i)",
              m_ResInfo[iRes].GUIOverscan.left, m_ResInfo[iRes].GUIOverscan.top,
              m_ResInfo[iRes].GUIOverscan.right, m_ResInfo[iRes].GUIOverscan.bottom);

    // iterate around
    pResolution = pResolution->NextSiblingElement("resolution");
  }
  return true;
}

bool CSettings::SaveCalibration(TiXmlNode* pRootNode) const
{
  TiXmlElement xmlRootElement("calibration");
  TiXmlNode *pRoot = pRootNode->InsertEndChild(xmlRootElement);
  for (int i = 0; i < 10; i++)
  {
    // Write the resolution tag
    TiXmlElement resElement("resolution");
    TiXmlNode *pNode = pRoot->InsertEndChild(resElement);
    // Now write each of the pieces of information we need...
    SetString(pNode, "description", m_ResInfo[i].strMode);
    SetInteger(pNode, "id", i);
    SetInteger(pNode, "subtitles", m_ResInfo[i].iSubtitles);
    SetInteger(pNode, "osdyoffset", m_ResInfo[i].iOSDYOffset);
    SetFloat(pNode, "pixelratio", m_ResInfo[i].fPixelRatio);
    // create the overscan child
    TiXmlElement overscanElement("overscan");
    TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
    SetInteger(pOverscanNode, "left", m_ResInfo[i].Overscan.left);
    SetInteger(pOverscanNode, "top", m_ResInfo[i].Overscan.top);
    SetInteger(pOverscanNode, "right", m_ResInfo[i].Overscan.right);
    SetInteger(pOverscanNode, "bottom", m_ResInfo[i].Overscan.bottom);
    // create the GUIoverscan child
    TiXmlElement guiOverscanElement("guioverscan");
    TiXmlNode *pGUIOverscanNode = pNode->InsertEndChild(guiOverscanElement);
    SetInteger(pGUIOverscanNode, "left", m_ResInfo[i].GUIOverscan.left);
    SetInteger(pGUIOverscanNode, "top", m_ResInfo[i].GUIOverscan.top);
    SetInteger(pGUIOverscanNode, "right", m_ResInfo[i].GUIOverscan.right);
    SetInteger(pGUIOverscanNode, "bottom", m_ResInfo[i].GUIOverscan.bottom);
  }
  return true;
}

bool CSettings::LoadSettings(const CStdString& strSettingsFile, const bool loadprofiles)
{
  // load the xml file
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strSettingsFile))
  {
    g_LoadErrorStr.Format("%s, Line %d\n%s", strSettingsFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }
  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "settings") != 0)
  {
    g_LoadErrorStr.Format("%s\nDoesn't contain <settings>", strSettingsFile.c_str());
    return false;
  }

  if (loadprofiles)
  {
    LoadProfiles(pRootElement, strSettingsFile);
    if (m_vecProfiles.size() == 0)
    {
      //no profiles yet, make one based on the default settings
      CProfile profile;
      profile.setFileName("profile0.xml");
      profile.setName("Default settings");
      m_vecProfiles.push_back(profile);
      SaveSettingsToProfile(0);
    }
  }

  // mypictures
  TiXmlElement *pElement = pRootElement->FirstChildElement("mypictures");
  if (pElement)
  {
    GetInteger(pElement, "viewmethod", (int&)g_stSettings.m_MyPicturesViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
    GetInteger(pElement, "viewmethodroot", (int&)g_stSettings.m_MyPicturesRootViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
    GetInteger(pElement, "sortmethod", (int&)g_stSettings.m_MyPicturesSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
    GetInteger(pElement, "sortmethodroot", (int&)g_stSettings.m_MyPicturesRootSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
    GetInteger(pElement, "sortorder", (int&)g_stSettings.m_MyPicturesSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
    GetInteger(pElement, "sortorderroot", (int&)g_stSettings.m_MyPicturesRootSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
  }
  // myfiles
/*  pElement = pRootElement->FirstChildElement("myfiles");
  if (pElement)
  {
    TiXmlElement *pChild = pElement->FirstChildElement("source");
    if (pChild)
    {
      GetBoolean(pChild, "srcfilesviewmethod", g_stSettings.m_bMyFilesSourceViewMethod);
      GetBoolean(pChild, "srcfilesviewmethodroot", g_stSettings.m_bMyFilesSourceRootViewMethod);
      GetInteger(pChild, "srcfilessortmethod", g_stSettings.m_iMyFilesSourceSortMethod, 0, 0, 2);
      GetInteger(pChild, "srcfilessortmethodroot", g_stSettings.m_iMyFilesSourceRootSortMethod, 0, 0, 3);
      GetBoolean(pChild, "srcfilessortorder", g_stSettings.m_bMyFilesSourceSortOrder);
      GetBoolean(pChild, "srcfilessortorderroot", g_stSettings.m_bMyFilesSourceRootSortOrder);
    }
    pChild = pElement->FirstChildElement("dest");
    if (pChild)
    {
      GetBoolean(pChild, "dstfilesviewmethod", g_stSettings.m_bMyFilesDestViewMethod);
      GetBoolean(pChild, "dstfilesviewmethodroot", g_stSettings.m_bMyFilesDestRootViewMethod);
      GetInteger(pChild, "dstfilessortmethod", g_stSettings.m_iMyFilesDestSortMethod, 0, 0, 2);
      GetInteger(pChild, "dstfilessortmethodroot", g_stSettings.m_iMyFilesDestRootSortMethod, 0, 0, 3);
      GetBoolean(pChild, "dstfilessortorder", g_stSettings.m_bMyFilesDestSortOrder);
      GetBoolean(pChild, "dstfilessortorderroot", g_stSettings.m_bMyFilesDestRootSortOrder);
    }
  }*/

  // mymusic settings
  pElement = pRootElement->FirstChildElement("mymusic");
  if (pElement)
  {
    TiXmlElement *pChild = pElement->FirstChildElement("songs");
    if (pChild)
    {
      GetInteger(pChild, "viewmethod", (int&)g_stSettings.m_MyMusicSongsViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "viewmethodroot", (int&)g_stSettings.m_MyMusicSongsRootViewMethod, VIEW_METHOD_ICONS, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "sortmethod", (int&)g_stSettings.m_MyMusicSongsSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortmethodroot", (int&)g_stSettings.m_MyMusicSongsRootSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortorder", (int&)g_stSettings.m_MyMusicSongsSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "sortorderroot", (int&)g_stSettings.m_MyMusicSongsRootSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
    }
    pChild = pElement->FirstChildElement("nav");
    if (pChild)
    {
      GetInteger(pChild, "rootviewmethod", (int&)g_stSettings.m_MyMusicNavRootViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "genresviewmethod", (int&)g_stSettings.m_MyMusicNavGenresViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "artistsviewmethod", (int&)g_stSettings.m_MyMusicNavArtistsViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "albumsviewmethod", (int&)g_stSettings.m_MyMusicNavAlbumsViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "songsviewmethod", (int&)g_stSettings.m_MyMusicNavSongsViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "topviewmethod", (int&)g_stSettings.m_MyMusicNavTopViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "playlistsviewmethod", (int&)g_stSettings.m_MyMusicNavPlaylistsViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);

      GetInteger(pChild, "genressortmethod", (int&)g_stSettings.m_MyMusicNavRootSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "albumssortmethod", (int&)g_stSettings.m_MyMusicNavAlbumsSortMethod, SORT_METHOD_ALBUM, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "songssortmethod", (int&)g_stSettings.m_MyMusicNavSongsSortMethod, SORT_METHOD_TRACKNUM, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "playlistssortmethod", (int&)g_stSettings.m_MyMusicNavPlaylistsSortMethod, SORT_METHOD_TRACKNUM, SORT_METHOD_NONE, SORT_METHOD_MAX-1);

      GetInteger(pChild, "genressortorder", (int&)g_stSettings.m_MyMusicNavGenresSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "artistssortorder", (int&)g_stSettings.m_MyMusicNavArtistsSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "albumssortorder", (int&)g_stSettings.m_MyMusicNavAlbumsSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "songssortorder", (int&)g_stSettings.m_MyMusicNavSongsSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "playlistssortorder", (int&)g_stSettings.m_MyMusicNavPlaylistsSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
    }

    pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    {
      GetInteger(pChild, "playlistviewmethodroot", (int&)g_stSettings.m_MyMusicPlaylistViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetBoolean(pChild, "repeat", g_stSettings.m_bMyMusicPlaylistRepeat);
      GetBoolean(pChild, "shuffle", g_stSettings.m_bMyMusicPlaylistShuffle);
    }
    // use tags and find thumbs should be enabled when a scan is iniated
    // these settings will keep track of the previous values and restore them
    // if the user happened to reboot in the middle of the scan
    pChild = pElement->FirstChildElement("scanning");
    if (pChild)
    {
      GetBoolean(pChild, "isscanning", g_stSettings.m_bMyMusicIsScanning);
      GetBoolean(pChild, "oldusetags", g_stSettings.m_bMyMusicOldUseTags);
      GetBoolean(pChild, "oldfindthumbs", g_stSettings.m_bMyMusicOldFindThumbs);
    }
    GetInteger(pElement, "startwindow", g_stSettings.m_iMyMusicStartWindow, WINDOW_MUSIC_FILES, WINDOW_MUSIC_FILES, WINDOW_MUSIC_NAV); //501; view songs
    GetBoolean(pElement, "songinfoinvis", g_stSettings.m_bMyMusicSongInfoInVis);
    GetBoolean(pElement, "songthumbinvis", g_stSettings.m_bMyMusicSongThumbInVis);
  }
  // myvideos settings
  pElement = pRootElement->FirstChildElement("myvideos");
  if (pElement)
  {
    GetInteger(pElement, "startwindow", g_stSettings.m_iVideoStartWindow, WINDOW_VIDEOS, WINDOW_VIDEO_GENRE, WINDOW_VIDEO_TITLE);
    GetInteger(pElement, "stackvideomode", g_stSettings.m_iMyVideoStack, STACK_NONE, STACK_NONE, STACK_SIMPLE);

    GetBoolean(pElement, "cleantitles", g_stSettings.m_bMyVideoCleanTitles);
    GetString(pElement, "cleantokens", g_stSettings.m_szMyVideoCleanTokens, g_stSettings.m_szMyVideoCleanTokens);
    GetString(pElement, "cleanseparators", g_stSettings.m_szMyVideoCleanSeparators, g_stSettings.m_szMyVideoCleanSeparators);

    StringUtils::SplitString(g_stSettings.m_szMyVideoCleanTokens, "|", g_settings.m_szMyVideoCleanTokensArray);
    g_settings.m_szMyVideoCleanSeparatorsString = g_stSettings.m_szMyVideoCleanSeparators;

    for (int i = 0; i < (int)g_settings.m_szMyVideoCleanTokensArray.size(); i++)
      g_settings.m_szMyVideoCleanTokensArray[i].MakeLower();

    TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    { // playlist
      GetInteger(pChild, "viewmethod", (int&)g_stSettings.m_MyVideoPlaylistViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetBoolean(pChild, "repeat", g_stSettings.m_bMyVideoPlaylistRepeat);
      GetBoolean(pChild, "shuffle", g_stSettings.m_bMyVideoPlaylistShuffle);
    }
    pChild = pElement->FirstChildElement("files");
    if (pChild)
    { // files
      GetInteger(pChild, "viewmethod", (int&)g_stSettings.m_MyVideoViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "viewmethodroot", (int&)g_stSettings.m_MyVideoRootViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "sortmethod", (int&)g_stSettings.m_MyVideoSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortmethodroot", (int&)g_stSettings.m_MyVideoRootSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortorder", (int&)g_stSettings.m_MyVideoSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "sortorderroot", (int&)g_stSettings.m_MyVideoRootSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
    }
    pChild = pElement->FirstChildElement("files");
    if (pChild)
    { // files
      GetInteger(pChild, "viewmethod", (int&)g_stSettings.m_MyVideoGenreViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "viewmethodroot", (int&)g_stSettings.m_MyVideoGenreRootViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "sortmethod", (int&)g_stSettings.m_MyVideoGenreSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortmethodroot", (int&)g_stSettings.m_MyVideoGenreRootSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortorder", (int&)g_stSettings.m_MyVideoGenreSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "sortorderroot", (int&)g_stSettings.m_MyVideoGenreRootSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "showmode", g_stSettings.m_iMyVideoGenreShowMode, VIDEO_SHOW_ALL, VIDEO_SHOW_ALL, VIDEO_SHOW_WATCHED);
    }
    pChild = pElement->FirstChildElement("actor");
    if (pChild)
    { // actor
      GetInteger(pChild, "viewmethod", (int&)g_stSettings.m_MyVideoActorViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "viewmethodroot", (int&)g_stSettings.m_MyVideoActorRootViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "sortmethod", (int&)g_stSettings.m_MyVideoActorSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortmethodroot", (int&)g_stSettings.m_MyVideoActorRootSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortorder", (int&)g_stSettings.m_MyVideoActorSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "sortorderroot", (int&)g_stSettings.m_MyVideoActorRootSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "showmode", g_stSettings.m_iMyVideoActorShowMode, VIDEO_SHOW_ALL, VIDEO_SHOW_ALL, VIDEO_SHOW_WATCHED);
    }
    pChild = pElement->FirstChildElement("year");
    if (pChild)
    { // year
      GetInteger(pChild, "viewmethod", (int&)g_stSettings.m_MyVideoYearViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "viewmethodroot", (int&)g_stSettings.m_MyVideoYearRootViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
      GetInteger(pChild, "sortmethod", (int&)g_stSettings.m_MyVideoYearSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortmethodroot", (int&)g_stSettings.m_MyVideoYearRootSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortorder", (int&)g_stSettings.m_MyVideoYearSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "sortorderroot", (int&)g_stSettings.m_MyVideoYearRootSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "showmode", g_stSettings.m_iMyVideoYearShowMode, VIDEO_SHOW_ALL, VIDEO_SHOW_ALL, VIDEO_SHOW_WATCHED);
    }
    pChild = pElement->FirstChildElement("title");
    if (pChild)
    { // titles
      GetInteger(pChild, "viewmethod", (int&)g_stSettings.m_MyVideoTitleViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST,  VIEW_METHOD_MAX-1);
      GetInteger(pChild, "sortmethod", (int&)g_stSettings.m_MyVideoTitleSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
      GetInteger(pChild, "sortorder", (int&)g_stSettings.m_MyVideoTitleSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
      GetInteger(pChild, "showmode", g_stSettings.m_iMyVideoTitleShowMode, VIDEO_SHOW_ALL, VIDEO_SHOW_ALL, VIDEO_SHOW_WATCHED);
    }

    GetFloat(pElement, "subsdelayrange", g_stSettings.m_fSubsDelayRange, 10, 10, 600);
    GetFloat(pElement, "audiodelayrange", g_stSettings.m_fAudioDelayRange, 10, 10, 600);
    GetInteger(pElement, "smallstepbackseconds", g_stSettings.m_iSmallStepBackSeconds, 7, 1, INT_MAX);
    GetInteger(pElement, "smallstepbacktries", g_stSettings.m_iSmallStepBackTries, 3, 1, 10);
    GetInteger(pElement, "smallstepbackdelay", g_stSettings.m_iSmallStepBackDelay, 300, 100, 5000); //MS

    GetInteger(pElement, "timeseekforward", g_stSettings.m_iMyVideoTimeSeekForward, 30, 0, 6000);
    GetInteger(pElement, "timeseekbackward", g_stSettings.m_iMyVideoTimeSeekBackward, -30, -6000, 0);
    GetInteger(pElement, "timeseekforwardbig", g_stSettings.m_iMyVideoTimeSeekForwardBig, 300, 0, 6000);
    GetInteger(pElement, "timeseekbackwardbig", g_stSettings.m_iMyVideoTimeSeekBackwardBig, -300, -6000, 0);

    GetInteger(pElement, "percentseekforward", g_stSettings.m_iMyVideoPercentSeekForward, 2, 0, 100);
    GetInteger(pElement, "percentseekbackward", g_stSettings.m_iMyVideoPercentSeekBackward, -2, -100, 0);
    GetInteger(pElement, "percentseekforwardbig", g_stSettings.m_iMyVideoPercentSeekForwardBig, 10, 0, 100);
    GetInteger(pElement, "percentseekbackwardbig", g_stSettings.m_iMyVideoPercentSeekBackwardBig, -10, -100, 0);
  }
  // myscripts settings
  pElement = pRootElement->FirstChildElement("myscripts");
  if (pElement)
  {
    GetInteger(pElement, "viewmethod", (int&)g_stSettings.m_ScriptsViewMethod, VIEW_METHOD_LIST, VIEW_METHOD_LIST, VIEW_METHOD_MAX-1);
    GetInteger(pElement, "sortmethod", (int&)g_stSettings.m_ScriptsSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
    GetInteger(pElement, "sortorder", (int&)g_stSettings.m_ScriptsSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
  }
  // general settings
  pElement = pRootElement->FirstChildElement("general");
  if (pElement)
  {
    GetInteger(pElement, "audiostream", g_stSettings.m_defaultVideoSettings.m_AudioStream, -1, -1, INT_MAX);
    GetInteger(pElement, "systemtotaluptime", g_stSettings.m_iSystemTimeTotalUp, 0, 0, INT_MAX);
    GetString(pElement, "kaiarenapass", g_stSettings.szOnlineArenaPassword, "");
    GetString(pElement, "kaiarenadesc", g_stSettings.szOnlineArenaDescription, "");
  }
  
  // screen settings
  pElement = pRootElement->FirstChildElement("screen");
  if (pElement)
  {
    //    GetInteger(pElement, "viewmode", g_stSettings.m_defaultVideoSettings.m_ViewMode, VIEW_MODE_NORMAL, VIEW_MODE_NORMAL, VIEW_MODE_CUSTOM);
    GetFloat(pElement, "zoomamount", g_stSettings.m_defaultVideoSettings.m_CustomZoomAmount, 1.0f, 1.0f, 2.0f);
    GetFloat(pElement, "pixelratio", g_stSettings.m_defaultVideoSettings.m_CustomPixelRatio, 1.0f, 0.5f, 2.0f);
    // there is no way to set these settings correctly, so lets default them
    g_stSettings.m_defaultVideoSettings.m_ViewMode = VIEW_MODE_NORMAL;
  }
  // audio settings
  pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    GetInteger(pElement, "volumelevel", g_stSettings.m_nVolumeLevel, VOLUME_MAXIMUM, VOLUME_MINIMUM, VOLUME_MAXIMUM);
  }

  // my programs
  pElement = pRootElement->FirstChildElement("myprograms");
  if (pElement)
  {
    GetInteger(pElement, "viewmethod", (int&)g_stSettings.m_MyProgramsViewMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
    GetInteger(pElement, "sortmethod", (int&)g_stSettings.m_MyProgramsSortMethod, SORT_METHOD_LABEL, SORT_METHOD_NONE, SORT_METHOD_MAX-1);
    GetInteger(pElement, "sortorder", (int&)g_stSettings.m_MyProgramsSortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
  }
  LoadCalibration(pRootElement, strSettingsFile);

  g_guiSettings.LoadXML(pRootElement);

  LoadSkinSettings(pRootElement);

  return true;
}

bool CSettings::SaveSettings(const CStdString& strSettingsFile, const bool saveprofiles) const
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("settings");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  // write our tags one by one - just a big list for now (can be flashed up later)

  if (saveprofiles)
  {
    SaveProfiles(pRoot);
  }

  // myprograms settings
  TiXmlElement programsNode("myprograms");
  TiXmlNode *pNode = pRoot->InsertEndChild(programsNode);
  if (!pNode) return false;
  SetInteger(pNode, "viewmethod", g_stSettings.m_MyProgramsViewMethod);
  SetInteger(pNode, "sortmethod", g_stSettings.m_MyProgramsSortMethod);
  SetInteger(pNode, "sortorder", g_stSettings.m_MyProgramsSortOrder);

  // mypictures settings
  TiXmlElement picturesNode("mypictures");
  pNode = pRoot->InsertEndChild(picturesNode);
  if (!pNode) return false;
  SetInteger(pNode, "viewmethod", g_stSettings.m_MyPicturesViewMethod);
  SetInteger(pNode, "viewmethodroot", g_stSettings.m_MyPicturesRootViewMethod);
  SetInteger(pNode, "sortmethod", g_stSettings.m_MyPicturesSortMethod);
  SetInteger(pNode, "sortmethodroot", g_stSettings.m_MyPicturesRootSortMethod);
  SetInteger(pNode, "sortorder", g_stSettings.m_MyPicturesSortOrder);
  SetInteger(pNode, "sortorderroot", g_stSettings.m_MyPicturesRootSortOrder);

  // myfiles settings
/*  TiXmlElement filesNode("myfiles");
  pNode = pRoot->InsertEndChild(filesNode);
  if (!pNode) return false;
  {
    TiXmlElement childNode("source");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetBoolean(pChild, "srcfilesviewmethod", g_stSettings.m_bMyFilesSourceViewMethod);
    SetBoolean(pChild, "srcfilesviewmethodroot", g_stSettings.m_bMyFilesSourceRootViewMethod);
    SetInteger(pChild, "srcfilessortmethod", g_stSettings.m_iMyFilesSourceSortMethod);
    SetInteger(pChild, "srcfilessortmethodroot", g_stSettings.m_iMyFilesSourceRootSortMethod);
    SetBoolean(pChild, "srcfilessortorder", g_stSettings.m_bMyFilesSourceSortOrder);
    SetBoolean(pChild, "srcfilessortorderroot", g_stSettings.m_bMyFilesSourceRootSortOrder);
  }
  {
    TiXmlElement childNode("dest");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetBoolean(pChild, "dstfilesviewmethod", g_stSettings.m_bMyFilesDestViewMethod);
    SetBoolean(pChild, "dstfilesviewmethodroot", g_stSettings.m_bMyFilesDestRootViewMethod);
    SetInteger(pChild, "dstfilessortmethod", g_stSettings.m_iMyFilesDestSortMethod);
    SetInteger(pChild, "dstfilessortmethodroot", g_stSettings.m_iMyFilesDestRootSortMethod);
    SetBoolean(pChild, "dstfilessortorder", g_stSettings.m_bMyFilesDestSortOrder);
    SetBoolean(pChild, "dstfilessortorderroot", g_stSettings.m_bMyFilesDestRootSortOrder);
  }*/

  // mymusic settings
  TiXmlElement musicNode("mymusic");
  pNode = pRoot->InsertEndChild(musicNode);
  if (!pNode) return false;
  {
    TiXmlElement childNode("songs");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetInteger(pChild, "viewmethod", g_stSettings.m_MyMusicSongsViewMethod);
    SetInteger(pChild, "viewmethodroot", g_stSettings.m_MyMusicSongsRootViewMethod);
    SetInteger(pChild, "sortmethod", g_stSettings.m_MyMusicSongsSortMethod);
    SetInteger(pChild, "sortmethodroot", g_stSettings.m_MyMusicSongsRootSortMethod);
    SetInteger(pChild, "sortorder", g_stSettings.m_MyMusicSongsSortOrder);
    SetInteger(pChild, "sortorderroot", g_stSettings.m_MyMusicSongsRootSortOrder);
  }
  {
    TiXmlElement childNode("nav");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;

    SetInteger(pChild, "rootviewmethod", g_stSettings.m_MyMusicNavRootViewMethod);
    SetInteger(pChild, "genresviewmethod", g_stSettings.m_MyMusicNavGenresViewMethod);
    SetInteger(pChild, "artistsviewmethod", g_stSettings.m_MyMusicNavArtistsViewMethod);
    SetInteger(pChild, "albumsviewmethod", g_stSettings.m_MyMusicNavAlbumsViewMethod);
    SetInteger(pChild, "songsviewmethod", g_stSettings.m_MyMusicNavSongsViewMethod);
    SetInteger(pChild, "topviewmethod", g_stSettings.m_MyMusicNavTopViewMethod);
    SetInteger(pChild, "playlistsviewmethod", g_stSettings.m_MyMusicNavPlaylistsViewMethod);

    SetInteger(pChild, "genressortmethod", g_stSettings.m_MyMusicNavRootSortMethod);
    SetInteger(pChild, "albumssortmethod", g_stSettings.m_MyMusicNavAlbumsSortMethod);
    SetInteger(pChild, "songssortmethod", g_stSettings.m_MyMusicNavSongsSortMethod);
    SetInteger(pChild, "playlistssortmethod", g_stSettings.m_MyMusicNavPlaylistsSortMethod);

    SetInteger(pChild, "genressortorder", g_stSettings.m_MyMusicNavGenresSortOrder);
    SetInteger(pChild, "artistssortorder", g_stSettings.m_MyMusicNavArtistsSortOrder);
    SetInteger(pChild, "albumssortorder", g_stSettings.m_MyMusicNavAlbumsSortOrder);
    SetInteger(pChild, "songssortorder", g_stSettings.m_MyMusicNavSongsSortOrder);
    SetInteger(pChild, "playlistssortorder", g_stSettings.m_MyMusicNavPlaylistsSortOrder);
  }
  {
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetInteger(pChild, "playlistviewmethodroot", g_stSettings.m_MyMusicPlaylistViewMethod);
    SetBoolean(pChild, "repeat", g_stSettings.m_bMyMusicPlaylistRepeat);
    SetBoolean(pChild, "shuffle", g_stSettings.m_bMyMusicPlaylistShuffle);
  }
  {
    TiXmlElement childNode("scanning");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetBoolean(pChild, "isscanning", g_stSettings.m_bMyMusicIsScanning);
    SetBoolean(pChild, "oldusetags", g_stSettings.m_bMyMusicOldUseTags);
    SetBoolean(pChild, "oldfindthumbs", g_stSettings.m_bMyMusicOldFindThumbs);
  }

  SetInteger(pNode, "startwindow", g_stSettings.m_iMyMusicStartWindow);
  SetBoolean(pNode, "songinfoinvis", g_stSettings.m_bMyMusicSongInfoInVis);
  SetBoolean(pNode, "songthumbinvis", g_stSettings.m_bMyMusicSongThumbInVis);

  // myvideos settings
  TiXmlElement videosNode("myvideos");
  pNode = pRoot->InsertEndChild(videosNode);
  if (!pNode) return false;

  SetInteger(pNode, "startwindow", g_stSettings.m_iVideoStartWindow);
  SetInteger(pNode, "stackvideomode", g_stSettings.m_iMyVideoStack);

  SetBoolean(pNode, "cleantitles", g_stSettings.m_bMyVideoCleanTitles);
  SetString(pNode, "cleantokens", g_stSettings.m_szMyVideoCleanTokens);
  SetString(pNode, "cleanseparators", g_stSettings.m_szMyVideoCleanSeparators);

  { // playlist window
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetInteger(pChild, "viewmethod", g_stSettings.m_MyVideoPlaylistViewMethod);
    SetBoolean(pChild, "repeat", g_stSettings.m_bMyVideoPlaylistRepeat);
    SetBoolean(pChild, "shuffle", g_stSettings.m_bMyVideoPlaylistShuffle);
  }
  { // files window
    TiXmlElement childNode("files");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetInteger(pChild, "viewmethod", g_stSettings.m_MyVideoViewMethod);
    SetInteger(pChild, "viewmethodroot", g_stSettings.m_MyVideoRootViewMethod);
    SetInteger(pChild, "sortmethod", g_stSettings.m_MyVideoSortMethod);
    SetInteger(pChild, "sortmethodroot", g_stSettings.m_MyVideoRootSortMethod);
    SetInteger(pChild, "sortorder", g_stSettings.m_MyVideoSortOrder);
    SetInteger(pChild, "sortorderroot", g_stSettings.m_MyVideoRootSortOrder);
  }
  { // genre window
    TiXmlElement childNode("genre");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetInteger(pChild, "viewmethod", g_stSettings.m_MyVideoGenreViewMethod);
    SetInteger(pChild, "viewmethodroot", g_stSettings.m_MyVideoGenreRootViewMethod);
    SetInteger(pChild, "sortmethod", g_stSettings.m_MyVideoGenreSortMethod);
    SetInteger(pChild, "sortmethodroot", g_stSettings.m_MyVideoGenreRootSortMethod);
    SetInteger(pChild, "sortorder", g_stSettings.m_MyVideoGenreSortOrder);
    SetInteger(pChild, "sortorderroot", g_stSettings.m_MyVideoGenreRootSortOrder);
    SetInteger(pChild, "showmode", g_stSettings.m_iMyVideoGenreShowMode);
  }
  { // actors window
    TiXmlElement childNode("actor");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetInteger(pChild, "viewmethod", g_stSettings.m_MyVideoActorViewMethod);
    SetInteger(pChild, "viewmethodroot", g_stSettings.m_MyVideoActorRootViewMethod);
    SetInteger(pChild, "sortmethod", g_stSettings.m_MyVideoActorSortMethod);
    SetInteger(pChild, "sortmethodroot", g_stSettings.m_MyVideoActorRootSortMethod);
    SetInteger(pChild, "sortorder", g_stSettings.m_MyVideoActorSortOrder);
    SetInteger(pChild, "sortorderroot", g_stSettings.m_MyVideoActorRootSortOrder);
    SetInteger(pChild, "showmode", g_stSettings.m_iMyVideoActorShowMode);
  }
  { // year window
    TiXmlElement childNode("year");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetInteger(pChild, "viewmethod", g_stSettings.m_MyVideoYearViewMethod);
    SetInteger(pChild, "viewmethodroot", g_stSettings.m_MyVideoYearRootViewMethod);
    SetInteger(pChild, "sortmethod", g_stSettings.m_MyVideoYearSortMethod);
    SetInteger(pChild, "sortmethodroot", g_stSettings.m_MyVideoYearRootSortMethod);
    SetInteger(pChild, "sortorder", g_stSettings.m_MyVideoYearSortOrder);
    SetInteger(pChild, "sortorderroot", g_stSettings.m_MyVideoYearRootSortOrder);
    SetInteger(pChild, "showmode", g_stSettings.m_iMyVideoYearShowMode);
  }
  { // title window
    TiXmlElement childNode("title");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetInteger(pChild, "viewmethod", g_stSettings.m_MyVideoTitleViewMethod);
    SetInteger(pChild, "sortmethod", g_stSettings.m_MyVideoTitleSortMethod);
    SetInteger(pChild, "sortorder", g_stSettings.m_MyVideoTitleSortOrder);
    SetInteger(pChild, "showmode", g_stSettings.m_iMyVideoTitleShowMode);
  }

  SetFloat(pNode, "subsdelayrange", g_stSettings.m_fSubsDelayRange);
  SetFloat(pNode, "audiodelayrange", g_stSettings.m_fAudioDelayRange);
  SetInteger(pNode, "smallstepbackseconds", g_stSettings.m_iSmallStepBackSeconds);
  SetInteger(pNode, "smallstepbacktries", g_stSettings.m_iSmallStepBackTries);
  SetInteger(pNode, "smallstepbackdelay", g_stSettings.m_iSmallStepBackDelay);

  SetInteger(pNode, "timeseekforward", g_stSettings.m_iMyVideoTimeSeekForward);
  SetInteger(pNode, "timeseekbackward", g_stSettings.m_iMyVideoTimeSeekBackward);
  SetInteger(pNode, "timeseekforwardbig", g_stSettings.m_iMyVideoTimeSeekForwardBig);
  SetInteger(pNode, "timeseekbackwardbig", g_stSettings.m_iMyVideoTimeSeekBackwardBig);

  SetInteger(pNode, "percentseekforward", g_stSettings.m_iMyVideoPercentSeekForward);
  SetInteger(pNode, "percentseekbackward", g_stSettings.m_iMyVideoPercentSeekBackward);
  SetInteger(pNode, "percentseekforwardbig", g_stSettings.m_iMyVideoPercentSeekForwardBig);
  SetInteger(pNode, "percentseekbackwardbig", g_stSettings.m_iMyVideoPercentSeekBackwardBig);
  
  // myscripts settings
  TiXmlElement scriptsNode("myscripts");
  pNode = pRoot->InsertEndChild(scriptsNode);
  if (!pNode) return false;
  SetInteger(pNode, "viewmethod", g_stSettings.m_ScriptsViewMethod);
  SetInteger(pNode, "sortmethod", g_stSettings.m_ScriptsSortMethod);
  SetInteger(pNode, "sortorder", g_stSettings.m_ScriptsSortOrder);

  // general settings
  TiXmlElement generalNode("general");
  pNode = pRoot->InsertEndChild(generalNode);
  if (!pNode) return false;
  SetString(pNode, "kaiarenapass", g_stSettings.szOnlineArenaPassword);
  SetString(pNode, "kaiarenadesc", g_stSettings.szOnlineArenaDescription);
  SetInteger(pNode, "systemtotaluptime", g_stSettings.m_iSystemTimeTotalUp);

  // screen settings
  TiXmlElement screenNode("screen");
  pNode = pRoot->InsertEndChild(screenNode);
  if (!pNode) return false;
  SetFloat(pNode, "zoomamount", g_stSettings.m_defaultVideoSettings.m_CustomZoomAmount);
  SetFloat(pNode, "pixelratio", g_stSettings.m_defaultVideoSettings.m_CustomPixelRatio);

  // audio settings
  TiXmlElement audioNode("audio");
  pNode = pRoot->InsertEndChild(audioNode);
  if (!pNode) return false;

  SetInteger(pNode, "volumelevel", g_stSettings.m_nVolumeLevel);

  SaveCalibration(pRoot);

  g_guiSettings.SaveXML(pRoot);

  SaveSkinSettings(pRoot);

  // save the file
  return xmlDoc.SaveFile(strSettingsFile);
}

bool CSettings::LoadProfile(int index)
{
  CProfile& profile = m_vecProfiles.at(index);
  if (LoadSettings("T:\\" + profile.getFileName(), false))
  {
    m_iLastLoadedProfileIndex = index;
    Save();
    return true;
  }
  return false;
}

void CSettings::DeleteProfile(int index)
{
  for (IVECPROFILES iProfile = g_settings.m_vecProfiles.begin(); iProfile != g_settings.m_vecProfiles.end(); ++iProfile)
  {
    if (iProfile == &g_settings.m_vecProfiles.at(index))
    {
      if (index == m_iLastLoadedProfileIndex) {m_iLastLoadedProfileIndex = -1;}
      ::DeleteFile("T:\\" + iProfile->getFileName());
      m_vecProfiles.erase(iProfile);
      Save();
      break;
    }
  }
}

bool CSettings::SaveSettingsToProfile(int index)
{
  CProfile& profile = m_vecProfiles.at(index);
  return SaveSettings("T:\\" + profile.getFileName(), false);
}


bool CSettings::LoadProfiles(const TiXmlElement* pRootElement, const CStdString& strSettingsFile)
{
  CLog::Log(LOGDEBUG, "Parsing <profiles> tag");
  const TiXmlElement *pChild = pRootElement->FirstChildElement("profiles");
  if (pChild)
  {
    GetInteger(pChild, "lastloaded", m_iLastLoadedProfileIndex, -1, -1, INT_MAX);
    TiXmlNode *pChildNode = pChild->FirstChild();
    while (pChildNode > 0)
    {
      CStdString strValue = pChildNode->Value();
      if (strValue == "profile")
      {
        const TiXmlNode *pProfileName = pChildNode->FirstChild("name");
        const TiXmlNode *pProfileFile = pChildNode->FirstChild("file");
        if (pProfileName && pProfileFile)
        {
          const char* szName = pProfileName->FirstChild()->Value();
          CLog::Log(LOGDEBUG, "    Profile Name: %s", szName);
          const char* szPath = pProfileFile->FirstChild()->Value();
          CLog::Log(LOGDEBUG, "    Profile Filename: %s", szPath);

          CProfile profile;
          CStdString str = szName;
          profile.setName(str);
          str = szPath;
          profile.setFileName(str);
          m_vecProfiles.push_back(profile);
        }
        else
        {
          CLog::Log(LOGERROR, "    <name> and/or <file> not properly defined within <profile>");
        }
      }
      pChildNode = pChildNode->NextSibling();
    }
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "  <profiles> tag is missing or %s is malformed", strSettingsFile.c_str());
    return false;
  }
}


bool CSettings::SaveProfiles(TiXmlNode* pRootElement) const
{
  TiXmlElement xmlProfilesElement("profiles");
  TiXmlNode *pProfileNode = pRootElement->InsertEndChild(xmlProfilesElement);
  if (!pProfileNode) return false;
  SetInteger(pProfileNode, "lastloaded", m_iLastLoadedProfileIndex);
  for (int i = 0; i < (int)m_vecProfiles.size(); ++i)
  {
    const CProfile& profile = m_vecProfiles.at(i);

    TiXmlElement profileElement("profile");
    TiXmlNode *pNode = pProfileNode->InsertEndChild(profileElement);
    if (!pNode) return false;
    SetString(pNode, "name", profile.getName());
    SetString(pNode, "file", profile.getFileName());
  }
  return true;
}

bool CSettings::LoadXml()
{
  // load xml file - we use the xbe path in case we were loaded as dash
  if (!xbmcXmlLoaded)
  {
    CStdString strPath;
    char szXBEFileName[1024];
    CIoSupport helper;
    helper.GetXbePath(szXBEFileName);
    strrchr(szXBEFileName, '\\')[0] = 0;
    strPath.Format("%s\\%s", szXBEFileName, "XboxMediaCenter.xml");
    if ( !xbmcXml.LoadFile( strPath.c_str() ) )
    {
      return false;
    }
    xbmcXmlLoaded = true;
  }
  return true;
}

bool CSettings::UpdateBookmark(const CStdString &strType, const CStdString &strOldName, const CStdString &strUpdateElement, const CStdString &strUpdateText)
{
  bool breturn(false);
  if (!LoadXml()) return false;

  VECSHARES *pShares = NULL;
  if (strType == "myprograms") pShares = &m_vecMyProgramsBookmarks;
  if (strType == "files") pShares = &m_vecMyFilesShares;
  if (strType == "music") pShares = &m_vecMyMusicShares;
  if (strType == "video") pShares = &m_vecMyVideoShares;
  if (strType == "pictures") pShares = &m_vecMyPictureShares;

  if (!pShares) return false;

  // disallow virtual paths
  if (strUpdateElement.Equals("path") && CUtil::IsVirualPath(strUpdateText))
    return false;

  for (IVECSHARES it = pShares->begin(); it != pShares->end(); it++)
  {
    if ((*it).strName == strOldName)
    {
      breturn = true;
      if ("name" == strUpdateElement)
        (*it).strName = strUpdateText;
      else if ("path" == strUpdateElement)
        (*it).strPath = strUpdateText;
      else if ("depth" == strUpdateElement)
        (*it).m_iDepthSize = atoi(strUpdateText);
      else if ("lockmode" == strUpdateElement)
        (*it).m_iLockMode = atoi(strUpdateText);
      else if ("lockcode" == strUpdateElement)
        (*it).m_strLockCode = strUpdateText;
      else if ("badpwdcount" == strUpdateElement)
        (*it).m_iBadPwdCount = atoi(strUpdateText);
      else
        return false;
      break;
    }
  }
  // Return bookmark of
  TiXmlElement *pRootElement = xbmcXml.RootElement();
  TiXmlNode *pNode = NULL;
  TiXmlNode *pIt = NULL;

  pNode = pRootElement->FirstChild(strType);

  // if valid bookmark, find child at pos (id)
  if (pNode && breturn)
  {
    pIt = pNode->FirstChild("bookmark");
    while (pIt)
    {
      TiXmlNode *pChild = pIt->FirstChild("name");
      if (pChild && pChild->FirstChild()->Value() == strOldName)
      {
        pChild = pIt->FirstChild(strUpdateElement);
        if (pChild)
        {
          pIt->FirstChild(strUpdateElement)->FirstChild()->SetValue(strUpdateText);
        }
        else
        {
          TiXmlText xmlText(strUpdateText);
          TiXmlElement eElement(strUpdateElement);
          eElement.InsertEndChild(xmlText);
          pIt->ToElement()->InsertEndChild(eElement);
        }
        break;
      }
      else
        pIt = pIt->NextSibling("bookmark");
    }
    return xbmcXml.SaveFile();
  }
  return breturn;
}

bool CSettings::UpDateXbmcXML(const CStdString &strFirstChild, const CStdString &strChild, const CStdString &strChildValue)
{
  bool breturn; breturn = false;
  if (!LoadXml()) return false;
  //<strFirstChild>
  //    <strChild>strChildValue</strChild>
  
  TiXmlElement *pRootElement = xbmcXml.RootElement();
  TiXmlNode *pNode = pRootElement->FirstChild(strFirstChild);;
  TiXmlNode *pIt = pNode->FirstChild(strChild);;
  if (pIt)
  {
    pIt->FirstChild()->SetValue(strChildValue);
    breturn = true;
  }
  else
  {
    TiXmlText xmlText(strChildValue);
    TiXmlElement eElement(strChild);
    eElement.InsertEndChild(xmlText);
    pNode->ToElement()->InsertEndChild(eElement);
    breturn = true;
  }
  if(breturn)
    return xbmcXml.SaveFile();
  else return false;
}
bool CSettings::UpDateXbmcXML(const CStdString &strFirstChild, const CStdString &strFirstChildValue)
{
  if (!LoadXml()) return false;
  
  //
  //<strFirstChild>strFirstChildValue<strFirstChild>
  
  TiXmlElement *pRootElement = xbmcXml.RootElement();
  TiXmlNode *pNode = pRootElement->FirstChild(strFirstChild);;
  if (pNode)
  {
    pNode->FirstChild()->SetValue(strFirstChildValue);
    return xbmcXml.SaveFile();
  }
  else return false;
}  
bool CSettings::DeleteBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath)
{
   bool breturn; breturn = false;
  if (!LoadXml()) return false;

  VECSHARES *pShares = NULL;
  if (strType == "myprograms") pShares = &m_vecMyProgramsBookmarks;
  if (strType == "files") pShares = &m_vecMyFilesShares;
  if (strType == "music") pShares = &m_vecMyMusicShares;
  if (strType == "video") pShares = &m_vecMyVideoShares;
  if (strType == "pictures") pShares = &m_vecMyPictureShares;

  if (!pShares) return false;

  for (IVECSHARES it = pShares->begin(); it != pShares->end(); it++)
  {
    if ((*it).strName == strName && (*it).strPath == strPath)
    {
      CLog::Log(LOGDEBUG,"found share, removing!");
      pShares->erase(it);
      breturn = true;
      break;
    }
  }
  // Return bookmark of
  if (breturn)
  {
    TiXmlElement *pRootElement = xbmcXml.RootElement();
    TiXmlNode *pNode = NULL;
    TiXmlNode *pIt = NULL;

    pNode = pRootElement->FirstChild(strType);

    // if valid bookmark, find child at pos (id)
    if (pNode)
    {
      pIt = pNode->FirstChild("bookmark");
      while (pIt)
      {
        TiXmlNode *pChild = pIt->FirstChild("name");
        if (pChild && pChild->FirstChild()->Value() == strName)
        {
          pChild->FirstChild()->SetValue(strName);
          pChild = pIt->FirstChild("path");
          if (pChild && pChild->FirstChild()->Value() == strPath)
          {
            pNode->RemoveChild(pIt);
            break;
          }
        }
        pIt = pIt->NextSibling("bookmark");
      }
    }
    return xbmcXml.SaveFile();
  }
  else return breturn;
}

bool CSettings::AddBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath)
{
  return AddBookmark(strType, strName, strPath, 0);
}

bool CSettings::AddBookmark(const CStdString &strType, const CStdString &strName, const CStdString &strPath, const int iDepth)
{
  if (!LoadXml()) return false;

  VECSHARES *pShares = NULL;
  if (strType == "myprograms") pShares = &m_vecMyProgramsBookmarks;
  if (strType == "files") pShares = &m_vecMyFilesShares;
  if (strType == "music") pShares = &m_vecMyMusicShares;
  if (strType == "video") pShares = &m_vecMyVideoShares;
  if (strType == "pictures") pShares = &m_vecMyPictureShares;

  if (!pShares) return false;

  CShare share;
  share.strName = strName;
  share.strPath = strPath;
  share.m_iBufferSize = 0;
  share.m_iDepthSize = 1;
  bool bSaveDepth = false;
  if (iDepth > 0)
  {
    share.m_iDepthSize = iDepth;
    bSaveDepth = true;
  }

  // translate dir
  ConvertHomeVar(share.strPath);
  CStdString strPath1 = share.strPath;
  strPath1.ToUpper();

  if (strPath1.at(0) == '$')
  {
    share.strPath = CUtil::TranslateSpecialDir(strPath1);
    if (!share.strPath.IsEmpty())
      CLog::Log(LOGDEBUG,"AddBookmark: Translated (%s) to Path (%s)",strPath.c_str(),share.strPath.c_str());
    else
    {
      CLog::Log(LOGDEBUG,"AddBookmark: Skipping invalid special directory token: %s",strPath.c_str());
      return false;
    }
  }

  if (strPath1.Left(4) == "UDF:")
  {
    share.m_iDriveType = SHARE_TYPE_VIRTUAL_DVD;
    share.strPath = "D:\\";
  }
  else if (strPath1.Left(11) == "SOUNDTRACK:")
    share.m_iDriveType = SHARE_TYPE_LOCAL;
  else if (CUtil::IsISO9660(share.strPath))
    share.m_iDriveType = SHARE_TYPE_VIRTUAL_DVD;
  else if (CUtil::IsDVD(share.strPath))
    share.m_iDriveType = SHARE_TYPE_DVD;
  else if (CUtil::IsRemote(share.strPath))
    share.m_iDriveType = SHARE_TYPE_REMOTE;
  else if (CUtil::IsHD(share.strPath))
    share.m_iDriveType = SHARE_TYPE_LOCAL;
  else
    share.m_iDriveType = SHARE_TYPE_UNKNOWN;
  // Initialize the lock settings to unlocked state
  share.m_iLockMode = 0;
  share.m_strLockCode = "";
  share.m_iBadPwdCount = 0;

  pShares->push_back(share);

  // Add to the xml file
  TiXmlElement *pRootElement = xbmcXml.RootElement();
  TiXmlNode *pNode = NULL;

  pNode = pRootElement->FirstChild(strType);

  // create a new Element
  TiXmlText xmlName(strName);
  TiXmlText xmlPath(strPath);
  CStdString strDepth;
  strDepth.Format("%i", share.m_iDepthSize);
  TiXmlText xmlDepth(strDepth);
  TiXmlElement eName("name");
  TiXmlElement ePath("path");
  TiXmlElement eDepth("depth");
  eName.InsertEndChild(xmlName);
  ePath.InsertEndChild(xmlPath);
  if (bSaveDepth)
    eDepth.InsertEndChild(xmlDepth);

  TiXmlElement bookmark("bookmark");
  bookmark.InsertEndChild(eName);
  bookmark.InsertEndChild(ePath);
  if (bSaveDepth)
    bookmark.InsertEndChild(eDepth);

  if (pNode)
  {
    pNode->ToElement()->InsertEndChild(bookmark);
  }
  return xbmcXml.SaveFile();
}

bool CSettings::SetBookmarkLocks(const CStdString &strType, bool bEngageLocks)
{
  if (!LoadXml()) return false;

  VECSHARES *pShares = NULL;
  if (strType == "files") pShares = &g_settings.m_vecMyFilesShares;
  if (strType == "music") pShares = &g_settings.m_vecMyMusicShares;
  if (strType == "video") pShares = &g_settings.m_vecMyVideoShares;
  if (strType == "pictures") pShares = &g_settings.m_vecMyPictureShares;
  if (strType == "myprograms") pShares = &g_settings.m_vecMyProgramsBookmarks;

  if (!pShares) return false;

  for (IVECSHARES it = pShares->begin(); it != pShares->end(); it++)
  {
    if ((*it).m_iLockMode < 0 && bEngageLocks)
      (*it).m_iLockMode = (*it).m_iLockMode * -1;
    else if ((*it).m_iLockMode > 0 && !bEngageLocks)
      (*it).m_iLockMode = (*it).m_iLockMode * -1;
  }
  // Return bookmark of
  TiXmlElement *pRootElement = xbmcXml.RootElement();
  TiXmlNode *pNode = NULL;
  TiXmlNode *pIt = NULL;

  pNode = pRootElement->FirstChild(strType);
  bool bXmlChanged = false;

  // if valid bookmark, find child at pos (id)
  if (pNode)
  {
    pIt = pNode->FirstChild("bookmark");
    while (pIt)
    {
      TiXmlNode *pChild = pIt->FirstChild("lockmode");
      if (pChild)
      {
        CStdString strLockModeValue = pChild->FirstChild()->Value();
        if (strLockModeValue.Mid(0, 1) == "-" && bEngageLocks)
        {
          strLockModeValue = strLockModeValue.Mid(1, strlen(strLockModeValue) - 1);
          pIt->FirstChild("lockmode")->FirstChild()->SetValue(strLockModeValue);
          bXmlChanged = true;
        }
        else if (strLockModeValue.Mid(0, 1) != "-" && !bEngageLocks)
        {
          strLockModeValue = "-" + strLockModeValue;
          pIt->FirstChild("lockmode")->FirstChild()->SetValue(strLockModeValue);
          bXmlChanged = true;
        }
      }
      pIt = pIt->NextSibling("bookmark");
    }
  }
  if (bXmlChanged)
    return xbmcXml.SaveFile();
  else
    return true;
}

void CSettings::LoadSkinSettings(const TiXmlElement* pRootElement)
{
  TiXmlElement *pElement = pRootElement->FirstChildElement("skinsettings");
  if (pElement)
  {
    TiXmlElement *pChild = pElement->FirstChildElement("setting");
    while (pChild && pChild->FirstChild())
    {
      CStdString settingName = pChild->Attribute("name");
      bool settingData = strcmpi(pChild->FirstChild()->Value(), "true") == 0;
      m_skinSettings.insert(pair<CStdString, bool>(settingName, settingData));
      pChild = pChild->NextSiblingElement("setting");
    }
  }
}

void CSettings::SaveSkinSettings(TiXmlNode *pRootElement) const
{
  // add the <skinsettings> tag
  TiXmlElement xmlSettingsElement("skinsettings");
  TiXmlNode *pSettingsNode = pRootElement->InsertEndChild(xmlSettingsElement);
  if (!pSettingsNode) return;
  for (std::map<CStdString, bool>::const_iterator it = m_skinSettings.begin(); it != m_skinSettings.end(); ++it)
  {
    // Add a <setting name="name">true/false</setting>
    TiXmlElement xmlSetting("setting");
    xmlSetting.SetAttribute("name", (*it).first);
    TiXmlText xmlBool((*it).second ? "true" : "false");
    xmlSetting.InsertEndChild(xmlBool);
    pSettingsNode->InsertEndChild(xmlSetting);
  }
}

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
void CSettings::LoadHomeButtons(TiXmlElement* pRootElement)
{
  TiXmlElement *pElement = pRootElement->FirstChildElement("homebuttons");
  if (pElement)
  {
    TiXmlElement *pChild = pElement->FirstChildElement("button");
    while (pChild)
    {
      char temp[1024];
      int iIcon;
      GetString(pChild, "execute", temp, "");
      GetInteger(pChild, "icon", iIcon, ICON_TYPE_NONE, ICON_TYPE_NONE, ICON_TYPE_NONE + 19);
      char temp2[1024];
      GetString(pChild, "label", temp2, "");
      CButtonScrollerSettings::CButton *pButton = NULL;
      if ((temp2[0] >= 'A') && (temp2[0] <= 'z'))
      {
        WCHAR wszLabel[1024];
        swprintf(wszLabel, L"%S", temp2);
        pButton = new CButtonScrollerSettings::CButton(wszLabel, temp, iIcon);
      }
      else
      {
        DWORD dwLabelID = atol(temp2);
        pButton = new CButtonScrollerSettings::CButton(dwLabelID, temp, iIcon);
      }
      if (pButton->m_strExecute.size() > 0)
        g_settings.m_buttonSettings.m_vecButtons.push_back(pButton);
      else
        delete pButton;

      pChild = pChild->NextSiblingElement();
    }
    GetInteger(pElement, "default", g_settings.m_buttonSettings.m_iDefaultButton, 0, 0, (int)g_settings.m_buttonSettings.m_vecButtons.size());
  }
  if (g_settings.m_buttonSettings.m_vecButtons.size() == 0)
  { // fill in the defaults
    CButtonScrollerSettings::CButton *button1 = new CButtonScrollerSettings::CButton(0, "XBMC.ActivateWindow(1)", ICON_TYPE_PROGRAMS);
    CButtonScrollerSettings::CButton *button2 = new CButtonScrollerSettings::CButton(7, "XBMC.ActivateWindow(3)", ICON_TYPE_FILES);
    CButtonScrollerSettings::CButton *button3 = new CButtonScrollerSettings::CButton(1, "XBMC.ActivateWindow(2)", ICON_TYPE_PICTURES);
    CButtonScrollerSettings::CButton *button4 = new CButtonScrollerSettings::CButton(2, "XBMC.ActivateWindow(501)", ICON_TYPE_MUSIC);
    CButtonScrollerSettings::CButton *button5 = new CButtonScrollerSettings::CButton(3, "XBMC.ActivateWindow(6)", ICON_TYPE_VIDEOS);
    CButtonScrollerSettings::CButton *button6 = new CButtonScrollerSettings::CButton(8, "XBMC.ActivateWindow(2600)", ICON_TYPE_WEATHER);
    CButtonScrollerSettings::CButton *button7 = new CButtonScrollerSettings::CButton(5, "XBMC.ActivateWindow(4)", ICON_TYPE_SETTINGS);
    g_settings.m_buttonSettings.m_vecButtons.push_back(button1);
    g_settings.m_buttonSettings.m_vecButtons.push_back(button2);
    g_settings.m_buttonSettings.m_vecButtons.push_back(button3);
    g_settings.m_buttonSettings.m_vecButtons.push_back(button4);
    g_settings.m_buttonSettings.m_vecButtons.push_back(button5);
    g_settings.m_buttonSettings.m_vecButtons.push_back(button6);
    g_settings.m_buttonSettings.m_vecButtons.push_back(button7);
    g_settings.m_buttonSettings.m_iDefaultButton = 0;
  }
}
#endif

bool CSettings::LoadFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders)
{ // load xml file...
  CStdString strXMLFile = "T:\\";
  strXMLFile += strFolderXML;

  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( strXMLFile.c_str() ) )
  {
    CLog::Log(LOGERROR, "LoadFolderViews - Unable to load XML file %s", strFolderXML.c_str());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if ( strValue != "folderviews")
  {
    g_LoadErrorStr.Format("%s Doesn't contain <folderviews>", strXMLFile.c_str());
    return false;
  }

  // cleanup vecFolders if necessary...
  if (vecFolders.size())
  {
    for (unsigned int i = 0; i < vecFolders.size(); i++)
      delete vecFolders[i];
    vecFolders.clear();
  }
  // parse the view mode for each folder
  const TiXmlNode *pChild = pRootElement->FirstChild("folder");
  while (pChild)
  {
    CStdString strPath;
    int iView = VIEW_METHOD_LIST;
    int iSort = 0;
    bool bSortUp = true;
    TiXmlNode *pPath = pChild->FirstChild("path");
    if (pPath && pPath->FirstChild())
      strPath = pPath->FirstChild()->Value();
    TiXmlNode *pView = pChild->FirstChild("view");
    if (pView && pView->FirstChild())
      iView = atoi(pView->FirstChild()->Value());
    TiXmlNode *pSort = pChild->FirstChild("sort");
    if (pSort && pSort->FirstChild())
      iSort = atoi(pSort->FirstChild()->Value());
    TiXmlNode *pDirection = pChild->FirstChild("direction");
    if (pDirection && pDirection->FirstChild())
      bSortUp = pDirection->FirstChild()->Value() == "up";
    // fill in element
    if (!strPath.IsEmpty())
    {
      CFolderView *pFolderView = new CFolderView(strPath, iView, iSort, bSortUp);
      if (pFolderView)
        vecFolders.push_back(pFolderView);
    }
    // run to next <folder> element
    pChild = pChild->NextSibling("folder");
  }
  return true;
}

bool CSettings::SaveFolderViews(const CStdString &strFolderXML, VECFOLDERVIEWS &vecFolders)
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("folderviews");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  // write our folders one by one
  CStdString strView, strSort, strSortUp;
  for (unsigned int i = 0; i < vecFolders.size(); i++)
  {
    CFolderView *pFolderView = vecFolders[i];
    strView.Format("%i", pFolderView->m_iView);
    strSort.Format("%i", pFolderView->m_iSort);
    strSortUp = pFolderView->m_bSortOrder ? "up" : "down";

    TiXmlText xmlPath(pFolderView->m_strPath.IsEmpty() ? "ROOT" : pFolderView->m_strPath);

    TiXmlText xmlView(strView);
    TiXmlText xmlSort(strSort);
    TiXmlText xmlSortUp(strSortUp);

    TiXmlElement ePath("path");
    TiXmlElement eView("view");
    TiXmlElement eSort("sort");
    TiXmlElement eSortUp("direction");

    ePath.InsertEndChild(xmlPath);
    eView.InsertEndChild(xmlView);
    eSort.InsertEndChild(xmlSort);
    eSortUp.InsertEndChild(xmlSortUp);

    TiXmlElement folderNode("folder");
    folderNode.InsertEndChild(ePath);
    folderNode.InsertEndChild(eView);
    folderNode.InsertEndChild(eSort);
    folderNode.InsertEndChild(eSortUp);

    pRoot->InsertEndChild(folderNode);
  }
  return xmlDoc.SaveFile(strFolderXML);
}

void CSettings::Clear()
{
  m_vecMyProgramsBookmarks.clear();
  m_vecMyPictureShares.clear();
  m_vecMyFilesShares.clear();
  m_vecMyMusicShares.clear();
  m_vecMyVideoShares.clear();
  m_vecSambeShres.clear();
  m_vecIcons.clear();
  m_vecProfiles.clear();
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  m_buttonSettings.Clear();
#endif
  m_szMyVideoStackTokensArray.clear();
  m_szMyVideoCleanTokensArray.clear();
  m_MyVideoStackRegExps.clear();
  m_mapRssUrls.clear();
  xbmcXml.Clear();
  m_skinSettings.clear();
}

bool CSettings::GetSkinSetting(const char *setting) const
{
  std::map<CStdString, bool>::const_iterator it = m_skinSettings.find(setting);
  if (it != m_skinSettings.end())
  {
    return (*it).second;
  }
  return false;
}

void CSettings::ToggleSkinSetting(const char *setting)
{
  CStdString settingName;
  settingName.Format("%s.%s", g_guiSettings.GetString("LookAndFeel.Skin").c_str(), setting);
  std::map<CStdString, bool>::iterator it = m_skinSettings.find(settingName);
  if (it != m_skinSettings.end())
  {
    (*it).second = !(*it).second;
  }
  else
  { // insert setting
    m_skinSettings.insert(pair<CStdString, bool>(settingName, true)); // defaults are false
  }
}
