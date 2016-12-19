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

/*
 *------------------------------------------------------------------------------
 * This header is only be used for Kodi itself and internally (not for add-on
 * development) to identify the several instance types.
 *
 * With this reason is also no doxygen part with "///" here used.
 * -----------------------------------------------------------------------------
 */

/*
 * Versions of all add-on instances are defined below.
 *
 * This is added here and not in related header to prevent not
 * needed includes during compile. Also have it here a better
 * overview.
 */

#define INSTANCE_VERSION_MAIN                   "1.0.0"
#define INSTANCE_VERSION_GUI                    "5.11.0"
#define INSTANCE_VERSION_ADSP                   "0.2.0"
#define INSTANCE_VERSION_AUDIODECODER           "1.1.0"
#define INSTANCE_VERSION_AUDIOENCODER           "1.1.0"
#define INSTANCE_VERSION_GAME                   "1.1.0"
#define INSTANCE_VERSION_INPUTSTREAM            "1.1.0"
#define INSTANCE_VERSION_PERIPHERAL             "1.3.0"
#define INSTANCE_VERSION_PVR                    "5.3.0"
#define INSTANCE_VERSION_SCREENSAVER            "1.0.0"
#define INSTANCE_VERSION_VISUALIZATION          "1.0.0"

/*
 * The currently used instance types for Kodi add-ons
 *
 * @note For add of new instance type take a new number on end. To change
 * existing numbers can be make problems on already compiled add-ons.
 */
typedef enum ADDON_INSTANCE_TYPE
{
  /* addon global instances */
  ADDON_INSTANCE_MAIN = 0,
  ADDON_INSTANCE_GUI = 1,
  ADDON_INSTANCE_GLOBAL_MAX = 1, // Last used global id, used in loops to check versions. Need to change if new global type becomes added.

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
} ADDON_INSTANCE_TYPE;

#ifdef __cplusplus
extern "C" {
namespace kodi {
namespace addon {
#endif

/*
 * This is used from Kodi to get the active version of add-on instances.
 * It is compiled in add-on and also in Kodi itself, with this can be Kodi
 * compare the version from him with them on add-on.
 *
 * Call for inside Kodi: kodi::addon::GetInstanceVersion(...)
 * Call to ask addon:    m_interface.toAddon.GetInstanceVersion(...)
 *
 * @param[in] instanceType The with 'enum ADDON_INSTANCE_TYPE' type to ask
 * @return version The current instance version of asked type
 *
 * The Kodi side is done in AddonDll.cpp by CAddonDll::CreateInstance(...)
 * call.
 */
inline const char* GetInstanceVersion(int instanceType)
{
  switch (instanceType)
  {
    case ADDON_INSTANCE_MAIN:
      return INSTANCE_VERSION_MAIN;
    case ADDON_INSTANCE_GUI:
      return INSTANCE_VERSION_GUI;
    case ADDON_INSTANCE_ADSP:
      return INSTANCE_VERSION_ADSP;
    case ADDON_INSTANCE_AUDIODECODER:
      return INSTANCE_VERSION_AUDIODECODER;
    case ADDON_INSTANCE_AUDIOENCODER:
      return INSTANCE_VERSION_AUDIOENCODER;
    case ADDON_INSTANCE_GAME:
      return INSTANCE_VERSION_GAME;
    case ADDON_INSTANCE_INPUTSTREAM:
      return INSTANCE_VERSION_INPUTSTREAM;
    case ADDON_INSTANCE_PERIPHERAL:
      return INSTANCE_VERSION_PERIPHERAL;
    case ADDON_INSTANCE_PVR:
      return INSTANCE_VERSION_PVR;
    case ADDON_INSTANCE_SCREENSAVER:
      return INSTANCE_VERSION_SCREENSAVER;
    case ADDON_INSTANCE_VISUALIZATION:
      return INSTANCE_VERSION_VISUALIZATION;
  }
  return "0.0.0";
}

/*
 * Function used internally on add-on and in Kodi itself to get instance name
 * about given type.
 *
 * @param[in] instanceType The with 'enum ADDON_INSTANCE_TYPE' type to ask
 * @return Name of the asked instance type
 */
inline const char* GetInstanceName(int instanceType)
{
  switch (instanceType)
  {
    case ADDON_INSTANCE_MAIN:
      return "Addon";
    case ADDON_INSTANCE_GUI:
      return "GUI";
    case ADDON_INSTANCE_ADSP:
      return "ADSP";
    case ADDON_INSTANCE_AUDIODECODER:
      return "AudioDecoder";
    case ADDON_INSTANCE_AUDIOENCODER:
      return "AudioEncoder";
    case ADDON_INSTANCE_GAME:
      return "Game";
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

#ifdef __cplusplus
} /* namespace addon */
} /* namespace kodi */
} /* extern "C" */
#endif
