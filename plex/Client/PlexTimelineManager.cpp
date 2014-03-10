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
  m_timelines[PLEX_MEDIA_TYPE_MUSIC] = CPlexTimelinePtr(new CPlexTimeline(PLEX_MEDIA_TYPE_MUSIC));
  m_timelines[PLEX_MEDIA_TYPE_VIDEO] = CPlexTimelinePtr(new CPlexTimeline(PLEX_MEDIA_TYPE_VIDEO));
  m_timelines[PLEX_MEDIA_TYPE_PHOTO] = CPlexTimelinePtr(new CPlexTimeline(PLEX_MEDIA_TYPE_PHOTO));
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
void CPlexTimelineManager::SendCurrentTimelineToSubscriber(CPlexRemoteSubscriberPtr subscriber)
{
  SendTimelineToSubscriber(subscriber, GetCurrentTimeLines());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::SendTimelineToSubscriber(CPlexRemoteSubscriberPtr subscriber, const CPlexTimelineCollectionPtr &timelines)
{
  if (!subscriber)
    return;

  if (!subscriber->queueTimeline(timelines))
    g_plexApplication.remoteSubscriberManager->removeSubscriber(subscriber);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::SendTimelineToSubscribers(const CPlexTimelineCollectionPtr &timelines, bool delay)
{
  BOOST_FOREACH(CPlexRemoteSubscriberPtr sub, g_plexApplication.remoteSubscriberManager->getSubscribers())
    SendTimelineToSubscriber(sub, timelines);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::SetTextFieldFocused(bool focused, const CStdString &name, const CStdString &contents, bool isSecure)
{
  CSingleLock lk(m_timelineManagerLock);

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

  lk.unlock();

  SendTimelineToSubscribers(GetCurrentTimeLines(), true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::UpdateLocation()
{
  SendTimelineToSubscribers(GetCurrentTimeLines());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::Stop()
{
  m_stopped = true;
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

  CPlexTimelinePtr timeline = CPlexTimelinePtr(new CPlexTimeline(type));
  timeline->setItem(CFileItemPtr(new CFileItem(*newItem.get())));
  timeline->setState(state);
  timeline->setCurrentPosition(currentPosition);

  CSingleLock lk(m_timelineManagerLock);

  CPlexTimelinePtr oldTimeline = m_timelines[type];

  lk.unlock();

  //CLog::Log(LOGDEBUG, "PlexTimelineManager::ReportProgress reporting for %s, current is %s", newItem->GetLabel().c_str(), m_currentItems[type] ? m_currentItems[type]->GetLabel().c_str() : "none");

  if ((oldTimeline && !oldTimeline->getItem()) || timeline->getItem()->GetPath() != oldTimeline->getItem()->GetPath())
  {
    if (oldTimeline->getState() != PLEX_MEDIA_STATE_STOPPED && oldTimeline)
    {
      // we need to stop the old media before playing the new one.
      CLog::Log(LOGDEBUG, "CPlexTimelineManager::ReportProgress Old item was never stopped, sending stop timeline now.");
      oldTimeline->setContinuing(true);
      oldTimeline->setState(PLEX_MEDIA_STATE_STOPPED);

      lk.lock();
      m_timelines[type] = oldTimeline;
      lk.unlock();

      ReportProgress(oldTimeline, true);
    }
  }

  /* force if the caller has set the force flag, or if the
   * context has changed enough to warrant a direct timeline
   */
  bool reallyForce = force ? true : !(timeline->compare(*oldTimeline.get()));

  lk.lock();
  m_timelines[type] = timeline;
  lk.unlock();

  ReportProgress(timeline, reallyForce);

  if (timeline->getState() == PLEX_MEDIA_STATE_STOPPED)
  {
    lk.lock();
    m_timelines[type] = CPlexTimelinePtr(new CPlexTimeline(type));
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexTimelineManager::ReportProgress(const CPlexTimelinePtr &timeline, bool force)
{
  /* now se the correct item, since we might have copied it */
  CFileItemPtr currentItem = timeline->getItem();

  int64_t realPosition = timeline->getCurrentPosition();

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
              PlexUtils::GetMediaStateString(timeline->getState()).c_str(),
              currentItem->GetLabel().c_str(),
              realPosition,
              GetItemDuration(currentItem));

    SendTimelineToSubscribers(GetCurrentTimeLines());
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
              PlexUtils::GetMediaStateString(timeline->getState()).c_str(),
              currentItem->GetLabel().c_str(),
              realPosition,
              GetItemDuration(currentItem));
    g_plexApplication.mediaServerClient->SendServerTimeline(currentItem, timeline->getTimeline());

    /* now we can see if we need to ping the transcoder as well */
    if (timeline->getType() == PLEX_MEDIA_TYPE_VIDEO &&
        timeline->getState() == PLEX_MEDIA_STATE_PAUSED &&
        currentItem &&
        currentItem->GetProperty("plexDidTranscode").asBoolean() &&
        server)
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
bool CPlexTimelineManager::GetTextFieldInfo(CStdString& name, CStdString& contents, bool& secure)
{
  CSingleLock lk(m_timelineManagerLock);

  if (!m_textFieldFocused)
    return false;

  name = m_textFieldName;
  contents = m_textFieldContents;
  secure = m_textFieldSecure;

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexTimelineCollectionPtr CPlexTimelineManager::GetCurrentTimeLines()
{
  return CPlexTimelineCollectionPtr(new CPlexTimelineCollection(m_timelines));
}

