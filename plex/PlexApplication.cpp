/*
 *  PlexApplication.cpp
 *  XBMC
 *
 *  Created by Jamie Kirkpatrick on 20/01/2011.
 *  Copyright 2014 Plex Inc. All rights reserved.
 *
 */

#include "Client/PlexNetworkServiceBrowser.h"
#include "PlexApplication.h"
#include "GUIUserMessages.h"
#include "MediaSource.h"
#include "plex/Helper/PlexHTHelper.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "AdvancedSettings.h"
#include "plex/CrashReporter/CrashSubmitter.h"

#include "Client/PlexServerManager.h"
#include "Client/PlexServerDataLoader.h"
#include "Remote/PlexRemoteSubscriberManager.h"
#include "Client/PlexMediaServerClient.h"
#include "PlexApplication.h"
#include "interfaces/AnnouncementManager.h"
#include "PlexAnalytics.h"
#include "Client/PlexTimelineManager.h"
#include "PlexThemeMusicPlayer.h"
#include "VideoThumbLoader.h"
#include "PlexFilterManager.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "dialogs/GUIDialogVideoOSD.h"
#include "GUIWindowManager.h"
#include "Utility/PlexProfiler.h"
#include "Client/PlexTranscoderClient.h"

#include "network/UdpClient.h"
#include "DNSNameCache.h"

#include "Client/PlexExtraInfoLoader.h"
#include "Playlists/PlexPlayQueueManager.h"

#ifdef ENABLE_AUTOUPDATE
#include "AutoUpdate/PlexAutoUpdate.h"
#endif

#include "AudioEngine/AEFactory.h"

#include <sstream>

////////////////////////////////////////////////////////////////////////////////
void PlexApplication::Start()
{
  timer = CPlexGlobalTimerPtr(new CPlexGlobalTimer);

  dataLoader = CPlexServerDataLoaderPtr(new CPlexServerDataLoader);
  serverManager = CPlexServerManagerPtr(new CPlexServerManager);
  myPlexManager = new CMyPlexManager;
  remoteSubscriberManager = new CPlexRemoteSubscriberManager;
  mediaServerClient = CPlexMediaServerClientPtr(new CPlexMediaServerClient);
  analytics = new CPlexAnalytics;
  timelineManager = CPlexTimelineManagerPtr(new CPlexTimelineManager);
  themeMusicPlayer = CPlexThemeMusicPlayerPtr(new CPlexThemeMusicPlayer);
  thumbCacher = new CPlexThumbCacher;
  filterManager = CPlexFilterManagerPtr(new CPlexFilterManager);
  profiler = CPlexProfilerPtr(new CPlexProfiler);
  extraInfo = new CPlexExtraInfoLoader;
  playQueueManager = CPlexPlayQueueManagerPtr(new CPlexPlayQueueManager);

  serverManager->load();

  ANNOUNCEMENT::CAnnouncementManager::AddAnnouncer(this);

#ifdef ENABLE_AUTOUPDATE
  autoUpdater = new CPlexAutoUpdate;
#endif

  new CrashSubmitter;

  if (g_advancedSettings.m_bEnableGDM)
    m_serviceListener = CPlexServiceListenerPtr(new CPlexServiceListener);

  // Add the manual server if it exists and is enabled.
  if (g_guiSettings.GetBool("plexmediaserver.manualaddress"))
  {
    string address = g_guiSettings.GetString("plexmediaserver.address");
    if (PlexUtils::IsValidIP(address))
    {
      PlexServerList list;
      CPlexServerPtr server = CPlexServerPtr(new CPlexServer("", address, 32400));
      list.push_back(server);
      g_plexApplication.serverManager->UpdateFromConnectionType(list,
                                                                CPlexConnection::CONNECTION_MANUAL);
    }
  }

  if (g_guiSettings.GetBool("advanced.collectanalytics"))
    analytics->startLogging();

  myPlexManager->Create();
}

////////////////////////////////////////////////////////////////////////////////
bool PlexApplication::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_APP_ACTIVATED:
    case GUI_MSG_APP_DEACTIVATED:
    {
      CLog::Log(LOGDEBUG, "Plex Application: Handling message %d", message.GetMessage());
      return true;
    }
    case GUI_MSG_BG_MUSIC_SETTINGS_UPDATED:
    {
      return true;
    }
    case GUI_MSG_BG_MUSIC_THEME_UPDATED:
    {
      //      g_plexApplication.backgroundMusicPlayer->SetTheme(message.GetStringParam());
      return true;
    }
  }

  return false;
}

#ifdef TARGET_DARWIN_OSX
// Hack
class CRemoteRestartThread : public CThread
{
public:
  CRemoteRestartThread() : CThread("RemoteRestart")
  {
  }
  void Process()
  {
    // This blocks until the helper is restarted
    PlexHTHelper::GetInstance().Restart();
  }
};
#endif

////////////////////////////////////////////////////////////////////////////////
void PlexApplication::OnWakeUp()
{
  /* Scan servers */
  m_serviceListener->ScanNow();
  myPlexManager->Poke();

#ifdef TARGET_DARWIN_OSX
  CRemoteRestartThread* hack = new CRemoteRestartThread;
  hack->Create(true);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::FailAddToPacketRender()
{
  if (g_application.m_pPlayer->IsPassthrough() && !m_triedToRestart)
  {
    CLog::Log(LOGDEBUG,
              "CPlexApplication::FailAddToPacketRender Let's try to restart the media player");
    CApplicationMessenger::Get().MediaRestart(false);
    m_triedToRestart = true;
  }
}

////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::ForceVersionCheck()
{
#ifdef ENABLE_AUTOUPDATE
  autoUpdater->ForceVersionCheckInBackground();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::setNetworkLogging(bool onOff)
{
  if (!myPlexManager->IsSignedIn())
  {
    g_guiSettings.SetBool("debug.networklogging", false);
    return;
  }

  if (onOff && !m_networkLoggingOn)
  {
    if (!Create())
    {
      CLog::Log(LOGWARNING, "CPlexApplication::setNetworkLogging failed to enable UDPClient");
      g_guiSettings.SetBool("debug.networklogging", false);
      return;
    }

    if (!CDNSNameCache::Lookup("logs.papertrailapp.com", m_ipAddress))
    {
      CLog::Log(LOGWARNING, "CPlexApplication::setNetworkLogging failed to resolve papertrail");
      g_guiSettings.SetBool("debug.networklogging", false);
      return;
    }
    timer->SetTimeout(1200000, this);
    m_networkLoggingOn = true;

    CLog::Log(LOGINFO, "Plex Home Theater v%s (%s %s) @ %s", g_infoManager.GetVersion().c_str(),
              PlexUtils::GetMachinePlatform().c_str(),
              PlexUtils::GetMachinePlatformVersion().c_str(),
              myPlexManager->GetCurrentUserInfo().email.c_str());
  }
  else if (!onOff && m_networkLoggingOn)
  {
    Destroy();

    m_networkLoggingOn = false;
    timer->RemoveTimeout(this);

    CLog::Log(LOGWARNING, "CPlexApplication::setNetworkLogging stopped networkLogging");
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::OnTimeout()
{
  g_guiSettings.SetBool("debug.networklogging", false);
  m_networkLoggingOn = false;
  Destroy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::sendNetworkLog(int level, const std::string& logline)
{
  if (boost::contains(logline, "DEBUG: UDPCLIENT"))
    return;

  if (!m_networkLoggingOn)
    return;

  if (!myPlexManager->IsSignedIn())
    return;

  int priority = 16 * 8;

  switch (level)
  {
    case LOGSEVERE:
    case LOGFATAL:
    case LOGERROR:
      priority += 0;
    case LOGWARNING:
      priority += 4;
    case LOGNOTICE:
    case LOGINFO:
      priority += 6;
    case LOGDEBUG:
      priority += 7;
  }

  tm t;
  CDateTime::GetCurrentDateTime().GetAsTm(t);
  char time[128];
  strftime(time, 63, "%b %d %H:%M:%S", &t);

  std::stringstream s;
  s << "<" << priority << ">" + std::string(time) << " x "
    << "Plex Home Theater: ";
  s << "[" << myPlexManager->GetCurrentUserInfo().email << "] ";

  int strleft = 1024 - s.str().size();
  s << logline.substr(0, strleft);

  CStdString packet(s.str());
  Send(m_ipAddress, 60969, packet);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::preShutdown()
{
  analytics->stopLogging();
  remoteSubscriberManager->Stop();
  timer->StopAllTimers();
  themeMusicPlayer->stop();
  if (m_serviceListener)
  {
    m_serviceListener->Stop();
    m_serviceListener.reset();
  }
  myPlexManager->Stop();
  serverManager->Stop();
  dataLoader->Stop();
  timelineManager->Stop();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::Shutdown()
{
  CLog::Log(LOGINFO, "CPlexApplication shutting down!");

  delete extraInfo;

  delete myPlexManager;
  delete analytics;

  timer.reset();

  serverManager.reset();
  dataLoader.reset();

  timelineManager.reset();

  mediaServerClient->CancelJobs();
  mediaServerClient.reset();

  profiler->Clear();
  profiler.reset();

  filterManager->saveFiltersToDisk();
  filterManager.reset();

  CPlexTranscoderClient::DeleteInstance();

  OnTimeout();

  delete remoteSubscriberManager;
  remoteSubscriberManager = NULL;

#ifdef ENABLE_AUTOUPDATE
  delete autoUpdater;
#endif

  delete thumbCacher;
}

////////////////////////////////////////////////////////////////////////////////////////
void PlexApplication::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char* sender,
                               const char* message, const CVariant& data)
{
  if (flag == ANNOUNCEMENT::Player && stricmp(sender, "xbmc") == 0 && stricmp(message, "OnPlay") == 0)
  {
    m_triedToRestart = false;

    CGUIDialogVideoOSD* osd
        = (CGUIDialogVideoOSD*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD);
    if (g_application.IsPlayingVideo() && osd && osd->IsOpenedFromPause())
      CApplicationMessenger::Get().Close(osd, false);
  }
  else if (flag == ANNOUNCEMENT::Player && stricmp(sender, "xbmc") == 0
           && stricmp(message, "OnPause") == 0)
  {
    CGUIDialogVideoOSD* osd
        = (CGUIDialogVideoOSD*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD);
    if (g_application.IsPlayingVideo() && osd && !osd->IsActive())
      CApplicationMessenger::Get().DoModal(osd, WINDOW_DIALOG_VIDEO_OSD, "pauseOpen", false);
  }
}
