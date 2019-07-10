/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
