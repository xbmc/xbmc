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

class CGUIDialogCMSSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogCMSSettings();
  virtual ~CGUIDialogCMSSettings();

protected:
  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting) override;

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const override { return false; }
  virtual bool OnBack(int actionID) override;
  virtual void Save() override;
  virtual void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings() override;

private:
  bool m_viewModeChanged;
  static void Cms3dLutsFiller(
    const CSetting *setting,
    std::vector< std::pair<std::string, std::string> > &list,
    std::string &current,
    void *data);
};
