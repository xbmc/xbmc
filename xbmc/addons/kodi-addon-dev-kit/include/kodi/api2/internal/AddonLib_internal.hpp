#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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

//                 _______    _______                              _______    _
//  ||        ||  ||     ||  ||     ||  ||     ||  ||  ||     ||  ||     ||  | |
//  ||        ||  ||     ||  ||     ||  ||\    ||  ||  ||\    ||  ||     ||  | |
//  ||   /\   ||  ||     ||  ||     ||  ||\\   ||  ||  ||\\   ||  ||         | |
//  ||  //\\  ||  ||=====||  ||=====    || \\  ||  ||  || \\  ||  ||  ____   | |
//  || //  \\ ||  ||     ||  ||  \\     ||  \\ ||  ||  ||  \\ ||  ||     ||  |_|
//  ||//    \\||  ||     ||  ||   \\    ||   \\||  ||  ||   \\||  ||     ||   _
//  ||/      \||  ||     ||  ||    \\   ||    \||  ||  ||    \||  ||_____||  |_|
//
//  ____________________________________________________________________________
// |____________________________________________________________________________|
//
// Note this in order to keep compatibility between the API levels!
//
// - Do not use enum's as values, use integer instead and define values as
//   "typedef enum".
// - Do not use CPP style to pass structures, define always as pointer with "*"!
// - Prevent use of structure definition in structure!
// - Do not include dev-kit headers on Kodi's side where function becomes done,
//   predefine them, see e.g. xbmc/addons/binary/interfaces/api2/Addon/Addon_File.h.
//   This is needed to prevent conflicts if Level 2 functions are needed in Level3!
// - Do not use "#defines" for interface values, use typedef!
// - Do not use for new parts headers from Kodi, create a translation system to
//   change add.on side to Kodi's!
//
//  ____________________________________________________________________________
// |____________________________________________________________________________|
//

#include "../definitions.hpp"

#ifdef BUILD_KODI_ADDON
  #include "kodi/AEChannelData.h"
#else
  #include "cores/AudioEngine/Utils/AEChannelData.h"
#endif

#include "../../kodi_audioengine_types.h"
#include "../../xbmc_epg_types.h"
#include "../../xbmc_pvr_types.h"
#include "../../kodi_adsp_types.h"

#include <map>

#ifdef _WIN32                   // windows
#ifndef _SSIZE_T_DEFINED
typedef intptr_t      ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED
#endif

/*
 * This file includes not for add-on developer used parts, but required for the
 * interface to Kodi over the library.
 */

extern "C"
{

API_NAMESPACE

namespace KodiAPI
{

  typedef struct KODI_API_ErrorTranslator
  {
    uint32_t errorCode;
    const char* errorName;
  } KODI_API_ErrorTranslator;

  extern const KODI_API_ErrorTranslator errorTranslator[];

  //============================================================================
  typedef void _addon_log_msg(void* hdl, const int loglevel, const char *msg);
  typedef void _free_string(void* hdl, char* str);

  struct CB_AddOnLib
  {
    _addon_log_msg* addon_log_msg;
    _free_string* free_string;
  };

} /* namespace KodiAPI */
} /* namespace V2 */

END_NAMESPACE()
