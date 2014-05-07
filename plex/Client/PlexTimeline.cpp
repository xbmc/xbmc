#include "PlexTimeline.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "PlexUtils.h"
#include "PlexTimelineManager.h"
#include "PlexApplication.h"
#include "PlexServerManager.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "PlayList.h"
#include "StringUtils.h"
#include "guilib/GUIWindowManager.h"
#include "PlexPlayQueueManager.h"
#include "music/tags/MusicInfoTag.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CUrlOptions CPlexTimeline::getTimeline(bool forServer)
{
  CUrlOptions options;
  std::string durationStr;

  options.AddOption("state", PlexUtils::GetMediaStateString(m_state));

  if (m_item)
  {
    CStdString time = m_item->GetProperty("viewOffset").asString();
    time = time.empty() ? "0" : time;

    options.AddOption("time", time);

    options.AddOption("ratingKey", m_item->GetProperty("ratingKey").asString());
    options.AddOption("key", m_item->GetProperty("unprocessed_key").asString());
    options.AddOption("containerKey", m_item->GetProperty("containerKey").asString());

    if (m_item->HasProperty("guid"))
      options.AddOption("guid", m_item->GetProperty("guid").asString());

    if (m_item->HasProperty("url"))
      options.AddOption("url", m_item->GetProperty("url").asString());

    if (CPlexTimelineManager::GetItemDuration(m_item) > 0)
    {
      durationStr = boost::lexical_cast<std::string>(CPlexTimelineManager::GetItemDuration(m_item));
      options.AddOption("duration", durationStr);
    }

  }
  else
  {
    options.AddOption("time", 0);
  }

  if (!forServer)
  {
    options.AddOption("type", PlexUtils::GetMediaTypeString(m_type));

    if (m_item)
    {
      CPlexServerPtr server = g_plexApplication.serverManager->FindByUUID(m_item->GetProperty("plexserver").asString());
      if (server && server->GetActiveConnection())
      {
        options.AddOption("port", server->GetActiveConnectionURL().GetPort());
        options.AddOption("protocol", server->GetActiveConnectionURL().GetProtocol());
        options.AddOption("address", server->GetActiveConnectionURL().GetHostName());

        options.AddOption("token", "");
      }

      options.AddOption("machineIdentifier", m_item->GetProperty("plexserver").asString());
    }

    int player = g_application.IsPlayingAudio() ? PLAYLIST_MUSIC : PLAYLIST_VIDEO;
    int playlistLen = g_playlistPlayer.GetPlaylist(player).size();
    int playlistPos = g_playlistPlayer.GetCurrentSong();

    /* determine what things are controllable */
    std::vector<std::string> controllable;

    controllable.push_back("playPause");
    controllable.push_back("stop");

    if (m_type == PLEX_MEDIA_TYPE_MUSIC || m_type == PLEX_MEDIA_TYPE_VIDEO)
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
      if (m_type == PLEX_MEDIA_TYPE_VIDEO)
      {
        controllable.push_back("subtitleStream");
        controllable.push_back("audioStream");
      }
    }
    else if (m_type == PLEX_MEDIA_TYPE_PHOTO)
    {
      controllable.push_back("skipPrevious");
      controllable.push_back("skipNext");
    }

    if (controllable.size() > 0 && m_state != PLEX_MEDIA_STATE_STOPPED)
      options.AddOption("controllable", StringUtils::Join(controllable, ","));

    if (g_application.IsPlaying() && m_state != PLEX_MEDIA_STATE_STOPPED)
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

      if (m_type == PLEX_MEDIA_TYPE_VIDEO && g_application.IsPlayingVideo())
      {
        int subid = g_application.m_pPlayer->GetSubtitleVisible() ? g_application.m_pPlayer->GetSubtitlePlexID() : -1;
        options.AddOption("subtitleStreamID", subid);
        options.AddOption("audioStreamID", g_application.m_pPlayer->GetAudioStreamPlexID());
      }
    }

    if (m_state != PLEX_MEDIA_STATE_STOPPED)
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
    else if (m_continuing)
      options.AddOption("continuing", "1");

    if (g_application.IsPlaying() && g_application.m_pPlayer)
    {
      if (g_application.m_pPlayer->CanSeek() && !durationStr.empty())
        options.AddOption("seekRange", "0-" + durationStr);
      else
        options.AddOption("seekRange", "0-0");
    }

    if (PlexUtils::IsPlayingPlaylist())
    {
      int playQueueId = g_plexApplication.playQueueManager->getCurrentID();
      if (playQueueId != -1)
        options.AddOption("playQueueID", boost::lexical_cast<std::string>(playQueueId));

      if (m_item && m_item->HasMusicInfoTag())
      {
        try
        {
          std::string pqid = boost::lexical_cast<std::string>(m_item->GetMusicInfoTag()->GetDatabaseId());
          options.AddOption("playQueueItemID", pqid);
        }
        catch (...)
        {
        }
      }

      int playQueueVersion = g_plexApplication.playQueueManager->getCurrentPlayQueueVersion();
      options.AddOption("playQueueVersion", boost::lexical_cast<std::string>(playQueueVersion));

    }

  }

  return options;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CXBMCTinyXML CPlexTimelineCollection::getTimelinesXML(int commandID)
{
  std::vector<CUrlOptions> tlines;

  tlines.push_back(m_timelines[PLEX_MEDIA_TYPE_MUSIC]->getTimeline(false));
  tlines.push_back(m_timelines[PLEX_MEDIA_TYPE_VIDEO]->getTimeline(false));
  tlines.push_back(m_timelines[PLEX_MEDIA_TYPE_PHOTO]->getTimeline(false));

  CXBMCTinyXML doc;
  doc.LinkEndChild(new TiXmlDeclaration("1.0", "utf-8", ""));
  TiXmlElement *mediaContainer = new TiXmlElement("MediaContainer");
  mediaContainer->SetAttribute("location", "navigation"); // default

  CStdString name, contents;
  bool secure;
  if (g_plexApplication.timelineManager->GetTextFieldInfo(name, contents, secure))
  {
    mediaContainer->SetAttribute("textFieldFocused", std::string(name));
    mediaContainer->SetAttribute("textFieldSecure", secure ? "1" : "0");
    mediaContainer->SetAttribute("textFieldContent", std::string(contents));
  }

  if (commandID != -1)
    mediaContainer->SetAttribute("commandID", commandID);

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
