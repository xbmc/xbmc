/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace ADDON
{
enum class AddonType;

class IAddon;
using AddonPtr = std::shared_ptr<IAddon>;

class CAddonInfo;
using AddonInfoPtr = std::shared_ptr<CAddonInfo>;

class CAddonBuilder
{
public:
  static AddonPtr Generate(const AddonInfoPtr& info, AddonType type);
};

} // namespace ADDON
