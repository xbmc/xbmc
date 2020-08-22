/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/AddonBase.h"

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
                            KODI_HANDLE firstKodiInstance);
  static void DeInitInterface(AddonGlobalInterface& addonInterface);
  static void RegisterInterface(ADDON_GET_INTERFACE_FN fn);
  static bool UpdateSettingInActiveDialog(CAddonDll* addon,
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
  static char* get_type_version(void* kodiBase, int type);
  static char* get_addon_path(void* kodiBase);
  static char* get_base_user_path(void* kodiBase);
  static void addon_log_msg(void* kodiBase, const int addonLogLevel, const char* strMessage);
  static bool is_setting_using_default(void* kodiBase, const char* id);
  static bool get_setting_bool(void* kodiBase, const char* id, bool* value);
  static bool get_setting_int(void* kodiBase, const char* id, int* value);
  static bool get_setting_float(void* kodiBase, const char* id, float* value);
  static bool get_setting_string(void* kodiBase, const char* id, char** value);
  static bool set_setting_bool(void* kodiBase, const char* id, bool value);
  static bool set_setting_int(void* kodiBase, const char* id, int value);
  static bool set_setting_float(void* kodiBase, const char* id, float value);
  static bool set_setting_string(void* kodiBase, const char* id, const char* value);
  static void free_string(void* kodiBase, char* str);
  static void free_string_array(void* kodiBase, char** arr, int numElements);
  static void* get_interface(void* kodiBase, const char* name, const char* version);
  //@}
};

} /* namespace ADDON */
} /* extern "C" */
