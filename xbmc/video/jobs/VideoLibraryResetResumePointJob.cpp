/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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

#include "VideoLibraryResetResumePointJob.h"

#include <vector>

#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/IDirectory.h"
#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif
#include "profiles/ProfilesManager.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

CVideoLibraryResetResumePointJob::CVideoLibraryResetResumePointJob(const CFileItemPtr item)
  : m_item(item)
{
}

bool CVideoLibraryResetResumePointJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CVideoLibraryResetResumePointJob* resetJob = dynamic_cast<const CVideoLibraryResetResumePointJob*>(job);
  if (!resetJob)
    return false;

  return m_item->IsSamePath(resetJob->m_item.get());
}

bool CVideoLibraryResetResumePointJob::Work(CVideoDatabase &db)
{
  if (!CProfilesManager::GetInstance().GetCurrentProfile().canWriteDatabases())
    return false;

  CFileItemList items;
  items.Add(std::make_shared<CFileItem>(*m_item));

  if (m_item->m_bIsFolder)
    CUtil::GetRecursiveListing(m_item->GetPath(), items, "", XFILE::DIR_FLAG_NO_FILE_INFO);

  std::vector<CFileItemPtr> resetItems;
  for (const auto& item : items)
  {
#ifdef HAS_UPNP
    if (URIUtils::IsUPnP(item->GetPath()) && UPNP::CUPnP::SaveFileState(*item, CBookmark(), false /* updatePlayCount */))
      continue;
#endif

    if (item->HasPVRRecordingInfoTag() && CServiceBroker::GetPVRManager().Recordings()->ResetResumePoint(item))
      continue;

    resetItems.emplace_back(item);
  }

  if (resetItems.empty())
    return true;

  db.BeginTransaction();

  for (const auto& resetItem : resetItems)
  {
    db.DeleteResumeBookMark(*resetItem);
  }

  db.CommitTransaction();
  db.Close();

  return true;
}
