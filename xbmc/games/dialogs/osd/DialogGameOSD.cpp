/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DialogGameOSD.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "input/Action.h"
#include "input/ActionIDs.h"

using namespace KODI;
using namespace GAME;

CDialogGameOSD::CDialogGameOSD() :
  CGUIDialog(WINDOW_DIALOG_GAME_OSD, "GameOSD.xml")
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

bool CDialogGameOSD::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_SHOW_OSD:
  {
    Close();
    return true;
  }
  case ACTION_PLAYER_PLAY:
  case ACTION_PLAYER_RESET:
  case ACTION_PREV_ITEM:
  case ACTION_STOP:
  {
    Close(true);
    break;
  }
  default:
    break;
  }

  return CGUIDialog::OnAction(action);
}

bool CDialogGameOSD::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:  // Fired when OSD is hidden
  {
    CloseSubDialogs();
    break;
  }
  default:
    break;
  }

  return CGUIDialog::OnMessage(message);
}

std::vector<int> CDialogGameOSD::GetSubDialogs()
{
  return std::vector<int>{
    WINDOW_DIALOG_GAME_VIDEO_SETTINGS,
    WINDOW_DIALOG_GAME_CONTROLLERS,
  };
}

void CDialogGameOSD::CloseSubDialogs()
{
  // Remove our subdialogs if visible
  for (auto windowId : GetSubDialogs())
  {
    CGUIDialog *pDialog = g_windowManager.GetDialog(windowId);
    if (pDialog)
      pDialog->Close(true);
  }
}
