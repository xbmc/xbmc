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

#ifndef LISTPROVIDERS_DIRECTORYPROVIDER_H_INCLUDED
#define LISTPROVIDERS_DIRECTORYPROVIDER_H_INCLUDED
#include "DirectoryProvider.h"
#endif

#ifndef LISTPROVIDERS_FILESYSTEM_DIRECTORY_H_INCLUDED
#define LISTPROVIDERS_FILESYSTEM_DIRECTORY_H_INCLUDED
#include "filesystem/Directory.h"
#endif

#ifndef LISTPROVIDERS_FILESYSTEM_FAVOURITESDIRECTORY_H_INCLUDED
#define LISTPROVIDERS_FILESYSTEM_FAVOURITESDIRECTORY_H_INCLUDED
#include "filesystem/FavouritesDirectory.h"
#endif

#ifndef LISTPROVIDERS_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#define LISTPROVIDERS_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#include "guilib/GUIWindowManager.h"
#endif

#ifndef LISTPROVIDERS_UTILS_JOBMANAGER_H_INCLUDED
#define LISTPROVIDERS_UTILS_JOBMANAGER_H_INCLUDED
#include "utils/JobManager.h"
#endif

#ifndef LISTPROVIDERS_UTILS_STRINGUTILS_H_INCLUDED
#define LISTPROVIDERS_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef LISTPROVIDERS_UTILS_TIMEUTILS_H_INCLUDED
#define LISTPROVIDERS_UTILS_TIMEUTILS_H_INCLUDED
#include "utils/TimeUtils.h"
#endif

#ifndef LISTPROVIDERS_UTILS_XMLUTILS_H_INCLUDED
#define LISTPROVIDERS_UTILS_XMLUTILS_H_INCLUDED
#include "utils/XMLUtils.h"
#endif

#ifndef LISTPROVIDERS_UTILS_STRINGUTILS_H_INCLUDED
#define LISTPROVIDERS_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef LISTPROVIDERS_UTILS_URIUTILS_H_INCLUDED
#define LISTPROVIDERS_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef LISTPROVIDERS_UTILS_LOG_H_INCLUDED
#define LISTPROVIDERS_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef LISTPROVIDERS_THREADS_SINGLELOCK_H_INCLUDED
#define LISTPROVIDERS_THREADS_SINGLELOCK_H_INCLUDED
#include "threads/SingleLock.h"
#endif

#ifndef LISTPROVIDERS_APPLICATIONMESSENGER_H_INCLUDED
#define LISTPROVIDERS_APPLICATIONMESSENGER_H_INCLUDED
#include "ApplicationMessenger.h"
#endif

#ifndef LISTPROVIDERS_FILEITEM_H_INCLUDED
#define LISTPROVIDERS_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef LISTPROVIDERS_VIDEO_VIDEOTHUMBLOADER_H_INCLUDED
#define LISTPROVIDERS_VIDEO_VIDEOTHUMBLOADER_H_INCLUDED
#include "video/VideoThumbLoader.h"
#endif

#ifndef LISTPROVIDERS_MUSIC_MUSICTHUMBLOADER_H_INCLUDED
#define LISTPROVIDERS_MUSIC_MUSICTHUMBLOADER_H_INCLUDED
#include "music/MusicThumbLoader.h"
#endif

#ifndef LISTPROVIDERS_PICTURES_PICTURETHUMBLOADER_H_INCLUDED
#define LISTPROVIDERS_PICTURES_PICTURETHUMBLOADER_H_INCLUDED
#include "pictures/PictureThumbLoader.h"
#endif

#include "boost/make_shared.hpp"
#ifndef LISTPROVIDERS_INTERFACES_ANNOUNCEMENTMANAGER_H_INCLUDED
#define LISTPROVIDERS_INTERFACES_ANNOUNCEMENTMANAGER_H_INCLUDED
#include "interfaces/AnnouncementManager.h"
#endif


using namespace std;
using namespace XFILE;
using namespace ANNOUNCEMENT;

class CDirectoryJob : public CJob
{
public:
  CDirectoryJob(const std::string &url, int parentID) : m_url(url), m_parentID(parentID) { };
  virtual ~CDirectoryJob() {};

  virtual const char* GetType() const { return "directory"; };
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
      // convert to CGUIStaticItem's and set visibility and targets
      m_items.reserve(items.Size());
      for (int i = 0; i < items.Size(); i++)
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

  boost::shared_ptr<CThumbLoader> getThumbLoader(CGUIStaticItemPtr &item)
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
      boost::shared_ptr<CThumbLoader> thumbLoader = boost::make_shared<CThumbLoaderClass>();
      thumbLoader->OnLoaderStart();
      m_thumbloaders.insert(make_pair(type, thumbLoader));
    }
  }

  const std::vector<CGUIStaticItemPtr> &GetItems() const { return m_items; }
  const std::string &GetTarget() const { return m_target; }
  std::vector<InfoTagType> GetItemTypes(std::vector<InfoTagType> &itemTypes) const
  {
    itemTypes.clear();
    for (std::map<InfoTagType, boost::shared_ptr<CThumbLoader> >::const_iterator
         i = m_thumbloaders.begin(); i != m_thumbloaders.end(); ++i)
      itemTypes.push_back(i->first);
    return itemTypes;
  }
private:
  std::string m_url;
  std::string m_target;
  int m_parentID;
  std::vector<CGUIStaticItemPtr> m_items;
  std::map<InfoTagType, boost::shared_ptr<CThumbLoader> > m_thumbloaders;
};

CDirectoryProvider::CDirectoryProvider(const TiXmlElement *element, int parentID)
 : IListProvider(parentID),
   m_updateTime(0),
   m_updateState(OK),
   m_isDbUpdating(false),
   m_isAnnounced(false),
   m_jobID(0)
{
  assert(element);
  if (!element->NoChildren())
  {
    const char *target = element->Attribute("target");
    if (target)
      m_target.SetLabel(target, "", parentID);
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

  // update the URL and fire off a new job if needed
  if (fireJob || UpdateURL())
    FireJob();

  for (vector<CGUIStaticItemPtr>::iterator i = m_items.begin(); i != m_items.end(); ++i)
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

    // don't update while scanning / cleaning
    if (strcmp(message, "OnScanStarted") == 0 ||
        strcmp(message, "OnCleanStarted") == 0)
    {
      m_isDbUpdating = true;
      return;
    }

    // if there was a database update, we set the update state
    // to PENDING to fire off a new job in the next update
    if (strcmp(message, "OnScanFinished") == 0 ||
        strcmp(message, "OnCleanFinished") == 0 ||
        ((strcmp(message, "OnUpdate") == 0 ||
          strcmp(message, "OnRemove") == 0) && !m_isDbUpdating))
    {
      m_isDbUpdating = false;
      m_updateState = PENDING;
    }
  }
}

void CDirectoryProvider::Fetch(vector<CGUIListItemPtr> &items) const
{
  CSingleLock lock(m_section);
  items.clear();
  for (vector<CGUIStaticItemPtr>::const_iterator i = m_items.begin(); i != m_items.end(); ++i)
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
  CFileItem fileItem(*boost::static_pointer_cast<CFileItem>(item));
  string target = fileItem.GetProperty("node.target").asString();
  if (target.empty())
    target = m_currentTarget;
  if (target.empty())
    target = m_target.GetLabel(m_parentID, false);
  if (fileItem.HasProperty("node.target_url"))
    fileItem.SetPath(fileItem.GetProperty("node.target_url").asString());
  // grab the execute string
  string execute = CFavouritesDirectory::GetExecutePath(fileItem, target);
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
  m_jobID = CJobManager::GetInstance().AddJob(new CDirectoryJob(m_currentUrl, m_parentID), this);
}

void CDirectoryProvider::RegisterListProvider(bool hasLibraryContent)
{
  if (hasLibraryContent && !m_isAnnounced)
  {
    m_isAnnounced = true;
    CAnnouncementManager::AddAnnouncer(this);
  }
  else if (!hasLibraryContent && m_isAnnounced)
  {
    m_isAnnounced = false;
    m_isDbUpdating = false;
    CAnnouncementManager::RemoveAnnouncer(this);
  }
}

bool CDirectoryProvider::UpdateURL()
{
  CStdString value(m_url.GetLabel(m_parentID, false));
  if (value == m_currentUrl)
    return false;

  m_currentUrl = value;

  // Register this provider only if we have library content
  RegisterListProvider(URIUtils::IsLibraryContent(m_currentUrl));

  return true;
}
