#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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

#ifndef _RASPBERRY_PI_POWER_SYSCALL_H_
#define _RASPBERRY_PI_POWER_SYSCALL_H_

#if defined(TARGET_RASPBERRY_PI)
#include "powermanagement/PowerSyscallVirtualSleep.h"

class CRaspberryPIPowerSyscall : public CPowerSyscallVirtualSleep
{
public:
  CRaspberryPIPowerSyscall() : CPowerSyscallVirtualSleep() {}
  virtual ~CRaspberryPIPowerSyscall() {}
  
  virtual bool Powerdown()    { return false; }
  virtual bool Hibernate()    { return false; }
  virtual bool Reboot()       { return false; }

  virtual bool CanPowerdown() { return false; }
  virtual bool CanHibernate() { return false; }
  virtual bool CanReboot()    { return true; }

  virtual int  BatteryLevel() { return 0; }

  virtual bool VirtualSleep();
  virtual bool VirtualWake();
};
#endif // TARGET_RASPBERRY_PI

#endif // _RASPBERRY_PI_POWER_SYSCALL_H_
