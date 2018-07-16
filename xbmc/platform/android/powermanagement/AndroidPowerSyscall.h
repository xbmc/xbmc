/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#if defined (TARGET_ANDROID)
#include "powermanagement/IPowerSyscall.h"

class CAndroidPowerSyscall : public CPowerSyscallWithoutEvents
{
public:
  CAndroidPowerSyscall();
  ~CAndroidPowerSyscall();

  static IPowerSyscall* CreateInstance();
  static void Register();

  virtual bool Powerdown(void) { return false; }
  virtual bool Suspend(void) { return false; }
  virtual bool Hibernate(void) { return false; }
  virtual bool Reboot(void) { return false; }

  virtual bool CanPowerdown(void) { return false; }
  virtual bool CanSuspend(void) { return false; }
  virtual bool CanHibernate(void) { return false; }
  virtual bool CanReboot(void) { return false; }
  virtual int  BatteryLevel(void);

  virtual bool PumpPowerEvents(IPowerEventsCallback *callback);
};
#endif
