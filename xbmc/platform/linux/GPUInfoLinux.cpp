/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUInfoLinux.h"

std::unique_ptr<CGPUInfo> CGPUInfo::GetGPUInfo()
{
  return std::make_unique<CGPUInfoLinux>();
}

bool CGPUInfoLinux::SupportsPlatformTemperature() const
{
  return false;
}

bool CGPUInfoLinux::GetGPUPlatformTemperature(CTemperature& temperature) const
{
  return false;
}
