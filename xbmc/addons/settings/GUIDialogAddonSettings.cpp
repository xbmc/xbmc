/*
 *      Copyright (C) 2005-2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogAddonSettings.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "addons/settings/AddonSettings.h"
#include "dialogs/GUIDialogOK.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "guilib/LocalizeStrings.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

CGUIDialogAddonSettings::CGUIDialogAddonSettings()
  : CGUIDialogSettingsManagerBase(WINDOW_DIALOG_ADDON_SETTINGS, "DialogAddonSettings.xml")
{ }

bool CGUIDialogAddonSettings::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_SETTINGS_CUSTOM_BUTTON)
      {
        OnResetSettings();
        return true;
      }
      break;
    }

    case GUI_MSG_SETTING_UPDATED:
    {
      std::string settingId = message.GetStringParam(0);
      std::string settingValue = message.GetStringParam(1);

      std::shared_ptr<CSetting> setting = GetSettingsManager()->GetSetting(settingId);
      if (setting != nullptr)
      {
        setting->FromString(settingValue);
        return true;
      }
      break;
    }

    default:
      break;
  }

  return CGUIDialogSettingsManagerBase::OnMessage(message);
}

bool CGUIDialogAddonSettings::ShowForAddon(const ADDON::AddonPtr &addon, bool saveToDisk /* = true */)
{
  if (addon == nullptr)
    return false;

  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return false;

  if (!addon->HasSettings())
  {
    // addon does not support settings, inform user
    CGUIDialogOK::ShowAndGetInput(CVariant{ 24000 }, CVariant{ 24030 });
    return false;
  }

  // Create the dialog
  CGUIDialogAddonSettings* dialog = g_windowManager.GetWindow<CGUIDialogAddonSettings>(WINDOW_DIALOG_ADDON_SETTINGS);
  if (dialog == nullptr)
    return false;

  dialog->m_addon = addon;
  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  if (saveToDisk)
    addon->SaveSettings();

  return true;
}

std::string CGUIDialogAddonSettings::GetCurrentAddonID() const
{
  if (m_addon == nullptr)
    return "";

  return m_addon->ID();
}

void CGUIDialogAddonSettings::SetupView()
{
  if (m_addon == nullptr || m_addon->GetSettings() == nullptr)
    return;

  auto settings = m_addon->GetSettings();
  if (!settings->IsLoaded())
    return;

  CGUIDialogSettingsManagerBase::SetupView();

  // set heading
  SetHeading(StringUtils::Format("$LOCALIZE[10004] - %s", m_addon->Name().c_str())); // "Settings - AddonName"

  // set control labels
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CUSTOM_BUTTON, 409);
}

std::string CGUIDialogAddonSettings::GetLocalizedString(uint32_t labelId) const
{
  std::string label = g_localizeStrings.GetAddonString(m_addon->ID(), labelId);
  if (!label.empty())
    return label;

  return CGUIDialogSettingsManagerBase::GetLocalizedString(labelId);
}

std::string CGUIDialogAddonSettings::GetSettingsLabel(std::shared_ptr<CSetting> setting)
{
  if (setting == nullptr)
    return "";

  std::string label = GetLocalizedString(setting->GetLabel());
  if (!label.empty())
    return label;

  // try the addon settings
  label = m_addon->GetSettings()->GetSettingLabel(setting->GetLabel());
  if (!label.empty())
    return label;

  return CGUIDialogSettingsManagerBase::GetSettingsLabel(setting);
}

int CGUIDialogAddonSettings::GetSettingLevel() const
{
  return static_cast<int>(SettingLevelStandard);
}

std::shared_ptr<CSettingSection> CGUIDialogAddonSettings::GetSection()
{
  const auto settingsManager = GetSettingsManager();
  if (settingsManager == nullptr)
    return nullptr;

  const auto sections = settingsManager->GetSections();
  if (!sections.empty())
    return sections.front();

  return nullptr;
}

CSettingsManager* CGUIDialogAddonSettings::GetSettingsManager() const
{
  if (m_addon == nullptr || m_addon->GetSettings() == nullptr)
    return nullptr;

  return m_addon->GetSettings()->GetSettingsManager();
}
