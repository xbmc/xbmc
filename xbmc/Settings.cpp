/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "Settings.h"
#include "Application.h"
#include "KeyboardLayoutConfiguration.h"
#include "Util.h"
#include "URL.h"
#include "GUIWindowFileManager.h"
#include "GUIDialogButtonMenu.h"
#include "GUIDialogContentSettings.h"
#include "GUIFontManager.h"
#include "LangCodeExpander.h"
#include "ButtonTranslator.h"
#include "XMLUtils.h"
#include "GUIPassword.h"
#include "GUIAudioManager.h"
#include "AudioContext.h"
#include "utils/GUIInfoManager.h"
#include "xbox/Network.h"
#include "FileSystem/MultiPathDirectory.h"
#include "FileSystem/SpecialProtocol.h"
#include "GUIBaseContainer.h" // for VIEW_TYPE enum
#include "utils/FanController.h"
#include "MediaManager.h"
#include "XBVideoConfig.h"
#include "DNSNameCache.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "FileSystem/Directory.h"
#include "FileItem.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/MemoryUnitManager.h"
#endif

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;
using namespace MEDIA_DETECT;

struct CSettings::stSettings g_stSettings;
struct CSettings::AdvancedSettings g_advancedSettings;
class CSettings g_settings;

extern CStdString g_LoadErrorStr;

CSettings::CSettings(void)
{
}

void CSettings::Initialize()
{
  for (int i = HDTV_1080i; i <= PAL60_16x9; i++)
  {
    g_graphicsContext.ResetScreenParameters((RESOLUTION)i);
    g_graphicsContext.ResetOverscan((RESOLUTION)i, m_ResInfo[i].Overscan);
  }

  g_stSettings.m_iMyVideoStack = STACK_NONE;

  strcpy(g_stSettings.szOnlineArenaPassword, "");
  strcpy(g_stSettings.szOnlineArenaDescription, "It's Good To Play Together!");

  g_stSettings.m_bMyMusicSongInfoInVis = true;    // UNUSED - depreciated.
  g_stSettings.m_bMyMusicSongThumbInVis = false;  // used for music info in vis screen

  g_stSettings.m_bMyMusicPlaylistRepeat = false;
  g_stSettings.m_bMyMusicPlaylistShuffle = false;

  g_stSettings.m_bMyVideoPlaylistRepeat = false;
  g_stSettings.m_bMyVideoPlaylistShuffle = false;
  g_stSettings.m_bMyVideoNavFlatten = false;

  g_stSettings.m_nVolumeLevel = 0;
  g_stSettings.m_dynamicRangeCompressionLevel = 0;
  g_stSettings.m_iPreMuteVolumeLevel = 0;
  g_stSettings.m_bMute = false;
  g_stSettings.m_fZoomAmount = 1.0f;
  g_stSettings.m_fPixelRatio = 1.0f;

  g_stSettings.m_pictureExtensions = ".png|.jpg|.jpeg|.bmp|.gif|.ico|.tif|.tiff|.tga|.pcx|.cbz|.zip|.cbr|.rar|.m3u";
  g_stSettings.m_musicExtensions = ".nsv|.m4a|.flac|.aac|.strm|.pls|.rm|.rma|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u|.mod|.amf|.669|.dmf|.dsm|.far|.gdm|.imf|.it|.m15|.med|.okt|.s3m|.stm|.sfx|.ult|.uni|.xm|.sid|.ac3|.dts|.cue|.aif|.aiff|.wpl|.ape|.mac|.mpc|.mp+|.mpp|.shn|.zip|.rar|.wv|.nsf|.spc|.gym|.adplug|.adx|.dsp|.adp|.ymf|.ast|.afc|.hps|.xsp|.xwav|.waa|.wvs|.wam|.gcm|.idsp|.mpdsp|.mss|.spt|.rsd|.mid|.kar|.sap|.cmc|.cmr|.dmc|.mpt|.mpd|.rmt|.tmc|.tm8|.tm2|.oga|.url|.pxml";
  g_stSettings.m_videoExtensions = ".m4v|.3g2|.3gp|.nsv|.ts|.ty|.strm|.pls|.rm|.rmvb|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.nrg|.img|.iso|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mp4|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli|.flv|.rar|.001|.wpl|.zip|.vdr|.dvr-ms|.xsp|.mts|.m2t|.m2ts|.evo|.ogv|.sdp|.avs|.rec|.url|.pxml";
  // internal music extensions
  g_stSettings.m_musicExtensions += "|.sidstream|.oggstream|.nsfstream|.asapstream|.cdda";

  // This shouldn't be set here but in CApp::Create!!!!!
//  g_stSettings.m_logFolder = "Q:\\";              // log file location
  g_stSettings.m_logFolder = "";

  m_iLastLoadedProfileIndex = 0;

  // defaults for scanning
  g_stSettings.m_bMyMusicIsScanning = false;

  g_stSettings.iAdditionalSubtitleDirectoryChecked = 0;

  g_settings.bUseLoginScreen = false;

  // Advanced settings
  g_advancedSettings.m_useMultipaths = true;
  g_advancedSettings.m_DisableModChipDetection = true;

  g_advancedSettings.m_audioHeadRoom = 0;
  g_advancedSettings.m_ac3Gain = 12.0f;
  g_advancedSettings.m_audioDefaultPlayer = "paplayer";
  
  g_advancedSettings.m_videoSubsDelayRange = 10;
  g_advancedSettings.m_videoAudioDelayRange = 10;
  g_advancedSettings.m_videoSmallStepBackSeconds = 7;
  g_advancedSettings.m_videoSmallStepBackTries = 3;
  g_advancedSettings.m_videoSmallStepBackDelay = 300;
  g_advancedSettings.m_videoUseTimeSeeking = true;
  g_advancedSettings.m_videoTimeSeekForward = 30;
  g_advancedSettings.m_videoTimeSeekBackward = -30;
  g_advancedSettings.m_videoTimeSeekForwardBig = 600;
  g_advancedSettings.m_videoTimeSeekBackwardBig = -600;
  g_advancedSettings.m_videoPercentSeekForward = 2;
  g_advancedSettings.m_videoPercentSeekBackward = -2;
  g_advancedSettings.m_videoPercentSeekForwardBig = 10;
  g_advancedSettings.m_videoPercentSeekBackwardBig = -10;
  g_advancedSettings.m_videoBlackBarColour = 1;
  g_advancedSettings.m_videoPPFFmpegType = "linblenddeint";
  g_advancedSettings.m_videoDefaultPlayer = "mplayer";
  
  g_advancedSettings.m_musicUseTimeSeeking = true;
  g_advancedSettings.m_musicTimeSeekForward = 10;
  g_advancedSettings.m_musicTimeSeekBackward = -10;
  g_advancedSettings.m_musicTimeSeekForwardBig = 60;
  g_advancedSettings.m_musicTimeSeekBackwardBig = -60;
  g_advancedSettings.m_musicPercentSeekForward = 1;
  g_advancedSettings.m_musicPercentSeekBackward = -1;
  g_advancedSettings.m_musicPercentSeekForwardBig = 10;
  g_advancedSettings.m_musicPercentSeekBackwardBig = -10;
  g_advancedSettings.m_musicResample = 48000;

  g_advancedSettings.m_cacheMemBufferSize = 256;
      
  g_advancedSettings.m_slideshowPanAmount = 2.5f;
  g_advancedSettings.m_slideshowZoomAmount = 5.0f;
  g_advancedSettings.m_slideshowBlackBarCompensation = 20.0f;

  g_advancedSettings.m_lcdRows = 4;
  g_advancedSettings.m_lcdColumns = 20;
  g_advancedSettings.m_lcdAddress1 = 0;
  g_advancedSettings.m_lcdAddress2 = 0x40;
  g_advancedSettings.m_lcdAddress3 = 0x14;
  g_advancedSettings.m_lcdAddress4 = 0x54;

  g_advancedSettings.m_autoDetectPingTime = 30;
  g_advancedSettings.m_playCountMinimumPercent = 90.0f;

  g_advancedSettings.m_songInfoDuration = 10;
  g_advancedSettings.m_busyDialogDelay = 2000;
  g_advancedSettings.m_logLevel = LOG_LEVEL_NORMAL;
  g_advancedSettings.m_cddbAddress = "freedb.freedb.org";
  g_advancedSettings.m_usePCDVDROM = false;
  g_advancedSettings.m_noDVDROM = false;
  g_advancedSettings.m_cachePath = "Z:\\";
  g_advancedSettings.m_displayRemoteCodes = false;
  
  g_advancedSettings.m_videoCleanRegExps.push_back("[ _\\,\\.\\(\\)\\[\\]\\-](ac3|dts|custom|dc|divx|divx5|dsr|dsrip|dutch|dvd|dvdrip|dvdscr|dvdscreener|screener|dvdivx|cam|fragment|fs|hdtv|hdrip|hdtvrip|internal|limited|multisubs|ntsc|ogg|ogm|pal|pdtv|proper|repack|rerip|retail|r3|r5|bd5|se|svcd|swedish|german|read.nfo|nfofix|unrated|ws|telesync|ts|telecine|tc|brrip|bdrip|480p|480i|576p|576i|720p|720i|1080p|1080i|hrhd|hrhdtv|hddvd|bluray|x264|h264|xvid|xvidvd|xxx|www.www|cd[1-9]|\\[.*\\])([ _\\,\\.\\(\\)\\[\\]\\-]|$)");
  g_advancedSettings.m_videoCleanRegExps.push_back("(\\[.*\\])");

  g_advancedSettings.m_moviesExcludeFromScanRegExps.push_back("[-\\._ ](sample|trailer)[-\\._ ]");
  g_advancedSettings.m_tvshowExcludeFromScanRegExps.push_back("[-\\._ ]sample[-\\._ ]");

  g_advancedSettings.m_videoStackRegExps.push_back("[ _\\.-]+cd[ _\\.-]*([0-9a-d]+)");
  g_advancedSettings.m_videoStackRegExps.push_back("[ _\\.-]+dvd[ _\\.-]*([0-9a-d]+)");
  g_advancedSettings.m_videoStackRegExps.push_back("[ _\\.-]+part[ _\\.-]*([0-9a-d]+)");
  g_advancedSettings.m_videoStackRegExps.push_back("[ _\\.-]+dis[ck][ _\\.-]*([0-9a-d]+)");
  g_advancedSettings.m_videoStackRegExps.push_back("()[ _\\.-]+([0-9]*[abcd]+)(\\.....?)$"); // can anyone explain this one?  should this be ([0-9a-d]+) ?
  g_advancedSettings.m_videoStackRegExps.push_back("()cd([0-9a-d]+)(\\.....?)$");
  g_advancedSettings.m_videoStackRegExps.push_back("([a-z])([0-9]+)(\\.....?)$");
  g_advancedSettings.m_videoStackRegExps.push_back("()([a-c])(\\.....?)$");

  // foo_[s01]_[e01]
  g_advancedSettings.m_tvshowStackRegExps.push_back("\\[[Ss]([0-9]+)\\]_\\[[Ee]([0-9]+)\\]?([^\\\\/]*)$");
  // foo.1x09* or just /1x09*
  g_advancedSettings.m_tvshowStackRegExps.push_back("[\\\\/\\._ \\[-]([0-9]+)x([0-9]+)([^\\\\/]*)$");
  // foo.s01.e01, foo.s01_e01, S01E02 foo
  g_advancedSettings.m_tvshowStackRegExps.push_back("[Ss]([0-9]+)[\\.-]?[Ee]([0-9]+)([^\\\\/]*)$");
  // foo.103*, 103 foo
  g_advancedSettings.m_tvshowStackRegExps.push_back("[\\\\/\\._ -]([0-9]+)([0-9][0-9])([\\._ -][^\\\\/]*)$");

  g_advancedSettings.m_tvshowMultiPartStackRegExp = "^[-EeXx]+([0-9]+)";

  g_advancedSettings.m_remoteRepeat = 480;
  g_advancedSettings.m_controllerDeadzone = 0.2f;
  g_advancedSettings.m_FTPShowCache = false;

  g_advancedSettings.m_playlistAsFolders = true;
  g_advancedSettings.m_detectAsUdf = false;

  g_advancedSettings.m_thumbSize = 192;

  g_advancedSettings.m_sambaclienttimeout = 10;
  g_advancedSettings.m_sambadoscodepage = "";
  g_advancedSettings.m_sambastatfiles = true;

  g_advancedSettings.m_bHTTPDirectoryLocalMode = false;
  g_advancedSettings.m_bHTTPDirectoryStatFilesize = false;

  g_advancedSettings.m_musicThumbs = "folder.jpg|Folder.jpg|folder.JPG|Folder.JPG|cover.jpg|Cover.jpg|cover.jpeg";
  g_advancedSettings.m_dvdThumbs = "folder.jpg|Folder.jpg|folder.JPG|Folder.JPG";

  g_advancedSettings.m_bMusicLibraryHideAllItems = false;
  g_advancedSettings.m_bMusicLibraryAllItemsOnBottom = false;
  g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear = false;
  g_advancedSettings.m_strMusicLibraryAlbumFormat = "";
  g_advancedSettings.m_strMusicLibraryAlbumFormatRight = "";
  g_advancedSettings.m_prioritiseAPEv2tags = false;
  g_advancedSettings.m_musicItemSeparator = " / ";
  g_advancedSettings.m_videoItemSeparator = " / ";

  g_advancedSettings.m_bVideoLibraryHideAllItems = false;
  g_advancedSettings.m_bVideoLibraryAllItemsOnBottom = false;
  g_advancedSettings.m_bVideoLibraryHideRecentlyAddedItems = false;
  g_advancedSettings.m_bVideoLibraryHideEmptySeries = false;
  g_advancedSettings.m_bVideoLibraryCleanOnUpdate = false;

  g_advancedSettings.m_bUseEvilB = true;

  g_advancedSettings.m_bTuxBoxAudioChannelSelection = false;
  g_advancedSettings.m_bTuxBoxSubMenuSelection = false;
  g_advancedSettings.m_bTuxBoxPictureIcon= true;
  g_advancedSettings.m_bTuxBoxSendAllAPids= false;
  g_advancedSettings.m_iTuxBoxEpgRequestTime = 10; //seconds
  g_advancedSettings.m_iTuxBoxDefaultSubMenu = 4;
  g_advancedSettings.m_iTuxBoxDefaultRootMenu = 0; //default TV Mode
  g_advancedSettings.m_iTuxBoxZapWaitTime = 0; // Time in sec. Default 0:OFF

  g_advancedSettings.m_curlconnecttimeout = 10;
  g_advancedSettings.m_curllowspeedtime = 5;

  g_advancedSettings.m_playlistRetries = 100;
  g_advancedSettings.m_playlistTimeout = 20; // 20 seconds timeout
  g_advancedSettings.m_iSkipLoopFilter = 0;
  g_advancedSettings.m_bVirtualShares = true;
  g_advancedSettings.m_bNavVKeyboard = false;

  g_advancedSettings.m_bPythonVerbose = false;
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
  if (!SaveSettings(GetSettingsFile()))
  {
    CLog::Log(LOGERROR, "Unable to save settings to %s", GetSettingsFile().c_str());
  }
}

bool CSettings::Reset()
{
  CLog::Log(LOGINFO, "Resetting settings");
  CFile::Delete(GetSettingsFile());
  Save();
  return LoadSettings(GetSettingsFile());
}

bool CSettings::Load(bool& bXboxMediacenter, bool& bSettings)
{
  // load settings file...
  bXboxMediacenter = bSettings = false;

#ifdef _XBOX
  char szDevicePath[1024]; 
  CStdString strMnt = _P(GetProfileUserDataFolder()); 
  if (strMnt.Left(2).Equals("Q:")) 
  { 
    CUtil::GetHomePath(strMnt); 
    strMnt += _P(GetProfileUserDataFolder()).substr(2); 
  } 
  CIoSupport::GetPartition(strMnt.c_str()[0], szDevicePath); 
  strcat(szDevicePath,strMnt.c_str()+2); 
  CIoSupport::RemapDriveLetter('P', szDevicePath); 
#endif
  CSpecialProtocol::SetProfilePath(GetProfileUserDataFolder());
  CLog::Log(LOGNOTICE, "loading %s", GetSettingsFile().c_str());
  if (!LoadSettings(GetSettingsFile()))
  {
    CLog::Log(LOGERROR, "Unable to load %s, creating new %s with default values", GetSettingsFile().c_str(), GetSettingsFile().c_str());
    Save();
    if (!(bSettings = Reset()))
      return false;
  }

  // clear sources, then load xml file...
  m_fileSources.clear();
  m_musicSources.clear();
  m_pictureSources.clear();
  m_programSources.clear();
  m_videoSources.clear();
  CStdString strXMLFile = GetSourcesFile();
  CLog::Log(LOGNOTICE, "%s", strXMLFile.c_str());
  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;
  if ( xmlDoc.LoadFile( strXMLFile ) )
  {
    pRootElement = xmlDoc.RootElement();
    CStdString strValue;
    if (pRootElement)
      strValue = pRootElement->Value();
    if ( strValue != "sources")
      CLog::Log(LOGERROR, "%s sources.xml file does not contain <sources>", __FUNCTION__);
  }
  else if (CFile::Exists(strXMLFile))
    CLog::Log(LOGERROR, "%s Error loading %s: Line %d, %s", __FUNCTION__, strXMLFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());

  // look for external sources file
  TiXmlNode *pInclude = pRootElement ? pRootElement->FirstChild("remote") : NULL;
  if (pInclude)
  {
    CStdString strRemoteFile = pInclude->FirstChild()->Value();
    if (!strRemoteFile.IsEmpty())
    {
      CLog::Log(LOGDEBUG, "Found <remote> tag");
      CLog::Log(LOGDEBUG, "Attempting to retrieve remote file: %s", strRemoteFile.c_str());
      // sometimes we have to wait for the network
      if (!g_network.IsAvailable(true) && CFile::Exists(strRemoteFile))
      {
        if ( xmlDoc.LoadFile(strRemoteFile) )
        {
          pRootElement = xmlDoc.RootElement();
          CStdString strValue;
          if (pRootElement)
            strValue = pRootElement->Value();
          if ( strValue != "sources")
            CLog::Log(LOGERROR, "%s remote_sources.xml file does not contain <sources>", __FUNCTION__);
        }
        else
          CLog::Log(LOGERROR, "%s unable to load file: %s, Line %d, %s", __FUNCTION__, strRemoteFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
      }
      else
        CLog::Log(LOGNOTICE, "Could not retrieve remote file, defaulting to local sources");
    }
  }

  if (pRootElement)
  { // parse sources...
    GetSources(pRootElement, "programs", m_programSources, m_defaultProgramSource);
    GetSources(pRootElement, "pictures", m_pictureSources, m_defaultPictureSource);
    GetSources(pRootElement, "files", m_fileSources, m_defaultFileSource);
    GetSources(pRootElement, "music", m_musicSources, m_defaultMusicSource);
    GetSources(pRootElement, "video", m_videoSources, m_defaultVideoSource);
  }

  bXboxMediacenter = true;

  LoadRSSFeeds();
  LoadUserFolderLayout();

  return true;
}

void CSettings::ConvertHomeVar(CStdString& strText)
{
  // Replaces first occurence of $HOME with the home directory.
  // "$HOME\foo" becomes for instance "e:\apps\xbmc\foo"

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
  vector<CStdString> token;
  CUtil::Tokenize(strText,token,"\\");
  if (token.size() > 1)
  {
    strText = "";
    for (unsigned int i=0;i<token.size();++i)
      if (token[i] == "..")
      {
        CStdString strParent;
        CUtil::GetParentPath(strText,strParent);
        strText = strParent;
      }
      else
        strText += token[i]+"\\";
  }
}

VECSOURCES *CSettings::GetSourcesFromType(const CStdString &type)
{
  if (type == "programs" || type == "myprograms")
    return &g_settings.m_programSources;
  else if (type == "files")
  {
    // this nasty block of code is needed as we have to
    // call getlocaldrives after localize strings has been loaded
    bool bAdded=false;
    for (unsigned int i=0;i<g_settings.m_fileSources.size();++i) 
    {
      if (g_settings.m_fileSources[i].m_ignore)
      {
        bAdded = true;
        break;
      }
    }
    if (!bAdded)
    {
      VECSOURCES shares;
      g_mediaManager.GetLocalDrives(shares, true);  // true to include Q
      m_fileSources.insert(m_fileSources.end(),shares.begin(),shares.end());
      
      CMediaSource source;
      source.strName = g_localizeStrings.Get(22013);
      source.m_ignore = true;
      source.strPath = "special://profile/";
      source.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
      m_fileSources.push_back(source);
    }

    return &g_settings.m_fileSources;
  }
  else if (type == "music")
    return &g_settings.m_musicSources;
  else if (type == "video")
    return &g_settings.m_videoSources;
  else if (type == "pictures")
    return &g_settings.m_pictureSources;
  else if (type == "upnpmusic")
    return &g_settings.m_UPnPMusicSources;
  else if (type == "upnpvideo")
    return &g_settings.m_UPnPVideoSources;
  else if (type == "upnppictures")
    return &g_settings.m_UPnPPictureSources;

  return NULL;
}

CStdString CSettings::GetDefaultSourceFromType(const CStdString &type)
{
  CStdString defaultShare;
  if (type == "programs" || type == "myprograms")
    defaultShare = g_settings.m_defaultProgramSource;
  else if (type == "files")
    defaultShare = g_settings.m_defaultFileSource;
  else if (type == "music")
    defaultShare = g_settings.m_defaultMusicSource;
  else if (type == "video")
    defaultShare = g_settings.m_defaultVideoSource;
  else if (type == "pictures")
    defaultShare = g_settings.m_defaultPictureSource;
  return defaultShare;
}

void CSettings::GetSources(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSOURCES& items, CStdString& strDefault)
{
  //CLog::Log(LOGDEBUG, "  Parsing <%s> tag", strTagName.c_str());
  strDefault = "";

  items.clear();
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    pChild = pChild->FirstChild();
    while (pChild > 0)
    {
      CStdString strValue = pChild->Value();
      if (strValue == "source" || strValue == "bookmark") // "bookmark" left in for backwards compatibility
      {
        CMediaSource share;
        if (GetSource(strTagName, pChild, share))
        {
          items.push_back(share);
        }
        else
        {
          CLog::Log(LOGERROR, "    Missing or invalid <name> and/or <path> in source");
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
          CLog::Log(LOGDEBUG, "    Setting <default> source to : %s", strDefault.c_str());
        }
      }
      pChild = pChild->NextSibling();
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "  <%s> tag is missing or sources.xml is malformed", strTagName.c_str());
  }
}

bool CSettings::GetSource(const CStdString &category, const TiXmlNode *source, CMediaSource &share)
{
  //CLog::Log(LOGDEBUG,"    ---- SOURCE START ----");
  const TiXmlNode *pNodeName = source->FirstChild("name");
  CStdString strName;
  if (pNodeName && pNodeName->FirstChild())
  {
    strName = pNodeName->FirstChild()->Value();
    //CLog::Log(LOGDEBUG,"    Found name: %s", strName.c_str());
  }
  // get multiple paths
  vector<CStdString> vecPaths;
  const TiXmlElement *pPathName = source->FirstChildElement("path");
  while (pPathName)
  {
    if (pPathName->FirstChild())
    {
      int pathVersion = 0;
      pPathName->Attribute("pathversion", &pathVersion);
      CStdString strPath = pPathName->FirstChild()->Value();
      strPath = CSpecialProtocol::ReplaceOldPath(strPath, pathVersion);
      // make sure there are no virtualpaths or stack paths defined in xboxmediacenter.xml
      //CLog::Log(LOGDEBUG,"    Found path: %s", strPath.c_str());
      if (!CUtil::IsVirtualPath(strPath) && !CUtil::IsStack(strPath))
      {
        // translate special tags
        if (!strPath.IsEmpty() && strPath.at(0) == '$')
        {
          CStdString strPathOld(strPath);
          strPath = CUtil::TranslateSpecialSource(strPath);
          if (!strPath.IsEmpty())
          {
            //CLog::Log(LOGDEBUG,"    -> Translated to path: %s", strPath.c_str());
          }
          else
          {
            //CLog::Log(LOGERROR,"    -> Skipping invalid token: %s", strPathOld.c_str());
            pPathName = pPathName->NextSiblingElement("path");
            continue;
          }
        }
        CUtil::AddSlashAtEnd(strPath);
        vecPaths.push_back(strPath);
      }
      else
        CLog::Log(LOGERROR,"    Invalid path type (%s) in source", strPath.c_str());
    }
    pPathName = pPathName->NextSiblingElement("path");
  }

  const TiXmlNode *pLockMode = source->FirstChild("lockmode");
  const TiXmlNode *pLockCode = source->FirstChild("lockcode");
  const TiXmlNode *pBadPwdCount = source->FirstChild("badpwdcount");
  const TiXmlNode *pThumbnailNode = source->FirstChild("thumbnail");

  if (!strName.IsEmpty() && vecPaths.size() > 0)
  {
    vector<CStdString> verifiedPaths;
    // disallowed for files, or theres only a single path in the vector
    if ((category.Equals("files")) || (vecPaths.size() == 1))
      verifiedPaths.push_back(vecPaths[0]);

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
        if (category.Equals("programs") || category.Equals("myprograms"))
        {
          // only allow HD and plugins
          if (url.IsLocal() || protocol.Equals("plugin"))
            verifiedPaths.push_back(vecPaths[j]);
          else
            bIsInvalid = true;
        }

        // for others allow everything (if the user does something silly, we can't stop them)
        else
          verifiedPaths.push_back(vecPaths[j]);

        // error message
        if (bIsInvalid)
          CLog::Log(LOGERROR,"    Invalid path type (%s) for multipath source", vecPaths[j].c_str());
      }

      // no valid paths? skip to next source
      if (verifiedPaths.size() == 0)
      {
        CLog::Log(LOGERROR,"    Missing or invalid <name> and/or <path> in source");
        return false;
      }
    }

    share.FromNameAndPaths(category, strName, verifiedPaths);

/*    CLog::Log(LOGDEBUG,"      Adding source:");
    CLog::Log(LOGDEBUG,"        Name: %s", share.strName.c_str());
    if (CUtil::IsVirtualPath(share.strPath) || CUtil::IsMultiPath(share.strPath))
    {
      for (int i = 0; i < (int)share.vecPaths.size(); ++i)
        CLog::Log(LOGDEBUG,"        Path (%02i): %s", i+1, share.vecPaths.at(i).c_str());
    }
    else
      CLog::Log(LOGDEBUG,"        Path: %s", share.strPath.c_str());
*/
    share.m_iBadPwdCount = 0;
    if (pLockMode)
    {
      share.m_iLockMode = LockType(atoi(pLockMode->FirstChild()->Value()));
      share.m_iHasLock = 2;
    }

    if (pLockCode)
    {
      if (pLockCode->FirstChild())
        share.m_strLockCode = pLockCode->FirstChild()->Value();
    }

    if (pBadPwdCount)
    {
      if (pBadPwdCount->FirstChild())
        share.m_iBadPwdCount = atoi( pBadPwdCount->FirstChild()->Value() );
    }

    if (pThumbnailNode)
    {
      if (pThumbnailNode->FirstChild())
        share.m_strThumbnailImage = pThumbnailNode->FirstChild()->Value();
    }

    return true;
  }
  return false;
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue)
{
  CStdString defaultValue = strValue;
  return GetString(pRootElement, tagName, strValue, defaultValue);
}

bool CSettings::GetPath(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue)
{
  CStdString strDefault = strValue;
  if (XMLUtils::GetPath(pRootElement, tagName, strValue))
  { // tag exists
    // check for "-" for backward compatibility
    if (!strValue.Equals("-"))
      return true;
  }
  // tag doesn't exist - set default
  strValue = strDefault;
  return false;
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue, const CStdString& strDefaultValue)
{
  if (XMLUtils::GetString(pRootElement, tagName, strValue))
  { // tag exists
    // check for "-" for backward compatibility
    if (!strValue.Equals("-"))
      return true;
  }
  // tag doesn't exist - set default
  strValue = strDefaultValue;
  return false;
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, char *szValue, const CStdString& strDefaultValue)
{
  CStdString strValue;
  bool ret = GetString(pRootElement, tagName, strValue, strDefaultValue);
  if (szValue)
    strcpy(szValue, strValue.c_str());
  return ret;
}

bool CSettings::GetInteger(const TiXmlElement* pRootElement, const char *tagName, int& iValue, const int iDefault, const int iMin, const int iMax)
{
  if (GetInteger(pRootElement, tagName, iValue, iMin, iMax))
    return true;
  // default
  iValue = iDefault;
  return false;
}

bool CSettings::GetInteger(const TiXmlElement* pRootElement, const char *tagName, int& iValue, const int iMin, const int iMax)
{
  if (XMLUtils::GetInt(pRootElement, tagName, iValue))
  { // check range
    if (iValue < iMin) iValue = iMin;
    if (iValue > iMax) iValue = iMax;
    return true;
  }
  return false;
}

bool CSettings::GetFloat(const TiXmlElement* pRootElement, const char *tagName, float& fValue, const float fDefault, const float fMin, const float fMax)
{
  if (GetFloat(pRootElement, tagName, fValue, fMin, fMax))
    return true;
  // default
  fValue = fDefault;
  return false;
}

bool CSettings::GetFloat(const TiXmlElement* pRootElement, const char *tagName, float& fValue, const float fMin, const float fMax)
{
  if (XMLUtils::GetFloat(pRootElement, tagName, fValue))
  { // check range
    if (fValue < fMin) fValue = fMin;
    if (fValue > fMax) fValue = fMax;
    return true;
  }
  return false;
}

void CSettings::GetViewState(const TiXmlElement *pRootElement, const CStdString &strTagName, CViewState &viewState, SORT_METHOD defaultSort, int defaultView)
{
  const TiXmlElement* pNode = pRootElement->FirstChildElement(strTagName);
  if (!pNode)
  {
    viewState.m_sortMethod = defaultSort;
    viewState.m_viewMode = defaultView;
    return;
  }
  GetInteger(pNode, "viewmode", viewState.m_viewMode, defaultView, DEFAULT_VIEW_LIST, DEFAULT_VIEW_MAX);
  
  int sortMethod;
  GetInteger(pNode, "sortmethod", sortMethod, defaultSort, SORT_METHOD_NONE, SORT_METHOD_MAX);
  viewState.m_sortMethod = (SORT_METHOD)sortMethod;

  int sortOrder;
  GetInteger(pNode, "sortorder", sortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
  viewState.m_sortOrder = (SORT_ORDER)sortOrder;
}

void CSettings::SetViewState(TiXmlNode *pRootNode, const CStdString &strTagName, const CViewState &viewState) const
{
  TiXmlElement newElement(strTagName);
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    XMLUtils::SetInt(pNewNode, "viewmode", viewState.m_viewMode);
    XMLUtils::SetInt(pNewNode, "sortmethod", (int)viewState.m_sortMethod);
    XMLUtils::SetInt(pNewNode, "sortorder", (int)viewState.m_sortOrder);
  }
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
    return false;
  }
  const TiXmlElement *pResolution = pRootElement->FirstChildElement("resolution");
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

    // get the overscan info
    const TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
    if (pOverscan)
    {
      GetInteger(pOverscan, "left", m_ResInfo[iRes].Overscan.left, 0, -m_ResInfo[iRes].iWidth / 4, m_ResInfo[iRes].iWidth / 4);
      GetInteger(pOverscan, "top", m_ResInfo[iRes].Overscan.top, 0, -m_ResInfo[iRes].iHeight / 4, m_ResInfo[iRes].iHeight / 4);
      GetInteger(pOverscan, "right", m_ResInfo[iRes].Overscan.right, m_ResInfo[iRes].iWidth, m_ResInfo[iRes].iWidth / 2, m_ResInfo[iRes].iWidth*3 / 2);
      GetInteger(pOverscan, "bottom", m_ResInfo[iRes].Overscan.bottom, m_ResInfo[iRes].iHeight, m_ResInfo[iRes].iHeight / 2, m_ResInfo[iRes].iHeight*3 / 2);
    }

/*    CLog::Log(LOGDEBUG, "  calibration for %s %ix%i", m_ResInfo[iRes].strMode, m_ResInfo[iRes].iWidth, m_ResInfo[iRes].iHeight);
    CLog::Log(LOGDEBUG, "    subtitle yposition:%i pixelratio:%03.3f offsets:(%i,%i)->(%i,%i)",
              m_ResInfo[iRes].iSubtitles, m_ResInfo[iRes].fPixelRatio,
              m_ResInfo[iRes].Overscan.left, m_ResInfo[iRes].Overscan.top,
              m_ResInfo[iRes].Overscan.right, m_ResInfo[iRes].Overscan.bottom);*/

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
    XMLUtils::SetString(pNode, "description", m_ResInfo[i].strMode);
    XMLUtils::SetInt(pNode, "id", i);
    XMLUtils::SetInt(pNode, "subtitles", m_ResInfo[i].iSubtitles);
    XMLUtils::SetFloat(pNode, "pixelratio", m_ResInfo[i].fPixelRatio);
    // create the overscan child
    TiXmlElement overscanElement("overscan");
    TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
    XMLUtils::SetInt(pOverscanNode, "left", m_ResInfo[i].Overscan.left);
    XMLUtils::SetInt(pOverscanNode, "top", m_ResInfo[i].Overscan.top);
    XMLUtils::SetInt(pOverscanNode, "right", m_ResInfo[i].Overscan.right);
    XMLUtils::SetInt(pOverscanNode, "bottom", m_ResInfo[i].Overscan.bottom);
  }
  return true;
}

bool CSettings::LoadSettings(const CStdString& strSettingsFile)
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

  // mymusic settings
  TiXmlElement *pElement = pRootElement->FirstChildElement("mymusic");
  if (pElement)
  {
    TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    {
      XMLUtils::GetBoolean(pChild, "repeat", g_stSettings.m_bMyMusicPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", g_stSettings.m_bMyMusicPlaylistShuffle);
    }
    // if the user happened to reboot in the middle of the scan we save this state
    pChild = pElement->FirstChildElement("scanning");
    if (pChild)
    {
      XMLUtils::GetBoolean(pChild, "isscanning", g_stSettings.m_bMyMusicIsScanning);
    }
    GetInteger(pElement, "startwindow", g_stSettings.m_iMyMusicStartWindow, WINDOW_MUSIC_FILES, WINDOW_MUSIC_FILES, WINDOW_MUSIC_NAV); //501; view songs
    XMLUtils::GetBoolean(pElement, "songinfoinvis", g_stSettings.m_bMyMusicSongInfoInVis);
    XMLUtils::GetBoolean(pElement, "songthumbinvis", g_stSettings.m_bMyMusicSongThumbInVis);
    GetPath(pElement, "defaultlibview", g_settings.m_defaultMusicLibSource);
  }
  // myvideos settings
  pElement = pRootElement->FirstChildElement("myvideos");
  if (pElement)
  {
    GetInteger(pElement, "startwindow", g_stSettings.m_iVideoStartWindow, WINDOW_VIDEO_FILES, WINDOW_VIDEO_FILES, WINDOW_VIDEO_NAV);
    GetInteger(pElement, "stackvideomode", g_stSettings.m_iMyVideoStack, STACK_NONE, STACK_NONE, STACK_SIMPLE);

    GetPath(pElement, "defaultlibview", g_settings.m_defaultVideoLibSource);
    GetInteger(pElement, "watchmode", g_stSettings.m_iMyVideoWatchMode, VIDEO_SHOW_ALL, VIDEO_SHOW_ALL, VIDEO_SHOW_WATCHED);
    XMLUtils::GetBoolean(pElement, "flatten", g_stSettings.m_bMyVideoNavFlatten);

    TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    { // playlist
      XMLUtils::GetBoolean(pChild, "repeat", g_stSettings.m_bMyVideoPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", g_stSettings.m_bMyVideoPlaylistShuffle);
    }
  }

  pElement = pRootElement->FirstChildElement("viewstates");
  if (pElement)
  {
    GetViewState(pElement, "musicnavartists", g_stSettings.m_viewStateMusicNavArtists);
    GetViewState(pElement, "musicnavalbums", g_stSettings.m_viewStateMusicNavAlbums);
    GetViewState(pElement, "musicnavsongs", g_stSettings.m_viewStateMusicNavSongs);
    GetViewState(pElement, "musicshoutcast", g_stSettings.m_viewStateMusicShoutcast);
    GetViewState(pElement, "musiclastfm", g_stSettings.m_viewStateMusicLastFM);
    GetViewState(pElement, "videonavactors", g_stSettings.m_viewStateVideoNavActors);
    GetViewState(pElement, "videonavyears", g_stSettings.m_viewStateVideoNavYears);
    GetViewState(pElement, "videonavgenres", g_stSettings.m_viewStateVideoNavGenres);
    GetViewState(pElement, "videonavtitles", g_stSettings.m_viewStateVideoNavTitles);
    GetViewState(pElement, "videonavepisodes", g_stSettings.m_viewStateVideoNavEpisodes, SORT_METHOD_EPISODE);
    GetViewState(pElement, "videonavtvshows", g_stSettings.m_viewStateVideoNavTvShows);
    GetViewState(pElement, "videonavseasons", g_stSettings.m_viewStateVideoNavSeasons);
    GetViewState(pElement, "videonavmusicvideos", g_stSettings.m_viewStateVideoNavMusicVideos);

    GetViewState(pElement, "programs", g_stSettings.m_viewStatePrograms, SORT_METHOD_LABEL, DEFAULT_VIEW_AUTO);
    GetViewState(pElement, "pictures", g_stSettings.m_viewStatePictures, SORT_METHOD_LABEL, DEFAULT_VIEW_AUTO);
    GetViewState(pElement, "videofiles", g_stSettings.m_viewStateVideoFiles, SORT_METHOD_LABEL, DEFAULT_VIEW_AUTO);
    GetViewState(pElement, "musicfiles", g_stSettings.m_viewStateMusicFiles, SORT_METHOD_LABEL, DEFAULT_VIEW_AUTO);
  }

  // general settings
  pElement = pRootElement->FirstChildElement("general");
  if (pElement)
  {
    GetInteger(pElement, "systemtotaluptime", g_stSettings.m_iSystemTimeTotalUp, 0, 0, INT_MAX);
    GetInteger(pElement, "httpapibroadcastlevel", g_stSettings.m_HttpApiBroadcastLevel, 0, 0,5);
    GetInteger(pElement, "httpapibroadcastport", g_stSettings.m_HttpApiBroadcastPort, 8278, 1, 65535);
  }

  pElement = pRootElement->FirstChildElement("defaultvideosettings");
  if (pElement)
  {
    int interlaceMethod;
    GetInteger(pElement, "interlacemethod", interlaceMethod, VS_INTERLACEMETHOD_NONE, VS_INTERLACEMETHOD_NONE, VS_INTERLACEMETHOD_RENDER_BLEND);
    g_stSettings.m_defaultVideoSettings.m_InterlaceMethod = (EINTERLACEMETHOD)interlaceMethod;

    GetInteger(pElement, "filmgrain", g_stSettings.m_defaultVideoSettings.m_FilmGrain, 0, 0, 10);
    GetInteger(pElement, "viewmode", g_stSettings.m_defaultVideoSettings.m_ViewMode, VIEW_MODE_NORMAL, VIEW_MODE_NORMAL, VIEW_MODE_CUSTOM);
    GetFloat(pElement, "zoomamount", g_stSettings.m_defaultVideoSettings.m_CustomZoomAmount, 1.0f, 0.5f, 2.0f);
    GetFloat(pElement, "pixelratio", g_stSettings.m_defaultVideoSettings.m_CustomPixelRatio, 1.0f, 0.5f, 2.0f);
    GetFloat(pElement, "volumeamplification", g_stSettings.m_defaultVideoSettings.m_VolumeAmplification, VOLUME_DRC_MINIMUM * 0.01f, VOLUME_DRC_MINIMUM * 0.01f, VOLUME_DRC_MAXIMUM * 0.01f);
    XMLUtils::GetBoolean(pElement, "outputtoallspeakers", g_stSettings.m_defaultVideoSettings.m_OutputToAllSpeakers);
    XMLUtils::GetBoolean(pElement, "showsubtitles", g_stSettings.m_defaultVideoSettings.m_SubtitleOn);
    GetInteger(pElement, "brightness", g_stSettings.m_defaultVideoSettings.m_Brightness, 50, 0, 100);
    GetInteger(pElement, "contrast", g_stSettings.m_defaultVideoSettings.m_Contrast, 50, 0, 100);
    GetInteger(pElement, "gamma", g_stSettings.m_defaultVideoSettings.m_Gamma, 20, 0, 100);
    GetFloat(pElement, "audiodelay", g_stSettings.m_defaultVideoSettings.m_AudioDelay, 0.0f, -10.0f, 10.0f);
    GetFloat(pElement, "subtitledelay", g_stSettings.m_defaultVideoSettings.m_SubtitleDelay, 0.0f, -10.0f, 10.0f);

    g_stSettings.m_defaultVideoSettings.m_SubtitleCached = false;
  }
  // audio settings
  pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    GetInteger(pElement, "volumelevel", g_stSettings.m_nVolumeLevel, VOLUME_MAXIMUM, VOLUME_MINIMUM, VOLUME_MAXIMUM);
    GetInteger(pElement, "dynamicrangecompression", g_stSettings.m_dynamicRangeCompressionLevel, VOLUME_DRC_MINIMUM, VOLUME_DRC_MINIMUM, VOLUME_DRC_MAXIMUM);
    for (int i = 0; i < 4; i++)
    {
      CStdString setting;
      setting.Format("karaoke%i", i);
#ifndef HAS_XBOX_AUDIO
#define XVOICE_MASK_PARAM_DISABLED (-1.0f)
#endif
      GetFloat(pElement, setting + "energy", g_stSettings.m_karaokeVoiceMask[i].energy, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
      GetFloat(pElement, setting + "pitch", g_stSettings.m_karaokeVoiceMask[i].pitch, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
      GetFloat(pElement, setting + "whisper", g_stSettings.m_karaokeVoiceMask[i].whisper, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
      GetFloat(pElement, setting + "robotic", g_stSettings.m_karaokeVoiceMask[i].robotic, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
    }
  }

  LoadCalibration(pRootElement, strSettingsFile);
  g_guiSettings.LoadXML(pRootElement);
  LoadSkinSettings(pRootElement);

  // Advanced settings
  LoadAdvancedSettings();
  // Default players?
  CLog::Log(LOGNOTICE, "Default Video Player: %s", g_advancedSettings.m_videoDefaultPlayer.c_str());
  CLog::Log(LOGNOTICE, "Default Audio Player: %s", g_advancedSettings.m_audioDefaultPlayer.c_str());

  // Override settings with avpack settings
  if ( m_vecProfiles[m_iLastLoadedProfileIndex].useAvpackSettings())
  {
    CLog::Log(LOGNOTICE, "Per AV pack settings are on");
    LoadAvpackXML();
  }
  else
    CLog::Log(LOGNOTICE, "Per AV pack settings are off");

  return true;
}

void CSettings::LoadAdvancedSettings()
{
  // NOTE: This routine should NOT set the default of any of these parameters
  //       it should instead use the versions of GetString/Integer/Float that
  //       don't take defaults in.  Defaults are set in the constructor above
  CStdString advancedSettingsXML;
  advancedSettingsXML  = g_settings.GetUserDataItem("advancedsettings.xml");
  TiXmlDocument advancedXML;
  if (!CFile::Exists(advancedSettingsXML))
  { // tell the user it doesn't exist
    CLog::Log(LOGNOTICE, "No advancedsettings.xml to load (%s)", advancedSettingsXML.c_str());
    return;
  }

  if (!advancedXML.LoadFile(advancedSettingsXML))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", advancedSettingsXML.c_str(), advancedXML.ErrorRow(), advancedXML.ErrorDesc());
    return;
  }

  TiXmlElement *pRootElement = advancedXML.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"advancedsettings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <advancedsettings> node", advancedSettingsXML.c_str());
    return;
  }

  // succeeded - tell the user it worked
  CLog::Log(LOGNOTICE, "Loaded advancedsettings.xml from %s", advancedSettingsXML.c_str());

  TiXmlElement *pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    GetFloat(pElement, "ac3downmixgain", g_advancedSettings.m_ac3Gain, 12.0f, -96.0f, 96.0f);
    GetInteger(pElement, "headroom", g_advancedSettings.m_audioHeadRoom, 0, 12);
    GetFloat(pElement, "karaokesyncdelay", g_advancedSettings.m_karaokeSyncDelay, -3.0f, 3.0f);
    GetString(pElement, "defaultplayer", g_advancedSettings.m_audioDefaultPlayer, "paplayer");
    XMLUtils::GetBoolean(pElement, "usetimeseeking", g_advancedSettings.m_musicUseTimeSeeking);
    GetInteger(pElement, "timeseekforward", g_advancedSettings.m_musicTimeSeekForward, 0, 6000);
    GetInteger(pElement, "timeseekbackward", g_advancedSettings.m_musicTimeSeekBackward, -6000, 0);
    GetInteger(pElement, "timeseekforwardbig", g_advancedSettings.m_musicTimeSeekForwardBig, 0, 6000);
    GetInteger(pElement, "timeseekbackwardbig", g_advancedSettings.m_musicTimeSeekBackwardBig, -6000, 0);

    GetInteger(pElement, "percentseekforward", g_advancedSettings.m_musicPercentSeekForward, 0, 100);
    GetInteger(pElement, "percentseekbackward", g_advancedSettings.m_musicPercentSeekBackward, -100, 0);
    GetInteger(pElement, "percentseekforwardbig", g_advancedSettings.m_musicPercentSeekForwardBig, 0, 100);
    GetInteger(pElement, "percentseekbackwardbig", g_advancedSettings.m_musicPercentSeekBackwardBig, -100, 0);

    GetInteger(pElement, "resample", g_advancedSettings.m_musicResample, 0, 48000);

    TiXmlElement* pAudioExcludes = pElement->FirstChildElement("excludefromlisting");
    if (pAudioExcludes)
      GetCustomRegexps(pAudioExcludes, g_advancedSettings.m_audioExcludeFromListingRegExps);
              
    pAudioExcludes = pElement->FirstChildElement("excludefromscan");
    if (pAudioExcludes)
      GetCustomRegexps(pAudioExcludes, g_advancedSettings.m_audioExcludeFromScanRegExps);
                            
  }

  pElement = pRootElement->FirstChildElement("video");
  if (pElement)
  {
    GetFloat(pElement, "subsdelayrange", g_advancedSettings.m_videoSubsDelayRange, 10, 600);
    GetFloat(pElement, "audiodelayrange", g_advancedSettings.m_videoAudioDelayRange, 10, 600);
    GetInteger(pElement, "smallstepbackseconds", g_advancedSettings.m_videoSmallStepBackSeconds, 1, INT_MAX);
    GetInteger(pElement, "smallstepbacktries", g_advancedSettings.m_videoSmallStepBackTries, 1, 10);
    GetInteger(pElement, "smallstepbackdelay", g_advancedSettings.m_videoSmallStepBackDelay, 100, 5000); //MS

    XMLUtils::GetBoolean(pElement, "usetimeseeking", g_advancedSettings.m_videoUseTimeSeeking);
    GetInteger(pElement, "timeseekforward", g_advancedSettings.m_videoTimeSeekForward, 0, 6000);
    GetInteger(pElement, "timeseekbackward", g_advancedSettings.m_videoTimeSeekBackward, -6000, 0);
    GetInteger(pElement, "timeseekforwardbig", g_advancedSettings.m_videoTimeSeekForwardBig, 0, 6000);
    GetInteger(pElement, "timeseekbackwardbig", g_advancedSettings.m_videoTimeSeekBackwardBig, -6000, 0);

    GetInteger(pElement, "percentseekforward", g_advancedSettings.m_videoPercentSeekForward, 0, 100);
    GetInteger(pElement, "percentseekbackward", g_advancedSettings.m_videoPercentSeekBackward, -100, 0);
    GetInteger(pElement, "percentseekforwardbig", g_advancedSettings.m_videoPercentSeekForwardBig, 0, 100);
    GetInteger(pElement, "percentseekbackwardbig", g_advancedSettings.m_videoPercentSeekBackwardBig, -100, 0);
    GetInteger(pElement, "blackbarcolour", g_advancedSettings.m_videoBlackBarColour, 0, 255);
    GetString(pElement, "defaultplayer", g_advancedSettings.m_videoDefaultPlayer, "mplayer");

    TiXmlElement* pVideoExcludes = pElement->FirstChildElement("excludefromlisting");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, g_advancedSettings.m_videoExcludeFromListingRegExps);
            
    pVideoExcludes = pElement->FirstChildElement("excludefromscan");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, g_advancedSettings.m_moviesExcludeFromScanRegExps);

    pVideoExcludes = pElement->FirstChildElement("excludetvshowsfromscan");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, g_advancedSettings.m_tvshowExcludeFromScanRegExps);

    pVideoExcludes = pElement->FirstChildElement("cleanfilenames");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, g_advancedSettings.m_videoCleanRegExps);            
  
    GetString(pElement,"postprocessing",g_advancedSettings.m_videoPPFFmpegType, "linblenddeint");
  }

  pElement = pRootElement->FirstChildElement("musiclibrary");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "hideallitems", g_advancedSettings.m_bMusicLibraryHideAllItems);
    XMLUtils::GetBoolean(pElement, "prioritiseapetags", g_advancedSettings.m_prioritiseAPEv2tags);
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", g_advancedSettings.m_bMusicLibraryAllItemsOnBottom);
    XMLUtils::GetBoolean(pElement, "albumssortbyartistthenyear", g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear);
    GetString(pElement, "albumformat", g_advancedSettings.m_strMusicLibraryAlbumFormat);
    GetString(pElement, "albumformatright", g_advancedSettings.m_strMusicLibraryAlbumFormatRight);
    GetString(pElement, "itemseparator", g_advancedSettings.m_musicItemSeparator);
  }

  pElement = pRootElement->FirstChildElement("videolibrary");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "hideallitems", g_advancedSettings.m_bVideoLibraryHideAllItems);
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", g_advancedSettings.m_bVideoLibraryAllItemsOnBottom);
    XMLUtils::GetBoolean(pElement, "hiderecentlyaddeditems", g_advancedSettings.m_bVideoLibraryHideRecentlyAddedItems);
    XMLUtils::GetBoolean(pElement, "hideemptyseries", g_advancedSettings.m_bVideoLibraryHideEmptySeries);
    XMLUtils::GetBoolean(pElement, "cleanonupdate", g_advancedSettings.m_bVideoLibraryCleanOnUpdate);
    GetString(pElement, "itemseparator", g_advancedSettings.m_videoItemSeparator);
  }

  pElement = pRootElement->FirstChildElement("slideshow");
  if (pElement)
  {
    GetFloat(pElement, "panamount", g_advancedSettings.m_slideshowPanAmount, 0.0f, 20.0f);
    GetFloat(pElement, "zoomamount", g_advancedSettings.m_slideshowZoomAmount, 0.0f, 20.0f);
    GetFloat(pElement, "blackbarcompensation", g_advancedSettings.m_slideshowBlackBarCompensation, 0.0f, 50.0f);
  }

  pElement = pRootElement->FirstChildElement("lcd");
  if (pElement)
  {
    GetInteger(pElement, "rows", g_advancedSettings.m_lcdRows, 1, 4);
    GetInteger(pElement, "columns", g_advancedSettings.m_lcdColumns, 1, 40);
    GetInteger(pElement, "address1", g_advancedSettings.m_lcdAddress1, 0, 0x100);
    GetInteger(pElement, "address2", g_advancedSettings.m_lcdAddress2, 0, 0x100);
    GetInteger(pElement, "address3", g_advancedSettings.m_lcdAddress3, 0, 0x100);
    GetInteger(pElement, "address4", g_advancedSettings.m_lcdAddress4, 0, 0x100);
  }

  pElement = pRootElement->FirstChildElement("network");
  if (pElement)
  {
    GetInteger(pElement, "autodetectpingtime", g_advancedSettings.m_autoDetectPingTime, 1, 240);
    GetInteger(pElement, "curlclienttimeout", g_advancedSettings.m_curlconnecttimeout, 1, 1000);
    GetInteger(pElement, "curllowspeedtime", g_advancedSettings.m_curllowspeedtime, 1, 1000);
  }

  GetFloat(pRootElement, "playcountminimumpercent", g_advancedSettings.m_playCountMinimumPercent, 1.0f, 100.0f);

  pElement = pRootElement->FirstChildElement("samba");
  if (pElement)
  {
    GetString(pElement,  "doscodepage",   g_advancedSettings.m_sambadoscodepage);
    GetInteger(pElement, "clienttimeout", g_advancedSettings.m_sambaclienttimeout, 5, 100);
    XMLUtils::GetBoolean(pElement, "statfiles", g_advancedSettings.m_sambastatfiles);
  }

  pElement = pRootElement->FirstChildElement("httpdirectory");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "localmode", g_advancedSettings.m_bHTTPDirectoryLocalMode);
    XMLUtils::GetBoolean(pElement, "statfilesize", g_advancedSettings.m_bHTTPDirectoryStatFilesize);
  }

  if (GetInteger(pRootElement, "loglevel", g_advancedSettings.m_logLevel, LOG_LEVEL_NONE, LOG_LEVEL_MAX))
  { // read the loglevel setting, so set the setting advanced to hide it in GUI
    // as altering it will do nothing - we don't write to advancedsettings.xml
    CSetting *setting = g_guiSettings.GetSetting("system.debuglogging");
    if (setting)
      setting->SetAdvanced();
  }

  pElement = pRootElement->FirstChildElement("python");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "verbose", g_advancedSettings.m_bPythonVerbose);
  }

  GetString(pRootElement, "cddbaddress", g_advancedSettings.m_cddbAddress);

  XMLUtils::GetBoolean(pRootElement, "usepcdvdrom", g_advancedSettings.m_usePCDVDROM);
  XMLUtils::GetBoolean(pRootElement, "nodvdrom", g_advancedSettings.m_noDVDROM);
  XMLUtils::GetBoolean(pRootElement, "usemultipaths", g_advancedSettings.m_useMultipaths);
  XMLUtils::GetBoolean(pRootElement, "disablemodchipdetection", g_advancedSettings.m_DisableModChipDetection);

  GetInteger(pRootElement, "songinfoduration", g_advancedSettings.m_songInfoDuration, 0, INT_MAX);
  GetInteger(pRootElement, "busydialogdelay", g_advancedSettings.m_busyDialogDelay, 0, 5000);
  GetInteger(pRootElement, "playlistretries", g_advancedSettings.m_playlistRetries, -1, 5000);
  GetInteger(pRootElement, "playlisttimeout", g_advancedSettings.m_playlistTimeout, 0, 5000);

  XMLUtils::GetBoolean(pRootElement,"rootovershoot",g_advancedSettings.m_bUseEvilB);
  GetInteger(pRootElement,"skiploopfilter", g_advancedSettings.m_iSkipLoopFilter, 0, -16, 48);
  XMLUtils::GetBoolean(pRootElement,"virtualshares", g_advancedSettings.m_bVirtualShares); 
  XMLUtils::GetBoolean(pRootElement,"navigatevirtualkeyboard", g_advancedSettings.m_bNavVKeyboard); 

  //Tuxbox
  pElement = pRootElement->FirstChildElement("tuxbox");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "audiochannelselection", g_advancedSettings.m_bTuxBoxAudioChannelSelection);
    XMLUtils::GetBoolean(pElement, "submenuselection", g_advancedSettings.m_bTuxBoxSubMenuSelection);
    XMLUtils::GetBoolean(pElement, "pictureicon", g_advancedSettings.m_bTuxBoxPictureIcon);
    XMLUtils::GetBoolean(pElement, "sendallaudiopids", g_advancedSettings.m_bTuxBoxSendAllAPids);
    GetInteger(pElement, "epgrequesttime", g_advancedSettings.m_iTuxBoxEpgRequestTime, 0, 3600);
    GetInteger(pElement, "defaultsubmenu", g_advancedSettings.m_iTuxBoxDefaultSubMenu, 1, 4);
    GetInteger(pElement, "defaultrootmenu", g_advancedSettings.m_iTuxBoxDefaultRootMenu, 0, 4);
    GetInteger(pElement, "zapwaittime", g_advancedSettings.m_iTuxBoxZapWaitTime, 0, 120);
    
  }

  // picture exclude regexps
  TiXmlElement* pPictureExcludes = pRootElement->FirstChildElement("pictureexcludes");
  if (pPictureExcludes)
    GetCustomRegexps(pPictureExcludes, g_advancedSettings.m_pictureExcludeFromListingRegExps);

  // picture extensions
  CStdString extraExtensions;
  TiXmlElement* pExts = pRootElement->FirstChildElement("pictureextensions");
  if (pExts)
    GetCustomExtensions(pExts,g_stSettings.m_pictureExtensions);

  // music extensions
  pExts = pRootElement->FirstChildElement("musicextensions");
  if (pExts)
    GetCustomExtensions(pExts,g_stSettings.m_musicExtensions);

  // video extensions
  pExts = pRootElement->FirstChildElement("videoextensions");
  if (pExts)
    GetCustomExtensions(pExts,g_stSettings.m_videoExtensions);

  const TiXmlNode *pTokens = pRootElement->FirstChild("sorttokens");
  g_advancedSettings.m_vecTokens.clear();
  if (pTokens && !pTokens->NoChildren())
  {
    const TiXmlNode *pToken = pTokens->FirstChild("token");
    while (pToken)
    {
      if (pToken->FirstChild() && pToken->FirstChild()->Value())
      {
        g_advancedSettings.m_vecTokens.push_back(CStdString(pToken->FirstChild()->Value()) + " ");
        g_advancedSettings.m_vecTokens.push_back(CStdString(pToken->FirstChild()->Value()) + ".");
      }
      pToken = pToken->NextSibling();
    }
  }

  XMLUtils::GetBoolean(pRootElement, "displayremotecodes", g_advancedSettings.m_displayRemoteCodes);

  // TODO: Should cache path be given in terms of our predefined paths??
  //       Are we even going to have predefined paths??
  GetPath(pRootElement, "cachepath", g_advancedSettings.m_cachePath);
  CUtil::AddSlashAtEnd(g_advancedSettings.m_cachePath);

  XMLUtils::GetBoolean(pRootElement, "ftpshowcache", g_advancedSettings.m_FTPShowCache);

  GetInteger(pRootElement, "cachemembufsize", g_advancedSettings.m_cacheMemBufferSize, 256, 0, 8192);
  
  g_LangCodeExpander.LoadUserCodes(pRootElement->FirstChildElement("languagecodes"));

  // stacking regexps
  TiXmlElement* pVideoStacking = pRootElement->FirstChildElement("moviestacking");
  if (pVideoStacking)
    GetCustomRegexps(pVideoStacking, g_advancedSettings.m_videoStackRegExps);

  //tv stacking regexps
  TiXmlElement* pTVStacking = pRootElement->FirstChildElement("tvshowmatching");
  if (pTVStacking)
    GetCustomRegexps(pTVStacking, g_advancedSettings.m_tvshowStackRegExps);

  // path substitutions
  TiXmlElement* pPathSubstitution = pRootElement->FirstChildElement("pathsubstitution");
  if (pPathSubstitution)
  {
    g_advancedSettings.m_pathSubstitutions.clear();
    CLog::Log(LOGDEBUG,"Configuring path substitutions");
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
        g_advancedSettings.m_pathSubstitutions.push_back(strFrom + " , " + strTo);
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

  GetInteger(pRootElement, "remoterepeat", g_advancedSettings.m_remoteRepeat, 1, INT_MAX);
  GetFloat(pRootElement, "controllerdeadzone", g_advancedSettings.m_controllerDeadzone, 0.0f, 1.0f);
  GetInteger(pRootElement, "thumbsize", g_advancedSettings.m_thumbSize, 64, 512);

  XMLUtils::GetBoolean(pRootElement, "playlistasfolders", g_advancedSettings.m_playlistAsFolders);
  XMLUtils::GetBoolean(pRootElement, "detectasudf", g_advancedSettings.m_detectAsUdf);

  // music thumbs
  CStdString extraThumbs;
  TiXmlElement* pThumbs = pRootElement->FirstChildElement("musicthumbs");
  if (pThumbs)
  {
    // remove before add so that the defaults can be restored after user defined ones
    // (ie, the list can be:cover.jpg|cover.png|folder.jpg)
    GetString(pThumbs, "remove", extraThumbs, "");
    if (extraThumbs != "")
    {
      CStdStringArray thumbs;
      StringUtils::SplitString(extraThumbs, "|", thumbs);
      for (unsigned int i = 0; i < thumbs.size(); ++i)
      {
        int iPos = g_advancedSettings.m_musicThumbs.Find(thumbs[i]);
        if (iPos == -1)
          continue;
        g_advancedSettings.m_musicThumbs.erase(iPos, thumbs[i].size() + 1);
      }
    }
    GetString(pThumbs, "add", extraThumbs,"");
    if (extraThumbs != "")
    {
      if (!g_advancedSettings.m_musicThumbs.IsEmpty())
        g_advancedSettings.m_musicThumbs += "|";
      g_advancedSettings.m_musicThumbs += extraThumbs;
    }
  }

  // dvd thumbs
  pThumbs = pRootElement->FirstChildElement("dvdthumbs");
  if (pThumbs)
  {
    // remove before add so that the defaults can be restored after user defined ones
    // (ie, the list can be:cover.jpg|cover.png|folder.jpg)
    GetString(pThumbs, "remove", extraThumbs, "");
    if (extraThumbs != "")
    {
      CStdStringArray thumbs;
      StringUtils::SplitString(extraThumbs, "|", thumbs);
      for (unsigned int i = 0; i < thumbs.size(); ++i)
      {
        int iPos = g_advancedSettings.m_dvdThumbs.Find(thumbs[i]);
        if (iPos == -1)
          continue;
        g_advancedSettings.m_dvdThumbs.erase(iPos, thumbs[i].size() + 1);
      }
    }
    GetString(pThumbs, "add", extraThumbs,"");
    if (extraThumbs != "")
    {
      if (!g_advancedSettings.m_dvdThumbs.IsEmpty())
        g_advancedSettings.m_dvdThumbs += "|";
      g_advancedSettings.m_dvdThumbs += extraThumbs;
    }
  }

  // music filename->tag filters
  TiXmlElement* filters = pRootElement->FirstChildElement("musicfilenamefilters");
  if (filters)
  {
    TiXmlNode* filter = filters->FirstChild("filter");
    while (filter)
    {
      if (filter->FirstChild())
        g_advancedSettings.m_musicTagsFromFileFilters.push_back(filter->FirstChild()->ValueStr());
      filter = filter->NextSibling("filter");
    }
  }

  TiXmlElement* pHostEntries = pRootElement->FirstChildElement("hosts");
  if (pHostEntries)
  {
    TiXmlElement* element = pHostEntries->FirstChildElement("entry");
    while(element)
    {
      CStdString name  = element->Attribute("name");
      CStdString value;
      if(element->GetText())
        value = element->GetText();

      if(name.length() > 0 && value.length() > 0)
        CDNSNameCache::Add(name, value);
      element = element->NextSiblingElement("entry");
    }
  }

  // load in the GUISettings overrides:
  g_guiSettings.LoadXML(pRootElement, true);  // true to hide the settings we read in
}

bool CSettings::LoadAvpackXML()
{
  CStdString avpackSettingsXML;
  avpackSettingsXML  = GetAvpackSettingsFile();
  TiXmlDocument avpackXML;
  if (!CFile::Exists(avpackSettingsXML))
  {
    CLog::Log(LOGERROR, "Error loading AV pack settings : %s not found !", avpackSettingsXML.c_str());
    return false;
  }

  CLog::Log(LOGNOTICE, "%s found : loading %s",
    g_videoConfig.GetAVPack().c_str(), avpackSettingsXML.c_str());

  if (!avpackXML.LoadFile(avpackSettingsXML.c_str()))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s",
      avpackSettingsXML.c_str(), avpackXML.ErrorRow(), avpackXML.ErrorDesc());
    return false;
  }

  TiXmlElement *pMainElement = avpackXML.RootElement();
  if (!pMainElement || strcmpi(pMainElement->Value(),"settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <settings> node", avpackSettingsXML.c_str());
    return false;
  }

  TiXmlElement *pRoot = pMainElement->FirstChildElement(g_videoConfig.GetAVPack());
  if (!pRoot)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <%s> node",
      avpackSettingsXML.c_str(), g_videoConfig.GetAVPack().c_str());
    return false;
  }

  // Load guisettings
  g_guiSettings.LoadXML(pRoot);

  // Load calibration
  return LoadCalibration(pRoot, avpackSettingsXML);
}

// Save the avpack settings in the current 'avpacksettings.xml' file
bool CSettings::SaveAvpackXML() const
{
  CStdString avpackSettingsXML;
  avpackSettingsXML  = GetAvpackSettingsFile();

  CLog::Log(LOGNOTICE, "Saving %s settings in %s",
    g_videoConfig.GetAVPack().c_str(), avpackSettingsXML.c_str());

  // The file does not exist : Save defaults
  if (!CFile::Exists(avpackSettingsXML))
    return SaveNewAvpackXML();

  // The file already exists :
  // We need to preserve other avpack settings

  // First load the previous settings
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(avpackSettingsXML))
  {
    CLog::Log(LOGERROR, "SaveAvpackSettings : Error loading %s, Line %d\n%s\nCreating new file.",
      avpackSettingsXML.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return SaveNewAvpackXML();
  }

  // Get the main element
  TiXmlElement *pMainElement = xmlDoc.RootElement();
  if (!pMainElement || strcmpi(pMainElement->Value(),"settings") != 0)
  {
    CLog::Log(LOGERROR, "SaveAvpackSettings : Error loading %s, no <settings> node.\nCreating new file.",
      avpackSettingsXML.c_str());
    return SaveNewAvpackXML();
  }

  // Delete the plugged avpack root if it exists, then recreate it
  // TODO : to support custom avpack settings, the two XMLs should
  // be synchronized, not just overwrite the old one
  TiXmlNode *pRoot = pMainElement->FirstChild(g_videoConfig.GetAVPack());
  if (pRoot)
    pMainElement->RemoveChild(pRoot);

  TiXmlElement pluggedNode(g_videoConfig.GetAVPack());
  pRoot = pMainElement->InsertEndChild(pluggedNode);
  if (!pRoot) return false;

  if (!SaveAvpackSettings(pRoot))
    return false;

  return xmlDoc.SaveFile(avpackSettingsXML);
}

// Create an 'avpacksettings.xml' file with in the current profile directory
bool CSettings::SaveNewAvpackXML() const
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlMainElement("settings");
  TiXmlNode *pMain = xmlDoc.InsertEndChild(xmlMainElement);
  if (!pMain) return false;

  TiXmlElement pluggedNode(g_videoConfig.GetAVPack());
  TiXmlNode *pRoot = pMain->InsertEndChild(pluggedNode);
  if (!pRoot) return false;

  if (!SaveAvpackSettings(pRoot))
    return false;

  return xmlDoc.SaveFile(GetAvpackSettingsFile());
}

// Save avpack settings in the provided xml node
bool CSettings::SaveAvpackSettings(TiXmlNode *io_pRoot) const
{
  TiXmlElement programsNode("myprograms");
  TiXmlNode *pNode = io_pRoot->InsertEndChild(programsNode);
  if (!pNode) return false;
  XMLUtils::SetBoolean(pNode, "gameautoregion", g_guiSettings.GetBool("myprograms.gameautoregion"));
  XMLUtils::SetInt(pNode, "ntscmode", g_guiSettings.GetInt("myprograms.ntscmode"));

  // default video settings
  TiXmlElement videoSettingsNode("defaultvideosettings");
  pNode = io_pRoot->InsertEndChild(videoSettingsNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "interlacemethod", g_stSettings.m_defaultVideoSettings.m_InterlaceMethod);
  XMLUtils::SetInt(pNode, "filmgrain", g_stSettings.m_currentVideoSettings.m_FilmGrain);
  XMLUtils::SetInt(pNode, "viewmode", g_stSettings.m_currentVideoSettings.m_ViewMode);
  XMLUtils::SetFloat(pNode, "zoomamount", g_stSettings.m_currentVideoSettings.m_CustomZoomAmount);
  XMLUtils::SetFloat(pNode, "pixelratio", g_stSettings.m_currentVideoSettings.m_CustomPixelRatio);
  XMLUtils::SetFloat(pNode, "volumeamplification", g_stSettings.m_currentVideoSettings.m_VolumeAmplification);
  XMLUtils::SetBoolean(pNode, "outputtoallspeakers", g_stSettings.m_currentVideoSettings.m_OutputToAllSpeakers);
  XMLUtils::SetBoolean(pNode, "showsubtitles", g_stSettings.m_currentVideoSettings.m_SubtitleOn);
  XMLUtils::SetInt(pNode, "brightness", g_stSettings.m_currentVideoSettings.m_Brightness);
  XMLUtils::SetInt(pNode, "contrast", g_stSettings.m_currentVideoSettings.m_Contrast);
  XMLUtils::SetInt(pNode, "gamma", g_stSettings.m_currentVideoSettings.m_Gamma);

  TiXmlElement audiooutputNode("audiooutput");
  pNode = io_pRoot->InsertEndChild(audiooutputNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "mode", g_guiSettings.GetInt("audiooutput.mode"));
  XMLUtils::SetBoolean(pNode, "ac3passthrough", g_guiSettings.GetBool("audiooutput.ac3passthrough"));
  XMLUtils::SetBoolean(pNode, "dtspassthrough", g_guiSettings.GetBool("audiooutput.dtspassthrough"));

  TiXmlElement videooutputNode("videooutput");
  pNode = io_pRoot->InsertEndChild(videooutputNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "aspect", g_guiSettings.GetInt("videooutput.aspect"));
  XMLUtils::SetBoolean(pNode, "hd480p", g_guiSettings.GetBool("videooutput.hd480p"));
  XMLUtils::SetBoolean(pNode, "hd720p", g_guiSettings.GetBool("videooutput.hd720p"));
  XMLUtils::SetBoolean(pNode, "hd1080i", g_guiSettings.GetBool("videooutput.hd1080i"));

  TiXmlElement videoscreenNode("videoscreen");
  pNode = io_pRoot->InsertEndChild(videoscreenNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "flickerfilter", g_guiSettings.GetInt("videoscreen.flickerfilter"));
  XMLUtils::SetInt(pNode, "resolution", g_guiSettings.GetInt("videoscreen.resolution"));
  XMLUtils::SetBoolean(pNode, "soften", g_guiSettings.GetBool("videoscreen.soften"));

  TiXmlElement videoplayerNode("videoplayer");
  pNode = io_pRoot->InsertEndChild(videoplayerNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "displayresolution", g_guiSettings.GetInt("videoplayer.displayresolution"));
  XMLUtils::SetInt(pNode, "flicker", g_guiSettings.GetInt("videoplayer.flicker"));
  XMLUtils::SetBoolean(pNode, "soften", g_guiSettings.GetBool("videoplayer.soften"));

  return SaveCalibration(io_pRoot);
}

void CSettings::GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings)
{
  int iAction = 0; // overwrite
  // for backward compatibility
  const char* szAppend = pRootElement->Attribute("append");
  if ((szAppend && stricmp(szAppend, "yes") == 0))
    iAction = 1;
  // action takes precedence if both attributes exist
  const char* szAction = pRootElement->Attribute("action");
  if (szAction)
  {
    iAction = 0; // overwrite
    if (stricmp(szAction, "append") == 0)
      iAction = 1; // append
    else if (stricmp(szAction, "prepend") == 0)
      iAction = 2; // prepend
  }
  if (iAction == 0)
    settings.clear();
  TiXmlNode* pRegExp = pRootElement->FirstChild("regexp");
  int i = 0;
  while (pRegExp)
  {
    if (pRegExp->FirstChild())
    {
      CStdString regExp = pRegExp->FirstChild()->Value();
      regExp.MakeLower();
      if (iAction == 2)
        settings.insert(settings.begin() + i++, 1, regExp);
      else
        settings.push_back(regExp);
    }
    pRegExp = pRegExp->NextSibling("regexp");
  }
}

void CSettings::GetCustomExtensions(TiXmlElement *pRootElement, CStdString& extensions)
{
  CStdString extraExtensions;
  GetString(pRootElement,"add",extraExtensions,"");
  if (extraExtensions != "")
    extensions += "|" + extraExtensions;
  GetString(pRootElement,"remove",extraExtensions,"");
  if (extraExtensions != "")
  {
    CStdStringArray exts;
    StringUtils::SplitString(extraExtensions,"|",exts);
    for (unsigned int i=0;i<exts.size();++i)
    {
      int iPos = extensions.Find(exts[i]);
      if (iPos == -1)
        continue;
      extensions.erase(iPos,exts[i].size()+1);
    }
  }
}

bool CSettings::SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings /* = NULL */) const
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("settings");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  // write our tags one by one - just a big list for now (can be flashed up later)

  // mymusic settings
  TiXmlElement musicNode("mymusic");
  TiXmlNode *pNode = pRoot->InsertEndChild(musicNode);
  if (!pNode) return false;
  {
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    XMLUtils::SetBoolean(pChild, "repeat", g_stSettings.m_bMyMusicPlaylistRepeat);
    XMLUtils::SetBoolean(pChild, "shuffle", g_stSettings.m_bMyMusicPlaylistShuffle);
  }
  {
    TiXmlElement childNode("scanning");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    XMLUtils::SetBoolean(pChild, "isscanning", g_stSettings.m_bMyMusicIsScanning);
  }

  XMLUtils::SetInt(pNode, "startwindow", g_stSettings.m_iMyMusicStartWindow);
  XMLUtils::SetBoolean(pNode, "songinfoinvis", g_stSettings.m_bMyMusicSongInfoInVis);
  XMLUtils::SetBoolean(pNode, "songthumbinvis", g_stSettings.m_bMyMusicSongThumbInVis);
  XMLUtils::SetPath(pNode, "defaultlibview", g_settings.m_defaultMusicLibSource);

  // myvideos settings
  TiXmlElement videosNode("myvideos");
  pNode = pRoot->InsertEndChild(videosNode);
  if (!pNode) return false;

  XMLUtils::SetInt(pNode, "startwindow", g_stSettings.m_iVideoStartWindow);

  XMLUtils::SetInt(pNode, "stackvideomode", g_stSettings.m_iMyVideoStack);

  XMLUtils::SetPath(pNode, "defaultlibview", g_settings.m_defaultVideoLibSource);

  XMLUtils::SetInt(pNode, "watchmode", g_stSettings.m_iMyVideoWatchMode);
  XMLUtils::SetBoolean(pNode, "flatten", g_stSettings.m_bMyVideoNavFlatten);

  { // playlist window
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    XMLUtils::SetBoolean(pChild, "repeat", g_stSettings.m_bMyVideoPlaylistRepeat);
    XMLUtils::SetBoolean(pChild, "shuffle", g_stSettings.m_bMyVideoPlaylistShuffle);
  }

  // view states
  TiXmlElement viewStateNode("viewstates");
  pNode = pRoot->InsertEndChild(viewStateNode);
  if (pNode)
  {
    SetViewState(pNode, "musicnavartists", g_stSettings.m_viewStateMusicNavArtists);
    SetViewState(pNode, "musicnavalbums", g_stSettings.m_viewStateMusicNavAlbums);
    SetViewState(pNode, "musicnavsongs", g_stSettings.m_viewStateMusicNavSongs);
    SetViewState(pNode, "musicshoutcast", g_stSettings.m_viewStateMusicShoutcast);
    SetViewState(pNode, "musiclastfm", g_stSettings.m_viewStateMusicLastFM);
    SetViewState(pNode, "videonavactors", g_stSettings.m_viewStateVideoNavActors);
    SetViewState(pNode, "videonavyears", g_stSettings.m_viewStateVideoNavYears);
    SetViewState(pNode, "videonavgenres", g_stSettings.m_viewStateVideoNavGenres);
    SetViewState(pNode, "videonavtitles", g_stSettings.m_viewStateVideoNavTitles);
    SetViewState(pNode, "videonavepisodes", g_stSettings.m_viewStateVideoNavEpisodes);
    SetViewState(pNode, "videonavseasons", g_stSettings.m_viewStateVideoNavSeasons);
    SetViewState(pNode, "videonavtvshows", g_stSettings.m_viewStateVideoNavTvShows);
    SetViewState(pNode, "videonavmusicvideos", g_stSettings.m_viewStateVideoNavMusicVideos);

    SetViewState(pNode, "programs", g_stSettings.m_viewStatePrograms);
    SetViewState(pNode, "pictures", g_stSettings.m_viewStatePictures);
    SetViewState(pNode, "videofiles", g_stSettings.m_viewStateVideoFiles);
    SetViewState(pNode, "musicfiles", g_stSettings.m_viewStateMusicFiles);
  }

  // general settings
  TiXmlElement generalNode("general");
  pNode = pRoot->InsertEndChild(generalNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "systemtotaluptime", g_stSettings.m_iSystemTimeTotalUp);
  XMLUtils::SetInt(pNode, "httpapibroadcastport", g_stSettings.m_HttpApiBroadcastPort);
  XMLUtils::SetInt(pNode, "httpapibroadcastlevel", g_stSettings.m_HttpApiBroadcastLevel);

  // default video settings
  TiXmlElement videoSettingsNode("defaultvideosettings");
  pNode = pRoot->InsertEndChild(videoSettingsNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "interlacemethod", g_stSettings.m_defaultVideoSettings.m_InterlaceMethod);
  XMLUtils::SetInt(pNode, "filmgrain", g_stSettings.m_defaultVideoSettings.m_FilmGrain);
  XMLUtils::SetInt(pNode, "viewmode", g_stSettings.m_defaultVideoSettings.m_ViewMode);
  XMLUtils::SetFloat(pNode, "zoomamount", g_stSettings.m_defaultVideoSettings.m_CustomZoomAmount);
  XMLUtils::SetFloat(pNode, "pixelratio", g_stSettings.m_defaultVideoSettings.m_CustomPixelRatio);
  XMLUtils::SetFloat(pNode, "volumeamplification", g_stSettings.m_defaultVideoSettings.m_VolumeAmplification);
  XMLUtils::SetBoolean(pNode, "outputtoallspeakers", g_stSettings.m_defaultVideoSettings.m_OutputToAllSpeakers);
  XMLUtils::SetBoolean(pNode, "showsubtitles", g_stSettings.m_defaultVideoSettings.m_SubtitleOn);
  XMLUtils::SetInt(pNode, "brightness", g_stSettings.m_defaultVideoSettings.m_Brightness);
  XMLUtils::SetInt(pNode, "contrast", g_stSettings.m_defaultVideoSettings.m_Contrast);
  XMLUtils::SetInt(pNode, "gamma", g_stSettings.m_defaultVideoSettings.m_Gamma);
  XMLUtils::SetFloat(pNode, "audiodelay", g_stSettings.m_defaultVideoSettings.m_AudioDelay);
  XMLUtils::SetFloat(pNode, "subtitledelay", g_stSettings.m_defaultVideoSettings.m_SubtitleDelay);


  // audio settings
  TiXmlElement volumeNode("audio");
  pNode = pRoot->InsertEndChild(volumeNode);
  if (!pNode) return false;
  XMLUtils::SetInt(pNode, "volumelevel", g_stSettings.m_nVolumeLevel);
  XMLUtils::SetInt(pNode, "dynamicrangecompression", g_stSettings.m_dynamicRangeCompressionLevel);
  for (int i = 0; i < 4; i++)
  {
    CStdString setting;
    setting.Format("karaoke%i", i);
    XMLUtils::SetFloat(pNode, setting + "energy", g_stSettings.m_karaokeVoiceMask[i].energy);
    XMLUtils::SetFloat(pNode, setting + "pitch", g_stSettings.m_karaokeVoiceMask[i].pitch);
    XMLUtils::SetFloat(pNode, setting + "whisper", g_stSettings.m_karaokeVoiceMask[i].whisper);
    XMLUtils::SetFloat(pNode, setting + "robotic", g_stSettings.m_karaokeVoiceMask[i].robotic);
  }

  SaveCalibration(pRoot);

  if (localSettings) // local settings to save
    localSettings->SaveXML(pRoot);
  else // save the global settings
    g_guiSettings.SaveXML(pRoot);

  SaveSkinSettings(pRoot);

  if ( m_vecProfiles[m_iLastLoadedProfileIndex].useAvpackSettings())
    SaveAvpackXML();

  // For mastercode
  SaveProfiles("Q:\\system\\profiles.xml");

  // save the file
  return xmlDoc.SaveFile(strSettingsFile);
}

bool CSettings::LoadProfile(int index)
{
  int iOldIndex = m_iLastLoadedProfileIndex;
  m_iLastLoadedProfileIndex = index;
  bool bSourcesXML=true;
  CStdString strOldSkin = g_guiSettings.GetString("lookandfeel.skin");
  CStdString strOldFont = g_guiSettings.GetString("lookandfeel.font");
  CStdString strOldTheme = g_guiSettings.GetString("lookandfeel.skintheme");
  CStdString strOldColors = g_guiSettings.GetString("lookandfeel.skincolors");
  //int iOldRes = g_guiSettings.GetInt("videoscreen.resolution");
  if (Load(bSourcesXML,bSourcesXML))
  {
    g_settings.CreateProfileFolders();

    // initialize our charset converter
    g_charsetConverter.reset();

    // Load the langinfo to have user charset <-> utf-8 conversion
    CStdString strLanguage = g_guiSettings.GetString("locale.language");

    CStdString strLangInfoPath;
    strLangInfoPath.Format("special://xbmc/language/%s/langinfo.xml", strLanguage.c_str());
    CLog::Log(LOGINFO, "load language info file:%s", strLangInfoPath.c_str());
    g_langInfo.Load(strLangInfoPath);

    CStdString strKeyboardLayoutConfigurationPath;
    strKeyboardLayoutConfigurationPath.Format("special://xbmc/language/%s/keyboardmap.xml", strLanguage.c_str());
    CLog::Log(LOGINFO, "load keyboard layout configuration info file: %s", strKeyboardLayoutConfigurationPath.c_str());
    g_keyboardLayoutConfiguration.Load(strKeyboardLayoutConfigurationPath);

    CStdString strLanguagePath;
    strLanguagePath.Format("special://xbmc/language/%s/strings.xml", strLanguage.c_str());

    g_buttonTranslator.Load();
    g_localizeStrings.Load(strLanguagePath);

    g_infoManager.ResetCache();

    // always reload the skin - we need it for the new language strings
    g_application.LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));

    if (m_iLastLoadedProfileIndex != 0)
    {
      TiXmlDocument doc;
      if (doc.LoadFile(CUtil::AddFileToFolder(GetUserDataFolder(),"guisettings.xml")))
        g_guiSettings.LoadMasterLock(doc.RootElement());
    }

#ifdef HAS_XBOX_HARDWARE
    if (g_guiSettings.GetBool("system.autotemperature"))
    {
      CLog::Log(LOGNOTICE, "start fancontroller");
      CFanController::Instance()->Start(g_guiSettings.GetInt("system.targettemperature"), g_guiSettings.GetInt("system.minfanspeed"));
    }
    else if (g_guiSettings.GetBool("system.fanspeedcontrol"))
    {
      CLog::Log(LOGNOTICE, "setting fanspeed");
      CFanController::Instance()->SetFanSpeed(g_guiSettings.GetInt("system.fanspeed"));
    }
    g_application.StartLEDControl(false);
#endif

    // to set labels - shares are reloaded
    CDetectDVDMedia::UpdateState();
    // init windows
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WINDOW_RESET);
    m_gWindowManager.SendMessage(msg);

    CUtil::DeleteMusicDatabaseDirectoryCache();
    CUtil::DeleteVideoDatabaseDirectoryCache();

    return true;
  }

  m_iLastLoadedProfileIndex = iOldIndex;

  return false;
}

bool CSettings::DeleteProfile(int index)
{
  if (index < 0 && index >= (int)g_settings.m_vecProfiles.size())
    return false;

  CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (dlgYesNo)
  {
    CStdString message;
    CStdString str = g_localizeStrings.Get(13201);
    message.Format(str.c_str(), g_settings.m_vecProfiles.at(index).getName());
    dlgYesNo->SetHeading(13200);
    dlgYesNo->SetLine(0, message);
    dlgYesNo->SetLine(1, "");
    dlgYesNo->SetLine(2, "");
    dlgYesNo->DoModal();

    if (dlgYesNo->IsConfirmed())
    {
      //delete profile
      CStdString strDirectory = g_settings.m_vecProfiles[index].getDirectory();
      m_vecProfiles.erase(g_settings.m_vecProfiles.begin()+index);
      if (index == g_settings.m_iLastLoadedProfileIndex)
      {
        g_settings.LoadProfile(0);
        g_settings.Save();
      }

      CFileItem item(CUtil::AddFileToFolder(GetUserDataFolder(), strDirectory));
      item.m_strPath = CUtil::AddFileToFolder(GetUserDataFolder(), strDirectory + "\\");
      item.m_bIsFolder = true;
      item.Select(true);
      CGUIWindowFileManager::DeleteItem(&item);
    }
    else
      return false;
  }

  SaveProfiles("Q:\\system\\profiles.xml");
  return true;
}

bool CSettings::SaveSettingsToProfile(int index)
{
  /*CProfile& profile = m_vecProfiles.at(index);
  return SaveSettings(profile.getFileName(), false);*/
  return true;
}


bool CSettings::LoadProfiles(const CStdString& strSettingsFile)
{
  TiXmlDocument profilesDoc;
  if (!CFile::Exists(strSettingsFile))
  { // set defaults, or assume no rss feeds??
    return false;
  }
  if (!profilesDoc.LoadFile(strSettingsFile))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", strSettingsFile.c_str(), profilesDoc.ErrorRow(), profilesDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = profilesDoc.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"profiles") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <profiles> node", strSettingsFile.c_str());
    return false;
  }
  GetInteger(pRootElement,"lastloaded",m_iLastLoadedProfileIndex,0,0,1000);
  if (m_iLastLoadedProfileIndex < 0)
    m_iLastLoadedProfileIndex = 0;

  XMLUtils::GetBoolean(pRootElement,"useloginscreen",bUseLoginScreen);

  TiXmlElement* pProfile = pRootElement->FirstChildElement("profile");
  CProfile profile;

  while (pProfile)
  {
    profile.setName("Master user");
    if (CDirectory::Exists("special://home/userdata"))
      profile.setDirectory("special://home/userdata");
    else
      profile.setDirectory("special://xbmc/userdata");

    CStdString strName;
    XMLUtils::GetString(pProfile,"name",strName);
    profile.setName(strName);

    CStdString strDirectory;
    XMLUtils::GetPath(pProfile,"directory",strDirectory);
    profile.setDirectory(strDirectory);

    CStdString strThumb;
    XMLUtils::GetPath(pProfile,"thumbnail",strThumb);
    profile.setThumb(strThumb);

    bool bHas=true;
    XMLUtils::GetBoolean(pProfile, "hasdatabases", bHas);
    profile.setDatabases(bHas);

    bHas = true;
    XMLUtils::GetBoolean(pProfile, "canwritedatabases", bHas);
    profile.setWriteDatabases(bHas);

    bHas = true;
    XMLUtils::GetBoolean(pProfile, "hassources", bHas);
    profile.setSources(bHas);

    bHas = true;
    XMLUtils::GetBoolean(pProfile, "canwritesources", bHas);
    profile.setWriteSources(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "locksettings", bHas);
    profile.setSettingsLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockfiles", bHas);
    profile.setFilesLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockmusic", bHas);
    profile.setMusicLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockvideo", bHas);
    profile.setVideoLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockpictures", bHas);
    profile.setPicturesLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockprograms", bHas);
    profile.setProgramsLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "useavpacksettings", bHas);
    profile.setUseAvpackSettings(bHas);
    LockType iLockMode=LOCK_MODE_EVERYONE;
    int lockMode;
    XMLUtils::GetInt(pProfile,"lockmode",lockMode);
    iLockMode = (LockType)lockMode;

    if (iLockMode > LOCK_MODE_QWERTY || iLockMode < LOCK_MODE_EVERYONE)
      iLockMode = LOCK_MODE_EVERYONE;
    profile.setLockMode(iLockMode);

    CStdString strLockCode;
    XMLUtils::GetString(pProfile,"lockcode",strLockCode);
    profile.setLockCode(strLockCode);

    CStdString strDate;
    XMLUtils::GetString(pProfile,"lastdate",strDate);
    profile.setDate(strDate);

    m_vecProfiles.push_back(profile);
    pProfile = pProfile->NextSiblingElement("profile");
  }

  if (m_iLastLoadedProfileIndex >= (int)m_vecProfiles.size() || m_iLastLoadedProfileIndex < 0)
    m_iLastLoadedProfileIndex = 0;

  return true;
}

bool CSettings::SaveProfiles(const CStdString& strSettingsFile) const
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("profiles");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  XMLUtils::SetInt(pRoot,"lastloaded",m_iLastLoadedProfileIndex);
  XMLUtils::SetBoolean(pRoot,"useloginscreen",bUseLoginScreen);
  for (unsigned int iProfile=0;iProfile<g_settings.m_vecProfiles.size();++iProfile)
  {
    TiXmlElement profileNode("profile");
    TiXmlNode *pNode = pRoot->InsertEndChild(profileNode);
    XMLUtils::SetString(pNode,"name",g_settings.m_vecProfiles[iProfile].getName());
    XMLUtils::SetPath(pNode,"directory",g_settings.m_vecProfiles[iProfile].getDirectory());
    XMLUtils::SetPath(pNode,"thumbnail",g_settings.m_vecProfiles[iProfile].getThumb());
    XMLUtils::SetString(pNode,"lastdate",g_settings.m_vecProfiles[iProfile].getDate());
    XMLUtils::SetBoolean(pNode,"useavpacksettings",g_settings.m_vecProfiles[iProfile].useAvpackSettings());

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    {
      XMLUtils::SetInt(pNode,"lockmode",g_settings.m_vecProfiles[iProfile].getLockMode());
      XMLUtils::SetString(pNode,"lockcode",g_settings.m_vecProfiles[iProfile].getLockCode());
      XMLUtils::SetBoolean(pNode,"lockmusic",g_settings.m_vecProfiles[iProfile].musicLocked());
      XMLUtils::SetBoolean(pNode,"lockvideo",g_settings.m_vecProfiles[iProfile].videoLocked());
      XMLUtils::SetBoolean(pNode,"lockpictures",g_settings.m_vecProfiles[iProfile].picturesLocked());
      XMLUtils::SetBoolean(pNode,"lockprograms",g_settings.m_vecProfiles[iProfile].programsLocked());
      XMLUtils::SetBoolean(pNode,"locksettings",g_settings.m_vecProfiles[iProfile].settingsLocked());
      XMLUtils::SetBoolean(pNode,"lockfiles",g_settings.m_vecProfiles[iProfile].filesLocked());
    }

    if (iProfile > 0)
    {
      XMLUtils::SetBoolean(pNode,"hasdatabases",g_settings.m_vecProfiles[iProfile].hasDatabases());
      XMLUtils::SetBoolean(pNode,"canwritedatabases",g_settings.m_vecProfiles[iProfile].canWriteDatabases());
      XMLUtils::SetBoolean(pNode,"hassources",g_settings.m_vecProfiles[iProfile].hasSources());
      XMLUtils::SetBoolean(pNode,"canwritesources",g_settings.m_vecProfiles[iProfile].canWriteSources());
    }
  }
  // save the file
  return xmlDoc.SaveFile(strSettingsFile);
}

bool CSettings::LoadUPnPXml(const CStdString& strSettingsFile)
{
  TiXmlDocument UPnPDoc;

  if (!CFile::Exists(strSettingsFile))
  { // set defaults, or assume no rss feeds??
    return false;
  }
  if (!UPnPDoc.LoadFile(strSettingsFile))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", strSettingsFile.c_str(), UPnPDoc.ErrorRow(), UPnPDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = UPnPDoc.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"upnpserver") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <upnpserver> node", strSettingsFile.c_str());
    return false;
  }
  // load settings

  // default values for ports
  g_settings.m_UPnPPortServer = 0;
  g_settings.m_UPnPPortRenderer = 0;
  g_settings.m_UPnPMaxReturnedItems = 0;

  XMLUtils::GetString(pRootElement, "UUID", g_settings.m_UPnPUUIDServer);
  XMLUtils::GetInt(pRootElement, "Port", g_settings.m_UPnPPortServer);
  XMLUtils::GetInt(pRootElement, "MaxReturnedItems", g_settings.m_UPnPMaxReturnedItems);
  XMLUtils::GetString(pRootElement, "UUIDRenderer", g_settings.m_UPnPUUIDRenderer);
  XMLUtils::GetInt(pRootElement, "PortRenderer", g_settings.m_UPnPPortRenderer);

  CStdString strDefault;
  GetSources(pRootElement,"music",g_settings.m_UPnPMusicSources,strDefault);
  GetSources(pRootElement,"video",g_settings.m_UPnPVideoSources,strDefault);
  GetSources(pRootElement,"pictures",g_settings.m_UPnPPictureSources,strDefault);

  return true;
}

bool CSettings::SaveUPnPXml(const CStdString& strSettingsFile) const
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("upnpserver");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  // create a new Element for UUID
  XMLUtils::SetString(pRoot, "UUID", g_settings.m_UPnPUUIDServer);
  XMLUtils::SetInt(pRoot, "Port", g_settings.m_UPnPPortServer);
  XMLUtils::SetInt(pRoot, "MaxReturnedItems", g_settings.m_UPnPMaxReturnedItems);
  XMLUtils::SetString(pRoot, "UUIDRenderer", g_settings.m_UPnPUUIDRenderer);
  XMLUtils::SetInt(pRoot, "PortRenderer", g_settings.m_UPnPPortRenderer);

  VECSOURCES* pShares[3];
  pShares[0] = &g_settings.m_UPnPMusicSources;
  pShares[1] = &g_settings.m_UPnPVideoSources;
  pShares[2] = &g_settings.m_UPnPPictureSources;
  for (int k=0;k<3;++k)
  {
    if ((*pShares)[k].size()==0)
      continue;

    TiXmlElement xmlType("");
    if (k==0)
      xmlType = TiXmlElement("music");
    if (k==1)
      xmlType = TiXmlElement("video");
    if (k==2)
      xmlType = TiXmlElement("pictures");
    
    TiXmlNode* pNode = pRoot->InsertEndChild(xmlType);  

    for (unsigned int j=0;j<(*pShares)[k].size();++j)
    {
      // create a new Element
      TiXmlText xmlName((*pShares)[k][j].strName);
      TiXmlElement eName("name");
      eName.InsertEndChild(xmlName);

      TiXmlElement source("source");
      source.InsertEndChild(eName);

      for (unsigned int i = 0; i < (*pShares)[k][j].vecPaths.size(); i++)
      {
        TiXmlText xmlPath((*pShares)[k][j].vecPaths[i]);
        TiXmlElement ePath("path");
        ePath.InsertEndChild(xmlPath);
        source.InsertEndChild(ePath);
      }

      if (pNode)
        pNode->ToElement()->InsertEndChild(source);
    }
  }
  // save the file
  return xmlDoc.SaveFile(strSettingsFile);
}

bool CSettings::UpdateShare(const CStdString &type, const CStdString oldName, const CMediaSource &share)
{
  VECSOURCES *pShares = GetSourcesFromType(type);

  if (!pShares) return false;

  // update our current share list
  CMediaSource* pShare=NULL;
  for (IVECSOURCES it = pShares->begin(); it != pShares->end(); it++)
  {
    if ((*it).strName == oldName)
    {
      (*it).strName = share.strName;
      (*it).strPath = share.strPath;
      (*it).vecPaths = share.vecPaths;
      pShare = &(*it);
      break;
    }
  }

  if (!pShare)
    return false;

  // Update our XML file as well
  return SaveSources();
}

// NOTE: This function does NOT save the sources.xml file - you need to call SaveSources() separately.
bool CSettings::UpdateSource(const CStdString &strType, const CStdString strOldName, const CStdString &strUpdateElement, const CStdString &strUpdateText)
{
  VECSOURCES *pShares = GetSourcesFromType(strType);

  if (!pShares) return false;

  // disallow virtual paths
  if (strUpdateElement.Equals("path") && CUtil::IsVirtualPath(strUpdateText))
    return false;

  for (IVECSOURCES it = pShares->begin(); it != pShares->end(); it++)
  {
    if ((*it).strName == strOldName)
    {
      if ("name" == strUpdateElement)
        (*it).strName = strUpdateText;
      else if ("lockmode" == strUpdateElement)
        (*it).m_iLockMode = LockType(atoi(strUpdateText));
      else if ("lockcode" == strUpdateElement)
        (*it).m_strLockCode = strUpdateText;
      else if ("badpwdcount" == strUpdateElement)
        (*it).m_iBadPwdCount = atoi(strUpdateText);
      else if ("thumbnail" == strUpdateElement)
        (*it).m_strThumbnailImage = strUpdateText;
      else if ("path" == strUpdateElement)
      {
        (*it).vecPaths.clear();
        (*it).strPath = strUpdateText;
        (*it).vecPaths.push_back(strUpdateText);
      }
      else
        return false;
      return true;
    }
  }
  return false;
}

bool CSettings::DeleteSource(const CStdString &strType, const CStdString strName, const CStdString strPath)
{
  VECSOURCES *pShares = GetSourcesFromType(strType);
  if (!pShares) return false;

  bool found(false);

  for (IVECSOURCES it = pShares->begin(); it != pShares->end(); it++)
  {
    if ((*it).strName == strName && (*it).strPath == strPath)
    {
      CLog::Log(LOGDEBUG,"found share, removing!");
      pShares->erase(it);
      found = true;
      break;
    }
  }

  if (strType.Find("upnp") > -1)
    return found;

  return SaveSources();
}

bool CSettings::AddShare(const CStdString &type, const CMediaSource &share)
{
  VECSOURCES *pShares = GetSourcesFromType(type);
  if (!pShares) return false;

  // translate dir and add to our current shares
  CStdString strPath1 = share.strPath;
  strPath1.ToUpper();
  if(strPath1.IsEmpty())
  {
    CLog::Log(LOGERROR, "unable to add empty path");
    return false;
  }

  CMediaSource shareToAdd = share;
  if (strPath1.at(0) == '$')
  {
    shareToAdd.strPath = CUtil::TranslateSpecialSource(strPath1);
    if (!share.strPath.IsEmpty())
      CLog::Log(LOGDEBUG, "%s Translated (%s) to Path (%s)",__FUNCTION__ ,strPath1.c_str(),shareToAdd.strPath.c_str());
    else
    {
      CLog::Log(LOGDEBUG, "%s Skipping invalid special directory token: %s",__FUNCTION__,strPath1.c_str());
      return false;
    }
  }
  pShares->push_back(shareToAdd);

  if (type.Find("upnp") < 0)
  {
    return SaveSources();
  }
  return true;
}

bool CSettings::SaveSources()
{
  // TODO: Should we be specifying utf8 here??
  TiXmlDocument doc;
  TiXmlElement xmlRootElement("sources");
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  // ok, now run through and save each sources section
  SetSources(pRoot, "programs", g_settings.m_programSources, g_settings.m_defaultProgramSource);
  SetSources(pRoot, "video", g_settings.m_videoSources, g_settings.m_defaultVideoSource);
  SetSources(pRoot, "music", g_settings.m_musicSources, g_settings.m_defaultMusicSource);
  SetSources(pRoot, "pictures", g_settings.m_pictureSources, g_settings.m_defaultPictureSource);
  SetSources(pRoot, "files", g_settings.m_fileSources, g_settings.m_defaultFileSource);

  return doc.SaveFile(g_settings.GetSourcesFile());
}

bool CSettings::SetSources(TiXmlNode *root, const char *section, const VECSOURCES &shares, const char *defaultPath)
{
  TiXmlElement sectionElement(section);
  TiXmlNode *sectionNode = root->InsertEndChild(sectionElement);
  if (sectionNode)
  {
    XMLUtils::SetPath(sectionNode, "default", defaultPath);
    for (unsigned int i = 0; i < shares.size(); i++)
    {
      const CMediaSource &share = shares[i];
      if (share.m_ignore)
        continue;
      TiXmlElement source("source");

      XMLUtils::SetString(&source, "name", share.strName);

      for (unsigned int i = 0; i < share.vecPaths.size(); i++)
        XMLUtils::SetPath(&source, "path", share.vecPaths[i]);

      if (share.m_iHasLock)
      {
        XMLUtils::SetInt(&source, "lockmode", share.m_iLockMode);
        XMLUtils::SetString(&source, "lockcode", share.m_strLockCode);
        XMLUtils::SetInt(&source, "badpwdcount", share.m_iBadPwdCount);
      }
      if (!share.m_strThumbnailImage.IsEmpty())
        XMLUtils::SetPath(&source, "thumbnail", share.m_strThumbnailImage);

      sectionNode->InsertEndChild(source);
    } 
  }
  return true;
}

void CSettings::LoadSkinSettings(const TiXmlElement* pRootElement)
{
  int number = 0;
  const TiXmlElement *pElement = pRootElement->FirstChildElement("skinsettings");
  if (pElement)
  {
    m_skinStrings.clear();
    m_skinBools.clear();
    const TiXmlElement *pChild = pElement->FirstChildElement("setting");
    while (pChild)
    {
      CStdString settingName = pChild->Attribute("name");
      if (pChild->Attribute("type") && strcmpi(pChild->Attribute("type"),"string") == 0)
      { // string setting
        CSkinString string;
        string.name = settingName;
        string.value = pChild->FirstChild() ? pChild->FirstChild()->Value() : "";
        m_skinStrings.insert(pair<int, CSkinString>(number++, string));
      }
      else
      { // bool setting
        CSkinBool setting;
        setting.name = settingName;
        setting.value = pChild->FirstChild() ? strcmpi(pChild->FirstChild()->Value(), "true") == 0 : false;
        m_skinBools.insert(pair<int, CSkinBool>(number++, setting));
      }
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
  for (map<int, CSkinBool>::const_iterator it = m_skinBools.begin(); it != m_skinBools.end(); ++it)
  {
    // Add a <setting type="bool" name="name">true/false</setting>
    TiXmlElement xmlSetting("setting");
    xmlSetting.SetAttribute("type", "bool");
    xmlSetting.SetAttribute("name", (*it).second.name.c_str());
    TiXmlText xmlBool((*it).second.value ? "true" : "false");
    xmlSetting.InsertEndChild(xmlBool);
    pSettingsNode->InsertEndChild(xmlSetting);
  }
  for (map<int, CSkinString>::const_iterator it = m_skinStrings.begin(); it != m_skinStrings.end(); ++it)
  {
    // Add a <setting type="string" name="name">string</setting>
    TiXmlElement xmlSetting("setting");
    xmlSetting.SetAttribute("type", "string");
    xmlSetting.SetAttribute("name", (*it).second.name.c_str());
    TiXmlText xmlLabel((*it).second.value);
    xmlSetting.InsertEndChild(xmlLabel);
    pSettingsNode->InsertEndChild(xmlSetting);
  }
}

void CSettings::Clear()
{
  m_programSources.clear();
  m_pictureSources.clear();
  m_fileSources.clear();
  m_musicSources.clear();
  m_videoSources.clear();
//  m_vecIcons.clear();
  m_vecProfiles.clear();
  g_advancedSettings.m_videoCleanRegExps.clear();
  g_advancedSettings.m_moviesExcludeFromScanRegExps.clear();
  g_advancedSettings.m_tvshowExcludeFromScanRegExps.clear();
  g_advancedSettings.m_videoExcludeFromListingRegExps.clear();
  g_advancedSettings.m_videoStackRegExps.clear();
  g_advancedSettings.m_audioExcludeFromScanRegExps.clear();
  g_advancedSettings.m_audioExcludeFromListingRegExps.clear();
  g_advancedSettings.m_pictureExcludeFromListingRegExps.clear();
  m_mapRssUrls.clear();
  m_skinBools.clear();
  m_skinStrings.clear();
}

int CSettings::TranslateSkinString(const CStdString &setting)
{
  CStdString settingName;
  settingName.Format("%s.%s", g_guiSettings.GetString("lookandfeel.skin").c_str(), setting);
  // run through and see if we have this setting
  for (map<int, CSkinString>::const_iterator it = m_skinStrings.begin(); it != m_skinStrings.end(); it++)
  {
    if (settingName.Equals((*it).second.name))
      return (*it).first;
  }
  // didn't find it - insert it
  CSkinString skinString;
  skinString.name = settingName;
  m_skinStrings.insert(pair<int, CSkinString>(m_skinStrings.size() + m_skinBools.size(), skinString));
  return m_skinStrings.size() + m_skinBools.size() - 1;
}

const CStdString &CSettings::GetSkinString(int setting) const
{
  map<int, CSkinString>::const_iterator it = m_skinStrings.find(setting);
  if (it != m_skinStrings.end())
  {
    return (*it).second.value;
  }
  return StringUtils::EmptyString;
}

void CSettings::SetSkinString(int setting, const CStdString &label)
{
  map<int, CSkinString>::iterator it = m_skinStrings.find(setting);
  if (it != m_skinStrings.end())
  {
    (*it).second.value = label;
    return;
  }
  assert(false);
  CLog::Log(LOGFATAL, "%s : Unknown setting requested", __FUNCTION__);
}

void CSettings::ResetSkinSetting(const CStdString &setting)
{
  CStdString settingName;
  settingName.Format("%s.%s", g_guiSettings.GetString("lookandfeel.skin").c_str(), setting);
  // run through and see if we have this setting as a string
  for (map<int, CSkinString>::iterator it = m_skinStrings.begin(); it != m_skinStrings.end(); it++)
  {
    if (settingName.Equals((*it).second.name))
    {
      (*it).second.value = "";
      return;
    }
  }
  // and now check for the skin bool
  for (map<int, CSkinBool>::iterator it = m_skinBools.begin(); it != m_skinBools.end(); it++)
  {
    if (settingName.Equals((*it).second.name))
    {
      (*it).second.value = false;
      return;
    }
  }
}

int CSettings::TranslateSkinBool(const CStdString &setting)
{
  CStdString settingName;
  settingName.Format("%s.%s", g_guiSettings.GetString("lookandfeel.skin").c_str(), setting);
  // run through and see if we have this setting
  for (map<int, CSkinBool>::const_iterator it = m_skinBools.begin(); it != m_skinBools.end(); it++)
  {
    if (settingName.Equals((*it).second.name))
      return (*it).first;
  }
  // didn't find it - insert it
  CSkinBool skinBool;
  skinBool.name = settingName;
  skinBool.value = false;
  m_skinBools.insert(pair<int, CSkinBool>(m_skinBools.size() + m_skinStrings.size(), skinBool));
  return m_skinBools.size() + m_skinStrings.size() - 1;
}

bool CSettings::GetSkinBool(int setting) const
{
  map<int, CSkinBool>::const_iterator it = m_skinBools.find(setting);
  if (it != m_skinBools.end())
  {
    return (*it).second.value;
  }
  // default is to return false
  return false;
}

void CSettings::SetSkinBool(int setting, bool set)
{
  map<int, CSkinBool>::iterator it = m_skinBools.find(setting);
  if (it != m_skinBools.end())
  {
    (*it).second.value = set;
    return;
  }
  assert(false);
  CLog::Log(LOGFATAL,"%s : Unknown setting requested", __FUNCTION__);
}

void CSettings::ResetSkinSettings()
{
  CStdString currentSkin = g_guiSettings.GetString("lookandfeel.skin") + ".";
  // clear all the settings and strings from this skin.
  map<int, CSkinBool>::iterator it = m_skinBools.begin();
  while (it != m_skinBools.end())
  {
    CStdString skinName = (*it).second.name;
    if (skinName.Left(currentSkin.size()) == currentSkin)
      (*it).second.value = false;

    it++;
  }
  map<int, CSkinString>::iterator it2 = m_skinStrings.begin();
  while (it2 != m_skinStrings.end())
  {
    CStdString skinName = (*it2).second.name;
    if (skinName.Left(currentSkin.size()) == currentSkin)
      (*it2).second.value = "";

    it2++;
  }
  g_infoManager.ResetCache();
}

void CSettings::LoadUserFolderLayout()
{
  // check them all
  CStdString strDir = g_guiSettings.GetString("system.playlistspath");
  if (strDir == "set default")
  {
    strDir = "P:\\playlists\\";
    g_guiSettings.SetString("system.playlistspath",strDir.c_str());
  }
  CDirectory::Create(strDir);
  CDirectory::Create(CUtil::AddFileToFolder(strDir,"music"));
  CDirectory::Create(CUtil::AddFileToFolder(strDir,"video"));
  CDirectory::Create(CUtil::AddFileToFolder(strDir,"mixed"));
}

CStdString CSettings::GetProfileUserDataFolder() const
{
  CStdString folder;
  if (m_iLastLoadedProfileIndex == 0)
    return GetUserDataFolder();

  CUtil::AddFileToFolder(GetUserDataFolder(),m_vecProfiles[m_iLastLoadedProfileIndex].getDirectory(),folder);

  return folder;
}

CStdString CSettings::GetUserDataItem(const CStdString& strFile) const
{
  CStdString folder;
  folder = "special://profile/"+strFile;
  if (!CFile::Exists(folder))
    folder = "special://masterprofile/"+strFile;
  return folder;
}

CStdString CSettings::GetUserDataFolder() const
{
  return m_vecProfiles[0].getDirectory();
}

CStdString CSettings::GetDatabaseFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Database", folder);
  else
    CUtil::AddFileToFolder(GetUserDataFolder(), "Database", folder);

  return folder;
}

CStdString CSettings::GetCDDBFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Database/CDDB", folder);
  else
    CUtil::AddFileToFolder(GetUserDataFolder(), "Database/CDDB", folder);

  return folder;
}

CStdString CSettings::GetThumbnailsFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails", folder);

  return folder;
}

CStdString CSettings::GetMusicThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/Music", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Music", folder);

  return folder;
}

CStdString CSettings::GetLastFMThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/Music/LastFM", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Music/LastFM", folder);

  return folder;
}

CStdString CSettings::GetMusicArtistThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/Music/Artists", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Music/Artists", folder);

  return folder;
}

CStdString CSettings::GetVideoThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/Video", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Video", folder);

  return folder;
}

CStdString CSettings::GetVideoFanartFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/Video/Fanart", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Video/Fanart", folder);

  return folder;
}

CStdString CSettings::GetMusicFanartFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/Music/Fanart", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Music/Fanart", folder);

  return folder;
}

CStdString CSettings::GetBookmarksThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/Video/Bookmarks", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Video/Bookmarks", folder);

  return folder;
}

CStdString CSettings::GetPicturesThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/Pictures", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Pictures", folder);

  return folder;
}

CStdString CSettings::GetProgramsThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/Programs", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Programs", folder);

  return folder;
}

CStdString CSettings::GetGameSaveThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails/GameSaves", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/GameSaves", folder);

  return folder;
}

CStdString CSettings::GetProfilesThumbFolder() const
{
  CStdString folder;
  CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails/Profiles", folder);

  return folder;
}

CStdString CSettings::GetSourcesFile() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasSources())
    CUtil::AddFileToFolder(GetProfileUserDataFolder(),"sources.xml",folder);
  else
    CUtil::AddFileToFolder(GetUserDataFolder(),"sources.xml",folder);

  return folder;
}

CStdString CSettings::GetSkinFolder() const
{
  CStdString folder;

  // Get the Current Skin Path
  return GetSkinFolder(g_guiSettings.GetString("lookandfeel.skin"));
}

CStdString CSettings::GetScriptsFolder() const
{
  CStdString folder = "special://home/scripts";

  if ( CDirectory::Exists(folder) )
    return folder;

  folder = "special://xbmc/scripts";
  return folder;
}

CStdString CSettings::GetSkinFolder(const CStdString &skinName) const
{
  CStdString folder;

  // Get the Current Skin Path
  CUtil::AddFileToFolder("special://home/skin/", skinName, folder);
  if ( ! CDirectory::Exists(folder) )
    CUtil::AddFileToFolder("special://xbmc/skin/", skinName, folder);

  return folder;
}

void CSettings::LoadRSSFeeds()
{
  CStdString rssXML;
  rssXML = GetUserDataItem("RssFeeds.xml");
  TiXmlDocument rssDoc;
  if (!CFile::Exists(rssXML))
  { // set defaults, or assume no rss feeds??
    return;
  }
  if (!rssDoc.LoadFile(rssXML))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", rssXML.c_str(), rssDoc.ErrorRow(), rssDoc.ErrorDesc());
    return;
  }

  TiXmlElement *pRootElement = rssDoc.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"rssfeeds") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <rssfeeds> node", rssXML.c_str());
    return;
  }

  g_settings.m_mapRssUrls.clear();
  TiXmlElement* pSet = pRootElement->FirstChildElement("set");
  while (pSet)
  {
    int iId;
    if (pSet->QueryIntAttribute("id", &iId) == TIXML_SUCCESS)
    {
      vector<string> vecSet;
      vector<int> vecIntervals;
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
          // TODO: UTF-8: Do these URLs need to be converted to UTF-8?
          //              What about the xml encoding?
          CStdString strUrl = pFeed->FirstChild()->Value();
          vecSet.push_back(strUrl);
          vecIntervals.push_back(iInterval);
        }
        pFeed = pFeed->NextSiblingElement("feed");
      }
      g_settings.m_mapRssUrls.insert(make_pair(iId,make_pair(vecIntervals,vecSet)));
    }
    else
      CLog::Log(LOGERROR,"found rss url set with no id in RssFeeds.xml, ignored");

    pSet = pSet->NextSiblingElement("set");
  }
}

CStdString CSettings::GetSettingsFile() const
{
  CStdString settings;
  if (g_settings.m_iLastLoadedProfileIndex == 0)
    settings = "special://masterprofile/guisettings.xml";
  else
    settings = "special://profile/guisettings.xml";
  return settings;
}

CStdString CSettings::GetAvpackSettingsFile() const
{
  CStdString  strAvpackSettingsFile;
  if (g_settings.m_iLastLoadedProfileIndex == 0)
    strAvpackSettingsFile = "T:\\avpacksettings.xml";
  else
    strAvpackSettingsFile = "P:\\avpacksettings.xml";
  return strAvpackSettingsFile;
}

void CSettings::CreateProfileFolders()
{
  CDirectory::Create(GetDatabaseFolder());
  CDirectory::Create(GetCDDBFolder());

  // Thumbnails/
  CDirectory::Create(GetThumbnailsFolder());
  CDirectory::Create(GetMusicThumbFolder());
  CDirectory::Create(GetMusicArtistThumbFolder());
  CDirectory::Create(GetLastFMThumbFolder());
  CDirectory::Create(GetVideoThumbFolder());
  CDirectory::Create(GetVideoFanartFolder());
  CDirectory::Create(GetMusicFanartFolder());
  CDirectory::Create(GetBookmarksThumbFolder());
  CDirectory::Create(GetProgramsThumbFolder());
  CDirectory::Create(GetPicturesThumbFolder());
  CLog::Log(LOGINFO, "  thumbnails folder:%s", GetThumbnailsFolder().c_str());
  for (unsigned int hex=0; hex < 16; hex++)
  {
    CStdString strHex;
    strHex.Format("%x",hex);
    CDirectory::Create(CUtil::AddFileToFolder(GetPicturesThumbFolder(), strHex));
    CDirectory::Create(CUtil::AddFileToFolder(GetMusicThumbFolder(), strHex));
    CDirectory::Create(CUtil::AddFileToFolder(GetVideoThumbFolder(), strHex));
  }
  CDirectory::Create("special://profile/visualisations");
}
