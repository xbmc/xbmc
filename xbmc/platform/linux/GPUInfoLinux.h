/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/posix/GPUInfoPosix.h"

#include <string>

class CGPUInfoLinux : public CGPUInfoPosix
{
public:
  CGPUInfoLinux() = default;
  ~CGPUInfoLinux() = default;

private:
  bool SupportsPlatformTemperature() const override;
  bool GetGPUPlatformTemperature(CTemperature& temperature) const override;

  /*! \brief The sensor to read, resolved on first use because the settings
   *         that may name it are read after this object is constructed */
  const std::string& TemperaturePath() const;

  mutable std::string m_tempPath;
  mutable bool m_tempPathResolved{false};
};
