/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUInfoMacOS.h"

#include "platform/darwin/osx/smc.h"

std::unique_ptr<CGPUInfo> CGPUInfo::GetGPUInfo()
{
  return std::make_unique<CGPUInfoMacOS>();
}

bool CGPUInfoMacOS::SupportsPlatformTemperature() const
{
  return true;
}

bool CGPUInfoMacOS::GetGPUPlatformTemperature(CTemperature& temperature) const
{
  double temperatureValue = SMCGetTemperature(SMC_KEY_GPU_TEMP);
  if (temperatureValue <= 0.0)
  {
    return false;
  }
  temperature = CTemperature::CreateFromCelsius(temperatureValue);
  return true;
}
