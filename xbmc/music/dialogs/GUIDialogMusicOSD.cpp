/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogMusicOSD.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/gui/GUIWindowAddonBrowser.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/InputManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#define CONTROL_VIS_BUTTON       500
#define CONTROL_LOCK_BUTTON      501

CGUIDialogMusicOSD::CGUIDialogMusicOSD(void)
    : CGUIDialog(WINDOW_DIALOG_MUSIC_OSD, "MusicOSD.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogMusicOSD::~CGUIDialogMusicOSD(void) = default;

bool CGUIDialogMusicOSD::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl == CONTROL_VIS_BUTTON)
      {
        std::string addonID;
        if (CGUIWindowAddonBrowser::SelectAddonID(ADDON::AddonType::VISUALIZATION, addonID, true) ==
            1)
        {
          const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
          settings->SetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION, addonID);
          settings->Save();
          CServiceBroker::GetGUI()->GetWindowManager().SendMessage(GUI_MSG_VISUALISATION_RELOAD, 0, 0);
        }
      }
      else if (iControl == CONTROL_LOCK_BUTTON)
      {
        CGUIMessage msg(GUI_MSG_VISUALISATION_ACTION, 0, 0, ACTION_VIS_PRESET_LOCK);
        CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
      }
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogMusicOSD::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_SHOW_OSD:
      Close();
      return true;
    default:
      break;
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogMusicOSD::FrameMove()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (CServiceBroker::GetInputManager().IsMouseActive() ||
        CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_VIS_SETTINGS) ||
        CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_VIS_PRESET_LIST) ||
        CServiceBroker::GetGUI()->GetWindowManager().IsWindowActive(WINDOW_DIALOG_PVR_RADIO_RDS_INFO))
      // extend show time by original value
      SetAutoClose(m_showDuration);
  }
  CGUIDialog::FrameMove();
}
