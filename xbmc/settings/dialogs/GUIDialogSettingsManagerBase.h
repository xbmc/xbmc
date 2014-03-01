#pragma once
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

#include "settings/dialogs/GUIDialogSettingsBase.h"

class CSettingsManager;

class CGUIDialogSettingsManagerBase : public CGUIDialogSettingsBase
{
public:
  CGUIDialogSettingsManagerBase(int windowId, const std::string &xmlFile);
  virtual ~CGUIDialogSettingsManagerBase();

protected:
  // implementation of CGUIDialogSettingsBase
  virtual CSetting* GetSetting(const std::string &settingId);

  virtual std::set<std::string> CreateSettings();
  virtual void FreeSettingsControls();

  // implementation of ISettingControlCreator
  virtual ISettingControl* CreateControl(const std::string &controlType);

  CSettingsManager *m_settingsManager;
};
