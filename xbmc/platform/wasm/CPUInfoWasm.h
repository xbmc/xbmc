/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/posix/CPUInfoPosix.h"

class CCPUInfoWasm : public CCPUInfoPosix
{
public:
  CCPUInfoWasm();
  ~CCPUInfoWasm() = default;

  int GetUsedPercentage() override;
  float GetCPUFrequency() override;
};
