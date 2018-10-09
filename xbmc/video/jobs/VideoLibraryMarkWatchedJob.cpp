/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <vector>

#include "VideoLibraryMarkWatchedJob.h"
#include "FileItem.h"
#include "Util.h"
#include "filesystem/Directory.h"
#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "ServiceBroker.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

CVideoLibraryMarkWatchedJob::CVideoLibraryMarkWatchedJob(const CFileItemPtr &item, bool mark)
  : m_item(item),
    m_mark(mark)
{ }

CVideoLibraryMarkWatchedJob::~CVideoLibraryMarkWatchedJob() = default;

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
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (!profileManager->GetCurrentProfile().canWriteDatabases())
    return false;

  CFileItemList items;
  items.Add(CFileItemPtr(new CFileItem(*m_item)));

  if (m_item->m_bIsFolder)
    CUtil::GetRecursiveListing(m_item->GetPath(), items, "", XFILE::DIR_FLAG_NO_FILE_INFO);

  std::vector<CFileItemPtr> markItems;
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items.Get(i);
    if (item->HasVideoInfoTag() && m_mark == (item->GetVideoInfoTag()->GetPlayCount() > 0))
      continue;

#ifdef HAS_UPNP
    if (URIUtils::IsUPnP(item->GetPath()) && UPNP::CUPnP::MarkWatched(*item, m_mark))
      continue;
#endif

    if (item->HasPVRRecordingInfoTag() && CServiceBroker::GetPVRManager().Recordings()->MarkWatched(item, m_mark))
      continue;

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
      if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->GetPath().empty())
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
