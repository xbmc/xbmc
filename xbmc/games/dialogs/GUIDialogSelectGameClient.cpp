/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include "GUIDialogSelectGameClient.h"
#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "games/addons/GameClient.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h" 
#include "utils/log.h"

using namespace KODI;
using namespace KODI::MESSAGING;
using namespace GAME;

std::string CGUIDialogSelectGameClient::ShowAndGetGameClient(const GameClientVector& candidates, const GameClientVector& installable)
{
  std::string gameClient;

  CLog::Log(LOGDEBUG, "Select game client dialog: Found %lu candidates", candidates.size());
  for (const auto& gameClient : candidates)
    CLog::Log(LOGDEBUG, "Adding %s as a candidate", gameClient->ID().c_str());

  if (!installable.empty())
  {
    CLog::Log(LOGDEBUG, "Select game client dialog: Found %lu installable clients", installable.size());
    for (const auto& gameClient : installable)
      CLog::Log(LOGDEBUG, "Adding %s as an installable client", gameClient->ID().c_str());
  }

  CContextButtons choiceButtons;

  // Add emulators
  int i = 0;
  for (const GameClientPtr& gameClient : candidates)
    choiceButtons.Add(i++, gameClient->Name());

  // Add button to install emulators
  const int iInstallEmulator = i++;
  if (!installable.empty())
    choiceButtons.Add(iInstallEmulator, 35253); // "Install emulator"

  // Add button to manage emulators
  const int iAddonMgr = i++;
  choiceButtons.Add(iAddonMgr, 35254); // "Manage emulators"

  // Do modal
  int result = CGUIDialogContextMenu::ShowAndGetChoice(choiceButtons);

  if (0 <= result && result < static_cast<int>(candidates.size()))
  {
    // Handle emulator
    gameClient = candidates[result]->ID();
  }
  else if (result == iInstallEmulator)
  {
    // Install emulator
    gameClient = InstallGameClient(installable);
  }
  else if (result == iAddonMgr)
  {
    // Go to add-on manager to manage emulators
    ActivateAddonMgr();
  }
  else
  {
    CLog::Log(LOGDEBUG, "Select game client dialog: User cancelled game client selection");
  }

  return gameClient;
}

std::string CGUIDialogSelectGameClient::InstallGameClient(const GameClientVector& installable)
{
  using namespace ADDON;

  std::string gameClient;

  //! @todo Switch to add-on browser when more emulators have icons
  /*
  std::string chosenClientId;
  if (CGUIWindowAddonBrowser::SelectAddonID(ADDON_GAMEDLL, chosenClientId, false, true, false, true, false) >= 0 && !chosenClientId.empty())
  {
    CLog::Log(LOGDEBUG, "Select game client dialog: User installed %s", chosenClientId.c_str());
    AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(chosenClientId, addon, ADDON_GAMEDLL))
      gameClient = addon->ID();

    if (gameClient.empty())
      CLog::Log(LOGERROR, "Select game client dialog: Failed to get addon %s", chosenClientId.c_str());
  }
  */

  CContextButtons choiceButtons;

  // Add emulators
  int i = 0;
  for (const GameClientPtr& gameClient : installable)
    choiceButtons.Add(i++, gameClient->Name());

  // Add button to browser all emulators
  const int iAddonBrowser = i++;
  choiceButtons.Add(iAddonBrowser, 35255); // "Browse all emulators"

  // Do modal
  int result = CGUIDialogContextMenu::ShowAndGetChoice(choiceButtons);

  if (0 <= result && result < static_cast<int>(installable.size()))
  {
    std::string gameClientId = installable[result]->ID();
    CLog::Log(LOGDEBUG, "Select game client dialog: Installing %s", gameClientId.c_str());
    AddonPtr installedAddon;
    if (CAddonInstaller::GetInstance().InstallModal(gameClientId, installedAddon, false))
    {
      CLog::Log(LOGDEBUG, "Select game client dialog: Successfully installed %s", installedAddon->ID().c_str());

      // if the addon is disabled we need to enable it
      if (CServiceBroker::GetAddonMgr().IsAddonDisabled(installedAddon->ID()))
        CServiceBroker::GetAddonMgr().EnableAddon(installedAddon->ID());

      gameClient = installedAddon->ID();
    }
    else
    {
      CLog::Log(LOGERROR, "Select game client dialog: Failed to install %s", gameClientId.c_str());
      // "Error"
      // "Failed to install add-on."
      HELPERS::ShowOKDialogText(257, 35256);
    }
  }
  else if (result == iAddonBrowser)
  {
    ActivateAddonBrowser();
  }
  else
  {
    CLog::Log(LOGDEBUG, "Select game client dialog: User cancelled game client installation");
  }

  return gameClient;
}

void CGUIDialogSelectGameClient::ActivateAddonMgr()
{
  CLog::Log(LOGDEBUG, "User chose to go to the add-on manager");
  std::vector<std::string> params;
  params.push_back("addons://user/category.emulators");
  g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
}

void CGUIDialogSelectGameClient::ActivateAddonBrowser()
{
  CLog::Log(LOGDEBUG, "User chose to go to the add-on browser");
  std::vector<std::string> params;
  params.push_back("addons://all/category.emulators");
  g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
}
