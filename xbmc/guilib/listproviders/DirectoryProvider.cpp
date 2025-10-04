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
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/RepositoryUpdater.h"
#include "favourites/FavouritesService.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/AnnouncementManager.h"
#include "interfaces/IAnnouncer.h"
#include "jobs/JobManager.h"
#include "music/MusicFileItemClassify.h"
#include "music/MusicThumbLoader.h"
#include "pictures/PictureThumbLoader.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRThumbLoader.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ExecString.h"
#include "utils/PlayerUtils.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "utils/guilib/GUIBuiltinsUtils.h"
#include "utils/guilib/GUIContentUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"
#include "video/VideoThumbLoader.h"
#include "video/guilib/VideoGUIUtils.h"
#include "video/guilib/VideoPlayActionProcessor.h"
#include "video/guilib/VideoSelectActionProcessor.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <utility>

using namespace XFILE;
using namespace KODI;
using namespace KODI::MESSAGING;
using namespace KODI::UTILS::GUILIB;
using namespace PVR;

class CDirectoryProvider::CSubscriber : public ANNOUNCEMENT::IAnnouncer
{
public:
  explicit CSubscriber(ISubscriberCallback& invalidate) : m_callback(invalidate)
  {
    CServiceBroker::GetAnnouncementManager()->AddAnnouncer(
        this, ANNOUNCEMENT::VideoLibrary | ANNOUNCEMENT::AudioLibrary | ANNOUNCEMENT::Player |
                  ANNOUNCEMENT::GUI);
  }
  ~CSubscriber() override { CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this); }

  virtual bool IsReadyToUse() const { return true; }

protected:
  bool OnEventPublished(Topic topic = Topic::UNSPECIFIED)
  {
    return m_callback.OnEventPublished(topic);
  }

private:
  CSubscriber() = delete;

  // IAnnouncer implementation
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override
  {
    if (flag & ANNOUNCEMENT::VideoLibrary && OnEventPublished(Topic::VIDEO_LIBRARY))
      return;

    if (flag & ANNOUNCEMENT::AudioLibrary && OnEventPublished(Topic::AUDIO_LIBRARY))
      return;

    if (flag & ANNOUNCEMENT::Player)
    {
      if (message == "OnPlay" || message == "OnResume" || message == "OnStop")
        OnEventPublished(Topic::PLAYER);
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
        OnEventPublished();
    }
  }

  CCriticalSection m_critSection;
  ISubscriberCallback& m_callback;
};

namespace
{
class CAddonsSubscriber : public CDirectoryProvider::CSubscriber
{
public:
  explicit CAddonsSubscriber(ISubscriberCallback& invalidate)
    : CDirectoryProvider::CSubscriber(invalidate)
  {
    CServiceBroker::GetAddonMgr().Events().Subscribe(
        this,
        [this](const ADDON::AddonEvent& event)
        {
          if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
              typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
              typeid(event) == typeid(ADDON::AddonEvents::ReInstalled) ||
              typeid(event) == typeid(ADDON::AddonEvents::UnInstalled) ||
              typeid(event) == typeid(ADDON::AddonEvents::MetadataChanged) ||
              typeid(event) == typeid(ADDON::AddonEvents::AutoUpdateStateChanged))
          {
            OnEventPublished();
          }
        });

    CServiceBroker::GetRepositoryUpdater().Events().Subscribe(
        this, [this](const ADDON::CRepositoryUpdater::RepositoryUpdated& /*event*/)
        { OnEventPublished(); });
  }
  ~CAddonsSubscriber() override
  {
    CServiceBroker::GetRepositoryUpdater().Events().Unsubscribe(this);
    CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
  }
};

class CPVRSubscriber : public CDirectoryProvider::CSubscriber
{
public:
  explicit CPVRSubscriber(ISubscriberCallback& invalidate)
    : CDirectoryProvider::CSubscriber(invalidate)
  {
    CServiceBroker::GetPVRManager().Events().Subscribe(
        this,
        [this](const PVR::PVREvent& event)
        {
          if (event == PVR::PVREvent::ManagerStarted)
            m_pvrStarted.test_and_set();
          else if (event == PVR::PVREvent::ManagerStarting ||
                   event == PVR::PVREvent::ManagerStopping ||
                   event == PVR::PVREvent::ManagerStopped)
            m_pvrStarted.clear();

          if (!m_pvrStarted.test())
            return;

          using enum PVR::PVREvent;
          if (event == ManagerStarted || event == ManagerStopped ||
              event == RecordingsInvalidated || event == TimersInvalidated ||
              event == ChannelGroupsInvalidated || event == SavedSearchesInvalidated ||
              event == ClientsInvalidated || event == ClientsPrioritiesInvalidated)
          {
            OnEventPublished();
          }
        });

    if (CServiceBroker::GetPVRManager().IsStarted())
      m_pvrStarted.test_and_set();
  }
  ~CPVRSubscriber() override { CServiceBroker::GetPVRManager().Events().Unsubscribe(this); }

  bool IsReadyToUse() const override { return m_pvrStarted.test(); }

private:
  std::atomic_flag m_pvrStarted{};
};

class CFavouritesSubscriber : public CDirectoryProvider::CSubscriber
{
public:
  explicit CFavouritesSubscriber(ISubscriberCallback& invalidate)
    : CDirectoryProvider::CSubscriber(invalidate)
  {
    CServiceBroker::GetFavouritesService().Events().Subscribe(
        this,
        [this](const CFavouritesService::FavouritesUpdated& /*event*/) { OnEventPublished(); });
  }
  ~CFavouritesSubscriber() override
  {
    CServiceBroker::GetFavouritesService().Events().Unsubscribe(this);
  }
};

std::unique_ptr<CDirectoryProvider::CSubscriber> GetSubscriber(const std::string& url,
                                                               ISubscriberCallback& invalidate)
{
  if (URIUtils::IsProtocol(url, "addons"))
    return std::make_unique<CAddonsSubscriber>(invalidate);
  else if (URIUtils::IsProtocol(url, "pvr"))
    return std::make_unique<CPVRSubscriber>(invalidate);
  else if (URIUtils::IsProtocol(url, "favourites"))
    return std::make_unique<CFavouritesSubscriber>(invalidate);
  else
    return std::make_unique<CDirectoryProvider::CSubscriber>(invalidate);
}

class CDirectoryJob : public CJob
{
public:
  CDirectoryJob(const std::string& url,
                const std::string& target,
                SortDescription sort,
                int limit,
                CDirectoryProvider::BrowseMode browse,
                int parentID)
    : m_url(url),
      m_target(target),
      m_sort(sort),
      m_limit(limit),
      m_browse(browse),
      m_parentID(parentID)
  {
  }
  ~CDirectoryJob() override = default;

  const char* GetType() const override { return "directory"; }
  bool Equals(const CJob* job) const override
  {
    if (strcmp(job->GetType(), GetType()) == 0)
    {
      const auto* dirJob = dynamic_cast<const CDirectoryJob*>(job);
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
      int limit = (m_limit == 0) ? items.Size() : std::min(static_cast<int>(m_limit), items.Size());
      if (limit < items.Size())
        m_items.reserve(limit + 1);
      else
        m_items.reserve(limit);

      for (int i = 0; i < limit; i++)
      {
        auto item{std::make_shared<CGUIStaticItem>(*items[i])};
        if (item->HasProperty("node.visible"))
          item->SetVisibleCondition(item->GetProperty("node.visible").asString(), m_parentID);

        GetThumbLoader(item)->LoadItem(item.get());

        m_items.emplace_back(std::move(item));
      }

      if (items.HasProperty("node.target"))
        m_target = items.GetProperty("node.target").asString();

      if ((m_browse == CDirectoryProvider::BrowseMode::ALWAYS && !items.IsEmpty()) ||
          (m_browse == CDirectoryProvider::BrowseMode::AUTO && limit < items.Size()))
      {
        // Add a special item to the end of the list, which can be used to open the
        // full listing containing all items in the given target window.
        if (!m_target.empty())
        {
          CFileItem item(m_url, true);
          item.SetLabel(g_localizeStrings.Get(22082)); // More...
          item.SetArt("icon", "DefaultFolder.png");
          item.SetProperty("node.target", m_target);
          item.SetProperty("node.type", "target_folder"); // make item identifiable, e.g. by skins

          m_items.emplace_back(std::make_shared<CGUIStaticItem>(item));
        }
        else
          CLog::LogF(LOGWARNING, "Cannot add 'More...' item to list. No target window given.");
      }
    }
    return true;
  }

  std::shared_ptr<CThumbLoader> GetThumbLoader(const std::shared_ptr<CGUIStaticItem>& item)
  {
    using enum InfoTagType;

    if (VIDEO::IsVideo(*item))
    {
      InitThumbLoader<CVideoThumbLoader>(VIDEO);
      return m_thumbloaders[InfoTagType::VIDEO];
    }
    if (MUSIC::IsAudio(*item))
    {
      InitThumbLoader<CMusicThumbLoader>(AUDIO);
      return m_thumbloaders[InfoTagType::AUDIO];
    }
    if (item->IsPicture())
    {
      InitThumbLoader<CPictureThumbLoader>(PICTURE);
      return m_thumbloaders[InfoTagType::PICTURE];
    }
    if (item->IsPVRChannelGroup())
    {
      InitThumbLoader<CPVRThumbLoader>(PVR);
      return m_thumbloaders[PVR];
    }
    InitThumbLoader<CProgramThumbLoader>(PROGRAM);
    return m_thumbloaders[PROGRAM];
  }

  template<class CThumbLoaderClass>
  void InitThumbLoader(InfoTagType type)
  {
    if (!m_thumbloaders.contains(type))
    {
      auto thumbLoader{std::make_shared<CThumbLoaderClass>()};
      thumbLoader->OnLoaderStart();
      m_thumbloaders.insert({type, std::move(thumbLoader)});
    }
  }

  const std::vector<CGUIStaticItemPtr>& GetItems() const { return m_items; }
  const std::string& GetTarget() const { return m_target; }
  std::vector<InfoTagType> GetItemTypes(std::vector<InfoTagType>& itemTypes) const
  {
    itemTypes.clear();
    for (const auto& [type, _] : m_thumbloaders)
      itemTypes.emplace_back(type);
    return itemTypes;
  }

private:
  std::string m_url;
  std::string m_target;
  SortDescription m_sort;
  unsigned int m_limit{10};
  CDirectoryProvider::BrowseMode m_browse{CDirectoryProvider::BrowseMode::AUTO};
  int m_parentID;
  std::vector<CGUIStaticItemPtr> m_items;
  std::map<InfoTagType, std::shared_ptr<CThumbLoader>> m_thumbloaders;
};
} // unnamed namespace

CDirectoryProvider::CDirectoryProvider(const TiXmlElement* element, int parentID)
  : IListProvider(parentID),
    m_nextJobTimer(this)
{
  assert(element);
  if (!element->NoChildren())
  {
    const char* target = element->Attribute("target");
    if (target)
      m_target.SetLabel(target, "", parentID);

    const char* sortMethod = element->Attribute("sortby");
    if (sortMethod)
      m_sortMethod.SetLabel(sortMethod, "", parentID);

    const char* sortOrder = element->Attribute("sortorder");
    if (sortOrder)
      m_sortOrder.SetLabel(sortOrder, "", parentID);

    const char* limit = element->Attribute("limit");
    if (limit)
      m_limit.SetLabel(limit, "", parentID);

    const char* browse = element->Attribute("browse");
    if (browse)
      m_browse.SetLabel(browse, "", parentID);

    m_url.SetLabel(element->FirstChild()->ValueStr(), "", parentID);
  }
}

CDirectoryProvider::CDirectoryProvider(const CDirectoryProvider& other)
  : IListProvider(other),
    m_updateState(UpdateState::INVALIDATED),
    m_nextJobTimer(this),
    m_url(other.m_url),
    m_target(other.m_target),
    m_sortMethod(other.m_sortMethod),
    m_sortOrder(other.m_sortOrder),
    m_limit(other.m_limit),
    m_browse(other.m_browse),
    m_currentUrl(other.m_currentUrl),
    m_currentTarget(other.m_currentTarget),
    m_currentSort(other.m_currentSort),
    m_currentLimit(other.m_currentLimit),
    m_currentBrowse(other.m_currentBrowse)
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

void CDirectoryProvider::StartDirectoryJob()
{
  std::unique_lock lock(m_section);
  m_jobPending = false;
  m_lastJobStartedAt = std::chrono::system_clock::now();
  m_nextJobTimer.Stop();

  CLog::Log(LOGDEBUG, "CDirectoryProvider[{}]: refreshing...", m_currentUrl);
  m_jobID = CServiceBroker::GetJobManager()->AddJob(
      new CDirectoryJob(m_currentUrl, m_target.GetLabel(GetParentId(), false), m_currentSort,
                        m_currentLimit, m_currentBrowse, GetParentId()),
      this);
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
  fireJob |= UpdateBrowse();
  fireJob &= !m_currentUrl.empty();

  std::unique_lock lock(m_section);
  if (m_updateState == UpdateState::INVALIDATED)
    fireJob = true;
  else if (m_updateState == UpdateState::DONE)
    changed = true;

  m_updateState = UpdateState::OK;

  if (fireJob)
  {
    if (m_jobID)
    {
      // Ignore update request for now.
      // We will start another update job once the currently running has finished.
      m_jobPending = true;
      changed = false;
    }
    else
    {
      if (m_jobPending)
      {
        // Ignore the update request.
        // We already have scheduled another update job.
        changed = false;
      }
      else
      {
        // Start a new update job.
        StartDirectoryJob();
      }
    }
  }

  if (!changed)
  {
    for (const auto& i : m_items)
      changed |= i->UpdateVisibility(GetParentId());
  }
  return changed; //! @todo Also returned changed if properties are changed (if so, need to update scroll to letter).
}

void CDirectoryProvider::Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items)
{
  std::unique_lock lock(m_section);
  items.clear();
  for (const auto& i : m_items)
  {
    if (i->IsVisible())
      items.push_back(i);
  }
}

void CDirectoryProvider::Reset()
{
  {
    std::unique_lock lock(m_section);
    if (m_jobID)
      CServiceBroker::GetJobManager()->CancelJob(m_jobID);
    m_jobID = 0;
    m_jobPending = false;
    m_lastJobStartedAt = {};
    m_nextJobTimer.Stop();
    m_items.clear();
    m_currentTarget.clear();
    m_currentUrl.clear();
    m_itemTypes.clear();
    m_currentSort.sortBy = SortByNone;
    m_currentSort.sortOrder = SortOrderAscending;
    m_currentLimit = 0;
    m_currentBrowse = BrowseMode::AUTO;
    m_updateState = UpdateState::OK;
  }

  {
    std::unique_lock subscriptionLock(m_subscriptionSection);
    m_subscriber.reset();
  }
}

void CDirectoryProvider::FreeResources(bool immediately)
{
  std::unique_lock lock(m_section);
  for (const auto& item : m_items)
    item->FreeMemory(immediately);
}

void CDirectoryProvider::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  std::unique_lock lock(m_section);
  if (success)
  {
    m_items = static_cast<CDirectoryJob*>(job)->GetItems();
    m_currentTarget = static_cast<CDirectoryJob*>(job)->GetTarget();
    static_cast<CDirectoryJob*>(job)->GetItemTypes(m_itemTypes);
    if (m_updateState == UpdateState::OK)
      m_updateState = UpdateState::DONE;
  }
  m_jobID = 0;

  if (m_jobPending)
  {
    // Handle delayed update request(s).

    using namespace std::chrono_literals;
    static constexpr auto JOB_RATE_LIMIT = 1s;
    const auto now{std::chrono::system_clock::now()};
    const auto nextJobAllowedAt{m_lastJobStartedAt + JOB_RATE_LIMIT};

    if (now >= nextJobAllowedAt)
    {
      // Finished job ended after job schedule timeslice was over. Start a new update job now.
      StartDirectoryJob();
    }
    else
    {
      // Finished job ended before job schedule timeslice was over. Start a new update job delayed.
      m_nextJobTimer.Start(
          std::chrono::duration_cast<std::chrono::milliseconds>(nextJobAllowedAt - now));
    }
  }
}

void CDirectoryProvider::OnTimeout()
{
  std::unique_lock lock(m_section);

  if (m_jobPending)
  {
    // Start a new update job.
    StartDirectoryJob();
  }
}

bool CDirectoryProvider::OnEventPublished(Topic topic /*= Topic::UNSPECIFIED*/)
{
  std::unique_lock lock(m_section);
  switch (topic)
  {
    case Topic::UNSPECIFIED:
      // Always invalidate.
      break;
    case Topic::PLAYER:
      // We don't need to do anything if there is no matching sort method.
      if (m_currentSort.sortBy != SortByNone && m_currentSort.sortBy != SortByLastPlayed &&
          m_currentSort.sortBy != SortByDateAdded && m_currentSort.sortBy != SortByPlaycount &&
          m_currentSort.sortBy != SortByLastUsed)
        return false;
      break;
    case Topic::VIDEO_LIBRARY:
      // We don't need to do anything if there are no fitting items.
      return std::ranges::find(m_itemTypes, InfoTagType::VIDEO) == m_itemTypes.cend();
    case Topic::AUDIO_LIBRARY:
      // We don't need to do anything if there are no fitting items.
      return std::ranges::find(m_itemTypes, InfoTagType::AUDIO) == m_itemTypes.cend();
  }
  m_updateState = UpdateState::INVALIDATED;
  return true;
}

std::string CDirectoryProvider::GetTarget(const CFileItem& item) const
{
  std::string target = item.GetProperty("node.target").asString();

  std::unique_lock lock(m_section);
  if (target.empty())
    target = m_currentTarget;
  if (target.empty())
    target = m_target.GetLabel(GetParentId(), false);

  return target;
}

bool CDirectoryProvider::OnClick(const std::shared_ptr<CGUIListItem>& item)
{
  std::shared_ptr<CFileItem> targetItem{std::static_pointer_cast<CFileItem>(item)};

  if (targetItem->IsFavourite())
  {
    targetItem = CServiceBroker::GetFavouritesService().ResolveFavourite(*targetItem);
    if (!targetItem)
      return false;
  }

  const CExecString exec{*targetItem, GetTarget(*targetItem)};
  const bool isPlayMedia{exec.GetFunction() == "playmedia"};

  // video select action setting is for files only, except exec func is playmedia...
  if (targetItem->HasVideoInfoTag() && (!targetItem->IsFolder() || isPlayMedia))
  {
    const std::string targetWindow{GetTarget(*targetItem)};
    if (!targetWindow.empty())
      targetItem->SetProperty("targetwindow", targetWindow);

    KODI::VIDEO::GUILIB::CVideoSelectActionProcessor proc{targetItem};
    if (proc.ProcessDefaultAction())
      return true;
  }

  // exec the execute string for the original (!) item
  CFileItem fileItem{*std::static_pointer_cast<CFileItem>(item)};

  if (fileItem.HasProperty("node.target_url"))
    fileItem.SetPath(fileItem.GetProperty("node.target_url").asString());

  return CGUIBuiltinsUtils::ExecuteAction({fileItem, GetTarget(fileItem)}, targetItem);
}

bool CDirectoryProvider::OnPlay(const std::shared_ptr<CGUIListItem>& item)
{
  std::shared_ptr<CFileItem> targetItem{std::static_pointer_cast<CFileItem>(item)};

  if (targetItem->IsFavourite())
  {
    targetItem = CServiceBroker::GetFavouritesService().ResolveFavourite(*targetItem);
    if (!targetItem)
      return false;
  }

  // video play action setting is for files and folders...
  if (targetItem->HasVideoInfoTag() ||
      (targetItem->IsFolder() && VIDEO::UTILS::IsItemPlayable(*targetItem)))
  {
    KODI::VIDEO::GUILIB::CVideoPlayActionProcessor proc{targetItem};
    if (proc.ProcessDefaultAction())
      return true;
  }

  if (CPlayerUtils::IsItemPlayable(*targetItem))
  {
    const CExecString exec{*targetItem, GetTarget(*targetItem)};
    if (exec.GetFunction() == "playmedia")
    {
      // exec as is
      return CGUIBuiltinsUtils::ExecuteAction(exec, targetItem);
    }
    else
    {
      // build a playmedia execute string for given target and exec this
      return CGUIBuiltinsUtils::ExecutePlayMediaAskResume(targetItem);
    }
  }
  return true;
}

bool CDirectoryProvider::OnInfo(const std::shared_ptr<CGUIListItem>& item)
{
  const auto fileItem{std::static_pointer_cast<CFileItem>(item)};
  const auto targetItem{fileItem->IsFavourite()
                            ? CServiceBroker::GetFavouritesService().ResolveFavourite(*fileItem)
                            : fileItem};

  return CGUIContentUtils::ShowInfoForItem(*targetItem);
}

bool CDirectoryProvider::OnContextMenu(const std::shared_ptr<CGUIListItem>& item)
{
  const auto fileItem{std::static_pointer_cast<CFileItem>(item)};
  const std::string target{GetTarget(*fileItem)};
  if (!target.empty())
    fileItem->SetProperty("targetwindow", target);

  return CONTEXTMENU::ShowFor(fileItem, CContextMenuManager::MAIN);
}

bool CDirectoryProvider::IsUpdating() const
{
  {
    std::unique_lock lock(m_section);
    if (m_jobID || m_jobPending || m_updateState == UpdateState::DONE ||
        m_updateState == UpdateState::INVALIDATED)
      return true;
  }

  std::unique_lock subscriptionLock(m_subscriptionSection);
  return m_subscriber && !m_subscriber->IsReadyToUse();
}

bool CDirectoryProvider::UpdateURL()
{
  std::string value;
  {
    std::unique_lock lock(m_section);
    value = m_url.GetLabel(GetParentId(), false);
    if (value == m_currentUrl)
      return false;

    m_currentUrl = value;
  }

  std::unique_lock subscriptionLock(m_subscriptionSection);
  m_subscriber = GetSubscriber(value, *this);
  return true;
}

bool CDirectoryProvider::UpdateLimit()
{
  std::unique_lock lock(m_section);
  unsigned int value = m_limit.GetIntValue(GetParentId());
  if (value == m_currentLimit)
    return false;

  m_currentLimit = value;
  return true;
}

bool CDirectoryProvider::UpdateBrowse()
{
  std::unique_lock lock(m_section);
  const std::string stringValue{m_browse.GetLabel(GetParentId(), false)};
  BrowseMode value{m_currentBrowse};
  if (StringUtils::EqualsNoCase(stringValue, "always"))
    value = BrowseMode::ALWAYS;
  else if (StringUtils::EqualsNoCase(stringValue, "auto"))
    value = BrowseMode::AUTO;
  else if (StringUtils::EqualsNoCase(stringValue, "never"))
    value = BrowseMode::NEVER;

  if (value == m_currentBrowse)
    return false;

  m_currentBrowse = value;
  return true;
}

bool CDirectoryProvider::UpdateSort()
{
  std::unique_lock lock(m_section);
  SortBy sortMethod(SortUtils::SortMethodFromString(m_sortMethod.GetLabel(GetParentId(), false)));
  SortOrder sortOrder(SortUtils::SortOrderFromString(m_sortOrder.GetLabel(GetParentId(), false)));
  if (sortOrder == SortOrderNone)
    sortOrder = SortOrderAscending;

  if (sortMethod == m_currentSort.sortBy && sortOrder == m_currentSort.sortOrder)
    return false;

  m_currentSort.sortBy = sortMethod;
  m_currentSort.sortOrder = sortOrder;
  m_currentSort.sortAttributes = SortAttributeIgnoreFolders;

  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    m_currentSort.sortAttributes =
        static_cast<SortAttribute>(m_currentSort.sortAttributes | SortAttributeIgnoreArticle);

  return true;
}
