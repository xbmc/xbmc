/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUInfoFreebsd.h"

std::unique_ptr<CGPUInfo> CGPUInfo::GetGPUInfo()
{
  return std::make_unique<CGPUInfoFreebsd>();
}

bool CGPUInfoFreebsd::SupportsPlatformTemperature() const
{
  return false;
}

bool CGPUInfoFreebsd::GetGPUPlatformTemperature(CTemperature& temperature) const
{
  return false;
}
