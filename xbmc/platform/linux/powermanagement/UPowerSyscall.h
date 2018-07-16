/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
