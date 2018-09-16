/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "powermanagement/IPowerSyscall.h"

#include <list>
#include <memory>
#include <string>

namespace sdbus
{
class IProxy;
}

class CUPowerSource
{
public:
  CUPowerSource(std::string powerSource);
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
  bool Powerdown() override { return false; }
  bool Suspend() override { return false; }
  bool Hibernate() override { return false; }
  bool Reboot() override { return false; }
  bool CanPowerdown() override { return false; }
  bool CanSuspend() override { return false; }
  bool CanHibernate() override { return false; }
  bool CanReboot() override { return false; }
  int  BatteryLevel() override;
  bool PumpPowerEvents(IPowerEventsCallback *callback) override;
  static bool HasUPower();

protected:
  void UpdateCapabilities();

private:
  std::list<CUPowerSource> m_powerSources;
  std::unique_ptr<sdbus::IProxy> m_proxy;

  bool m_lowBattery;
  void EnumeratePowerSources();
};
