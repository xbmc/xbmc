/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ListItem.h"

#include "FileItem.h"
#include "General.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/ListItem.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIListItem::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->listItem = new AddonToKodiFuncTable_kodi_gui_listItem();

  addonInterface->toKodi->kodi_gui->listItem->create = create;
  addonInterface->toKodi->kodi_gui->listItem->destroy = destroy;
  addonInterface->toKodi->kodi_gui->listItem->get_label = get_label;
  addonInterface->toKodi->kodi_gui->listItem->set_label = set_label;
  addonInterface->toKodi->kodi_gui->listItem->get_label2 = get_label2;
  addonInterface->toKodi->kodi_gui->listItem->set_label2 = set_label2;
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
  delete addonInterface->toKodi->kodi_gui->listItem;
}

KODI_GUI_LISTITEM_HANDLE Interface_GUIListItem::create(KODI_HANDLE kodiBase,
                                                       const char* label,
                                                       const char* label2,
                                                       const char* path)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "ADDON::Interface_GUIListItem::{} - invalid data", __func__);
    return nullptr;
  }

  CFileItemPtr* item = new CFileItemPtr(new CFileItem());
  if (label)
    item->get()->SetLabel(label);
  if (label2)
    item->get()->SetLabel2(label2);
  if (path)
    item->get()->SetPath(path);

  return item;
}

void Interface_GUIListItem::destroy(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "ADDON::Interface_GUIListItem::{} - invalid data", __func__);
    return;
  }

  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (item)
    delete item;
}

char* Interface_GUIListItem::get_label(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return nullptr;
  }

  char* ret;
  Interface_GUIGeneral::lock();
  ret = strdup(item->get()->GetLabel().c_str());
  Interface_GUIGeneral::unlock();
  return ret;
}

void Interface_GUIListItem::set_label(KODI_HANDLE kodiBase,
                                      KODI_GUI_LISTITEM_HANDLE handle,
                                      const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "label='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(label),
              addon ? addon->ID() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->SetLabel(label);
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIListItem::get_label2(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return nullptr;
  }

  char* ret;
  Interface_GUIGeneral::lock();
  ret = strdup(item->get()->GetLabel2().c_str());
  Interface_GUIGeneral::unlock();
  return ret;
}

void Interface_GUIListItem::set_label2(KODI_HANDLE kodiBase,
                                       KODI_GUI_LISTITEM_HANDLE handle,
                                       const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "label='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(label),
              addon ? addon->ID() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->SetLabel2(label);
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIListItem::get_art(KODI_HANDLE kodiBase,
                                     KODI_GUI_LISTITEM_HANDLE handle,
                                     const char* type)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !type)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', type='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(type),
              addon ? addon->ID() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return nullptr;
  }

  char* ret;
  Interface_GUIGeneral::lock();
  ret = strdup(item->get()->GetArt(type).c_str());
  Interface_GUIGeneral::unlock();
  return ret;
}

void Interface_GUIListItem::set_art(KODI_HANDLE kodiBase,
                                    KODI_GUI_LISTITEM_HANDLE handle,
                                    const char* type,
                                    const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !type || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}', type= "
              "'{}', label='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(type),
              static_cast<const void*>(label), addon ? addon->ID() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->SetArt(type, label);
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIListItem::get_path(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return nullptr;
  }

  char* ret;
  Interface_GUIGeneral::lock();
  ret = strdup(item->get()->GetPath().c_str());
  Interface_GUIGeneral::unlock();
  return ret;
}


void Interface_GUIListItem::set_path(KODI_HANDLE kodiBase,
                                     KODI_GUI_LISTITEM_HANDLE handle,
                                     const char* path)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !path)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', path='{}', "
              "handle='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(path),
              addon ? addon->ID() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->SetPath(path);
  Interface_GUIGeneral::unlock();
}

void Interface_GUIListItem::set_property(KODI_HANDLE kodiBase,
                                         KODI_GUI_LISTITEM_HANDLE handle,
                                         const char* key,
                                         const char* value)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !key || !value)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}', value='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              static_cast<const void*>(value), addon ? addon->ID() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  item->get()->SetProperty(lowerKey, CVariant(value));
  Interface_GUIGeneral::unlock();
}

char* Interface_GUIListItem::get_property(KODI_HANDLE kodiBase,
                                          KODI_GUI_LISTITEM_HANDLE handle,
                                          const char* key)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item || !key)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "key='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(key),
              addon ? addon->ID() : "unknown");
    return nullptr;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return nullptr;
  }

  std::string lowerKey = key;
  StringUtils::ToLower(lowerKey);

  Interface_GUIGeneral::lock();
  char* ret = strdup(item->get()->GetProperty(lowerKey).asString().c_str());
  Interface_GUIGeneral::unlock();

  return ret;
}

void Interface_GUIListItem::select(KODI_HANDLE kodiBase,
                                   KODI_GUI_LISTITEM_HANDLE handle,
                                   bool select)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return;
  }

  Interface_GUIGeneral::lock();
  item->get()->Select(select);
  Interface_GUIGeneral::unlock();
}

bool Interface_GUIListItem::is_selected(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CFileItemPtr* item = static_cast<CFileItemPtr*>(handle);
  if (!addon || !item)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIListItem::{} - invalid handler data (kodiBase='{}', handle='{}') on "
              "addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return false;
  }

  if (item->get() == nullptr)
  {
    CLog::Log(LOGERROR, "Interface_GUIListItem::{} - empty list item called on addon '{}'",
              __func__, addon->ID());
    return false;
  }

  Interface_GUIGeneral::lock();
  bool ret = item->get()->IsSelected();
  Interface_GUIGeneral::unlock();

  return ret;
}

} /* namespace ADDON */
