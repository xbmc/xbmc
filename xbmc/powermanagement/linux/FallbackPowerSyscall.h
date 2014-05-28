/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#pragma once
#ifndef LINUX_POWERMANAGEMENT_IPOWERSYSCALL_H_INCLUDED
#define LINUX_POWERMANAGEMENT_IPOWERSYSCALL_H_INCLUDED
#include "powermanagement/IPowerSyscall.h"
#endif

#ifndef LINUX_SYSTEM_H_INCLUDED
#define LINUX_SYSTEM_H_INCLUDED
#include "system.h"
#endif

#if defined(TARGET_POSIX)

class CFallbackPowerSyscall : public CPowerSyscallWithoutEvents
{
public:
  virtual bool Powerdown() {return true; }
  virtual bool Suspend() {return false; }
  virtual bool Hibernate() {return false; }
  virtual bool Reboot() {return true; }

  virtual bool CanPowerdown() {return true; }
  virtual bool CanSuspend() {return false; }
  virtual bool CanHibernate() {return false; }
  virtual bool CanReboot() {return true; }
  virtual int  BatteryLevel() {return 0; }
};
#endif
