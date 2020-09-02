/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

/*
 *------------------------------------------------------------------------------
 * Some parts on headers are only be used for Kodi itself and internally (not
 * for add-on development).
 *
 * For this reason also no doxygen part with "///" defined there.
 * -----------------------------------------------------------------------------
 */

/*
 * Versions of all add-on globals and instances are defined below.
 *
 * This is added here and not in related header to prevent not
 * needed includes during compile. Also have it here a better
 * overview.
 */

// Ignore clang here, as this must be good in overview and as the main reason,
// because cmake uses this area in this form to perform its addon dependency
// check.
// clang-format off
#define ADDON_GLOBAL_VERSION_MAIN                     "1.2.4"
#define ADDON_GLOBAL_VERSION_MAIN_MIN                 "1.2.0"
#define ADDON_GLOBAL_VERSION_MAIN_XML_ID              "kodi.binary.global.main"
#define ADDON_GLOBAL_VERSION_MAIN_DEPENDS             "AddonBase.h" \
                                                      "addon-instance/" \
                                                      "c-api/addon_base.h"

#define ADDON_GLOBAL_VERSION_GENERAL                  "1.0.5"
#define ADDON_GLOBAL_VERSION_GENERAL_MIN              "1.0.4"
#define ADDON_GLOBAL_VERSION_GENERAL_XML_ID           "kodi.binary.global.general"
#define ADDON_GLOBAL_VERSION_GENERAL_DEPENDS          "General.h"

#define ADDON_GLOBAL_VERSION_GUI                      "5.14.1"
#define ADDON_GLOBAL_VERSION_GUI_MIN                  "5.14.0"
#define ADDON_GLOBAL_VERSION_GUI_XML_ID               "kodi.binary.global.gui"
#define ADDON_GLOBAL_VERSION_GUI_DEPENDS              "ActionIDs.h" \
                                                      "gui/"

#define ADDON_GLOBAL_VERSION_AUDIOENGINE              "1.1.1"
#define ADDON_GLOBAL_VERSION_AUDIOENGINE_MIN          "1.1.0"
#define ADDON_GLOBAL_VERSION_AUDIOENGINE_XML_ID       "kodi.binary.global.audioengine"
#define ADDON_GLOBAL_VERSION_AUDIOENGINE_DEPENDS      "AudioEngine.h" \
                                                      "c-api/audio_engine.h"

#define ADDON_GLOBAL_VERSION_FILESYSTEM               "1.1.4"
#define ADDON_GLOBAL_VERSION_FILESYSTEM_MIN           "1.1.0"
#define ADDON_GLOBAL_VERSION_FILESYSTEM_XML_ID        "kodi.binary.global.filesystem"
#define ADDON_GLOBAL_VERSION_FILESYSTEM_DEPENDS       "Filesystem.h" \
                                                      "c-api/filesystem.h" \
                                                      "gui/gl/Shader.h" \
                                                      "tools/DllHelper.h"

#define ADDON_GLOBAL_VERSION_NETWORK                  "1.0.4"
#define ADDON_GLOBAL_VERSION_NETWORK_MIN              "1.0.0"
#define ADDON_GLOBAL_VERSION_NETWORK_XML_ID           "kodi.binary.global.network"
#define ADDON_GLOBAL_VERSION_NETWORK_DEPENDS          "Network.h" \
                                                      "c-api/network.h"

#define ADDON_GLOBAL_VERSION_TOOLS                    "1.0.1"
#define ADDON_GLOBAL_VERSION_TOOLS_MIN                "1.0.0"
#define ADDON_GLOBAL_VERSION_TOOLS_XML_ID             "kodi.binary.global.tools"
#define ADDON_GLOBAL_VERSION_TOOLS_DEPENDS            "tools/DllHelper.h"

#define ADDON_INSTANCE_VERSION_AUDIODECODER           "3.0.0"
#define ADDON_INSTANCE_VERSION_AUDIODECODER_MIN       "3.0.0"
#define ADDON_INSTANCE_VERSION_AUDIODECODER_XML_ID    "kodi.binary.instance.audiodecoder"
#define ADDON_INSTANCE_VERSION_AUDIODECODER_DEPENDS   "c-api/addon-instance/audio_decoder.h" \
                                                      "addon-instance/AudioDecoder.h"

#define ADDON_INSTANCE_VERSION_AUDIOENCODER           "2.1.0"
#define ADDON_INSTANCE_VERSION_AUDIOENCODER_MIN       "2.1.0"
#define ADDON_INSTANCE_VERSION_AUDIOENCODER_XML_ID    "kodi.binary.instance.audioencoder"
#define ADDON_INSTANCE_VERSION_AUDIOENCODER_DEPENDS   "c-api/addon-instance/audio_encoder.h" \
                                                      "addon-instance/AudioEncoder.h"

#define ADDON_INSTANCE_VERSION_GAME                   "2.0.2"
#define ADDON_INSTANCE_VERSION_GAME_MIN               "2.0.1"
#define ADDON_INSTANCE_VERSION_GAME_XML_ID            "kodi.binary.instance.game"
#define ADDON_INSTANCE_VERSION_GAME_DEPENDS           "addon-instance/Game.h"

#define ADDON_INSTANCE_VERSION_IMAGEDECODER           "2.1.1"
#define ADDON_INSTANCE_VERSION_IMAGEDECODER_MIN       "2.1.0"
#define ADDON_INSTANCE_VERSION_IMAGEDECODER_XML_ID    "kodi.binary.instance.imagedecoder"
#define ADDON_INSTANCE_VERSION_IMAGEDECODER_DEPENDS   "addon-instance/ImageDecoder.h"

#define ADDON_INSTANCE_VERSION_INPUTSTREAM            "2.3.3"
#define ADDON_INSTANCE_VERSION_INPUTSTREAM_MIN        "2.3.1"
#define ADDON_INSTANCE_VERSION_INPUTSTREAM_XML_ID     "kodi.binary.instance.inputstream"
#define ADDON_INSTANCE_VERSION_INPUTSTREAM_DEPENDS    "addon-instance/Inputstream.h"

#define ADDON_INSTANCE_VERSION_PERIPHERAL             "2.0.0"
#define ADDON_INSTANCE_VERSION_PERIPHERAL_MIN         "2.0.0"
#define ADDON_INSTANCE_VERSION_PERIPHERAL_XML_ID      "kodi.binary.instance.peripheral"
#define ADDON_INSTANCE_VERSION_PERIPHERAL_DEPENDS     "addon-instance/Peripheral.h" \
                                                      "addon-instance/PeripheralUtils.h"

#define ADDON_INSTANCE_VERSION_PVR                    "7.0.1"
#define ADDON_INSTANCE_VERSION_PVR_MIN                "7.0.0"
#define ADDON_INSTANCE_VERSION_PVR_XML_ID             "kodi.binary.instance.pvr"
#define ADDON_INSTANCE_VERSION_PVR_DEPENDS            "c-api/addon-instance/pvr.h" \
                                                      "c-api/addon-instance/pvr/pvr_channel_groups.h" \
                                                      "c-api/addon-instance/pvr/pvr_channels.h" \
                                                      "c-api/addon-instance/pvr/pvr_defines.h" \
                                                      "c-api/addon-instance/pvr/pvr_edl.h" \
                                                      "c-api/addon-instance/pvr/pvr_epg.h" \
                                                      "c-api/addon-instance/pvr/pvr_general.h" \
                                                      "c-api/addon-instance/pvr/pvr_menu_hook.h" \
                                                      "c-api/addon-instance/pvr/pvr_recordings.h" \
                                                      "c-api/addon-instance/pvr/pvr_stream.h" \
                                                      "c-api/addon-instance/pvr/pvr_timers.h" \
                                                      "addon-instance/PVR.h" \
                                                      "addon-instance/pvr/ChannelGroups.h" \
                                                      "addon-instance/pvr/Channels.h" \
                                                      "addon-instance/pvr/EDL.h" \
                                                      "addon-instance/pvr/EPG.h" \
                                                      "addon-instance/pvr/General.h" \
                                                      "addon-instance/pvr/MenuHook.h" \
                                                      "addon-instance/pvr/Recordings.h" \
                                                      "addon-instance/pvr/Stream.h" \
                                                      "addon-instance/pvr/Timers.h"

#define ADDON_INSTANCE_VERSION_SCREENSAVER            "2.1.0"
#define ADDON_INSTANCE_VERSION_SCREENSAVER_MIN        "2.1.0"
#define ADDON_INSTANCE_VERSION_SCREENSAVER_XML_ID     "kodi.binary.instance.screensaver"
#define ADDON_INSTANCE_VERSION_SCREENSAVER_DEPENDS    "c-api/addon-instance/screensaver.h" \
                                                      "addon-instance/Screensaver.h"

#define ADDON_INSTANCE_VERSION_VFS                    "2.3.2"
#define ADDON_INSTANCE_VERSION_VFS_MIN                "2.3.1"
#define ADDON_INSTANCE_VERSION_VFS_XML_ID             "kodi.binary.instance.vfs"
#define ADDON_INSTANCE_VERSION_VFS_DEPENDS            "addon-instance/VFS.h"

#define ADDON_INSTANCE_VERSION_VISUALIZATION          "3.0.0"
#define ADDON_INSTANCE_VERSION_VISUALIZATION_MIN      "3.0.0"
#define ADDON_INSTANCE_VERSION_VISUALIZATION_XML_ID   "kodi.binary.instance.visualization"
#define ADDON_INSTANCE_VERSION_VISUALIZATION_DEPENDS  "addon-instance/Visualization.h" \
                                                      "c-api/addon-instance/visualization.h"

#define ADDON_INSTANCE_VERSION_VIDEOCODEC             "1.0.3"
#define ADDON_INSTANCE_VERSION_VIDEOCODEC_MIN         "1.0.2"
#define ADDON_INSTANCE_VERSION_VIDEOCODEC_XML_ID      "kodi.binary.instance.videocodec"
#define ADDON_INSTANCE_VERSION_VIDEOCODEC_DEPENDS     "addon-instance/VideoCodec.h" \
                                                      "StreamCodec.h" \
                                                      "StreamCrypto.h"
// clang-format on

//==============================================================================
///
/// @ingroup cpp_kodi_addon_addonbase
/// The currently available instance types for Kodi add-ons
///
/// \internal
/// @note For add of new types take a new number on end. To change
/// existing numbers can be make problems on already compiled add-ons.
/// \endinternal
///
typedef enum ADDON_TYPE
{
  /* addon global parts */
  ADDON_GLOBAL_MAIN = 0,
  ADDON_GLOBAL_GUI = 1,
  ADDON_GLOBAL_AUDIOENGINE = 2,
  ADDON_GLOBAL_GENERAL = 3,
  ADDON_GLOBAL_NETWORK = 4,
  ADDON_GLOBAL_FILESYSTEM = 5,
  ADDON_GLOBAL_TOOLS = 6,
  // Last used global id, used in loops to check versions.
  // Need to change if new global type becomes added!
  ADDON_GLOBAL_MAX = 6,

  /* addon type instances */

  /// Audio decoder instance, see \ref cpp_kodi_addon_audiodecoder "kodi::addon::CInstanceAudioDecoder"
  ADDON_INSTANCE_AUDIODECODER = 102,

  /// Audio encoder instance, see \ref cpp_kodi_addon_audioencoder "kodi::addon::CInstanceAudioEncoder"
  ADDON_INSTANCE_AUDIOENCODER = 103,

  /// Game instance, see \ref cpp_kodi_addon_game "kodi::addon::CInstanceGame"
  ADDON_INSTANCE_GAME = 104,

  /// Input stream instance, see \ref cpp_kodi_addon_inputstream "kodi::addon::CInstanceInputStream"
  ADDON_INSTANCE_INPUTSTREAM = 105,

  /// Peripheral instance, see \ref cpp_kodi_addon_peripheral "kodi::addon::CInstancePeripheral"
  ADDON_INSTANCE_PERIPHERAL = 106,

  /// Game instance, see \ref cpp_kodi_addon_pvr "kodi::addon::CInstancePVRClient"
  ADDON_INSTANCE_PVR = 107,

  /// PVR client instance, see \ref cpp_kodi_addon_screensaver "kodi::addon::CInstanceScreensaver"
  ADDON_INSTANCE_SCREENSAVER = 108,

  /// Music visualization instance, see \ref cpp_kodi_addon_visualization "kodi::addon::CInstanceVisualization"
  ADDON_INSTANCE_VISUALIZATION = 109,

  /// Virtual Filesystem (VFS) instance, see \ref cpp_kodi_addon_vfs "kodi::addon::CInstanceVFS"
  ADDON_INSTANCE_VFS = 110,

  /// Image Decoder instance, see \ref cpp_kodi_addon_imagedecoder "kodi::addon::CInstanceImageDecoder"
  ADDON_INSTANCE_IMAGEDECODER = 111,

  /// Video Decoder instance, see \ref cpp_kodi_addon_videocodec "kodi::addon::CInstanceVideoCodec"
  ADDON_INSTANCE_VIDEOCODEC = 112,
} ADDON_TYPE;
//------------------------------------------------------------------------------

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
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_GLOBAL_VERSION_GENERAL_USED)
    case ADDON_GLOBAL_GENERAL:
      return ADDON_GLOBAL_VERSION_GENERAL;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_GLOBAL_VERSION_GUI_USED)
    case ADDON_GLOBAL_GUI:
      return ADDON_GLOBAL_VERSION_GUI;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_GLOBAL_VERSION_AUDIOENGINE_USED)
    case ADDON_GLOBAL_AUDIOENGINE:
      return ADDON_GLOBAL_VERSION_AUDIOENGINE;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_GLOBAL_VERSION_FILESYSTEM_USED)
    case ADDON_GLOBAL_FILESYSTEM:
      return ADDON_GLOBAL_VERSION_FILESYSTEM;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_GLOBAL_VERSION_NETWORK_USED)
    case ADDON_GLOBAL_NETWORK:
      return ADDON_GLOBAL_VERSION_NETWORK;
#endif
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_GLOBAL_VERSION_TOOLS_USED)
    case ADDON_GLOBAL_TOOLS:
      return ADDON_GLOBAL_VERSION_TOOLS;
#endif

    /* addon type instances */
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
#if !defined(BUILD_KODI_ADDON) || defined(ADDON_INSTANCE_VERSION_VIDEOCODEC_USED)
    case ADDON_INSTANCE_VIDEOCODEC:
      return ADDON_INSTANCE_VERSION_VIDEOCODEC;
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
    case ADDON_GLOBAL_GENERAL:
      return ADDON_GLOBAL_VERSION_GENERAL_MIN;
    case ADDON_GLOBAL_AUDIOENGINE:
      return ADDON_GLOBAL_VERSION_AUDIOENGINE_MIN;
    case ADDON_GLOBAL_FILESYSTEM:
      return ADDON_GLOBAL_VERSION_FILESYSTEM_MIN;
    case ADDON_GLOBAL_NETWORK:
      return ADDON_GLOBAL_VERSION_NETWORK_MIN;
    case ADDON_GLOBAL_TOOLS:
      return ADDON_GLOBAL_VERSION_TOOLS_MIN;

    /* addon type instances */
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
    case ADDON_INSTANCE_VIDEOCODEC:
      return ADDON_INSTANCE_VERSION_VIDEOCODEC_MIN;
  }
  return "0.0.0";
}

///
/// Function used internally on add-on and in Kodi itself to get name
/// about given type.
///
/// @param[in] type The with 'enum ADDON_TYPE' defined type to ask
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
    case ADDON_GLOBAL_GENERAL:
      return "General";
    case ADDON_GLOBAL_AUDIOENGINE:
      return "AudioEngine";
    case ADDON_GLOBAL_FILESYSTEM:
      return "Filesystem";
    case ADDON_GLOBAL_NETWORK:
      return "Network";
    case ADDON_GLOBAL_TOOLS:
      return "Tools";

    /* addon type instances */
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
    case ADDON_INSTANCE_VIDEOCODEC:
      return "VideoCodec";
  }
  return "unknown";
}

///
/// Function used internally on add-on and in Kodi itself to get id number
/// about given type name.
///
/// @param[in] name The type name string to ask
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
    else if (strcmp(name, "general") == 0)
      return ADDON_GLOBAL_GENERAL;
    else if (strcmp(name, "gui") == 0)
      return ADDON_GLOBAL_GUI;
    else if (strcmp(name, "audioengine") == 0)
      return ADDON_GLOBAL_AUDIOENGINE;
    else if (strcmp(name, "filesystem") == 0)
      return ADDON_GLOBAL_FILESYSTEM;
    else if (strcmp(name, "network") == 0)
      return ADDON_GLOBAL_NETWORK;
    else if (strcmp(name, "tools") == 0)
      return ADDON_GLOBAL_TOOLS;
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
    else if (strcmp(name, "videocodec") == 0)
      return ADDON_INSTANCE_VIDEOCODEC;
  }
  return -1;
}

#ifdef __cplusplus
} /* namespace addon */
} /* namespace kodi */
} /* extern "C" */
#endif
