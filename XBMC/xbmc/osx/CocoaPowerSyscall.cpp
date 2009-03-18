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

#include "CocoaPowerSyscall.h"
#ifdef __APPLE__
#include <sys/reboot.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#include "CocoaUtils.h"

CCocoaPowerSyscall::CCocoaPowerSyscall()
{
}

bool CCocoaPowerSyscall::Powerdown()
{
  // The prefered method is via AppleScript
  //Cocoa_DoAppleScript("tell application \"System Events\" to shut down");
  return true;
}

bool CCocoaPowerSyscall::Suspend()
{
  // The prefered method is via AppleScript
  Cocoa_DoAppleScript("tell application \"System Events\" to sleep");
  return true;
}

bool CCocoaPowerSyscall::Hibernate()
{
  // The prefered method is via AppleScript
  Cocoa_DoAppleScript("tell application \"System Events\" to sleep");
  return true;
}

bool CCocoaPowerSyscall::Reboot()
{
  // The prefered method is via AppleScript
  //Cocoa_DoAppleScript("tell application \"System Events\" to reboot");
  return true;
}

bool CCocoaPowerSyscall::CanPowerdown()
{
  return false;
}

bool CCocoaPowerSyscall::CanSuspend()
{
  return(IOPMSleepEnabled());
}

bool CCocoaPowerSyscall::CanHibernate()
{
  return(IOPMSleepEnabled());
}

bool CCocoaPowerSyscall::CanReboot()
{
  return false;
}
#endif
