/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRClientPriorities.h"

#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cstdlib>
#include <memory>

using namespace PVR;

CGUIDialogPVRClientPriorities::CGUIDialogPVRClientPriorities() :
  CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_CLIENT_PRIORITIES, "DialogSettings.xml")
{
  m_loadType = LOAD_EVERY_TIME;
}

void CGUIDialogPVRClientPriorities::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  SetHeading(19240); // Client priorities
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186); // OK
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222); // Cancel
}

std::string CGUIDialogPVRClientPriorities::GetSettingsLabel(
    const std::shared_ptr<ISetting>& pSetting)
{
  int iClientId = std::atoi(pSetting->GetId().c_str());
  auto clientEntry = m_clients.find(iClientId);
  if (clientEntry != m_clients.end())
    return clientEntry->second->GetFullClientName();

  CLog::LogF(LOGERROR, "Unable to obtain pvr client with id '{}'", iClientId);
  return CGUIDialogSettingsManualBase::GetLocalizedString(13205); // Unknown
}

void CGUIDialogPVRClientPriorities::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("pvrclientpriorities", -1);
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

  m_clients = CServiceBroker::GetPVRManager().Clients()->GetCreatedClients();
  for (const auto& client : m_clients)
  {
    AddEdit(group, std::to_string(client.second->GetID()), 13205 /* Unknown */, SettingLevel::Basic,
            client.second->GetPriority());
  }
}

void CGUIDialogPVRClientPriorities::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  m_changedValues[setting->GetId()] = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
}

bool CGUIDialogPVRClientPriorities::Save()
{
  for (const auto& changedClient : m_changedValues)
  {
    int iClientId = std::atoi(changedClient.first.c_str());
    auto clientEntry = m_clients.find(iClientId);
    if (clientEntry != m_clients.end())
      clientEntry->second->SetPriority(changedClient.second);
  }

  return true;
}
