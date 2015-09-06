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

#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/FavouritesDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "music/MusicThumbLoader.h"
#include "pictures/PictureThumbLoader.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/JobManager.h"
#include "utils/SortUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XMLUtils.h"
#include "video/VideoThumbLoader.h"

using namespace XFILE;
using namespace ANNOUNCEMENT;
using namespace KODI::MESSAGING;

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
      initThumbLoader<CVideoThumbLoader>(VIDEO);
      return m_thumbloaders[VIDEO];
    }
    if (item->IsAudio())
    {
      initThumbLoader<CMusicThumbLoader>(AUDIO);
      return m_thumbloaders[AUDIO];
    }
    if (item->IsPicture())
    {
      initThumbLoader<CPictureThumbLoader>(PICTURE);
      return m_thumbloaders[PICTURE];
    }
    initThumbLoader<CProgramThumbLoader>(PROGRAM);
    return m_thumbloaders[PROGRAM];
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
   m_updateTime(0),
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
  {
    CSingleLock lock(m_section);
    if (m_updateState == DONE)
      changed = true;
    else if (m_updateState == PENDING)
      fireJob = true;
    m_updateState = OK;
  }

  // update the URL & limit and fire off a new job if needed
  fireJob |= UpdateURL();
  fireJob |= UpdateSort();
  fireJob |= UpdateLimit();
  if (fireJob)
    FireJob();

  for (std::vector<CGUIStaticItemPtr>::iterator i = m_items.begin(); i != m_items.end(); ++i)
    changed |= (*i)->UpdateVisibility(m_parentID);
  return changed; // TODO: Also returned changed if properties are changed (if so, need to update scroll to letter).
}

void CDirectoryProvider::Announce(AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  // we are only interested in library changes
  if ((flag & (VideoLibrary | AudioLibrary)) == 0)
    return;

  {
    CSingleLock lock(m_section);
    // we don't need to refresh anything if there are no fitting
    // items in this list provider for the announcement flag
    if (((flag & VideoLibrary) &&
         (std::find(m_itemTypes.begin(), m_itemTypes.end(), VIDEO) == m_itemTypes.end())) ||
        ((flag & AudioLibrary) &&
         (std::find(m_itemTypes.begin(), m_itemTypes.end(), AUDIO) == m_itemTypes.end())))
      return;

    // if we're in a database transaction, don't bother doing anything just yet
    if (data.isMember("transaction") && data["transaction"].asBoolean())
      return;

    // if there was a database update, we set the update state
    // to PENDING to fire off a new job in the next update
    if (strcmp(message, "OnScanFinished") == 0 ||
        strcmp(message, "OnCleanFinished") == 0 ||
        strcmp(message, "OnUpdate") == 0 ||
        strcmp(message, "OnRemove") == 0)
      m_updateState = PENDING;
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
    RegisterListProvider(false);
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
    m_updateState = DONE;
  }
  m_jobID = 0;
}

bool CDirectoryProvider::OnClick(const CGUIListItemPtr &item)
{
  CFileItem fileItem(*std::static_pointer_cast<CFileItem>(item));
  std::string target = fileItem.GetProperty("node.target").asString();
  if (target.empty())
    target = m_currentTarget;
  if (target.empty())
    target = m_target.GetLabel(m_parentID, false);
  if (fileItem.HasProperty("node.target_url"))
    fileItem.SetPath(fileItem.GetProperty("node.target_url").asString());
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

bool CDirectoryProvider::IsUpdating() const
{
  CSingleLock lock(m_section);
  return m_jobID || (m_updateState == DONE);
}

void CDirectoryProvider::FireJob()
{
  CSingleLock lock(m_section);
  if (m_jobID)
    CJobManager::GetInstance().CancelJob(m_jobID);
  m_jobID = CJobManager::GetInstance().AddJob(new CDirectoryJob(m_currentUrl, m_currentSort, m_currentLimit, m_parentID), this);
}

void CDirectoryProvider::RegisterListProvider(bool hasLibraryContent)
{
  if (hasLibraryContent && !m_isAnnounced)
  {
    m_isAnnounced = true;
    CAnnouncementManager::GetInstance().AddAnnouncer(this);
  }
  else if (!hasLibraryContent && m_isAnnounced)
  {
    m_isAnnounced = false;
    CAnnouncementManager::GetInstance().RemoveAnnouncer(this);
  }
}

bool CDirectoryProvider::UpdateURL()
{
  std::string value(m_url.GetLabel(m_parentID, false));
  if (value == m_currentUrl)
    return false;

  m_currentUrl = value;

  // Register this provider only if we have library content
  RegisterListProvider(URIUtils::IsLibraryContent(m_currentUrl));

  return true;
}

bool CDirectoryProvider::UpdateLimit()
{
  unsigned int value = m_limit.GetIntValue(m_parentID);
  if (value == m_currentLimit)
    return false;

  m_currentLimit = value;

  return true;
}

bool CDirectoryProvider::UpdateSort()
{
  SortBy sortMethod(SortUtils::SortMethodFromString(m_sortMethod.GetLabel(m_parentID, false)));
  SortOrder sortOrder(SortUtils::SortOrderFromString(m_sortOrder.GetLabel(m_parentID, false)));
  if (sortOrder == SortOrderNone)
    sortOrder = SortOrderAscending;

  if (sortMethod == m_currentSort.sortBy && sortOrder == m_currentSort.sortOrder)
    return false;

  m_currentSort.sortBy = sortMethod;
  m_currentSort.sortOrder = sortOrder;

  if (CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING))
    m_currentSort.sortAttributes = static_cast<SortAttribute>(m_currentSort.sortAttributes | SortAttributeIgnoreArticle);

  return true;
}
