/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryProvider.h"

#include "ContextMenuManager.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/gui/GUIDialogAddonInfo.h"
#include "favourites/FavouritesService.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "music/MusicThumbLoader.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "pictures/PictureThumbLoader.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRThumbLoader.h"
#include "pvr/guilib/PVRGUIActions.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JobManager.h"
#include "utils/SortUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"
#include "video/VideoThumbLoader.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoBase.h"

#include <memory>
#include <mutex>
#include <utility>

using namespace XFILE;
using namespace KODI::MESSAGING;
using namespace PVR;

class CDirectoryJob : public CJob
{
public:
  CDirectoryJob(const std::string &url, SortDescription sort, int limit, int parentID)
    : m_url(url),
      m_sort(sort),
      m_limit(limit),
      m_parentID(parentID)
  { }
  ~CDirectoryJob() override = default;

  const char* GetType() const override { return "directory"; }
  bool operator==(const CJob *job) const override
  {
    if (strcmp(job->GetType(),GetType()) == 0)
    {
      const CDirectoryJob* dirJob = dynamic_cast<const CDirectoryJob*>(job);
      if (dirJob && dirJob->m_url == m_url)
        return true;
    }
    return false;
  }

  bool DoWork() override
  {
    CFileItemList items;
    if (CDirectory::GetDirectory(m_url, items, "", DIR_FLAG_DEFAULTS))
    {
      // sort the items if necessary
      if (m_sort.sortBy != SortByNone)
        items.Sort(m_sort);

      // limit must not exceed the number of items
      int limit = (m_limit == 0) ? items.Size() : std::min((int) m_limit, items.Size());
      // convert to CGUIStaticItem's and set visibility and targets
      m_items.reserve(limit);
      for (int i = 0; i < limit; i++)
      {
        CGUIStaticItemPtr item(new CGUIStaticItem(*items[i]));
        if (item->HasProperty("node.visible"))
          item->SetVisibleCondition(item->GetProperty("node.visible").asString(), m_parentID);

        getThumbLoader(item)->LoadItem(item.get());

        m_items.push_back(item);
      }
      m_target = items.GetProperty("node.target").asString();
    }
    return true;
  }

  std::shared_ptr<CThumbLoader> getThumbLoader(const CGUIStaticItemPtr& item)
  {
    if (item->IsVideo())
    {
      initThumbLoader<CVideoThumbLoader>(InfoTagType::VIDEO);
      return m_thumbloaders[InfoTagType::VIDEO];
    }
    if (item->IsAudio())
    {
      initThumbLoader<CMusicThumbLoader>(InfoTagType::AUDIO);
      return m_thumbloaders[InfoTagType::AUDIO];
    }
    if (item->IsPicture())
    {
      initThumbLoader<CPictureThumbLoader>(InfoTagType::PICTURE);
      return m_thumbloaders[InfoTagType::PICTURE];
    }
    if (item->IsPVRChannelGroup())
    {
      initThumbLoader<CPVRThumbLoader>(InfoTagType::PVR);
      return m_thumbloaders[InfoTagType::PVR];
    }
    initThumbLoader<CProgramThumbLoader>(InfoTagType::PROGRAM);
    return m_thumbloaders[InfoTagType::PROGRAM];
  }

  template<class CThumbLoaderClass>
  void initThumbLoader(InfoTagType type)
  {
    if (!m_thumbloaders.count(type))
    {
      std::shared_ptr<CThumbLoader> thumbLoader = std::make_shared<CThumbLoaderClass>();
      thumbLoader->OnLoaderStart();
      m_thumbloaders.insert(make_pair(type, thumbLoader));
    }
  }

  const std::vector<CGUIStaticItemPtr> &GetItems() const { return m_items; }
  const std::string &GetTarget() const { return m_target; }
  std::vector<InfoTagType> GetItemTypes(std::vector<InfoTagType> &itemTypes) const
  {
    itemTypes.clear();
    for (const auto& i : m_thumbloaders)
      itemTypes.push_back(i.first);
    return itemTypes;
  }
private:
  std::string m_url;
  std::string m_target;
  SortDescription m_sort;
  unsigned int m_limit;
  int m_parentID;
  std::vector<CGUIStaticItemPtr> m_items;
  std::map<InfoTagType, std::shared_ptr<CThumbLoader> > m_thumbloaders;
};

CDirectoryProvider::CDirectoryProvider(const TiXmlElement *element, int parentID)
 : IListProvider(parentID),
   m_updateState(OK),
   m_isAnnounced(false),
   m_jobID(0),
   m_currentLimit(0)
{
  assert(element);
  if (!element->NoChildren())
  {
    const char *target = element->Attribute("target");
    if (target)
      m_target.SetLabel(target, "", parentID);

    const char *sortMethod = element->Attribute("sortby");
    if (sortMethod)
      m_sortMethod.SetLabel(sortMethod, "", parentID);

    const char *sortOrder = element->Attribute("sortorder");
    if (sortOrder)
      m_sortOrder.SetLabel(sortOrder, "", parentID);

    const char *limit = element->Attribute("limit");
    if (limit)
      m_limit.SetLabel(limit, "", parentID);

    m_url.SetLabel(element->FirstChild()->ValueStr(), "", parentID);
  }
}

CDirectoryProvider::CDirectoryProvider(const CDirectoryProvider& other)
  : IListProvider(other.m_parentID),
    m_updateState(INVALIDATED),
    m_isAnnounced(false),
    m_jobID(0),
    m_url(other.m_url),
    m_target(other.m_target),
    m_sortMethod(other.m_sortMethod),
    m_sortOrder(other.m_sortOrder),
    m_limit(other.m_limit),
    m_currentUrl(other.m_currentUrl),
    m_currentTarget(other.m_currentTarget),
    m_currentSort(other.m_currentSort),
    m_currentLimit(other.m_currentLimit)
{
}

CDirectoryProvider::~CDirectoryProvider()
{
  Reset();
}

std::unique_ptr<IListProvider> CDirectoryProvider::Clone()
{
  return std::make_unique<CDirectoryProvider>(*this);
}

bool CDirectoryProvider::Update(bool forceRefresh)
{
  // we never need to force refresh here
  bool changed = false;
  bool fireJob = false;

  // update the URL & limit and fire off a new job if needed
  fireJob |= UpdateURL();
  fireJob |= UpdateSort();
  fireJob |= UpdateLimit();
  fireJob &= !m_currentUrl.empty();

  std::unique_lock<CCriticalSection> lock(m_section);
  if (m_updateState == INVALIDATED)
    fireJob = true;
  else if (m_updateState == DONE)
    changed = true;

  m_updateState = OK;

  if (fireJob)
  {
    CLog::Log(LOGDEBUG, "CDirectoryProvider[{}]: refreshing..", m_currentUrl);
    if (m_jobID)
      CServiceBroker::GetJobManager()->CancelJob(m_jobID);
    m_jobID = CServiceBroker::GetJobManager()->AddJob(
        new CDirectoryJob(m_currentUrl, m_currentSort, m_currentLimit, m_parentID), this);
  }

  if (!changed)
  {
    for (auto& i : m_items)
      changed |= i->UpdateVisibility(m_parentID);
  }
  return changed; //! @todo Also returned changed if properties are changed (if so, need to update scroll to letter).
}

void CDirectoryProvider::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                  const std::string& sender,
                                  const std::string& message,
                                  const CVariant& data)
{
  // we are only interested in library, player and GUI changes
  if ((flag & (ANNOUNCEMENT::VideoLibrary | ANNOUNCEMENT::AudioLibrary | ANNOUNCEMENT::Player | ANNOUNCEMENT::GUI)) == 0)
    return;

  {
    std::unique_lock<CCriticalSection> lock(m_section);
    // we don't need to refresh anything if there are no fitting
    // items in this list provider for the announcement flag
    if (((flag & ANNOUNCEMENT::VideoLibrary) &&
         (std::find(m_itemTypes.begin(), m_itemTypes.end(), InfoTagType::VIDEO) == m_itemTypes.end())) ||
        ((flag & ANNOUNCEMENT::AudioLibrary) &&
         (std::find(m_itemTypes.begin(), m_itemTypes.end(), InfoTagType::AUDIO) == m_itemTypes.end())))
      return;

    if (flag & ANNOUNCEMENT::Player)
    {
      if (message == "OnPlay" || message == "OnResume" || message == "OnStop")
      {
        if (m_currentSort.sortBy == SortByNone || // not nice, but many directories that need to be refreshed on start/stop have no special sort order (e.g. in progress movies)
            m_currentSort.sortBy == SortByLastPlayed ||
            m_currentSort.sortBy == SortByPlaycount ||
            m_currentSort.sortBy == SortByLastUsed)
          m_updateState = INVALIDATED;
      }
    }
    else
    {
      // if we're in a database transaction, don't bother doing anything just yet
      if (data.isMember("transaction") && data["transaction"].asBoolean())
        return;

      // if there was a database update, we set the update state
      // to PENDING to fire off a new job in the next update
      if (message == "OnScanFinished" || message == "OnCleanFinished" || message == "OnUpdate" ||
          message == "OnRemove" || message == "OnRefresh")
        m_updateState = INVALIDATED;
    }
  }
}

void CDirectoryProvider::Fetch(std::vector<CGUIListItemPtr> &items)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  items.clear();
  for (const auto& i : m_items)
  {
    if (i->IsVisible())
      items.push_back(i);
  }
}

void CDirectoryProvider::OnAddonEvent(const ADDON::AddonEvent& event)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (URIUtils::IsProtocol(m_currentUrl, "addons"))
  {
    if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
        typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
        typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
        typeid(event) == typeid(ADDON::AddonEvents::UnInstalled) ||
        typeid(event) == typeid(ADDON::AddonEvents::MetadataChanged) ||
        typeid(event) == typeid(ADDON::AddonEvents::AutoUpdateStateChanged))
      m_updateState = INVALIDATED;
  }
}

void CDirectoryProvider::OnAddonRepositoryEvent(const ADDON::CRepositoryUpdater::RepositoryUpdated& event)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (URIUtils::IsProtocol(m_currentUrl, "addons"))
  {
    m_updateState = INVALIDATED;
  }
}

void CDirectoryProvider::OnPVRManagerEvent(const PVR::PVREvent& event)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (URIUtils::IsProtocol(m_currentUrl, "pvr"))
  {
    if (event == PVR::PVREvent::ManagerStarted || event == PVR::PVREvent::ManagerStopped ||
        event == PVR::PVREvent::ManagerError || event == PVR::PVREvent::ManagerInterrupted ||
        event == PVR::PVREvent::RecordingsInvalidated ||
        event == PVR::PVREvent::TimersInvalidated ||
        event == PVR::PVREvent::ChannelGroupsInvalidated ||
        event == PVR::PVREvent::SavedSearchesInvalidated)
      m_updateState = INVALIDATED;
  }
}

void CDirectoryProvider::OnFavouritesEvent(const CFavouritesService::FavouritesUpdated& event)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (URIUtils::IsProtocol(m_currentUrl, "favourites"))
    m_updateState = INVALIDATED;
}

void CDirectoryProvider::Reset()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (m_jobID)
    CServiceBroker::GetJobManager()->CancelJob(m_jobID);
  m_jobID = 0;
  m_items.clear();
  m_currentTarget.clear();
  m_currentUrl.clear();
  m_itemTypes.clear();
  m_currentSort.sortBy = SortByNone;
  m_currentSort.sortOrder = SortOrderAscending;
  m_currentLimit = 0;
  m_updateState = OK;

  if (m_isAnnounced)
  {
    m_isAnnounced = false;
    CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
    CServiceBroker::GetFavouritesService().Events().Unsubscribe(this);
    CServiceBroker::GetRepositoryUpdater().Events().Unsubscribe(this);
    CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
    CServiceBroker::GetPVRManager().Events().Unsubscribe(this);
  }
}

void CDirectoryProvider::FreeResources(bool immediately)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  for (const auto& item : m_items)
    item->FreeMemory(immediately);
}

void CDirectoryProvider::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  std::unique_lock<CCriticalSection> lock(m_section);
  if (success)
  {
    m_items = static_cast<CDirectoryJob*>(job)->GetItems();
    m_currentTarget = static_cast<CDirectoryJob*>(job)->GetTarget();
    static_cast<CDirectoryJob*>(job)->GetItemTypes(m_itemTypes);
    if (m_updateState == OK)
      m_updateState = DONE;
  }
  m_jobID = 0;
}

std::string CDirectoryProvider::GetTarget(const CFileItem& item) const
{
  std::string target = item.GetProperty("node.target").asString();

  std::unique_lock<CCriticalSection> lock(m_section);
  if (target.empty())
    target = m_currentTarget;
  if (target.empty())
    target = m_target.GetLabel(m_parentID, false);

  return target;
}

bool CDirectoryProvider::OnClick(const CGUIListItemPtr &item)
{
  CFileItem fileItem(*std::static_pointer_cast<CFileItem>(item));

  if (fileItem.HasVideoInfoTag()
      && CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_MYVIDEOS_SELECTACTION) == SELECT_ACTION_INFO
      && OnInfo(item))
    return true;

  if (fileItem.HasProperty("node.target_url"))
    fileItem.SetPath(fileItem.GetProperty("node.target_url").asString());

  // grab the execute string
  std::string execute = CServiceBroker::GetFavouritesService().GetExecutePath(fileItem, GetTarget(fileItem));
  if (!execute.empty())
  {
    CGUIMessage message(GUI_MSG_EXECUTE, 0, 0);
    message.SetStringParam(execute);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
    return true;
  }
  return false;
}

bool CDirectoryProvider::OnInfo(const CGUIListItemPtr& item)
{
  auto fileItem = std::static_pointer_cast<CFileItem>(item);

  if (fileItem->HasAddonInfo())
  {
    return CGUIDialogAddonInfo::ShowForItem(fileItem);
  }
  else if (fileItem->IsPVR())
  {
    return CServiceBroker::GetPVRManager().GUIActions()->OnInfo(fileItem);
  }
  else if (fileItem->HasVideoInfoTag())
  {
    auto mediaType = fileItem->GetVideoInfoTag()->m_type;
    if (mediaType == MediaTypeMovie ||
        mediaType == MediaTypeTvShow ||
        mediaType == MediaTypeEpisode ||
        mediaType == MediaTypeVideo ||
        mediaType == MediaTypeMusicVideo)
    {
      CGUIDialogVideoInfo::ShowFor(*fileItem);
      return true;
    }
  }
  else if (fileItem->HasMusicInfoTag())
  {
    CGUIDialogMusicInfo::ShowFor(fileItem.get());
    return true;
  }
  return false;
}

bool CDirectoryProvider::OnContextMenu(const CGUIListItemPtr& item)
{
  auto fileItem = std::static_pointer_cast<CFileItem>(item);

  const std::string target = GetTarget(*fileItem);
  if (!target.empty())
    fileItem->SetProperty("targetwindow", target);

  return CONTEXTMENU::ShowFor(fileItem);
}

bool CDirectoryProvider::IsUpdating() const
{
  std::unique_lock<CCriticalSection> lock(m_section);
  return m_jobID || m_updateState == DONE || m_updateState == INVALIDATED;
}

bool CDirectoryProvider::UpdateURL()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  std::string value(m_url.GetLabel(m_parentID, false));
  if (value == m_currentUrl)
    return false;

  m_currentUrl = value;

  if (!m_isAnnounced)
  {
    m_isAnnounced = true;
    CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
    CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CDirectoryProvider::OnAddonEvent);
    CServiceBroker::GetRepositoryUpdater().Events().Subscribe(this, &CDirectoryProvider::OnAddonRepositoryEvent);
    CServiceBroker::GetPVRManager().Events().Subscribe(this, &CDirectoryProvider::OnPVRManagerEvent);
    CServiceBroker::GetFavouritesService().Events().Subscribe(this, &CDirectoryProvider::OnFavouritesEvent);
  }
  return true;
}

bool CDirectoryProvider::UpdateLimit()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  unsigned int value = m_limit.GetIntValue(m_parentID);
  if (value == m_currentLimit)
    return false;

  m_currentLimit = value;

  return true;
}

bool CDirectoryProvider::UpdateSort()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  SortBy sortMethod(SortUtils::SortMethodFromString(m_sortMethod.GetLabel(m_parentID, false)));
  SortOrder sortOrder(SortUtils::SortOrderFromString(m_sortOrder.GetLabel(m_parentID, false)));
  if (sortOrder == SortOrderNone)
    sortOrder = SortOrderAscending;

  if (sortMethod == m_currentSort.sortBy && sortOrder == m_currentSort.sortOrder)
    return false;

  m_currentSort.sortBy = sortMethod;
  m_currentSort.sortOrder = sortOrder;
  m_currentSort.sortAttributes = SortAttributeIgnoreFolders;

  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    m_currentSort.sortAttributes = static_cast<SortAttribute>(m_currentSort.sortAttributes | SortAttributeIgnoreArticle);

  return true;
}
