/*
 *  Copyright (C) 2021- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace WSDiscovery
{
class IWSDiscovery
{
public:
  virtual ~IWSDiscovery() = default;
  virtual bool StartServices() = 0;
  virtual bool StopServices() = 0;
  virtual bool IsRunning() = 0;

  static std::unique_ptr<IWSDiscovery> GetInstance();
};
} // namespace WSDiscovery
