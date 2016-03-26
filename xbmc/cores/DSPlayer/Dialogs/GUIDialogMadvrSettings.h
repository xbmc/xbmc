#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://xbmc.org
 *
 *      Copyright (C) 2014-2015 Aracnoz
 *      http://github.com/aracnoz/xbmc
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

#include "GUIDialogMadvrSettingsBase.h"

class CGUIDialogMadvrSettings : public CGUIDialogMadvrSettingsBase
{
public:
  CGUIDialogMadvrSettings();
  virtual ~CGUIDialogMadvrSettings();

protected:

  // implementations of ISettingCallback
  virtual bool AllowResettingSettings() const { return false; }
  virtual bool OnBack(int actionID);
  virtual void OnCancel();

  // specialization of CGUIDialogSettingsManualBase
  virtual void SetupView();
  virtual void Save() {};

  void ReturnToSection();
};
