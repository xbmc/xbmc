/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIChannelIconUpdater.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/guilib/PVRGUIProgressHandler.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <map>
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

  CPVRGUIProgressHandler* progressHandler = new CPVRGUIProgressHandler(g_localizeStrings.Get(19286)); // Searching for channel icons

  for (const auto& group : m_groups)
  {
    const std::vector<std::shared_ptr<PVRChannelGroupMember>> members = group->GetMembers();
    int channelIndex = 0;
    for (const auto& member : members)
    {
      progressHandler->UpdateProgress(member->channel->ChannelName(), channelIndex++, members.size());

      // skip if an icon is already set and exists
      if (XFILE::CFile::Exists(member->channel->IconPath()))
        continue;

      // reset icon before searching for a new one
      member->channel->SetIconPath("");

      const std::string strChannelUid = StringUtils::Format("%08d", member->channel->UniqueID());
      std::string strLegalClientChannelName = CUtil::MakeLegalFileName(member->channel->ClientChannelName());
      StringUtils::ToLower(strLegalClientChannelName);
      std::string strLegalChannelName = CUtil::MakeLegalFileName(member->channel->ChannelName());
      StringUtils::ToLower(strLegalChannelName);

      std::map<std::string, std::string>::iterator itItem;
      if ((itItem = fileItemMap.find(strLegalClientChannelName)) != fileItemMap.end() ||
          (itItem = fileItemMap.find(strLegalChannelName)) != fileItemMap.end() ||
          (itItem = fileItemMap.find(strChannelUid)) != fileItemMap.end())
      {
        member->channel->SetIconPath(itItem->second, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bPVRAutoScanIconsUserSet);
      }

      if (m_bUpdateDb)
        member->channel->Persist();
    }
  }

  progressHandler->DestroyProgress();
}
