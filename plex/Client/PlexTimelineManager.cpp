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
  m_currentItems[MUSIC] = CFileItemPtr();
  m_currentItems[PHOTO] = CFileItemPtr();
  m_currentItems[VIDEO] = CFileItemPtr();
}

////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexTimelineManager::StateToString(MediaState state)
{
  CStdString strstate;
  switch (state) {
    case MEDIA_STATE_STOPPED:
      strstate = "stopped";
      break;
    case MEDIA_STATE_BUFFERING:
      strstate = "buffering";
      break;
    case MEDIA_STATE_PLAYING:
      strstate = "playing";
      break;
    case MEDIA_STATE_PAUSED:
      strstate = "paused";
      break;
  }
  return strstate;
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
void CPlexTimelineManager::SendTimelineToSubscribers()
{
  g_plexApplication.timer.SetTimeout(200, this);
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

  SendTimelineToSubscribers();
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
  BOOST_FOREACH(CPlexRemoteSubscriberPtr sub, g_plexApplication.remoteSubscriberManager->getSubscribers())
  {
    if (sub->isPoller() && sub->m_pollEvent.getNumWaits() > 0)
      sub->m_pollEvent.Set();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CUrlOptions CPlexTimelineManager::GetCurrentTimeline(MediaType type, bool forServer)
{
  CUrlOptions options;
  std::string durationStr;

  options.AddOption("state", StateToString(m_currentStates[type]));

  CFileItemPtr item = m_currentItems[type];
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
    options.AddOption("type", MediaTypeToString(type));

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

    if (type == MUSIC || type == VIDEO)
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
      if (type == VIDEO)
      {
        controllable.push_back("subtitleStream");
        controllable.push_back("audioStream");
      }
    }
    else if (type == PHOTO)
    {
      controllable.push_back("skipPrevious");
      controllable.push_back("skipNext");
    }

    if (controllable.size() > 0 && m_currentStates[type] != MEDIA_STATE_STOPPED)
      options.AddOption("controllable", StringUtils::Join(controllable, ","));

    if (g_application.IsPlaying() && m_currentStates[type] != MEDIA_STATE_STOPPED)
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

      if (type == VIDEO && g_application.IsPlayingVideo())
      {
        int subid = g_application.m_pPlayer->GetSubtitleVisible() ? g_application.m_pPlayer->GetSubtitlePlexID() : -1;
        options.AddOption("subtitleStreamID", subid);
        options.AddOption("audioStreamID", g_application.m_pPlayer->GetAudioStreamPlexID());
      }
    }

    if (m_currentStates[type] != MEDIA_STATE_STOPPED)
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
void CPlexTimelineManager::ReportProgress(const CFileItemPtr &newItem, CPlexTimelineManager::MediaState state, uint64_t currentPosition, bool force)
{
  if (!newItem)
    return;

  MediaType type = GetMediaType(newItem);
  if (type == UNKNOWN)
  {
    CLog::Log(LOGDEBUG, "PlexTimelineManager::ReportProgress unknown item %s", newItem->GetPath().c_str());
    return;
  }

  bool stateChange = false;

  CLog::Log(LOGDEBUG, "PlexTimelineManager::ReportProgress reporting for %s, current is %s", newItem->GetLabel().c_str(), m_currentItems[type] ? m_currentItems[type]->GetLabel().c_str() : "none");

  if (!m_currentItems[type] || newItem->GetPath() != m_currentItems[type]->GetPath())
  {
    if (m_currentStates[type] != MEDIA_STATE_STOPPED && m_currentItems[type])
    {
      // we need to stop the old media before playing the new one.
      CLog::Log(LOGDEBUG, "CPlexTimelineManager::ReportProgress Old item was never stopped, sending stop timeline now.");
      ReportProgress(m_currentItems[type], MEDIA_STATE_STOPPED, m_currentItems[type]->GetProperty("viewOffset").asInteger(), true);
    }
    /* we need a copy */
    m_currentItems[type] = CFileItemPtr(new CFileItem(*newItem.get()));
    stateChange = true;
  }

  /* now se the correct item, since we might have copied it */
  CFileItemPtr currentItem = m_currentItems[type];

  if (state != m_currentStates[type])
  {
    m_currentStates[type] = state;
    stateChange = true;
  }

  if (currentItem)
  {
    /* Let's cheat, if the timecode is something absurd, like bigger than our current
     * duration, let's just reset it to 0 */
    if (currentPosition > GetItemDuration(currentItem))
      currentPosition = 0;

    if (currentItem->GetProperty("viewOffset").asInteger() != currentPosition)
      currentItem->SetProperty("viewOffset", currentPosition);

    if (g_plexApplication.m_preplayItem &&
        g_plexApplication.m_preplayItem->GetPath() == currentItem->GetPath() &&
        g_plexApplication.m_preplayItem->GetProperty("viewOffset").asInteger() != currentPosition)
      g_plexApplication.m_preplayItem->SetProperty("viewOffset", currentPosition);
  }

  if (g_plexApplication.remoteSubscriberManager->hasSubscribers() &&
      (stateChange || m_subTimer.elapsedMs() >= 950))
  {
    CLog::Log(LOGDEBUG, "CPlexTimelineManager::ReportProgress updating subscribers: (%s) %s [%lld/%lld]",
              StateToString(m_currentStates[type]).c_str(), currentItem->GetLabel().c_str(), currentPosition, GetItemDuration(m_currentItems[type]));
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

  if (force || stateChange || m_serverTimer.elapsedMs() >= serverTimeout)
  {
    CLog::Log(LOGDEBUG, "CPlexTimelineManager::ReportProgress updating server: (%s) %s [%lld/%lld]",
              StateToString(m_currentStates[type]).c_str(), currentItem->GetLabel().c_str(), currentPosition, GetItemDuration(currentItem));
    g_plexApplication.mediaServerClient->SendServerTimeline(currentItem, GetCurrentTimeline(type));

    /* now we can see if we need to ping the transcoder as well */
    if (type == VIDEO && state == MEDIA_STATE_PAUSED && currentItem &&
        currentItem->GetProperty("plexDidTranscode").asBoolean() && server)
      g_plexApplication.mediaServerClient->SendTranscoderPing(server);

    m_serverTimer.restart();
  }

  /* Mark progress for the item */
  float percentage = 0.0;
  if (GetItemDuration(currentItem) > 0)
    percentage = (float)(((float)currentPosition/(float)GetItemDuration(currentItem)) * 100.0);

  if (currentItem->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_UNWATCHED &&
      currentPosition >= 5)
    currentItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_IN_PROGRESS);

  if ((currentItem->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_IN_PROGRESS ||
      currentItem->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_UNWATCHED) &&
      percentage > g_advancedSettings.m_videoPlayCountMinimumPercent)
    currentItem->MarkAsWatched();

  /* if we are stopping, we need to reset our currentItem */
  if (state == MEDIA_STATE_STOPPED)
    m_currentItems[type].reset();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexTimelineManager::MediaType CPlexTimelineManager::GetMediaType(CFileItemPtr item)
{
  EPlexDirectoryType plexType = item->GetPlexDirectoryType();

  switch(plexType)
  {
    case PLEX_DIR_TYPE_TRACK:
      return MUSIC;
    case PLEX_DIR_TYPE_VIDEO:
    case PLEX_DIR_TYPE_EPISODE:
    case PLEX_DIR_TYPE_CLIP:
    case PLEX_DIR_TYPE_MOVIE:
      return VIDEO;
    case PLEX_DIR_TYPE_IMAGE:
    case PLEX_DIR_TYPE_PHOTO:
    case PLEX_DIR_TYPE_PHOTOALBUM:
      return PHOTO;
    default:
      return UNKNOWN;
  }
  return UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexTimelineManager::MediaTypeToString(MediaType type)
{
  switch(type)
  {
    case MUSIC:
      return "music";
    case PHOTO:
      return "photo";
    case VIDEO:
      return "video";
    default:
      return "unknown";
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexTimelineManager::MediaType CPlexTimelineManager::GetMediaType(const CStdString &typestr)
{
  if (typestr == "music")
    return MUSIC;
  if (typestr == "photo")
    return PHOTO;
  if (typestr == "video")
    return VIDEO;
  return UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CXBMCTinyXML CPlexTimelineManager::WaitForTimeline(CPlexRemoteSubscriberPtr subscriber)
{
  if (!subscriber)
    return NULL;

  CLog::Log(LOGDEBUG, "CPlexTimelineManager::WaitForTimeline - %s is waiting until pollEvent is set.", subscriber->getUUID().c_str());

  subscriber->m_pollEvent.Reset();
  subscriber->m_pollEvent.WaitMSec(10000); /* wait 10 seconds */

  if (!m_stopped)
    return GetCurrentTimeLinesXML(subscriber);

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<CUrlOptions> CPlexTimelineManager::GetCurrentTimeLines(int commandID)
{
  std::vector<CUrlOptions> array;

  array.push_back(GetCurrentTimeline(MUSIC, false));
  array.push_back(GetCurrentTimeline(PHOTO, false));
  array.push_back(GetCurrentTimeline(VIDEO, false));

  return array;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CXBMCTinyXML CPlexTimelineManager::GetCurrentTimeLinesXML(CPlexRemoteSubscriberPtr subscriber)
{
  std::vector<CUrlOptions> tlines = GetCurrentTimeLines();

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
