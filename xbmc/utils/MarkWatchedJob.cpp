/*
 *      Copyright (C) 2014 Team XBMC
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

#include "FileItem.h"
#include "filesystem/Directory.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "MarkWatchedJob.h"
#include "profiles/ProfilesManager.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif

#include <vector>

CMarkWatchedJob::CMarkWatchedJob(const CFileItemPtr &item, bool bMark)
{
  m_item = item;
  m_bMark = bMark;
}

CMarkWatchedJob::~CMarkWatchedJob()
{
}

bool CMarkWatchedJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) == 0)
  {
    const CMarkWatchedJob* markJob = dynamic_cast<const CMarkWatchedJob*>(job);
    if (markJob)
      return (m_item->IsSamePath(markJob->m_item.get()) && markJob->m_bMark == m_bMark);
  }
  return false;
}

bool CMarkWatchedJob::DoWork()
{
  if (!CProfilesManager::Get().GetCurrentProfile().canWriteDatabases())
    return false;

  CFileItemList items;
  items.Add(CFileItemPtr(new CFileItem(*m_item)));

  if (m_item->m_bIsFolder)
    CUtil::GetRecursiveListing(m_item->GetPath(), items, "", XFILE::DIR_FLAG_NO_FILE_INFO);

  std::vector<CFileItemPtr> markItems;
  for (int i = 0; i < items.Size(); i++)
  {
    if (items[i]->HasVideoInfoTag() &&
        (( m_bMark && items[i]->GetVideoInfoTag()->m_playCount) ||
         (!m_bMark && !(items[i]->GetVideoInfoTag()->m_playCount))))
      continue;

#ifdef HAS_UPNP
    if (!URIUtils::IsUPnP(items[i]->GetPath()) || !UPNP::CUPnP::MarkWatched(*items[i], m_bMark))
#endif
    {
      markItems.push_back(items[i]);
    }
  }

  if (!markItems.empty())
  {
    CVideoDatabase database;
    if (!database.Open())
      return false;

    database.BeginTransaction();

    for (std::vector<CFileItemPtr>::const_iterator iter = markItems.begin(); iter != markItems.end(); ++iter)
    {
      CFileItemPtr pItem = *iter;
      if (m_bMark)
        database.ClearBookMarksOfFile(pItem->GetPath(), CBookmark::RESUME);
      database.SetPlayCount(*pItem, m_bMark ? 1 : 0);
    }

    database.CommitTransaction();
    database.Close();
  }

  return true;
}

CMarkWatchedQueue &CMarkWatchedQueue::Get()
{
  static CMarkWatchedQueue markWatchedQueue;
  return markWatchedQueue;
}

void CMarkWatchedQueue::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (success)
  {
    if (QueueEmpty())
    {
      CUtil::DeleteVideoDatabaseDirectoryCache();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
      g_windowManager.SendThreadMessage(msg);
    }
    return CJobQueue::OnJobComplete(jobID, success, job);
  }
  CancelJobs();
}
