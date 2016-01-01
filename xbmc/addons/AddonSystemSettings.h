#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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

#include "settings/lib/ISettingCallback.h"


namespace ADDON
{

const int AUTO_UPDATES_ON = 0;
const int AUTO_UPDATES_NOTIFY = 1;
const int AUTO_UPDATES_NEVER = 2;

class CAddonSystemSettings : public ISettingCallback
{
public:
  static CAddonSystemSettings& GetInstance();
  void OnSettingAction(const CSetting* setting) override;

private:
  CAddonSystemSettings() = default;
  CAddonSystemSettings(const CAddonSystemSettings&) = default;
  CAddonSystemSettings& operator=(const CAddonSystemSettings&) = default;
  CAddonSystemSettings(CAddonSystemSettings&&);
  CAddonSystemSettings& operator=(CAddonSystemSettings&&);
  virtual ~CAddonSystemSettings() = default;
};
};