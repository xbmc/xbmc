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
 
// defined in PlatformDefs.h but I don't want to include that here
typedef unsigned char   BYTE;

#include "Log.h"
#include "SystemInfo.h"
#include "CocoaPowerSyscall.h"
#include "Application.h"
#ifdef __APPLE__
#include <sys/reboot.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#include "CocoaInterface.h"

CCocoaPowerSyscall::CCocoaPowerSyscall()
{
}

bool CCocoaPowerSyscall::Powerdown()
{
  CLog::Log(LOGDEBUG, "CCocoaPowerSyscall::Powerdown");
  if (g_sysinfo.IsAppleTV())
  {
    // The ATV prefered method is via command-line
    system("echo frontrow | sudo -S shutdown -h now");
  }
  else
  {
    // The OSX prefered method is via AppleScript
    Cocoa_DoAppleScript("tell application \"System Events\" to shut down");
  }
  return true;
}

bool CCocoaPowerSyscall::Suspend()
{
  CLog::Log(LOGDEBUG, "CCocoaPowerSyscall::Suspend");
  // The OSX prefered method is via AppleScript
  Cocoa_DoAppleScript("tell application \"System Events\" to sleep");

  return true;
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
    // The ATV prefered method is via command-line
    system("echo frontrow | sudo -S reboot");
  }
  else
  {
    // The OSX prefered method is via AppleScript
    Cocoa_DoAppleScript("tell application \"System Events\" to reboot");
  }
  return true;
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
#endif
