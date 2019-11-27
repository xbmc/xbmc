/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Temperature.h"

#include "platform/posix/CPUInfoPosix.h"

class CCPUInfoAndroid : public CCPUInfoPosix
{
public:
  CCPUInfoAndroid();
  ~CCPUInfoAndroid() = default;

  bool SupportsCPUUsage() const override { return false; }
  int GetUsedPercentage() override { return 0; }
  float GetCPUFrequency() override { return 0; }
};
