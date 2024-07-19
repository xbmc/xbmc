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

#include <memory>

using namespace XFILE;

class CCPUInfoAndroid : public CCPUInfoPosix
{
public:
  CCPUInfoAndroid();
  ~CCPUInfoAndroid() = default;

  bool SupportsCPUUsage() const override { return false; }
  int GetUsedPercentage() override { return 0; }
  float GetCPUFrequency() override;

private:
  std::unique_ptr<CPosixFile> m_posixFile;

  int GetCPUCount();
  bool HasNeon();
};
