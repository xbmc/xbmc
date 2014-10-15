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


#include "AddonsDirectory.h"
#include "addons/AddonDatabase.h"
#include "DirectoryFactory.h"
#include "Directory.h"
#include "DirectoryCache.h"
#include "FileItem.h"
#include "addons/Repository.h"
#include "addons/AddonInstaller.h"
#include "addons/PluginSource.h"
#include "addons/ContextItemAddon.h"
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

bool CAddonsDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  const std::string strPath(url.Get());
  std::string path1(strPath);
  URIUtils::RemoveSlashAtEnd(path1);
  CURL path(path1);
  items.ClearProperties();

  items.SetContent("addons");

  VECADDONS addons;
  // get info from repository
  bool groupAddons = true;
  bool reposAsFolders = true;
  if (path.GetHostName() == "enabled")
  { // grab all enabled addons, including enabled repositories
    reposAsFolders = false;
    CAddonMgr::Get().GetAllAddons(addons, true, true);
    items.SetProperty("reponame",g_localizeStrings.Get(24062));
    items.SetLabel(g_localizeStrings.Get(24062));
  }
  else if (path.GetHostName() == "disabled")
  { // grab all disabled addons, including disabled repositories
    reposAsFolders = false;
    groupAddons = false;
    CAddonMgr::Get().GetAllAddons(addons, false, true);
    items.SetProperty("reponame",g_localizeStrings.Get(24039));
    items.SetLabel(g_localizeStrings.Get(24039));
  }
  else if (path.GetHostName() == "outdated")
  {
    reposAsFolders = false;
    groupAddons = false;
    // ensure our repos are up to date
    CAddonInstaller::Get().UpdateRepos(false, true);
    CAddonMgr::Get().GetAllOutdatedAddons(addons);
    items.SetProperty("reponame",g_localizeStrings.Get(24043));
    items.SetLabel(g_localizeStrings.Get(24043));
  }
  else if (path.GetHostName() == "repos")
  {
    groupAddons = false;
    CAddonMgr::Get().GetAddons(ADDON_REPOSITORY,addons,true);
    items.SetLabel(g_localizeStrings.Get(24033)); // Get Add-ons
  }
  else if (path.GetHostName() == "sources")
  {
    return GetScriptsAndPlugins(path.GetFileName(), items);
  }
  else if (path.GetHostName() == "all")
  {
    CAddonDatabase database;
    database.Open();
    database.GetAddons(addons);
    items.SetProperty("reponame",g_localizeStrings.Get(24032));
    items.SetLabel(g_localizeStrings.Get(24032));
  }
  else if (path.GetHostName() == "search")
  {
    std::string search(path.GetFileName());
    if (search.empty() && !GetKeyboardInput(16017, search))
      return false;

    items.SetProperty("reponame",g_localizeStrings.Get(283));
    items.SetLabel(g_localizeStrings.Get(283));

    CAddonDatabase database;
    database.Open();
    database.Search(search, addons);
    GenerateListing(path, addons, items, true);

    path.SetFileName(search);
    items.SetPath(path.Get());
    return true;
  }
  else
  {
    reposAsFolders = false;
    AddonPtr addon;
    CAddonMgr::Get().GetAddon(path.GetHostName(),addon);
    if (!addon)
      return false;

    // ensure our repos are up to date
    CAddonInstaller::Get().UpdateRepos(false, true);
    CAddonDatabase database;
    database.Open();
    database.GetRepository(addon->ID(),addons);
    items.SetProperty("reponame",addon->Name());
    items.SetLabel(addon->Name());
  }

  bool addManageCategory = false;

  if (path.GetFileName().empty())
  {
    if (groupAddons)
    {
      //Context addons are two different types (items and categories). This stores if we have any of them,
      //so we don't add two context addon types.
      bool hasContextAddons = false;

      for (int i=ADDON_UNKNOWN+1;i<ADDON_MAX;++i)
      {
        for (unsigned int j=0;j<addons.size();++j)
        {
          if (addons[j]->IsType((TYPE)i))
          {
            TYPE type = (TYPE)i;
            if (i == ADDON_CONTEXT_ITEM || i == ADDON_CONTEXT_CATEGORY)
            {
              if (hasContextAddons)
                break; //already have context items!
              hasContextAddons = true;
              type = ADDON_CONTEXT_ITEM; //Make sure we always add the context item path and never the context category one!
            }
            CFileItemPtr item(new CFileItem(TranslateType(type, true)));
            item->SetPath(URIUtils::AddFileToFolder(strPath,TranslateType(type,false)));
            item->m_bIsFolder = true;
            std::string thumb = GetIcon(type);
            if (!thumb.empty() && g_TextureManager.HasTexture(thumb))
              item->SetArt("thumb", thumb);
            items.Add(item);
            break;
          }
        }
      }
      items.SetPath(strPath);
      return true;
    }
  }
  else
  {
    TYPE type = TranslateType(path.GetFileName());
    if (type == ADDON_UNKNOWN && StringUtils::StartsWith(path.GetFileName(), "addon.context.item"))
      type = ADDON_CONTEXT_ITEM;
    items.SetProperty("addoncategory",TranslateType(type, true));
    items.SetLabel(TranslateType(type, true));
    items.SetPath(strPath);

    // FIXME: Categorisation of addons needs adding here
    for (unsigned int j=0;j<addons.size();++j)
    {
      if (!addons[j]->IsLogicalType(type))
        addons.erase(addons.begin()+j--);
    }

    //TODO: empty categories show up... not sure what's the best way to hide them

    if (type == ADDON_CONTEXT_ITEM && path.GetFileName() == "addon.context.item")
    { //We want the root context menu entries. (empty parent)
      //Since we can deal with unmet dependencies, we also want items whose parent is not available
      for (unsigned int j = 0; j < addons.size(); ++j)
      {
        const ContextAddonPtr contextAddon = boost::static_pointer_cast<IContextItem>(addons[j]);
        if (!contextAddon->GetParent().empty())
        { //we are in the context item root directory, so do not show items with a parent
          //(unless that parent is not available)

          //special case for the core manage context category (it would fail to get a addon for that!)
          if (contextAddon->GetParent() == MANAGE_CATEGORY_NAME)
          {
            addManageCategory = true;
            addons.erase(addons.begin() + j--);
            continue;
          }

          AddonPtr addon;
          //TODO: this should get the addon, even if it's not installed?!?
          CAddonMgr::Get().GetAddon(contextAddon->GetParent(), addon, ADDON_CONTEXT_CATEGORY, false);
          if (addon)
            addons.erase(addons.begin() + j--);
        }
      }
    }
    else if (type == ADDON_CONTEXT_ITEM)
    {
      std::string parent = URIUtils::GetFileName(path);
      for (unsigned int j = 0; j < addons.size(); ++j)
      {
        const ContextAddonPtr contextAddon = boost::static_pointer_cast<IContextItem>(addons[j]);
        if (contextAddon->GetParent() != parent)
        { //wrong parent
          addons.erase(addons.begin() + j--);
        }
      }
    }
  }

  items.SetPath(strPath);
  GenerateListing(path, addons, items, reposAsFolders);
  // check for available updates
  if (path.GetHostName() == "enabled")
  {
    CAddonDatabase database;
    database.Open();
    for (int i=0;i<items.Size();++i)
    {
      AddonPtr addon2;
      database.GetAddon(items[i]->GetProperty("Addon.ID").asString(),addon2);
      if (addon2 && addon2->Version() > AddonVersion(items[i]->GetProperty("Addon.Version").asString())
                 && !database.IsAddonBlacklisted(addon2->ID(),addon2->Version().asString()))
      {
        items[i]->SetProperty("Addon.Status",g_localizeStrings.Get(24068));
        items[i]->SetProperty("Addon.UpdateAvail", true);
      }
    }
  }
  if (path.GetHostName() == "repos" && items.Size() > 1)
  {
    CFileItemPtr item(new CFileItem("addons://all/",true));
    item->SetLabel(g_localizeStrings.Get(24032));
    items.Add(item);
  }
  else if (path.GetHostName() == "outdated" && items.Size() > 1)
  {
    CFileItemPtr item(new CFileItem("addons://update_all/", true));
    item->SetLabel(g_localizeStrings.Get(24122));
    item->SetSpecialSort(SortSpecialOnTop);
    items.Add(item);
  }
  if (addManageCategory)
  {
    CFileItemPtr pItem(new CFileItem());
    path.SetFileName(URIUtils::AddFileToFolder(path.GetFileName(), MANAGE_CATEGORY_NAME));
    pItem->SetLabel(g_localizeStrings.Get(16106));
    pItem->SetPath(path.Get());
    pItem->m_bIsFolder = true;
    pItem->SetProperty("Addon.Type", TranslateType(ADDON_CONTEXT_CATEGORY, true));
    pItem->SetProperty("Addon.intType", TranslateType(ADDON_CONTEXT_CATEGORY));
    pItem->SetProperty("Addon.Name", g_localizeStrings.Get(16106));
    pItem->SetProperty("Addon.Description", g_localizeStrings.Get(16108));
    pItem->SetProperty("Addon.Creator", "Team XBMC");
    items.Add(pItem);
  }

  return true;
}

void CAddonsDirectory::GenerateListing(CURL &path, VECADDONS& addons, CFileItemList &items, bool reposAsFolders)
{
  items.ClearItems();
  for (unsigned i=0; i < addons.size(); i++)
  {
    AddonPtr addon = addons[i];
    CFileItemPtr pItem;
    if (reposAsFolders && addon->Type() == ADDON_REPOSITORY)
      pItem = FileItemFromAddon(addon, "addons://", true);
    else
      pItem = FileItemFromAddon(addon, path.Get(), addon->IsType(ADDON_CONTEXT_CATEGORY));
    AddonPtr addon2;
    if (CAddonMgr::Get().GetAddon(addon->ID(),addon2))
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(305));
    else if (CAddonMgr::Get().IsAddonDisabled(addon->ID()))
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(24023));

    if (addon->Props().broken == "DEPSNOTMET")
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(24049));
    else if (!addon->Props().broken.empty())
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(24098));
    if (addon2 && addon2->Version() < addon->Version())
    {
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(24068));
      pItem->SetProperty("Addon.UpdateAvail", true);
    }
    CAddonDatabase::SetPropertiesFromAddon(addon,pItem);
    items.Add(pItem);
  }
}

CFileItemPtr CAddonsDirectory::FileItemFromAddon(const AddonPtr &addon, const std::string &basePath, bool folder)
{
  if (!addon)
    return CFileItemPtr();

  // TODO: This can probably be done more efficiently
  CURL url(basePath);
  if (addon->IsType(ADDON_CONTEXT_CATEGORY)) //make sure the path to context item addons respects the hierarchy
    url.SetFileName(URIUtils::AddFileToFolder(url.GetFileName(), addon->ID()));
  else
    url.SetFileName(addon->ID());
  std::string path(url.Get());
  if (folder)
    URIUtils::AddSlashAtEnd(path);

  CFileItemPtr item(new CFileItem(path, folder));

  std::string strLabel(addon->Name());
  if (url.GetHostName() == "search")
    strLabel = StringUtils::Format("%s - %s", TranslateType(addon->Type(), true).c_str(), addon->Name().c_str());

  item->SetLabel(strLabel);

  if (!URIUtils::PathEquals(basePath, "addons://") && addon->Type() == ADDON_REPOSITORY)
    item->SetLabel2(addon->Version().asString());
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
    PluginPtr plugin = boost::dynamic_pointer_cast<CPluginSource>(tempAddons[i]);
    if (plugin && plugin->Provides(type))
      addons.push_back(tempAddons[i]);
  }
  tempAddons.clear();
  CAddonMgr::Get().GetAddons(ADDON_SCRIPT, tempAddons);
  for (unsigned i=0; i<tempAddons.size(); i++)
  {
    PluginPtr plugin = boost::dynamic_pointer_cast<CPluginSource>(tempAddons[i]);
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

  for (unsigned i=0; i<addons.size(); i++)
  {
    CFileItemPtr item(FileItemFromAddon(addons[i], 
                      addons[i]->Type()==ADDON_PLUGIN?"plugin://":"script://",
                      addons[i]->Type() == ADDON_PLUGIN));
    PluginPtr plugin = boost::dynamic_pointer_cast<CPluginSource>(addons[i]);
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

