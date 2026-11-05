/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/posix/CPUInfoPosix.h"
#include "platform/posix/filesystem/PosixFile.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <sys/types.h>

using namespace XFILE;

class CCPUInfoAndroid : public CCPUInfoPosix
{
public:
  CCPUInfoAndroid();
  ~CCPUInfoAndroid() = default;

  int GetUsedPercentage() override;
  float GetCPUFrequency() override;

private:
  std::unique_ptr<CPosixFile> m_posixFile;
  std::unordered_map<pid_t, uint64_t> m_threadTicks;
  std::chrono::steady_clock::time_point m_lastUsageRead;
  double m_lastProcessCpuTime{0.0};
  long m_clockTicks;

  int GetCPUCount();
  bool HasNeon();
};
