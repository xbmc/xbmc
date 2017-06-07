/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "Application.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "settings/Settings.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"

#include "PVRActionListener.h"

namespace PVR
{

CPVRActionListener::CPVRActionListener()
{
  g_application.RegisterActionListener(this);
  CServiceBroker::GetSettings().RegisterCallback(this, {
    CSettings::SETTING_PVRPARENTAL_ENABLED,
    CSettings::SETTING_PVRMANAGER_RESETDB,
    CSettings::SETTING_EPG_RESETEPG,
    CSettings::SETTING_PVRMANAGER_CHANNELMANAGER,
    CSettings::SETTING_PVRMANAGER_GROUPMANAGER,
    CSettings::SETTING_PVRMANAGER_CHANNELSCAN,
    CSettings::SETTING_PVRMENU_SEARCHICONS,
    CSettings::SETTING_PVRCLIENT_MENUHOOK,
    CSettings::SETTING_EPG_DAYSTODISPLAY
  });
}

CPVRActionListener::~CPVRActionListener()
{
  CServiceBroker::GetSettings().UnregisterCallback(this);
  g_application.UnregisterActionListener(this);
}

bool CPVRActionListener::OnAction(const CAction &action)
{
  bool bIsJumpSMS = false;

  switch (action.GetID())
  {
    case ACTION_PVR_PLAY:
    case ACTION_PVR_PLAY_TV:
    case ACTION_PVR_PLAY_RADIO:
    {
      // see if we're already playing a PVR stream and if not or the stream type
      // doesn't match the demanded type, start playback of according type
      bool isPlayingPvr(CServiceBroker::GetPVRManager().IsPlaying() && g_application.CurrentFileItem().HasPVRChannelInfoTag());
      switch (action.GetID())
      {
        case ACTION_PVR_PLAY:
          if (!isPlayingPvr)
            CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(PlaybackTypeAny);
          break;
        case ACTION_PVR_PLAY_TV:
          if (!isPlayingPvr || g_application.CurrentFileItem().GetPVRChannelInfoTag()->IsRadio())
            CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(PlaybackTypeTV);
          break;
        case ACTION_PVR_PLAY_RADIO:
          if (!isPlayingPvr || !g_application.CurrentFileItem().GetPVRChannelInfoTag()->IsRadio())
            CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(PlaybackTypeRadio);
          break;
      }
      return true;
    }

    case ACTION_JUMP_SMS2:
    case ACTION_JUMP_SMS3:
    case ACTION_JUMP_SMS4:
    case ACTION_JUMP_SMS5:
    case ACTION_JUMP_SMS6:
    case ACTION_JUMP_SMS7:
    case ACTION_JUMP_SMS8:
    case ACTION_JUMP_SMS9:
      bIsJumpSMS = true;
      // fallthru is intended
    case REMOTE_0:
    case REMOTE_1:
    case REMOTE_2:
    case REMOTE_3:
    case REMOTE_4:
    case REMOTE_5:
    case REMOTE_6:
    case REMOTE_7:
    case REMOTE_8:
    case REMOTE_9:
    {
      if (g_application.CurrentFileItem().IsLiveTV() &&
          (g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_CHANNELS) ||
           g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_GUIDE)))
      {
        // do not consume action if a python modal is the top most dialog
        // as a python modal can't return that it consumed the action.
        if (g_windowManager.IsPythonWindow(g_windowManager.GetTopMostModalDialogID()))
          return false;

        int iRemote = bIsJumpSMS ? action.GetID() - (ACTION_JUMP_SMS2 - REMOTE_2) : action.GetID();
        CServiceBroker::GetPVRManager().GUIActions()->GetChannelNumberInputHandler().AppendChannelNumberDigit(iRemote - REMOTE_0);
        return true;
      }
    }
    break;
  }
  return false;
}

void CPVRActionListener::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_PVRPARENTAL_ENABLED)
  {
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue() && CServiceBroker::GetSettings().GetString(CSettings::SETTING_PVRPARENTAL_PIN).empty())
    {
      std::string newPassword = "";
      // password set... save it
      if (CGUIDialogNumeric::ShowAndVerifyNewPassword(newPassword))
        CServiceBroker::GetSettings().SetString(CSettings::SETTING_PVRPARENTAL_PIN, newPassword);
      // password not set... disable parental
      else
        std::static_pointer_cast<CSettingBool>(std::const_pointer_cast<CSetting>(setting))->SetValue(false);
    }
  }
  else if (settingId == CSettings::SETTING_EPG_DAYSTODISPLAY)
  {
    CServiceBroker::GetPVRManager().Clients()->SetEPGTimeFrame(std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  }
}

void CPVRActionListener::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == CSettings::SETTING_PVRMANAGER_RESETDB)
  {
    CServiceBroker::GetPVRManager().GUIActions()->ResetPVRDatabase(false);
  }
  else if (settingId == CSettings::SETTING_EPG_RESETEPG)
  {
    CServiceBroker::GetPVRManager().GUIActions()->ResetPVRDatabase(true);
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_CHANNELMANAGER)
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      CGUIDialog *dialog = g_windowManager.GetDialog(WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
      if (dialog)
        dialog->Open();
    }
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_GROUPMANAGER)
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      CGUIDialog *dialog = g_windowManager.GetDialog(WINDOW_DIALOG_PVR_GROUP_MANAGER);
      if (dialog)
        dialog->Open();
    }
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_CHANNELSCAN)
  {
    CServiceBroker::GetPVRManager().GUIActions()->StartChannelScan();
  }
  else if (settingId == CSettings::SETTING_PVRMENU_SEARCHICONS)
  {
    CServiceBroker::GetPVRManager().TriggerSearchMissingChannelIcons();
  }
  else if (settingId == CSettings::SETTING_PVRCLIENT_MENUHOOK)
  {
    CServiceBroker::GetPVRManager().GUIActions()->ProcessMenuHooks(CFileItemPtr());
  }
}

} // namespace PVR
