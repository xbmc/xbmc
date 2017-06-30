#pragma once
/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
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

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CFileItem;

namespace PERIPHERALS
{
class CGUIDialogPeripheralSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogPeripheralSettings();
  ~CGUIDialogPeripheralSettings() override;

  // specializations of CGUIControl
  bool OnMessage(CGUIMessage &message) override;

  virtual void SetFileItem(const CFileItem *item);

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  void Save() override;
  void OnResetSettings() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

  CFileItem *m_item;
  bool m_initialising;
  std::map<std::string, std::shared_ptr<CSetting>> m_settingsMap;
};
}
