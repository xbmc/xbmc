/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DBusUtil.h"

#include <string>
#include <vector>

class CDBusReserve
{
public:
  CDBusReserve();
 ~CDBusReserve();

  bool AcquireDevice(const std::string &device);
  bool ReleaseDevice(const std::string &device);

private:
  CDBusConnection m_conn;
  std::vector<std::string> m_devs;
};

