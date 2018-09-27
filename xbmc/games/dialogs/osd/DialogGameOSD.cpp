/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameOSD.h"
#include "DialogGameOSDHelp.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace GAME;

CDialogGameOSD::CDialogGameOSD() :
  CGUIDialog(WINDOW_DIALOG_GAME_OSD, "GameOSD.xml"),
  m_helpDialog(new CDialogGameOSDHelp(*this))
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

bool CDialogGameOSD::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_PARENT_DIR:
    case ACTION_PREVIOUS_MENU:
    case ACTION_NAV_BACK:
    case ACTION_SHOW_OSD:
    case ACTION_PLAYER_PLAY:
    {
      // Disable OSD help if visible
      if (m_helpDialog->IsVisible() && CServiceBroker::IsServiceManagerUp())
      {
        GAME::CGameSettings &gameSettings = CServiceBroker::GetGameServices().GameSettings();
        if (gameSettings.ShowOSDHelp())
        {
          gameSettings.SetShowOSDHelp(false);
          return true;
        }
      }
      break;
    }
    default:
      break;
  }

  return CGUIDialog::OnAction(action);
}

void CDialogGameOSD::OnInitWindow()
{
  // Init parent class
  CGUIDialog::OnInitWindow();

  // Init help dialog
  m_helpDialog->OnInitWindow();
}

void CDialogGameOSD::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  if (CServiceBroker::IsServiceManagerUp())
  {
    GAME::CGameSettings &gameSettings = CServiceBroker::GetGameServices().GameSettings();
    gameSettings.SetShowOSDHelp(false);
  }
}

bool CDialogGameOSD::PlayInBackground(int dialogId)
{
  return dialogId == WINDOW_DIALOG_GAME_VOLUME;
}
