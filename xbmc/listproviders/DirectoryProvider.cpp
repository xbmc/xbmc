/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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
#include "filesystem/Directory.h"
#include "filesystem/FavouritesDirectory.h"
#include "guilib/GUIWindowManager.h"
#include "utils/JobManager.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/XMLUtils.h"
#include "threads/SingleLock.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"

using namespace std;
using namespace XFILE;

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
        m_items.push_back(item);
      }
      m_target = items.GetProperty("node.target").asString();
    }
    return true;    
  }

  const std::vector<CGUIStaticItemPtr> &GetItems() const { return m_items; }
  const std::string &GetTarget() const { return m_target; }
private:
  std::string m_url;
  std::string m_target;
  int m_parentID;
  std::vector<CGUIStaticItemPtr> m_items;
};

CDirectoryProvider::CDirectoryProvider(const TiXmlElement *element, int parentID)
 : IListProvider(parentID),
   m_updateTime(0),
   m_invalid(false),
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
  Reset();
}

bool CDirectoryProvider::Update(bool refresh)
{
  bool changed = refresh;
  {
    CSingleLock lock(m_section);
    changed |= m_invalid;
    m_invalid = false;
  }

  // update the URL and fire off a new job if needed
  CStdString value(m_url.GetLabel(m_parentID, false));
  if (value != m_currentUrl)
  {
    m_currentUrl = value;

    // fire job
    CSingleLock lock(m_section);
    if (m_jobID)
      CJobManager::GetInstance().CancelJob(m_jobID);
    m_jobID = CJobManager::GetInstance().AddJob(new CDirectoryJob(m_currentUrl, m_parentID), this);
  }

  for (vector<CGUIStaticItemPtr>::iterator i = m_items.begin(); i != m_items.end(); ++i)
    changed |= (*i)->UpdateVisibility(m_parentID);
  return changed; // TODO: Also returned changed if properties are changed (if so, need to update scroll to letter).
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

void CDirectoryProvider::Reset()
{
  // cancel any pending jobs
  CSingleLock lock(m_section);
  if (m_jobID)
    CJobManager::GetInstance().CancelJob(m_jobID);
  m_jobID = 0;
  m_items.clear();
  m_currentTarget.clear();
  m_currentUrl.clear();
  m_invalid = false;
}

void CDirectoryProvider::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CSingleLock lock(m_section);
  if (success)
  {
    m_items = ((CDirectoryJob*)job)->GetItems();
    m_currentTarget = ((CDirectoryJob*)job)->GetTarget();
    m_invalid = true;
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
  return m_jobID || m_invalid;
}
