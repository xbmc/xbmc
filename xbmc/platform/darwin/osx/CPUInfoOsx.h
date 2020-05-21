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

class CPosixResourceCounter;

class CCPUInfoOsx : public CCPUInfoPosix
{
public:
  CCPUInfoOsx();
  ~CCPUInfoOsx() = default;

  int GetUsedPercentage() override;
  float GetCPUFrequency() override;
  bool GetTemperature(CTemperature& temperature) override;

private:
  std::unique_ptr<CPosixResourceCounter> m_resourceCounter;
};
