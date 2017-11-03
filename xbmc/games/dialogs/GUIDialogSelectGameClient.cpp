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
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/AddonsDirectory.h"
#include "games/addons/savestates/Savestate.h"
#include "games/addons/savestates/SavestateDatabase.h"
#include "games/addons/savestates/SavestateUtils.h"
#include "games/addons/GameClient.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h" 
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "ServiceBroker.h"
#include "URL.h"

using namespace KODI;
using namespace KODI::MESSAGING;
using namespace GAME;

std::string CGUIDialogSelectGameClient::ShowAndGetGameClient(const std::string &gamePath, const GameClientVector& candidates, const GameClientVector& installable)
{
  std::string gameClient;

  LogGameClients(candidates, installable);

  std::string extension = URIUtils::GetExtension(gamePath);
  std::string xmlPath = CSavestateUtils::MakeMetadataPath(gamePath);

  std::string selected;
  CSavestate save;
  CSavestateDatabase db;
  CLog::Log(LOGDEBUG, "Select game client dialog: Loading savestate metadata %s", CURL::GetRedacted(xmlPath).c_str());
  if (db.GetSavestate(xmlPath, save))
  {
    selected = save.GameClient();
    CLog::Log(LOGDEBUG, "Select game client dialog: Auto-selecting %s", selected.c_str());
  }

  // "Select emulator for {0:s}"
  CGUIDialogSelect *dialog = GetDialog(StringUtils::Format(g_localizeStrings.Get(35258), extension));
  if (dialog != nullptr)
  {
    // Turn the addons into items
    CFileItemList items;
    for (const auto &candidate : candidates)
    {
      CFileItemPtr item(XFILE::CAddonsDirectory::FileItemFromAddon(candidate, candidate->ID()));
      item->SetLabel2(g_localizeStrings.Get(35257)); // "Installed"
      if (item->GetPath() == selected)
        item->SetLabel2(item->GetLabel2() + ", " + g_localizeStrings.Get(35259)); // "Saved"
      items.Add(std::move(item));
    }
    for (const auto &addon : installable)
    {
      CFileItemPtr item(XFILE::CAddonsDirectory::FileItemFromAddon(addon, addon->ID()));
      items.Add(std::move(item));
    }
    items.Sort(SortByLabel, SortOrderAscending);

    dialog->SetItems(items);

    for (int i = 0; i < items.Size(); i++)
    {
      if (items[i]->GetPath() == selected)
        dialog->SetSelected(i);
    }

    dialog->Open();

    // If the "Get More" button has been pressed, show a list of installable addons
    if (dialog->IsConfirmed())
    {
      int selectedIndex = dialog->GetSelectedItem();

      if (0 <= selectedIndex && selectedIndex < items.Size())
      {
        gameClient = items[selectedIndex]->GetPath();

        CLog::Log(LOGDEBUG, "Select game client dialog: User selected emulator %s", gameClient.c_str());

        if (Install(gameClient))
        {
          // If the addon is disabled we need to enable it
          if (!Enable(gameClient))
            CLog::Log(LOGDEBUG, "Failed to enable game client %s", gameClient.c_str());
        }
        else
          CLog::Log(LOGDEBUG, "Failed to install game client: %s", gameClient.c_str());
      }
      else
      {
        CLog::Log(LOGDEBUG, "Select game client dialog: User selected invalid emulator %d", selectedIndex);
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "Select game client dialog: User cancelled game client installation");
    }
  }

  return gameClient;
}

bool CGUIDialogSelectGameClient::Install(const std::string &gameClient)
{
  // If the addon isn't installed we need to install it
  bool bInstalled = CServiceBroker::GetAddonMgr().IsAddonInstalled(gameClient);
  if (!bInstalled)
  {
    ADDON::AddonPtr installedAddon;
    bInstalled = CAddonInstaller::GetInstance().InstallModal(gameClient, installedAddon, false);
    if (!bInstalled)
    {
      CLog::Log(LOGERROR, "Select game client dialog: Failed to install %s", gameClient.c_str());
      // "Error"
      // "Failed to install add-on."
      HELPERS::ShowOKDialogText(257, 35256);
    }
  }

  return bInstalled;
}

bool CGUIDialogSelectGameClient::Enable(const std::string &gameClient)
{
  bool bSuccess = true;

  if (CServiceBroker::GetAddonMgr().IsAddonDisabled(gameClient))
    bSuccess = CServiceBroker::GetAddonMgr().EnableAddon(gameClient);

  return bSuccess;
}

CGUIDialogSelect *CGUIDialogSelectGameClient::GetDialog(const std::string &title)
{
  CGUIDialogSelect *dialog = g_windowManager.GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (dialog != nullptr)
  {
    dialog->Reset();
    dialog->SetHeading(CVariant{ title });
    dialog->SetUseDetails(true);
  }

  return dialog;
}

void CGUIDialogSelectGameClient::LogGameClients(const GameClientVector& candidates, const GameClientVector& installable)
{
  CLog::Log(LOGDEBUG, "Select game client dialog: Found %u candidates", static_cast<unsigned int>(candidates.size()));
  for (const auto& gameClient : candidates)
    CLog::Log(LOGDEBUG, "Adding %s as a candidate", gameClient->ID().c_str());

  if (!installable.empty())
  {
    CLog::Log(LOGDEBUG, "Select game client dialog: Found %u installable clients", static_cast<unsigned int>(installable.size()));
    for (const auto& gameClient : installable)
      CLog::Log(LOGDEBUG, "Adding %s as an installable client", gameClient->ID().c_str());
  }
}
