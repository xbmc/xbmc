#pragma once
/*
 *      Copyright (C) 2016-2017 Team KODI
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

#include <string.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/*
 *------------------------------------------------------------------------------
 * This header is only be used for Kodi itself and internally (not for add-on
 * development) to identify the several types.
 *
 * With this reason is also no doxygen part with "///" here used.
 * -----------------------------------------------------------------------------
 */

/*
 * Versions of all add-on globals and instances are defined below.
 *
 * This is added here and not in related header to prevent not
 * needed includes during compile. Also have it here a better
 * overview.
 */


#define ADDON_GLOBAL_VERSION_MAIN                     "1.0.1"
#define ADDON_GLOBAL_VERSION_MAIN_MIN                 "1.0.1"
#define ADDON_GLOBAL_VERSION_MAIN_XML_ID              "kodi.binary.global.main"
#define ADDON_GLOBAL_VERSION_MAIN_DEPENDS             "AddonBase.h" \
                                                      "xbmc_addon_dll.h" \
                                                      "xbmc_addon_types.h" \
                                                      "kodi_audioengine_types.h" \
                                                      "libKODI_audioengine.h" \
                                                      "libXBMC_addon.h" \
                                                      "addon-instance/Screensaver.h"
// @note "addon-instance/Screensaver.h" above must be improved to check only to included directory with "addon-instance"!
#define ADDON_GLOBAL_VERSION_GUI                      "5.11.0"
#define ADDON_GLOBAL_VERSION_GUI_MIN                  "5.10.0"
#define ADDON_GLOBAL_VERSION_GUI_XML_ID               "kodi.binary.global.gui"
#define ADDON_GLOBAL_VERSION_GUI_DEPENDS              "libKODI_guilib.h"

#define ADDON_INSTANCE_VERSION_ADSP                   "0.1.10"
#define ADDON_INSTANCE_VERSION_ADSP_MIN               "0.1.10"
#define ADDON_INSTANCE_VERSION_ADSP_XML_ID            "kodi.binary.instance.adsp"
#define ADDON_INSTANCE_VERSION_ADSP_DEPENDS           "kodi_adsp_dll.h" \
                                                      "kodi_adsp_types.h" \
                                                      "libKODI_adsp.h"

#define ADDON_INSTANCE_VERSION_AUDIODECODER           "1.0.1"
#define ADDON_INSTANCE_VERSION_AUDIODECODER_MIN       "1.0.1"
#define ADDON_INSTANCE_VERSION_AUDIODECODER_XML_ID    "kodi.binary.instance.audiodecoder"
#define ADDON_INSTANCE_VERSION_AUDIODECODER_DEPENDS   "kodi_audiodec_dll.h" \
                                                      "kodi_audiodec_types.h"

#define ADDON_INSTANCE_VERSION_AUDIOENCODER           "1.0.1"
#define ADDON_INSTANCE_VERSION_AUDIOENCODER_MIN       "1.0.1"
#define ADDON_INSTANCE_VERSION_AUDIOENCODER_XML_ID    "kodi.binary.instance.audioencoder"
#define ADDON_INSTANCE_VERSION_AUDIOENCODER_DEPENDS   "xbmc_audioenc_dll.h" \
                                                      "xbmc_audioenc_types.h"

#define ADDON_INSTANCE_VERSION_GAME                   "1.0.32"
#define ADDON_INSTANCE_VERSION_GAME_MIN               "1.0.32"
#define ADDON_INSTANCE_VERSION_GAME_XML_ID            "kodi.binary.instance.game"
#define ADDON_INSTANCE_VERSION_GAME_DEPENDS           "kodi_game_dll.h" \
                                                      "kodi_game_types.h" \
                                                      "libKODI_game.h"

#define ADDON_INSTANCE_VERSION_IMAGEDECODER           "1.0.1"
#define ADDON_INSTANCE_VERSION_IMAGEDECODER_MIN       "1.0.1"
#define ADDON_INSTANCE_VERSION_IMAGEDECODER_XML_ID    "kodi.binary.instance.imagedecoder"
#define ADDON_INSTANCE_VERSION_IMAGEDECODER_DEPENDS   "kodi_imagedec_dll.h" \
                                                      "kodi_imagedec_types.h"

#define ADDON_INSTANCE_VERSION_INPUTSTREAM            "1.0.8"
#define ADDON_INSTANCE_VERSION_INPUTSTREAM_MIN        "1.0.8"
#define ADDON_INSTANCE_VERSION_INPUTSTREAM_XML_ID     "kodi.binary.instance.inputstream"
#define ADDON_INSTANCE_VERSION_INPUTSTREAM_DEPENDS    "libKODI_inputstream.h" \
                                                      "kodi_inputstream_dll.h" \
                                                      "kodi_inputstream_types.h"

#define ADDON_INSTANCE_VERSION_PERIPHERAL             "1.3.3"
#define ADDON_INSTANCE_VERSION_PERIPHERAL_MIN         "1.3.3"
#define ADDON_INSTANCE_VERSION_PERIPHERAL_XML_ID      "kodi.binary.instance.peripheral"
#define ADDON_INSTANCE_VERSION_PERIPHERAL_DEPENDS     "kodi_peripheral_dll.h" \
                                                      "kodi_peripheral_types.h" \
                                                      "kodi_peripheral_utils.hpp" \
                                                      "libKODI_peripheral.h"

#define ADDON_INSTANCE_VERSION_PVR                    "5.2.4"
#define ADDON_INSTANCE_VERSION_PVR_MIN                "5.2.4"
#define ADDON_INSTANCE_VERSION_PVR_XML_ID             "kodi.binary.instance.pvr"
#define ADDON_INSTANCE_VERSION_PVR_DEPENDS            "xbmc_pvr_dll.h" \
                                                      "xbmc_pvr_types.h" \
                                                      "xbmc_epg_types.h" \
                                                      "libXBMC_pvr.h"

#define ADDON_INSTANCE_VERSION_SCREENSAVER            "2.0.0"
#define ADDON_INSTANCE_VERSION_SCREENSAVER_MIN        "2.0.0"
#define ADDON_INSTANCE_VERSION_SCREENSAVER_XML_ID     "kodi.binary.instance.screensaver"
#define ADDON_INSTANCE_VERSION_SCREENSAVER_DEPENDS    "addon-instance/Screensaver.h"

#define ADDON_INSTANCE_VERSION_VFS                    "1.0.1"
#define ADDON_INSTANCE_VERSION_VFS_MIN                "1.0.1"
#define ADDON_INSTANCE_VERSION_VFS_XML_ID             "kodi.binary.instance.vfs"
#define ADDON_INSTANCE_VERSION_VFS_DEPENDS            "kodi_vfs_dll.h" \
                                                      "kodi_vfs_types.h" \
                                                      "kodi_vfs_utils.hpp"

#define ADDON_INSTANCE_VERSION_VISUALIZATION          "1.0.1"
#define ADDON_INSTANCE_VERSION_VISUALIZATION_MIN      "1.0.1"
#define ADDON_INSTANCE_VERSION_VISUALIZATION_XML_ID   "kodi.binary.instance.visualization"
#define ADDON_INSTANCE_VERSION_VISUALIZATION_DEPENDS  "xbmc_vis_dll.h" \
                                                      "xbmc_vis_types.h"

///
/// The currently used types for Kodi add-ons
///
/// @note For add of new types take a new number on end. To change
/// existing numbers can be make problems on already compiled add-ons.
///
typedef enum ADDON_TYPE
{
  /* addon global parts */
  ADDON_GLOBAL_MAIN = 0,
  ADDON_GLOBAL_GUI = 1,
  ADDON_GLOBAL_MAX = 1, // Last used global id, used in loops to check versions. Need to change if new global type becomes added.

  /* addon type instances */
  ADDON_INSTANCE_ADSP = 101,
  ADDON_INSTANCE_AUDIODECODER = 102,
  ADDON_INSTANCE_AUDIOENCODER = 103,
  ADDON_INSTANCE_GAME = 104,
  ADDON_INSTANCE_INPUTSTREAM = 105,
  ADDON_INSTANCE_PERIPHERAL = 106,
  ADDON_INSTANCE_PVR = 107,
  ADDON_INSTANCE_SCREENSAVER = 108,
  ADDON_INSTANCE_VISUALIZATION = 109,
  ADDON_INSTANCE_VFS = 110,
  ADDON_INSTANCE_IMAGEDECODER = 111,
} ADDON_TYPE;

#ifdef __cplusplus
extern "C" {
namespace kodi {
namespace addon {
#endif

///
/// This is used from Kodi to get the active version of add-on parts.
/// It is compiled in add-on and also in Kodi itself, with this can be Kodi
/// compare the version from him with them on add-on.
///
/// @param[in] type The with 'enum ADDON_TYPE' type to ask
/// @return version The current version of asked type
///
inline const char* GetTypeVersion(int type)
{
  /*
   * #ifdef's below becomes set by cmake, no set by hand needed.
   */
  switch (type)
  {
    /* addon global parts */
    case ADDON_GLOBAL_MAIN:
      return ADDON_GLOBAL_VERSION_MAIN;
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_GLOBAL_VERSION_GUI_USED)
    case ADDON_GLOBAL_GUI:
      return ADDON_GLOBAL_VERSION_GUI;
#endif

    /* addon type instances */
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_ADSP_USED)
    case ADDON_INSTANCE_ADSP:
      return ADDON_INSTANCE_VERSION_ADSP;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_AUDIODECODER_USED)
    case ADDON_INSTANCE_AUDIODECODER:
      return ADDON_INSTANCE_VERSION_AUDIODECODER;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_AUDIOENCODER_USED)
    case ADDON_INSTANCE_AUDIOENCODER:
      return ADDON_INSTANCE_VERSION_AUDIOENCODER;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_GAME_USED)
    case ADDON_INSTANCE_GAME:
      return ADDON_INSTANCE_VERSION_GAME;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_IMAGEDECODER_USED)
    case ADDON_INSTANCE_IMAGEDECODER:
      return ADDON_INSTANCE_VERSION_IMAGEDECODER;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_INPUTSTREAM_USED)
    case ADDON_INSTANCE_INPUTSTREAM:
      return ADDON_INSTANCE_VERSION_INPUTSTREAM;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_PERIPHERAL_USED)
    case ADDON_INSTANCE_PERIPHERAL:
      return ADDON_INSTANCE_VERSION_PERIPHERAL;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_PVR_USED)
    case ADDON_INSTANCE_PVR:
      return ADDON_INSTANCE_VERSION_PVR;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_SCREENSAVER_USED)
    case ADDON_INSTANCE_SCREENSAVER:
      return ADDON_INSTANCE_VERSION_SCREENSAVER;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_VFS_USED)
    case ADDON_INSTANCE_VFS:
      return ADDON_INSTANCE_VERSION_VFS;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_VISUALIZATION_USED)
    case ADDON_INSTANCE_VISUALIZATION:
      return ADDON_INSTANCE_VERSION_VISUALIZATION;
#endif
  }
  return "0.0.0";
}

///
/// This is used from Kodi to get the minimum supported version of add-on parts.
/// It is compiled in add-on and also in Kodi itself, with this can be Kodi
/// compare the version from him with them on add-on.
///
/// @param[in] type The with 'enum ADDON_TYPE' type to ask
/// @return version The minimum version of asked type
///
inline const char* GetTypeMinVersion(int type)
{
  switch (type)
  {
    /* addon global parts */
    case ADDON_GLOBAL_MAIN:
      return ADDON_GLOBAL_VERSION_MAIN_MIN;
    case ADDON_GLOBAL_GUI:
      return ADDON_GLOBAL_VERSION_GUI_MIN;

    /* addon type instances */
    case ADDON_INSTANCE_ADSP:
      return ADDON_INSTANCE_VERSION_ADSP_MIN;
    case ADDON_INSTANCE_AUDIODECODER:
      return ADDON_INSTANCE_VERSION_AUDIODECODER_MIN;
    case ADDON_INSTANCE_AUDIOENCODER:
      return ADDON_INSTANCE_VERSION_AUDIOENCODER_MIN;
    case ADDON_INSTANCE_GAME:
      return ADDON_INSTANCE_VERSION_GAME_MIN;
    case ADDON_INSTANCE_IMAGEDECODER:
      return ADDON_INSTANCE_VERSION_IMAGEDECODER_MIN;
    case ADDON_INSTANCE_INPUTSTREAM:
      return ADDON_INSTANCE_VERSION_INPUTSTREAM_MIN;
    case ADDON_INSTANCE_PERIPHERAL:
      return ADDON_INSTANCE_VERSION_PERIPHERAL_MIN;
    case ADDON_INSTANCE_PVR:
      return ADDON_INSTANCE_VERSION_PVR_MIN;
    case ADDON_INSTANCE_SCREENSAVER:
      return ADDON_INSTANCE_VERSION_SCREENSAVER_MIN;
    case ADDON_INSTANCE_VFS:
      return ADDON_INSTANCE_VERSION_VFS_MIN;
    case ADDON_INSTANCE_VISUALIZATION:
      return ADDON_INSTANCE_VERSION_VISUALIZATION_MIN;
  }
  return "0.0.0";
}

///
/// Function used internally on add-on and in Kodi itself to get name
/// about given type.
///
/// @param[in] instanceType The with 'enum ADDON_TYPE' type to ask
/// @return Name of the asked instance type
///
inline const char* GetTypeName(int type)
{
  switch (type)
  {
    /* addon global parts */
    case ADDON_GLOBAL_MAIN:
      return "Addon";
    case ADDON_GLOBAL_GUI:
      return "GUI";

    /* addon type instances */
    case ADDON_INSTANCE_ADSP:
      return "ADSP";
    case ADDON_INSTANCE_AUDIODECODER:
      return "AudioDecoder";
    case ADDON_INSTANCE_AUDIOENCODER:
      return "AudioEncoder";
    case ADDON_INSTANCE_GAME:
      return "Game";
    case ADDON_INSTANCE_IMAGEDECODER:
      return "ImageDecoder";
    case ADDON_INSTANCE_INPUTSTREAM:
      return "Inputstream";
    case ADDON_INSTANCE_PERIPHERAL:
      return "Peripheral";
    case ADDON_INSTANCE_PVR:
      return "PVR";
    case ADDON_INSTANCE_SCREENSAVER:
      return "ScreenSaver";
    case ADDON_INSTANCE_VISUALIZATION:
      return "Visualization";
  }
  return "unknown";
}

///
/// Function used internally on add-on and in Kodi itself to get id number
/// about given type name.
///
/// @param[in] instanceType The with name type to ask
/// @return Id number of the asked instance type
///
/// @warning String must be lower case here!
///
inline int GetTypeId(const char* name)
{
  if (name)
  {
    if (strcmp(name, "addon") == 0)
      return ADDON_GLOBAL_MAIN;
    else if (strcmp(name, "gui") == 0)
      return ADDON_GLOBAL_GUI;
    else if (strcmp(name, "adsp") == 0)
      return ADDON_INSTANCE_ADSP;
    else if (strcmp(name, "audiodecoder") == 0)
      return ADDON_INSTANCE_AUDIODECODER;
    else if (strcmp(name, "audioencoder") == 0)
      return ADDON_INSTANCE_AUDIOENCODER;
    else if (strcmp(name, "game") == 0)
      return ADDON_INSTANCE_GAME;
    else if (strcmp(name, "imagedecoder") == 0)
      return ADDON_INSTANCE_IMAGEDECODER;
    else if (strcmp(name, "inputstream") == 0)
      return ADDON_INSTANCE_INPUTSTREAM;
    else if (strcmp(name, "peripheral") == 0)
      return ADDON_INSTANCE_PERIPHERAL;
    else if (strcmp(name, "pvr") == 0)
      return ADDON_INSTANCE_PVR;
    else if (strcmp(name, "screensaver") == 0)
      return ADDON_INSTANCE_SCREENSAVER;
    else if (strcmp(name, "vfs") == 0)
      return ADDON_INSTANCE_VFS;
    else if (strcmp(name, "visualization") == 0)
      return ADDON_INSTANCE_VISUALIZATION;
  }
  return -1;
}

#ifdef __cplusplus
} /* namespace addon */
} /* namespace kodi */
} /* extern "C" */
#endif
