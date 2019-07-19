/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "addons/kodi-addon-dev-kit/include/kodi/versions.h"
#include "CompileInfo.h"

#include <algorithm>
#include <array>

namespace ADDON
{
  struct APIVersion
  {
    std::string id;
    AddonVersion version;
    AddonVersion backwardsCompatibility;
    APIVersion(std::string id, AddonVersion version, AddonVersion backwardsCompatibility) :
        id(id), version(version), backwardsCompatibility(backwardsCompatibility) {};
  };

  const std::array<const APIVersion, 25> API_VERSIONS = {{
      {"xbmc.addon", AddonVersion(CCompileInfo::GetAddonApiVersion()), AddonVersion("12.0.0")},
      {"xbmc.core", AddonVersion("0.1.0"), AddonVersion("0.1.0")},
      {"xbmc.gui", AddonVersion("5.14.0"), AddonVersion("5.14.0")},
      {"xbmc.json", AddonVersion(CCompileInfo::GetJsonRpcVersion()), AddonVersion("6.0.0")},
      {"xbmc.metadata", AddonVersion("2.1.0"), AddonVersion("1.0")},
      {"xbmc.python", AddonVersion("2.26.0"), AddonVersion("2.1.0")},
      {"xbmc.webinterface", AddonVersion("1.0.0"), AddonVersion("1.0.0")},
      {"kodi.resource", AddonVersion("1.0.0"), AddonVersion("1.0.0")},
      {ADDON_GLOBAL_VERSION_MAIN_XML_ID, AddonVersion(ADDON_GLOBAL_VERSION_MAIN), AddonVersion(ADDON_GLOBAL_VERSION_MAIN_MIN)},
      {ADDON_GLOBAL_VERSION_GENERAL_XML_ID, AddonVersion(ADDON_GLOBAL_VERSION_GENERAL), AddonVersion(ADDON_GLOBAL_VERSION_GENERAL_MIN)},
      {ADDON_GLOBAL_VERSION_GUI_XML_ID, AddonVersion(ADDON_GLOBAL_VERSION_GUI), AddonVersion(ADDON_GLOBAL_VERSION_GUI_MIN)},
      {ADDON_GLOBAL_VERSION_AUDIOENGINE_XML_ID, AddonVersion(ADDON_GLOBAL_VERSION_AUDIOENGINE), AddonVersion(ADDON_GLOBAL_VERSION_AUDIOENGINE_MIN)},
      {ADDON_GLOBAL_VERSION_FILESYSTEM_XML_ID, AddonVersion(ADDON_GLOBAL_VERSION_FILESYSTEM), AddonVersion(ADDON_GLOBAL_VERSION_FILESYSTEM_MIN)},
      {ADDON_GLOBAL_VERSION_NETWORK_XML_ID, AddonVersion(ADDON_GLOBAL_VERSION_NETWORK), AddonVersion(ADDON_GLOBAL_VERSION_NETWORK_MIN)},
      {ADDON_INSTANCE_VERSION_AUDIODECODER_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_AUDIODECODER), AddonVersion(ADDON_INSTANCE_VERSION_AUDIODECODER_MIN)},
      {ADDON_INSTANCE_VERSION_AUDIOENCODER_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_AUDIOENCODER), AddonVersion(ADDON_INSTANCE_VERSION_AUDIOENCODER_MIN)},
      {ADDON_INSTANCE_VERSION_GAME_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_GAME), AddonVersion(ADDON_INSTANCE_VERSION_GAME_MIN)},
      {ADDON_INSTANCE_VERSION_IMAGEDECODER_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_IMAGEDECODER), AddonVersion(ADDON_INSTANCE_VERSION_IMAGEDECODER_MIN)},
      {ADDON_INSTANCE_VERSION_INPUTSTREAM_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_INPUTSTREAM), AddonVersion(ADDON_INSTANCE_VERSION_INPUTSTREAM_MIN)},
      {ADDON_INSTANCE_VERSION_PERIPHERAL_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_PERIPHERAL), AddonVersion(ADDON_INSTANCE_VERSION_PERIPHERAL_MIN)},
      {ADDON_INSTANCE_VERSION_PVR_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_PVR), AddonVersion(ADDON_INSTANCE_VERSION_PVR_MIN)},
      {ADDON_INSTANCE_VERSION_SCREENSAVER_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_SCREENSAVER), AddonVersion(ADDON_INSTANCE_VERSION_SCREENSAVER_MIN)},
      {ADDON_INSTANCE_VERSION_VFS_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_VFS), AddonVersion(ADDON_INSTANCE_VERSION_VFS_MIN)},
      {ADDON_INSTANCE_VERSION_VISUALIZATION_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_VISUALIZATION), AddonVersion(ADDON_INSTANCE_VERSION_VISUALIZATION_MIN)},
      {ADDON_INSTANCE_VERSION_VIDEOCODEC_XML_ID, AddonVersion(ADDON_INSTANCE_VERSION_VIDEOCODEC), AddonVersion(ADDON_INSTANCE_VERSION_VIDEOCODEC_MIN)}
  }};

};
