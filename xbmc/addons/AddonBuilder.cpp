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
  m_built = true;
  
  /*
   * Temporary to create props from path
   * becomes later done on other place
   */
  CAddonInfo addonInfo(m_addonInfo.Path());

  switch (addonInfo.Type())
  {
    case ADDON_PLUGIN:
    case ADDON_SCRIPT:
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_MODULE:
    case ADDON_SUBTITLE_MODULE:
    case ADDON_SCRIPT_WEATHER:
      return std::make_shared<CAddon>(std::move(addonInfo));
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
    case ADDON_VIZ:
    case ADDON_SCREENSAVER:
      return std::make_shared<CAddonDll>(std::move(addonInfo));
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
      return std::make_shared<GAME::CController>(std::move(addonInfo));
    default:
      break;
  }

  return std::make_shared<CAddon>(std::move(addonInfo));
}

}
