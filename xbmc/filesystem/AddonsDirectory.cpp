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
#include "addons/AddonDatabase.h"
#include "addons/AddonSystemSettings.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "FileItem.h"
#include "addons/AddonInstaller.h"
#include "addons/PluginSource.h"
#include "addons/RepositoryUpdater.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/TextureManager.h"
#include "File.h"
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
};

static bool IsInfoProviderType(TYPE type)
{
  return infoProviderTypes.find(type) != infoProviderTypes.end();
}

static bool IsInfoProviderTypeAddon(const AddonPtr& addon)
{
  return IsInfoProviderType(addon->Type());
}

static bool IsLookAndFeelType(TYPE type)
{
  return lookAndFeelTypes.find(type) != lookAndFeelTypes.end();
}

static bool IsLookAndFeelTypeAddon(const AddonPtr& addon)
{
  return IsLookAndFeelType(addon->Type());
}

static bool IsGameType(TYPE type)
{
  return gameTypes.find(type) != gameTypes.end();
}

static bool IsGameAddon(const AddonPtr& addon)
{
  return IsGameType(addon->Type());
}

static bool IsDependecyType(TYPE type)
{
  return dependencyTypes.find(type) != dependencyTypes.end();
}

static bool IsUserInstalled(const AddonPtr& addon)
{
  return std::find_if(dependencyTypes.begin(), dependencyTypes.end(),
      [&](TYPE type){ return addon->IsType(type); }) == dependencyTypes.end();
}


static bool IsOrphaned(const AddonPtr& addon, const VECADDONS& all)
{
  if (CAddonMgr::GetInstance().IsSystemAddon(addon->ID()) || IsUserInstalled(addon))
    return false;

  for (const AddonPtr& other : all)
  {
    const auto& deps = other->GetDeps();
    if (deps.find(addon->ID()) != deps.end())
      return false;
  }
  return true;
}


// Creates categories from addon types, if we have any addons with that type.
static void GenerateTypeListing(const CURL& path, const std::set<TYPE>& types,
    const VECADDONS& addons, CFileItemList& items)
{
  for (const auto& type : types)
  {
    for (const auto& addon : addons)
    {
      if (addon->IsType(type))
      {
        CFileItemPtr item(new CFileItem(TranslateType(type, true)));
        CURL itemPath = path;
        itemPath.SetFileName(TranslateType(type, false));
        item->SetPath(itemPath.Get());
        item->m_bIsFolder = true;
        std::string thumb = GetIcon(type);
        if (!thumb.empty() && g_TextureManager.HasTexture(thumb))
          item->SetArt("thumb", thumb);
        items.Add(item);
        break;
      }
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
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(35049))); // Game add-ons
    item->SetPath(URIUtils::AddFileToFolder(path.Get(), CATEGORY_GAME_ADDONS));
    item->m_bIsFolder = true;
    const std::string thumb = "DefaultGameAddons.png";
    if (g_TextureManager.HasTexture(thumb))
      item->SetArt("thumb", thumb);
    items.Add(item);
  }

  std::set<TYPE> uncategorized;
  for (unsigned int i = ADDON_UNKNOWN + 1; i < ADDON_MAX - 1; ++i)
  {
    const TYPE type = (TYPE)i;
    if (!IsInfoProviderType(type) && !IsLookAndFeelType(type) && !IsDependecyType(type))
      uncategorized.insert(static_cast<TYPE>(i));
  }
  GenerateTypeListing(path, uncategorized, addons, items);
}

//Creates sub-categories or addon list for a category
static void GenerateCategoryListing(const CURL& path, VECADDONS& addons,
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
    items.SetProperty("addoncategory", g_localizeStrings.Get(35049)); // Game add-ons
    items.SetLabel(g_localizeStrings.Get(35049)); // Game add-ons
    GenerateTypeListing(path, gameTypes, addons, items);
  }
  else
  { // fallback to addon type
    TYPE type = TranslateType(category);
    items.SetProperty("addoncategory", TranslateType(type, true));
    addons.erase(std::remove_if(addons.begin(), addons.end(),
        [type](const AddonPtr& addon){ return !addon->IsType(type); }), addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, TranslateType(type, true));
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
  CAddonMgr::GetInstance().GetInstalledAddons(addons);
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
  VECADDONS all;
  CAddonMgr::GetInstance().GetInstalledAddons(all);

  VECADDONS deps;
  std::copy_if(all.begin(), all.end(), std::back_inserter(deps),
      [&](const AddonPtr& _){ return !IsUserInstalled(_); });

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
  VECADDONS addons = CAddonMgr::GetInstance().GetAvailableUpdates();
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
  VECADDONS addons;
  CAddonMgr::GetInstance().GetAddons(addons, ADDON_SERVICE);

  addons.erase(std::remove_if(addons.begin(), addons.end(),
      [](const AddonPtr& addon){ return !CScriptInvocationManager::GetInstance().IsRunning(addon->LibPath()); }), addons.end());
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24994));
}

static bool Browse(const CURL& path, CFileItemList &items)
{
  const std::string repo = path.GetHostName();

  VECADDONS addons;
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

static bool GetRecentlyUpdatedAddons(VECADDONS& addons)
{
  if (!CAddonMgr::GetInstance().GetInstalledAddons(addons))
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
  CAddonMgr::GetInstance().GetAddons(addons, ADDON_REPOSITORY);
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
  if (CSettings::GetInstance().GetInt(CSettings::SETTING_ADDONS_AUTOUPDATES) == ADDON::AUTO_UPDATES_ON
      && HasRecentlyUpdatedAddons())
  {
    CFileItemPtr item(new CFileItem("addons://recently_updated/", true));
    item->SetLabel(g_localizeStrings.Get(24004));
    item->SetIconImage("DefaultAddonsRecentlyUpdated.png");
    items.Add(item);
  }
  if (CAddonMgr::GetInstance().HasAddons(ADDON_REPOSITORY))
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
    VECADDONS addons;
    ADDON::TYPE type;

    if (path.GetFileName() == "xbmc.pvrclient")
      type = ADDON_PVRDLL;
    else if (path.GetFileName() == "kodi.adsp")
      type = ADDON_ADSPDLL;
    else
      type = ADDON_UNKNOWN;

    if (type != ADDON_UNKNOWN && CAddonMgr::GetInstance().GetInstalledAddons(addons, type))
    {
      CAddonsDirectory::GenerateAddonListing(path, addons, items, TranslateType(type, true));
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
    std::string type = path.GetFileName();
    if (type == "video" || type == "audio" || type == "image" || type == "executable")
      return Browse(CURL("addons://all/xbmc.addon." + type), items);
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
      || CAddonMgr::GetInstance().GetAddon(url.GetHostName(), tmp, ADDON_REPOSITORY);
}

void CAddonsDirectory::GenerateAddonListing(const CURL &path,
    const VECADDONS& addons, CFileItemList &items, const std::string label)
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
    CFileItemPtr pItem = FileItemFromAddon(addon, itemPath.Get(), false);

    AddonPtr localAddon;
    bool installed = CAddonMgr::GetInstance().GetAddon(addon->ID(), localAddon, ADDON_UNKNOWN, false);
    bool disabled = CAddonMgr::GetInstance().IsAddonDisabled(addon->ID());
    bool hasUpdate = outdated.find(addon->ID()) != outdated.end();

    pItem->SetProperty("Addon.IsInstalled", installed);
    pItem->SetProperty("Addon.IsEnabled", installed && !disabled);
    pItem->SetProperty("Addon.HasUpdate", hasUpdate);

    if (installed)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(305));
    if (disabled)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24023));
    if (hasUpdate)
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24068));
    if (addon->Broken() == "DEPSNOTMET")
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24049));
    else if (!addon->Broken().empty())
      pItem->SetProperty("Addon.Status", g_localizeStrings.Get(24098));

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
    strLabel = StringUtils::Format("%s - %s", TranslateType(addon->Type(), true).c_str(), addon->Name().c_str());
  item->SetLabel(strLabel);
  item->SetArt("thumb", addon->Icon());
  item->SetIconImage("DefaultAddon.png");
  if (URIUtils::IsInternetStream(addon->FanArt()) || CFile::Exists(addon->FanArt()))
    item->SetArt("fanart", addon->FanArt());

  //! @todo fix hacks that depends on these
  item->SetProperty("Addon.ID", addon->ID());
  item->SetProperty("Addon.Name", addon->Name());
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
  CAddonMgr::GetInstance().GetAddons(tempAddons, ADDON_PLUGIN);
  for (unsigned i=0; i<tempAddons.size(); i++)
  {
    PluginPtr plugin = std::dynamic_pointer_cast<CPluginSource>(tempAddons[i]);
    if (plugin && plugin->Provides(type))
      addons.push_back(tempAddons[i]);
  }
  tempAddons.clear();
  CAddonMgr::GetInstance().GetAddons(tempAddons, ADDON_SCRIPT);
  for (unsigned i=0; i<tempAddons.size(); i++)
  {
    PluginPtr plugin = std::dynamic_pointer_cast<CPluginSource>(tempAddons[i]);
    if (plugin && plugin->Provides(type))
      addons.push_back(tempAddons[i]);
  }
  return true;
}

bool CAddonsDirectory::GetScriptsAndPlugins(const std::string &content, CFileItemList &items)
{
  items.Clear();

  VECADDONS addons;
  if (!GetScriptsAndPlugins(content, addons))
    return false;

  for (VECADDONS::const_iterator it = addons.begin(); it != addons.end(); ++it)
  {
    const AddonPtr addon = *it;
    const std::string prot = addon->Type() == ADDON_PLUGIN ? "plugin://" : "script://";
    CFileItemPtr item(FileItemFromAddon(addon, prot + addon->ID(), addon->Type() == ADDON_PLUGIN));
    PluginPtr plugin = std::dynamic_pointer_cast<CPluginSource>(addon);
    if (plugin->ProvidesSeveral())
    {
      CURL url = item->GetURL();
      std::string opt = StringUtils::Format("?content_type=%s",content.c_str());
      url.SetOptions(opt);
      item->SetURL(url);
    }
    items.Add(item);
  }

  items.SetContent("addons");
  items.SetLabel(g_localizeStrings.Get(24001)); // Add-ons

  return true;
}

}

