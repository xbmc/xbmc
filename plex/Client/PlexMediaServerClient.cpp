//
//  PlexMediaServerClient.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-06-11.
//
//

#include "PlexMediaServerClient.h"

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <string>

#include "PlexFile.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "Client/PlexTranscoderClient.h"
#include "PlexJobs.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "guilib/Key.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"

#include "Application.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexMediaServerClientJob *clientJob = static_cast<CPlexMediaServerClientJob*>(job);

  if (success && clientJob->m_msg.GetMessage() != 0)
    g_windowManager.SendThreadMessage(clientJob->m_msg);
  else if (!success && clientJob->m_errorMsg)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(257), g_localizeStrings.Get(clientJob->m_errorMsg));
  
  CJobQueue::OnJobComplete(jobID, success, job);
}


////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SelectStream(const CFileItemPtr &item,
                                     int partID,
                                     int subtitleStreamID,
                                     int audioStreamID)
{
  CURL u(item->GetPath());
  
  u.SetFileName("/library/parts/" + boost::lexical_cast<std::string>(partID));
  if (subtitleStreamID != -1)
    u.SetOption("subtitleStreamID", boost::lexical_cast<std::string>(subtitleStreamID));
  if (audioStreamID != -1)
    u.SetOption("audioStreamID", boost::lexical_cast<std::string>(audioStreamID));
  
  AddJob(new CPlexMediaServerClientJob(u, "PUT"));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::ReportItemProgress(const CFileItemPtr &item, CPlexMediaServerClient::MediaState state, int64_t currentPosition)
{
  int64_t totalDuration = item->GetProperty("duration").asInteger();
  time_t now = time(NULL);
  
  
  if (m_lastItemKey == item->GetProperty("key").asString() &&
      m_lastItemState == state)
  {
    int64_t lastUpdated = item->GetProperty("lastTimelineUpdate").asInteger();

    if (currentPosition == item->GetProperty("viewOffset").asInteger() &&
        (now - lastUpdated < 10))
      /* same state, same key no change in played duration */
      return;
    
    if (now - lastUpdated >= 5)
    {
      item->SetProperty("viewOffset", currentPosition);
      /* report to the server */
      CURL u = constructTimelineRequest(item, state, currentPosition, false);
      AddJob(new CPlexMediaServerClientJob(u));
      
      item->SetProperty("lastTimelineUpdate", (int64_t)now);
    }
    
    if (now - lastUpdated >= 1 && g_plexRemoteSubscriberManager.hasSubscribers())
    {
      std::vector<CURL> subs = g_plexRemoteSubscriberManager.getSubscriberURL();
      BOOST_FOREACH(CURL su, subs)
        ReportItemProgressToSubscriber(su, item, state, currentPosition);
      
      item->SetProperty("viewOffset", currentPosition);
    }
  }
  else
  {
    CURL u = constructTimelineRequest(item, state, currentPosition, false);
    AddJob(new CPlexMediaServerClientJob(u));
    
    /* notify any subscribers as well */
    std::vector<CURL> subs = g_plexRemoteSubscriberManager.getSubscriberURL();
    BOOST_FOREACH(CURL su, subs)
      ReportItemProgressToSubscriber(su, item, state, currentPosition);
    
    m_lastItemKey = item->GetProperty("key").asString();
    m_lastItemState = state;
    item->SetProperty("lastTimelineUpdate", (int64_t)now);
  }
  
  float percentage = 0.0;
  if (totalDuration)
  {
    percentage = (float)(((float)currentPosition/(float)totalDuration) * 100.0);
    CLog::Log(LOGDEBUG, "CPlexMediaServerClient::ReportItemProgress %lld / %lld = %f", currentPosition, totalDuration, percentage);
  }
  
  if (item->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_UNWATCHED &&
      currentPosition >= 5)
  {
    item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_IN_PROGRESS);
  }
  
  CLog::Log(LOGDEBUG, "CPlexMediaServerClient::ReportItemProgress percentage is %f", percentage);
  if ((item->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_IN_PROGRESS ||
      item->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_UNWATCHED) &&
      percentage > g_advancedSettings.m_videoPlayCountMinimumPercent)
  {
    item->MarkAsWatched();
  }
    
  
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::ReportItemProgressToSubscriber(const CURL &url, CFileItemPtr item, CPlexMediaServerClient::MediaState state, int64_t currentPosition)
{
  CURL u = constructTimelineRequest(item, state, currentPosition, true);
  u.SetProtocol("http");
  u.SetHostName(url.GetHostName());
  u.SetPort(url.GetPort());
  
  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SetItemWatchStatus(const CFileItemPtr &item, bool watched)
{
  CURL u(item->GetPath());
  
  u.SetFileName(GetPrefix(item) + (watched ? "scrobble" : "unscrobble"));
  u.SetOption("key", item->GetProperty("ratingKey").asString());
  u.SetOption("identifier", item->GetProperty("identifier").asString());
  
  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SetItemRating(const CFileItemPtr &item, float rating)
{
  CURL u(item->GetPath());
  
  u.SetFileName(GetPrefix(item) + "rate");
  u.SetOption("rating", boost::lexical_cast<std::string>(rating));
  
  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::SetViewMode(const CFileItem &item, int viewMode, int sortMode, int sortAsc)
{
  CURL u(item.GetPath());
  u.SetFileName("/:/viewChange");
  u.SetOption("identifier", item.GetProperty("identifier").asString());
  u.SetOption("viewGroup", item.GetProperty("viewGroup").asString());

  u.SetOption("viewMode", boost::lexical_cast<CStdString>(viewMode));
  u.SetOption("sortMode", boost::lexical_cast<CStdString>(sortMode));
  u.SetOption("sortAsc", boost::lexical_cast<CStdString>(sortAsc));

  AddJob(new CPlexMediaServerClientJob(u));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::StopTranscodeSession(CPlexServerPtr server)
{
  AddJob(new CPlexMediaServerClientJob(CPlexTranscoderClient::GetTranscodeStopURL(server)));
}

////////////////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerClient::deleteItem(const CFileItemPtr &item)
{
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE, g_windowManager.GetActiveWindow());
  AddJob(new CPlexMediaServerClientJob(item->GetPath(), "DELETE", msg, 16205));
}

////////////////////////////////////////////////////////////////////////////////////////
std::string CPlexMediaServerClient::StateToString(CPlexMediaServerClient::MediaState state)
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

////////////////////////////////////////////////////////////////////////////////////////
CURL CPlexMediaServerClient::constructTimelineRequest(CFileItemPtr item, CPlexMediaServerClient::MediaState state, int64_t currentPosition, bool includeSystemVars)
{
  CURL u("http://localhost:32400/");
  if (item)
    u = CURL(item->GetPath());
  
  u.SetFileName(":/timeline");
  
  u.SetOption("state", StateToString(state));
  u.SetOption("controllable", "volume,shuffle,repeat,audioStream,videoStream,subtitleStream");
  u.SetOption("time", boost::lexical_cast<std::string>(currentPosition));
  
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
    else if (item->HasMusicInfoTag())
      u.SetOption("duration", boost::lexical_cast<std::string>(item->GetMusicInfoTag()->GetDuration() * 1000));
  }
  
  if (includeSystemVars)
  {
    u.SetOption("volume", boost::lexical_cast<std::string>(g_application.GetVolume()));
    if (g_application.IsPlayingAudio())
    {
      if (g_playlistPlayer.IsShuffled(PLAYLIST_MUSIC))
        u.SetOption("shuffle", "1");
      else
        u.SetOption("shuffle", "0");
      
      if (g_playlistPlayer.GetRepeat(PLAYLIST_MUSIC) == PLAYLIST::REPEAT_ONE)
        u.SetOption("repeat", "1");
      else if (g_playlistPlayer.GetRepeat(PLAYLIST_MUSIC) == PLAYLIST::REPEAT_ALL)
        u.SetOption("repeat", "2");
      else
        u.SetOption("repeat", "0");
    }
    else if (g_application.IsPlayingVideo())
    {
      std::string subid = boost::lexical_cast<std::string>(g_application.m_pPlayer->GetSubtitlePlexID());
      if (!g_application.m_pPlayer->GetSubtitleVisible())
        subid = "-1";
      u.SetOption("subtitleStreamID", subid);
      
      u.SetOption("audioStreamID", boost::lexical_cast<std::string>(g_application.m_pPlayer->GetAudioStreamPlexID()));
    }
  }
  
  return u;
}
