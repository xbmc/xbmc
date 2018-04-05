/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "powermanagement/IPowerSyscall.h"
#include "DBusUtil.h"

#include <list>
#include <string>

class CUPowerSource
{
public:
  CUPowerSource(const char *powerSource);
  ~CUPowerSource();

  void    Update();
  bool    IsRechargeable();
  double  BatteryLevel();

private:
  std::string m_powerSource;
  bool m_isRechargeable;
  double m_batteryLevel;
};

class CUPowerSyscall : public CAbstractPowerSyscall
{
public:
  CUPowerSyscall();
  bool Powerdown() override;
  bool Suspend() override;
  bool Hibernate() override;
  bool Reboot() override;
  bool CanPowerdown() override;
  bool CanSuspend() override;
  bool CanHibernate() override;
  bool CanReboot() override;
  int  BatteryLevel() override;
  bool PumpPowerEvents(IPowerEventsCallback *callback) override;
  static bool HasUPower();
protected:
  bool m_CanPowerdown;
  bool m_CanSuspend;
  bool m_CanHibernate;
  bool m_CanReboot;

  void UpdateCapabilities();
private:
  std::list<CUPowerSource> m_powerSources;
  CDBusConnection m_connection;

  bool m_lowBattery;
  void EnumeratePowerSources();
};
