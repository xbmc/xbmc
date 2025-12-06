/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Resource.h"

namespace ADDON
{

class CSkinResource : public CResource
{
public:
  explicit CSkinResource(const AddonInfoPtr& addonInfo);

  bool IsAllowed(const std::string& file) const override;
};

} // namespace ADDON
