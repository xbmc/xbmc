//
//  PlexRemoteSubscriberManager.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-19.
//
//

#include "PlexRemoteSubscriberManager.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "utils/log.h"
#include "Client/PlexMediaServerClient.h"
#include "video/VideoInfoTag.h"

#include "Application.h"
#include "PlexJobs.h"

////////////////////////////////////////////////////////////////////////////////////////
CPlexRemoteSubscriber::CPlexRemoteSubscriber(const std::string &uuid, const std::string &ipaddress, int port)
{
  m_url.SetProtocol("http");
  m_url.SetHostName(ipaddress);
  m_url.SetPort(port);
  
  m_uuid = uuid;
  m_lastUpdated.restart();
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemoteSubscriberManager::addSubscriber(CPlexRemoteSubscriberPtr subscriber)
{
  if (!subscriber) return;
  
  CSingleLock lk(m_crit);
  
  if (m_map.find(subscriber->getUUID()) != m_map.end())
  {
    CLog::Log(LOGDEBUG, "CPlexRemoteSubscriberManager::addSubscriber refreshed %s", subscriber->getUUID().c_str());
    m_map[subscriber->getUUID()]->refresh();
  }
  else
  {
    m_map[subscriber->getUUID()] = subscriber;
    CLog::Log(LOGDEBUG, "CPlexRemoteSubscriberManager::addSubscriber added %s", subscriber->getUUID().c_str());
    
    CPlexMediaServerClient::MediaState state;
    CFileItemPtr item;
    
    if (g_application.IsPlaying())
    {
      if (g_application.GetPlaySpeed() == 0)
        state = CPlexMediaServerClient::MEDIA_STATE_PAUSED;
      else
        state = CPlexMediaServerClient::MEDIA_STATE_PLAYING;
      
      item = g_application.CurrentFileItemPtr();
    }
    else
      state = CPlexMediaServerClient::MEDIA_STATE_STOPPED;
    
    sendTimeLineRequest(subscriber, item, state, g_application.GetTime());
  }
  
  if (!m_refreshTimer.IsRunning())
    m_refreshTimer.Start(PLEX_REMOTE_SUBSCRIBER_REMOVE_INTERVAL);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemoteSubscriberManager::removeSubscriber(CPlexRemoteSubscriberPtr subscriber)
{
  if (!subscriber) return;
  
  CSingleLock lk(m_crit);
  
  if (m_map.find(subscriber->getUUID()) == m_map.end())
    return;
  
  m_map.erase(subscriber->getUUID());
  
  if (m_map.size() == 0)
    m_refreshTimer.Stop();
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemoteSubscriberManager::OnTimeout()
{
  CSingleLock lk(m_crit);
  
  std::vector<std::string> subsToRemove;
  
  CLog::Log(LOGDEBUG, "CPlexRemoteSubscriberManager::OnTimeout starting with %lu clients", m_map.size());
  
  BOOST_FOREACH(SubscriberPair p, m_map)
  {
    if (p.second->shouldRemove())
      subsToRemove.push_back(p.first);
  }
  
  BOOST_FOREACH(std::string s, subsToRemove)
    m_map.erase(s);
  
  CLog::Log(LOGDEBUG, "CPlexRemoteSubscriberManager::OnTimeout %lu clients left after timeout", m_map.size());
  
  /* still clients to handle, restart the timer */
  if (m_map.size() > 0)
    m_refreshTimer.Start(PLEX_REMOTE_SUBSCRIBER_REMOVE_INTERVAL);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemoteSubscriberManager::reportItemProgress(const CFileItemPtr &item, CPlexMediaServerClient::MediaState state, int64_t currentPosition)
{
  if (!hasSubscribers())
    return;
  
  CSingleLock lk(m_crit);
  
  BOOST_FOREACH(SubscriberPair p, m_map)
    sendTimeLineRequest(p.second, item, state, currentPosition);
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexRemoteSubscriberManager::sendTimeLineRequest(CPlexRemoteSubscriberPtr sub, const CFileItemPtr &item, CPlexMediaServerClient::MediaState state, int64_t currentPosition)
{
  CURL u(sub->getURL());
  u.SetFileName(":/timeline");
  
  u.SetOption("state", CPlexMediaServerClient::StateToString(state));
  u.SetOption("controllable", "volume,shuffle,repeat,audioStream,videoStream,subtitleStream");
  u.SetOption("volume", boost::lexical_cast<std::string>(g_application.GetVolume()));

  if (item)
  {
    u.SetOption("ratingKey", item->GetProperty("ratingKey").asString());
    u.SetOption("key", item->GetProperty("unprocessed_key").asString());
    u.SetOption("containerKey", item->GetProperty("containerKey").asString());
    u.SetOption("machineIdentifier", item->GetProperty("plexserver").asString());

    if (item->HasProperty("guid"))
      u.SetOption("guid", item->GetProperty("guid").asString());
    
    if (item->HasProperty("url"))
      u.SetOption("url", item->GetProperty("url").asString());

    if (item->HasVideoInfoTag())
      u.SetOption("duration", boost::lexical_cast<std::string>(item->GetVideoInfoTag()->m_duration * 1000));
  }
  
  if (currentPosition != 0)
    u.SetOption("time", boost::lexical_cast<std::string>(currentPosition));
  
  if (g_application.IsPlayingAudio())
  {
    if (g_playlistPlayer.IsShuffled(PLAYLIST_MUSIC))
      u.SetOption("shuffled", "1");
    
    if (g_playlistPlayer.GetRepeat(PLAYLIST_MUSIC) == PLAYLIST::REPEAT_ONE)
      u.SetOption("repeat", "1");
    else if (g_playlistPlayer.GetRepeat(PLAYLIST_MUSIC) == PLAYLIST::REPEAT_ALL)
      u.SetOption("repeat", "2");
  }
  else if (g_application.IsPlayingVideo())
  {
    u.SetOption("subtitleStreamID", boost::lexical_cast<std::string>(g_application.m_pPlayer->GetSubtitlePlexID()));
    u.SetOption("audioStreamID", boost::lexical_cast<std::string>(g_application.m_pPlayer->GetAudioStreamPlexID()));
    
    /* TODO */
    u.SetOption("videoStreamID", "0");
  }
  
  g_plexMediaServerClient.AddJob(new CPlexMediaServerClientJob(u));
  
}