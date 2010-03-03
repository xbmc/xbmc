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
#include "FactoryDirectory.h"
#include "Directory.h"
#include "DirectoryCache.h"
#include "FileItem.h"

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
  if (!path.GetProtocol().Equals("addons"))
    return false;

  VECADDONS addons;

  if (path.GetPassWord().empty())
    CAddonMgr::Get()->GetAddons(TranslateType(path.GetHostName()), addons);
  else
    CAddonMgr::Get()->GetAddons(TranslateType(path.GetHostName()), addons, TranslateContent(path.GetPassWord()));

  GenerateListing(path, addons, items);

  return !items.IsEmpty();
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
    pItem->SetProperty("Addon.Type", TranslateType(addon->Type()));
    pItem->SetProperty("Addon.Name", addon->Name());
    pItem->SetProperty("Addon.Summary", addon->Summary());
    pItem->SetProperty("Addon.Description", addon->Description());
    pItem->SetProperty("Addon.Creator", addon->Author());
    pItem->SetProperty("Addon.Disclaimer", addon->Disclaimer());
    pItem->SetProperty("Addon.Rating", addon->Stars());
    pItem->SetThumbnailImage(addon->Icon());
    items.Add(pItem);
  }
  items.FillInDefaultIcons();
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
}

}

