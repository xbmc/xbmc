#pragma once
/*
 *      Copyright (C) 2012-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

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
