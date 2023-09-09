/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUInfoAndroid.h"

std::unique_ptr<CGPUInfo> CGPUInfo::GetGPUInfo()
{
  return std::make_unique<CGPUInfoAndroid>();
}

bool CGPUInfoAndroid::SupportsPlatformTemperature() const
{
  return false;
}

bool CGPUInfoAndroid::GetGPUPlatformTemperature(CTemperature& temperature) const
{
  return false;
}
