#include "PlexTimelineManager.h"
#include "UrlOptions.h"
#include "log.h"
#include "Application.h"
#include "PlexApplication.h"
#include "Client/PlexMediaServerClient.h"
#include "GUIWindowManager.h"
#include "PlayList.h"
#include "plex/Remote/PlexRemoteSubscriberManager.h"
#include "utils/StringUtils.h"
#include <boost/lexical_cast.hpp>
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include <boost/foreach.hpp>
#include "pictures/GUIWindowSlideShow.h"

#include "Client/PlexServer.h"
#include "Client/PlexServerManager.h"

#include "FileItem.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexTimelineManager::CPlexTimelineManager() : m_stopped(false), m_textFieldFocused(false), m_textFieldSecure(false)
{
  m_contexts[PLEX_MEDIA_TYPE_MUSIC] = CPlexTimelineContext(PLEX_MEDIA_TYPE_MUSIC);
  m_contexts[PLEX_MEDIA_TYPE_VIDEO] = CPlexTimelineContext(PLEX_MEDIA_TYPE_VIDEO);
  m_contexts[PLEX_MEDIA_TYPE_PHOTO] = CPlexTimelineContext(PLEX_MEDIA_TYPE_PHOTO);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
uint64_t CPlexTimelineManager::GetItemDuration(CFileItemPtr item)
{
  if (item->HasProperty("duration"))
    return item->GetProperty("duration").asInteger();
  else if (item->HasVideoInfoTag())
    return item->GetVideoInfoTag()->m_duration * 1000;
  else if (item->HasMusicInfoTag())
    return item->GetMusicInfoTag()->GetDuration() * 1000;

  return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::SendTimelineToSubscriber(CPlexRemoteSubscriberPtr subscriber)
{
  if (!subscriber)
    return;

  if (subscriber->isPoller())
    return;

  CXBMCTinyXML timelineXML = GetCurrentTimeLinesXML(subscriber);
  if (!timelineXML.FirstChild())
    return;

  CURL url = subscriber->getURL();
  url.SetFileName(":/timeline");

  g_plexApplication.mediaServerClient->SendSubscriberTimeline(url, PlexUtils::GetXMLString(timelineXML));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::SendTimelineToSubscribers(bool delay)
{
  if (delay)
    g_plexApplication.timer.SetTimeout(200, this);
  else
    OnTimeout();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::OnTimeout()
{
  BOOST_FOREACH(CPlexRemoteSubscriberPtr sub, g_plexApplication.remoteSubscriberManager->getSubscribers())
    SendTimelineToSubscriber(sub);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::SetTextFieldFocused(bool focused, const CStdString &name, const CStdString &contents, bool isSecure)
{
  m_textFieldFocused = focused;
  if (m_textFieldFocused)
  {
    m_textFieldName = name;
    m_textFieldContents = contents;
    m_textFieldSecure = isSecure;
  }
  else if (name == m_textFieldName) /* we only remove the data if the fieldname matches */
  {
    m_textFieldName = "";
    m_textFieldContents = "";
    m_textFieldSecure = false;
  }

  SendTimelineToSubscribers(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::UpdateLocation()
{
  SendTimelineToSubscribers();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::Stop()
{
  m_stopped = true;
  NotifyPollers();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::NotifyPollers()
{
  std::vector<CPlexRemoteSubscriberPtr> pollers;
  BOOST_FOREACH(CPlexRemoteSubscriberPtr sub, g_plexApplication.remoteSubscriberManager->getSubscribers())
  {
    if (sub->isPoller())
      pollers.push_back(sub);
  }

  if (!pollers.empty())
  {
    {
      CSingleLock lk(m_pollerTimelineLock);
      CSingleLock lk2(m_timelineLock);
      m_pollerContexts.push(m_contexts);
    }

    BOOST_FOREACH(CPlexRemoteSubscriberPtr sub, pollers)
    {
      if (sub->m_pollEvent.getNumWaits() > 0)
        sub->m_pollEvent.Set();
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CUrlOptions CPlexTimelineManager::GetCurrentTimeline(const CPlexTimelineContext& context, bool forServer)
{
  CUrlOptions options;
  std::string durationStr;

  options.AddOption("state", PlexUtils::GetMediaStateString(context.state));

  CFileItemPtr item = context.item;
  if (item)
  {
    options.AddOption("time", item->GetProperty("viewOffset").asString());

    options.AddOption("ratingKey", item->GetProperty("ratingKey").asString());
    options.AddOption("key", item->GetProperty("unprocessed_key").asString());
    options.AddOption("containerKey", item->GetProperty("containerKey").asString());

    if (item->HasProperty("guid"))
      options.AddOption("guid", item->GetProperty("guid").asString());

    if (item->HasProperty("url"))
      options.AddOption("url", item->GetProperty("url").asString());

    if (GetItemDuration(item) > 0)
    {
      durationStr = boost::lexical_cast<std::string>(GetItemDuration(item));
      options.AddOption("duration", durationStr);
    }

  }
  else
  {
    options.AddOption("time", 0);
  }

  if (!forServer)
  {
    options.AddOption("type", PlexUtils::GetMediaTypeString(context.type));

    if (item)
    {
      CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(item->GetProperty("plexserver").asString());
      if (server && server->GetActiveConnection())
      {
        options.AddOption("port", server->GetActiveConnectionURL().GetPort());
        options.AddOption("protocol", server->GetActiveConnectionURL().GetProtocol());
        options.AddOption("address", server->GetActiveConnectionURL().GetHostName());

        options.AddOption("token", "");
      }

      options.AddOption("machineIdentifier", item->GetProperty("plexserver").asString());
    }

    int player = g_application.IsPlayingAudio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO;
    int playlistLen = g_playlistPlayer.GetPlaylist(player).size();
    int playlistPos = g_playlistPlayer.GetCurrentSong();

    /* determine what things are controllable */
    std::vector<std::string> controllable;

    controllable.push_back("playPause");
    controllable.push_back("stop");

    if (context.type == PLEX_MEDIA_TYPE_MUSIC || context.type == PLEX_MEDIA_TYPE_VIDEO)
    {
      if (playlistLen > 0)
      {
        if (playlistPos > 0)
          controllable.push_back("skipPrevious");
        if (playlistLen > (playlistPos + 1))
          controllable.push_back("skipNext");

        controllable.push_back("shuffle");
        controllable.push_back("repeat");
      }

      if (g_application.m_pPlayer && !g_application.m_pPlayer->IsPassthrough())
        controllable.push_back("volume");

      controllable.push_back("stepBack");
      controllable.push_back("stepForward");
      controllable.push_back("seekTo");
      if (context.type == PLEX_MEDIA_TYPE_VIDEO)
      {
        controllable.push_back("subtitleStream");
        controllable.push_back("audioStream");
      }
    }
    else if (context.type == PLEX_MEDIA_TYPE_PHOTO)
    {
      controllable.push_back("skipPrevious");
      controllable.push_back("skipNext");
    }

    if (controllable.size() > 0 && context.state != PLEX_MEDIA_STATE_STOPPED)
      options.AddOption("controllable", StringUtils::Join(controllable, ","));

    if (g_application.IsPlaying() && context.state != PLEX_MEDIA_STATE_STOPPED)
    {
      options.AddOption("volume", g_application.GetVolume());

      if (g_playlistPlayer.IsShuffled(player))
        options.AddOption("shuffle", 1);
      else
        options.AddOption("shuffle", 0);

      if (g_playlistPlayer.GetRepeat(player) == PLAYLIST::REPEAT_ONE)
        options.AddOption("repeat", 1);
      else if (g_playlistPlayer.GetRepeat(player) == PLAYLIST::REPEAT_ALL)
        options.AddOption("repeat", 2);
      else
        options.AddOption("repeat", 0);

      options.AddOption("mute", g_application.IsMuted() ? "1" : "0");

      if (context.type == PLEX_MEDIA_TYPE_VIDEO && g_application.IsPlayingVideo())
      {
        int subid = g_application.m_pPlayer->GetSubtitleVisible() ? g_application.m_pPlayer->GetSubtitlePlexID() : -1;
        options.AddOption("subtitleStreamID", subid);
        options.AddOption("audioStreamID", g_application.m_pPlayer->GetAudioStreamPlexID());
      }
    }

    if (context.state != PLEX_MEDIA_STATE_STOPPED)
    {
      std::string location = "navigation";

      int currentWindow = g_windowManager.GetActiveWindow();
      if (g_application.IsPlayingFullScreenVideo())
        location = "fullScreenVideo";
      else if (currentWindow == WINDOW_SLIDESHOW)
        location = "fullScreenPhoto";
      else if (currentWindow == WINDOW_NOW_PLAYING || currentWindow == WINDOW_VISUALISATION)
        location = "fullScreenMusic";

      options.AddOption("location", location);
    }
    else if (context.continuing)
      options.AddOption("continuing", "1");

    if (g_application.IsPlaying() && g_application.m_pPlayer)
    {
      if (g_application.m_pPlayer->CanSeek() && !durationStr.empty())
        options.AddOption("seekRange", "0-" + durationStr);
      else
        options.AddOption("seekRange", "0-0");
    }

  }

  return options;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::ReportProgress(const CFileItemPtr &newItem, ePlexMediaState state, uint64_t currentPosition, bool force)
{
  if (!newItem)
    return;

  ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(newItem);
  if (type == PLEX_MEDIA_TYPE_UNKNOWN)
  {
    CLog::Log(LOGDEBUG, "PlexTimelineManager::ReportProgress unknown item %s", newItem->GetPath().c_str());
    return;
  }

  CPlexTimelineContext newContext;
  newContext.item = CFileItemPtr(new CFileItem(*newItem.get()));
  newContext.state = state;
  newContext.currentPosition = currentPosition;

  CSingleLock lk(m_timelineLock);

  CPlexTimelineContext oldContext = m_contexts[type];

  lk.unlock();

  //CLog::Log(LOGDEBUG, "PlexTimelineManager::ReportProgress reporting for %s, current is %s", newItem->GetLabel().c_str(), m_currentItems[type] ? m_currentItems[type]->GetLabel().c_str() : "none");

  if (!oldContext || newContext.item->GetPath() != oldContext.item->GetPath())
  {
    if (oldContext.state != PLEX_MEDIA_STATE_STOPPED && oldContext)
    {
      // we need to stop the old media before playing the new one.
      CLog::Log(LOGDEBUG, "CPlexTimelineManager::ReportProgress Old item was never stopped, sending stop timeline now.");
      oldContext.continuing = true;
      oldContext.state = PLEX_MEDIA_STATE_STOPPED;

      lk.lock();
      m_contexts[type] = oldContext;
      lk.unlock();

      ReportProgress(oldContext, true);
    }
  }

  /* force if the caller has set the force flag, or if the
   * context has changed enough to warrant a direct timeline
   */
  bool reallyForce = force ? true : (newContext != oldContext);

  lk.lock();
  m_contexts[type] = newContext;
  lk.unlock();

  ReportProgress(newContext, reallyForce);

  if (newContext.state == PLEX_MEDIA_STATE_STOPPED)
  {
    lk.lock();
    m_contexts[type] = CPlexTimelineContext();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::ReportProgress(const CPlexTimelineContext &context, bool force)
{
  /* now se the correct item, since we might have copied it */
  CFileItemPtr currentItem = context.item;

  int64_t realPosition = context.currentPosition;

  if (currentItem)
  {
    /* Let's cheat, if the timecode is something absurd, like bigger than our current
     * duration, let's just reset it to 0 */
    if (realPosition > GetItemDuration(currentItem))
      realPosition = 0;

    if (currentItem->GetProperty("viewOffset").asInteger() != realPosition)
      currentItem->SetProperty("viewOffset", realPosition);

    if (g_plexApplication.m_preplayItem &&
        g_plexApplication.m_preplayItem->GetPath() == currentItem->GetPath() &&
        g_plexApplication.m_preplayItem->GetProperty("viewOffset").asInteger() != realPosition)
      g_plexApplication.m_preplayItem->SetProperty("viewOffset", realPosition);
  }

  if (g_plexApplication.remoteSubscriberManager->hasSubscribers() &&
      (force || m_subTimer.elapsedMs() >= 950))
  {
    CLog::Log(LOGDEBUG, "CPlexTimelineManager::ReportProgress updating subscribers: (%s) %s [%lld/%lld]",
              PlexUtils::GetMediaStateString(context.state).c_str(),
              currentItem->GetLabel().c_str(),
              realPosition,
              GetItemDuration(currentItem));

    NotifyPollers();
    SendTimelineToSubscribers();
    m_subTimer.restart();
  }

  int serverTimeout = 9950; /* default to 10 seconds for local servers */

  CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(currentItem);

  if (server && (server->GetUUID() == "myplex" || server->GetUUID() == "node"))
    serverTimeout = 9950 * 3; // 30 seconds for myPlex or node
  else if (server && server->GetActiveConnection() && !server->GetActiveConnection()->IsLocal())
    serverTimeout = 9950 * 3; // 30 seconds for remote server

  if (force || m_serverTimer.elapsedMs() >= serverTimeout)
  {
    CLog::Log(LOGDEBUG, "CPlexTimelineManager::ReportProgress updating server: (%s) %s [%lld/%lld]",
              PlexUtils::GetMediaStateString(context.state).c_str(),
              currentItem->GetLabel().c_str(),
              realPosition,
              GetItemDuration(currentItem));
    g_plexApplication.mediaServerClient->SendServerTimeline(currentItem, GetCurrentTimeline(context.type));

    /* now we can see if we need to ping the transcoder as well */
    if (context.type == PLEX_MEDIA_TYPE_VIDEO && context.state == PLEX_MEDIA_STATE_PAUSED && currentItem &&
        currentItem->GetProperty("plexDidTranscode").asBoolean() && server)
      g_plexApplication.mediaServerClient->SendTranscoderPing(server);

    m_serverTimer.restart();
  }

  /* Mark progress for the item */
  float percentage = 0.0;
  if (GetItemDuration(currentItem) > 0)
    percentage = (float)(((float)realPosition/(float)GetItemDuration(currentItem)) * 100.0);

  if (currentItem->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_UNWATCHED &&
      realPosition >= 5)
    currentItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_IN_PROGRESS);

  if ((currentItem->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_IN_PROGRESS ||
      currentItem->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_UNWATCHED) &&
      percentage > g_advancedSettings.m_videoPlayCountMinimumPercent)
    currentItem->MarkAsWatched();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CXBMCTinyXML CPlexTimelineManager::WaitForTimeline(CPlexRemoteSubscriberPtr subscriber)
{
  if (!subscriber)
    return NULL;

  CLog::Log(LOGDEBUG, "CPlexTimelineManager::WaitForTimeline - %s is waiting until pollEvent is set.", subscriber->getUUID().c_str());

  bool wait = false;
  {
    CSingleLock lk(m_pollerTimelineLock);
    wait = m_pollerContexts.empty();
  }

  if (wait)
  {
    subscriber->m_pollEvent.Reset();
    subscriber->m_pollEvent.WaitMSec(10000); /* wait 10 seconds */
  }

  if (!m_stopped)
    return GetCurrentTimeLinesXML(subscriber);

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<CUrlOptions> CPlexTimelineManager::GetCurrentTimeLines(int commandID)
{
  std::vector<CUrlOptions> array;

  CSingleLock lk(m_timelineLock);

  array.push_back(GetCurrentTimeline(m_contexts[PLEX_MEDIA_TYPE_MUSIC], false));
  array.push_back(GetCurrentTimeline(m_contexts[PLEX_MEDIA_TYPE_PHOTO], false));
  array.push_back(GetCurrentTimeline(m_contexts[PLEX_MEDIA_TYPE_VIDEO], false));

  return array;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CXBMCTinyXML CPlexTimelineManager::GetCurrentTimeLinesXML(CPlexRemoteSubscriberPtr subscriber)
{
  std::vector<CUrlOptions> tlines;

  {
    CSingleLock lk(m_pollerTimelineLock);
    PlexTimelineContextMap cmap;

    if (!subscriber->isPoller() || m_pollerContexts.empty())
    {
      CSingleLock lk(m_timelineLock);
      cmap = m_contexts;
    }
    else
    {
      cmap = m_pollerContexts.front();
    }

    tlines.push_back(GetCurrentTimeline(cmap[PLEX_MEDIA_TYPE_MUSIC], false));
    tlines.push_back(GetCurrentTimeline(cmap[PLEX_MEDIA_TYPE_PHOTO], false));
    tlines.push_back(GetCurrentTimeline(cmap[PLEX_MEDIA_TYPE_VIDEO], false));
  }


  CXBMCTinyXML doc;
  doc.LinkEndChild(new TiXmlDeclaration("1.0", "utf-8", ""));
  TiXmlElement *mediaContainer = new TiXmlElement("MediaContainer");
  mediaContainer->SetAttribute("location", "navigation"); // default
  if (m_textFieldFocused)
  {
    mediaContainer->SetAttribute("textFieldFocused", std::string(m_textFieldName));
    mediaContainer->SetAttribute("textFieldSecure", m_textFieldSecure ? "1" : "0");
    mediaContainer->SetAttribute("textFieldContent", std::string(m_textFieldContents));
  }

  if (subscriber->getCommandID() != -1)
    mediaContainer->SetAttribute("commandID", subscriber->getCommandID());

  BOOST_FOREACH(CUrlOptions options, tlines)
  {
    std::pair<std::string, CVariant> p;
    TiXmlElement *lineEl = new TiXmlElement("Timeline");
    BOOST_FOREACH(p, options.GetOptions())
    {
      if (p.first == "location")
        mediaContainer->SetAttribute("location", p.second.asString().c_str());

      if (p.second.isString() && p.second.empty())
        continue;

      lineEl->SetAttribute(p.first, p.second.asString());
    }
    mediaContainer->LinkEndChild(lineEl);
  }
  doc.LinkEndChild(mediaContainer);

  return doc;
}
