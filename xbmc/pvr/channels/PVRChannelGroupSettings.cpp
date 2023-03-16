/*
 *  Copyright (C) 2012-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupSettings.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "settings/Settings.h"
#include "settings/lib/Setting.h"

using namespace PVR;

CPVRChannelGroupSettings::CPVRChannelGroupSettings()
  : m_settings({CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER,
                CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS,
                CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERSALWAYS,
                CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE})
{
  UpdateUseBackendChannelOrder();
  UpdateUseBackendChannelNumbers();
  UpdateStartGroupChannelNumbersFromOne();

  m_settings.RegisterCallback(this);
}

CPVRChannelGroupSettings::~CPVRChannelGroupSettings()
{
  m_settings.UnregisterCallback(this);
}

void CPVRChannelGroupSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (!setting)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER)
  {
    if (UseBackendChannelOrder() != UpdateUseBackendChannelOrder())
    {
      for (const auto& callback : m_callbacks)
        callback->UseBackendChannelOrderChanged();
    }
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS ||
           settingId == CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERSALWAYS)
  {
    if (UseBackendChannelNumbers() != UpdateUseBackendChannelNumbers())
    {
      for (const auto& callback : m_callbacks)
        callback->UseBackendChannelNumbersChanged();
    }
  }
  else if (settingId == CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE)
  {
    if (StartGroupChannelNumbersFromOne() != UpdateStartGroupChannelNumbersFromOne())
    {
      for (const auto& callback : m_callbacks)
        callback->StartGroupChannelNumbersFromOneChanged();
    }
  }
}

void CPVRChannelGroupSettings::RegisterCallback(IChannelGroupSettingsCallback* callback)
{
  m_callbacks.insert(callback);
}

void CPVRChannelGroupSettings::UnregisterCallback(IChannelGroupSettingsCallback* callback)
{
  m_callbacks.erase(callback);
}

bool CPVRChannelGroupSettings::UpdateUseBackendChannelOrder()
{
  m_bUseBackendChannelOrder =
      m_settings.GetBoolValue(CSettings::SETTING_PVRMANAGER_BACKENDCHANNELORDER);
  return m_bUseBackendChannelOrder;
}

bool CPVRChannelGroupSettings::UpdateUseBackendChannelNumbers()
{
  const int enabledClientAmount = CServiceBroker::GetPVRManager().Clients()->EnabledClientAmount();
  m_bUseBackendChannelNumbers =
      m_settings.GetBoolValue(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERS) &&
      (enabledClientAmount == 1 ||
       (m_settings.GetBoolValue(CSettings::SETTING_PVRMANAGER_USEBACKENDCHANNELNUMBERSALWAYS) &&
        enabledClientAmount > 1));
  return m_bUseBackendChannelNumbers;
}

bool CPVRChannelGroupSettings::UpdateStartGroupChannelNumbersFromOne()
{
  m_bStartGroupChannelNumbersFromOne =
      m_settings.GetBoolValue(CSettings::SETTING_PVRMANAGER_STARTGROUPCHANNELNUMBERSFROMONE) &&
      !UseBackendChannelNumbers();
  return m_bStartGroupChannelNumbersFromOne;
}
