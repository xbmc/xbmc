/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ListItem.h"
#include "General.h"

#include "FileItem.h"
#include "addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/ListItem.h"
#include "utils/log.h"

using namespace kodi; // addon-dev-kit namespace
using namespace kodi::gui; // addon-dev-kit namespace

extern "C"
{
namespace ADDON
{

void Interface_GUIListItem::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->listItem = static_cast<AddonToKodiFuncTable_kodi_gui_listItem*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_listItem)));

  addonInterface->toKodi->kodi_gui->listItem->create = create;
  addonInterface->toKodi->kodi_gui->listItem->destroy = destroy;
}

void Interface_GUIListItem::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->listItem);
}

void* Interface_GUIListItem::create(void* kodiBase, const char* label, const char* label2, const char* iconImage, const char* thumbnailImage, const char* path)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "ADDON::Interface_GUIListItem::%s - invalid data", __FUNCTION__);
    return nullptr;
  }

  // create CFileItem
  CFileItemPtr* item = new CFileItemPtr(new CFileItem());
  //CFileItem* item = new CFileItem();
  if (label)
    item->get()->SetLabel(label);
  if (label2)
    item->get()->SetLabel2(label2);
  if (iconImage)
    item->get()->SetIconImage(iconImage);
  if (thumbnailImage)
    item->get()->SetArt("thumb", thumbnailImage);
  if (path)
    item->get()->SetPath(path);

  return item;
}

void Interface_GUIListItem::destroy(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "ADDON::Interface_GUIListItem::%s - invalid data", __FUNCTION__);
    return;
  }
  
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (item)
    delete item;
}

} /* namespace ADDON */
} /* extern "C" */
