#include "PlexPlayQueueLocal.h"
#include "PlexJobs.h"
#include "JobManager.h"
#include "URL.h"
#include "ApplicationMessenger.h"
#include "PlexApplication.h"
#include "PlayListPlayer.h"
#include "music/tags/MusicInfoTag.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexPlayQueueLocal::CPlexPlayQueueLocal(const CPlexServerPtr& server) : m_server(server)
{
  m_list = CFileItemListPtr(new CFileItemList);
  m_list->SetFastLookup(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueLocal::create(const CFileItem& container, const CStdString& uri,
                                 const CPlexPlayQueueOptions& options)
{
  CURL containerURL(container.GetPath());
  if (container.GetProperty("isSynthesized").asBoolean())
    containerURL = CURL(container.GetProperty("containerPath").asString());

  return g_plexApplication.busy.blockWaitingForJob(new CPlexPlayQueueFetchJob(containerURL, options), this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueLocal::removeItem(const CFileItemPtr& item)
{
  ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(item);
  if (m_list)
  {
    for (int i = 0; i < m_list->Size(); i++)
    {
      if (m_list->Get(i)->GetProperty("playQueueItemID").asString() ==
          item->GetProperty("playQueueItemID").asString())
      {
        m_list->Remove(m_list->Get(i).get());
        OnPlayQueueUpdated(type, false);
        return;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueLocal::addItem(const CFileItemPtr& item, bool next)
{
  ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(item);

  if (m_list)
  {
    if (next)
    {
      int currentSong = 0;
      if (CPlexPlayQueueManager::getPlaylistFromType(type) == g_playlistPlayer.GetCurrentPlaylist())
      {
        if (g_playlistPlayer.GetCurrentSong() != -1)
          currentSong = g_playlistPlayer.GetCurrentSong() + 1;
      }
      m_list->AddFront(item, currentSong);
    }
    else
    {
      m_list->Add(item);
    }
    OnPlayQueueUpdated(type, false);
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CPlexPlayQueueLocal::getCurrentID()
{
  if (m_list)
    return m_list->GetProperty("playQueueID").asInteger();
  return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueLocal::get(const CStdString& playQueueID, const CPlexPlayQueueOptions &options)
{
  if (m_list && m_list->GetProperty("playQueueID").asString() == playQueueID)
    OnPlayQueueUpdated(PlexUtils::GetMediaTypeFromItem(m_list), options.startPlaying);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueLocal::refreshCurrent()
{
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueLocal::getCurrent(CFileItemList& list)
{
  if (m_list)
  {
    list.Copy(*m_list);
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueLocal::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CPlexPlayQueueFetchJob* fj = static_cast<CPlexPlayQueueFetchJob*>(job);
  if (success && fj)
  {
    ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(fj->m_items);
    if (type == PLEX_MEDIA_TYPE_UNKNOWN)
      return;

    m_list->Clear();
    m_list->Copy(fj->m_items);
    m_list->SetPath("plexserver://playqueue/");

    /* If we need to shuffle the list do it here */
    if (fj->m_options.shuffle)
      m_list->Randomize();

    if (!fj->m_options.showPrompts && m_list->Get(0))
    {
      m_list->Get(0)->SetProperty("avoidPrompts", true);
      PlexUtils::SetItemResumeOffset(m_list->Get(0), fj->m_options.resumeOffset);
    }

    if (!fj->m_options.startItemKey.empty())
    {
      CFileItemPtr item = PlexUtils::GetItemWithKey(*m_list, fj->m_options.startItemKey);
      if (item && item->HasMusicInfoTag())
        m_list->SetProperty("playQueueSelectedItemID", item->GetMusicInfoTag()->GetDatabaseId());
    }

    if (m_list->HasProperty("ratingKey"))
      m_list->SetProperty("playQueueID", m_list->GetProperty("ratingKey"));
    else
      m_list->SetProperty("playQueueID", rand());

    m_list->SetProperty("playQueueIsLocal", true);

    OnPlayQueueUpdated(type, fj->m_options.startPlaying);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueLocal::OnPlayQueueUpdated(ePlexMediaType type, bool startPlaying)
{
  m_list->SetProperty("size", m_list->Size());
  CApplicationMessenger::Get().PlexUpdatePlayQueue(type, startPlaying);
}
