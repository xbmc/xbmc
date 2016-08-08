/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DirectoryProvider.h"

#include <memory>
#include <utility>
#include "addons/GUIDialogAddonInfo.h"
#include "ContextMenuManager.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/FavouritesDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "music/MusicThumbLoader.h"
#include "pictures/PictureThumbLoader.h"
#include "pvr/PVRManager.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"
#include "utils/log.h"
#include "utils/SortUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "video/VideoThumbLoader.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoBase.h"

using namespace XFILE;
using namespace ANNOUNCEMENT;
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
  virtual ~CDirectoryJob() { }

  virtual const char* GetType() const { return "directory"; }
  virtual bool operator==(const CJob *job) const
  {
    if (strcmp(job->GetType(),GetType()) == 0)
    {
      const CDirectoryJob* dirJob = dynamic_cast<const CDirectoryJob*>(job);
      if (dirJob && dirJob->m_url == m_url)
        return true;
    }
    return false;
  }

  virtual bool DoWork()
  {
    CFileItemList items;
    if (CDirectory::GetDirectory(m_url, items, ""))
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

  std::shared_ptr<CThumbLoader> getThumbLoader(CGUIStaticItemPtr &item)
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
    for (std::map<InfoTagType, std::shared_ptr<CThumbLoader> >::const_iterator
         i = m_thumbloaders.begin(); i != m_thumbloaders.end(); ++i)
      itemTypes.push_back(i->first);
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

CDirectoryProvider::~CDirectoryProvider()
{
  Reset(true);
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

  CSingleLock lock(m_section);
  if (m_updateState == INVALIDATED)
    fireJob = true;
  else if (m_updateState == DONE)
    changed = true;

  m_updateState = OK;

  if (fireJob)
  {
    CLog::Log(LOGDEBUG, "CDirectoryProvider[%s]: refreshing..", m_currentUrl.c_str());
    if (m_jobID)
      CJobManager::GetInstance().CancelJob(m_jobID);
    m_jobID = CJobManager::GetInstance().AddJob(new CDirectoryJob(m_currentUrl, m_currentSort, m_currentLimit, m_parentID), this);
  }

  if (!changed)
  {
    for (std::vector<CGUIStaticItemPtr>::iterator i = m_items.begin(); i != m_items.end(); ++i)
      changed |= (*i)->UpdateVisibility(m_parentID);
  }
  return changed; //! @todo Also returned changed if properties are changed (if so, need to update scroll to letter).
}

void CDirectoryProvider::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  // we are only interested in library and player changes
  if ((flag & (VideoLibrary | AudioLibrary | Player)) == 0)
    return;

  {
    CSingleLock lock(m_section);
    // we don't need to refresh anything if there are no fitting
    // items in this list provider for the announcement flag
    if (((flag & VideoLibrary) &&
         (std::find(m_itemTypes.begin(), m_itemTypes.end(), InfoTagType::VIDEO) == m_itemTypes.end())) ||
        ((flag & AudioLibrary) &&
         (std::find(m_itemTypes.begin(), m_itemTypes.end(), InfoTagType::AUDIO) == m_itemTypes.end())))
      return;

    if (flag & Player)
    {
      if (strcmp(message, "OnPlay") == 0 ||
          strcmp(message, "OnStop") == 0)
      {
        if (m_currentSort.sortBy == SortByLastPlayed ||
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
      if (strcmp(message, "OnScanFinished") == 0 ||
          strcmp(message, "OnCleanFinished") == 0 ||
          strcmp(message, "OnUpdate") == 0 ||
          strcmp(message, "OnRemove") == 0)
        m_updateState = INVALIDATED;
    }
  }
}

void CDirectoryProvider::Fetch(std::vector<CGUIListItemPtr> &items) const
{
  CSingleLock lock(m_section);
  items.clear();
  for (std::vector<CGUIStaticItemPtr>::const_iterator i = m_items.begin(); i != m_items.end(); ++i)
  {
    if ((*i)->IsVisible())
      items.push_back(*i);
  }
}

void CDirectoryProvider::OnAddonEvent(const ADDON::AddonEvent& event)
{
  CSingleLock lock(m_section);
  if (URIUtils::IsProtocol(m_currentUrl, "addons"))
  {
    if (typeid(event) == typeid(ADDON::AddonEvents::Enabled) ||
        typeid(event) == typeid(ADDON::AddonEvents::Disabled) ||
        typeid(event) == typeid(ADDON::AddonEvents::InstalledChanged) ||
        typeid(event) == typeid(ADDON::AddonEvents::MetadataChanged))
      m_updateState = INVALIDATED;
  }
}

void CDirectoryProvider::OnPVRManagerEvent(const PVR::ManagerState& event)
{
  CSingleLock lock(m_section);
  if (URIUtils::IsProtocol(m_currentUrl, "pvr"))
  {
    if (event == ManagerStateStarted ||
        event == ManagerStateStopped ||
        event == ManagerStateError ||
        event == ManagerStateInterrupted)
      m_updateState = INVALIDATED;
  }
}

void CDirectoryProvider::Reset(bool immediately /* = false */)
{
  // cancel any pending jobs
  CSingleLock lock(m_section);
  if (m_jobID)
    CJobManager::GetInstance().CancelJob(m_jobID);
  m_jobID = 0;
  // reset only if this is going to be destructed
  if (immediately)
  {
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
      CAnnouncementManager::GetInstance().RemoveAnnouncer(this);
      ADDON::CAddonMgr::GetInstance().Events().Unsubscribe(this);
      g_PVRManager.Events().Unsubscribe(this);
    }
  }
}

void CDirectoryProvider::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CSingleLock lock(m_section);
  if (success)
  {
    m_items = ((CDirectoryJob*)job)->GetItems();
    m_currentTarget = ((CDirectoryJob*)job)->GetTarget();
    ((CDirectoryJob*)job)->GetItemTypes(m_itemTypes);
    if (m_updateState == OK)
      m_updateState = DONE;
  }
  m_jobID = 0;
}

bool CDirectoryProvider::OnClick(const CGUIListItemPtr &item)
{
  CFileItem fileItem(*std::static_pointer_cast<CFileItem>(item));

  if (fileItem.HasVideoInfoTag()
      && CSettings::GetInstance().GetInt(CSettings::SETTING_MYVIDEOS_SELECTACTION) == SELECT_ACTION_INFO
      && OnInfo(item))
    return true;

  std::string target = fileItem.GetProperty("node.target").asString();
  {
    CSingleLock lock(m_section);
    if (target.empty())
      target = m_currentTarget;
    if (target.empty())
      target = m_target.GetLabel(m_parentID, false);
    if (fileItem.HasProperty("node.target_url"))
      fileItem.SetPath(fileItem.GetProperty("node.target_url").asString());
  }
  // grab the execute string
  std::string execute = CFavouritesDirectory::GetExecutePath(fileItem, target);
  if (!execute.empty())
  {
    CGUIMessage message(GUI_MSG_EXECUTE, 0, 0);
    message.SetStringParam(execute);
    g_windowManager.SendMessage(message);
    return true;
  }
  return false;
}

bool CDirectoryProvider::OnInfo(const CGUIListItemPtr& item)
{
  auto fileItem = std::static_pointer_cast<CFileItem>(item);

  if (fileItem->HasAddonInfo())
    return CGUIDialogAddonInfo::ShowForItem(fileItem);
  else if (fileItem->HasVideoInfoTag())
  {
    CGUIDialogVideoInfo::ShowFor(*fileItem.get());
    return true;
  }
  else if (fileItem->HasMusicInfoTag())
  {
    CGUIDialogMusicInfo::ShowFor(*fileItem.get());
    return true;
  }
  return false;
}

bool CDirectoryProvider::OnContextMenu(const CGUIListItemPtr& item)
{
  auto fileItem = std::static_pointer_cast<CFileItem>(item);
  return CONTEXTMENU::ShowFor(fileItem);
}

bool CDirectoryProvider::IsUpdating() const
{
  CSingleLock lock(m_section);
  return m_jobID || m_updateState == DONE || m_updateState == INVALIDATED;
}

bool CDirectoryProvider::UpdateURL()
{
  CSingleLock lock(m_section);
  std::string value(m_url.GetLabel(m_parentID, false));
  if (value == m_currentUrl)
    return false;

  m_currentUrl = value;

  if (!m_isAnnounced)
  {
    m_isAnnounced = true;
    CAnnouncementManager::GetInstance().AddAnnouncer(this);
    ADDON::CAddonMgr::GetInstance().Events().Subscribe(this, &CDirectoryProvider::OnAddonEvent);
    g_PVRManager.Events().Subscribe(this, &CDirectoryProvider::OnPVRManagerEvent);
  }
  return true;
}

bool CDirectoryProvider::UpdateLimit()
{
  CSingleLock lock(m_section);
  unsigned int value = m_limit.GetIntValue(m_parentID);
  if (value == m_currentLimit)
    return false;

  m_currentLimit = value;

  return true;
}

bool CDirectoryProvider::UpdateSort()
{
  CSingleLock lock(m_section);
  SortBy sortMethod(SortUtils::SortMethodFromString(m_sortMethod.GetLabel(m_parentID, false)));
  SortOrder sortOrder(SortUtils::SortOrderFromString(m_sortOrder.GetLabel(m_parentID, false)));
  if (sortOrder == SortOrderNone)
    sortOrder = SortOrderAscending;

  if (sortMethod == m_currentSort.sortBy && sortOrder == m_currentSort.sortOrder)
    return false;

  m_currentSort.sortBy = sortMethod;
  m_currentSort.sortOrder = sortOrder;
  m_currentSort.sortAttributes = SortAttributeIgnoreFolders;

  if (CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    m_currentSort.sortAttributes = static_cast<SortAttribute>(m_currentSort.sortAttributes | SortAttributeIgnoreArticle);

  return true;
}
