/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <array>
#include <algorithm>
#include <functional>
#include <set>
#include "AddonsDirectory.h"
#include "ServiceBroker.h"
#include "addons/AddonDatabase.h"
#include "addons/AddonSystemSettings.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "FileItem.h"
#include "addons/AddonInstaller.h"
#include "addons/BinaryAddonCache.h"
#include "addons/RepositoryUpdater.h"
#include "dialogs/GUIDialogOK.h"
#include "games/addons/GameClient.h"
#include "games/GameUtils.h"
#include "guilib/TextureManager.h"
#include "File.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "URL.h"

using namespace ADDON;

namespace XFILE
{

CAddonsDirectory::CAddonsDirectory(void) {}

CAddonsDirectory::~CAddonsDirectory(void) {}

const auto CATEGORY_INFO_PROVIDERS = "category.infoproviders";
const auto CATEGORY_LOOK_AND_FEEL = "category.lookandfeel";
const auto CATEGORY_GAME_ADDONS = "category.gameaddons";
const auto CATEGORY_EMULATORS = "category.emulators";
const auto CATEGORY_STANDALONE_GAMES = "category.standalonegames";
const auto CATEGORY_GAME_PROVIDERS = "category.gameproviders";
const auto CATEGORY_GAME_RESOURCES = "category.gameresources";
const auto CATEGORY_GAME_SUPPORT_ADDONS = "category.gamesupport";

const std::set<TYPE> dependencyTypes = {
    ADDON_SCRAPER_LIBRARY,
    ADDON_SCRIPT_LIBRARY,
    ADDON_SCRIPT_MODULE,
};

const std::set<TYPE> infoProviderTypes = {
  ADDON_SCRAPER_ALBUMS,
  ADDON_SCRAPER_ARTISTS,
  ADDON_SCRAPER_MOVIES,
  ADDON_SCRAPER_MUSICVIDEOS,
  ADDON_SCRAPER_TVSHOWS,
};

const std::set<TYPE> lookAndFeelTypes = {
  ADDON_SKIN,
  ADDON_SCREENSAVER,
  ADDON_RESOURCE_IMAGES,
  ADDON_RESOURCE_LANGUAGE,
  ADDON_RESOURCE_UISOUNDS,
  ADDON_VIZ,
};

const std::set<TYPE> gameTypes = {
  ADDON_GAME_CONTROLLER,
  ADDON_GAMEDLL,
  ADDON_GAME,
  ADDON_RESOURCE_GAMES,
};

static bool IsInfoProviderType(TYPE type)
{
  return infoProviderTypes.find(type) != infoProviderTypes.end();
}

static bool IsInfoProviderTypeAddon(const AddonInfoPtr& addon)
{
  for (auto providerType : infoProviderTypes)
    if (addon->IsType(providerType))
      return true;
  return false;
}

static bool IsLookAndFeelType(TYPE type)
{
  return lookAndFeelTypes.find(type) != lookAndFeelTypes.end();
}

static bool IsLookAndFeelTypeAddon(const AddonInfoPtr& addon)
{
  for (auto lookAndFeelType : lookAndFeelTypes)
    if (addon->IsType(lookAndFeelType))
      return true;
  return false;
}

static bool IsGameType(TYPE type)
{
  return gameTypes.find(type) != gameTypes.end();
}

static bool IsGameTypeAddon(const AddonInfoPtr& addon)
{
  for (auto gameType : gameTypes)
    if (addon->IsType(gameType))
      return true;
  return false;
}

static bool IsStandaloneGame(const AddonInfoPtr& addon)
{
  return GAME::CGameUtils::IsStandaloneGame(addon);
}

static bool IsEmulator(const AddonInfoPtr& addon)
{
  return addon->IsType(ADDON_GAMEDLL) && !addon->Type(ADDON_GAMEDLL)->GetValue("extensions").empty();
}

static bool IsGameProvider(const AddonInfoPtr& addon)
{
  return addon->IsType(ADDON_PLUGIN) && addon->IsType(ADDON_GAME);
}

static bool IsGameResource(const AddonInfoPtr& addon)
{
  return addon->IsType(ADDON_RESOURCE_GAMES);
}

static bool IsGameSupportAddon(const AddonInfoPtr& addon)
{
  return addon->IsType(ADDON_GAMEDLL) && 
         addon->Type(ADDON_GAMEDLL)->GetValue("extensions").empty() &&
         !addon->Type(ADDON_GAMEDLL)->GetValue("supports_standalone").asBoolean();
}

static bool IsGameAddon(const AddonInfoPtr& addon)
{
  return IsGameTypeAddon(addon) ||
         IsStandaloneGame(addon) ||
         IsGameProvider(addon) ||
         IsGameResource(addon) ||
         IsGameSupportAddon(addon);
}

static bool IsDependencyType(TYPE type)
{
  return dependencyTypes.find(type) != dependencyTypes.end();
}

static bool IsUserInstalled(const AddonInfoPtr& addon)
{
  if (StringUtils::StartsWith(addon->ID(), "xbmc.") ||
      StringUtils::StartsWith(addon->ID(), "kodi."))
  {
    return false;
  }

  return std::find_if(dependencyTypes.begin(), dependencyTypes.end(),
      [&](TYPE type){ return addon->IsType(type); }) == dependencyTypes.end();
}

static bool IsOrphaned(const AddonInfoPtr& addon, const AddonInfos& all)
{
  if (CAddonMgr::GetInstance().IsSystemAddon(addon->ID()) || IsUserInstalled(addon))
    return false;

  for (const AddonInfoPtr& other : all)
  {
    const auto& deps = other->GetDeps();
    if (deps.find(addon->ID()) != deps.end())
      return false;
  }
  return true;
}


// Creates categories from addon types, if we have any addons with that type.
static void GenerateTypeListing(const CURL& path, const std::set<TYPE>& types,
    const AddonInfos& addons, CFileItemList& items)
{
  for (const auto& type : types)
  {
    for (const auto& addon : addons)
    {
      if (addon->IsType(type))
      {
        CFileItemPtr item(new CFileItem(CAddonInfo::TranslateType(type, true)));
        CURL itemPath = path;
        itemPath.SetFileName(CAddonInfo::TranslateType(type, false));
        item->SetPath(itemPath.Get());
        item->m_bIsFolder = true;
        std::string thumb = CAddonInfo::TranslateIconType(type);
        if (!thumb.empty() && g_TextureManager.HasTexture(thumb))
          item->SetArt("thumb", thumb);
        items.Add(item);
        break;
      }
    }
  }
}

// Creates categories for game add-ons, if we have any game add-ons
static void GenerateGameListing(const CURL& path, const AddonInfos& addons, CFileItemList& items)
{
  // Game controllers
  for (const auto& addon : addons)
  {
    if (addon->IsType(ADDON_GAME_CONTROLLER))
    {
      CFileItemPtr item(new CFileItem(CAddonInfo::TranslateType(ADDON_GAME_CONTROLLER, true)));
      CURL itemPath = path;
      itemPath.SetFileName(CAddonInfo::TranslateType(ADDON_GAME_CONTROLLER, false));
      item->SetPath(itemPath.Get());
      item->m_bIsFolder = true;
      std::string thumb = CAddonInfo::TranslateIconType(ADDON_GAME_CONTROLLER);
      if (!thumb.empty() && g_TextureManager.HasTexture(thumb))
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
      std::string thumb = CAddonInfo::TranslateIconType(ADDON_GAMEDLL);
      if (!thumb.empty() && g_TextureManager.HasTexture(thumb))
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
      std::string thumb = CAddonInfo::TranslateIconType(ADDON_GAMEDLL);
      if (!thumb.empty() && g_TextureManager.HasTexture(thumb))
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
      std::string thumb = CAddonInfo::TranslateIconType(ADDON_GAMEDLL);
      if (!thumb.empty() && g_TextureManager.HasTexture(thumb))
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
      std::string thumb = CAddonInfo::TranslateIconType(ADDON_GAMEDLL);
      if (!thumb.empty() && g_TextureManager.HasTexture(thumb))
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
      std::string thumb = CAddonInfo::TranslateIconType(ADDON_GAMEDLL);
      if (!thumb.empty() && g_TextureManager.HasTexture(thumb))
        item->SetArt("thumb", thumb);
      items.Add(item);
      break;
    }
  }
}

//Creates the top-level category list
static void GenerateMainCategoryListing(const CURL& path, const AddonInfos& addons,
    CFileItemList& items)
{
  if (std::any_of(addons.begin(), addons.end(), IsInfoProviderTypeAddon))
  {
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(24993)));
    item->SetPath(URIUtils::AddFileToFolder(path.Get(), CATEGORY_INFO_PROVIDERS));
    item->m_bIsFolder = true;
    const std::string thumb = "DefaultAddonInfoProvider.png";
    if (g_TextureManager.HasTexture(thumb))
      item->SetArt("thumb", thumb);
    items.Add(item);
  }
  if (std::any_of(addons.begin(), addons.end(), IsLookAndFeelTypeAddon))
  {
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(24997)));
    item->SetPath(URIUtils::AddFileToFolder(path.Get(), CATEGORY_LOOK_AND_FEEL));
    item->m_bIsFolder = true;
    const std::string thumb = "DefaultAddonLookAndFeel.png";
    if (g_TextureManager.HasTexture(thumb))
      item->SetArt("thumb", thumb);
    items.Add(item);
  }
  if (std::any_of(addons.begin(), addons.end(), IsGameAddon))
  {
    CFileItemPtr item(new CFileItem(CAddonInfo::TranslateType(ADDON_GAME, true)));
    item->SetPath(URIUtils::AddFileToFolder(path.Get(), CATEGORY_GAME_ADDONS));
    item->m_bIsFolder = true;
    const std::string thumb = CAddonInfo::TranslateIconType(ADDON_GAME);
    if (g_TextureManager.HasTexture(thumb))
      item->SetArt("thumb", thumb);
    items.Add(item);
  }

  std::set<TYPE> uncategorized;
  for (unsigned int i = ADDON_UNKNOWN + 1; i < ADDON_MAX - 1; ++i)
  {
    const TYPE type = (TYPE)i;
    if (!IsInfoProviderType(type) && !IsLookAndFeelType(type) && !IsDependencyType(type) && !IsGameType(type))
      uncategorized.insert(static_cast<TYPE>(i));
  }
  GenerateTypeListing(path, uncategorized, addons, items);
}

//Creates sub-categories or addon list for a category
static void GenerateCategoryListing(const CURL& path, AddonInfos& addons,
    CFileItemList& items)
{
  const std::string category = path.GetFileName();
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
    items.SetProperty("addoncategory", CAddonInfo::TranslateType(ADDON_GAME, true));
    items.SetLabel(CAddonInfo::TranslateType(ADDON_GAME, true));
    GenerateGameListing(path, addons, items);
  }
  else if (category == CATEGORY_EMULATORS)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35207)); // Emulators
    addons.erase(std::remove_if(addons.begin(), addons.end(),
        [](const AddonInfoPtr& addon){ return !IsEmulator(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35207)); // Emulators
  }
  else if (category == CATEGORY_STANDALONE_GAMES)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35208)); // Standalone games
    addons.erase(std::remove_if(addons.begin(), addons.end(),
        [](const AddonInfoPtr& addon){ return !IsStandaloneGame(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35208)); // Standalone games
  }
  else if (category == CATEGORY_GAME_PROVIDERS)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35220)); // Game providers
    addons.erase(std::remove_if(addons.begin(), addons.end(),
                                [](const AddonInfoPtr& addon){ return !IsGameProvider(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35220)); // Game providers
  }
  else if (category == CATEGORY_GAME_RESOURCES)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35209)); // Game resources
    addons.erase(std::remove_if(addons.begin(), addons.end(),
                                [](const AddonInfoPtr& addon){ return !IsGameResource(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35209)); // Game resources
  }
  else if (category == CATEGORY_GAME_SUPPORT_ADDONS)
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(35216)); // Support add-ons
    addons.erase(std::remove_if(addons.begin(), addons.end(),
      [](const AddonInfoPtr& addon) { return !IsGameSupportAddon(addon); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(35216)); // Support add-ons
  }
  else
  { // fallback to addon type
    TYPE type = CAddonInfo::TranslateType(category);
    items.SetProperty("addoncategory", CAddonInfo::TranslateType(type, true));
    addons.erase(std::remove_if(addons.begin(), addons.end(),
        [type](const AddonInfoPtr& addon){ return !addon->IsType(type); }), addons.end());
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

  AddonInfos addons;
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

  AddonInfos addons;
  CAddonMgr::GetInstance().GetAddonInfos(addons, false, ADDON_UNKNOWN, true);
  addons.erase(std::remove_if(addons.begin(), addons.end(),
                              std::not1(std::ptr_fun(IsUserInstalled))), addons.end());
  if (addons.empty())
    return;

  const std::string category = path.GetFileName();
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
  AddonInfos all;
  CAddonMgr::GetInstance().GetAddonInfos(all, false, ADDON_UNKNOWN, true);

  AddonInfos deps;
  std::copy_if(all.begin(), all.end(), std::back_inserter(deps),
      [&](const AddonInfoPtr& _){ return !IsUserInstalled(_); });

  CAddonsDirectory::GenerateAddonListing(path, deps, items, g_localizeStrings.Get(24996));

  //Set orphaned status
  std::set<std::string> orphaned;
  for (const auto& addon : deps)
  {
    if (IsOrphaned(addon, all))
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
  AddonInfos addons = CAddonMgr::GetInstance().GetAvailableUpdates();
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24043));

  if (items.Size() > 1)
  {
    CFileItemPtr item(new CFileItem("addons://update_all/", false));
    item->SetLabel(g_localizeStrings.Get(24122));
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
}

static void RunningAddons(const CURL& path, CFileItemList &items)
{
  AddonInfos addons = CAddonMgr::GetInstance().GetAddonInfos(false, ADDON_SERVICE);

  addons.erase(std::remove_if(addons.begin(), addons.end(),
      [](const AddonInfoPtr& addon){ return !CScriptInvocationManager::GetInstance().IsRunning(addon->Type(ADDON_SERVICE)->LibPath()); }), addons.end());
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24994));
}

static bool Browse(const CURL& path, CFileItemList &items)
{
  const std::string repo = path.GetHostName();

  AddonInfos addons;
  items.SetPath(path.Get());
  if (repo == "all")
  {
    CAddonDatabase database;
    if (!database.Open() || !database.GetRepositoryContent(addons))
      return false;
    items.SetProperty("reponame", g_localizeStrings.Get(24087));
    items.SetLabel(g_localizeStrings.Get(24087));
  }
  else
  {
    AddonPtr addon;
    if (!CAddonMgr::GetInstance().GetAddon(repo, addon, ADDON_REPOSITORY))
      return false;

    CAddonDatabase database;
    database.Open();
    if (!database.GetRepositoryContent(addon->ID(), addons))
    {
      //Repo content is invalid. Ask for update and wait.
      CRepositoryUpdater::GetInstance().CheckForUpdates(std::static_pointer_cast<CRepository>(addon));
      CRepositoryUpdater::GetInstance().Await();

      if (!database.GetRepositoryContent(addon->ID(), addons))
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{addon->Name()}, CVariant{24991});
        return false;
      }
    }

    items.SetProperty("reponame", addon->Name());
    items.SetLabel(addon->Name());
  }

  const std::string category = path.GetFileName();
  if (category.empty())
    GenerateMainCategoryListing(path, addons, items);
  else
    GenerateCategoryListing(path, addons, items);
  return true;
}

static bool GetRecentlyUpdatedAddons(AddonInfos& addons)
{
  addons = CAddonMgr::GetInstance().GetAddonInfos(true, ADDON_UNKNOWN, true);
  if (addons.empty())
    return false;

  auto limit = CDateTime::GetCurrentDateTime() - CDateTimeSpan(1, 0, 0, 0);
  auto isOld = [limit](const AddonInfoPtr& addon){ return addon->LastUpdated() < limit; };
  addons.erase(std::remove_if(addons.begin(), addons.end(), isOld), addons.end());

  return true;
}

static bool HasRecentlyUpdatedAddons()
{
  AddonInfos addons;
  return GetRecentlyUpdatedAddons(addons) && !addons.empty();
}

static bool Repos(const CURL& path, CFileItemList &items)
{
  items.SetLabel(g_localizeStrings.Get(24033));
  AddonInfos addons = CAddonMgr::GetInstance().GetAddonInfos(true, ADDON_REPOSITORY);
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
    CFileItemPtr item = CAddonsDirectory::FileItemFromAddonInfo(repo, "addons://" + repo->ID(), true);
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
    item->SetIconImage("DefaultAddonsInstalled.png");
    items.Add(item);
  }
  if (CAddonMgr::GetInstance().HasAvailableUpdates())
  {
    CFileItemPtr item(new CFileItem("addons://outdated/", true));
    item->SetLabel(g_localizeStrings.Get(24043));
    item->SetIconImage("DefaultAddonsUpdates.png");
    items.Add(item);
  }
  if (CAddonInstaller::GetInstance().IsDownloading())
  {
    CFileItemPtr item(new CFileItem("addons://downloading/", true));
    item->SetLabel(g_localizeStrings.Get(24067));
    item->SetIconImage("DefaultNetwork.png");
    items.Add(item);
  }
  if (CServiceBroker::GetSettings().GetInt(CSettings::SETTING_ADDONS_AUTOUPDATES) == ADDON::AUTO_UPDATES_ON
      && HasRecentlyUpdatedAddons())
  {
    CFileItemPtr item(new CFileItem("addons://recently_updated/", true));
    item->SetLabel(g_localizeStrings.Get(24004));
    item->SetIconImage("DefaultAddonsRecentlyUpdated.png");
    items.Add(item);
  }
  if (CAddonMgr::GetInstance().HasEnabledAddons(ADDON_REPOSITORY))
  {
    CFileItemPtr item(new CFileItem("addons://repos/", true));
    item->SetLabel(g_localizeStrings.Get(24033));
    item->SetIconImage("DefaultAddonsRepo.png");
    items.Add(item);
  }
  {
    CFileItemPtr item(new CFileItem("addons://install/", false));
    item->SetLabel(g_localizeStrings.Get(24041));
    item->SetIconImage("DefaultAddonsZip.png");
    items.Add(item);
  }
  {
    CFileItemPtr item(new CFileItem("addons://search/", true));
    item->SetLabel(g_localizeStrings.Get(137));
    item->SetIconImage("DefaultAddonsSearch.png");
    items.Add(item);
  }
}

bool CAddonsDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string tmp(url.Get());
  URIUtils::RemoveSlashAtEnd(tmp);
  CURL path(tmp);
  const std::string endpoint = path.GetHostName();
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
  // PVR & adsp hardcodes this view so keep for compatibility
  else if (endpoint == "disabled")
  {
    ADDON::TYPE type;

    if (path.GetFileName() == "kodi.pvrclient")
      type = ADDON_PVRDLL;
    else if (path.GetFileName() == "kodi.adsp")
      type = ADDON_ADSPDLL;
    else if (path.GetFileName() == "kodi.vfs")
      type = ADDON_VFS;
    else
      type = ADDON_UNKNOWN;

    if (type != ADDON_UNKNOWN)
    {
      AddonInfos addons = CAddonMgr::GetInstance().GetAddonInfos(false, type);
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
    AddonInfos addons;
    if (!GetRecentlyUpdatedAddons(addons))
      return false;

    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24004));
    return true;

  }
  else if (endpoint == "downloading")
  {
    AddonInfos addons;
    CAddonInstaller::GetInstance().GetInstallList(addons);
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24067));
    return true;
  }
  else if (endpoint == "more")
  {
    std::string type = path.GetFileName();
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
  return url.GetHostName() == "repos"
      || url.GetHostName() == "all"
      || url.GetHostName() == "search"
      || CAddonMgr::GetInstance().IsAddonEnabled(url.GetHostName()); // ADDON_REPOSITORY
}

void CAddonsDirectory::GenerateAddonListing(const CURL &path,
    const AddonInfos& addons, CFileItemList &items, const std::string label)
{
  std::set<std::string> outdated;
  for (const auto& addon : CAddonMgr::GetInstance().GetAvailableUpdates())
    outdated.insert(addon->ID());

  items.ClearItems();
  items.SetContent("addons");
  items.SetLabel(label);
  for (const auto& addon : addons)
  {
    CURL itemPath = path;
    itemPath.SetFileName(addon->ID());
    CFileItemPtr pItem = FileItemFromAddonInfo(addon, itemPath.Get(), false);

    bool installed = CAddonMgr::GetInstance().IsAddonInstalled(addon->ID());
    bool enabled = CAddonMgr::GetInstance().IsAddonEnabled(addon->ID());
    bool hasUpdate = outdated.find(addon->ID()) != outdated.end();

    pItem->SetProperty("Addon.IsInstalled", installed);
    pItem->SetProperty("Addon.IsEnabled", installed && enabled);
    pItem->SetProperty("Addon.HasUpdate", hasUpdate);

    if (installed)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(305));
    if (!enabled)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24023));
    if (hasUpdate)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24068));
    else if (!addon->Broken().empty())
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24098));

    items.Add(pItem);
  }
}

CFileItemPtr CAddonsDirectory::FileItemFromAddonInfo(const AddonInfoPtr &addonInfo,
    const std::string& path, bool folder)
{
  if (!addonInfo)
    return CFileItemPtr();

  CFileItemPtr item(new CFileItem(addonInfo));
  item->m_bIsFolder = folder;
  item->SetPath(path);

  std::string strLabel(addonInfo->Name());
  if (CURL(path).GetHostName() == "search")
    strLabel = StringUtils::Format("%s - %s", CAddonInfo::TranslateType(addonInfo->MainType(), true).c_str(), addonInfo->Name().c_str());
  item->SetLabel(strLabel);
  item->SetArt("thumb", addonInfo->Icon());
  item->SetIconImage("DefaultAddon.png");
  if (URIUtils::IsInternetStream(addonInfo->FanArt()) || CFile::Exists(addonInfo->FanArt()))
    item->SetArt("fanart", addonInfo->FanArt());

  //! @todo fix hacks that depends on these
  item->SetProperty("Addon.ID", addonInfo->ID());
  item->SetProperty("Addon.Name", addonInfo->Name());
  strLabel = addonInfo->Language();
  if (!strLabel.empty())
    item->SetProperty("Addon.Language", strLabel);

  return item;
}

bool CAddonsDirectory::GetScriptsAndPlugins(const std::string &content, AddonInfos &addons)
{
  AddonInfos tempAddons;

  TYPE type = CAddonInfo::TranslateSubContent(content);
  if (type == ADDON_UNKNOWN)
    return false;

  tempAddons = CAddonMgr::GetInstance().GetAddonInfos(true, ADDON_PLUGIN);
  for (auto addon : tempAddons)
  {
    if (addon->ProvidesSubContent(type, ADDON_PLUGIN))
      addons.push_back(addon);
  }
  tempAddons.clear();

  tempAddons = CAddonMgr::GetInstance().GetAddonInfos(true, ADDON_SCRIPT);
  for (auto addon : tempAddons)
  {
    if (addon->ProvidesSubContent(type, ADDON_SCRIPT))
      addons.push_back(addon);
  }
  tempAddons.clear();

  if (type == ADDON_GAME)
  {
    tempAddons = CAddonMgr::GetInstance().GetAddonInfos(true, ADDON_GAMEDLL);
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
  std::string path;
  AddonInfos addons;
  if (!GetScriptsAndPlugins(content, addons))
    return false;

  for (auto addon : addons)
  {
    bool bIsFolder = false;

    if (addon->IsType(ADDON_PLUGIN))
    {
      bIsFolder = true;
      path = "plugin://" + addon->ID();
      if (addon->ProvidesSeveralSubContents())
      {
        CURL url(path);
        std::string opt = StringUtils::Format("?content_type=%s", content.c_str());
        url.SetOptions(opt);
        path = url.Get();
      }
    }
    else if (addon->IsType(ADDON_SCRIPT))
    {
      path = "script://" + addon->ID();
    }
    else if (addon->IsType(ADDON_GAMEDLL))
    {
      // Kodi fails to launch games with empty path from home screen
      path = "game://" + addon->ID();
    }

    items.Add(FileItemFromAddonInfo(addon, path, bIsFolder));
  }

  items.SetContent("addons");
  items.SetLabel(g_localizeStrings.Get(24001)); // Add-ons

  return true;
}

}
