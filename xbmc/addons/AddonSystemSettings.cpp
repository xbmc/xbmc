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

static const std::map<ADDON::TYPE, std::string> settingMap = {
    {ADDON_VIZ, CSettings::SETTING_MUSICPLAYER_VISUALISATION},
    {ADDON_SCREENSAVER, CSettings::SETTING_SCREENSAVER_MODE},
    {ADDON_SCRAPER_ALBUMS, CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER},
    {ADDON_SCRAPER_ARTISTS, CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER},
    {ADDON_SCRAPER_MOVIES, CSettings::SETTING_SCRAPERS_MOVIESDEFAULT},
    {ADDON_SCRAPER_MUSICVIDEOS, CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT},
    {ADDON_SCRAPER_TVSHOWS, CSettings::SETTING_SCRAPERS_TVSHOWSDEFAULT},
    {ADDON_WEB_INTERFACE, CSettings::SETTING_SERVICES_WEBSKIN},
    {ADDON_RESOURCE_LANGUAGE, CSettings::SETTING_LOCALE_LANGUAGE},
    {ADDON_SCRIPT_WEATHER, CSettings::SETTING_WEATHER_ADDON},
    {ADDON_SKIN, CSettings::SETTING_LOOKANDFEEL_SKIN},
    {ADDON_RESOURCE_UISOUNDS, CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN},
};

bool CAddonSystemSettings::GetActive(const TYPE& type, AddonPtr& addon)
{
  auto it = settingMap.find(type);
  if (it != settingMap.end())
  {
    auto settingValue = CSettings::GetInstance().GetString(it->second);
    return CAddonMgr::GetInstance().GetAddon(settingValue, addon, type);
  }
  return false;
}

bool CAddonSystemSettings::SetActive(const TYPE& type, const std::string& addonID)
{
  auto it = settingMap.find(type);
  if (it != settingMap.end())
  {
    CSettings::GetInstance().SetString(it->second, addonID);
    return true;
  }
  return false;
}

bool CAddonSystemSettings::IsActive(const IAddon& addon)
{
  AddonPtr active;
  return GetActive(addon.Type(), active) && active->ID() == addon.ID();
}

bool CAddonSystemSettings::UnsetActive(const AddonPtr& addon)
{
  auto it = settingMap.find(addon->Type());
  if (it == settingMap.end())
    return true;

  auto setting = static_cast<CSettingString*>(CSettings::GetInstance().GetSetting(it->second));
  if (setting->GetValue() != addon->ID())
    return true;

  if (setting->GetDefault() == addon->ID())
    return false; // Cant unset defaults

  setting->Reset();
  return true;
}

}
