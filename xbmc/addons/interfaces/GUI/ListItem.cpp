/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ListItem.h"
#include "General.h"

#include "FileItem.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/ListItem.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

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
  addonInterface->toKodi->kodi_gui->listItem->get_label = get_label;
  addonInterface->toKodi->kodi_gui->listItem->set_label = set_label;
  addonInterface->toKodi->kodi_gui->listItem->get_label2 = get_label2;
  addonInterface->toKodi->kodi_gui->listItem->set_label2 = set_label2;
  addonInterface->toKodi->kodi_gui->listItem->get_icon_image = get_icon_image;
  addonInterface->toKodi->kodi_gui->listItem->set_icon_image = set_icon_image;
  addonInterface->toKodi->kodi_gui->listItem->get_art = get_art;
  addonInterface->toKodi->kodi_gui->listItem->set_art = set_art;
  addonInterface->toKodi->kodi_gui->listItem->get_path = get_path;
  addonInterface->toKodi->kodi_gui->listItem->set_path = set_path;
  addonInterface->toKodi->kodi_gui->listItem->get_property = get_property;
  addonInterface->toKodi->kodi_gui->listItem->set_property = set_property;
  addonInterface->toKodi->kodi_gui->listItem->select = select;
  addonInterface->toKodi->kodi_gui->listItem->is_selected = is_selected;
}

void Interface_GUIListItem::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->listItem);
}

void* Interface_GUIListItem::create(void* kodiBase, const char* label, const char* label2, const char* icon_image, const char* path)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "ADDON::Interface_GUIListItem::%s - invalid data", __FUNCTION__);
    return nullptr;
  }

  CFileItemPtr* item = new CFileItemPtr(new CFileItem());
  if (label)
    item->get()->SetLabel(label);
  if (label2)
    item->get()->SetLabel2(label2);
  if (icon_image)
    item->get()->SetIconImage(icon_image);
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

char* Interface_GUIListItem::get_label(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p') on "
              "addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return nullptr;
  }

  char* ret;
  Interface_GUIGeneral::lock();
  ret = strdup(item->get()->GetLabel().c_str());
  Interface_GUIGeneral::unlock();
  return ret;
}

void Interface_GUIListItem::set_label(void* kodiBase, void* handle, const char *label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p', "
              "label='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, label, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->SetLabel(label);
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIListItem::get_label2(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p') on "
              "addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return nullptr;
  }

  char* ret;
  Interface_GUIGeneral::lock();
  ret = strdup(item->get()->GetLabel2().c_str());
  Interface_GUIGeneral::unlock();
  return ret;
}

void Interface_GUIListItem::set_label2(void* kodiBase, void* handle, const char *label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p', "
              "label='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, label, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->SetLabel2(label);
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIListItem::get_icon_image(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p') on "
              "addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return nullptr;
  }

  char* ret;
  Interface_GUIGeneral::lock();
  ret = strdup(item->get()->GetIconImage().c_str());
  Interface_GUIGeneral::unlock();
  return ret;
}

void Interface_GUIListItem::set_icon_image(void* kodiBase, void* handle, const char *image)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !image)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p', "
              "image='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, image, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->SetIconImage(image);
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIListItem::get_art(void* kodiBase, void* handle, const char* type)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !type)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', type='%p', "
              "handle='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, type, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return nullptr;
  }

  char* ret;
  Interface_GUIGeneral::lock();
  ret = strdup(item->get()->GetArt(type).c_str());
  Interface_GUIGeneral::unlock();
  return ret;
}

void Interface_GUIListItem::set_art(void* kodiBase, void* handle, const char* type, const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !type | !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p', type= "
              "'%p', label='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, type, label, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->SetArt(type, label);
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIListItem::get_path(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p') on "
              "addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return nullptr;
  }

  char* ret;
  Interface_GUIGeneral::lock();
  ret = strdup(item->get()->GetPath().c_str());
  Interface_GUIGeneral::unlock();
  return ret;
}


void Interface_GUIListItem::set_path(void* kodiBase, void* handle, const char* path)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !path)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', path='%p', "
              "handle='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, path, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->SetPath(path);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIListItem::set_property(void* kodiBase, void* handle, const char* key, const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !key || !value)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p', "
              "key='%p', value='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, key, value, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  item->get()->SetProperty(lowerKey, CVariant(value));
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIListItem::get_property(void* kodiBase, void* handle, const char* key)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p', "
              "key='%p') on addon '%s'",
              __FUNCTION__, kodiBase, handle, key, addon ? addon->ID().c_str() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return nullptr;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  char* ret = strdup(item->get()->GetProperty(lowerKey).asString().c_str());
  Interface_GUIGeneral::unlock();

  return ret;
}

void Interface_GUIListItem::select(void* kodiBase, void* handle, bool select)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p') on "
              "addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->Select(select);
  Interface_GUIGeneral::unlock();
}

bool Interface_GUIListItem::is_selected(void* kodiBase, void* handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::%s - invalid handler data (kodiBase='%p', handle='%p') on "
              "addon '%s'",
              __FUNCTION__, kodiBase, handle, addon ? addon->ID().c_str() : "unknown");
    return false;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::%s - empty list item called on addon '%s'",
              __FUNCTION__, addon->ID().c_str());
    return false;
  }

  Interface_GUIGeneral::lock();
  bool ret = item->get()->IsSelected();
  Interface_GUIGeneral::unlock();

  return ret;
}

} /* namespace ADDON */
} /* extern "C" */
