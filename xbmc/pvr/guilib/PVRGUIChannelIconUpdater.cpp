/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIChannelIconUpdater.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/guilib/PVRGUIProgressHandler.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace PVR;

void CPVRGUIChannelIconUpdater::SearchAndUpdateMissingChannelIcons() const
{
  const std::string iconPath = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_PVRMENU_ICONPATH);
  if (iconPath.empty())
    return;

  // fetch files in icon path for fast lookup
  CFileItemList fileItemList;
  XFILE::CDirectory::GetDirectory(iconPath, fileItemList, ".jpg|.png|.tbn", XFILE::DIR_FLAG_DEFAULTS);

  if (fileItemList.IsEmpty())
    return;

  CLog::Log(LOGINFO, "Starting PVR channel icon search");

  // create a map for fast lookup of normalized file base name
  std::map<std::string, std::string> fileItemMap;
  for (const auto& item : fileItemList)
  {
    std::string baseName = URIUtils::GetFileName(item->GetPath());
    URIUtils::RemoveExtension(baseName);
    StringUtils::ToLower(baseName);
    fileItemMap.insert({baseName, item->GetPath()});
  }

  std::unique_ptr<CPVRGUIProgressHandler> progressHandler;
  if (!m_groups.empty())
    progressHandler = std::make_unique<CPVRGUIProgressHandler>(
        g_localizeStrings.Get(19286)); // Searching for channel icons

  for (const auto& group : m_groups)
  {
    const std::vector<std::shared_ptr<CPVRChannelGroupMember>> members = group->GetMembers();
    int channelIndex = 0;
    for (const auto& member : members)
    {
      const std::shared_ptr<CPVRChannel> channel = member->Channel();

      progressHandler->UpdateProgress(channel->ChannelName(), channelIndex++, members.size());

      // skip if an icon is already set and exists
      if (CFileUtils::Exists(channel->IconPath()))
        continue;

      // reset icon before searching for a new one
      channel->SetIconPath("");

      const std::string strChannelUid = StringUtils::Format("{:08}", channel->UniqueID());
      std::string strLegalClientChannelName =
          CUtil::MakeLegalFileName(channel->ClientChannelName());
      StringUtils::ToLower(strLegalClientChannelName);
      std::string strLegalChannelName = CUtil::MakeLegalFileName(channel->ChannelName());
      StringUtils::ToLower(strLegalChannelName);

      std::map<std::string, std::string>::iterator itItem;
      if ((itItem = fileItemMap.find(strLegalClientChannelName)) != fileItemMap.end() ||
          (itItem = fileItemMap.find(strLegalChannelName)) != fileItemMap.end() ||
          (itItem = fileItemMap.find(strChannelUid)) != fileItemMap.end())
      {
        channel->SetIconPath(itItem->second, CServiceBroker::GetSettingsComponent()
                                                 ->GetAdvancedSettings()
                                                 ->m_bPVRAutoScanIconsUserSet);
      }

      if (m_bUpdateDb)
        channel->Persist();
    }
  }
}
