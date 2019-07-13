/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IAddon.h"
#include "Service.h"

namespace ADDON
{

class CAddonBuilder
{
public:
  static AddonPtr Generate(const AddonInfoPtr& info, TYPE type);
};

};
