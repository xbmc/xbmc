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
#include "addons/PVRClient.h"
#include "addons/PluginSource.h"
#include "addons/Repository.h"
#include "addons/Scraper.h"
#include "addons/Service.h"
#include "addons/Skin.h"
#include "addons/UISoundsResource.h"
#include "addons/Webinterface.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
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
  case ADDON_VFS:
  case ADDON_VIZ:
  case ADDON_SCREENSAVER:
    return std::make_shared<CAddonDll>(info, type);
  case ADDON_PVRDLL:
    return std::make_shared<PVR::CPVRClient>(info);
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


std::shared_ptr<IAddon> CAddonBuilder::Build()
{
  if (m_built)
    throw std::logic_error("Already built");

  if (m_addonInfo->m_id.empty())
    return nullptr;

  m_built = true;

  if (m_addonInfo->m_mainType == ADDON_UNKNOWN)
    return std::make_shared<CAddon>(m_addonInfo, ADDON_UNKNOWN);

  if (m_extPoint == nullptr)
    return FromProps(m_addonInfo);

  const TYPE type(m_addonInfo->m_mainType);

  // Handle screensaver special cases
  if (type == ADDON_SCREENSAVER)
  {
    // built in screensaver or python screensaver
    if (StringUtils::StartsWithNoCase(m_extPoint->plugin->identifier, "screensaver.xbmc.builtin.") ||
        URIUtils::HasExtension(CServiceBroker::GetAddonMgr().GetExtValue(m_extPoint->configuration, "@library"), ".py"))
      return std::make_shared<CAddon>(m_addonInfo, ADDON_SCREENSAVER);
  }

  // Handle audio encoder special cases
  if (type == ADDON_AUDIOENCODER)
  {
    // built in audio encoder
    if (StringUtils::StartsWithNoCase(m_extPoint->plugin->identifier, "audioencoder.kodi.builtin."))
      return std::make_shared<CAddonDll>(m_addonInfo, ADDON_AUDIOENCODER);
  }

  // Ensure binary types have a valid library for the platform
  if (type == ADDON_VIZ ||
      type == ADDON_SCREENSAVER ||
      type == ADDON_PVRDLL ||
      type == ADDON_AUDIOENCODER ||
      type == ADDON_AUDIODECODER ||
      type == ADDON_VFS ||
      type == ADDON_IMAGEDECODER ||
      type == ADDON_INPUTSTREAM ||
      type == ADDON_PERIPHERALDLL ||
      type == ADDON_GAMEDLL)
  {
    std::string value = CServiceBroker::GetAddonMgr().GetPlatformLibraryName(m_extPoint->plugin->extensions->configuration);
    if (value.empty())
      return AddonPtr();
  }

  switch (type)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
      return CPluginSource::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
    case ADDON_SCRIPT_WEATHER:
      return std::make_shared<CAddon>(m_addonInfo, type);
    case ADDON_WEB_INTERFACE:
      return CWebinterface::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_SERVICE:
      return CService::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      return CScraper::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_AUDIODECODER:
    case ADDON_AUDIOENCODER:
    case ADDON_IMAGEDECODER:
    case ADDON_INPUTSTREAM:
    case ADDON_PERIPHERALDLL:
    case ADDON_VFS:
    case ADDON_VIZ:
    case ADDON_SCREENSAVER:
      return std::make_shared<CAddonDll>(m_addonInfo, type);
    case ADDON_PVRDLL:
      return std::make_shared<PVR::CPVRClient>(m_addonInfo);
    case ADDON_GAMEDLL:
      return GAME::CGameClient::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_SKIN:
      return CSkinInfo::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_RESOURCE_FONT:
      return CFontResource::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_RESOURCE_IMAGES:
      return CImageResource::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_RESOURCE_GAMES:
      return CGameResource::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_RESOURCE_LANGUAGE:
      return CLanguageResource::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_RESOURCE_UISOUNDS:
      return std::make_shared<CUISoundsResource>(m_addonInfo);
    case ADDON_REPOSITORY:
      return CRepository::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_CONTEXT_ITEM:
      return CContextMenuAddon::FromExtension(m_addonInfo, m_extPoint);
    case ADDON_GAME_CONTROLLER:
      return GAME::CController::FromExtension(m_addonInfo, m_extPoint);
    default:
      break;
  }
  return AddonPtr();
}


AddonPtr CAddonBuilder::FromProps(const AddonInfoPtr& addonInfo)
{
  // FIXME: there is no need for this as none of the derived classes will contain any useful
  // information. We should return CAddon instances only, however there are several places that
  // down casts, which need to fixed first.
  switch (addonInfo->m_mainType)
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
      return AddonPtr(new CPluginSource(addonInfo, addonInfo->m_mainType));
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_WEATHER:
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
      return AddonPtr(new CAddon(addonInfo, addonInfo->m_mainType));
    case ADDON_WEB_INTERFACE:
      return AddonPtr(new CWebinterface(addonInfo));
    case ADDON_SERVICE:
      return AddonPtr(new CService(addonInfo));
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      return AddonPtr(new CScraper(addonInfo, addonInfo->m_mainType));
    case ADDON_SKIN:
      return AddonPtr(new CSkinInfo(addonInfo));
    case ADDON_AUDIODECODER:
    case ADDON_AUDIOENCODER:
    case ADDON_IMAGEDECODER:
    case ADDON_INPUTSTREAM:
    case ADDON_PERIPHERALDLL:
    case ADDON_VFS:
    case ADDON_VIZ:
    case ADDON_SCREENSAVER:
      return AddonPtr(new CAddonDll(addonInfo, addonInfo->m_mainType));
    case ADDON_PVRDLL:
      return AddonPtr(new PVR::CPVRClient(addonInfo));
    case ADDON_RESOURCE_FONT:
      return AddonPtr(new CFontResource(addonInfo));
    case ADDON_RESOURCE_IMAGES:
      return AddonPtr(new CImageResource(addonInfo));
    case ADDON_RESOURCE_GAMES:
      return AddonPtr(new CGameResource(addonInfo));
    case ADDON_RESOURCE_LANGUAGE:
      return AddonPtr(new CLanguageResource(addonInfo));
    case ADDON_RESOURCE_UISOUNDS:
      return AddonPtr(new CUISoundsResource(addonInfo));
    case ADDON_REPOSITORY:
      return AddonPtr(new CRepository(addonInfo));
    case ADDON_CONTEXT_ITEM:
      return AddonPtr(new CContextMenuAddon(addonInfo));
    case ADDON_GAME_CONTROLLER:
      return AddonPtr(new GAME::CController(addonInfo));
    case ADDON_GAMEDLL:
      return AddonPtr(new GAME::CGameClient(addonInfo));
    default:
      break;
  }
  return AddonPtr();
}
}
