/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRMediaTagSettings.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogHelper.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/media/PVRMediaTag.h"
#include "settings/dialogs/GUIDialogSettingsBase.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;
using namespace KODI::MESSAGING;

#define SETTING_MEDIA_TAG_NAME "mediatag.name"
#define SETTING_MEDIA_TAG_PLAYCOUNT "mediatag.playcount"

CGUIDialogPVRMediaTagSettings::CGUIDialogPVRMediaTagSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_MEDIA_TAG_SETTING, "DialogSettings.xml")
{
  m_loadType = LOAD_EVERY_TIME;
}

void CGUIDialogPVRMediaTagSettings::SetMediaTag(const std::shared_ptr<CPVRMediaTag>& mediaTag)
{
  if (!mediaTag)
  {
    CLog::LogF(LOGERROR, "No mediaTag given");
    return;
  }

  m_mediaTag = mediaTag;

  // Copy data we need from tag. Do not modify the tag itself until Save()!
  m_strTitle = m_mediaTag->m_strTitle;
  m_iPlayCount = m_mediaTag->GetLocalPlayCount();
}

void CGUIDialogPVRMediaTagSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(19068); // MediaTag settings
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186); // OK
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222); // Cancel
}

void CGUIDialogPVRMediaTagSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("pvrmediaettings", -1);
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
  const std::shared_ptr<CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(m_mediaTag->ClientID());

  // Play count
  if (client && client->GetClientCapabilities().SupportsMediaPlayCount())
    setting = AddEdit(group, SETTING_MEDIA_TAG_PLAYCOUNT, 567, SettingLevel::Basic,
                      m_mediaTag->GetLocalPlayCount());
}

bool CGUIDialogPVRMediaTagSettings::CanEditMediaTag(const CFileItem& item)
{
  if (!item.HasPVRMediaInfoTag())
    return false;

  const std::shared_ptr<CPVRClient> client =
      CServiceBroker::GetPVRManager().GetClient(item.GetPVRMediaInfoTag()->ClientID());

  if (!client)
    return false;

  const CPVRClientCapabilities& capabilities = client->GetClientCapabilities();

  return capabilities.SupportsMediaPlayCount();
}

bool CGUIDialogPVRMediaTagSettings::OnSettingChanging(
    const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "No setting");
    return false;
  }

  return CGUIDialogSettingsManualBase::OnSettingChanging(setting);
}

void CGUIDialogPVRMediaTagSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
  {
    CLog::LogF(LOGERROR, "No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string& settingId = setting->GetId();

  if (settingId == SETTING_MEDIA_TAG_NAME)
  {
    m_strTitle = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  }
  else if (settingId == SETTING_MEDIA_TAG_PLAYCOUNT)
  {
    m_iPlayCount = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
}

bool CGUIDialogPVRMediaTagSettings::Save()
{
  // Name
  m_mediaTag->m_strTitle = m_strTitle;

  // Play count
  m_mediaTag->SetLocalPlayCount(m_iPlayCount);

  return true;
}
