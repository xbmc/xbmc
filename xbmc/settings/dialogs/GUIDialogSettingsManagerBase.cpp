/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSettingsManagerBase.h"

#include "settings/lib/SettingsManager.h"

#include <cassert>

CGUIDialogSettingsManagerBase::CGUIDialogSettingsManagerBase(int windowId, const std::string &xmlFile)
    : CGUIDialogSettingsBase(windowId, xmlFile)
{ }

CGUIDialogSettingsManagerBase::~CGUIDialogSettingsManagerBase() = default;

std::shared_ptr<CSetting> CGUIDialogSettingsManagerBase::GetSetting(const std::string &settingId)
{
  assert(GetSettingsManager() != nullptr);

  return GetSettingsManager()->GetSetting(settingId);
}

void CGUIDialogSettingsManagerBase::OnOkay()
{
  Save();

  CGUIDialogSettingsBase::OnOkay();
}

std::set<std::string> CGUIDialogSettingsManagerBase::CreateSettings()
{
  assert(GetSettingsManager() != nullptr);

  std::set<std::string> settings = CGUIDialogSettingsBase::CreateSettings();

  if (!settings.empty())
    GetSettingsManager()->RegisterCallback(this, settings);

  return settings;
}

void CGUIDialogSettingsManagerBase::FreeSettingsControls()
{
  CGUIDialogSettingsBase::FreeSettingsControls();

  if (GetSettingsManager() != nullptr)
    GetSettingsManager()->UnregisterCallback(this);
}

std::shared_ptr<ISettingControl> CGUIDialogSettingsManagerBase::CreateControl(const std::string &controlType) const
{
  assert(GetSettingsManager() != nullptr);

  return GetSettingsManager()->CreateControl(controlType);
}
