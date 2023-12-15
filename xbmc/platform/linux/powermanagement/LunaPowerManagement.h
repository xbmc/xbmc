/*
*  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "powermanagement/IPowerSyscall.h"

class CLunaPowerManagement : public CPowerSyscallWithoutEvents
{
public:
  bool CanPowerdown() override { return true; }
  bool CanSuspend() override { return false; }
  bool CanHibernate() override { return false; }
  bool CanReboot() override { return true; }

  bool Powerdown() override;
  bool Reboot() override;
  bool Suspend() override { return false; }
  bool Hibernate() override { return false; }

  int BatteryLevel() override { return 0; }

  static IPowerSyscall* CreateInstance();
  static void Register();
};
