/*
 *      Copyright (C) 2014 Team XBMC
 *      http://www.xbmc.org
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

#include <set>
#include <string>

#include "GUIDialogSettingsManagerBase.h"
#include "settings/lib/SettingsManager.h"

CGUIDialogSettingsManagerBase::CGUIDialogSettingsManagerBase(int windowId, const std::string &xmlFile)
    : CGUIDialogSettingsBase(windowId, xmlFile)
{ }

CGUIDialogSettingsManagerBase::~CGUIDialogSettingsManagerBase()
{ }

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
