/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUInfoPosix.h"

#include <stdio.h>

bool CGPUInfoPosix::SupportsCustomTemperatureCommand() const
{
  return true;
}

bool CGPUInfoPosix::GetGPUTemperatureFromCommand(CTemperature& temperature,
                                                 const std::string& cmd) const
{
  int value = 0;
  char scale = 0;
  int ret = 0;
  FILE* p = nullptr;

  if (cmd.empty() || !(p = popen(cmd.c_str(), "r")))
  {
    return false;
  }

  ret = fscanf(p, "%d %c", &value, &scale);
  pclose(p);

  if (ret != 2)
  {
    return false;
  }

  if (scale == 'C' || scale == 'c')
  {
    temperature = CTemperature::CreateFromCelsius(value);
  }
  else if (scale == 'F' || scale == 'f')
  {
    temperature = CTemperature::CreateFromFahrenheit(value);
  }
  else
  {
    return false;
  }
  return true;
}
