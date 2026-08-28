/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameUtils.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/Addon.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/BinaryAddonCache.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/AddonsDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "games/addons/GameClient.h"
#include "games/database/GameDatabase.h"
#include "games/dialogs/GUIDialogSelectGameClient.h"
#include "games/dialogs/GUIDialogSelectSavestate.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

// Initialize static state
ADDON::VECADDONS CGameUtils::m_installableGameAddons;
bool CGameUtils::m_checkInstallable{false};
std::mutex CGameUtils::m_installableMutex;

bool CGameUtils::FillInGameClient(CFileItem& item, std::string& savestatePath)
{
  using namespace ADDON;

  if (item.GetGameInfoTag()->GetGameClient().empty())
  {
    // If the fileitem is an add-on, fall back to that
    if (item.HasAddonInfo() && item.GetAddonInfo()->Type() == AddonType::GAMEDLL)
    {
      item.GetGameInfoTag()->SetGameClient(item.GetAddonInfo()->ID());
    }
    else
    {
      if (!CGUIDialogSelectSavestate::ShowAndGetSavestate(item.GetPath(), savestatePath))
        return false;

      if (!savestatePath.empty())
      {
        RETRO::CSavestateDatabase db;
        std::unique_ptr<RETRO::ISavestate> save = RETRO::CSavestateDatabase::AllocateSavestate();
        db.GetSavestate(savestatePath, *save);
        item.GetGameInfoTag()->SetGameClient(save->GameClientID());
      }
      else
      {
        // No game client specified, need to ask the user
        GameClientVector candidates;
        GameClientVector installable;
        bool bHasVfsGameClient;
        GetGameClients(item, candidates, installable, bHasVfsGameClient);

        // An emulator remembered for this game, or for a folder above it,
        // answers the question without asking
        const std::string defaultClient = GetDefaultGameClient(item.GetPath(), candidates);
        if (!defaultClient.empty())
        {
          item.GetGameInfoTag()->SetGameClient(defaultClient);
        }
        else if (candidates.empty() && installable.empty())
        {
          // if: "This game can only be played directly from a hard drive or partition. Compressed files must be extracted."
          // else: "This game isn't compatible with any available emulators."
          int errorTextId = bHasVfsGameClient ? 35214 : 35212;

          // "Failed to play game"
          MESSAGING::HELPERS::ShowOKDialogText(CVariant{35210}, CVariant{errorTextId});
        }
        else if (candidates.size() == 1 && installable.empty())
        {
          // Only 1 option, avoid prompting the user
          item.GetGameInfoTag()->SetGameClient(candidates[0]->ID());
        }
        else
        {
          std::string gameClient = CGUIDialogSelectGameClient::ShowAndGetGameClient(
              item.GetPath(), candidates, installable);

          if (!gameClient.empty())
            item.GetGameInfoTag()->SetGameClient(gameClient);
        }
      }
    }
  }

  const std::string gameClient = item.GetGameInfoTag()->GetGameClient();
  if (gameClient.empty())
    return false;

  if (Install(gameClient))
  {
    // If the addon is disabled we need to enable it
    if (!Enable(gameClient))
    {
      CLog::Log(LOGDEBUG, "Failed to enable game client {}", gameClient);
      item.GetGameInfoTag()->SetGameClient("");
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "Failed to install game client: {}", gameClient);
    item.GetGameInfoTag()->SetGameClient("");
  }

  return !item.GetGameInfoTag()->GetGameClient().empty();
}

std::string CGameUtils::GetDefaultGameClient(const std::string& path,
                                             const GameClientVector& candidates)
{
  if (path.empty())
    return "";

  CGameDatabase db;
  if (!db.Open())
    return "";

  const std::string gameClient = db.GameClients().GetGameClientForGame(path);
  if (gameClient.empty())
    return "";

  // A remembered emulator is a preference, not an instruction. It is only used
  // if it can still open this game: one set on a folder has no idea what else
  // was put in that folder later, and one set before the emulator was
  // uninstalled would otherwise stop the game loading at all. Where it does not
  // fit, say so and let the user be asked, which is what would have happened
  // had nothing been remembered.
  const bool bCanOpen = std::any_of(candidates.begin(), candidates.end(),
                                    [&gameClient](const GameClientPtr& candidate)
                                    { return candidate->ID() == gameClient; });
  if (!bCanOpen)
  {
    CLog::Log(LOGDEBUG, "GAME: Ignoring remembered emulator {} for {}: it can't open this game",
              gameClient, CURL::GetRedacted(path));
    return "";
  }

  CLog::Log(LOGDEBUG, "GAME: Opening {} with remembered emulator {}", CURL::GetRedacted(path),
            gameClient);

  return gameClient;
}

bool CGameUtils::ChooseAndSetDefaultGameClient(const CFileItem& item)
{
  using namespace ADDON;

  const std::string path = item.GetPath();
  if (path.empty())
    return false;

  // A folder can be given anything later, so it offers every emulator that is
  // installed. A game only offers the ones that can open it.
  GameClientVector emulators;
  if (item.IsFolder())
  {
    VECADDONS addons;
    CServiceBroker::GetBinaryAddonCache().GetAddons(addons, AddonType::GAMEDLL);
    for (const auto& addon : addons)
      emulators.emplace_back(std::static_pointer_cast<CGameClient>(addon));
  }
  else
  {
    GameClientVector installable;
    bool bHasVfsGameClient = false;
    GetGameClients(item, emulators, installable, bHasVfsGameClient);
  }

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  if (dialog == nullptr)
    return false;

  CGameDatabase db;
  if (!db.Open())
    return false;

  const std::string currentGameClient = db.GameClients().GetGameClient(path);

  dialog->Reset();
  dialog->SetHeading(CVariant{35510}); // "Default emulator"
  dialog->SetUseDetails(true);

  CFileItemList items;

  // First, so that clearing is as easy to reach as setting
  {
    CFileItemPtr noneItem = std::make_shared<CFileItem>(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(231)); // "None"
    noneItem->SetPath("");
    items.Add(std::move(noneItem));
  }

  for (const auto& emulator : emulators)
  {
    CFileItemPtr emulatorItem(XFILE::CAddonsDirectory::FileItemFromAddon(emulator, emulator->ID()));
    if (emulator->ID() == currentGameClient)
    {
      emulatorItem->SetLabel2(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(35511)); // "Current"
      emulatorItem->Select(true);
    }
    items.Add(std::move(emulatorItem));
  }

  dialog->SetItems(items);
  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  const int selectedIndex = dialog->GetSelectedItem();
  if (selectedIndex < 0 || selectedIndex >= items.Size())
    return false;

  // An empty path is the "None" entry, which forgets rather than stores
  const std::string gameClient = items[selectedIndex]->GetPath();

  if (!db.GameClients().SetGameClient(path, gameClient))
    return false;

  if (gameClient.empty())
    CLog::Log(LOGDEBUG, "GAME: Forgot the emulator for {}", CURL::GetRedacted(path));
  else
    CLog::Log(LOGDEBUG, "GAME: Remembered emulator {} for {}", gameClient, CURL::GetRedacted(path));

  return true;
}

void CGameUtils::GetGameClients(const CFileItem& file,
                                GameClientVector& candidates,
                                GameClientVector& installable,
                                bool& bHasVfsGameClient)
{
  using namespace ADDON;

  bHasVfsGameClient = false;

  // Try to resolve path to a local file, as not all game clients support VFS
  CURL translatedUrl(CSpecialProtocol::TranslatePath(file.GetPath()));

  // Get local candidates
  VECADDONS localAddons;
  CBinaryAddonCache& addonCache = CServiceBroker::GetBinaryAddonCache();
  addonCache.GetAddons(localAddons, AddonType::GAMEDLL);

  bool bVfs = false;
  GetGameClients(localAddons, translatedUrl, candidates, bVfs);
  bHasVfsGameClient |= bVfs;

  // Get remote candidates
  VECADDONS remoteAddons;
  if (CServiceBroker::GetAddonMgr().GetInstallableAddons(remoteAddons, AddonType::GAMEDLL))
  {
    GetGameClients(remoteAddons, translatedUrl, installable, bVfs);
    bHasVfsGameClient |= bVfs;
  }

  // Sort by name
  //! @todo Move to presentation code
  auto SortByName = [](const GameClientPtr& lhs, const GameClientPtr& rhs)
  {
    std::string lhsName = lhs->Name();
    std::string rhsName = rhs->Name();

    StringUtils::ToLower(lhsName);
    StringUtils::ToLower(rhsName);

    return lhsName < rhsName;
  };

  std::sort(candidates.begin(), candidates.end(), SortByName);
  std::sort(installable.begin(), installable.end(), SortByName);
}

void CGameUtils::GetGameClients(const ADDON::VECADDONS& addons,
                                const CURL& translatedUrl,
                                GameClientVector& candidates,
                                bool& bHasVfsGameClient)
{
  bHasVfsGameClient = false;

  const std::string extension = URIUtils::GetExtension(translatedUrl.Get());

  const bool bIsLocalFile =
      (translatedUrl.GetProtocol() == "file" || translatedUrl.GetProtocol().empty());

  for (auto& addon : addons)
  {
    GameClientPtr gameClient = std::static_pointer_cast<CGameClient>(addon);

    // Filter by extension
    if (!gameClient->IsExtensionValid(extension))
      continue;

    // Filter by VFS
    if (!bIsLocalFile && !gameClient->SupportsVFS())
    {
      bHasVfsGameClient = true;
      continue;
    }

    candidates.push_back(gameClient);
  }
}

bool CGameUtils::HasGameExtension(const std::string& path)
{
  using namespace ADDON;

  // Get filename from CURL so that top-level zip directories will become
  // normal paths:
  //
  //   zip://%2Fpath_to_zip_file.zip/  ->  /path_to_zip_file.zip
  //
  std::string filename = CURL(path).GetFileNameWithoutPath();

  // Get the file extension
  std::string extension = URIUtils::GetExtension(filename);
  if (extension.empty())
    return false;

  StringUtils::ToLower(extension);

  // Look for a game client that supports this extension
  VECADDONS gameClients;
  CBinaryAddonCache& addonCache = CServiceBroker::GetBinaryAddonCache();
  addonCache.GetInstalledAddons(gameClients, AddonType::GAMEDLL);
  for (auto& gameClient : gameClients)
  {
    GameClientPtr gc(std::static_pointer_cast<CGameClient>(gameClient));
    if (gc->IsExtensionValid(extension))
      return true;
  }

  // Check remote add-ons
  std::lock_guard<std::mutex> installableLock(m_installableMutex);
  LoadInstallableAddons();
  if (!m_installableGameAddons.empty())
  {
    for (auto& gameClient : m_installableGameAddons)
    {
      GameClientPtr gc(std::static_pointer_cast<CGameClient>(gameClient));
      if (gc->IsExtensionValid(extension))
        return true;
    }
  }

  return false;
}

std::set<std::string> CGameUtils::GetGameExtensions()
{
  using namespace ADDON;

  std::set<std::string> extensions;

  VECADDONS gameClients;
  CBinaryAddonCache& addonCache = CServiceBroker::GetBinaryAddonCache();
  addonCache.GetAddons(gameClients, AddonType::GAMEDLL);
  for (auto& gameClient : gameClients)
  {
    GameClientPtr gc(std::static_pointer_cast<CGameClient>(gameClient));
    extensions.insert(gc->GetExtensions().begin(), gc->GetExtensions().end());
  }

  // Check remote add-ons
  std::lock_guard<std::mutex> installableLock(m_installableMutex);
  LoadInstallableAddons();
  if (!m_installableGameAddons.empty())
  {
    for (auto& gameClient : m_installableGameAddons)
    {
      GameClientPtr gc(std::static_pointer_cast<CGameClient>(gameClient));
      extensions.insert(gc->GetExtensions().begin(), gc->GetExtensions().end());
    }
  }

  // Remove special libretro extensions
  extensions.erase("*");
  extensions.erase(".");
  extensions.erase("./");
  extensions.erase("/");

  return extensions;
}

bool CGameUtils::IsStandaloneGame(const ADDON::AddonPtr& addon)
{
  using namespace ADDON;

  switch (addon->Type())
  {
    case AddonType::GAMEDLL:
    {
      return std::static_pointer_cast<GAME::CGameClient>(addon)->SupportsStandalone();
    }
    case AddonType::SCRIPT:
    {
      return addon->HasType(AddonType::GAME);
    }
    default:
      break;
  }

  return false;
}

void CGameUtils::UpdateInstallableAddons()
{
  std::lock_guard<std::mutex> installableLock(m_installableMutex);
  m_checkInstallable = true;
}

bool CGameUtils::Install(const std::string& gameClient)
{
  // If the addon isn't installed we need to install it
  bool installed = CServiceBroker::GetAddonMgr().IsAddonInstalled(gameClient);
  if (!installed)
  {
    ADDON::AddonPtr installedAddon;
    installed = ADDON::CAddonInstaller::GetInstance().InstallModal(
        gameClient, installedAddon, ADDON::InstallModalPrompt::CHOICE_NO);
    if (!installed)
    {
      CLog::Log(LOGERROR, "Game utils: Failed to install {}", gameClient);
      // "Error"
      // "Failed to install add-on."
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{257}, CVariant{35256});
    }
  }

  return installed;
}

bool CGameUtils::Enable(const std::string& gameClient)
{
  bool bSuccess = true;

  if (CServiceBroker::GetAddonMgr().IsAddonDisabled(gameClient))
    bSuccess = CServiceBroker::GetAddonMgr().EnableAddon(gameClient);

  return bSuccess;
}

void CGameUtils::LoadInstallableAddons()
{
  using namespace ADDON;

  if (m_checkInstallable)
  {
    m_checkInstallable = false;
    m_installableGameAddons.clear();
    CServiceBroker::GetAddonMgr().GetInstallableAddons(m_installableGameAddons, AddonType::GAMEDLL);
  }
}
