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
#include "AddonDatabase.h"
#include "FactoryDirectory.h"
#include "Directory.h"
#include "DirectoryCache.h"
#include "FileItem.h"
#include "addons/Repository.h"
#include "StringUtils.h"

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
  CURL path(strPath);
  items.ClearProperties();

  VECADDONS addons;
  // get info from repository
  if (path.GetHostName().Equals("enabled"))
  {
    CAddonMgr::Get()->GetAllAddons(addons);
    items.SetProperty("reponame",g_localizeStrings.Get(24062));
  }
  else
  {
    AddonPtr addon;
    CAddonMgr::Get()->GetAddon(path.GetHostName(),addon);
    if (!addon)
      return false;
    CAddonDatabase database;
    database.Open();
    if (!database.GetRepository(addon->ID(),addons))
    {
      RepositoryPtr repo = boost::dynamic_pointer_cast<CRepository>(addon);
      addons = CRepositoryUpdateJob::GrabAddons(repo,false);
    }
    items.SetProperty("reponame",addon->Name());
  }

  if (path.GetFileName().IsEmpty())
  {
    for (int i=ADDON_UNKNOWN+1;i<ADDON_VIZ_LIBRARY;++i)
    {
      for (unsigned int j=0;j<addons.size();++j)
      {
        if (addons[j]->Type() == (TYPE)i)
        {
          CFileItemPtr item(new CFileItem(TranslateType((TYPE)i,true)));
          item->m_strPath = CUtil::AddFileToFolder(strPath,TranslateType((TYPE)i,false));
          item->m_bIsFolder = true;
          items.Add(item);
          break;
        }
      }
    }
    items.m_strPath = strPath;
    return true;
  }

  items.SetProperty("addoncategory",path.GetFileName());
  TYPE type = TranslateType(path.GetFileName());
  for (unsigned int j=0;j<addons.size();++j)
    if (addons[j]->Type() != type)
      addons.erase(addons.begin()+j--);

  items.m_strPath = strPath;
  GenerateListing(path, addons, items);

  return true;
}

void CAddonsDirectory::GenerateListing(CURL &path, VECADDONS& addons, CFileItemList &items)
{
  items.ClearItems();
  for (unsigned i=0; i < addons.size(); i++)
  {
    AddonPtr addon = addons[i];
    path.SetFileName(addon->ID());
    CFileItemPtr pItem(new CFileItem(path.Get(), false));
    pItem->SetLabel(addon->Name());
    pItem->SetLabel2(addon->Summary());
    pItem->SetThumbnailImage(addon->Icon());
    CAddonDatabase::SetPropertiesFromAddon(addon,pItem);
    items.Add(pItem);
  }
}

}

