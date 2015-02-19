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

#include <vector>

#include "VideoLibraryMarkWatchedJob.h"
#include "FileItem.h"
#include "Util.h"
#include "filesystem/Directory.h"
#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif
#include "profiles/ProfilesManager.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

CVideoLibraryMarkWatchedJob::CVideoLibraryMarkWatchedJob(const CFileItemPtr &item, bool mark)
  : m_item(item),
    m_mark(mark)
{ }

CVideoLibraryMarkWatchedJob::~CVideoLibraryMarkWatchedJob()
{ }

bool CVideoLibraryMarkWatchedJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CVideoLibraryMarkWatchedJob* markJob = dynamic_cast<const CVideoLibraryMarkWatchedJob*>(job);
  if (markJob == NULL)
    return false;

  return m_item->IsSamePath(markJob->m_item.get()) && markJob->m_mark == m_mark;
}

bool CVideoLibraryMarkWatchedJob::Work(CVideoDatabase &db)
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
    CFileItemPtr item = items.Get(i);
    if (item->HasVideoInfoTag() && m_mark == (item->GetVideoInfoTag()->m_playCount > 0))
      continue;

#ifdef HAS_UPNP
    if (URIUtils::IsUPnP(item->GetPath()) && UPNP::CUPnP::MarkWatched(*item, m_mark))
      continue;
#endif

    markItems.push_back(item);
  }

  if (markItems.empty())
    return true;

  db.BeginTransaction();

  for (std::vector<CFileItemPtr>::const_iterator iter = markItems.begin(); iter != markItems.end(); ++iter)
  {
    CFileItemPtr item = *iter;
    if (m_mark)
    {
      std::string path(item->GetPath());
      if (item->HasVideoInfoTag())
        path = item->GetVideoInfoTag()->GetPath();

      db.ClearBookMarksOfFile(path, CBookmark::RESUME);
      db.IncrementPlayCount(*item);
    }
    else
      db.SetPlayCount(*item, 0);
  }

  db.CommitTransaction();
  db.Close();

  return true;
}
