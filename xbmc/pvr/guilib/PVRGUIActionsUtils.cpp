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
#include "filesystem/Directory.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
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

    const std::string parentPath{URIUtils::GetParentPath(item.GetPath())};

    //! @todo optimize, find a way to set the details of the item without loading parent directory.
    CFileItemList items;
    if (XFILE::CDirectory::GetDirectory(parentPath, items, "", XFILE::DIR_FLAG_DEFAULTS))
    {
      const std::string& path{item.GetPath()};
      const auto it = std::find_if(items.cbegin(), items.cend(),
                                   [&path](const auto& entry) { return entry->GetPath() == path; });
      if (it != items.cend())
        return *it;
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
