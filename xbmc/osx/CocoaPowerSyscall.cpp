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
#include "PowerManager.h"
#include "WindowingFactory.h"
#include "CocoaPowerSyscall.h"
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>

// missing in 10.4/10.5 SDKs.
#if (MAC_OS_X_VERSION_MAX_ALLOWED < 1060)
#define kIOPSNotifyLowBattery   "com.apple.system.powersources.lowbattery"
#endif

#include "CocoaInterface.h"

CCocoaPowerSyscall::CCocoaPowerSyscall()
{
  m_OnResume = false;
  m_OnSuspend = false;
  // assume on AC power at startup
  m_OnBattery = false;
  m_HasBattery = -1;
  m_BatteryPercent = 100;
  m_SentBatteryMessage = false;
  m_power_source = NULL;

  CreateOSPowerCallBacks();
}

CCocoaPowerSyscall::~CCocoaPowerSyscall()
{
  if (HasBattery())
    DeleteOSPowerCallBacks();
}

bool CCocoaPowerSyscall::Powerdown(void)
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

bool CCocoaPowerSyscall::Suspend(void)
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

bool CCocoaPowerSyscall::Hibernate(void)
{
  CLog::Log(LOGDEBUG, "CCocoaPowerSyscall::Hibernate");
  // just in case hibernate is ever called
  return Suspend();
}

bool CCocoaPowerSyscall::Reboot(void)
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

bool CCocoaPowerSyscall::CanPowerdown(void)
{
  // All Apple products can power down
  return true;
}

bool CCocoaPowerSyscall::CanSuspend(void)
{
  // Only OSX boxes can suspend, the AppleTV cannot
  bool result = true;
  
  if (g_sysinfo.IsAppleTV())
    result = false;
  else
    result =IOPMSleepEnabled();

  return(result);
}

bool CCocoaPowerSyscall::CanHibernate(void)
{
  // Darwin does "sleep" which automatically handles hibernate
  // so always return false so the GUI does not show hibernate
  return false;
}

bool CCocoaPowerSyscall::CanReboot(void)
{
  // All Apple products can reboot
  return true;
}

bool CCocoaPowerSyscall::HasBattery(void)
{
  bool result = true;

  if (m_HasBattery == -1)
  {
    if (g_sysinfo.IsAppleTV())
    {
      result = false;
    }
    else
    {
      CCocoaAutoPool autopool;
      CFArrayRef battery_info = NULL;

      if (IOPMCopyBatteryInfo(kIOMasterPortDefault, &battery_info) != kIOReturnSuccess)
        result = false;
      else
        CFRelease(battery_info);
    }
    // cache if we have a battery or not
    m_HasBattery = result;
  }
  else
  {
    result = m_HasBattery;
  }

  return result;
}

bool CCocoaPowerSyscall::PumpPowerEvents(IPowerEventsCallback *callback)
{
  if (m_OnSuspend)
  {
    callback->OnSleep();
    m_OnSuspend = false;
  }
  else if (m_OnResume)
  {
    callback->OnWake();
    if (g_Windowing.IsFullScreen())
      Cocoa_HideDock();
    m_OnResume = false;
  } 
  
  if (m_HasBattery && m_OnBattery && !m_SentBatteryMessage)
  {
    if (m_BatteryPercent < 10)
    {
      callback->OnLowBattery();
      m_SentBatteryMessage = true;
    }
  }
    
  return true;
}

void CCocoaPowerSyscall::CreateOSPowerCallBacks(void)
{
  CCocoaAutoPool autopool;
  // we want sleep/wake notifications, register to receive system power notifications
  m_root_port = IORegisterForSystemPower(this, &m_notify_port, OSPowerCallBack, &m_notifier_object);
  if (m_root_port)
  {
    // add the notification port to the application runloop
    CFRunLoopAddSource(CFRunLoopGetCurrent(),
      IONotificationPortGetRunLoopSource(m_notify_port), kCFRunLoopDefaultMode);
  }
  else
  {
    CLog::Log(LOGERROR, "%s - IORegisterForSystemPower failed", __FUNCTION__);
  }

  // if we have a battery, we want power source change notifications (on AC, on Battery, etc)
  if (m_HasBattery)
  {
    m_power_source = IOPSNotificationCreateRunLoopSource(OSPowerSourceCallBack, this);
    if (m_power_source)
      CFRunLoopAddSource(CFRunLoopGetCurrent(), m_power_source, kCFRunLoopDefaultMode);
    else
      CLog::Log(LOGERROR, "%s - IOPSNotificationCreateRunLoopSource failed", __FUNCTION__);
  }
}

void CCocoaPowerSyscall::DeleteOSPowerCallBacks(void)
{
  CCocoaAutoPool autopool;
  // we no longer want sleep/wake notifications
  // remove the sleep notification port from the application runloop
  CFRunLoopRemoveSource( CFRunLoopGetCurrent(),
    IONotificationPortGetRunLoopSource(m_notify_port), kCFRunLoopDefaultMode );
  // deregister for system sleep notifications
  IODeregisterForSystemPower(&m_notifier_object);
  // IORegisterForSystemPower implicitly opens the Root Power Domain IOService
  // so we close it here
  IOServiceClose(m_root_port);
  // destroy the notification port allocated by IORegisterForSystemPower
  IONotificationPortDestroy(m_notify_port);
  
  // we no longer want power source change notifications
  if (m_HasBattery)
  {
    if (m_power_source)
    {
      CFRunLoopRemoveSource( CFRunLoopGetCurrent(), m_power_source, kCFRunLoopDefaultMode );
      CFRelease(m_power_source);
    }
  }
}

void CCocoaPowerSyscall::OSPowerCallBack(void *refcon, io_service_t service, natural_t msg_type, void *msg_arg)
{
  CCocoaAutoPool autopool;
  CCocoaPowerSyscall  *ctx;
  
  ctx = (CCocoaPowerSyscall*)refcon;

  switch (msg_type)
  {
    case kIOMessageCanSystemSleep:
      // System has been idle for sleeptime and will sleep soon.
      // we can either allow or cancel this notification.
      // if we don't respond, OS will sleep in 30 second.
      ctx->m_OnSuspend = true;
      IOAllowPowerChange(ctx->m_root_port, (long)msg_arg);
      //CLog::Log(LOGDEBUG, "%s - kIOMessageCanSystemSleep", __FUNCTION__);
    break;
    case kIOMessageSystemWillSleep:
      // System demanded sleep from:
      //   1) selecting sleep from the Apple menu.
      //   2) closing the lid of a laptop.
      //   3) running out of battery power.
      ctx->m_OnSuspend = true;
      // force processing of this power event. This callback runs
      // in main thread so we can do this.
      g_powerManager.ProcessEvents();
      IOAllowPowerChange(ctx->m_root_port, (long)msg_arg);
      //CLog::Log(LOGDEBUG, "%s - kIOMessageSystemWillSleep", __FUNCTION__);
      // let XBMC know system will sleep
      // TODO:
    break;
    case kIOMessageSystemHasPoweredOn:
      // System has awakened from sleep.
      // let XBMC know system has woke
      // TODO:
      ctx->m_OnResume = true;
      //CLog::Log(LOGDEBUG, "%s - kIOMessageSystemHasPoweredOn", __FUNCTION__);
    break;
	}
}

static bool stringsAreEqual(CFStringRef a, CFStringRef b)
{
	if (a == nil || b == nil) 
		return 0;
	return (CFStringCompare (a, b, 0) == kCFCompareEqualTo);
}

void CCocoaPowerSyscall::OSPowerSourceCallBack(void *refcon)
{
  // Called whenever any power source is added, removed, or changes. 
  // When on battery, we get called periodically as battery level changes.
  CCocoaAutoPool autopool;
  CCocoaPowerSyscall  *ctx = (CCocoaPowerSyscall*)refcon;

  CFTypeRef power_sources_info = IOPSCopyPowerSourcesInfo();
  CFArrayRef power_sources_list = IOPSCopyPowerSourcesList(power_sources_info);

  for (int i = 0; i < CFArrayGetCount(power_sources_list); i++)
  {
		CFTypeRef power_source;
		CFDictionaryRef description;

		power_source = CFArrayGetValueAtIndex(power_sources_list, i);
		description  = IOPSGetPowerSourceDescription(power_sources_info, power_source);

    // skip power sources that are not present (i.e. an absent second battery in a 2-battery machine)
    if ((CFBooleanRef)CFDictionaryGetValue(description, CFSTR(kIOPSIsPresentKey)) == kCFBooleanFalse)
      continue;

    if (stringsAreEqual((CFStringRef)CFDictionaryGetValue(description, CFSTR (kIOPSTransportTypeKey)), CFSTR (kIOPSInternalType))) 
    {
      CFStringRef currentState = (CFStringRef)CFDictionaryGetValue(description, CFSTR (kIOPSPowerSourceStateKey));

      if (stringsAreEqual (currentState, CFSTR (kIOPSACPowerValue)))
      {
        ctx->m_OnBattery = false;
        ctx->m_BatteryPercent = 100;
        ctx->m_SentBatteryMessage = false;
      }
      else if (stringsAreEqual (currentState, CFSTR (kIOPSBatteryPowerValue)))
      {
        CFNumberRef cf_number_ref;
        int32_t curCapacity, maxCapacity;

        cf_number_ref = (CFNumberRef)CFDictionaryGetValue(description, CFSTR(kIOPSCurrentCapacityKey));
        CFNumberGetValue(cf_number_ref, kCFNumberSInt32Type, &curCapacity);

        cf_number_ref = (CFNumberRef)CFDictionaryGetValue(description, CFSTR(kIOPSMaxCapacityKey));
        CFNumberGetValue(cf_number_ref, kCFNumberSInt32Type, &maxCapacity);

        ctx->m_OnBattery = true;
        ctx->m_BatteryPercent = (int)((double)curCapacity/(double)maxCapacity * 100);
      }
		} 
  }

  CFRelease(power_sources_list);
  CFRelease(power_sources_info);
}

#endif
