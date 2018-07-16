/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "powermanagement/IPowerSyscall.h"
#include "powermanagement/PowerManager.h"

class CPowerSyscall : public CAbstractPowerSyscall
{
public:
  CPowerSyscall();
  ~CPowerSyscall();

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
  int  BatteryLevel() override;

  bool PumpPowerEvents(IPowerEventsCallback *callback) override;
};
