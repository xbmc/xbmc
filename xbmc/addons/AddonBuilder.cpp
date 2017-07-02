/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/AddonBuilder.h"
#include "addons/ContextMenuAddon.h"
#include "addons/GameResource.h"
#include "addons/ImageDecoder.h"
#include "addons/ImageResource.h"
#include "addons/LanguageResource.h"
#include "addons/PluginSource.h"
#include "addons/Repository.h"
#include "addons/Scraper.h"
#include "addons/ScreenSaver.h"
#include "addons/Service.h"
#include "addons/Skin.h"
#include "addons/UISoundsResource.h"
#include "addons/VFSEntry.h"
#include "addons/Visualization.h"
#include "addons/Webinterface.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "addons/PVRClient.h"
#include "utils/StringUtils.h"

using namespace KODI;

namespace ADDON
{

std::shared_ptr<IAddon> CAddonBuilder::Build()
{
  if (m_built)
    throw std::logic_error("Already built");

  if (m_addonInfo.m_id.empty())
    return nullptr;

  m_built = true;

  if (m_addonInfo.m_mainType == ADDON_UNKNOWN)
    return std::make_shared<CAddon>(std::move(m_addonInfo));

  if (m_extPoint == nullptr)
    return FromProps(std::move(m_addonInfo));

  const TYPE type(m_addonInfo.m_mainType);

  // Handle screensaver special cases
  if (type == ADDON_SCREENSAVER)
  {
    // built in screensaver or python screensaver
    if (StringUtils::StartsWithNoCase(m_extPoint->plugin->identifier, "screensaver.xbmc.builtin.") ||
        URIUtils::HasExtension(CAddonMgr::GetInstance().GetExtValue(m_extPoint->configuration, "@library"), ".py"))
      return std::make_shared<CAddon>(std::move(m_addonInfo));
  }

  // Handle audio encoder special cases
  if (type == ADDON_AUDIOENCODER)
  {
    // built in audio encoder
    if (StringUtils::StartsWithNoCase(m_extPoint->plugin->identifier, "audioencoder.kodi.builtin."))
      return std::make_shared<CAddonDll>(std::move(m_addonInfo));
  }

  // Ensure binary types have a valid library for the platform
  if (type == ADDON_VIZ ||
      type == ADDON_SCREENSAVER ||
      type == ADDON_PVRDLL ||
      type == ADDON_ADSPDLL ||
      type == ADDON_AUDIOENCODER ||
      type == ADDON_AUDIODECODER ||
      type == ADDON_VFS ||
      type == ADDON_IMAGEDECODER ||
      type == ADDON_INPUTSTREAM ||
      type == ADDON_PERIPHERALDLL ||
      type == ADDON_GAMEDLL)
  {
    std::string value = CAddonMgr::GetInstance().GetPlatformLibraryName(m_extPoint->plugin->extensions->configuration);
    if (value.empty())
      return AddonPtr();
  }

  switch (type)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
      return CPluginSource::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
    case ADDON_SCRIPT_WEATHER:
      return std::make_shared<CAddon>(std::move(m_addonInfo));
    case ADDON_WEB_INTERFACE:
      return CWebinterface::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_SERVICE:
      return CService::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      return CScraper::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_ADSPDLL:
    case ADDON_AUDIODECODER:
    case ADDON_AUDIOENCODER:
    case ADDON_IMAGEDECODER:
    case ADDON_INPUTSTREAM:
    case ADDON_PERIPHERALDLL:
    case ADDON_VFS:
    case ADDON_VIZ:
    case ADDON_SCREENSAVER:
      return std::make_shared<CAddonDll>(std::move(m_addonInfo));
    case ADDON_PVRDLL:
      return std::make_shared<PVR::CPVRClient>(std::move(m_addonInfo));
    case ADDON_GAMEDLL:
      return GAME::CGameClient::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_SKIN:
      return CSkinInfo::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_RESOURCE_IMAGES:
      return CImageResource::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_RESOURCE_GAMES:
      return CGameResource::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_RESOURCE_LANGUAGE:
      return CLanguageResource::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_RESOURCE_UISOUNDS:
      return std::make_shared<CUISoundsResource>(std::move(m_addonInfo));
    case ADDON_REPOSITORY:
      return CRepository::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_CONTEXT_ITEM:
      return CContextMenuAddon::FromExtension(std::move(m_addonInfo), m_extPoint);
    case ADDON_GAME_CONTROLLER:
      return GAME::CController::FromExtension(std::move(m_addonInfo), m_extPoint);
    default:
      break;
  }
  return AddonPtr();
}


AddonPtr CAddonBuilder::FromProps(CAddonInfo addonInfo)
{
  // FIXME: there is no need for this as none of the derived classes will contain any useful
  // information. We should return CAddon instances only, however there are several places that
  // down casts, which need to fixed first.
  switch (addonInfo.m_mainType)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
      return AddonPtr(new CPluginSource(std::move(addonInfo)));
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_WEATHER:
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
      return AddonPtr(new CAddon(std::move(addonInfo)));
    case ADDON_WEB_INTERFACE:
      return AddonPtr(new CWebinterface(std::move(addonInfo)));
    case ADDON_SERVICE:
      return AddonPtr(new CService(std::move(addonInfo)));
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      return AddonPtr(new CScraper(std::move(addonInfo)));
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(std::move(addonInfo)));
    case ADDON_ADSPDLL:
    case ADDON_AUDIODECODER:
    case ADDON_AUDIOENCODER:
    case ADDON_IMAGEDECODER:
    case ADDON_INPUTSTREAM:
    case ADDON_PERIPHERALDLL:
    case ADDON_VFS:
    case ADDON_VIZ:
    case ADDON_SCREENSAVER:
      return AddonPtr(new CAddonDll(std::move(addonInfo)));
    case ADDON_PVRDLL:
      return AddonPtr(new PVR::CPVRClient(std::move(addonInfo)));
    case ADDON_RESOURCE_IMAGES:
      return AddonPtr(new CImageResource(std::move(addonInfo)));
    case ADDON_RESOURCE_GAMES:
      return AddonPtr(new CGameResource(std::move(addonInfo)));
    case ADDON_RESOURCE_LANGUAGE:
      return AddonPtr(new CLanguageResource(std::move(addonInfo)));
    case ADDON_RESOURCE_UISOUNDS:
      return AddonPtr(new CUISoundsResource(std::move(addonInfo)));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(std::move(addonInfo)));
    case ADDON_CONTEXT_ITEM:
      return AddonPtr(new CContextMenuAddon(std::move(addonInfo)));
    case ADDON_GAME_CONTROLLER:
      return AddonPtr(new GAME::CController(std::move(addonInfo)));
    case ADDON_GAMEDLL:
      return AddonPtr(new GAME::CGameClient(std::move(addonInfo)));
    default:
      break;
  }
  return AddonPtr();
}
}
