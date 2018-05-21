#pragma once
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

#include "settings/dialogs/GUIDialogSettingsBase.h"

class CSettingsManager;

class CGUIDialogSettingsManagerBase : public CGUIDialogSettingsBase
{
public:
  CGUIDialogSettingsManagerBase(int windowId, const std::string &xmlFile);
  ~CGUIDialogSettingsManagerBase() override;

protected:
  virtual void Save() = 0;
  virtual CSettingsManager* GetSettingsManager() const = 0;

  // implementation of CGUIDialogSettingsBase
  std::shared_ptr<CSetting> GetSetting(const std::string &settingId) override;
  void OnOkay() override;

  std::set<std::string> CreateSettings() override;
  void FreeSettingsControls() override;

  // implementation of ISettingControlCreator
  std::shared_ptr<ISettingControl> CreateControl(const std::string &controlType) const override;
};
