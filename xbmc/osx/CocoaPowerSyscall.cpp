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
 
#ifdef __APPLE__
// defined in PlatformDefs.h but I don't want to include that here
typedef unsigned char   BYTE;

#include "Log.h"
#include "SystemInfo.h"
#include "Application.h"
#include "CocoaPowerSyscall.h"
#include <IOKit/pwr_mgt/IOPMLib.h>

#include "CocoaInterface.h"

CCocoaPowerSyscall::CCocoaPowerSyscall()
{
  CreateOSPowerCallBack();
}

CCocoaPowerSyscall::~CCocoaPowerSyscall()
{
  DeleteOSPowerCallBack();
}

bool CCocoaPowerSyscall::Powerdown()
{
  if (g_sysinfo.IsAppleTV())
  {
    // The ATV prefered method is via command-line, others don't seem to work
    system("echo frontrow | sudo -S shutdown -h now");
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "CCocoaPowerSyscall::Powerdown");
    //sending shutdown event to system
    OSErr error = SendAppleEventToSystemProcess(kAEShutDown);
    if (error == noErr)
      CLog::Log(LOGINFO, "Computer is going to shutdown!");
    else
      CLog::Log(LOGINFO, "Computer wouldn't shutdown!");
    return (error == noErr);
  }
}

bool CCocoaPowerSyscall::Suspend()
{
  CLog::Log(LOGDEBUG, "CCocoaPowerSyscall::Suspend");
  m_OnSuspend = true;

  //sending sleep event to system
  OSErr error = SendAppleEventToSystemProcess(kAESleep);
  if (error == noErr)
    CLog::Log(LOGINFO, "Computer is going to sleep!");
  else
    CLog::Log(LOGINFO, "Computer wouldn't sleep!");
  return (error == noErr);
}

bool CCocoaPowerSyscall::Hibernate()
{
  CLog::Log(LOGDEBUG, "CCocoaPowerSyscall::Hibernate");
  // just in case hibernate is ever called
  return Suspend();
}

bool CCocoaPowerSyscall::Reboot()
{
  CLog::Log(LOGDEBUG, "CCocoaPowerSyscall::Reboot");

  if (g_sysinfo.IsAppleTV())
  {
    // The ATV prefered method is via command-line, others don't seem to work
    system("echo frontrow | sudo -S reboot");
    return true;
  }
  else
  {
    OSErr error = SendAppleEventToSystemProcess(kAERestart);
    if (error == noErr)
      CLog::Log(LOGINFO, "Computer is going to restart!");
    else
      CLog::Log(LOGINFO, "Computer wouldn't restart!");
    return (error == noErr);
  }
}

bool CCocoaPowerSyscall::CanPowerdown()
{
  // All Apple products can power down
  return true;
}

bool CCocoaPowerSyscall::CanSuspend()
{
  // Only OSX boxes can suspend, the AppleTV cannot
  bool result = true;
  
  if (g_sysinfo.IsAppleTV())
  {
    result = false;
  }
  else
  {
    result =IOPMSleepEnabled();
  }

  return(result);
}

bool CCocoaPowerSyscall::CanHibernate()
{
  // Darwin does "sleep" which automatically handles hibernate
  // so always return false so the GUI does not show hibernate
  return false;
}

bool CCocoaPowerSyscall::CanReboot()
{
  // All Apple products can reboot
  return true;
}

bool CCocoaPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  if (m_OnSuspend)
  {
    callback->OnSleep();
    m_OnSuspend = false;
    return true;
  }
  else if (m_OnResume)
  {
    callback->OnWake();
    Cocoa_HideDock();
    m_OnResume = false;
    return true;
  }
  else
    return false;
}

void CCocoaPowerSyscall::CreateOSPowerCallBack(void)
{
  // we want sleep/wake notifications
  // register to receive system sleep notifications
  root_port = IORegisterForSystemPower(this, &notify_port, OSPowerCallBack, &notifier_object);
  if (root_port)
  {
    // add the notification port to the application runloop
    CFRunLoopAddSource(CFRunLoopGetCurrent(),
      IONotificationPortGetRunLoopSource(notify_port), kCFRunLoopDefaultMode);
  }
  else
  {
    CLog::Log(LOGERROR, "%s - IORegisterForSystemPower failed", __FUNCTION__);
  }
}
void CCocoaPowerSyscall::DeleteOSPowerCallBack(void)
{
  // we no longer want sleep/wake notifications
  // remove the sleep notification port from the application runloop
  CFRunLoopRemoveSource( CFRunLoopGetCurrent(),
    IONotificationPortGetRunLoopSource(notify_port), kCFRunLoopCommonModes );

  // deregister for system sleep notifications
  IODeregisterForSystemPower(&notifier_object);

  // IORegisterForSystemPower implicitly opens the Root Power Domain IOService
  // so we close it here
  IOServiceClose(root_port);

  // destroy the notification port allocated by IORegisterForSystemPower
  IONotificationPortDestroy(notify_port);
}

void CCocoaPowerSyscall::OSPowerCallBack(void *refcon, io_service_t service, natural_t msg_type, void *msg_arg)
{
  CCocoaPowerSyscall  *ctx;
  
  ctx = (CCocoaPowerSyscall*)refcon;

  switch (msg_type)
  {
    case kIOMessageCanSystemSleep:
      // System has been idle for sleeptime and will sleep soon.
      // we can either allow or cancel this notification.
      // if we don't respond, OS will sleep in 30 second.
      IOAllowPowerChange(ctx->root_port, (long)msg_arg);
    break;
    case kIOMessageSystemWillSleep:
      // System demanded sleep from:
      //   1) selecting sleep from the Apple menu.
      //   2) closing the lid of a laptop.
      //   3) running out of battery power.
      ctx->m_OnSuspend = true;
      IOAllowPowerChange(ctx->root_port, (long)msg_arg);
      // let XBMC know system will sleep
      // TODO:
    break;
    case kIOMessageSystemHasPoweredOn:
      // System has awakened from sleep.
      // let XBMC know system has woke
      // TODO:
      ctx->m_OnResume = true;
    break;
	}
}
#endif
