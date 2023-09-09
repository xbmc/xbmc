/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUInfoWin32.h"

std::unique_ptr<CGPUInfo> CGPUInfo::GetGPUInfo()
{
  return std::make_unique<CGPUInfoWin32>();
}

bool CGPUInfoWin32::SupportsCustomTemperatureCommand() const
{
  return false;
}

bool CGPUInfoWin32::SupportsPlatformTemperature() const
{
  return false;
}

bool CGPUInfoWin32::GetGPUPlatformTemperature(CTemperature& temperature) const
{
  return false;
}

bool CGPUInfoWin32::GetGPUTemperatureFromCommand(CTemperature& temperature,
                                                 const std::string& cmd) const
{
  return false;
}
