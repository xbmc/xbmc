/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameUtils.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/Addon.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/BinaryAddonCache.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/savestates/ISavestate.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "filesystem/SpecialProtocol.h"
#include "games/addons/GameClient.h"
#include "games/dialogs/GUIDialogSelectGameClient.h"
#include "games/dialogs/GUIDialogSelectSavestate.h"
#include "games/tags/GameInfoTag.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

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

        if (candidates.empty() && installable.empty())
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
  gameClients.clear();
  if (CServiceBroker::GetAddonMgr().GetInstallableAddons(gameClients, AddonType::GAMEDLL))
  {
    for (auto& gameClient : gameClients)
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
  gameClients.clear();
  if (CServiceBroker::GetAddonMgr().GetInstallableAddons(gameClients, AddonType::GAMEDLL))
  {
    for (auto& gameClient : gameClients)
    {
      GameClientPtr gc(std::static_pointer_cast<CGameClient>(gameClient));
      extensions.insert(gc->GetExtensions().begin(), gc->GetExtensions().end());
    }
  }

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
