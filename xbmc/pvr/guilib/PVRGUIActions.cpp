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
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClientMenuHooks.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "settings/Settings.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace KODI::MESSAGING;

namespace PVR
{
CPVRGUIActions::CPVRGUIActions()
  : m_settings({CSettings::SETTING_PVRMANAGER_PRESELECTPLAYINGCHANNEL})
{
}

  bool CPVRGUIActions::ProcessSettingsMenuHooks()
  {
    const CPVRClientMap clients = CServiceBroker::GetPVRManager().Clients()->GetCreatedClients();

    std::vector<std::pair<std::shared_ptr<CPVRClient>, CPVRClientMenuHook>> settingsHooks;
    for (const auto& client : clients)
    {
      const auto hooks = client.second->GetMenuHooks()->GetSettingsHooks();
      std::transform(hooks.cbegin(), hooks.cend(), std::back_inserter(settingsHooks),
                     [&client](const auto& hook) { return std::make_pair(client.second, hook); });
    }

    if (settingsHooks.empty())
    {
      HELPERS::ShowOKDialogText(
          CVariant{19033}, // "Information"
          CVariant{
              19347}); // "None of the active PVR clients does provide client-specific settings."
      return true; // no settings hooks, no error
    }

    auto selectedHook = settingsHooks.begin();

    // if there is only one settings hook, execute it directly, otherwise let the user select
    if (settingsHooks.size() > 1)
    {
      CGUIDialogSelect* pDialog= CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
      if (!pDialog)
      {
        CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_SELECT!");
        return false;
      }

      pDialog->Reset();
      pDialog->SetHeading(CVariant{19196}); // "PVR client specific actions"

      for (const auto& hook : settingsHooks)
      {
        if (clients.size() == 1)
          pDialog->Add(hook.second.GetLabel());
        else
          pDialog->Add(hook.first->GetBackendName() + ": " + hook.second.GetLabel());
      }

      pDialog->Open();

      int selection = pDialog->GetSelectedItem();
      if (selection < 0)
        return true; // cancelled

      std::advance(selectedHook, selection);
    }
    return selectedHook->first->CallSettingsMenuHook(selectedHook->second) == PVR_ERROR_NO_ERROR;
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
