/*
 *      Copyright (C) 2018 Team Kodi
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

#include "DialogGameAdvancedSettings.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "addons/AddonManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "ServiceBroker.h"

using namespace KODI;
using namespace GAME;

CDialogGameAdvancedSettings::CDialogGameAdvancedSettings() :
  CGUIDialog(WINDOW_DIALOG_GAME_ADVANCED_SETTINGS, "")
{
}

bool CDialogGameAdvancedSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
  {
    auto gameSettingsHandle = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
    if (gameSettingsHandle)
    {
      ADDON::AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(gameSettingsHandle->GameClientID(), addon, ADDON::ADDON_GAMEDLL))
      {
        gameSettingsHandle.reset();
        CGUIDialogAddonSettings::ShowForAddon(addon);
      }
    }

    return false;
  }
  default:
    break;
  }

  return CGUIDialog::OnMessage(message);
}
