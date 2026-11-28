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

#include <string>

class CCPUInfoLinux : public CCPUInfoPosix
{
public:
  CCPUInfoLinux();
  ~CCPUInfoLinux() = default;

  int GetUsedPercentage() override;
  float GetCPUFrequency() override;
  bool GetTemperature(CTemperature& temperature) override;

private:
  /*! \brief The sensor to read, resolved on first use because the settings
   *         that may name it are read after this object is constructed */
  std::string FindSensor() const;

  std::string m_sensorPath;
  std::string m_freqPath;
  std::string m_tempPath;
  bool m_sensorResolved{false};
};
