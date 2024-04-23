/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonsDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/AddonRepos.h"
#include "addons/AddonSystemSettings.h"
#include "addons/PluginSource.h"
#include "addons/RepositoryUpdater.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/TextureManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <array>
#include <functional>
#include <set>

using namespace KODI;
using namespace ADDON;
using namespace KODI::MESSAGING;

namespace XFILE
{

CAddonsDirectory::CAddonsDirectory(void) = default;

CAddonsDirectory::~CAddonsDirectory(void) = default;

const auto CATEGORY_INFO_PROVIDERS = "category.infoproviders";
const auto CATEGORY_LOOK_AND_FEEL = "category.lookandfeel";
const auto CATEGORY_GAME_ADDONS = "category.gameaddons";
const auto CATEGORY_EMULATORS = "category.emulators";
const auto CATEGORY_STANDALONE_GAMES = "category.standalonegames";
const auto CATEGORY_GAME_PROVIDERS = "category.gameproviders";
const auto CATEGORY_GAME_RESOURCES = "category.gameresources";
const auto CATEGORY_GAME_SUPPORT_ADDONS = "category.gamesupport";

const std::set<AddonType> infoProviderTypes = {
    AddonType::SCRAPER_ALBUMS,      AddonType::SCRAPER_ARTISTS, AddonType::SCRAPER_MOVIES,
    AddonType::SCRAPER_MUSICVIDEOS, AddonType::SCRAPER_TVSHOWS,
};

const std::set<AddonType> lookAndFeelTypes = {
    AddonType::SKIN,
    AddonType::SCREENSAVER,
    AddonType::RESOURCE_IMAGES,
    AddonType::RESOURCE_LANGUAGE,
    AddonType::RESOURCE_UISOUNDS,
    AddonType::RESOURCE_FONT,
    AddonType::VISUALIZATION,
};

const std::set<AddonType> gameTypes = {
    AddonType::GAME_CONTROLLER,
    AddonType::GAMEDLL,
    AddonType::GAME,
    AddonType::RESOURCE_GAMES,
};

static bool IsInfoProviderType(AddonType type)
{
  return infoProviderTypes.find(type) != infoProviderTypes.end();
}

static bool IsInfoProviderTypeAddon(const AddonPtr& addon)
{
  return IsInfoProviderType(addon->Type());
}

static bool IsLookAndFeelType(AddonType type)
{
  return lookAndFeelTypes.find(type) != lookAndFeelTypes.end();
}

static bool IsLookAndFeelTypeAddon(const AddonPtr& addon)
{
  return IsLookAndFeelType(addon->Type());
}

static bool IsGameType(AddonType type)
{
  return gameTypes.find(type) != gameTypes.end();
}

static bool IsStandaloneGame(const AddonPtr& addon)
{
  return GAME::CGameUtils::IsStandaloneGame(addon);
}

static bool IsEmulator(const AddonPtr& addon)
{
  return addon->Type() == AddonType::GAMEDLL &&
         std::static_pointer_cast<GAME::CGameClient>(addon)->SupportsPath();
}

static bool IsGameProvider(const AddonPtr& addon)
{
  return addon->Type() == AddonType::PLUGIN && addon->HasType(AddonType::GAME);
}

static bool IsGameResource(const AddonPtr& addon)
{
  return addon->Type() == AddonType::RESOURCE_GAMES;
}

static bool IsGameSupportAddon(const AddonPtr& addon)
{
  return addon->Type() == AddonType::GAMEDLL &&
         !std::static_pointer_cast<GAME::CGameClient>(addon)->SupportsPath() &&
         !std::static_pointer_cast<GAME::CGameClient>(addon)->SupportsStandalone();
}

static bool IsGameAddon(const AddonPtr& addon)
{
  return IsGameType(addon->Type()) ||
         IsStandaloneGame(addon) ||
         IsGameProvider(addon) ||
         IsGameResource(addon) ||
         IsGameSupportAddon(addon);
}

static bool IsUserInstalled(const AddonPtr& addon)
{
  return !CAddonType::IsDependencyType(addon->MainType());
}

// Creates categories from addon types, if we have any addons with that type.
static void GenerateTypeListing(const CURL& path,
                                const std::set<AddonType>& types,
                                const VECADDONS& addons,
                                CFileItemList& items)
{
  for (const auto& type : types)
  {
    for (const auto& addon : addons)
    {
      if (addon->HasType(type))
      {
        CFileItemPtr item(new CFileItem(CAddonInfo::TranslateType(type, true)));
        CURL itemPath = path;
        itemPath.SetFileName(CAddonInfo::TranslateType(type, false));
        item->SetPath(itemPath.Get());
        item->m_bIsFolder = true;
        std::string thumb = CAddonInfo::TranslateIconType(type);
        if (!thumb.empty() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
          item->SetArt("thumb", thumb);
        items.Add(item);
        break;
      }
    }
  }
}

// Creates categories for game add-ons, if we have any game add-ons
static void GenerateGameListing(const CURL& path, const VECADDONS& addons, CFileItemList& items)
{
  // Game controllers
  for (const auto& addon : addons)
  {
    if (addon->Type() == AddonType::GAME_CONTROLLER)
    {
      CFileItemPtr item(new CFileItem(CAddonInfo::TranslateType(AddonType::GAME_CONTROLLER, true)));
      CURL itemPath = path;
      itemPath.SetFileName(CAddonInfo::TranslateType(AddonType::GAME_CONTROLLER, false));
      item->SetPath(itemPath.Get());
      item->m_bIsFolder = true;
      std::string thumb = CAddonInfo::TranslateIconType(AddonType::GAME_CONTROLLER);
      if (!thumb.empty() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
        item->SetArt("thumb", thumb);
      items.Add(item);
      break;
    }
  }
  // Emulators
  for (const auto& addon : addons)
  {
    if (IsEmulator(addon))
    {
      CFileItemPtr item(new CFileItem(g_localizeStrings.Get(35207))); // Emulators
      CURL itemPath = path;
      itemPath.SetFileName(CATEGORY_EMULATORS);
      item->SetPath(itemPath.Get());
      item->m_bIsFolder = true;
      std::string thumb = CAddonInfo::TranslateIconType(AddonType::GAMEDLL);
      if (!thumb.empty() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
        item->SetArt("thumb", thumb);
      items.Add(item);
      break;
    }
  }
  // Standalone games
  for (const auto& addon : addons)
  {
    if (IsStandaloneGame(addon))
    {
      CFileItemPtr item(new CFileItem(g_localizeStrings.Get(35208))); // Standalone games
      CURL itemPath = path;
      itemPath.SetFileName(CATEGORY_STANDALONE_GAMES);
      item->SetPath(itemPath.Get());
      item->m_bIsFolder = true;
      std::string thumb = CAddonInfo::TranslateIconType(AddonType::GAMEDLL);
      if (!thumb.empty() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
        item->SetArt("thumb", thumb);
      items.Add(item);
      break;
    }
  }
  // Game providers
  for (const auto& addon : addons)
  {
    if (IsGameProvider(addon))
    {
      CFileItemPtr item(new CFileItem(g_localizeStrings.Get(35220))); // Game providers
      CURL itemPath = path;
      itemPath.SetFileName(CATEGORY_GAME_PROVIDERS);
      item->SetPath(itemPath.Get());
      item->m_bIsFolder = true;
      std::string thumb = CAddonInfo::TranslateIconType(AddonType::GAMEDLL);
      if (!thumb.empty() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
        item->SetArt("thumb", thumb);
      items.Add(item);
      break;
    }
  }
  // Game resources
  for (const auto& addon : addons)
  {
    if (IsGameResource(addon))
    {
      CFileItemPtr item(new CFileItem(g_localizeStrings.Get(35209))); // Game resources
      CURL itemPath = path;
      itemPath.SetFileName(CATEGORY_GAME_RESOURCES);
      item->SetPath(itemPath.Get());
      item->m_bIsFolder = true;
      std::string thumb = CAddonInfo::TranslateIconType(AddonType::GAMEDLL);
      if (!thumb.empty() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
        item->SetArt("thumb", thumb);
      items.Add(item);
      break;
    }
  }
  // Game support add-ons
  for (const auto& addon : addons)
  {
    if (IsGameSupportAddon(addon))
    {
      CFileItemPtr item(new CFileItem(g_localizeStrings.Get(35216))); // Support add-ons
      CURL itemPath = path;
      itemPath.SetFileName(CATEGORY_GAME_SUPPORT_ADDONS);
      item->SetPath(itemPath.Get());
      item->m_bIsFolder = true;
      std::string thumb = CAddonInfo::TranslateIconType(AddonType::GAMEDLL);
      if (!thumb.empty() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
        item->SetArt("thumb", thumb);
      items.Add(item);
      break;
    }
  }
}

//Creates the top-level category list
static void GenerateMainCategoryListing(const CURL& path, const VECADDONS& addons,
    CFileItemList& items)
{
  if (std::any_of(addons.begin(), addons.end(), IsInfoProviderTypeAddon))
  {
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(24993)));
    item->SetPath(URIUtils::AddFileToFolder(path.Get(), CATEGORY_INFO_PROVIDERS));
    item->m_bIsFolder = true;
    const std::string thumb = "DefaultAddonInfoProvider.png";
    if (CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
      item->SetArt("thumb", thumb);
    items.Add(item);
  }
  if (std::any_of(addons.begin(), addons.end(), IsLookAndFeelTypeAddon))
  {
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(24997)));
    item->SetPath(URIUtils::AddFileToFolder(path.Get(), CATEGORY_LOOK_AND_FEEL));
    item->m_bIsFolder = true;
    const std::string thumb = "DefaultAddonLookAndFeel.png";
    if (CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
      item->SetArt("thumb", thumb);
    items.Add(item);
  }
  if (std::any_of(addons.begin(), addons.end(), IsGameAddon))
  {
    CFileItemPtr item(new CFileItem(CAddonInfo::TranslateType(AddonType::GAME, true)));
    item->SetPath(URIUtils::AddFileToFolder(path.Get(), CATEGORY_GAME_ADDONS));
    item->m_bIsFolder = true;
    const std::string thumb = CAddonInfo::TranslateIconType(AddonType::GAME);
    if (CServiceBroker::GetGUI()->GetTextureManager().HasTexture(thumb))
      item->SetArt("thumb", thumb);
    items.Add(item);
  }

  std::set<AddonType> uncategorized;
  for (unsigned int i = static_cast<unsigned int>(AddonType::UNKNOWN) + 1;
       i < static_cast<unsigned int>(AddonType::MAX_TYPES) - 1; ++i)
  {
    const AddonType type = static_cast<AddonType>(i);
    /*
     * Check and prevent insert for this cases:
     * - By a provider, look and feel, dependency and game becomes given to
     *   subdirectory to control the types
     * - By ADDON_SCRIPT and ADDON_PLUGIN, them contains one of the possible
     *   subtypes (audio, video, app or/and game) and not needed to show
     *   together in a Script or Plugin list
     */
    if (!IsInfoProviderType(type) && !IsLookAndFeelType(type) &&
        !CAddonType::IsDependencyType(type) && !IsGameType(type) && type != AddonType::SCRIPT &&
        type != AddonType::PLUGIN)
      uncategorized.insert(type);
  }
  GenerateTypeListing(path, uncategorized, addons, items);
}

//Creates sub-categories or addon list for a category
static void GenerateCategoryListing(const CURL& path, VECADDONS& addons,
    CFileItemList& items)
{
  const std::string& category = path.GetFileName();
  if (category == CATEGORY_INFO_PROVIDERS)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(24993));
    items.SetLabel(g_localizeStrings.Get(24993));
    GenerateTypeListing(path, infoProviderTypes, addons, items);
  }
  else if (category == CATEGORY_LOOK_AND_FEEL)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(24997));
    items.SetLabel(g_localizeStrings.Get(24997));
    GenerateTypeListing(path, lookAndFeelTypes, addons, items);
  }
  else if (category == CATEGORY_GAME_ADDONS)
  {
    items.SetProperty("addoncategory", CAddonInfo::TranslateType(AddonType::GAME, true));
    items.SetLabel(CAddonInfo::TranslateType(AddonType::GAME, true));
    GenerateGameListing(path, addons, items);
  }
  else if (category == CATEGORY_EMULATORS)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35207)); // Emulators
    addons.erase(std::remove_if(addons.begin(), addons.end(),
        [](const AddonPtr& addon){ return !IsEmulator(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35207)); // Emulators
  }
  else if (category == CATEGORY_STANDALONE_GAMES)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35208)); // Standalone games
    addons.erase(std::remove_if(addons.begin(), addons.end(),
        [](const AddonPtr& addon){ return !IsStandaloneGame(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35208)); // Standalone games
  }
  else if (category == CATEGORY_GAME_PROVIDERS)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35220)); // Game providers
    addons.erase(std::remove_if(addons.begin(), addons.end(),
                                [](const AddonPtr& addon){ return !IsGameProvider(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35220)); // Game providers
  }
  else if (category == CATEGORY_GAME_RESOURCES)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35209)); // Game resources
    addons.erase(std::remove_if(addons.begin(), addons.end(),
                                [](const AddonPtr& addon){ return !IsGameResource(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35209)); // Game resources
  }
  else if (category == CATEGORY_GAME_SUPPORT_ADDONS)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35216)); // Support add-ons
    addons.erase(std::remove_if(addons.begin(), addons.end(),
      [](const AddonPtr& addon) { return !IsGameSupportAddon(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35216)); // Support add-ons
  }
  else
  { // fallback to addon type
    AddonType type = CAddonInfo::TranslateType(category);
    items.SetProperty("addoncategory", CAddonInfo::TranslateType(type, true));
    addons.erase(std::remove_if(addons.begin(), addons.end(),
                                [type](const AddonPtr& addon) { return !addon->HasType(type); }),
                 addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, CAddonInfo::TranslateType(type, true));
  }
}

bool CAddonsDirectory::GetSearchResults(const CURL& path, CFileItemList &items)
{
  std::string search(path.GetFileName());
  if (search.empty() && !GetKeyboardInput(16017, search))
    return false;

  CAddonDatabase database;
  database.Open();

  VECADDONS addons;
  database.Search(search, addons);
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(283));
  CURL searchPath(path);
  searchPath.SetFileName(search);
  items.SetPath(searchPath.Get());
  return true;
}

static void UserInstalledAddons(const CURL& path, CFileItemList &items)
{
  items.ClearItems();
  items.SetLabel(g_localizeStrings.Get(24998));

  VECADDONS addons;
  CServiceBroker::GetAddonMgr().GetInstalledAddons(addons);
  addons.erase(std::remove_if(addons.begin(), addons.end(),
                              [](const AddonPtr& addon) { return !IsUserInstalled(addon); }), addons.end());

  if (addons.empty())
    return;

  const std::string& category = path.GetFileName();
  if (category.empty())
  {
    GenerateMainCategoryListing(path, addons, items);

    //"All" node
    CFileItemPtr item(new CFileItem());
    item->m_bIsFolder = true;
    CURL itemPath = path;
    itemPath.SetFileName("all");
    item->SetPath(itemPath.Get());
    item->SetLabel(g_localizeStrings.Get(593));
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
  else if (category == "all")
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24998));
  else
    GenerateCategoryListing(path, addons, items);
}

static void DependencyAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS all;
  CServiceBroker::GetAddonMgr().GetInstalledAddons(all);

  VECADDONS deps;
  std::copy_if(all.begin(), all.end(), std::back_inserter(deps),
      [&](const AddonPtr& _){ return !IsUserInstalled(_); });

  CAddonsDirectory::GenerateAddonListing(path, deps, items, g_localizeStrings.Get(24996));

  //Set orphaned status
  std::set<std::string> orphaned;
  for (const auto& addon : deps)
  {
    if (CServiceBroker::GetAddonMgr().IsOrphaned(addon, all))
      orphaned.insert(addon->ID());
  }

  for (int i = 0; i < items.Size(); ++i)
  {
    if (orphaned.find(items[i]->GetProperty("Addon.ID").asString()) != orphaned.end())
    {
      items[i]->SetProperty("Addon.Status", g_localizeStrings.Get(24995));
      items[i]->SetProperty("Addon.Orphaned", true);
    }
  }
}

static void OutdatedAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS addons = CServiceBroker::GetAddonMgr().GetAvailableUpdates();
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24043));

  if (!items.IsEmpty())
  {
    if (CAddonSystemSettings::GetInstance().GetAddonAutoUpdateMode() == AUTO_UPDATES_ON)
    {
      const CFileItemPtr itemUpdateAllowed(
          std::make_shared<CFileItem>("addons://update_allowed/", false));
      itemUpdateAllowed->SetLabel(g_localizeStrings.Get(24137));
      itemUpdateAllowed->SetSpecialSort(SortSpecialOnTop);
      items.Add(itemUpdateAllowed);
    }

    const CFileItemPtr itemUpdateAll(std::make_shared<CFileItem>("addons://update_all/", false));
    itemUpdateAll->SetLabel(g_localizeStrings.Get(24122));
    itemUpdateAll->SetSpecialSort(SortSpecialOnTop);
    items.Add(itemUpdateAll);
  }
}

static void RunningAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS addons;
  CServiceBroker::GetAddonMgr().GetAddons(addons, AddonType::SERVICE);

  addons.erase(std::remove_if(addons.begin(), addons.end(),
      [](const AddonPtr& addon){ return !CScriptInvocationManager::GetInstance().IsRunning(addon->LibPath()); }), addons.end());
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24994));
}

static bool Browse(const CURL& path, CFileItemList &items)
{
  const std::string& repoId = path.GetHostName();

  VECADDONS addons;
  items.SetPath(path.Get());
  if (repoId == "all")
  {
    CAddonRepos addonRepos;
    if (!addonRepos.IsValid())
      return false;

    // get all latest addon versions by repo
    addonRepos.GetLatestAddonVersionsFromAllRepos(addons);

    items.SetProperty("reponame", g_localizeStrings.Get(24087));
    items.SetLabel(g_localizeStrings.Get(24087));
  }
  else
  {
    AddonPtr repoAddon;
    if (!CServiceBroker::GetAddonMgr().GetAddon(repoId, repoAddon, AddonType::REPOSITORY,
                                                OnlyEnabled::CHOICE_YES))
    {
      return false;
    }

    CAddonRepos addonRepos(repoAddon);
    if (!addonRepos.IsValid())
      return false;

    // get all addons from the single repository
    addonRepos.GetLatestAddonVersions(addons);

    items.SetProperty("reponame", repoAddon->Name());
    items.SetLabel(repoAddon->Name());
  }

  const std::string& category = path.GetFileName();
  if (category.empty())
    GenerateMainCategoryListing(path, addons, items);
  else
    GenerateCategoryListing(path, addons, items);
  return true;
}

static bool GetRecentlyUpdatedAddons(VECADDONS& addons)
{
  if (!CServiceBroker::GetAddonMgr().GetInstalledAddons(addons))
    return false;

  auto limit = CDateTime::GetCurrentDateTime() - CDateTimeSpan(14, 0, 0, 0);
  auto isOld = [limit](const AddonPtr& addon){ return addon->LastUpdated() < limit; };
  addons.erase(std::remove_if(addons.begin(), addons.end(), isOld), addons.end());
  return true;
}

static bool HasRecentlyUpdatedAddons()
{
  VECADDONS addons;
  return GetRecentlyUpdatedAddons(addons) && !addons.empty();
}

static bool Repos(const CURL& path, CFileItemList &items)
{
  items.SetLabel(g_localizeStrings.Get(24033));

  VECADDONS addons;
  CServiceBroker::GetAddonMgr().GetAddons(addons, AddonType::REPOSITORY);
  if (addons.empty())
    return true;
  else if (addons.size() == 1)
    return Browse(CURL("addons://" + addons[0]->ID()), items);
  CFileItemPtr item(new CFileItem("addons://all/", true));
  item->SetLabel(g_localizeStrings.Get(24087));
  item->SetSpecialSort(SortSpecialOnTop);
  items.Add(item);
  for (const auto& repo : addons)
  {
    CFileItemPtr item = CAddonsDirectory::FileItemFromAddon(repo, "addons://" + repo->ID(), true);
    items.Add(item);
  }
  items.SetContent("addons");
  return true;
}

static void RootDirectory(CFileItemList& items)
{
  items.SetLabel(g_localizeStrings.Get(10040));
  {
    CFileItemPtr item(new CFileItem("addons://user/", true));
    item->SetLabel(g_localizeStrings.Get(24998));
    item->SetArt("icon", "DefaultAddonsInstalled.png");
    items.Add(item);
  }
  if (CServiceBroker::GetAddonMgr().HasAvailableUpdates())
  {
    CFileItemPtr item(new CFileItem("addons://outdated/", true));
    item->SetLabel(g_localizeStrings.Get(24043));
    item->SetArt("icon", "DefaultAddonsUpdates.png");
    items.Add(item);
  }
  if (CAddonInstaller::GetInstance().IsDownloading())
  {
    CFileItemPtr item(new CFileItem("addons://downloading/", true));
    item->SetLabel(g_localizeStrings.Get(24067));
    item->SetArt("icon", "DefaultNetwork.png");
    items.Add(item);
  }
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_ADDONS_AUTOUPDATES) == ADDON::AUTO_UPDATES_ON
      && HasRecentlyUpdatedAddons())
  {
    CFileItemPtr item(new CFileItem("addons://recently_updated/", true));
    item->SetLabel(g_localizeStrings.Get(24004));
    item->SetArt("icon", "DefaultAddonsRecentlyUpdated.png");
    items.Add(item);
  }
  if (CServiceBroker::GetAddonMgr().HasAddons(AddonType::REPOSITORY))
  {
    CFileItemPtr item(new CFileItem("addons://repos/", true));
    item->SetLabel(g_localizeStrings.Get(24033));
    item->SetArt("icon", "DefaultAddonsRepo.png");
    items.Add(item);
  }
  {
    CFileItemPtr item(new CFileItem("addons://install/", false));
    item->SetLabel(g_localizeStrings.Get(24041));
    item->SetArt("icon", "DefaultAddonsZip.png");
    items.Add(item);
  }
  {
    CFileItemPtr item(new CFileItem("addons://search/", true));
    item->SetLabel(g_localizeStrings.Get(137));
    item->SetArt("icon", "DefaultAddonsSearch.png");
    items.Add(item);
  }
}

bool CAddonsDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string tmp(url.Get());
  URIUtils::RemoveSlashAtEnd(tmp);
  CURL path(tmp);
  const std::string& endpoint = path.GetHostName();
  items.ClearItems();
  items.ClearProperties();
  items.SetCacheToDisc(CFileItemList::CACHE_NEVER);
  items.SetPath(path.Get());

  if (endpoint.empty())
  {
    RootDirectory(items);
    return true;
  }
  else if (endpoint == "user")
  {
    UserInstalledAddons(path, items);
    return true;
  }
  else if (endpoint == "dependencies")
  {
    DependencyAddons(path, items);
    return true;
  }
  // PVR hardcodes this view so keep for compatibility
  else if (endpoint == "disabled")
  {
    VECADDONS addons;
    AddonType type;

    if (path.GetFileName() == "kodi.pvrclient")
      type = AddonType::PVRDLL;
    else if (path.GetFileName() == "kodi.vfs")
      type = AddonType::VFS;
    else
      type = AddonType::UNKNOWN;

    if (type != AddonType::UNKNOWN &&
        CServiceBroker::GetAddonMgr().GetInstalledAddons(addons, type))
    {
      CAddonsDirectory::GenerateAddonListing(path, addons, items, CAddonInfo::TranslateType(type, true));
      return true;
    }
    return false;
  }
  else if (endpoint == "outdated")
  {
    OutdatedAddons(path, items);
    return true;
  }
  else if (endpoint == "running")
  {
    RunningAddons(path, items);
    return true;
  }
  else if (endpoint == "repos")
  {
    return Repos(path, items);
  }
  else if (endpoint == "sources")
  {
    return GetScriptsAndPlugins(path.GetFileName(), items);
  }
  else if (endpoint == "search")
  {
    return GetSearchResults(path, items);
  }
  else if (endpoint == "recently_updated")
  {
    VECADDONS addons;
    if (!GetRecentlyUpdatedAddons(addons))
      return false;

    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24004));
    return true;
  }
  else if (endpoint == "downloading")
  {
    VECADDONS addons;
    CAddonInstaller::GetInstance().GetInstallList(addons);
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24067));
    return true;
  }
  else if (endpoint == "more")
  {
    const std::string& type = path.GetFileName();
    if (type == "video" || type == "audio" || type == "image" || type == "executable")
      return Browse(CURL("addons://all/xbmc.addon." + type), items);
    else if (type == "game")
      return Browse(CURL("addons://all/category.gameaddons"), items);
    return false;
  }
  else
  {
    return Browse(path, items);
  }
}

bool CAddonsDirectory::IsRepoDirectory(const CURL& url)
{
  if (url.GetHostName().empty() || !url.IsProtocol("addons"))
    return false;

  AddonPtr tmp;
  return url.GetHostName() == "repos" || url.GetHostName() == "all" ||
         url.GetHostName() == "search" ||
         CServiceBroker::GetAddonMgr().GetAddon(url.GetHostName(), tmp, AddonType::REPOSITORY,
                                                OnlyEnabled::CHOICE_YES);
}

void CAddonsDirectory::GenerateAddonListing(const CURL& path,
                                            const VECADDONS& addons,
                                            CFileItemList& items,
                                            const std::string& label)
{
  std::map<std::string, AddonWithUpdate> addonsWithUpdate =
      CServiceBroker::GetAddonMgr().GetAddonsWithAvailableUpdate();

  items.ClearItems();
  items.SetContent("addons");
  items.SetLabel(label);
  for (const auto& addon : addons)
  {
    CURL itemPath = path;
    itemPath.SetFileName(addon->ID());
    CFileItemPtr pItem = FileItemFromAddon(addon, itemPath.Get(), false);

    bool installed = CServiceBroker::GetAddonMgr().IsAddonInstalled(addon->ID(), addon->Origin(),
                                                                    addon->Version());
    bool disabled = CServiceBroker::GetAddonMgr().IsAddonDisabled(addon->ID());

    bool isUpdate{false};
    bool hasUpdate{false};
    std::string validUpdateVersion;
    std::string validUpdateOrigin;

    auto _ = addonsWithUpdate.find(addon->ID());
    if (_ != addonsWithUpdate.end())
    {
      auto [installed, update] = _->second;

      auto CheckAddon = [&addon](const std::shared_ptr<IAddon>& _)
      { return _->Origin() == addon->Origin() && _->Version() == addon->Version(); };

      isUpdate = CheckAddon(update); // check if listed add-on is update to an installed add-on
      hasUpdate = CheckAddon(installed); // check if installed add-on has an update available

      if (hasUpdate)
      {
        validUpdateVersion = update->Version().asString();
        validUpdateOrigin = update->Origin();
      }
    }

    bool fromOfficialRepo = CAddonRepos::IsFromOfficialRepo(addon, CheckAddonPath::CHOICE_NO);

    pItem->SetProperty("Addon.IsInstalled", installed);
    pItem->SetProperty("Addon.IsEnabled", installed && !disabled);
    pItem->SetProperty("Addon.HasUpdate", hasUpdate);
    pItem->SetProperty("Addon.IsUpdate", isUpdate);
    pItem->SetProperty("Addon.ValidUpdateVersion", validUpdateVersion);
    pItem->SetProperty("Addon.ValidUpdateOrigin", validUpdateOrigin);
    pItem->SetProperty("Addon.IsFromOfficialRepo", fromOfficialRepo);
    pItem->SetProperty("Addon.IsBinary", addon->IsBinary());

    if (installed)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(305));
    if (disabled)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24023));
    if (hasUpdate)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24068));
    else if (addon->LifecycleState() == AddonLifecycleState::BROKEN)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24098));
    else if (addon->LifecycleState() == AddonLifecycleState::DEPRECATED)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24170));

    items.Add(pItem);
  }
}

CFileItemPtr CAddonsDirectory::FileItemFromAddon(const AddonPtr &addon,
    const std::string& path, bool folder)
{
  if (!addon)
    return CFileItemPtr();

  CFileItemPtr item(new CFileItem(addon));
  item->m_bIsFolder = folder;
  item->SetPath(path);

  std::string strLabel(addon->Name());
  if (CURL(path).GetHostName() == "search")
    strLabel = StringUtils::Format("{} - {}", CAddonInfo::TranslateType(addon->Type(), true),
                                   addon->Name());
  item->SetLabel(strLabel);
  item->SetArt(addon->Art());
  item->SetArt("thumb", addon->Icon());
  item->SetArt("icon", "DefaultAddon.png");

  //! @todo fix hacks that depends on these
  item->SetProperty("Addon.ID", addon->ID());
  item->SetProperty("Addon.Name", addon->Name());
  item->SetCanQueue(false);
  const auto it = addon->ExtraInfo().find("language");
  if (it != addon->ExtraInfo().end())
    item->SetProperty("Addon.Language", it->second);

  return item;
}

bool CAddonsDirectory::GetScriptsAndPlugins(const std::string &content, VECADDONS &addons)
{
  CPluginSource::Content type = CPluginSource::Translate(content);
  if (type == CPluginSource::UNKNOWN)
    return false;

  VECADDONS tempAddons;
  CServiceBroker::GetAddonMgr().GetAddons(tempAddons, AddonType::PLUGIN);
  for (unsigned i=0; i<tempAddons.size(); i++)
  {
    const auto plugin = std::dynamic_pointer_cast<CPluginSource>(tempAddons[i]);
    if (plugin && plugin->Provides(type))
      addons.push_back(tempAddons[i]);
  }
  tempAddons.clear();
  CServiceBroker::GetAddonMgr().GetAddons(tempAddons, AddonType::SCRIPT);
  for (unsigned i=0; i<tempAddons.size(); i++)
  {
    const auto plugin = std::dynamic_pointer_cast<CPluginSource>(tempAddons[i]);
    if (plugin && plugin->Provides(type))
      addons.push_back(tempAddons[i]);
  }
  tempAddons.clear();

  if (type == CPluginSource::GAME)
  {
    CServiceBroker::GetAddonMgr().GetAddons(tempAddons, AddonType::GAMEDLL);
    for (auto& addon : tempAddons)
    {
      if (IsStandaloneGame(addon))
        addons.push_back(addon);
    }
  }

  return true;
}

bool CAddonsDirectory::GetScriptsAndPlugins(const std::string &content, CFileItemList &items)
{
  VECADDONS addons;
  if (!GetScriptsAndPlugins(content, addons))
    return false;

  for (AddonPtr& addon : addons)
  {
    const bool bIsFolder = (addon->Type() == AddonType::PLUGIN);

    std::string path;
    if (addon->HasType(AddonType::PLUGIN))
    {
      path = "plugin://" + addon->ID();
      const auto plugin = std::dynamic_pointer_cast<CPluginSource>(addon);
      if (plugin && plugin->ProvidesSeveral())
      {
        CURL url(path);
        std::string opt = StringUtils::Format("?content_type={}", content);
        url.SetOptions(opt);
        path = url.Get();
      }
    }
    else if (addon->HasType(AddonType::SCRIPT))
    {
      path = "script://" + addon->ID();
    }
    else if (addon->HasType(AddonType::GAMEDLL))
    {
      // Kodi fails to launch games with empty path from home screen
      path = "game://" + addon->ID();
    }

    items.Add(FileItemFromAddon(addon, path, bIsFolder));
  }

  items.SetContent("addons");
  items.SetLabel(g_localizeStrings.Get(24001)); // Add-ons

  return true;
}

}

