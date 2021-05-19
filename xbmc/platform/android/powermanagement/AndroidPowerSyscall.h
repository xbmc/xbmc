/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "powermanagement/IPowerSyscall.h"

class CAndroidPowerSyscall : public CPowerSyscallWithoutEvents
{
public:
  CAndroidPowerSyscall() = default;
  ~CAndroidPowerSyscall() override = default;

  static IPowerSyscall* CreateInstance();
  static void Register();

  bool Powerdown() override { return false; }
  bool Suspend() override { return false; }
  bool Hibernate() override { return false; }
  bool Reboot() override { return false; }

  bool CanPowerdown() override { return false; }
  bool CanSuspend() override { return false; }
  bool CanHibernate() override { return false; }
  bool CanReboot() override { return false; }
  int BatteryLevel() override;

  bool PumpPowerEvents(IPowerEventsCallback* callback) override;

  void SetSuspended() { m_state = SUSPENDED; }
  void SetResumed() { m_state = RESUMED; }

private:
  enum STATE : unsigned int
  {
    REPORTED = 0,
    SUSPENDED = 1,
    RESUMED = 2,
  };
  STATE m_state = REPORTED;
};
