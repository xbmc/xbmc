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
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "pvr/addons/PVRClient.h"
#include "utils/StringUtils.h"

using namespace KODI;

namespace ADDON
{

AddonPtr CAddonBuilder::Generate(const AddonInfoPtr& info, AddonType type)
{
  if (!info || info->ID().empty())
    return AddonPtr();

  if (type == AddonType::UNKNOWN)
    type = info->MainType();
  if (type == AddonType::UNKNOWN)
    return std::make_shared<CAddon>(info, AddonType::UNKNOWN);

  // Handle screensaver special cases
  if (type == AddonType::SCREENSAVER)
  {
    // built in screensaver or python screensaver
    if (StringUtils::StartsWithNoCase(info->ID(), "screensaver.xbmc.builtin.") ||
        URIUtils::HasExtension(info->LibName(), ".py"))
      return std::make_shared<CAddon>(info, type);
  }

  // Handle audio encoder special cases
  if (type == AddonType::AUDIOENCODER)
  {
    // built in audio encoder
    if (StringUtils::StartsWithNoCase(info->ID(), "audioencoder.kodi.builtin."))
      return std::make_shared<CAddonDll>(info, type);
  }

  switch (type)
  {
    case AddonType::AUDIODECODER:
    case AddonType::AUDIOENCODER:
    case AddonType::IMAGEDECODER:
    case AddonType::INPUTSTREAM:
    case AddonType::PERIPHERALDLL:
    case AddonType::PVRDLL:
    case AddonType::VFS:
    case AddonType::VISUALIZATION:
    case AddonType::SCREENSAVER:
      return std::make_shared<CAddonDll>(info, type);
    case AddonType::GAMEDLL:
      return std::make_shared<GAME::CGameClient>(info);
    case AddonType::PLUGIN:
    case AddonType::SCRIPT:
      return std::make_shared<CPluginSource>(info, type);
    case AddonType::SCRIPT_LIBRARY:
    case AddonType::SCRIPT_LYRICS:
    case AddonType::SCRIPT_MODULE:
    case AddonType::SUBTITLE_MODULE:
    case AddonType::SCRIPT_WEATHER:
      return std::make_shared<CAddon>(info, type);
    case AddonType::WEB_INTERFACE:
      return std::make_shared<CWebinterface>(info);
    case AddonType::SERVICE:
      return std::make_shared<CService>(info);
    case AddonType::SCRAPER_ALBUMS:
    case AddonType::SCRAPER_ARTISTS:
    case AddonType::SCRAPER_MOVIES:
    case AddonType::SCRAPER_MUSICVIDEOS:
    case AddonType::SCRAPER_TVSHOWS:
    case AddonType::SCRAPER_LIBRARY:
      return std::make_shared<CScraper>(info, type);
    case AddonType::SKIN:
      return std::make_shared<CSkinInfo>(info);
    case AddonType::RESOURCE_FONT:
      return std::make_shared<CFontResource>(info);
    case AddonType::RESOURCE_IMAGES:
      return std::make_shared<CImageResource>(info);
    case AddonType::RESOURCE_GAMES:
      return std::make_shared<CGameResource>(info);
    case AddonType::RESOURCE_LANGUAGE:
      return std::make_shared<CLanguageResource>(info);
    case AddonType::RESOURCE_UISOUNDS:
      return std::make_shared<CUISoundsResource>(info);
    case AddonType::REPOSITORY:
      return std::make_shared<CRepository>(info);
    case AddonType::CONTEXTMENU_ITEM:
      return std::make_shared<CContextMenuAddon>(info);
    case AddonType::GAME_CONTROLLER:
      return std::make_shared<GAME::CController>(info);
    default:
      break;
  }
  return AddonPtr();
}

}
