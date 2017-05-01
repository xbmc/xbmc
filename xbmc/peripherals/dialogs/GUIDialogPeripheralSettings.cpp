/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "GUIDialogPeripheralSettings.h"

#include <utility>

#include "addons/Skin.h"
#include "dialogs/GUIDialogYesNo.h"
#include "FileItem.h"
#include "peripherals/Peripherals.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingSection.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "ServiceBroker.h"

using namespace PERIPHERALS;

CGUIDialogPeripheralSettings::CGUIDialogPeripheralSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PERIPHERAL_SETTINGS, "DialogSettings.xml"),
    m_item(NULL),
    m_initialising(false)
{ }

CGUIDialogPeripheralSettings::~CGUIDialogPeripheralSettings()
{
  if (m_item != NULL)
    delete m_item;

  m_settingsMap.clear();
}

bool CGUIDialogPeripheralSettings::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED &&
      message.GetSenderId() == CONTROL_SETTINGS_CUSTOM_BUTTON)
  {
    OnResetSettings();
    return true;
  }

  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogPeripheralSettings::SetFileItem(const CFileItem *item)
{
  if (item == NULL)
    return;

  if (m_item != NULL)
    delete m_item;

  m_item = new CFileItem(*item);
}

void CGUIDialogPeripheralSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  // we need to copy the new value of the setting from the copy to the
  // original setting
  std::map<std::string, std::shared_ptr<CSetting>>::iterator itSetting = m_settingsMap.find(setting->GetId());
  if (itSetting == m_settingsMap.end())
    return;

  itSetting->second->FromString(setting->ToString());
}

void CGUIDialogPeripheralSettings::Save()
{
  if (m_item == NULL || m_initialising)
    return;

  PeripheralPtr peripheral = CServiceBroker::GetPeripherals().GetByPath(m_item->GetPath());
  if (!peripheral)
    return;

  peripheral->PersistSettings();
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
    CLog::Log(LOGDEBUG, "%s - no peripheral", __FUNCTION__);
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
  for (std::vector<SettingPtr>::iterator itSetting = settings.begin(); itSetting != settings.end(); ++itSetting)
  {
    SettingPtr setting = *itSetting;
    if (setting == NULL)
      continue;

    if (!setting->IsVisible())
    {
      CLog::Log(LOGDEBUG, "%s - invisible", __FUNCTION__);
      continue;
    }

    // we need to create a copy of the setting because the CSetting instances
    // are destroyed when leaving the dialog
    SettingPtr settingCopy;
    switch(setting->GetType())
    {
      case SettingTypeBool:
      {
        std::shared_ptr<CSettingBool> settingBool = std::make_shared<CSettingBool>(setting->GetId(), *std::static_pointer_cast<CSettingBool>(setting));
        settingBool->SetControl(GetCheckmarkControl());

        settingCopy = std::static_pointer_cast<CSetting>(settingBool);
        break;
      }

      case SettingTypeInteger:
      {
        std::shared_ptr<CSettingInt> settingInt = std::make_shared<CSettingInt>(setting->GetId(), *std::static_pointer_cast<CSettingInt>(setting));
        if (settingInt->GetTranslatableOptions().empty())
          settingInt->SetControl(GetSliderControl("integer", false, -1, usePopup, -1, "%i"));
        else
          settingInt->SetControl(GetSpinnerControl("string"));

        settingCopy = std::static_pointer_cast<CSetting>(settingInt);
        break;
      }

      case SettingTypeNumber:
      {
        std::shared_ptr<CSettingNumber> settingNumber = std::make_shared<CSettingNumber>(setting->GetId(), *std::static_pointer_cast<CSettingNumber>(setting));
        settingNumber->SetControl(GetSliderControl("number", false, -1, usePopup, -1, "%2.2f"));

        settingCopy = std::static_pointer_cast<CSetting>(settingNumber);
        break;
      }

      case SettingTypeString:
      {
        std::shared_ptr<CSettingString> settingString = std::make_shared<CSettingString>(setting->GetId(), *std::static_pointer_cast<CSettingString>(setting));
        settingString->SetControl(GetEditControl("string"));

        settingCopy = std::static_pointer_cast<CSetting>(settingString);
        break;
      }

      default:
        //! @todo add more types if needed
        CLog::Log(LOGDEBUG, "%s - unknown type", __FUNCTION__);
        break;
    }

    if (settingCopy != NULL && settingCopy->GetControl() != NULL)
    {
      settingCopy->SetLevel(SettingLevelBasic);
      group->AddSetting(settingCopy);
      m_settingsMap.insert(std::make_pair(setting->GetId(), setting));
    }
  }

  m_initialising = false;
}
