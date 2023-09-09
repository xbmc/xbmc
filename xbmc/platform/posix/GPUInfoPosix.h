/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/GpuInfo.h"

class CGPUInfoPosix : public CGPUInfo
{
protected:
  CGPUInfoPosix() = default;
  virtual ~CGPUInfoPosix() = default;

  virtual bool SupportsCustomTemperatureCommand() const override;
  virtual bool GetGPUTemperatureFromCommand(CTemperature& temperature,
                                            const std::string& cmd) const override;
};
