/*
 *      Copyright (C) 2016 Team Kodi
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

#pragma once

#include "utils/Observer.h"
#include "Addon.h"
#include "AddonEvents.h"
#include "threads/CriticalSection.h"
#include <map>
#include <vector>

namespace ADDON {

class CBinaryAddonCache
{
public:
  virtual ~CBinaryAddonCache();
  void Init();
  void Deinit();
  void GetAddons(VECADDONS& addons, const TYPE& type);
  void GetDisabledAddons(VECADDONS& addons, const TYPE& type);
  void GetInstalledAddons(VECADDONS& addons, const TYPE& type);
  AddonPtr GetAddonInstance(const std::string& strId, TYPE type);

protected:
  void Update();
  void OnEvent(const AddonEvent& event);

  CCriticalSection m_critSection;
  std::multimap<TYPE, VECADDONS> m_addons;
};

}
