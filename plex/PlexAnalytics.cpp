#include "PlexAnalytics.h"

#include "URL.h"
#include "settings/GUISettings.h"
#include "Settings.h"
#include "threads/Timer.h"
#include "filesystem/CurlFile.h"
#include "FileSystem/PlexFile.h"
#include "Application.h"
#include "interfaces/AnnouncementManager.h"

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include "cores/VideoRenderers/RenderManager.h"
#include "log.h"
#include "PlexJobs.h"
#include "JobManager.h"
#include "GUIInfoManager.h"

#define ANALYTICS_TID_PHT "UA-6111912-18"

#define INITIAL_EVENT_DELAY   3
#define PING_INTERVAL_SECONDS 13500

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexAnalytics::CPlexAnalytics() : m_timer(this), m_firstEvent(true), m_numberOfPlays(0)
{

  ANNOUNCEMENT::CAnnouncementManager::AddAnnouncer(this);

  /*
  m_defaultArgs["v"] = "1";
  m_defaultArgs["tid"] = ANALYTICS_TID_PMS;
  m_defaultArgs["ul"] = "en-us";
  m_defaultArgs["an"] = "Plex Media Server";
  m_defaultArgs["av"] = Cocoa_GetAppVersion();
  m_defaultArgs["cid"] = GetMachineIdentifier();
  */
  m_baseOptions.AddOption("v", "1");
  m_baseOptions.AddOption("tid", ANALYTICS_TID_PHT);
  m_baseOptions.AddOption("ul", g_guiSettings.GetString("locale.language"));
  m_baseOptions.AddOption("an", "Plex Home Theater");
  m_baseOptions.AddOption("av", g_infoManager.GetVersion());
  m_baseOptions.AddOption("cid", g_guiSettings.GetString("system.uuid"));

  /* let's jump through some hoops to get some resolution info */
#if 0
  RESOLUTION res = g_renderManager.GetResolution();
  RESOLUTION_INFO resInfo = g_settings.m_ResInfo[res];
  CStdString resStr;
  resStr.Format("%dx%d", resInfo.iWidth, resInfo.iHeight);

  m_baseOptions.AddOption("sr", resStr);
#endif

  m_timer.Start(INITIAL_EVENT_DELAY, false);
  m_sessionLength.restart();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexAnalytics::didUpgradeEvent(bool success, const std::string &fromVersion, const std::string &toVersion, bool delta)
{
  CUrlOptions o;
  o.AddOption("cd9", (int)delta);
  o.AddOption("cd10", fromVersion);
  o.AddOption("cd11", toVersion);
  trackEvent("App", "Upgrade", success ? "success" : "failed", (int)success, o);
}

typedef std::pair<std::string, std::string> strPair;

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexAnalytics::setCustomDimensions(CUrlOptions &options)
{
  /*
    Custom Dimension 1 (cd1=): X-Plex-Product
    Custom Dimension 2 (cd2=): X-Plex-Client-Identifier
    Custom Dimension 3 (cd3=): X-Plex-Platform
    Custom Dimension 4 (cd4=): X-Plex-Platform-Version
    Custom Dimension 5 (cd5=): X-Plex-Device
    Custom Dimension 6 (cd6=): X-Plex-Version
   */

  std::vector<strPair> headers = XFILE::CPlexFile::GetHeaderList();
  strPair pair;

  BOOST_FOREACH(pair, headers)
  {
    if (pair.first == "X-Plex-Product")
      options.AddOption("cd1", pair.second);
    if (pair.first == "X-Plex-Client-Identifier")
      options.AddOption("cd2", pair.second);
    /* Instead of sending X-Plex-Platform in cd3, that is always set to
     * Plex Home Theater for PHT, we are going to send the OS since this
     * make much more sense for us to track.
     */
    if (pair.first == "X-Plex-Model")
      options.AddOption("cd3", pair.second);
    if (pair.first == "X-Plex-Platform-Version")
      options.AddOption("cd4", pair.second);
    if (pair.first == "X-Plex-Device")
      options.AddOption("cd5", pair.second);
    if (pair.first == "X-Plex-Version")
      options.AddOption("cd6", pair.second);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexAnalytics::trackEvent(const std::string &category, const std::string &action, const std::string &label, int64_t value, const CUrlOptions &args)
{
  CUrlOptions opts(m_baseOptions);
  opts.AddOptions(args);

  opts.AddOption("t", "event");

  if (!category.empty())
    opts.AddOption("ec", category);
  if(!action.empty())
    opts.AddOption("ea", action);
  if (!label.empty())
    opts.AddOption("el", label);

  opts.AddOption("ev", boost::lexical_cast<std::string>(value));

  setCustomDimensions(opts);
  sendTrackingRequest(opts);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexAnalytics::sendPing()
{
  trackEvent("App", "Ping", "", 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexAnalytics::sendTrackingRequest(const CUrlOptions &request)
{
  CURL u("http://www.google-analytics.com/collect");
  CPlexMediaServerClientJob *j = new CPlexMediaServerClientJob(u, "POST");
  j->m_postData = request.GetOptionsString();

  CLog::Log(LOGDEBUG, "CPlexAnalytics::sendTrackingRequest sending %s", j->m_postData.c_str());

#ifndef _DEBUG //don't send analytics for test builds
  CJobManager::GetInstance().AddJob(j, NULL, CJob::PRIORITY_LOW);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexAnalytics::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == ANNOUNCEMENT::System && (stricmp(sender, "xbmc") == 0))
  {
    if (stricmp(message, "OnQuit") == 0)
    {
      CUrlOptions o;
      o.AddOption("cm1", boost::lexical_cast<std::string>(m_numberOfPlays));
      trackEvent("App", "Shutdown", "", m_sessionLength.elapsed(), o);

      m_timer.Stop();
    }
  } else if (flag == ANNOUNCEMENT::Player && (stricmp(sender, "xbmc") == 0))
  {
    if (stricmp(message, "OnPlay") == 0)
    {
      m_currentItem = g_application.CurrentFileItemPtr();
      m_startOffset = m_currentItem->GetProperty("viewOffset").asInteger() / 1000;
    }
    else if (stricmp(message, "OnStop") == 0 && m_currentItem)
    {
      int64_t playbackTime;
      if (m_currentItem->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_WATCHED)
        playbackTime = (m_currentItem->GetProperty("duration").asInteger() / 1000)- m_startOffset;
      else
        playbackTime = (m_currentItem->GetProperty("viewOffset").asInteger() / 1000) - m_startOffset;

      trackEvent("Playback",
                 m_currentItem->GetProperty("type").asString(),
                 m_currentItem->GetProperty("identifier").asString(),
                 playbackTime);
      m_timer.Restart();

      m_currentItem.reset();
      m_startOffset = 0;
      m_numberOfPlays += 1;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexAnalytics::OnTimeout()
{
  if (m_firstEvent)
  {
    m_firstEvent = false;
    m_timer.Start(PING_INTERVAL_SECONDS, true);

    CUrlOptions o("sc=start");
    trackEvent("App", "Startup", "", 1, o);
  } else {
    sendPing();
  }
}
