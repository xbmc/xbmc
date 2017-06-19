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

#include "AddonSystemSettings.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/AddonInstaller.h"
#include "addons/RepositoryUpdater.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/Settings.h"
#include "utils/log.h"


namespace ADDON
{

CAddonSystemSettings::CAddonSystemSettings() :
  m_activeSettings{
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
  }
{}

CAddonSystemSettings& CAddonSystemSettings::GetInstance()
{
  static CAddonSystemSettings inst;
  return inst;
}

void CAddonSystemSettings::OnSettingAction(std::shared_ptr<const CSetting> setting)
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

void CAddonSystemSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  using namespace KODI::MESSAGING::HELPERS;

  if (setting->GetId() == CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES
    && CServiceBroker::GetSettings().GetBool(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES)
    && ShowYesNoDialogText(19098, 36618) != DialogResponse::YES)
  {
    CServiceBroker::GetSettings().SetBool(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES, false);
  }
}

bool CAddonSystemSettings::GetActive(const TYPE& type, AddonPtr& addon)
{
  auto it = m_activeSettings.find(type);
  if (it != m_activeSettings.end())
  {
    auto settingValue = CServiceBroker::GetSettings().GetString(it->second);
    return CAddonMgr::GetInstance().GetAddon(settingValue, addon, type);
  }
  return false;
}

bool CAddonSystemSettings::SetActive(const TYPE& type, const std::string& addonID)
{
  auto it = m_activeSettings.find(type);
  if (it != m_activeSettings.end())
  {
    CServiceBroker::GetSettings().SetString(it->second, addonID);
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
  auto it = m_activeSettings.find(addon->Type());
  if (it == m_activeSettings.end())
    return true;

  auto setting = std::static_pointer_cast<CSettingString>(CServiceBroker::GetSettings().GetSetting(it->second));
  if (setting->GetValue() != addon->ID())
    return true;

  if (setting->GetDefault() == addon->ID())
    return false; // Cant unset defaults

  setting->Reset();
  return true;
}


std::vector<std::string> CAddonSystemSettings::MigrateAddons(std::function<void(void)> onMigrate)
{
  auto getIncompatible = [](){
    VECADDONS incompatible;
    CAddonMgr::GetInstance().GetAddons(incompatible);
    incompatible.erase(std::remove_if(incompatible.begin(), incompatible.end(),
        [](const AddonPtr a){ return CAddonMgr::GetInstance().IsCompatible(*a); }), incompatible.end());
    return incompatible;
  };

  if (getIncompatible().empty())
    return std::vector<std::string>();

  if (CServiceBroker::GetSettings().GetInt(CSettings::SETTING_ADDONS_AUTOUPDATES) == AUTO_UPDATES_ON)
  {
    onMigrate();

    if (CRepositoryUpdater::GetInstance().CheckForUpdates())
      CRepositoryUpdater::GetInstance().Await();

    CLog::Log(LOGINFO, "ADDON: waiting for add-ons to update...");
    CAddonInstaller::GetInstance().InstallUpdatesAndWait();
  }

  auto incompatible = getIncompatible();
  for (const auto& addon : incompatible)
    CLog::Log(LOGNOTICE, "ADDON: %s version %s is incompatible", addon->ID().c_str(), addon->Version().asString().c_str());

  std::vector<std::string> changed;
  for (const auto& addon : incompatible)
  {
    if (!UnsetActive(addon))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to unset %s", addon->ID().c_str());
      continue;
    }
    if (!CAddonMgr::GetInstance().DisableAddon(addon->ID()))
    {
      CLog::Log(LOGWARNING, "ADDON: failed to disable %s", addon->ID().c_str());
    }
    changed.push_back(addon->Name());
  }

  return changed;
}
}
