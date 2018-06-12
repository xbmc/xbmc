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

#pragma once

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
