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
#include "AddonsDirectory.h"
#include "addons/AddonDatabase.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "FileItem.h"
#include "addons/AddonInstaller.h"
#include "addons/PluginSource.h"
#include "guilib/TextureManager.h"
#include "File.h"
#include "SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "URL.h"

using namespace ADDON;

namespace XFILE
{

CAddonsDirectory::CAddonsDirectory(void)
{
}

CAddonsDirectory::~CAddonsDirectory(void)
{}


const std::array<TYPE, 4> dependencyTypes = {
    ADDON_VIZ_LIBRARY,
    ADDON_SCRAPER_LIBRARY,
    ADDON_SCRIPT_LIBRARY,
    ADDON_SCRIPT_MODULE,
};

const std::array<TYPE, 5> infoProviderTypes = {
  ADDON_SCRAPER_ALBUMS,
  ADDON_SCRAPER_ARTISTS,
  ADDON_SCRAPER_MOVIES,
  ADDON_SCRAPER_MUSICVIDEOS,
  ADDON_SCRAPER_TVSHOWS,
};


static bool IsInfoProviderType(const AddonPtr& addon)
{
  return std::find(infoProviderTypes.begin(), infoProviderTypes.end(), addon->Type()) != infoProviderTypes.end();
}

static bool IsSystemAddon(const AddonPtr& addon)
{
  return StringUtils::StartsWith(addon->Path(), CSpecialProtocol::TranslatePath("special://xbmc/addons"));
}


static bool IsUserInstalled(const AddonPtr& addon)
{
  return std::find_if(dependencyTypes.begin(), dependencyTypes.end(),
      [&](TYPE type){ return addon->IsType(type); }) == dependencyTypes.end();
}


static bool IsOrphaned(const AddonPtr& addon, const VECADDONS& all)
{
  if (IsSystemAddon(addon) || IsUserInstalled(addon))
    return false;

  for (const AddonPtr& other : all)
  {
    const auto& deps = other->GetDeps();
    if (deps.find(addon->ID()) != deps.end())
      return false;
  }
  return true;
}


static void SetUpdateAvailProperties(CFileItemList &items)
{
  CAddonDatabase database;
  database.Open();
  for (int i = 0; i < items.Size(); ++i)
  {
    const std::string addonId = items[i]->GetProperty("Addon.ID").asString();
    if (!CAddonMgr::Get().IsAddonDisabled(addonId))
    {
      const AddonVersion installedVersion = AddonVersion(items[i]->GetProperty("Addon.Version").asString());
      AddonPtr repoAddon;
      database.GetAddon(addonId, repoAddon);
      if (repoAddon && repoAddon->Version() > installedVersion &&
          !database.IsAddonBlacklisted(addonId, repoAddon->Version().asString()))
      {
        items[i]->SetProperty("Addon.Status", g_localizeStrings.Get(24068));
        items[i]->SetProperty("Addon.UpdateAvail", true);
      }
    }
  }
}

static void GenerateCategoryListing(const CURL& path, const VECADDONS& addons, CFileItemList& items)
{
  if (std::any_of(addons.begin(), addons.end(), IsInfoProviderType))
  {
    CFileItemPtr item(new CFileItem(g_localizeStrings.Get(24993)));
    item->SetPath(URIUtils::AddFileToFolder(path.Get(), "group.infoproviders"));
    item->m_bIsFolder = true;
    const std::string thumb = "DefaultAddonInfoProvider.png";
    if (g_TextureManager.HasTexture(thumb))
      item->SetArt("thumb", thumb);
    items.Add(item);
  }
  for (unsigned int i = ADDON_UNKNOWN + 1; i < ADDON_MAX - 1; ++i)
  {
    const TYPE type = (TYPE)i;
    if (std::find(dependencyTypes.begin(), dependencyTypes.end(), type) != dependencyTypes.end())
      continue;
    if (std::find(infoProviderTypes.begin(), infoProviderTypes.end(), type) != infoProviderTypes.end())
      continue;

    for (unsigned int j = 0; j < addons.size(); ++j)
    {
      if (addons[j]->IsType(type))
      {
        CFileItemPtr item(new CFileItem(TranslateType(type, true)));
        item->SetPath(URIUtils::AddFileToFolder(path.Get(), TranslateType(type, false)));
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

static void GenerateAddonListingForCategory(const CURL& path, VECADDONS& addons, CFileItemList& items)
{
  const std::string category = path.GetFileName();
  if (category == "group.infoproviders")
  {
    items.SetProperty("addoncategory", g_localizeStrings.Get(24993));
    items.SetLabel(g_localizeStrings.Get(24993));

    for (const auto& type : infoProviderTypes)
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
  CAddonMgr::Get().GetAllAddons(addons, true);
  CAddonMgr::Get().GetAllAddons(addons, false);
  addons.erase(std::remove_if(addons.begin(), addons.end(),
                              std::not1(std::ptr_fun(IsUserInstalled))), addons.end());
  if (addons.empty())
    return;

  const std::string category = path.GetFileName();
  if (category.empty())
  {
    GenerateCategoryListing(path, addons, items);

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
  {
    CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24998));
    SetUpdateAvailProperties(items);
  }
  else
  {
    GenerateAddonListingForCategory(path, addons, items);
    SetUpdateAvailProperties(items);
  }
}

static void DependencyAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS all;
  CAddonMgr::Get().GetAllAddons(all, true);
  CAddonMgr::Get().GetAllAddons(all, false);

  VECADDONS deps;
  std::copy_if(all.begin(), all.end(), std::back_inserter(deps),
      [&](const AddonPtr& _){ return !IsUserInstalled(_) && !IsOrphaned(_, all); });

  CAddonsDirectory::GenerateAddonListing(path, deps, items, g_localizeStrings.Get(24996));
  SetUpdateAvailProperties(items);
}

static void OrphanedAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS all;
  CAddonMgr::Get().GetAllAddons(all, true);
  CAddonMgr::Get().GetAllAddons(all, false);

  VECADDONS orphaned;
  std::copy_if(all.begin(), all.end(), std::back_inserter(orphaned),
      [&](const AddonPtr& _){ return IsOrphaned(_, all); });

  CAddonsDirectory::GenerateAddonListing(path, orphaned, items, g_localizeStrings.Get(24995));
}

static bool HaveOrphaned()
{
  VECADDONS addons;
  CAddonMgr::Get().GetAllAddons(addons, true);
  CAddonMgr::Get().GetAllAddons(addons, false);
  return std::any_of(addons.begin(), addons.end(),
                     [&](const AddonPtr& _){ return IsOrphaned(_, addons); });
}

static void OutdatedAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS addons;
  // Wait for running update to complete
  CAddonInstaller::Get().UpdateRepos(false, true);
  CAddonMgr::Get().GetAllOutdatedAddons(addons);
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24043));

  if (items.Size() > 1)
  {
    CFileItemPtr item(new CFileItem("addons://update_all/", true));
    item->SetLabel(g_localizeStrings.Get(24122));
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
}

static void RunningAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS addons;
  CAddonMgr::Get().GetAddons(ADDON_SERVICE, addons);

  addons.erase(std::remove_if(addons.begin(), addons.end(),
      [](const AddonPtr& addon){ return !CScriptInvocationManager::Get().IsRunning(addon->LibPath()); }), addons.end());
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24994));
}

static bool Browse(const CURL& path, CFileItemList &items)
{
  const std::string repo = path.GetHostName();

  VECADDONS addons;
  if (repo == "all")
  {
    CAddonDatabase database;
    database.Open();
    database.GetAddons(addons);
    items.SetProperty("reponame", g_localizeStrings.Get(24087));
    items.SetLabel(g_localizeStrings.Get(24087));
  }
  else
  {
    AddonPtr addon;
    if (!CAddonMgr::Get().GetAddon(repo, addon, ADDON_REPOSITORY))
      return false;
    //Wait for runnig update to complete
    CAddonInstaller::Get().UpdateRepos(false, true);
    CAddonDatabase database;
    database.Open();
    if (!database.GetRepository(addon->ID(), addons))
      return false;
    items.SetProperty("reponame", addon->Name());
    items.SetLabel(addon->Name());
  }

  const std::string category = path.GetFileName();
  if (category.empty())
    GenerateCategoryListing(path, addons, items);
  else
    GenerateAddonListingForCategory(path, addons, items);
  return true;
}


static bool Repos(const CURL& path, CFileItemList &items)
{
  items.SetLabel(g_localizeStrings.Get(24033));

  VECADDONS addons;
  CAddonMgr::Get().GetAddons(ADDON_REPOSITORY, addons, true);
  if (addons.empty())
    return true;
  else if (addons.size() == 1)
    return Browse(CURL("addons://" + addons[0]->ID()), items);
  else
  {
    CFileItemPtr item(new CFileItem("addons://all/", true));
    item->SetLabel(g_localizeStrings.Get(24087));
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }

  for (const auto& repo : addons)
  {
    CFileItemPtr item = CAddonsDirectory::FileItemFromAddon(repo, "addons://" + repo->ID(), true);
    CAddonDatabase::SetPropertiesFromAddon(repo, item);
    items.Add(item);
  }
  return true;
}

static void Manage(CFileItemList &items)
{
  items.SetLabel(g_localizeStrings.Get(24992));

  {
    CFileItemPtr item(new CFileItem("addons://dependencies/", true));
    item->SetLabel(g_localizeStrings.Get(24996));
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
  if (HaveOrphaned())
  {
    CFileItemPtr item(new CFileItem("addons://orphaned/", true));
    item->SetLabel(g_localizeStrings.Get(24995));
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
  {
    CFileItemPtr item(new CFileItem("addons://running/", true));
    item->SetLabel(g_localizeStrings.Get(24994));
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
}

bool CAddonsDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string tmp(url.Get());
  URIUtils::RemoveSlashAtEnd(tmp);
  CURL path(tmp);
  const std::string endpoint = path.GetHostName();
  items.ClearProperties();
  items.SetContent("addons");
  items.SetPath(path.Get());

  if (endpoint == "user")
  {
    UserInstalledAddons(path, items);
    return true;
  }
  else if (endpoint == "dependencies")
  {
    DependencyAddons(path, items);
    return true;
  }
  else if (endpoint == "orphaned")
  {
    OrphanedAddons(path, items);
    return true;
  }
  //Pvr hardcodes this view so keep for compatibility
  else if (endpoint == "disabled" && path.GetFileName() == "xbmc.pvrclient")
  {
    VECADDONS addons;
    if (CAddonMgr::Get().GetAddons(ADDON_PVRDLL, addons, false))
    {
      CAddonsDirectory::GenerateAddonListing(path, addons, items, TranslateType(ADDON_PVRDLL, true));
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
  else if (endpoint == "manage")
  {
    Manage(items);
    return true;
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
      || CAddonMgr::Get().GetAddon(url.GetHostName(), tmp, ADDON_REPOSITORY);
}

void CAddonsDirectory::GenerateAddonListing(const CURL &path,
                                            const VECADDONS& addons,
                                            CFileItemList &items,
                                            const std::string label)
{
  items.ClearItems();
  items.SetLabel(label);
  for (const auto& addon : addons)
  {
    CURL itemPath = path;
    itemPath.SetFileName(addon->ID());
    CFileItemPtr pItem = FileItemFromAddon(addon, itemPath.Get(), false);

    AddonPtr installedAddon;
    if (CAddonMgr::Get().GetAddon(addon->ID(), installedAddon))
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(305));
    else if (CAddonMgr::Get().IsAddonDisabled(addon->ID()))
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(24023));

    if (addon->Props().broken == "DEPSNOTMET")
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(24049));
    else if (!addon->Props().broken.empty())
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(24098));
    if (installedAddon && installedAddon->Version() < addon->Version())
    {
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(24068));
      pItem->SetProperty("Addon.UpdateAvail", true);
    }
    CAddonDatabase::SetPropertiesFromAddon(addon,pItem);
    items.Add(pItem);
  }
}

CFileItemPtr CAddonsDirectory::FileItemFromAddon(const AddonPtr &addon, const std::string& path, bool folder)
{
  if (!addon)
    return CFileItemPtr();

  CFileItemPtr item(new CFileItem(path, folder));

  std::string strLabel(addon->Name());
  if (CURL(path).GetHostName() == "search")
    strLabel = StringUtils::Format("%s - %s", TranslateType(addon->Type(), true).c_str(), addon->Name().c_str());
  item->SetLabel(strLabel);
  item->SetArt("thumb", addon->Icon());
  item->SetLabelPreformated(true);
  item->SetIconImage("DefaultAddon.png");
  if (URIUtils::IsInternetStream(addon->FanArt()) || CFile::Exists(addon->FanArt()))
    item->SetArt("fanart", addon->FanArt());
  CAddonDatabase::SetPropertiesFromAddon(addon, item);
  return item;
}

bool CAddonsDirectory::GetScriptsAndPlugins(const std::string &content, VECADDONS &addons)
{
  CPluginSource::Content type = CPluginSource::Translate(content);
  if (type == CPluginSource::UNKNOWN)
    return false;

  VECADDONS tempAddons;
  CAddonMgr::Get().GetAddons(ADDON_PLUGIN, tempAddons);
  for (unsigned i=0; i<tempAddons.size(); i++)
  {
    PluginPtr plugin = std::dynamic_pointer_cast<CPluginSource>(tempAddons[i]);
    if (plugin && plugin->Provides(type))
      addons.push_back(tempAddons[i]);
  }
  tempAddons.clear();
  CAddonMgr::Get().GetAddons(ADDON_SCRIPT, tempAddons);
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

  items.Add(GetMoreItem(content));

  items.SetContent("addons");
  items.SetLabel(g_localizeStrings.Get(24001)); // Add-ons

  return items.Size() > 0;
}

CFileItemPtr CAddonsDirectory::GetMoreItem(const std::string &content)
{
  CFileItemPtr item(new CFileItem("addons://more/"+content,false));
  item->SetLabelPreformated(true);
  item->SetLabel(g_localizeStrings.Get(21452));
  item->SetIconImage("DefaultAddon.png");
  item->SetSpecialSort(SortSpecialOnBottom);
  return item;
}
  
}

