/*
 *  Copyright (C) 2012-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "powermanagement/IPowerSyscall.h"

class CTVOSPowerSyscall : public CPowerSyscallWithoutEvents
{
public:
  CTVOSPowerSyscall() = default;
  ~CTVOSPowerSyscall() override = default;

  static IPowerSyscall* CreateInstance();
  static void Register();

  bool Powerdown() override;
  bool Suspend() override;
  bool Hibernate() override;
  bool Reboot() override;

  bool CanPowerdown() override;
  bool CanSuspend() override;
  bool CanHibernate() override;
  bool CanReboot() override;
  int BatteryLevel() override;

  bool PumpPowerEvents(IPowerEventsCallback* callback) override;

  void SetOnPause();
  void SetOnResume();

private:
  enum STATE : unsigned int
  {
    REPORTED = 0,
    SUSPENDED = 1,
    RESUMED = 2,
  };
  STATE m_state = REPORTED;
};