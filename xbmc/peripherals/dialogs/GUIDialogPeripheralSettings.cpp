/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPeripheralSettings.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "addons/Skin.h"
#include "dialogs/GUIDialogYesNo.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerManager.h"
#include "guilib/GUIMessage.h"
#include "peripherals/Peripherals.h"
#include "settings/SettingAddon.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <string_view>
#include <utility>

using namespace KODI;
using namespace PERIPHERALS;

// Settings for peripherals
constexpr std::string_view SETTING_APPEARANCE = "appearance";

CGUIDialogPeripheralSettings::CGUIDialogPeripheralSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PERIPHERAL_SETTINGS, "DialogSettings.xml"),
    m_item(NULL)
{
}

CGUIDialogPeripheralSettings::~CGUIDialogPeripheralSettings()
{
  if (m_item != NULL)
    delete m_item;

  m_settingsMap.clear();
}

bool CGUIDialogPeripheralSettings::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED &&
      message.GetSenderId() == CONTROL_SETTINGS_CUSTOM_BUTTON)
  {
    OnResetSettings();
    return true;
  }

  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogPeripheralSettings::RegisterPeripheralManager(CPeripherals& manager)
{
  m_manager = &manager;
}

void CGUIDialogPeripheralSettings::UnregisterPeripheralManager()
{
  m_manager = nullptr;
}

void CGUIDialogPeripheralSettings::SetFileItem(const CFileItem* item)
{
  if (item == NULL)
    return;

  if (m_item != NULL)
    delete m_item;

  m_item = new CFileItem(*item);
}

void CGUIDialogPeripheralSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string& settingId = setting->GetId();

  // we need to copy the new value of the setting from the copy to the
  // original setting
  std::map<std::string, std::shared_ptr<CSetting>>::iterator itSetting =
      m_settingsMap.find(settingId);
  if (itSetting == m_settingsMap.end())
    return;

  itSetting->second->FromString(setting->ToString());

  // Get peripheral associated with this setting
  PeripheralPtr peripheral;
  if (m_item != nullptr)
    peripheral = CServiceBroker::GetPeripherals().GetByPath(m_item->GetPath());

  if (!peripheral)
    return;

  if (settingId == SETTING_APPEARANCE)
  {
    // Get the controller profile of the new appearance
    GAME::ControllerPtr controller;

    if (setting->GetType() == SettingType::String)
    {
      std::shared_ptr<const CSettingString> settingString =
          std::static_pointer_cast<const CSettingString>(setting);
      const std::string& addonId = settingString->GetValue();

      if (m_manager != nullptr)
        controller = m_manager->GetControllerProfiles().GetController(addonId);
    }

    if (controller)
      peripheral->SetControllerProfile(controller);
  }
}

bool CGUIDialogPeripheralSettings::Save()
{
  if (m_item == NULL || m_initialising)
    return true;

  PeripheralPtr peripheral = CServiceBroker::GetPeripherals().GetByPath(m_item->GetPath());
  if (!peripheral)
    return true;

  peripheral->PersistSettings();

  return true;
}

void CGUIDialogPeripheralSettings::OnResetSettings()
{
  if (m_item == NULL)
    return;

  PeripheralPtr peripheral = CServiceBroker::GetPeripherals().GetByPath(m_item->GetPath());
  if (!peripheral)
    return;

  if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{10041}, CVariant{10042}))
    return;

  // reset the settings in the peripheral
  peripheral->ResetDefaultSettings();

  // re-create all settings and their controls
  SetupView();
}

void CGUIDialogPeripheralSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(m_item->GetLabel());
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CUSTOM_BUTTON, 409);
}

void CGUIDialogPeripheralSettings::InitializeSettings()
{
  if (m_item == NULL)
  {
    m_initialising = false;
    return;
  }

  m_initialising = true;
  bool usePopup = g_SkinInfo->HasSkinFile("DialogSlider.xml");

  PeripheralPtr peripheral = CServiceBroker::GetPeripherals().GetByPath(m_item->GetPath());
  if (!peripheral)
  {
    CLog::Log(LOGDEBUG, "{} - no peripheral", __FUNCTION__);
    m_initialising = false;
    return;
  }

  m_settingsMap.clear();
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("peripheralsettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPeripheralSettings: unable to setup settings");
    return;
  }

  const std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPeripheralSettings: unable to setup settings");
    return;
  }

  std::vector<SettingPtr> settings = peripheral->GetSettings();
  for (auto& setting : settings)
  {
    if (setting == NULL)
      continue;

    if (!setting->IsVisible())
    {
      CLog::Log(LOGDEBUG, "{} - invisible", __FUNCTION__);
      continue;
    }

    // we need to create a copy of the setting because the CSetting instances
    // are destroyed when leaving the dialog
    SettingPtr settingCopy;
    switch (setting->GetType())
    {
      case SettingType::Boolean:
      {
        std::shared_ptr<CSettingBool> settingBool = std::make_shared<CSettingBool>(
            setting->GetId(), *std::static_pointer_cast<CSettingBool>(setting));
        settingBool->SetControl(GetCheckmarkControl());

        settingCopy = std::static_pointer_cast<CSetting>(settingBool);
        break;
      }

      case SettingType::Integer:
      {
        std::shared_ptr<CSettingInt> settingInt = std::make_shared<CSettingInt>(
            setting->GetId(), *std::static_pointer_cast<CSettingInt>(setting));
        if (settingInt->GetTranslatableOptions().empty())
          settingInt->SetControl(GetSliderControl("integer", false, -1, usePopup, -1, "{:d}"));
        else
          settingInt->SetControl(GetSpinnerControl("string"));

        settingCopy = std::static_pointer_cast<CSetting>(settingInt);
        break;
      }

      case SettingType::Number:
      {
        std::shared_ptr<CSettingNumber> settingNumber = std::make_shared<CSettingNumber>(
            setting->GetId(), *std::static_pointer_cast<CSettingNumber>(setting));
        settingNumber->SetControl(GetSliderControl("number", false, -1, usePopup, -1, "{:2.2f}"));

        settingCopy = std::static_pointer_cast<CSetting>(settingNumber);
        break;
      }

      case SettingType::String:
      {
        if (auto settingAsAddon = std::dynamic_pointer_cast<const CSettingAddon>(setting))
        {
          std::shared_ptr<CSettingAddon> settingAddon =
              std::make_shared<CSettingAddon>(setting->GetId(), *settingAsAddon);

          // Control properties
          const std::string format = "addon";
          const bool delayed = false;
          const int heading = -1;
          const bool hideValue = false;
          const bool showInstalledAddons = true;
          const bool showInstallableAddons = true;
          const bool showMoreAddons = false;

          settingAddon->SetControl(GetButtonControl(format, delayed, heading, hideValue,
                                                    showInstalledAddons, showInstallableAddons,
                                                    showMoreAddons));

          GAME::ControllerPtr controller = peripheral->ControllerProfile();
          if (controller)
            settingAddon->SetValue(controller->ID());

          settingCopy = std::static_pointer_cast<CSetting>(settingAddon);
        }
        else
        {
          std::shared_ptr<CSettingString> settingString = std::make_shared<CSettingString>(
              setting->GetId(), *std::static_pointer_cast<CSettingString>(setting));
          settingString->SetControl(GetEditControl("string"));

          settingCopy = std::static_pointer_cast<CSetting>(settingString);
        }
        break;
      }

      default:
        //! @todo add more types if needed
        CLog::Log(LOGDEBUG, "{} - unknown type", __FUNCTION__);
        break;
    }

    if (settingCopy != NULL && settingCopy->GetControl() != NULL)
    {
      settingCopy->SetLevel(SettingLevel::Basic);
      group->AddSetting(settingCopy);
      m_settingsMap.insert(std::make_pair(setting->GetId(), setting));
    }
  }

  m_initialising = false;
}
