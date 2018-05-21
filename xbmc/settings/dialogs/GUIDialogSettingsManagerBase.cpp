/*
 *      Copyright (C) 2014-present Team Kodi
 *      http://www.xbmc.org
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <set>
#include <string>

#include "GUIDialogSettingsManagerBase.h"
#include "settings/lib/SettingsManager.h"

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
