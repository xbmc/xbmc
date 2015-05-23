#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#if defined (TARGET_ANDROID)
#include "powermanagement/IPowerSyscall.h"

#include <string>

class CAndroidPowerSyscall : public CPowerSyscallWithoutEvents
{
public:
  CAndroidPowerSyscall();
  ~CAndroidPowerSyscall();

  virtual bool Powerdown(void);
  virtual bool Suspend(void) { return false; }
  virtual bool Hibernate(void) { return false; }
  virtual bool Reboot(void);

  virtual bool CanPowerdown(void) { return m_isRooted; }
  virtual bool CanSuspend(void) { return false; }
  virtual bool CanHibernate(void) { return false; }
  virtual bool CanReboot(void) { return m_isRooted; }
  virtual int  BatteryLevel(void);

  virtual bool PumpPowerEvents(IPowerEventsCallback *callback);

private:
  bool m_isRooted;
  std::string m_su_path;
};
#endif
