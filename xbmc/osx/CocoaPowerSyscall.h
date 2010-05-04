/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#ifndef _COCOA_POWER_SYSCALL_H_
#define _COCOA_POWER_SYSCALL_H_

#ifdef __APPLE__
#include "IPowerSyscall.h"
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>

class CCocoaPowerSyscall : public CPowerSyscallWithoutEvents
{
public:
  CCocoaPowerSyscall();
  ~CCocoaPowerSyscall();

  virtual bool Powerdown();
  virtual bool Suspend();
  virtual bool Hibernate();
  virtual bool Reboot();

  virtual bool CanPowerdown();
  virtual bool CanSuspend();
  virtual bool CanHibernate();
  virtual bool CanReboot();

  virtual bool PumpPowerEvents(IPowerEventsCallback *callback);
private:
          void CreateOSPowerCallBack(void);
          void DeleteOSPowerCallBack(void);
  static  void OSPowerCallBack(void *refcon, io_service_t service, natural_t msg_type, void *msg_arg);

  bool m_OnResume;
  bool m_OnSuspend;

  io_connect_t root_port;             // a reference to the Root Power Domain IOService
  io_object_t  notifier_object;       // notifier object, used to deregister later
  IONotificationPortRef notify_port;  // notification port allocated by IORegisterForSystemPower
};
#endif
#endif
