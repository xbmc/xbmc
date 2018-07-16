/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

struct AddonGlobalInterface;

namespace ADDON
{

  /*!
   * @brief Global general Add-on to Kodi callback functions
   *
   * To hold network functions not related to a instance type and usable for
   * every add-on type.
   *
   * Related add-on header is "./xbmc/addons/kodi-addon-dev-kit/include/kodi/Network.h"
   */
  struct Interface_Network
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
    static bool wake_on_lan(void* kodiBase, const char* mac);
    static char* get_ip_address(void* kodiBase);
    static char* dns_lookup(void* kodiBase, const char* url, bool* ret);
    static char* url_encode(void* kodiBase, const char* url);
    //@}
  };

} /* namespace ADDON */
