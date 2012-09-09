/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _WIN32
#ifndef _WIN32_POWER_SYSCALL_H_
#define _WIN32_POWER_SYSCALL_H_
#include "powermanagement/IPowerSyscall.h"

class CWin32PowerSyscall : public IPowerSyscall
{
public:
  CWin32PowerSyscall();

  virtual bool Powerdown();
  virtual bool Suspend();
  virtual bool Hibernate();
  virtual bool Reboot();

  virtual bool CanPowerdown();
  virtual bool CanSuspend();
  virtual bool CanHibernate();
  virtual bool CanReboot();
  virtual int  BatteryLevel();

  virtual bool PumpPowerEvents(IPowerEventsCallback *callback);

  static void SetOnResume() { m_OnResume = true; }
  static void SetOnSuspend() { m_OnSuspend = true; }

private:

  static bool m_OnResume;
  static bool m_OnSuspend;

};
#endif
#endif
