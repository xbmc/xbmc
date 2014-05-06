#include "PlexPlayQueueLocal.h"
#include "PlexJobs.h"
#include "JobManager.h"
#include "URL.h"
#include "ApplicationMessenger.h"
#include "PlexApplication.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexPlayQueueLocal::CPlexPlayQueueLocal(const CPlexServerPtr& server) : m_server(server)
{
}

void CPlexPlayQueueLocal::create(const CFileItem& container, const CStdString& uri,
                                 const CStdString& startItemKey, bool shuffle)
{
  CURL containerURL(container.GetPath());
  g_plexApplication.busy.blockWaitingForJob(new CPlexPlayQueueFetchJob(containerURL, true, shuffle,
                                                                       startItemKey), this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexPlayQueueLocal::removeItem(const CFileItemPtr& item)
{
  ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(item);
  if (m_list)
  {
    m_list->Remove(item.get());
    CApplicationMessenger::Get().PlexUpdatePlayQueue(type, false);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexPlayQueueLocal::addItem(const CFileItemPtr& item)
{
  ePlexMediaType type = PlexUtils::GetMediaTypeFromItem(item);

  if (m_list)
  {
    m_list->Add(item);
    CApplicationMessenger::Get().PlexUpdatePlayQueue(type, false);
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
void CPlexPlayQueueLocal::get(const CStdString& playQueueID, bool startPlay)
{
  if (m_list && m_list->GetProperty("playQueueID").asString() == playQueueID)
    CApplicationMessenger::Get().PlexUpdatePlayQueue(PlexUtils::GetMediaTypeFromItem(m_list),
                                                     false);
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

    CFileItemListPtr list = CFileItemListPtr(new CFileItemList);
    list->Assign(fj->m_items);

    /* If we need to shuffle the list do it here */
    if (fj->m_shuffle)
      list->Randomize();

    if (!fj->m_startItem.empty())
    {
      int startOffset = list->IndexOfItem(fj->m_startItem);
      if (startOffset != -1)
        list->SetProperty("playQueueSelectedItemOffset", startOffset);
    }

    list->SetProperty("playQueueID", list->GetProperty("ratingKey"));
    m_list = list;
    CApplicationMessenger::Get().PlexUpdatePlayQueue(type, fj->m_startPlaying);
  }
}
