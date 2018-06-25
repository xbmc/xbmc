/*
 *      Copyright (C) 2017 Team Kodi
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

#include "GUIDialogPVRClientPriorities.h"
#include "guilib/GUIMessage.h"

#include <cstdlib>

#include "ServiceBroker.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"

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

std::string CGUIDialogPVRClientPriorities::GetSettingsLabel(std::shared_ptr<ISetting> pSetting)
{
  int iClientId = std::atoi(pSetting->GetId().c_str());
  auto clientEntry = m_clients.find(iClientId);
  if (clientEntry != m_clients.end())
    return clientEntry->second->GetFriendlyName();

  CLog::LogF(LOGERROR, "Unable to obtain pvr client with id '%i'", iClientId);
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

  std::shared_ptr<CSetting> setting = nullptr;

  CServiceBroker::GetPVRManager().Clients()->GetCreatedClients(m_clients);
  for (const auto& client : m_clients)
  {
    setting = AddEdit(group, StringUtils::Format("%i", client.second->GetID()), 13205 /* Unknown */, SettingLevel::Basic, client.second->GetPriority());
  }
}

void CGUIDialogPVRClientPriorities::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  m_changedValues[setting->GetId()] = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
}

void CGUIDialogPVRClientPriorities::Save()
{
  for (const auto& changedClient : m_changedValues)
  {
    int iClientId = std::atoi(changedClient.first.c_str());
    auto clientEntry = m_clients.find(iClientId);
    if (clientEntry != m_clients.end())
      clientEntry->second->SetPriority(changedClient.second);
  }
}
