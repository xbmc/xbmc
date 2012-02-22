/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "AddonsDirectory.h"
#include "addons/AddonDatabase.h"
#include "FactoryDirectory.h"
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

using namespace ADDON;

namespace XFILE
{

CAddonsDirectory::CAddonsDirectory(void)
{
  m_allowPrompting = true;
  m_cacheDirectory = DIR_CACHE_ONCE;
}

CAddonsDirectory::~CAddonsDirectory(void)
{}

bool CAddonsDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString path1(strPath);
  URIUtils::RemoveSlashAtEnd(path1);
  CURL path(path1);
  items.ClearProperties();

  items.SetContent("addons");

  VECADDONS addons;
  // get info from repository
  bool reposAsFolders = true;
  if (path.GetHostName().Equals("enabled"))
  {
    CAddonMgr::Get().GetAllAddons(addons, true);
    items.SetProperty("reponame",g_localizeStrings.Get(24062));
    items.SetLabel(g_localizeStrings.Get(24062));
  }
  else if (path.GetHostName().Equals("disabled"))
  { // grab all disabled addons, including disabled repositories
    reposAsFolders = false;
    CAddonMgr::Get().GetAllAddons(addons, false, true, false);
    items.SetProperty("reponame",g_localizeStrings.Get(24039));
    items.SetLabel(g_localizeStrings.Get(24039));
  }
  else if (path.GetHostName().Equals("outdated"))
  {
    reposAsFolders = false;
    CAddonMgr::Get().GetAllOutdatedAddons(addons);
    items.SetProperty("reponame",g_localizeStrings.Get(24043));
    items.SetLabel(g_localizeStrings.Get(24043));
  }
  else if (path.GetHostName().Equals("repos"))
  {
    CAddonMgr::Get().GetAddons(ADDON_REPOSITORY,addons,true);
    items.SetLabel(g_localizeStrings.Get(24033)); // Get Add-ons
  }
  else if (path.GetHostName().Equals("sources"))
  {
    return GetScriptsAndPlugins(path.GetFileName(), items);
  }
  else if (path.GetHostName().Equals("all"))
  {
    CAddonDatabase database;
    database.Open();
    database.GetAddons(addons);
    items.SetProperty("reponame",g_localizeStrings.Get(24032));
    items.SetLabel(g_localizeStrings.Get(24032));
  }
  else if (path.GetHostName().Equals("search"))
  {
    CStdString search(path.GetFileName());
    if (search.IsEmpty() && !GetKeyboardInput(16017, search))
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

  if (path.GetFileName().IsEmpty())
  {
    if (!path.GetHostName().Equals("repos"))
    {
      for (int i=ADDON_UNKNOWN+1;i<ADDON_VIZ_LIBRARY;++i)
      {
        for (unsigned int j=0;j<addons.size();++j)
        {
          if (addons[j]->IsType((TYPE)i))
          {
            CFileItemPtr item(new CFileItem(TranslateType((TYPE)i,true)));
            item->SetPath(URIUtils::AddFileToFolder(strPath,TranslateType((TYPE)i,false)));
            item->m_bIsFolder = true;
            CStdString thumb = GetIcon((TYPE)i);
            if (!thumb.IsEmpty() && g_TextureManager.HasTexture(thumb))
              item->SetThumbnailImage(thumb);
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
    items.SetProperty("addoncategory",TranslateType(type, true));
    items.SetLabel(TranslateType(type, true));
    items.SetPath(strPath);

    // FIXME: Categorisation of addons needs adding here
    for (unsigned int j=0;j<addons.size();++j)
    {
      if (!addons[j]->IsType(type))
        addons.erase(addons.begin()+j--);
    }
  }

  items.SetPath(strPath);
  GenerateListing(path, addons, items, reposAsFolders);
  // check for available updates
  if (path.GetHostName().Equals("enabled"))
  {
    CAddonDatabase database;
    database.Open();
    for (int i=0;i<items.Size();++i)
    {
      AddonPtr addon2;
      database.GetAddon(items[i]->GetProperty("Addon.ID").asString(),addon2);
      if (addon2 && addon2->Version() > AddonVersion(items[i]->GetProperty("Addon.Version").asString())
                 && !database.IsAddonBlacklisted(addon2->ID(),addon2->Version().c_str()))
      {
        items[i]->SetProperty("Addon.Status",g_localizeStrings.Get(24068));
        items[i]->SetProperty("Addon.UpdateAvail", true);
      }
    }
  }
  if (path.GetHostName().Equals("repos") && items.Size() > 1)
  {
    CFileItemPtr item(new CFileItem("addons://all/",true));
    item->SetLabel(g_localizeStrings.Get(24032));
    items.Add(item);
  }

  return true;
}

void CAddonsDirectory::GenerateListing(CURL &path, VECADDONS& addons, CFileItemList &items, bool reposAsFolders)
{
  CStdString xbmcPath = _P("special://xbmc/addons");
  items.ClearItems();
  for (unsigned i=0; i < addons.size(); i++)
  {
    AddonPtr addon = addons[i];
    CFileItemPtr pItem;
    if (reposAsFolders && addon->Type() == ADDON_REPOSITORY)
      pItem = FileItemFromAddon(addon, "addons://", true);
    else
      pItem = FileItemFromAddon(addon, path.Get(), false);
    AddonPtr addon2;
    if (CAddonMgr::Get().GetAddon(addon->ID(),addon2))
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(305));
    else if ((addon->Type() == ADDON_PVRDLL) && (CStdString(pItem->GetProperty("Addon.Path").asString()).Left(xbmcPath.size()).Equals(xbmcPath)))
      pItem->SetProperty("Addon.Status",g_localizeStrings.Get(24023));

    if (!addon->Props().broken.IsEmpty())
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

CFileItemPtr CAddonsDirectory::FileItemFromAddon(AddonPtr &addon, const CStdString &basePath, bool folder)
{
  if (!addon)
    return CFileItemPtr();

  // TODO: This can probably be done more efficiently
  CURL url(basePath);
  url.SetFileName(addon->ID());
  CStdString path(url.Get());
  if (folder)
    URIUtils::AddSlashAtEnd(path);

  CFileItemPtr item(new CFileItem(path, folder));

  CStdString strLabel(addon->Name());
  if (url.GetHostName().Equals("search"))
    strLabel.Format("%s - %s", TranslateType(addon->Type(), true), addon->Name());

  item->SetLabel(strLabel);

  if (!(basePath.Equals("addons://") && addon->Type() == ADDON_REPOSITORY))
    item->SetLabel2(addon->Version().c_str());
  item->SetThumbnailImage(addon->Icon());
  item->SetLabelPreformated(true);
  item->SetIconImage("DefaultAddon.png");
  if (!addon->FanArt().IsEmpty() && 
      (URIUtils::IsInternetStream(addon->FanArt()) || 
       CFile::Exists(addon->FanArt())))
  {
    item->SetProperty("fanart_image", addon->FanArt());
  }
  CAddonDatabase::SetPropertiesFromAddon(addon, item);
  return item;
}

bool CAddonsDirectory::GetScriptsAndPlugins(const CStdString &content, VECADDONS &addons)
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

bool CAddonsDirectory::GetScriptsAndPlugins(const CStdString &content, CFileItemList &items)
{
  items.Clear();

  VECADDONS addons;
  if (!GetScriptsAndPlugins(content, addons))
    return false;

  for (unsigned i=0; i<addons.size(); i++)
  {
    if (addons[i]->Type() == ADDON_PLUGIN)
      items.Add(FileItemFromAddon(addons[i], "plugin://", true));
    else
      items.Add(FileItemFromAddon(addons[i], "script://", false));
  }

  items.Add(GetMoreItem(content));

  items.SetContent("addons");
  items.SetLabel(g_localizeStrings.Get(24001)); // Add-ons

  return items.Size() > 0;
}

CFileItemPtr CAddonsDirectory::GetMoreItem(const CStdString &content)
{
  CFileItemPtr item(new CFileItem("addons://more/"+content,false));
  item->SetLabelPreformated(true);
  item->SetLabel(g_localizeStrings.Get(21452));
  item->SetIconImage("DefaultAddon.png");
  item->SetSpecialSort(SORT_ON_BOTTOM);
  return item;
}
  
}

