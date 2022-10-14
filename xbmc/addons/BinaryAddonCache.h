/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <vector>

namespace ADDON
{

enum class AddonType;

class IAddon;
using AddonPtr = std::shared_ptr<IAddon>;
using VECADDONS = std::vector<AddonPtr>;

struct AddonEvent;

class CBinaryAddonCache
{
public:
  virtual ~CBinaryAddonCache();
  void Init();
  void Deinit();
  void GetAddons(VECADDONS& addons, AddonType type);
  void GetDisabledAddons(VECADDONS& addons, AddonType type);
  void GetInstalledAddons(VECADDONS& addons, AddonType type);
  AddonPtr GetAddonInstance(const std::string& strId, AddonType type);

protected:
  void Update();
  void OnEvent(const AddonEvent& event);

  CCriticalSection m_critSection;
  std::multimap<AddonType, VECADDONS> m_addons;
};

} // namespace ADDON
