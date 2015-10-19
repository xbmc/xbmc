/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once
#include "system.h"
#include <string>
#include <vector>

struct DBusConnection;

class CDBusReserve
{
public:
  CDBusReserve();
 ~CDBusReserve();

  bool AcquireDevice(const std::string &device);
  bool ReleaseDevice(const std::string &device);

private:
  DBusConnection *m_conn;
  std::vector<std::string> m_devs;
};

