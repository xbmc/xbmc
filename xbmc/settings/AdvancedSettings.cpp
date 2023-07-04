/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AdvancedSettings.h"

#include "LangInfo.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/AppParams.h"
#include "filesystem/SpecialProtocol.h"
#include "network/DNSNameCache.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "utils/FileUtils.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <climits>
#include <regex>
#include <string>
#include <vector>

#include <tinyxml2.h>

using namespace ADDON;

CAdvancedSettings::CAdvancedSettings()
{
  m_initialized = false;
  m_fullScreen = false;
}

void CAdvancedSettings::OnSettingsLoaded()
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  // load advanced settings
  Load(*profileManager);

  // default players?
  CLog::Log(LOGINFO, "Default Video Player: {}", m_videoDefaultPlayer);
  CLog::Log(LOGINFO, "Default Audio Player: {}", m_audioDefaultPlayer);

  // setup any logging...
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  if (settings->GetBool(CSettings::SETTING_DEBUG_SHOWLOGINFO))
  {
    m_logLevel = std::max(m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
    CLog::Log(LOGINFO, "Enabled debug logging due to GUI setting ({})", m_logLevel);
  }
  else
  {
    m_logLevel = std::min(m_logLevelHint, LOG_LEVEL_DEBUG/*LOG_LEVEL_NORMAL*/);
    CLog::Log(LOGINFO, "Disabled debug logging due to GUI setting. Level {}.", m_logLevel);
  }
  CServiceBroker::GetLogging().SetLogLevel(m_logLevel);
}

void CAdvancedSettings::OnSettingsUnloaded()
{
  m_initialized = false;
}

void CAdvancedSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_DEBUG_SHOWLOGINFO)
    SetDebugMode(std::static_pointer_cast<const CSettingBool>(setting)->GetValue());
}

void CAdvancedSettings::Initialize(CSettingsManager& settingsMgr)
{
  Initialize();

  const auto params = CServiceBroker::GetAppParams();

  if (params->GetLogLevel() == LOG_LEVEL_DEBUG)
  {
    m_logLevel = LOG_LEVEL_DEBUG;
    m_logLevelHint = LOG_LEVEL_DEBUG;
    CServiceBroker::GetLogging().SetLogLevel(LOG_LEVEL_DEBUG);
  }

  const std::string& settingsFile = params->GetSettingsFile();
  if (!settingsFile.empty())
    AddSettingsFile(settingsFile);

  if (params->IsStartFullScreen())
    m_startFullScreen = true;

  if (params->IsStandAlone())
    m_handleMounting = true;

  settingsMgr.RegisterSettingsHandler(this, true);
  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_DEBUG_SHOWLOGINFO);
  settingsMgr.RegisterCallback(this, settingSet);
}

void CAdvancedSettings::Uninitialize(CSettingsManager& settingsMgr)
{
  settingsMgr.UnregisterCallback(this);
  settingsMgr.UnregisterSettingsHandler(this);
  settingsMgr.UnregisterSettingOptionsFiller("loggingcomponents");

  Clear();

  m_initialized = false;
}

void CAdvancedSettings::Initialize()
{
  if (m_initialized)
    return;

  m_audioApplyDrc = -1.0f;
  m_VideoPlayerIgnoreDTSinWAV = false;

  //default hold time of 25 ms, this allows a 20 hertz sine to pass undistorted
  m_limiterHold = 0.025f;
  m_limiterRelease = 0.1f;

  m_seekSteps = { 10, 30, 60, 180, 300, 600, 1800 };

  m_audioDefaultPlayer = "paplayer";
  m_audioPlayCountMinimumPercent = 90.0f;

  m_videoSubsDelayRange = 60;
  m_videoAudioDelayRange = 10;
  m_videoUseTimeSeeking = true;
  m_videoTimeSeekForward = 30;
  m_videoTimeSeekBackward = -30;
  m_videoTimeSeekForwardBig = 600;
  m_videoTimeSeekBackwardBig = -600;
  m_videoPercentSeekForward = 2;
  m_videoPercentSeekBackward = -2;
  m_videoPercentSeekForwardBig = 10;
  m_videoPercentSeekBackwardBig = -10;

  m_videoPPFFmpegPostProc = "ha:128:7,va,dr";
  m_videoDefaultPlayer = "VideoPlayer";
  m_videoIgnoreSecondsAtStart = 3*60;
  m_videoIgnorePercentAtEnd   = 8.0f;
  m_videoPlayCountMinimumPercent = 90.0f;
  m_videoVDPAUScaling = -1;
  m_videoNonLinStretchRatio = 0.5f;
  m_videoAutoScaleMaxFps = 30.0f;
  m_videoCaptureUseOcclusionQuery = -1; //-1 is auto detect
  m_videoVDPAUtelecine = false;
  m_videoVDPAUdeintSkipChromaHD = false;
  m_DXVACheckCompatibility = false;
  m_DXVACheckCompatibilityPresent = false;
  m_videoFpsDetect = 1;
  m_maxTempo = 1.55f;
  m_videoPreferStereoStream = false;

  m_videoDefaultLatency = 0.0;
  m_videoDefaultHdrExtraLatency = 0.0;

  m_musicUseTimeSeeking = true;
  m_musicTimeSeekForward = 10;
  m_musicTimeSeekBackward = -10;
  m_musicTimeSeekForwardBig = 60;
  m_musicTimeSeekBackwardBig = -60;
  m_musicPercentSeekForward = 1;
  m_musicPercentSeekBackward = -1;
  m_musicPercentSeekForwardBig = 10;
  m_musicPercentSeekBackwardBig = -10;

  m_slideshowPanAmount = 2.5f;
  m_slideshowZoomAmount = 5.0f;
  m_slideshowBlackBarCompensation = 20.0f;

  m_songInfoDuration = 10;

  m_cddbAddress = "gnudb.gnudb.org";
  m_addSourceOnTop = false;

  m_handleMounting = CServiceBroker::GetAppParams()->IsStandAlone();

  m_fullScreenOnMovieStart = true;
  m_cachePath = "special://temp/";

  m_videoFilenameIdentifierRegExp = R"([\{\[](\w+?)(?:id)?[-=](\w+)[\}|\]])";
  m_videoCleanDateTimeRegExp = "(.*[^ _\\,\\.\\(\\)\\[\\]\\-])[ _\\.\\(\\)\\[\\]\\-]+(19[0-9][0-9]|20[0-9][0-9])([ _\\,\\.\\(\\)\\[\\]\\-]|[^0-9]$)?";

  m_videoCleanStringRegExps.clear();
  m_videoCleanStringRegExps.emplace_back(
      "[ "
      "_\\,\\.\\(\\)\\[\\]\\-](10bit|480p|480i|576p|576i|720p|720i|1080p|1080i|2160p|3d|aac|ac3|"
      "aka|atmos|avi|bd5|bdrip|bdremux|bluray|brrip|cam|cd[1-9]|custom|dc|ddp|divx|divx5|"
      "dolbydigital|dolbyvision|dsr|dsrip|dts|dts-hdma|dts-hra|dts-x|dv|dvd|dvd5|dvd9|dvdivx|"
      "dvdrip|dvdscr|dvdscreener|extended|fragment|fs|h264|h265|hdr|hdr10|hevc|hddvd|hdrip|"
      "hdtv|hdtvrip|hrhd|hrhdtv|internal|limited|multisubs|nfofix|ntsc|ogg|ogm|pal|pdtv|proper|"
      "r3|r5|read.nfo|remastered|remux|repack|rerip|retail|screener|se|svcd|tc|telecine|"
      "telesync|truehd|ts|uhd|unrated|ws|x264|x265|xvid|xvidvd|xxx|web-dl|webrip|www.www|"
      "\\[.*\\])([ _\\,\\.\\(\\)\\[\\]\\-]|$)");
  m_videoCleanStringRegExps.emplace_back("(\\[.*\\])");

  // this vector will be inserted at the end to
  // m_moviesExcludeFromScanRegExps, m_tvshowExcludeFromScanRegExps and
  // m_audioExcludeFromScanRegExps
  m_allExcludeFromScanRegExps.clear();
  m_allExcludeFromScanRegExps.emplace_back("[\\/].+\\.ite[\\/]"); // ignore itunes extras dir
  m_allExcludeFromScanRegExps.emplace_back("[\\/]\\.\\_");
  m_allExcludeFromScanRegExps.emplace_back("\\.DS_Store");
  m_allExcludeFromScanRegExps.emplace_back("\\.AppleDouble");
  m_allExcludeFromScanRegExps.emplace_back("\\@eaDir"); // auto generated by DSM (Synology)

  m_moviesExcludeFromScanRegExps.clear();
  m_moviesExcludeFromScanRegExps.emplace_back("-trailer");
  m_moviesExcludeFromScanRegExps.emplace_back("[!-._ \\\\/]sample[-._ \\\\/]");
  m_moviesExcludeFromScanRegExps.emplace_back("[\\/](proof|subs)[\\/]");
  m_moviesExcludeFromScanRegExps.insert(m_moviesExcludeFromScanRegExps.end(),
                                        m_allExcludeFromScanRegExps.begin(),
                                        m_allExcludeFromScanRegExps.end());


  m_tvshowExcludeFromScanRegExps.clear();
  m_tvshowExcludeFromScanRegExps.emplace_back("[!-._ \\\\/]sample[-._ \\\\/]");
  m_tvshowExcludeFromScanRegExps.insert(m_tvshowExcludeFromScanRegExps.end(),
                                        m_allExcludeFromScanRegExps.begin(),
                                        m_allExcludeFromScanRegExps.end());


  m_audioExcludeFromScanRegExps.clear();
  m_audioExcludeFromScanRegExps.insert(m_audioExcludeFromScanRegExps.end(),
                                        m_allExcludeFromScanRegExps.begin(),
                                        m_allExcludeFromScanRegExps.end());

  m_folderStackRegExps.clear();
  m_folderStackRegExps.emplace_back("((cd|dvd|dis[ck])[0-9]+)$");

  m_videoStackRegExps.clear();
  m_videoStackRegExps.emplace_back("(.*?)([ _.-]*(?:cd|dvd|p(?:(?:ar)?t)|dis[ck])[ _.-]*[0-9]+)(.*?)(\\.[^.]+)$");
  m_videoStackRegExps.emplace_back("(.*?)([ _.-]*(?:cd|dvd|p(?:(?:ar)?t)|dis[ck])[ _.-]*[a-d])(.*?)(\\.[^.]+)$");
  m_videoStackRegExps.emplace_back("(.*?)([ ._-]*[a-d])(.*?)(\\.[^.]+)$");
  // This one is a bit too greedy to enable by default.  It will stack sequels
  // in a flat dir structure, but is perfectly safe in a dir-per-vid one.
  //m_videoStackRegExps.push_back("(.*?)([ ._-]*[0-9])(.*?)(\\.[^.]+)$");

  m_tvshowEnumRegExps.clear();
  // foo.s01.e01, foo.s01_e01, S01E02 foo, S01 - E02, S01xE02
  m_tvshowEnumRegExps.emplace_back(
      false, "s([0-9]+)[ ._x-]*e([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([^\\\\/]*)$");
  // foo.ep01, foo.EP_01, foo.E01
  m_tvshowEnumRegExps.emplace_back(
      false, "[\\._ -]?()e(?:p[ ._-]?)?([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([^\\\\/]*)$");
  // foo.yyyy.mm.dd.* (byDate=true)
  m_tvshowEnumRegExps.emplace_back(true, "([0-9]{4})[\\.-]([0-9]{2})[\\.-]([0-9]{2})");
  // foo.mm.dd.yyyy.* (byDate=true)
  m_tvshowEnumRegExps.emplace_back(true, "([0-9]{2})[\\.-]([0-9]{2})[\\.-]([0-9]{4})");
  // foo.1x09* or just /1x09*
  m_tvshowEnumRegExps.emplace_back(
      false, "[\\\\/\\._ \\[\\(-]([0-9]+)x([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([^\\\\/]*)$");
  // Part I, Pt.VI, Part 1
  m_tvshowEnumRegExps.emplace_back(false,
                                   "[\\/._ -]p(?:ar)?t[_. -]()([ivx]+|[0-9]+)([._ -][^\\/]*)$");
  // This regexp is for matching special episodes by their title, e.g. foo.special.mp4
  m_tvshowEnumRegExps.emplace_back(false, "[\\\\/]([^\\\\/]+)\\.special\\.[a-z0-9]+$", 0, true);

  // foo.103*, 103 foo
  // XXX: This regex is greedy and will match years in show names.  It should always be last.
  m_tvshowEnumRegExps.emplace_back(
      false,
      "[\\\\/\\._ -]([0-9]+)([0-9][0-9](?:(?:[a-i]|\\.[1-9])(?![0-9]))?)([\\._ -][^\\\\/]*)$");

  m_tvshowMultiPartEnumRegExp = "^[-_ex]+([0-9]+(?:(?:[a-i]|\\.[1-9])(?![0-9]))?)";

  m_remoteDelay = 3;
  m_bScanIRServer = true;

  m_playlistAsFolders = true;
  m_detectAsUdf = false;

  m_fanartRes = 1080;
  m_imageRes = 720;
  m_imageScalingAlgorithm = CPictureScalingAlgorithm::Default;
  m_imageQualityJpeg = 4;

  m_sambaclienttimeout = 30;
  m_sambadoscodepage = "";
  m_sambastatfiles = true;

  m_bHTTPDirectoryStatFilesize = false;

  m_bFTPThumbs = false;

  m_bShoutcastArt = true;

  m_musicThumbs = "folder.jpg|Folder.jpg|folder.JPG|Folder.JPG|cover.jpg|Cover.jpg|cover.jpeg|thumb.jpg|Thumb.jpg|thumb.JPG|Thumb.JPG";

  m_bMusicLibraryAllItemsOnBottom = false;
  m_bMusicLibraryCleanOnUpdate = false;
  m_bMusicLibraryArtistSortOnUpdate = false;
  m_iMusicLibraryRecentlyAddedItems = 25;
  m_strMusicLibraryAlbumFormat = "";
  m_prioritiseAPEv2tags = false;
  m_musicItemSeparator = " / ";
  m_musicArtistSeparators = { ";", " feat. ", " ft. " };
  m_videoItemSeparator = " / ";
  m_iMusicLibraryDateAdded = 1; // prefer mtime over ctime and current time
  m_bMusicLibraryUseISODates = false;
  m_bMusicLibraryArtistNavigatesToSongs = false;

  m_bVideoLibraryAllItemsOnBottom = false;
  m_iVideoLibraryRecentlyAddedItems = 25;
  m_bVideoLibraryCleanOnUpdate = false;
  m_bVideoLibraryUseFastHash = true;
  m_bVideoScannerIgnoreErrors = false;
  m_iVideoLibraryDateAdded = 1; // prefer mtime over ctime and current time

  m_iEpgUpdateCheckInterval = 300; /* Check every X seconds, if EPG data need to be updated. This does not mean that
                                      every X seconds an EPG update is actually triggered, it's just the interval how
                                      often to check whether an update should be triggered. If this value is greater
                                      than GUI setting 'epg.epgupdate' value, then EPG updates will done with the value
                                      specified for 'updatecheckinterval', effectively overriding the GUI setting's value. */
  m_iEpgCleanupInterval = 900; /* Remove old entries from the EPG every X seconds */
  m_iEpgActiveTagCheckInterval = 60; /* Check for updated active tags every X seconds */
  m_iEpgRetryInterruptedUpdateInterval = 30; /* Retry an interrupted EPG update after X seconds */
  m_iEpgUpdateEmptyTagsInterval = 7200; /* If a TV channel has no EPG data, try to obtain data for that channel every
                                           X seconds. This overrides the GUI setting 'epg.epgupdate' value, but only
                                           for channels without EPG data. If this value is less than 'updatecheckinterval'
                                           value, then data update will be done with the interval specified by
                                           'updatecheckinterval'.
                                           Example 1: epg.epgupdate = 120 (minutes!), updatecheckinterval = 300,
                                                      updateemptytagsinterval = 60 => trigger an EPG update for every
                                                      channel without EPG data every 5 minutes and trigger an EPG update
                                                      for every channel with EPG data every 2 hours.
                                           Example 2: epg.epgupdate = 120 (minutes!), updatecheckinterval = 300,
                                                      updateemptytagsinterval = 3600 => trigger an EPG update for every
                                                      channel without EPG data every 2 hours and trigger an EPG update
                                                      for every channel with EPG data every 1 hour. */
  m_bEpgDisplayUpdatePopup = true; /* Display a progress popup while updating EPG data from clients */
  m_bEpgDisplayIncrementalUpdatePopup = false; /* Display a progress popup while doing incremental EPG updates, but
                                                  only if 'displayupdatepopup' is also enabled. */

  m_bEdlMergeShortCommBreaks = false;      // Off by default
  m_EdlDisplayCommbreakNotifications = true; // On by default
  m_iEdlMaxCommBreakLength = 8 * 30 + 10;  // Just over 8 * 30 second commercial break.
  m_iEdlMinCommBreakLength = 3 * 30;       // 3 * 30 second commercial breaks.
  m_iEdlMaxCommBreakGap = 4 * 30;          // 4 * 30 second commercial breaks.
  m_iEdlMaxStartGap = 5 * 60;              // 5 minutes.
  m_iEdlCommBreakAutowait = 0;             // Off by default
  m_iEdlCommBreakAutowind = 0;             // Off by default

  m_curlconnecttimeout = 30;
  m_curllowspeedtime = 20;
  m_curlretries = 2;
  m_curlKeepAliveInterval = 30;
  m_curlDisableIPV6 = false;      //Certain hardware/OS combinations have trouble
                                  //with ipv6.
  m_curlDisableHTTP2 = false;

#if defined(TARGET_WINDOWS_DESKTOP)
  m_minimizeToTray = false;
#endif
#if defined(TARGET_DARWIN_EMBEDDED)
  m_startFullScreen = true;
#else
  m_startFullScreen = false;
#endif
  m_showExitButton = true;
  m_splashImage = true;

  m_playlistRetries = 100;
  m_playlistTimeout = 20; // 20 seconds timeout
  m_GLRectangleHack = false;
  m_iSkipLoopFilter = 0;
  m_bVirtualShares = true;

  m_cpuTempCmd = "";
  m_gpuTempCmd = "";
#if defined(TARGET_DARWIN)
  // default for osx is fullscreen always on top
  m_alwaysOnTop = true;
#else
  // default for windows is not always on top
  m_alwaysOnTop = false;
#endif

  m_iPVRTimeCorrection             = 0;
  m_iPVRInfoToggleInterval = 5000;
  m_bPVRChannelIconsAutoScan       = true;
  m_bPVRAutoScanIconsUserSet       = false;
  m_iPVRNumericChannelSwitchTimeout = 2000;
  m_iPVRTimeshiftThreshold = 10;
  m_bPVRTimeshiftSimpleOSD = true;
  m_PVRDefaultSortOrder.sortBy = SortByDate;
  m_PVRDefaultSortOrder.sortOrder = SortOrderDescending;

  m_addonPackageFolderSize = 200;

  m_jsonOutputCompact = true;
  m_jsonTcpPort = 9090;

  m_enableMultimediaKeys = false;

  m_canWindowed = true;
  m_guiVisualizeDirtyRegions = false;
  m_guiAlgorithmDirtyRegions = 3;
  m_guiSmartRedraw = false;
  m_airTunesPort = 36666;
  m_airPlayPort = 36667;

  m_databaseMusic.Reset();
  m_databaseVideo.Reset();

  m_useLocaleCollation = true;

  m_pictureExtensions =
      ".png|.jpg|.jpeg|.bmp|.gif|.ico|.tif|.tiff|.tga|.pcx|.cbz|.zip|.rss|.webp|.jp2|.apng|.avif";
  m_musicExtensions = ".b4s|.nsv|.m4a|.flac|.aac|.strm|.pls|.rm|.rma|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u|.gdm|.imf|.m15|.sfx|.uni|.ac3|.dts|.cue|.aif|.aiff|.wpl|.xspf|.ape|.mac|.mpc|.mp+|.mpp|.shn|.zip|.wv|.dsp|.xsp|.xwav|.waa|.wvs|.wam|.gcm|.idsp|.mpdsp|.mss|.spt|.rsd|.sap|.cmc|.cmr|.dmc|.mpt|.mpd|.rmt|.tmc|.tm8|.tm2|.oga|.url|.pxml|.tta|.rss|.wtv|.mka|.tak|.opus|.dff|.dsf|.m4b|.dtshd";
  m_videoExtensions = ".m4v|.3g2|.3gp|.nsv|.tp|.ts|.ty|.strm|.pls|.rm|.rmvb|.mpd|.m3u|.m3u8|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.nrg|.img|.iso|.udf|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mp4|.mkv|.mk3d|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli|.flv|.001|.wpl|.xspf|.zip|.vdr|.dvr-ms|.xsp|.mts|.m2t|.m2ts|.evo|.ogv|.sdp|.avs|.rec|.url|.pxml|.vc1|.h264|.rcv|.rss|.mpls|.mpl|.webm|.bdmv|.bdm|.wtv|.trp|.f4v";
  m_subtitlesExtensions = ".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|"
                          ".ass|.vtt|.idx|.ifo|.zip|.sup";
  m_discStubExtensions = ".disc";
  // internal music extensions
  m_musicExtensions += "|.cdda";
  // internal video extensions
  m_videoExtensions += "|.pvr";

  m_stereoscopicregex_3d = "[-. _]3d[-. _]";
  m_stereoscopicregex_sbs = "[-. _]h?sbs[-. _]";
  m_stereoscopicregex_tab = "[-. _]h?tab[-. _]";

  m_logLevelHint = m_logLevel = LOG_LEVEL_NORMAL;

  m_openGlDebugging = false;

  m_userAgent = g_sysinfo.GetUserAgent();

  m_nfsTimeout = 30;
  m_nfsRetries = -1;

  m_initialized = true;
}

bool CAdvancedSettings::Load(const CProfileManager &profileManager)
{
  // NOTE: This routine should NOT set the default of any of these parameters
  //       it should instead use the versions of GetString/Integer/Float that
  //       don't take defaults in.  Defaults are set in the constructor above
  Initialize(); // In case of profile switch.
  ParseSettingsFile("special://xbmc/system/advancedsettings.xml");
  for (unsigned int i = 0; i < m_settingsFiles.size(); i++)
    ParseSettingsFile(m_settingsFiles[i]);

  ParseSettingsFile(profileManager.GetUserDataItem("advancedsettings.xml"));

  // Add the list of disc stub extensions (if any) to the list of video extensions
  if (!m_discStubExtensions.empty())
    m_videoExtensions += "|" + m_discStubExtensions;

  return true;
}

void CAdvancedSettings::ParseSettingsFile(const std::string &file)
{
  CXBMCTinyXML2 advancedXML;
  if (!CFileUtils::Exists(file))
  {
    CLog::Log(LOGINFO, "No settings file to load ({})", file);
    return;
  }

  if (!advancedXML.LoadFile(file))
  {
    CLog::Log(LOGERROR, "Error loading {}, Line {}\n{}", file, advancedXML.ErrorLineNum(),
              advancedXML.ErrorStr());
    return;
  }

  auto* rootElement = advancedXML.RootElement();
  if (!rootElement || StringUtils::CompareNoCase(rootElement->Value(), "advancedsettings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading {}, no <advancedsettings> node", file);
    return;
  }

  // succeeded - tell the user it worked
  CLog::Log(LOGINFO, "Loaded settings file from {}", file);

  //Make a copy of the AS.xml and hide advancedsettings passwords
  tinyxml2::XMLDocument advancedXMLCopy;
  advancedXML.DeepCopy(&advancedXMLCopy);

  auto* rootElementCopy = advancedXMLCopy.RootElement();
  for (const auto& dbname : { "videodatabase", "musicdatabase", "tvdatabase", "epgdatabase" })
  {
    auto* db = rootElementCopy->FirstChildElement(dbname);
    if (db)
    {
      auto* passTag = db->FirstChildElement("pass");
      if (passTag)
      {
        auto* pass = passTag->FirstChild();
        if (pass)
        {
          passTag->DeleteChild(pass);
          auto* elem = advancedXMLCopy.NewElement("*****");
          passTag->InsertEndChild(elem);
        }
      }
    }
  }
  auto* network = rootElementCopy->FirstChildElement("network");
  if (network)
  {
    auto* passTag = network->FirstChildElement("httpproxypassword");
    if (passTag)
    {
      auto* pass = passTag->FirstChild();
      if (pass)
      {
        passTag->DeleteChild(pass);
        auto* elem = advancedXMLCopy.NewElement("*****");
        passTag->InsertEndChild(elem);
      }
    }
    if (network->FirstChildElement("nfstimeout"))
    {
#ifdef HAS_NFS_SET_TIMEOUT
      XMLUtils::GetUInt(network, "nfstimeout", m_nfsTimeout, 0, 3600);
#else
      CLog::Log(LOGWARNING, "nfstimeout unsupported");
#endif
    }
    if (network->FirstChildElement("nfsretries"))
    {
      XMLUtils::GetInt(network, "nfsretries", m_nfsRetries, -1, 30);
    }
  }

  // Dump contents of copied AS.xml to debug log
  tinyxml2::XMLPrinter printer;
  advancedXMLCopy.Accept(&printer);
  // redact User/pass in URLs
  std::regex redactRe("(\\w+://)\\S+:\\S+@");
  CLog::Log(LOGINFO, "Contents of {} are...\n{}", file,
            std::regex_replace(printer.CStr(), redactRe, "$1USERNAME:PASSWORD@"));

  auto* element = rootElement->FirstChildElement("audio");
  if (element)
  {
    XMLUtils::GetString(element, "defaultplayer", m_audioDefaultPlayer);
    // 101 on purpose - can be used to never automark as watched
    XMLUtils::GetFloat(element, "playcountminimumpercent", m_audioPlayCountMinimumPercent, 0.0f,
                       101.0f);

    XMLUtils::GetBoolean(element, "usetimeseeking", m_musicUseTimeSeeking);
    XMLUtils::GetInt(element, "timeseekforward", m_musicTimeSeekForward, 0, 6000);
    XMLUtils::GetInt(element, "timeseekbackward", m_musicTimeSeekBackward, -6000, 0);
    XMLUtils::GetInt(element, "timeseekforwardbig", m_musicTimeSeekForwardBig, 0, 6000);
    XMLUtils::GetInt(element, "timeseekbackwardbig", m_musicTimeSeekBackwardBig, -6000, 0);

    XMLUtils::GetInt(element, "percentseekforward", m_musicPercentSeekForward, 0, 100);
    XMLUtils::GetInt(element, "percentseekbackward", m_musicPercentSeekBackward, -100, 0);
    XMLUtils::GetInt(element, "percentseekforwardbig", m_musicPercentSeekForwardBig, 0, 100);
    XMLUtils::GetInt(element, "percentseekbackwardbig", m_musicPercentSeekBackwardBig, -100, 0);

    auto* audioExcludesNode = element->FirstChildElement("excludefromlisting");
    if (audioExcludesNode)
      GetCustomRegexps(audioExcludesNode, m_audioExcludeFromListingRegExps);

    audioExcludesNode = element->FirstChildElement("excludefromscan");
    if (audioExcludesNode)
      GetCustomRegexps(audioExcludesNode, m_audioExcludeFromScanRegExps);

    XMLUtils::GetFloat(element, "applydrc", m_audioApplyDrc);
    XMLUtils::GetBoolean(element, "VideoPlayerignoredtsinwav", m_VideoPlayerIgnoreDTSinWAV);

    XMLUtils::GetFloat(element, "limiterhold", m_limiterHold, 0.0f, 100.0f);
    XMLUtils::GetFloat(element, "limiterrelease", m_limiterRelease, 0.001f, 100.0f);
    XMLUtils::GetUInt(element, "maxpassthroughoffsyncduration", m_maxPassthroughOffSyncDuration, 20,
                      80);
    XMLUtils::GetBoolean(element, "allowmultichannelfloat", m_AllowMultiChannelFloat);
    XMLUtils::GetBoolean(element, "superviseaudiodelay", m_superviseAudioDelay);
  }

  element = rootElement->FirstChildElement("x11");
  if (element)
  {
    XMLUtils::GetBoolean(element, "omlsync", m_omlSync);
  }

  element = rootElement->FirstChildElement("video");
  if (element)
  {
    XMLUtils::GetString(element, "stereoscopicregex3d", m_stereoscopicregex_3d);
    XMLUtils::GetString(element, "stereoscopicregexsbs", m_stereoscopicregex_sbs);
    XMLUtils::GetString(element, "stereoscopicregextab", m_stereoscopicregex_tab);
    XMLUtils::GetFloat(element, "subsdelayrange", m_videoSubsDelayRange, 10, 600);
    XMLUtils::GetFloat(element, "audiodelayrange", m_videoAudioDelayRange, 10, 600);
    XMLUtils::GetString(element, "defaultplayer", m_videoDefaultPlayer);
    XMLUtils::GetBoolean(element, "fullscreenonmoviestart", m_fullScreenOnMovieStart);
    // 101 on purpose - can be used to never automark as watched
    XMLUtils::GetFloat(element, "playcountminimumpercent", m_videoPlayCountMinimumPercent, 0.0f,
                       101.0f);
    XMLUtils::GetInt(element, "ignoresecondsatstart", m_videoIgnoreSecondsAtStart, 0, 900);
    XMLUtils::GetFloat(element, "ignorepercentatend", m_videoIgnorePercentAtEnd, 0, 100.0f);

    XMLUtils::GetBoolean(element, "usetimeseeking", m_videoUseTimeSeeking);
    XMLUtils::GetInt(element, "timeseekforward", m_videoTimeSeekForward, 0, 6000);
    XMLUtils::GetInt(element, "timeseekbackward", m_videoTimeSeekBackward, -6000, 0);
    XMLUtils::GetInt(element, "timeseekforwardbig", m_videoTimeSeekForwardBig, 0, 6000);
    XMLUtils::GetInt(element, "timeseekbackwardbig", m_videoTimeSeekBackwardBig, -6000, 0);

    XMLUtils::GetInt(element, "percentseekforward", m_videoPercentSeekForward, 0, 100);
    XMLUtils::GetInt(element, "percentseekbackward", m_videoPercentSeekBackward, -100, 0);
    XMLUtils::GetInt(element, "percentseekforwardbig", m_videoPercentSeekForwardBig, 0, 100);
    XMLUtils::GetInt(element, "percentseekbackwardbig", m_videoPercentSeekBackwardBig, -100, 0);

    auto* videoExcludesNode = element->FirstChildElement("excludefromlisting");
    if (videoExcludesNode)
      GetCustomRegexps(videoExcludesNode, m_videoExcludeFromListingRegExps);

    videoExcludesNode = element->FirstChildElement("excludefromscan");
    if (videoExcludesNode)
      GetCustomRegexps(videoExcludesNode, m_moviesExcludeFromScanRegExps);

    videoExcludesNode = element->FirstChildElement("excludetvshowsfromscan");
    if (videoExcludesNode)
      GetCustomRegexps(videoExcludesNode, m_tvshowExcludeFromScanRegExps);

    videoExcludesNode = element->FirstChildElement("cleanstrings");
    if (videoExcludesNode)
      GetCustomRegexps(videoExcludesNode, m_videoCleanStringRegExps);

    XMLUtils::GetString(element, "filenameidentifier", m_videoFilenameIdentifierRegExp);
    XMLUtils::GetString(element, "cleandatetime", m_videoCleanDateTimeRegExp);
    XMLUtils::GetString(element, "ppffmpegpostprocessing", m_videoPPFFmpegPostProc);
    XMLUtils::GetInt(element, "vdpauscaling", m_videoVDPAUScaling);
    XMLUtils::GetFloat(element, "nonlinearstretchratio", m_videoNonLinStretchRatio, 0.01f, 1.0f);
    XMLUtils::GetFloat(element, "autoscalemaxfps", m_videoAutoScaleMaxFps, 0.0f, 1000.0f);
    XMLUtils::GetInt(element, "useocclusionquery", m_videoCaptureUseOcclusionQuery, -1, 1);
    XMLUtils::GetBoolean(element, "vdpauInvTelecine", m_videoVDPAUtelecine);
    XMLUtils::GetBoolean(element, "vdpauHDdeintSkipChroma", m_videoVDPAUdeintSkipChromaHD);

    auto* adjustRefreshrateNode = element->FirstChildElement("adjustrefreshrate");
    if (adjustRefreshrateNode)
    {
      auto* refreshOverrideNode = adjustRefreshrateNode->FirstChildElement("override");
      while (refreshOverrideNode)
      {
        RefreshOverride override = {};

        float fps;
        if (XMLUtils::GetFloat(refreshOverrideNode, "fps", fps))
        {
          override.fpsmin = fps - 0.01f;
          override.fpsmax = fps + 0.01f;
        }

        float fpsmin, fpsmax;
        if (XMLUtils::GetFloat(refreshOverrideNode, "fpsmin", fpsmin) &&
            XMLUtils::GetFloat(refreshOverrideNode, "fpsmax", fpsmax))
        {
          override.fpsmin = fpsmin;
          override.fpsmax = fpsmax;
        }

        float refresh;
        if (XMLUtils::GetFloat(refreshOverrideNode, "refresh", refresh))
        {
          override.refreshmin = refresh - 0.01f;
          override.refreshmax = refresh + 0.01f;
        }

        float refreshmin, refreshmax;
        if (XMLUtils::GetFloat(refreshOverrideNode, "refreshmin", refreshmin) &&
            XMLUtils::GetFloat(refreshOverrideNode, "refreshmax", refreshmax))
        {
          override.refreshmin = refreshmin;
          override.refreshmax = refreshmax;
        }

        bool fpsCorrect     = (override.fpsmin > 0.0f && override.fpsmax >= override.fpsmin);
        bool refreshCorrect = (override.refreshmin > 0.0f && override.refreshmax >= override.refreshmin);

        if (fpsCorrect && refreshCorrect)
          m_videoAdjustRefreshOverrides.push_back(override);
        else
          CLog::Log(LOGWARNING,
                    "Ignoring malformed refreshrate override, fpsmin:{:f} fpsmax:{:f} "
                    "refreshmin:{:f} refreshmax:{:f}",
                    override.fpsmin, override.fpsmax, override.refreshmin, override.refreshmax);

        refreshOverrideNode = refreshOverrideNode->NextSiblingElement("override");
      }

      auto* refreshFallbackNode = adjustRefreshrateNode->FirstChildElement("fallback");
      while (refreshFallbackNode)
      {
        RefreshOverride fallback = {};
        fallback.fallback = true;

        float refresh;
        if (XMLUtils::GetFloat(refreshFallbackNode, "refresh", refresh))
        {
          fallback.refreshmin = refresh - 0.01f;
          fallback.refreshmax = refresh + 0.01f;
        }

        float refreshmin, refreshmax;
        if (XMLUtils::GetFloat(refreshFallbackNode, "refreshmin", refreshmin) &&
            XMLUtils::GetFloat(refreshFallbackNode, "refreshmax", refreshmax))
        {
          fallback.refreshmin = refreshmin;
          fallback.refreshmax = refreshmax;
        }

        if (fallback.refreshmin > 0.0f && fallback.refreshmax >= fallback.refreshmin)
          m_videoAdjustRefreshOverrides.push_back(fallback);
        else
          CLog::Log(LOGWARNING,
                    "Ignoring malformed refreshrate fallback, fpsmin:{:f} fpsmax:{:f} "
                    "refreshmin:{:f} refreshmax:{:f}",
                    fallback.fpsmin, fallback.fpsmax, fallback.refreshmin, fallback.refreshmax);

        refreshFallbackNode = refreshFallbackNode->NextSiblingElement("fallback");
      }
    }

    m_DXVACheckCompatibilityPresent =
        XMLUtils::GetBoolean(element, "checkdxvacompatibility", m_DXVACheckCompatibility);

    //0 = disable fps detect, 1 = only detect on timestamps with uniform spacing, 2 detect on all timestamps
    XMLUtils::GetInt(element, "fpsdetect", m_videoFpsDetect, 0, 2);
    XMLUtils::GetFloat(element, "maxtempo", m_maxTempo, 1.5, 2.1);
    XMLUtils::GetBoolean(element, "preferstereostream", m_videoPreferStereoStream);

    // Store global display latency settings
    auto* videoLatencyNode = element->FirstChildElement("latency");
    if (videoLatencyNode)
    {
      float refresh, refreshmin, refreshmax;
      auto* refreshVideoLatencyNode = videoLatencyNode->FirstChildElement("refresh");

      while (refreshVideoLatencyNode)
      {
        RefreshVideoLatency videolatency = {};

        if (XMLUtils::GetFloat(refreshVideoLatencyNode, "rate", refresh))
        {
          videolatency.refreshmin = refresh - 0.01f;
          videolatency.refreshmax = refresh + 0.01f;
        }
        else if (XMLUtils::GetFloat(refreshVideoLatencyNode, "min", refreshmin) &&
                 XMLUtils::GetFloat(refreshVideoLatencyNode, "max", refreshmax))
        {
          videolatency.refreshmin = refreshmin;
          videolatency.refreshmax = refreshmax;
        }
        XMLUtils::GetFloat(refreshVideoLatencyNode, "delay", videolatency.delay, -600.0f, 600.0f);
        XMLUtils::GetFloat(refreshVideoLatencyNode, "hdrextradelay", videolatency.hdrextradelay,
                           -600.0f, 600.0f);

        if (videolatency.refreshmin > 0.0f && videolatency.refreshmax >= videolatency.refreshmin)
          m_videoRefreshLatency.push_back(videolatency);
        else
          CLog::Log(LOGWARNING,
                    "Ignoring malformed display latency <refresh> entry, min:{:f} max:{:f}",
                    videolatency.refreshmin, videolatency.refreshmax);

        refreshVideoLatencyNode = refreshVideoLatencyNode->NextSiblingElement("refresh");
      }

      // Get default global display latency values
      XMLUtils::GetFloat(videoLatencyNode, "delay", m_videoDefaultLatency, -600.0f, 600.0f);
      XMLUtils::GetFloat(videoLatencyNode, "hdrextradelay", m_videoDefaultHdrExtraLatency, -600.0f,
                         600.0f);
    }
  }

  element = rootElement->FirstChildElement("musiclibrary");
  if (element)
  {
    XMLUtils::GetInt(element, "recentlyaddeditems", m_iMusicLibraryRecentlyAddedItems, 1, INT_MAX);
    XMLUtils::GetBoolean(element, "prioritiseapetags", m_prioritiseAPEv2tags);
    XMLUtils::GetBoolean(element, "allitemsonbottom", m_bMusicLibraryAllItemsOnBottom);
    XMLUtils::GetBoolean(element, "cleanonupdate", m_bMusicLibraryCleanOnUpdate);
    XMLUtils::GetBoolean(element, "artistsortonupdate", m_bMusicLibraryArtistSortOnUpdate);
    XMLUtils::GetString(element, "albumformat", m_strMusicLibraryAlbumFormat);
    XMLUtils::GetString(element, "itemseparator", m_musicItemSeparator);
    XMLUtils::GetInt(element, "dateadded", m_iMusicLibraryDateAdded);
    XMLUtils::GetBoolean(element, "useisodates", m_bMusicLibraryUseISODates);
    XMLUtils::GetBoolean(element, "artistnavigatestosongs", m_bMusicLibraryArtistNavigatesToSongs);
    //Music artist name separators
    auto* separators = element->FirstChildElement("artistseparators");
    if (separators)
    {
      m_musicArtistSeparators.clear();
      auto* separator = separators->FirstChildElement("separator");
      while (separator)
      {
        if (separator->FirstChild())
          m_musicArtistSeparators.push_back(separator->FirstChild()->Value());
        separator = separator->NextSiblingElement("separator");
      }
    }
  }

  element = rootElement->FirstChildElement("videolibrary");
  if (element)
  {
    XMLUtils::GetBoolean(element, "allitemsonbottom", m_bVideoLibraryAllItemsOnBottom);
    XMLUtils::GetInt(element, "recentlyaddeditems", m_iVideoLibraryRecentlyAddedItems, 1, INT_MAX);
    XMLUtils::GetBoolean(element, "cleanonupdate", m_bVideoLibraryCleanOnUpdate);
    XMLUtils::GetBoolean(element, "usefasthash", m_bVideoLibraryUseFastHash);
    XMLUtils::GetString(element, "itemseparator", m_videoItemSeparator);
    XMLUtils::GetBoolean(element, "importwatchedstate", m_bVideoLibraryImportWatchedState);
    XMLUtils::GetBoolean(element, "importresumepoint", m_bVideoLibraryImportResumePoint);
    XMLUtils::GetInt(element, "dateadded", m_iVideoLibraryDateAdded);
  }

  element = rootElement->FirstChildElement("videoscanner");
  if (element)
  {
    XMLUtils::GetBoolean(element, "ignoreerrors", m_bVideoScannerIgnoreErrors);
  }

  // Backward-compatibility of ExternalPlayer config
  element = rootElement->FirstChildElement("externalplayer");
  if (element)
  {
    CLog::Log(LOGWARNING, "External player configuration has been removed from advancedsettings.xml.  It can now be configured in userdata/playercorefactory.xml");
  }
  element = rootElement->FirstChildElement("slideshow");
  if (element)
  {
    XMLUtils::GetFloat(element, "panamount", m_slideshowPanAmount, 0.0f, 20.0f);
    XMLUtils::GetFloat(element, "zoomamount", m_slideshowZoomAmount, 0.0f, 20.0f);
    XMLUtils::GetFloat(element, "blackbarcompensation", m_slideshowBlackBarCompensation, 0.0f,
                       50.0f);
  }

  element = rootElement->FirstChildElement("network");
  if (element)
  {
    XMLUtils::GetInt(element, "curlclienttimeout", m_curlconnecttimeout, 1, 1000);
    XMLUtils::GetInt(element, "curllowspeedtime", m_curllowspeedtime, 1, 1000);
    XMLUtils::GetInt(element, "curlretries", m_curlretries, 0, 10);
    XMLUtils::GetInt(element, "curlkeepaliveinterval", m_curlKeepAliveInterval, 0, 300);
    XMLUtils::GetBoolean(element, "disableipv6", m_curlDisableIPV6);
    XMLUtils::GetBoolean(element, "disablehttp2", m_curlDisableHTTP2);
    XMLUtils::GetString(element, "catrustfile", m_caTrustFile);
  }

  element = rootElement->FirstChildElement("jsonrpc");
  if (element)
  {
    XMLUtils::GetBoolean(element, "compactoutput", m_jsonOutputCompact);
    XMLUtils::GetUInt(element, "tcpport", m_jsonTcpPort);
  }

  element = rootElement->FirstChildElement("samba");
  if (element)
  {
    XMLUtils::GetString(element, "doscodepage", m_sambadoscodepage);
    XMLUtils::GetInt(element, "clienttimeout", m_sambaclienttimeout, 5, 100);
    XMLUtils::GetBoolean(element, "statfiles", m_sambastatfiles);
  }

  element = rootElement->FirstChildElement("httpdirectory");
  if (element)
    XMLUtils::GetBoolean(element, "statfilesize", m_bHTTPDirectoryStatFilesize);

  element = rootElement->FirstChildElement("ftp");
  if (element)
  {
    XMLUtils::GetBoolean(element, "remotethumbs", m_bFTPThumbs);
  }

  element = rootElement->FirstChildElement("loglevel");
  if (element)
  { // read the loglevel setting, so set the setting advanced to hide it in GUI
    // as altering it will do nothing - we don't write to advancedsettings.xml
    XMLUtils::GetInt(rootElement, "loglevel", m_logLevelHint, LOG_LEVEL_NONE, LOG_LEVEL_MAX);
    const char* hide = element->Attribute("hide");
    if (hide == NULL || StringUtils::CompareNoCase("false", hide, 5) != 0)
    {
      SettingPtr setting = CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_DEBUG_SHOWLOGINFO);
      if (setting != NULL)
        setting->SetVisible(false);
    }
    m_logLevel = std::max(m_logLevel, m_logLevelHint);
    CServiceBroker::GetLogging().SetLogLevel(m_logLevel);
  }

  XMLUtils::GetString(rootElement, "cddbaddress", m_cddbAddress);
  XMLUtils::GetBoolean(rootElement, "addsourceontop", m_addSourceOnTop);

  //airtunes + airplay
  XMLUtils::GetInt(rootElement, "airtunesport", m_airTunesPort);
  XMLUtils::GetInt(rootElement, "airplayport", m_airPlayPort);

  XMLUtils::GetBoolean(rootElement, "handlemounting", m_handleMounting);
  XMLUtils::GetBoolean(rootElement, "automountopticalmedia", m_autoMountOpticalMedia);

#if defined(TARGET_WINDOWS_DESKTOP)
  XMLUtils::GetBoolean(rootElement, "minimizetotray", m_minimizeToTray);
#endif
#if defined(TARGET_DARWIN_OSX) || defined(TARGET_WINDOWS)
  XMLUtils::GetBoolean(rootElement, "fullscreen", m_startFullScreen);
#endif
  XMLUtils::GetBoolean(rootElement, "splash", m_splashImage);
  XMLUtils::GetBoolean(rootElement, "showexitbutton", m_showExitButton);
  XMLUtils::GetBoolean(rootElement, "canwindowed", m_canWindowed);

  XMLUtils::GetInt(rootElement, "songinfoduration", m_songInfoDuration, 0, INT_MAX);
  XMLUtils::GetInt(rootElement, "playlistretries", m_playlistRetries, -1, 5000);
  XMLUtils::GetInt(rootElement, "playlisttimeout", m_playlistTimeout, 0, 5000);

  XMLUtils::GetBoolean(rootElement, "glrectanglehack", m_GLRectangleHack);
  XMLUtils::GetInt(rootElement, "skiploopfilter", m_iSkipLoopFilter, -16, 48);

  XMLUtils::GetBoolean(rootElement, "virtualshares", m_bVirtualShares);
  XMLUtils::GetUInt(rootElement, "packagefoldersize", m_addonPackageFolderSize);

  // EPG
  element = rootElement->FirstChildElement("epg");
  if (element)
  {
    XMLUtils::GetInt(element, "updatecheckinterval", m_iEpgUpdateCheckInterval);
    XMLUtils::GetInt(element, "cleanupinterval", m_iEpgCleanupInterval);
    XMLUtils::GetInt(element, "activetagcheckinterval", m_iEpgActiveTagCheckInterval);
    XMLUtils::GetInt(element, "retryinterruptedupdateinterval",
                     m_iEpgRetryInterruptedUpdateInterval);
    XMLUtils::GetInt(element, "updateemptytagsinterval", m_iEpgUpdateEmptyTagsInterval);
    XMLUtils::GetBoolean(element, "displayupdatepopup", m_bEpgDisplayUpdatePopup);
    XMLUtils::GetBoolean(element, "displayincrementalupdatepopup",
                         m_bEpgDisplayIncrementalUpdatePopup);
  }

  // EDL commercial break handling
  element = rootElement->FirstChildElement("edl");
  if (element)
  {
    XMLUtils::GetBoolean(element, "mergeshortcommbreaks", m_bEdlMergeShortCommBreaks);
    XMLUtils::GetBoolean(element, "displaycommbreaknotifications",
                         m_EdlDisplayCommbreakNotifications);
    XMLUtils::GetInt(element, "maxcommbreaklength", m_iEdlMaxCommBreakLength, 0,
                     10 * 60); // Between 0 and 10 minutes
    XMLUtils::GetInt(element, "mincommbreaklength", m_iEdlMinCommBreakLength, 0,
                     5 * 60); // Between 0 and 5 minutes
    XMLUtils::GetInt(element, "maxcommbreakgap", m_iEdlMaxCommBreakGap, 0,
                     5 * 60); // Between 0 and 5 minutes.
    XMLUtils::GetInt(element, "maxstartgap", m_iEdlMaxStartGap, 0,
                     10 * 60); // Between 0 and 10 minutes
    XMLUtils::GetInt(element, "commbreakautowait", m_iEdlCommBreakAutowait, -60,
                     60); // Between -60 and 60 seconds
    XMLUtils::GetInt(element, "commbreakautowind", m_iEdlCommBreakAutowind, -60,
                     60); // Between -60 and 60 seconds
  }

  // picture exclude regexps
  auto* pictureExcludesNode = rootElement->FirstChildElement("pictureexcludes");
  if (pictureExcludesNode)
    GetCustomRegexps(pictureExcludesNode, m_pictureExcludeFromListingRegExps);

  // picture extensions
  auto* extsElement = rootElement->FirstChildElement("pictureextensions");
  if (extsElement)
    GetCustomExtensions(extsElement, m_pictureExtensions);

  // music extensions
  extsElement = rootElement->FirstChildElement("musicextensions");
  if (extsElement)
    GetCustomExtensions(extsElement, m_musicExtensions);

  // video extensions
  extsElement = rootElement->FirstChildElement("videoextensions");
  if (extsElement)
    GetCustomExtensions(extsElement, m_videoExtensions);

  // stub extensions
  extsElement = rootElement->FirstChildElement("discstubextensions");
  if (extsElement)
    GetCustomExtensions(extsElement, m_discStubExtensions);

  m_vecTokens.clear();
  CLangInfo::LoadTokens(rootElement->FirstChildElement("sorttokens"), m_vecTokens);

  //! @todo Should cache path be given in terms of our predefined paths??
  //! Are we even going to have predefined paths??
  std::string tmp;
  if (XMLUtils::GetPath(rootElement, "cachepath", tmp))
    m_cachePath = tmp;
  URIUtils::AddSlashAtEnd(m_cachePath);

  g_LangCodeExpander.LoadUserCodes(rootElement->FirstChildElement("languagecodes"));

  // trailer matching regexps
  auto* trailerMatchingElement = rootElement->FirstChildElement("trailermatching");
  if (trailerMatchingElement)
    GetCustomRegexps(trailerMatchingElement, m_trailerMatchRegExps);

  //everything that's a trailer is not a movie
  m_moviesExcludeFromScanRegExps.insert(m_moviesExcludeFromScanRegExps.end(),
                                        m_trailerMatchRegExps.begin(),
                                        m_trailerMatchRegExps.end());

  // video stacking regexps
  auto* videoStackingElement = rootElement->FirstChildElement("moviestacking");
  if (videoStackingElement)
    GetCustomRegexps(videoStackingElement, m_videoStackRegExps);

  // folder stacking regexps
  auto* folderStackingElement = rootElement->FirstChildElement("folderstacking");
  if (folderStackingElement)
    GetCustomRegexps(folderStackingElement, m_folderStackRegExps);

  //tv stacking regexps
  auto* TVStackingElement = rootElement->FirstChildElement("tvshowmatching");
  if (TVStackingElement)
    GetCustomTVRegexps(TVStackingElement, m_tvshowEnumRegExps);

  //tv multipart enumeration regexp
  XMLUtils::GetString(rootElement, "tvmultipartmatching", m_tvshowMultiPartEnumRegExp);

  // path substitutions
  auto* pathSubstitutionElement = rootElement->FirstChildElement("pathsubstitution");
  if (pathSubstitutionElement)
  {
    m_pathSubstitutions.clear();
    CLog::Log(LOGDEBUG,"Configuring path substitutions");
    auto* substituteNode = pathSubstitutionElement->FirstChildElement("substitute");
    while (substituteNode)
    {
      std::string strFrom, strTo;
      auto* fromNode = substituteNode->FirstChildElement("from");
      if (fromNode && !fromNode->NoChildren())
        strFrom = CSpecialProtocol::TranslatePath(fromNode->FirstChild()->Value()).c_str();
      auto* toNode = substituteNode->FirstChildElement("to");
      if (toNode && !toNode->NoChildren())
        strTo = toNode->FirstChild()->Value();

      if (!strFrom.empty() && !strTo.empty())
      {
        CLog::Log(LOGDEBUG,"  Registering substitution pair:");
        CLog::Log(LOGDEBUG, "    From: [{}]", CURL::GetRedacted(strFrom));
        CLog::Log(LOGDEBUG, "    To:   [{}]", CURL::GetRedacted(strTo));
        m_pathSubstitutions.emplace_back(strFrom, strTo);
      }
      else
      {
        // error message about missing tag
        if (strFrom.empty())
          CLog::Log(LOGERROR,"  Missing <from> tag");
        else
          CLog::Log(LOGERROR,"  Missing <to> tag");
      }

      // get next one
      substituteNode = substituteNode->NextSiblingElement("substitute");
    }
  }

  XMLUtils::GetInt(rootElement, "remotedelay", m_remoteDelay, 0, 20);
  XMLUtils::GetBoolean(rootElement, "scanirserver", m_bScanIRServer);

  XMLUtils::GetUInt(rootElement, "fanartres", m_fanartRes, 0, 9999);
  XMLUtils::GetUInt(rootElement, "imageres", m_imageRes, 0, 9999);
  if (XMLUtils::GetString(rootElement, "imagescalingalgorithm", tmp))
    m_imageScalingAlgorithm = CPictureScalingAlgorithm::FromString(tmp);
  XMLUtils::GetUInt(rootElement, "imagequalityjpeg", m_imageQualityJpeg, 0, 21);
  XMLUtils::GetBoolean(rootElement, "playlistasfolders", m_playlistAsFolders);
  XMLUtils::GetBoolean(rootElement, "uselocalecollation", m_useLocaleCollation);
  XMLUtils::GetBoolean(rootElement, "detectasudf", m_detectAsUdf);

  // music thumbs
  auto* thumbsNode = rootElement->FirstChildElement("musicthumbs");
  if (thumbsNode)
    GetCustomExtensions(thumbsNode, m_musicThumbs);

  // show art for shoutcast v2 streams (set to false for devices with limited storage)
  XMLUtils::GetBoolean(rootElement, "shoutcastart", m_bShoutcastArt);
  // music filename->tag filters
  auto* filters = rootElement->FirstChildElement("musicfilenamefilters");
  if (filters)
  {
    auto* filter = filters->FirstChildElement("filter");
    while (filter)
    {
      if (filter->FirstChild())
        m_musicTagsFromFileFilters.push_back(filter->FirstChild()->Value());
      filter = filter->NextSiblingElement("filter");
    }
  }

  auto* hostEntries = rootElement->FirstChildElement("hosts");
  if (hostEntries)
  {
    auto* element = hostEntries->FirstChildElement("entry");
    while(element)
    {
      if(!element->NoChildren())
      {
        std::string name  = XMLUtils::GetAttribute(element, "name");
        std::string value = element->FirstChild()->Value();
        if (!name.empty())
          CDNSNameCache::Add(name, value);
      }
      element = element->NextSiblingElement("entry");
    }
  }

  XMLUtils::GetString(rootElement, "cputempcommand", m_cpuTempCmd);
  XMLUtils::GetString(rootElement, "gputempcommand", m_gpuTempCmd);

  XMLUtils::GetBoolean(rootElement, "alwaysontop", m_alwaysOnTop);

  auto* PVRElement = rootElement->FirstChildElement("pvr");
  if (PVRElement)
  {
    XMLUtils::GetInt(PVRElement, "timecorrection", m_iPVRTimeCorrection, 0, 1440);
    XMLUtils::GetInt(PVRElement, "infotoggleinterval", m_iPVRInfoToggleInterval, 0, 30000);
    XMLUtils::GetBoolean(PVRElement, "channeliconsautoscan", m_bPVRChannelIconsAutoScan);
    XMLUtils::GetBoolean(PVRElement, "autoscaniconsuserset", m_bPVRAutoScanIconsUserSet);
    XMLUtils::GetInt(PVRElement, "numericchannelswitchtimeout", m_iPVRNumericChannelSwitchTimeout,
                     50, 60000);
    XMLUtils::GetInt(PVRElement, "timeshiftthreshold", m_iPVRTimeshiftThreshold, 0, 60);
    XMLUtils::GetBoolean(PVRElement, "timeshiftsimpleosd", m_bPVRTimeshiftSimpleOSD);
    auto* sortDecription = PVRElement->FirstChildElement("pvrrecordings");
    if (sortDecription)
    {
      const char* XML_SORTMETHOD = "sortmethod";
      const char* XML_SORTORDER = "sortorder";
      int sortMethod;
      // ignore SortByTime for duration defaults
      if (XMLUtils::GetInt(sortDecription, XML_SORTMETHOD, sortMethod, SortByLabel, SortByFile))
      {
        int sortOrder;
        if (XMLUtils::GetInt(sortDecription, XML_SORTORDER, sortOrder, SortOrderAscending,
                             SortOrderDescending))
        {
          m_PVRDefaultSortOrder.sortBy = (SortBy)sortMethod;
          m_PVRDefaultSortOrder.sortOrder = (SortOrder)sortOrder;
        }
      }
    }
  }

  auto* database = rootElement->FirstChildElement("videodatabase");
  if (database)
  {
    CLog::Log(LOGWARNING, "VIDEO database configuration is experimental.");
    XMLUtils::GetString(database, "type", m_databaseVideo.type);
    XMLUtils::GetString(database, "host", m_databaseVideo.host);
    XMLUtils::GetString(database, "port", m_databaseVideo.port);
    XMLUtils::GetString(database, "user", m_databaseVideo.user);
    XMLUtils::GetString(database, "pass", m_databaseVideo.pass);
    XMLUtils::GetString(database, "name", m_databaseVideo.name);
    XMLUtils::GetString(database, "key", m_databaseVideo.key);
    XMLUtils::GetString(database, "cert", m_databaseVideo.cert);
    XMLUtils::GetString(database, "ca", m_databaseVideo.ca);
    XMLUtils::GetString(database, "capath", m_databaseVideo.capath);
    XMLUtils::GetString(database, "ciphers", m_databaseVideo.ciphers);
    XMLUtils::GetBoolean(database, "compression", m_databaseVideo.compression);
  }

  database = rootElement->FirstChildElement("musicdatabase");
  if (database)
  {
    XMLUtils::GetString(database, "type", m_databaseMusic.type);
    XMLUtils::GetString(database, "host", m_databaseMusic.host);
    XMLUtils::GetString(database, "port", m_databaseMusic.port);
    XMLUtils::GetString(database, "user", m_databaseMusic.user);
    XMLUtils::GetString(database, "pass", m_databaseMusic.pass);
    XMLUtils::GetString(database, "name", m_databaseMusic.name);
    XMLUtils::GetString(database, "key", m_databaseMusic.key);
    XMLUtils::GetString(database, "cert", m_databaseMusic.cert);
    XMLUtils::GetString(database, "ca", m_databaseMusic.ca);
    XMLUtils::GetString(database, "capath", m_databaseMusic.capath);
    XMLUtils::GetString(database, "ciphers", m_databaseMusic.ciphers);
    XMLUtils::GetBoolean(database, "compression", m_databaseMusic.compression);
  }

  database = rootElement->FirstChildElement("tvdatabase");
  if (database)
  {
    XMLUtils::GetString(database, "type", m_databaseTV.type);
    XMLUtils::GetString(database, "host", m_databaseTV.host);
    XMLUtils::GetString(database, "port", m_databaseTV.port);
    XMLUtils::GetString(database, "user", m_databaseTV.user);
    XMLUtils::GetString(database, "pass", m_databaseTV.pass);
    XMLUtils::GetString(database, "name", m_databaseTV.name);
    XMLUtils::GetString(database, "key", m_databaseTV.key);
    XMLUtils::GetString(database, "cert", m_databaseTV.cert);
    XMLUtils::GetString(database, "ca", m_databaseTV.ca);
    XMLUtils::GetString(database, "capath", m_databaseTV.capath);
    XMLUtils::GetString(database, "ciphers", m_databaseTV.ciphers);
    XMLUtils::GetBoolean(database, "compression", m_databaseTV.compression);
  }

  database = rootElement->FirstChildElement("epgdatabase");
  if (database)
  {
    XMLUtils::GetString(database, "type", m_databaseEpg.type);
    XMLUtils::GetString(database, "host", m_databaseEpg.host);
    XMLUtils::GetString(database, "port", m_databaseEpg.port);
    XMLUtils::GetString(database, "user", m_databaseEpg.user);
    XMLUtils::GetString(database, "pass", m_databaseEpg.pass);
    XMLUtils::GetString(database, "name", m_databaseEpg.name);
    XMLUtils::GetString(database, "key", m_databaseEpg.key);
    XMLUtils::GetString(database, "cert", m_databaseEpg.cert);
    XMLUtils::GetString(database, "ca", m_databaseEpg.ca);
    XMLUtils::GetString(database, "capath", m_databaseEpg.capath);
    XMLUtils::GetString(database, "ciphers", m_databaseEpg.ciphers);
    XMLUtils::GetBoolean(database, "compression", m_databaseEpg.compression);
  }

  element = rootElement->FirstChildElement("enablemultimediakeys");
  if (element)
  {
    XMLUtils::GetBoolean(rootElement, "enablemultimediakeys", m_enableMultimediaKeys);
  }

  element = rootElement->FirstChildElement("gui");
  if (element)
  {
    XMLUtils::GetBoolean(element, "visualizedirtyregions", m_guiVisualizeDirtyRegions);
    XMLUtils::GetInt(element, "algorithmdirtyregions", m_guiAlgorithmDirtyRegions);
    XMLUtils::GetBoolean(element, "smartredraw", m_guiSmartRedraw);
    XMLUtils::GetInt(element, "anisotropicfiltering", m_guiAnisotropicFiltering);
    XMLUtils::GetBoolean(element, "fronttobackrendering", m_guiFrontToBackRendering);
    XMLUtils::GetBoolean(element, "geometryclear", m_guiGeometryClear);
    XMLUtils::GetBoolean(element, "asynctextureupload", m_guiAsyncTextureUpload);
  }

  std::string seekSteps;
  XMLUtils::GetString(rootElement, "seeksteps", seekSteps);
  if (!seekSteps.empty())
  {
    m_seekSteps.clear();
    std::vector<std::string> steps = StringUtils::Split(seekSteps, ',');
    for(std::vector<std::string>::iterator it = steps.begin(); it != steps.end(); ++it)
      m_seekSteps.push_back(atoi((*it).c_str()));
  }

  XMLUtils::GetBoolean(rootElement, "opengldebugging", m_openGlDebugging);

  // load in the settings overrides
  CServiceBroker::GetSettingsComponent()->GetSettings()->LoadHidden(rootElement);
}

void CAdvancedSettings::Clear()
{
  m_videoCleanStringRegExps.clear();
  m_moviesExcludeFromScanRegExps.clear();
  m_tvshowExcludeFromScanRegExps.clear();
  m_videoExcludeFromListingRegExps.clear();
  m_videoStackRegExps.clear();
  m_folderStackRegExps.clear();
  m_allExcludeFromScanRegExps.clear();
  m_audioExcludeFromScanRegExps.clear();
  m_audioExcludeFromListingRegExps.clear();
  m_pictureExcludeFromListingRegExps.clear();

  m_pictureExtensions.clear();
  m_musicExtensions.clear();
  m_videoExtensions.clear();
  m_discStubExtensions.clear();

  m_userAgent.clear();
}

void CAdvancedSettings::GetCustomTVRegexps(tinyxml2::XMLElement* rootElement,
                                           SETTINGS_TVSHOWLIST& settings)
{
  auto* element = rootElement;
  while (element)
  {
    int iAction = 0; // overwrite
    // for backward compatibility
    const char* szAppend = element->Attribute("append");
    if ((szAppend && StringUtils::CompareNoCase(szAppend, "yes") == 0))
      iAction = 1;
    // action takes precedence if both attributes exist
    const char* szAction = element->Attribute("action");
    if (szAction)
    {
      iAction = 0; // overwrite
      if (StringUtils::CompareNoCase(szAction, "append") == 0)
        iAction = 1; // append
      else if (StringUtils::CompareNoCase(szAction, "prepend") == 0)
        iAction = 2; // prepend
    }
    if (iAction == 0)
      settings.clear();
    auto* regExpNode = element->FirstChildElement("regexp");
    int i = 0;
    while (regExpNode)
    {
      if (regExpNode->FirstChild())
      {
        bool bByDate = false;
        bool byTitle = false;
        int iDefaultSeason = 1;
        if (regExpNode->ToElement())
        {
          std::string byDate = XMLUtils::GetAttribute(regExpNode->ToElement(), "bydate");
          if (byDate == "true")
          {
            bByDate = true;
          }
          std::string byTitleAttr = XMLUtils::GetAttribute(regExpNode->ToElement(), "bytitle");
          byTitle = (byTitleAttr == "true");
          std::string defaultSeason =
              XMLUtils::GetAttribute(regExpNode->ToElement(), "defaultseason");
          if(!defaultSeason.empty())
          {
            iDefaultSeason = atoi(defaultSeason.c_str());
          }
        }
        std::string regExp = regExpNode->FirstChild()->Value();
        if (iAction == 2)
        {
          settings.insert(settings.begin() + i++, 1,
                          TVShowRegexp(bByDate, regExp, iDefaultSeason, byTitle));
        }
        else
        {
          settings.emplace_back(bByDate, regExp, iDefaultSeason, byTitle);
        }
      }
      regExpNode = regExpNode->NextSiblingElement("regexp");
    }

    element = element->NextSiblingElement(rootElement->Value());
  }
}

void CAdvancedSettings::GetCustomRegexps(tinyxml2::XMLElement* rootElement,
                                         std::vector<std::string>& settings)
{
  auto* element = rootElement;
  while (element)
  {
    int iAction = 0; // overwrite
    // for backward compatibility
    const char* szAppend = element->Attribute("append");
    if ((szAppend && StringUtils::CompareNoCase(szAppend, "yes") == 0))
      iAction = 1;
    // action takes precedence if both attributes exist
    const char* szAction = element->Attribute("action");
    if (szAction)
    {
      iAction = 0; // overwrite
      if (StringUtils::CompareNoCase(szAction, "append") == 0)
        iAction = 1; // append
      else if (StringUtils::CompareNoCase(szAction, "prepend") == 0)
        iAction = 2; // prepend
    }
    if (iAction == 0)
      settings.clear();
    auto* regExpNode = element->FirstChildElement("regexp");
    int i = 0;
    while (regExpNode)
    {
      if (regExpNode->FirstChild())
      {
        std::string regExp = regExpNode->FirstChild()->Value();
        if (iAction == 2)
          settings.insert(settings.begin() + i++, 1, regExp);
        else
          settings.push_back(regExp);
      }
      regExpNode = regExpNode->NextSiblingElement("regexp");
    }

    element = element->NextSiblingElement(rootElement->Value());
  }
}

void CAdvancedSettings::GetCustomExtensions(tinyxml2::XMLElement* rootElement,
                                            std::string& extensions)
{
  std::string extraExtensions;
  if (XMLUtils::GetString(rootElement, "add", extraExtensions) && !extraExtensions.empty())
    extensions += "|" + extraExtensions;
  if (XMLUtils::GetString(rootElement, "remove", extraExtensions) && !extraExtensions.empty())
  {
    std::vector<std::string> exts = StringUtils::Split(extraExtensions, '|');
    for (std::vector<std::string>::const_iterator i = exts.begin(); i != exts.end(); ++i)
    {
      size_t iPos = extensions.find(*i);
      if (iPos != std::string::npos)
        extensions.erase(iPos,i->size()+1);
    }
  }
}

void CAdvancedSettings::AddSettingsFile(const std::string &filename)
{
  m_settingsFiles.push_back(filename);
}

float CAdvancedSettings::GetLatencyTweak(float refreshrate, bool isHDREnabled)
{
  float delay{};
  const auto& latency =
      std::find_if(m_videoRefreshLatency.cbegin(), m_videoRefreshLatency.cend(),
                   [refreshrate](const auto& param)
                   { return refreshrate >= param.refreshmin && refreshrate <= param.refreshmax; });

  if (latency != m_videoRefreshLatency.cend()) //refresh rate specific setting is found
  {
    delay = latency->delay == 0.0f ? m_videoDefaultLatency : latency->delay;
    if (isHDREnabled)
      delay +=
          latency->hdrextradelay == 0.0f ? m_videoDefaultHdrExtraLatency : latency->hdrextradelay;
  }
  else //apply default delay settings
  {
    delay = isHDREnabled ? m_videoDefaultLatency + m_videoDefaultHdrExtraLatency
                         : m_videoDefaultLatency;
  }
  return delay; // in milliseconds
}

void CAdvancedSettings::SetDebugMode(bool debug)
{
  if (debug)
  {
    int level = std::max(m_logLevelHint, LOG_LEVEL_DEBUG_FREEMEM);
    m_logLevel = level;
    CServiceBroker::GetLogging().SetLogLevel(level);
    CLog::Log(LOGINFO, "Enabled debug logging due to GUI setting. Level {}.", level);
  }
  else
  {
    int level = std::min(m_logLevelHint, LOG_LEVEL_DEBUG/*LOG_LEVEL_NORMAL*/);
    CLog::Log(LOGINFO, "Disabled debug logging due to GUI setting. Level {}.", level);
    m_logLevel = level;
    CServiceBroker::GetLogging().SetLogLevel(level);
  }
}

void CAdvancedSettings::SetExtraArtwork(const tinyxml2::XMLElement* arttypes,
                                        std::vector<std::string>& artworkMap)
{
  if (!arttypes)
    return;
  artworkMap.clear();
  const auto* arttype = arttypes->FirstChildElement("arttype");
  while (arttype)
  {
    if (arttype->FirstChild())
      artworkMap.push_back(arttype->FirstChild()->Value());
    arttype = arttype->NextSiblingElement("arttype");
  }
}
