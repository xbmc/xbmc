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
#include "DirectoryFactory.h"
#include "Directory.h"
#include "DirectoryCache.h"
#include "FileItem.h"
#include "addons/Repository.h"
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


bool IsSystemAddon(const AddonPtr& addon)
{
  return StringUtils::StartsWith(addon->Path(), CSpecialProtocol::TranslatePath("special://xbmc/addons"));
}


bool IsUserInstalled(const AddonPtr& addon)
{
  if (IsSystemAddon(addon))
    return false;

  auto res = std::find_if(dependencyTypes.begin(), dependencyTypes.end(),
    [&addon](TYPE type){ return addon->IsType(type); });
  return res == dependencyTypes.end();
}


bool IsOrphaned(const AddonPtr& addon)
{
  if (IsSystemAddon(addon) || IsUserInstalled(addon))
    return false;

  // Check if it's required by an installed addon
  VECADDONS allAddons;
  CAddonMgr::Get().GetAllAddons(allAddons, true);
  CAddonMgr::Get().GetAllAddons(allAddons, false);
  for (const AddonPtr& other : allAddons)
  {
    const auto& deps = other->GetDeps();
    if (deps.find(addon->ID()) != deps.end())
      return false;
  }
  return true;
}


void SetUpdateAvailProperties(CFileItemList &items)
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

void UserInstalledAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS addons;
  CAddonMgr::Get().GetAllAddons(addons, true);
  CAddonMgr::Get().GetAllAddons(addons, false);
  addons.erase(std::remove_if(addons.begin(), addons.end(),
                              std::not1(std::ptr_fun(IsUserInstalled))), addons.end());
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24998));
  SetUpdateAvailProperties(items);
}

void SystemAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS addons;
  CAddonMgr::Get().GetAllAddons(addons, true);
  CAddonMgr::Get().GetAllAddons(addons, false);
  addons.erase(std::remove_if(addons.begin(), addons.end(),
                              std::not1(std::ptr_fun(IsSystemAddon))), addons.end());
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24997));
}

void DependencyAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS addons;
  CAddonMgr::Get().GetAllAddons(addons, true);
  CAddonMgr::Get().GetAllAddons(addons, false);
  addons.erase(std::remove_if(addons.begin(), addons.end(), IsSystemAddon), addons.end());
  addons.erase(std::remove_if(addons.begin(), addons.end(), IsUserInstalled), addons.end());
  addons.erase(std::remove_if(addons.begin(), addons.end(), IsOrphaned), addons.end());
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24996));
  SetUpdateAvailProperties(items);
}

void OrphanedAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS addons;
  CAddonMgr::Get().GetAllAddons(addons, true);
  CAddonMgr::Get().GetAllAddons(addons, false);
  addons.erase(std::remove_if(addons.begin(), addons.end(),
                              std::not1(std::ptr_fun(IsOrphaned))), addons.end());
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24995));
}

void OutdatedAddons(const CURL& path, CFileItemList &items)
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

void RunningAddons(const CURL& path, CFileItemList &items)
{
  VECADDONS addons;
  CAddonMgr::Get().GetAddons(ADDON_SERVICE, addons, true);
  CAddonsDirectory::GenerateAddonListing(path, addons, items, g_localizeStrings.Get(24994));
  //TODO: include web interfaces if web server is running
}

bool Browse(const CURL& path, CFileItemList &items)
{
  const std::string repo = path.GetHostName();
  const std::string category = path.GetFileName();

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

  if (category.empty())
  {
    {
      CFileItemPtr item(new CFileItem(g_localizeStrings.Get(24993)));
      item->SetPath(URIUtils::AddFileToFolder(path.Get(), "group.infoproviders"));
      item->m_bIsFolder = true;
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
  else if (category == "group.infoproviders")
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
    items.SetProperty("addoncategory",TranslateType(type, true));
    addons.erase(std::remove_if(addons.begin(), addons.end(),
        [=](const AddonPtr& addon){ return !addon->IsType(type); })  , addons.end());
    CAddonsDirectory::GenerateAddonListing(path, addons, items, TranslateType(type, true));
  }
  return true;
}


bool Repos(const CURL& path, CFileItemList &items)
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
  else if (endpoint == "system")
  {
    SystemAddons(path, items);
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
  else
  {
    return Browse(path, items);
  }
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

