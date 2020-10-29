/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_NETWORK_H
#define C_API_NETWORK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  /*
   * For interface between add-on and kodi.
   *
   * This structure defines the addresses of functions stored inside Kodi which
   * are then available for the add-on to call
   *
   * All function pointers there are used by the C++ interface functions below.
   * You find the set of them on xbmc/addons/interfaces/General.cpp
   *
   * Note: For add-on development itself this is not needed
   */
  typedef struct AddonToKodiFuncTable_kodi_network
  {
    bool (*wake_on_lan)(void* kodiBase, const char* mac);
    char* (*get_ip_address)(void* kodiBase);
    char* (*dns_lookup)(void* kodiBase, const char* url, bool* ret);
    char* (*url_encode)(void* kodiBase, const char* url);
    char* (*get_hostname)(void* kodiBase);
    bool (*is_local_host)(void* kodiBase, const char* hostname);
    bool (*is_host_on_lan)(void* kodiBase, const char* hostname, bool offLineCheck);
    char* (*get_user_agent)(void* kodiBase);
  } AddonToKodiFuncTable_kodi_network;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* C_API_NETWORK_H */
