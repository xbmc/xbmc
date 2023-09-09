/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/GpuInfo.h"

class CGPUInfoWin10 : public CGPUInfo
{
public:
  CGPUInfoWin10() = default;
  ~CGPUInfoWin10() = default;

private:
  bool SupportsCustomTemperatureCommand() const override;
  bool SupportsPlatformTemperature() const override;
  bool GetGPUPlatformTemperature(CTemperature& temperature) const override;
  bool GetGPUTemperatureFromCommand(CTemperature& temperature,
                                    const std::string& cmd) const override;
};
