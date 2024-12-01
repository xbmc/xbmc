/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsUtils.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/filesystem/PVRGUIDirectory.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "pvr/recordings/PVRRecordings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

namespace PVR
{
bool CPVRGUIActionsUtils::HasInfoForItem(const CFileItem& item) const
{
  return item.HasPVRRecordingInfoTag() || item.HasPVRChannelInfoTag() ||
         item.HasPVRTimerInfoTag() || item.HasEPGSearchFilter();
}

bool CPVRGUIActionsUtils::OnInfo(const CFileItem& item)
{
  if (item.HasPVRRecordingInfoTag())
  {
    return CServiceBroker::GetPVRManager().Get<PVR::GUI::Recordings>().ShowRecordingInfo(item);
  }
  else if (item.HasPVRChannelInfoTag() || item.HasPVRTimerInfoTag())
  {
    return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().ShowEPGInfo(item);
  }
  else if (item.HasEPGSearchFilter())
  {
    return CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().EditSavedSearch(item);
  }
  return false;
}

namespace
{
std::shared_ptr<CFileItem> LoadRecordingFileOrFolderItem(const CFileItem& item)
{
  if (URIUtils::IsPVRRecordingFileOrFolder(item.GetPath()))
  {
    //! @todo prop misused to detect loaded state for recording folder item
    if (item.HasPVRRecordingInfoTag() || item.HasProperty("watchedepisodes"))
      return std::make_shared<CFileItem>(item); // already loaded

    if (item.m_bIsFolder)
    {
      CFileItem loadedItem{item};
      if (CPVRGUIDirectory::GetRecordingsDirectoryInfo(loadedItem))
        return std::make_shared<CFileItem>(loadedItem);
    }
    else
    {
      // recording without info tag; find by path
      const std::shared_ptr<CPVRRecording> recording{
          CServiceBroker::GetPVRManager().Recordings()->GetByPath(item.GetPath())};
      if (recording)
        return std::make_shared<CFileItem>(recording);
    }
  }
  return {};
}

std::shared_ptr<CFileItem> LoadChannelItem(const CFileItem& item)
{
  if (URIUtils::IsPVRChannel(item.GetPath()))
  {
    if (item.HasPVRChannelInfoTag())
      return std::make_shared<CFileItem>(item); // already loaded

    const auto groups{CServiceBroker::GetPVRManager().ChannelGroups()};
    const std::shared_ptr<CPVRChannelGroupMember> groupMember{
        groups->GetChannelGroupMemberByPath(item.GetPath())};
    if (groupMember)
      return std::make_shared<CFileItem>(groupMember);
  }
  return {};
}
} // unnamed namespace

std::shared_ptr<CFileItem> CPVRGUIActionsUtils::LoadItem(const CFileItem& item)
{
  std::shared_ptr<CFileItem> loadedItem{LoadRecordingFileOrFolderItem(item)};
  if (loadedItem)
    return loadedItem;

  loadedItem = LoadChannelItem(item);
  if (loadedItem)
    return loadedItem;

  CLog::LogFC(LOGWARNING, LOGPVR, "Error loading item details (path={})", item.GetPath());
  return {};
}

} // namespace PVR
