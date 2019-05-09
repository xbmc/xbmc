/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/CPUInfo.h"
#include "utils/Temperature.h"

class CCPUInfoAndroid : public CCPUInfo
{
public:
  CCPUInfoAndroid();
  ~CCPUInfoAndroid() = default;

  int GetUsedPercentage() override { return 0; }
  float GetCPUFrequency() override { return 0; }
};
