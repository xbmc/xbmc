/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/gui/list_item.h"

extern "C"
{

  struct AddonGlobalInterface;

  namespace ADDON
  {

  /*!
   * @brief Global gui Add-on to Kodi callback functions
   *
   * To hold functions not related to a instance type and usable for
   * every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-dev-kit/include/kodi/gui/ListItem.h"
   */
  struct Interface_GUIListItem
  {
    static void Init(AddonGlobalInterface* addonInterface);
    static void DeInit(AddonGlobalInterface* addonInterface);

    /*!
     * @brief callback functions from add-on to kodi
     *
     * @note To add a new function use the "_" style to directly identify an
     * add-on callback function. Everything with CamelCase is only to be used
     * in Kodi.
     *
     * The parameter `kodiBase` is used to become the pointer for a `CAddonDll`
     * class.
     */
    //@{
    static KODI_GUI_LISTITEM_HANDLE create(KODI_HANDLE kodiBase,
                                           const char* label,
                                           const char* label2,
                                           const char* path);
    static void destroy(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);
    static char* get_label(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);
    static void set_label(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, const char* label);
    static char* get_label2(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);
    static void set_label2(KODI_HANDLE kodiBase,
                           KODI_GUI_LISTITEM_HANDLE handle,
                           const char* label);
    static char* get_art(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, const char* type);
    static void set_art(KODI_HANDLE kodiBase,
                        KODI_GUI_LISTITEM_HANDLE handle,
                        const char* type,
                        const char* image);
    static char* get_path(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);
    static void set_path(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, const char* path);
    static char* get_property(KODI_HANDLE kodiBase,
                              KODI_GUI_LISTITEM_HANDLE handle,
                              const char* key);
    static void set_property(KODI_HANDLE kodiBase,
                             KODI_GUI_LISTITEM_HANDLE handle,
                             const char* key,
                             const char* value);
    static void select(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle, bool select);
    static bool is_selected(KODI_HANDLE kodiBase, KODI_GUI_LISTITEM_HANDLE handle);
    //@}
  };

  } /* namespace ADDON */
} /* extern "C" */
