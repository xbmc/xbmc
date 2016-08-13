/*
 *      Copyright (C) 2015 Team Kodi
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

#include "addons/AddonManager.h"
#include "addons/AddonSystemSettings.h"
#include "addons/RepositoryUpdater.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"


namespace ADDON
{

CAddonSystemSettings& CAddonSystemSettings::GetInstance()
{
  static CAddonSystemSettings inst;
  return inst;
}

void CAddonSystemSettings::OnSettingAction(const CSetting* setting)
{
  if (setting->GetId() == CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES)
  {
    std::vector<std::string> params{"addons://dependencies/", "return"};
    g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
  }
  else if (setting->GetId() == CSettings::SETTING_ADDONS_SHOW_RUNNING)
  {
    std::vector<std::string> params{"addons://running/", "return"};
    g_windowManager.ActivateWindow(WINDOW_ADDON_BROWSER, params);
  }
}

void CAddonSystemSettings::OnSettingChanged(const CSetting* setting)
{
  using namespace KODI::MESSAGING::HELPERS;

  if (setting->GetId() == CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES
    && CSettings::GetInstance().GetBool(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES)
    && ShowYesNoDialogText(19098, 36618) != DialogResponse::YES)
  {
    CSettings::GetInstance().SetBool(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES, false);
  }
}

bool CAddonSystemSettings::GetActive(const TYPE& type, AddonPtr& addon)
{
  std::string setting;
  switch (type)
  {
    case ADDON_VIZ:
      setting = CSettings::GetInstance().GetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION);
      break;
    case ADDON_SCREENSAVER:
      setting = CSettings::GetInstance().GetString(CSettings::SETTING_SCREENSAVER_MODE);
      break;
    case ADDON_SCRAPER_ALBUMS:
      setting = CSettings::GetInstance().GetString(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER);
      break;
    case ADDON_SCRAPER_ARTISTS:
      setting = CSettings::GetInstance().GetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER);
      break;
    case ADDON_SCRAPER_MOVIES:
      setting = CSettings::GetInstance().GetString(CSettings::SETTING_SCRAPERS_MOVIESDEFAULT);
      break;
    case ADDON_SCRAPER_MUSICVIDEOS:
      setting = CSettings::GetInstance().GetString(CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT);
      break;
    case ADDON_SCRAPER_TVSHOWS:
      setting = CSettings::GetInstance().GetString(CSettings::SETTING_SCRAPERS_TVSHOWSDEFAULT);
      break;
    case ADDON_WEB_INTERFACE:
      setting = CSettings::GetInstance().GetString(CSettings::SETTING_SERVICES_WEBSKIN);
      break;
    case ADDON_RESOURCE_LANGUAGE:
      setting = CSettings::GetInstance().GetString(CSettings::SETTING_LOCALE_LANGUAGE);
      break;
    default:
      return false;
  }
  return CAddonMgr::GetInstance().GetAddon(setting, addon, type);
}

bool CAddonSystemSettings::SetActive(const TYPE& type, const std::string& addonID)
{
  switch (type)
  {
    case ADDON_VIZ:
      CSettings::GetInstance().SetString(CSettings::SETTING_MUSICPLAYER_VISUALISATION, addonID);
      break;
    case ADDON_SCREENSAVER:
      CSettings::GetInstance().SetString(CSettings::SETTING_SCREENSAVER_MODE, addonID);
      break;
    case ADDON_SCRAPER_ALBUMS:
      CSettings::GetInstance().SetString(CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER, addonID);
      break;
    case ADDON_SCRAPER_ARTISTS:
      CSettings::GetInstance().SetString(CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER, addonID);
      break;
    case ADDON_SCRAPER_MOVIES:
      CSettings::GetInstance().SetString(CSettings::SETTING_SCRAPERS_MOVIESDEFAULT, addonID);
      break;
    case ADDON_SCRAPER_MUSICVIDEOS:
      CSettings::GetInstance().SetString(CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT, addonID);
      break;
    case ADDON_SCRAPER_TVSHOWS:
      CSettings::GetInstance().SetString(CSettings::SETTING_SCRAPERS_TVSHOWSDEFAULT, addonID);
      break;
    case ADDON_RESOURCE_LANGUAGE:
      CSettings::GetInstance().SetString(CSettings::SETTING_LOCALE_LANGUAGE, addonID);
      break;
    case ADDON_SCRIPT_WEATHER:
      CSettings::GetInstance().SetString(CSettings::SETTING_WEATHER_ADDON, addonID);
      break;
    case ADDON_SKIN:
      CSettings::GetInstance().SetString(CSettings::SETTING_LOOKANDFEEL_SKIN, addonID);
      break;
    case ADDON_RESOURCE_UISOUNDS:
      CSettings::GetInstance().SetString(CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN, addonID);
      break;
    default:
      return false;
  }

  return true;
}

}
