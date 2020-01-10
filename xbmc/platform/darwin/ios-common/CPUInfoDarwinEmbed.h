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

#include <memory>

class CPosixResourceCounter;

class CCPUInfoDarwinEmbed : public CCPUInfoPosix
{
public:
  CCPUInfoDarwinEmbed();
  ~CCPUInfoDarwinEmbed() = default;

  int GetUsedPercentage() override;
  float GetCPUFrequency() override;

private:
  std::unique_ptr<CPosixResourceCounter> m_resourceCounter;
};
