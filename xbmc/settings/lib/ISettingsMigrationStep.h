/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/XBMCTinyXML.h"

#include <memory>
#include <vector>

namespace KODI::SETTINGS
{
class ISettingsMigrationStep
{
public:
  virtual ~ISettingsMigrationStep() = default;
  virtual int TargetVersion() const = 0;
  virtual bool Apply(TiXmlElement* root) = 0;
};

using MigrationStepList = std::vector<std::shared_ptr<ISettingsMigrationStep>>;
} // namespace KODI::SETTINGS
