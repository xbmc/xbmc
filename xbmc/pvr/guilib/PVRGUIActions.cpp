/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActions.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "settings/Settings.h"

#include <memory>
#include <mutex>
#include <string>

namespace PVR
{
CPVRGUIActions::CPVRGUIActions()
  : m_settings({CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL})
{
}

  void CPVRGUIActions::SetSelectedItemPath(bool bRadio, const std::string& path)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    if (bRadio)
      m_selectedItemPathRadio = path;
    else
      m_selectedItemPathTV = path;
  }

  std::string CPVRGUIActions::GetSelectedItemPath(bool bRadio) const
  {
    if (m_settings.GetBoolValue(CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL))
    {
      CPVRManager& mgr = CServiceBroker::GetPVRManager();

      // if preselect playing channel is activated, return the path of the playing channel, if any.
      const std::shared_ptr<CPVRChannel> playingChannel = mgr.PlaybackState()->GetPlayingChannel();
      if (playingChannel && playingChannel->IsRadio() == bRadio)
        return GetChannelGroupMember(playingChannel)->Path();

      const std::shared_ptr<CPVREpgInfoTag> playingTag = mgr.PlaybackState()->GetPlayingEpgTag();
      if (playingTag && playingTag->IsRadio() == bRadio)
      {
        const std::shared_ptr<CPVRChannel> channel =
            mgr.ChannelGroups()->GetChannelForEpgTag(playingTag);
        if (channel)
          return GetChannelGroupMember(channel)->Path();
      }
    }

    std::unique_lock<CCriticalSection> lock(m_critSection);
    return bRadio ? m_selectedItemPathRadio : m_selectedItemPathTV;
  }

  bool CPVRGUIActions::OnInfo(const std::shared_ptr<CFileItem>& item)
  {
    if (item->HasPVRRecordingInfoTag())
    {
      return ShowRecordingInfo(item);
    }
    else if (item->HasPVRChannelInfoTag() || item->HasPVRTimerInfoTag())
    {
      return ShowEPGInfo(item);
    }
    else if (item->HasEPGSearchFilter())
    {
      return EditSavedSearch(item);
    }
    return false;
  }

  } // namespace PVR
