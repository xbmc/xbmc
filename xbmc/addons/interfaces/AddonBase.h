/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon_base.h"

extern "C"
{
namespace ADDON
{

typedef void* (*ADDON_GET_INTERFACE_FN)(const std::string& name, const std::string& version);

class CAddonDll;

/*!
 * @brief Global general Add-on to Kodi callback functions
 *
 * To hold general functions not related to a instance type and usable for
 * every add-on type.
 *
 * Related add-on header is "./xbmc/addons/kodi-dev-kit/include/kodi/General.h"
 */
struct Interface_Base
{
  static bool InitInterface(CAddonDll* addon,
                            AddonGlobalInterface& addonInterface,
                            KODI_ADDON_INSTANCE_STRUCT* firstKodiInstance);
  static void DeInitInterface(AddonGlobalInterface& addonInterface);
  static void RegisterInterface(ADDON_GET_INTERFACE_FN fn);
  static bool UpdateSettingInActiveDialog(CAddonDll* addon,
                                          AddonInstanceId instanceId,
                                          const char* id,
                                          const std::string& value);

  static std::vector<ADDON_GET_INTERFACE_FN> s_registeredInterfaces;

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
  static void addon_log_msg(const KODI_ADDON_BACKEND_HDL hdl,
                            const int addonLogLevel,
                            const char* strMessage);
  static char* get_type_version(const KODI_ADDON_BACKEND_HDL hdl, int type);
  static char* get_addon_path(const KODI_ADDON_BACKEND_HDL hdl);
  static char* get_lib_path(const KODI_ADDON_BACKEND_HDL hdl);
  static char* get_user_path(const KODI_ADDON_BACKEND_HDL hdl);
  static char* get_temp_path(const KODI_ADDON_BACKEND_HDL hdl);
  static char* get_localized_string(const KODI_ADDON_BACKEND_HDL hdl, long label_id);
  static char* get_addon_info(const KODI_ADDON_BACKEND_HDL hdl, const char* id);
  static bool open_settings_dialog(const KODI_ADDON_BACKEND_HDL hdl);
  static bool is_setting_using_default(const KODI_ADDON_BACKEND_HDL hdl, const char* id);
  static bool get_setting_bool(const KODI_ADDON_BACKEND_HDL hdl, const char* id, bool* value);
  static bool get_setting_int(const KODI_ADDON_BACKEND_HDL hdl, const char* id, int* value);
  static bool get_setting_float(const KODI_ADDON_BACKEND_HDL hdl, const char* id, float* value);
  static bool get_setting_string(const KODI_ADDON_BACKEND_HDL hdl, const char* id, char** value);
  static bool set_setting_bool(const KODI_ADDON_BACKEND_HDL hdl, const char* id, bool value);
  static bool set_setting_int(const KODI_ADDON_BACKEND_HDL hdl, const char* id, int value);
  static bool set_setting_float(const KODI_ADDON_BACKEND_HDL hdl, const char* id, float value);
  static bool set_setting_string(const KODI_ADDON_BACKEND_HDL hdl,
                                 const char* id,
                                 const char* value);
  static void free_string(const KODI_ADDON_BACKEND_HDL hdl, char* str);
  static void free_string_array(const KODI_ADDON_BACKEND_HDL hdl, char** arr, int numElements);
  static void* get_interface(const KODI_ADDON_BACKEND_HDL hdl,
                             const char* name,
                             const char* version);
  //@}
};

} /* namespace ADDON */
} /* extern "C" */
