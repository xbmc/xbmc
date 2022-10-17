/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonSystemSettings.h"

#include "ServiceBroker.h"
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

namespace ADDON
{

CAddonSystemSettings::CAddonSystemSettings()
  : m_activeSettings{
        {AddonType::AUDIOENCODER, CSettings::SETTING_AUDIOCDS_ENCODER},
        {AddonType::RESOURCE_LANGUAGE, CSettings::SETTING_LOCALE_LANGUAGE},
        {AddonType::RESOURCE_UISOUNDS, CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN},
        {AddonType::SCRAPER_ALBUMS, CSettings::SETTING_MUSICLIBRARY_ALBUMSSCRAPER},
        {AddonType::SCRAPER_ARTISTS, CSettings::SETTING_MUSICLIBRARY_ARTISTSSCRAPER},
        {AddonType::SCRAPER_MOVIES, CSettings::SETTING_SCRAPERS_MOVIESDEFAULT},
        {AddonType::SCRAPER_MUSICVIDEOS, CSettings::SETTING_SCRAPERS_MUSICVIDEOSDEFAULT},
        {AddonType::SCRAPER_TVSHOWS, CSettings::SETTING_SCRAPERS_TVSHOWSDEFAULT},
        {AddonType::SCREENSAVER, CSettings::SETTING_SCREENSAVER_MODE},
        {AddonType::SCRIPT_WEATHER, CSettings::SETTING_WEATHER_ADDON},
        {AddonType::SKIN, CSettings::SETTING_LOOKANDFEEL_SKIN},
        {AddonType::WEB_INTERFACE, CSettings::SETTING_SERVICES_WEBSKIN},
        {AddonType::VISUALIZATION, CSettings::SETTING_MUSICPLAYER_VISUALISATION},
    }
{}

CAddonSystemSettings& CAddonSystemSettings::GetInstance()
{
  static CAddonSystemSettings inst;
  return inst;
}

void CAddonSystemSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting->GetId() == CSettings::SETTING_ADDONS_MANAGE_DEPENDENCIES)
  {
    std::vector<std::string> params{"addons://dependencies/", "return"};
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_ADDON_BROWSER, params);
  }
  else if (setting->GetId() == CSettings::SETTING_ADDONS_SHOW_RUNNING)
  {
    std::vector<std::string> params{"addons://running/", "return"};
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_ADDON_BROWSER, params);
  }
  else if (setting->GetId() == CSettings::SETTING_ADDONS_REMOVE_ORPHANED_DEPENDENCIES)
  {
    using namespace KODI::MESSAGING::HELPERS;

    const auto removedItems = CAddonInstaller::GetInstance().RemoveOrphanedDepsRecursively();
    if (removedItems.size() > 0)
    {
      const auto message =
          StringUtils::Format(g_localizeStrings.Get(36641), StringUtils::Join(removedItems, ", "));

      ShowOKDialogText(CVariant{36640}, CVariant{message}); // "following orphaned were removed..."
    }
    else
    {
      ShowOKDialogText(CVariant{36640}, CVariant{36642}); // "no orphaned found / removed"
    }
  }
}

void CAddonSystemSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  using namespace KODI::MESSAGING::HELPERS;

  if (setting->GetId() == CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES &&
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES) &&
      ShowYesNoDialogText(19098, 36618) != DialogResponse::CHOICE_YES)
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetBool(CSettings::SETTING_ADDONS_ALLOW_UNKNOWN_SOURCES, false);
  }
}

bool CAddonSystemSettings::GetActive(AddonType type, AddonPtr& addon)
{
  auto it = m_activeSettings.find(type);
  if (it != m_activeSettings.end())
  {
    auto settingValue = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(it->second);
    return CServiceBroker::GetAddonMgr().GetAddon(settingValue, addon, type,
                                                  OnlyEnabled::CHOICE_YES);
  }
  return false;
}

bool CAddonSystemSettings::SetActive(AddonType type, const std::string& addonID)
{
  auto it = m_activeSettings.find(type);
  if (it != m_activeSettings.end())
  {
    CServiceBroker::GetSettingsComponent()->GetSettings()->SetString(it->second, addonID);
    return true;
  }
  return false;
}

bool CAddonSystemSettings::IsActive(const IAddon& addon)
{
  AddonPtr active;
  return GetActive(addon.Type(), active) && active->ID() == addon.ID();
}

bool CAddonSystemSettings::UnsetActive(const AddonInfoPtr& addon)
{
  auto it = m_activeSettings.find(addon->MainType());
  if (it == m_activeSettings.end())
    return true;

  auto setting = std::static_pointer_cast<CSettingString>(CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(it->second));
  if (setting->GetValue() != addon->ID())
    return true;

  if (setting->GetDefault() == addon->ID())
    return false; // Cant unset defaults

  setting->Reset();
  return true;
}

int CAddonSystemSettings::GetAddonAutoUpdateMode() const
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_ADDONS_AUTOUPDATES);
}

AddonRepoUpdateMode CAddonSystemSettings::GetAddonRepoUpdateMode() const
{
  const int updateMode = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_ADDONS_UPDATEMODE);
  return static_cast<AddonRepoUpdateMode>(updateMode);
}
}
