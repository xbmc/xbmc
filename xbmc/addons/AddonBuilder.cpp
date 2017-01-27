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
#include "addons/AudioDecoder.h"
#include "addons/AudioEncoder.h"
#include "addons/ContextMenuAddon.h"
#include "addons/GameResource.h"
#include "addons/ImageResource.h"
#include "addons/InputStream.h"
#include "addons/LanguageResource.h"
#include "addons/Repository.h"
#include "addons/Scraper.h"
#include "addons/Service.h"
#include "addons/Skin.h"
#include "addons/UISoundsResource.h"
#include "addons/VFSEntry.h"
#include "addons/Webinterface.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"
#include "games/addons/GameClient.h"
#include "games/controllers/Controller.h"
#include "peripherals/addons/PeripheralAddon.h"
#include "addons/PVRClient.h"
#include "utils/StringUtils.h"

namespace ADDON
{

std::shared_ptr<CAddon> CAddonBuilder::Build()
{
  if (m_built)
    throw std::logic_error("Already built");

  if (m_addonInfo.m_id.empty())
    return nullptr;

  m_built = true;

  if (m_addonInfo.m_type == ADDON_UNKNOWN)
    return std::make_shared<CAddon>(std::move(m_addonInfo));

  const TYPE type(m_addonInfo.m_type);

  /*
   * Temporary to create props from path
   * becomes later done on other place
   */
  CAddonInfo addonInfo(m_addonInfo.Path());
  
  /*
   * Startup of reworked addon interfaces, switch currently two times until
   * rework is finished. On end is only for all binary addons one
   * "return std::make_shared<CAddonDll>(std::move(m_addonInfo));" needed.
   */
  switch (type)
  {
    case ADDON_VIZ:
    case ADDON_SCREENSAVER:
      return std::make_shared<CAddonDll>(std::move(addonInfo));
    default:
      break;
  }

  if (m_extPoint == nullptr)
  {
    fprintf(stderr, "(m_extPoint == nullptr)!!!!!!!! '%s'\n", m_addonInfo.m_id.c_str());
    int* a = nullptr;
    a[0] = 0;
  }

  // Handle audio encoder special cases
  if (type == ADDON_AUDIOENCODER)
  {
    // built in audio encoder
    if (StringUtils::StartsWithNoCase(addonInfo.ID(), "audioencoder.xbmc.builtin."))
      return std::make_shared<CAudioEncoder>(std::move(addonInfo));
  }

  // Ensure binary types have a valid library for the platform
  if (type == ADDON_PVRDLL ||
      type == ADDON_ADSPDLL ||
      type == ADDON_AUDIOENCODER ||
      type == ADDON_AUDIODECODER ||
      type == ADDON_VFS ||
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
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
    case ADDON_SCRIPT_WEATHER:
      return std::make_shared<CAddon>(std::move(m_addonInfo));
    case ADDON_WEB_INTERFACE:
      return std::make_shared<CWebinterface>(std::move(addonInfo));
    case ADDON_SERVICE:
      return std::make_shared<CService>(std::move(addonInfo));
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      return std::make_shared<CScraper>(std::move(addonInfo));
    case ADDON_PVRDLL:
      return std::make_shared<PVR::CPVRClient>(std::move(addonInfo));
    case ADDON_ADSPDLL:
      return std::make_shared<ActiveAE::CActiveAEDSPAddon>(std::move(addonInfo));
    case ADDON_AUDIOENCODER:
      return std::make_shared<CAudioEncoder>(std::move(addonInfo));
    case ADDON_AUDIODECODER:
      return std::make_shared<CAudioDecoder>(std::move(addonInfo));
    case ADDON_INPUTSTREAM:
      return std::make_shared<CInputStream>(std::move(addonInfo));
    case ADDON_PERIPHERALDLL:
      return std::make_shared<PERIPHERALS::CPeripheralAddon>(std::move(addonInfo));
    case ADDON_GAMEDLL:
      return std::make_shared<GAME::CGameClient>(std::move(addonInfo));
    case ADDON_VFS:
      return std::make_shared<CVFSEntry>(std::move(addonInfo));
    case ADDON_SKIN:
      return std::make_shared<CSkinInfo>(std::move(addonInfo));
    case ADDON_RESOURCE_IMAGES:
      return std::make_shared<CImageResource>(std::move(addonInfo));
    case ADDON_RESOURCE_GAMES:
      return std::make_shared<CGameResource>(std::move(addonInfo));
    case ADDON_RESOURCE_LANGUAGE:
      return std::make_shared<CLanguageResource>(std::move(addonInfo));
    case ADDON_RESOURCE_UISOUNDS:
      return std::make_shared<CUISoundsResource>(std::move(addonInfo));
    case ADDON_REPOSITORY:
      return std::make_shared<CRepository>(std::move(addonInfo));
    case ADDON_CONTEXT_ITEM:
      return std::make_shared<CContextMenuAddon>(std::move(addonInfo));
    case ADDON_GAME_CONTROLLER:
      return GAME::CController::FromExtension(std::move(m_addonInfo), m_extPoint);
    default:
      break;
  }
  return AddonPtr();
}

}
