/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/CPUInfo.h"

class CCPUInfoPosix : public CCPUInfo
{
public:
  virtual bool GetTemperature(CTemperature& temperature) override;

protected:
  CCPUInfoPosix() = default;
  virtual ~CCPUInfoPosix() = default;
};
