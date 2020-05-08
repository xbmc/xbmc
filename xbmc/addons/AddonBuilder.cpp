/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/AddonBuilder.h"

#include "ServiceBroker.h"
#include "addons/ContextMenuAddon.h"
#include "addons/FontResource.h"
#include "addons/GameResource.h"
#include "addons/ImageResource.h"
#include "addons/LanguageResource.h"
#include "addons/PluginSource.h"
#include "addons/Repository.h"
#include "addons/Scraper.h"
#include "addons/Service.h"
#include "addons/Skin.h"
#include "addons/UISoundsResource.h"
#include "addons/Webinterface.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "pvr/addons/PVRClient.h"
#include "utils/StringUtils.h"

using namespace KODI;

namespace ADDON
{

AddonPtr CAddonBuilder::Generate(const AddonInfoPtr& info, TYPE type)
{
  if (!info || info->ID().empty())
    return AddonPtr();

  if (type == ADDON_UNKNOWN)
    type = info->MainType();
  if (type == ADDON_UNKNOWN)
    return std::make_shared<CAddon>(info, ADDON_UNKNOWN);

  // Handle screensaver special cases
  if (type == ADDON_SCREENSAVER)
  {
    // built in screensaver or python screensaver
    if (StringUtils::StartsWithNoCase(info->ID(), "screensaver.xbmc.builtin.") ||
        URIUtils::HasExtension(info->LibName(), ".py"))
      return std::make_shared<CAddon>(info, type);
  }

  // Handle audio encoder special cases
  if (type == ADDON_AUDIOENCODER)
  {
    // built in audio encoder
    if (StringUtils::StartsWithNoCase(info->ID(), "audioencoder.kodi.builtin."))
      return std::make_shared<CAddonDll>(info, type);
  }

  switch (type)
  {
  case ADDON_AUDIODECODER:
  case ADDON_AUDIOENCODER:
  case ADDON_IMAGEDECODER:
  case ADDON_INPUTSTREAM:
  case ADDON_PERIPHERALDLL:
  case ADDON_PVRDLL:
  case ADDON_VFS:
  case ADDON_VIZ:
  case ADDON_SCREENSAVER:
    return std::make_shared<CAddonDll>(info, type);
  case ADDON_GAMEDLL:
    return std::make_shared<GAME::CGameClient>(info);
  case ADDON_PLUGIN:
  case ADDON_SCRIPT:
    return std::make_shared<CPluginSource>(info, type);
  case ADDON_SCRIPT_LIBRARY:
  case ADDON_SCRIPT_LYRICS:
  case ADDON_SCRIPT_MODULE:
  case ADDON_SUBTITLE_MODULE:
  case ADDON_SCRIPT_WEATHER:
    return std::make_shared<CAddon>(info, type);
  case ADDON_WEB_INTERFACE:
    return std::make_shared<CWebinterface>(info);
  case ADDON_SERVICE:
    return std::make_shared<CService>(info);
  case ADDON_SCRAPER_ALBUMS:
  case ADDON_SCRAPER_ARTISTS:
  case ADDON_SCRAPER_MOVIES:
  case ADDON_SCRAPER_MUSICVIDEOS:
  case ADDON_SCRAPER_TVSHOWS:
  case ADDON_SCRAPER_LIBRARY:
    return std::make_shared<CScraper>(info, type);
  case ADDON_SKIN:
    return std::make_shared<CSkinInfo>(info);
  case ADDON_RESOURCE_FONT:
    return std::make_shared<CFontResource>(info);
  case ADDON_RESOURCE_IMAGES:
    return std::make_shared<CImageResource>(info);
  case ADDON_RESOURCE_GAMES:
    return std::make_shared<CGameResource>(info);
  case ADDON_RESOURCE_LANGUAGE:
    return std::make_shared<CLanguageResource>(info);
  case ADDON_RESOURCE_UISOUNDS:
    return std::make_shared<CUISoundsResource>(info);
  case ADDON_REPOSITORY:
    return std::make_shared<CRepository>(info);
  case ADDON_CONTEXT_ITEM:
    return std::make_shared<CContextMenuAddon>(info);
  case ADDON_GAME_CONTROLLER:
    return std::make_shared<GAME::CController>(info);
  default:
    break;
  }
  return AddonPtr();
}

}
