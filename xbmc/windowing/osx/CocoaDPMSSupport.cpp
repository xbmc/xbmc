/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CocoaDPMSSupport.h"

#include "ServiceBroker.h"

#include <CoreFoundation/CFNumber.h>
#include <IOKit/IOKitLib.h>

CCocoaDPMSSupport::CCocoaDPMSSupport()
{
  m_supportedModes.push_back(OFF);
  m_supportedModes.push_back(STANDBY);
}

bool CCocoaDPMSSupport::EnablePowerSaving(PowerSavingMode mode)
{
  // http://lists.apple.com/archives/Cocoa-dev/2007/Nov/msg00267.html
  // This is an unsupported system call that might kernel panic on PPC boxes
  // The reported OSX-PPC panic is unverified so we are going to enable this until
  // we find out which OSX-PPC boxes have problems, then add detect/disable for those boxes.
  io_registry_entry_t r = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/IOResources/IODisplayWrangler");
  if (!r)
    return false;

  switch (mode)
  {
  case OFF:
  case STANDBY:
    return (IORegistryEntrySetCFProperty(r, CFSTR("IORequestIdle"), kCFBooleanTrue) == 0);
  default:
    return false;
  }
}

bool CCocoaDPMSSupport::DisablePowerSaving()
{
  // http://lists.apple.com/archives/Cocoa-dev/2007/Nov/msg00267.html
  // This is an unsupported system call that might kernel panic on PPC boxes
  // The reported OSX-PPC panic is unverified so we are going to enable this until
  // we find out which OSX-PPC boxes have problems, then add detect/disable for those boxes.
  io_registry_entry_t r = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/IOResources/IODisplayWrangler");
  if (!r)
    return false;

  // Turn display on
  return (IORegistryEntrySetCFProperty(r, CFSTR("IORequestIdle"), kCFBooleanFalse) == 0);
}
