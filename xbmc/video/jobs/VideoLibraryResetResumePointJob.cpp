/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoLibraryResetResumePointJob.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/IDirectory.h"

#include <vector>
#ifdef HAS_UPNP
#include "network/upnp/UPnP.h"
#endif
#include "profiles/ProfileManager.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

CVideoLibraryResetResumePointJob::CVideoLibraryResetResumePointJob(
    const std::shared_ptr<CFileItem>& item)
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
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (!profileManager->GetCurrentProfile().canWriteDatabases())
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

    if (item->HasPVRRecordingInfoTag() &&
        CServiceBroker::GetPVRManager().Recordings()->ResetResumePoint(item->GetPVRRecordingInfoTag()))
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
