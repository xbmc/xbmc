/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRActionListener.h"

#include "Application.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/Key.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include "pvr/PVRGUIActions.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

namespace PVR
{

CPVRActionListener::CPVRActionListener()
{
  g_application.RegisterActionListener(this);
  CServiceBroker::GetSettingsComponent()->GetSettings()->RegisterCallback(this, {
    CSettings::SETTING_PVRPARENTAL_ENABLED,
    CSettings::SETTING_PVRMANAGER_RESETDB,
    CSettings::SETTING_EPG_RESETEPG,
    CSettings::SETTING_PVRMANAGER_CLIENTPRIORITIES,
    CSettings::SETTING_PVRMANAGER_CHANNELMANAGER,
    CSettings::SETTING_PVRMANAGER_GROUPMANAGER,
    CSettings::SETTING_PVRMANAGER_CHANNELSCAN,
    CSettings::SETTING_PVRMENU_SEARCHICONS,
    CSettings::SETTING_PVRCLIENT_MENUHOOK,
    CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY
  });
}

CPVRActionListener::~CPVRActionListener()
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->UnregisterCallback(this);
  g_application.UnregisterActionListener(this);
}

void CPVRActionListener::Init(CPVRManager& mgr)
{
  mgr.Events().Subscribe(this, &CPVRActionListener::OnPVRManagerEvent);
}

void CPVRActionListener::Deinit(CPVRManager& mgr)
{
  mgr.Events().Unsubscribe(this);
}

void CPVRActionListener::OnPVRManagerEvent(const PVREvent& event)
{
  if (event == PVREvent::AnnounceReminder)
  {
    // dispatch to GUI thread and handle the action there
    KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID,
                                                                  -1,
                                                                  static_cast<void*>(new CAction(ACTION_PVR_ANNOUNCE_REMINDERS)));
  }
}

ChannelSwitchMode CPVRActionListener::GetChannelSwitchMode(int iAction)
{
  if ((iAction == ACTION_MOVE_UP || iAction == ACTION_MOVE_DOWN) &&
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH))
    return ChannelSwitchMode::NO_SWITCH;

  return ChannelSwitchMode::INSTANT_OR_DELAYED_SWITCH;
}

bool CPVRActionListener::OnAction(const CAction &action)
{
  bool bIsJumpSMS = false;
  bool bIsPlayingPVR(CServiceBroker::GetPVRManager().IsPlaying() &&
                     g_application.CurrentFileItem().HasPVRChannelInfoTag());

  switch (action.GetID())
  {
    case ACTION_PVR_PLAY:
    case ACTION_PVR_PLAY_TV:
    case ACTION_PVR_PLAY_RADIO:
    {
      // see if we're already playing a PVR stream and if not or the stream type
      // doesn't match the demanded type, start playback of according type
      switch (action.GetID())
      {
        case ACTION_PVR_PLAY:
          if (!bIsPlayingPVR)
            CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(PlaybackTypeAny);
          break;
        case ACTION_PVR_PLAY_TV:
          if (!bIsPlayingPVR || g_application.CurrentFileItem().GetPVRChannelInfoTag()->IsRadio())
            CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(PlaybackTypeTV);
          break;
        case ACTION_PVR_PLAY_RADIO:
          if (!bIsPlayingPVR || !g_application.CurrentFileItem().GetPVRChannelInfoTag()->IsRadio())
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
    case ACTION_CHANNEL_NUMBER_SEP:
    {
      if (!bIsPlayingPVR)
        return false;

      if (CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_FULLSCREEN_VIDEO) || CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_VISUALISATION))
      {
        // do not consume action if a python modal is the top most dialog
        // as a python modal can't return that it consumed the action.
        if (CServiceBroker::GetGUI()->GetWindowManager().IsPythonWindow(CServiceBroker::GetGUI()->GetWindowManager().GetTopmostModalDialog()))
          return false;

        char cCharacter;
        if (action.GetID() == ACTION_CHANNEL_NUMBER_SEP)
        {
          cCharacter = CPVRChannelNumber::SEPARATOR;
        }
        else
        {
          int iRemote = bIsJumpSMS ? action.GetID() - (ACTION_JUMP_SMS2 - REMOTE_2) : action.GetID();
          cCharacter = (iRemote - REMOTE_0) + '0';
        }
        CServiceBroker::GetPVRManager().GUIActions()->GetChannelNumberInputHandler().AppendChannelNumberCharacter(cCharacter);
        return true;
      }
      return false;
    }

    case ACTION_SHOW_INFO:
    {
      if (!bIsPlayingPVR)
        return false;

      CServiceBroker::GetPVRManager().GUIActions()->GetChannelNavigator().ToggleInfo();
      return true;
    }

    case ACTION_SELECT_ITEM:
    {
      if (!bIsPlayingPVR)
        return false;

      // If the button that caused this action matches action "Select" ...
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRPLAYBACK_CONFIRMCHANNELSWITCH) &&
          CServiceBroker::GetPVRManager().GUIActions()->GetChannelNavigator().IsPreview())
      {
        // ... and if "confirm channel switch" setting is active and a channel
        // preview is currently shown, switch to the currently previewed channel.
        CServiceBroker::GetPVRManager().GUIActions()->GetChannelNavigator().SwitchToCurrentChannel();
        return true;
      }
      else if (CServiceBroker::GetPVRManager().GUIActions()->GetChannelNumberInputHandler().CheckInputAndExecuteAction())
      {
        // ... or if the action was processed by direct channel number input, we're done.
        return true;
      }
      return false;
    }

    case ACTION_NEXT_ITEM:
    {
      if (!bIsPlayingPVR)
        return false;

      CServiceBroker::GetPVRManager().GUIActions()->SeekForward();
      return true;
    }

    case ACTION_PREV_ITEM:
    {
      if (!bIsPlayingPVR)
        return false;

      CServiceBroker::GetPVRManager().GUIActions()->SeekBackward(CApplication::ACTION_PREV_ITEM_THRESHOLD);
      return true;
    }

    case ACTION_MOVE_UP:
    case ACTION_CHANNEL_UP:
    {
      if (!bIsPlayingPVR)
        return false;

      CServiceBroker::GetPVRManager().GUIActions()->GetChannelNavigator().SelectNextChannel(GetChannelSwitchMode(action.GetID()));
      return true;
    }

    case ACTION_MOVE_DOWN:
    case ACTION_CHANNEL_DOWN:
    {
      if (!bIsPlayingPVR)
        return false;

      CServiceBroker::GetPVRManager().GUIActions()->GetChannelNavigator().SelectPreviousChannel(GetChannelSwitchMode(action.GetID()));
      return true;
    }

    case ACTION_CHANNEL_SWITCH:
    {
      if (!bIsPlayingPVR)
        return false;

      int iChannelNumber = static_cast<int>(action.GetAmount(0));
      int iSubChannelNumber = static_cast<int>(action.GetAmount(1));

      const CPVRChannelPtr currentChannel = CServiceBroker::GetPVRManager().GetPlayingChannel();
      const CPVRChannelGroupPtr selectedGroup = CServiceBroker::GetPVRManager().ChannelGroups()->Get(currentChannel->IsRadio())->GetSelectedGroup();
      const CFileItemPtr item(selectedGroup->GetByChannelNumber(CPVRChannelNumber(iChannelNumber, iSubChannelNumber)));

      if (!item)
        return false;

      CServiceBroker::GetPVRManager().GUIActions()->SwitchToChannel(item, false);
      return true;
    }

    case ACTION_RECORD:
    {
      CServiceBroker::GetPVRManager().GUIActions()->ToggleRecordingOnPlayingChannel();
      return true;
    }

    case ACTION_PVR_ANNOUNCE_REMINDERS:
    {
      CServiceBroker::GetPVRManager().GUIActions()->AnnounceReminders();
      return true;
    }
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
    if (std::static_pointer_cast<const CSettingBool>(setting)->GetValue() && CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_PVRPARENTAL_PIN).empty())
    {
      std::string newPassword = "";
      // password set... save it
      if (CGUIDialogNumeric::ShowAndVerifyNewPassword(newPassword))
        CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(CSettings::SETTING_PVRPARENTAL_PIN, newPassword);
      // password not set... disable parental
      else
        std::static_pointer_cast<CSettingBool>(std::const_pointer_cast<CSetting>(setting))->SetValue(false);
    }
  }
  else if (settingId == CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY)
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
  else if (settingId == CSettings::SETTING_PVRMANAGER_CLIENTPRIORITIES)
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      CGUIDialog *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_PVR_CLIENT_PRIORITIES);
      if (dialog)
      {
        dialog->Open();
        CServiceBroker::GetPVRManager().ChannelGroups()->Update();
      }
    }
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_CHANNELMANAGER)
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      CGUIDialog *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
      if (dialog)
        dialog->Open();
    }
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_GROUPMANAGER)
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      CGUIDialog *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_PVR_GROUP_MANAGER);
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
    CServiceBroker::GetPVRManager().GUIActions()->ProcessSettingsMenuHooks();
  }
}

} // namespace PVR
