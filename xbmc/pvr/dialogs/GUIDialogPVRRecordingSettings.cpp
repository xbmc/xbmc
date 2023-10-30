/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRRecordingSettings.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/recordings/PVRRecording.h"
#include "settings/dialogs/GUIDialogSettingsBase.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;
using namespace KODI::MESSAGING;

#define SETTING_RECORDING_NAME "recording.name"
#define SETTING_RECORDING_PLAYCOUNT "recording.playcount"
#define SETTING_RECORDING_LIFETIME "recording.lifetime"

CGUIDialogPVRRecordingSettings::CGUIDialogPVRRecordingSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_RECORDING_SETTING, "DialogSettings.xml")
{
  m_loadType = LOAD_EVERY_TIME;
}

void CGUIDialogPVRRecordingSettings::SetRecording(const std::shared_ptr<CPVRRecording>& recording)
{
  if (!recording)
  {
    CLog::LogF(LOGERROR, "No recording given");
    return;
  }

  m_recording = recording;

  // Copy data we need from tag. Do not modify the tag itself until Save()!
  m_strTitle = m_recording->m_strTitle;
  m_iPlayCount = m_recording->GetLocalPlayCount();
  m_iLifetime = m_recording->LifeTime();
}

void CGUIDialogPVRRecordingSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(19068); // Recording settings
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186); // OK
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222); // Cancel
}

void CGUIDialogPVRRecordingSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("pvrrecordingsettings", -1);
  if (category == nullptr)
  {
    CLog::LogF(LOGERROR, "Unable to add settings category");
    return;
  }

  const std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == nullptr)
  {
    CLog::LogF(LOGERROR, "Unable to add settings group");
    return;
  }

  std::shared_ptr<CSetting> setting = nullptr;
  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_recording->ClientID());

  // Name
  setting = AddEdit(group, SETTING_RECORDING_NAME, 19075, SettingLevel::Basic, m_strTitle);
  setting->SetEnabled(client && client->GetClientCapabilities().SupportsRecordingsRename());

  // Play count
  if (client && client->GetClientCapabilities().SupportsRecordingsPlayCount())
    setting = AddEdit(group, SETTING_RECORDING_PLAYCOUNT, 567, SettingLevel::Basic,
                      m_recording->GetLocalPlayCount());

  // Lifetime
  if (client && client->GetClientCapabilities().SupportsRecordingsLifetimeChange())
    setting = AddList(group, SETTING_RECORDING_LIFETIME, 19083, SettingLevel::Basic, m_iLifetime,
                      LifetimesFiller, 19083);
}

bool CGUIDialogPVRRecordingSettings::CanEditRecording(const CFileItem& item)
{
  if (!item.HasPVRRecordingInfoTag())
    return false;

  const std::shared_ptr<const CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(item.GetPVRRecordingInfoTag()->ClientID());

  if (!client)
    return false;

  const CPVRClientCapabilities& capabilities = client->GetClientCapabilities();

  return capabilities.SupportsRecordingsRename() || capabilities.SupportsRecordingsPlayCount() ||
         capabilities.SupportsRecordingsLifetimeChange();
}

bool CGUIDialogPVRRecordingSettings::OnSettingChanging(
    const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "No setting");
    return false;
  }

  const std::string& settingId = setting->GetId();

  if (settingId == SETTING_RECORDING_LIFETIME)
  {
    int iNewLifetime = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    if (m_recording->WillBeExpiredWithNewLifetime(iNewLifetime))
    {
      if (HELPERS::ShowYesNoDialogText(
              CVariant{19068}, // "Recording settings"
              StringUtils::Format(g_localizeStrings.Get(19147),
                                  iNewLifetime)) // "Setting the lifetime..."
          != HELPERS::DialogResponse::CHOICE_YES)
        return false;
    }
  }

  return CGUIDialogSettingsManualBase::OnSettingChanging(setting);
}

void CGUIDialogPVRRecordingSettings::OnSettingChanged(
    const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string& settingId = setting->GetId();

  if (settingId == SETTING_RECORDING_NAME)
  {
    m_strTitle = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  }
  else if (settingId == SETTING_RECORDING_PLAYCOUNT)
  {
    m_iPlayCount = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
  else if (settingId == SETTING_RECORDING_LIFETIME)
  {
    m_iLifetime = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
}

bool CGUIDialogPVRRecordingSettings::Save()
{
  // Name
  m_recording->m_strTitle = m_strTitle;

  // Play count
  m_recording->SetLocalPlayCount(m_iPlayCount);

  // Lifetime
  m_recording->SetLifeTime(m_iLifetime);

  return true;
}

void CGUIDialogPVRRecordingSettings::LifetimesFiller(const SettingConstPtr& setting,
                                                     std::vector<IntegerSettingOption>& list,
                                                     int& current,
                                                     void* data)
{
  CGUIDialogPVRRecordingSettings* pThis = static_cast<CGUIDialogPVRRecordingSettings*>(data);
  if (pThis)
  {
    list.clear();

    const std::shared_ptr<const CPVRClient> client =
        CServiceBroker::GetPVRManager().GetClient(pThis->m_recording->ClientID());
    if (client)
    {
      std::vector<std::pair<std::string, int>> values;
      client->GetClientCapabilities().GetRecordingsLifetimeValues(values);
      std::transform(
          values.cbegin(), values.cend(), std::back_inserter(list),
          [](const auto& value) { return IntegerSettingOption(value.first, value.second); });
    }

    current = pThis->m_iLifetime;

    auto it = list.begin();
    while (it != list.end())
    {
      if (it->value == current)
        break; // value already in list

      ++it;
    }

    if (it == list.end())
    {
      // PVR backend supplied value is not in the list of predefined values. Insert it.
      list.insert(it, IntegerSettingOption(
                          StringUtils::Format(g_localizeStrings.Get(17999), current) /* {} days */,
                          current));
    }
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}
