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
  virtual ~CGUIDialogPeripheralSettings();

  // specializations of CGUIControl
  virtual bool OnMessage(CGUIMessage &message);

  virtual void SetFileItem(const CFileItem *item);

protected:
  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save();
  virtual void OnResetSettings();
  virtual void SetupView();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  CFileItem *m_item;
  bool m_initialising;
  std::map<std::string, CSetting*> m_settingsMap;
};
}
