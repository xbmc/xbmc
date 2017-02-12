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

#include <string.h>

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

#define ADDON_GLOBAL_VERSION_MAIN                     "1.0.0"
#define ADDON_GLOBAL_VERSION_GUI                      "5.11.1"

#define ADDON_INSTANCE_VERSION_ADSP                   "0.1.10"
#define ADDON_INSTANCE_VERSION_AUDIODECODER           "1.0.1"
#define ADDON_INSTANCE_VERSION_AUDIOENCODER           "1.0.2"
#define ADDON_INSTANCE_VERSION_GAME                   "1.0.30"
#define ADDON_INSTANCE_VERSION_IMAGEDECODER           "1.0.0"
#define ADDON_INSTANCE_VERSION_INPUTSTREAM            "1.0.9"
#define ADDON_INSTANCE_VERSION_PERIPHERAL             "1.3.0"
#define ADDON_INSTANCE_VERSION_PVR                    "5.2.3"
#define ADDON_INSTANCE_VERSION_SCREENSAVER            "1.0.2"
#define ADDON_INSTANCE_VERSION_VISUALIZATION          "1.0.2"

/*
 * The currently used types for Kodi add-ons
 *
 * @note For add of new types take a new number on end. To change
 * existing numbers can be make problems on already compiled add-ons.
 */
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

/*
 * This is used from Kodi to get the active version of add-on parts.
 * It is compiled in add-on and also in Kodi itself, with this can be Kodi
 * compare the version from him with them on add-on.
 *
 * @param[in] type The with 'enum ADDON_TYPE' type to ask
 * @return version The current version of asked type
 */
inline const char* GetTypeVersion(int type)
{
  switch (type)
  {
    /* addon global parts */
    case ADDON_GLOBAL_MAIN:
      return ADDON_GLOBAL_VERSION_MAIN;
    case ADDON_GLOBAL_GUI:
      return ADDON_GLOBAL_VERSION_GUI;

    /* addon type instances */
    case ADDON_INSTANCE_ADSP:
      return ADDON_INSTANCE_VERSION_ADSP;
    case ADDON_INSTANCE_AUDIODECODER:
      return ADDON_INSTANCE_VERSION_AUDIODECODER;
    case ADDON_INSTANCE_AUDIOENCODER:
      return ADDON_INSTANCE_VERSION_AUDIOENCODER;
    case ADDON_INSTANCE_GAME:
      return ADDON_INSTANCE_VERSION_GAME;
    case ADDON_INSTANCE_IMAGEDECODER:
      return ADDON_INSTANCE_VERSION_IMAGEDECODER;
    case ADDON_INSTANCE_INPUTSTREAM:
      return ADDON_INSTANCE_VERSION_INPUTSTREAM;
    case ADDON_INSTANCE_PERIPHERAL:
      return ADDON_INSTANCE_VERSION_PERIPHERAL;
    case ADDON_INSTANCE_PVR:
      return ADDON_INSTANCE_VERSION_PVR;
    case ADDON_INSTANCE_SCREENSAVER:
      return ADDON_INSTANCE_VERSION_SCREENSAVER;
    case ADDON_INSTANCE_VISUALIZATION:
      return ADDON_INSTANCE_VERSION_VISUALIZATION;
  }
  return "0.0.0";
}

/*
 * Function used internally on add-on and in Kodi itself to get name
 * about given type.
 *
 * @param[in] instanceType The with 'enum ADDON_TYPE' type to ask
 * @return Name of the asked instance type
 */
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
      return "ImageDecocer";
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

/*
 * Function used internally on add-on and in Kodi itself to get id number
 * about given type name.
 *
 * @param[in] instanceType The with name type to ask
 * @return Id number of the asked instance type
 *
 * @warning String must be lower case here!
 */
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
