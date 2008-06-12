/*
**********************************
**********************************
**      BROUGHT TO YOU BY:      **
**********************************
**********************************
**                              **
**       [TEAM ASSEMBLY]        **
**                              **
**     www.team-assembly.com    **
**                              **
******************************************************************************************************
* This is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
******************************************************************************************************
********************************************************************************************************
**       XKUTILS.CPP - XBOX Utility Class' Implementation
********************************************************************************************************
**
**  This Class encapsulates some XBOX utility functions and are mostly self explanatory
**  not a lot of commenting or documentation needed here...
**
********************************************************************************************************
********************************************************************************************************
**  CREDITS:
********************************************************************************************************
**  XBOX-LINUX TEAM:
**  ---------------
**    Wow, you guys are awsome !!  I bow down to your greatness !!
**    REFERENCE URL:  http://xbox-linux.sourceforge.net
**
********************************************************************************************************

UPDATE LOG:
--------------------------------------------------------------------------------------------------------
Date: 02/18/2003
By: UNDEAD [team-assembly]
Reason: Prepared 0.2 for Public Release
--------------------------------------------------------------------------------------------------------
Date: 01/01/2007
By: Kreet
Reason: Changed ReadEEPROMFromXBOX to use ExQueryNonVolatileSetting, WriteEEPROMToXbox to use
ExSaveNonVolatileSetting to improve speed and compatibility.
--------------------------------------------------------------------------------------------------------
*/
#include "stdafx.h"
#include <string.h>
#pragma once
#include "XKUtils.h"
#include "XKflash.h"
#include "XKExports.h"
#include "undocumented.h"
#include "utils/log.h"

CXBoxFlash  *mpFlash;
fci_t *fci=NULL;

XKUtils::XKUtils()
{
}
XKUtils::~XKUtils()
{
}
LONG XKUtils::MountDevice(LPSTR sSymbolicLinkName, LPSTR sDeviceName)
{
  UNICODE_STRING  deviceName;
  deviceName.Buffer  = sDeviceName;
  deviceName.Length = (USHORT)strlen(sDeviceName);
  deviceName.MaximumLength = (USHORT)strlen(sDeviceName) + 1;

  UNICODE_STRING  symbolicLinkName;
  symbolicLinkName.Buffer  = sSymbolicLinkName;
  symbolicLinkName.Length = (USHORT)strlen(sSymbolicLinkName);
  symbolicLinkName.MaximumLength = (USHORT)strlen(sSymbolicLinkName) + 1;

  return IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
}
LONG XKUtils::UnMountDevice(LPSTR sSymbolicLinkName)
{
  UNICODE_STRING  symbolicLinkName;
  symbolicLinkName.Buffer  = sSymbolicLinkName;
  symbolicLinkName.Length = (USHORT)strlen(sSymbolicLinkName);
  symbolicLinkName.MaximumLength = (USHORT)strlen(sSymbolicLinkName) + 1;

  return IoDeleteSymbolicLink(&symbolicLinkName);
}
LONG XKUtils::MountDiskC()
{
  return MountDevice(DriveC, DeviceC);
}
LONG XKUtils::MountDiskD()
{
  return MountDevice(DriveD, CdRom);
}
LONG XKUtils::MountDiskE()
{
  return MountDevice(DriveE, DeviceE);
}
LONG XKUtils::MountDiskF()
{
  return MountDevice(DriveF, DeviceF);
}
void XKUtils::MountAllDisks()
{
  UnMountAllDisks();
  MountDiskC();
  MountDiskD();
  MountDiskE();
  MountDiskF();
}
LONG XKUtils::UnMountDiskC()
{
  return UnMountDevice(DriveC);
}
LONG XKUtils::UnMountDiskD()
{
  return UnMountDevice(DriveD);
}
LONG XKUtils::UnMountDiskE()
{
  return UnMountDevice(DriveE);
}
LONG XKUtils::UnMountDiskF()
{
  return UnMountDevice(DriveF);
}
void XKUtils::UnMountAllDisks()
{
  UnMountDiskC();
  UnMountDiskD();
  UnMountDiskE();
  UnMountDiskF();
}

void XKUtils::XBOXRebootToDash()
{
  XLaunchNewImage( NULL, NULL );
}
// USE: g_applicationMessenger.Restart() or .Reset();
// To Restart or Reset the box !!!
void XKUtils::XBOXReset()
{
  OUTPUT_DEBUG_STRING("XKUtils: Reset...\n");
  HalWriteSMBusValue(SMBDEV_PIC16L, PIC16L_CMD_POWER, 0, POWER_SUBCMD_RESET);
}

void XKUtils::SetXBOXLEDStatus(UCHAR LEDStatus)
{
  OUTPUT_DEBUG_STRING("XKUtils: Setting LED Status Registers...\n");
  HalWriteSMBusValue(SMBDEV_PIC16L, PIC16L_CMD_LED_REGISTER, 0, LEDStatus);
  Sleep(10);
  HalWriteSMBusValue(SMBDEV_PIC16L, PIC16L_CMD_LED_MODE, 0, 1);
}
// USE: g_applicationMessenger.Shutdown();
// To Turn off the box !!!
void XKUtils::XBOXPowerOff()
{
  //Console Shutdown...
  OUTPUT_DEBUG_STRING("XKUtils: Shutdown...\n");
  HalWriteSMBusValue(SMBDEV_PIC16L, PIC16L_CMD_POWER, 0, POWER_SUBCMD_POWER_OFF);
}

// USE: g_applicationMessenger.Restart() or .Reset();
// To Restart or Reset the box !!!
void XKUtils::XBOXPowerCycle()
{
  //Console Shutdown...
  OUTPUT_DEBUG_STRING("XKUtils: Cycle...\n");
  HalWriteSMBusValue(SMBDEV_PIC16L, PIC16L_CMD_POWER, 0, POWER_SUBCMD_CYCLE);
}
void XKUtils::WriteEEPROMToXBOX(LPBYTE EEPROMDATA)
{
  DWORD type = 3, size = 256;
  OUTPUT_DEBUG_STRING( "XKUtils: Writing EEPROM to XBOX...\n" );
  ExSaveNonVolatileSetting(0xFFFF, &type, &EEPROMDATA, size);
}
bool XKUtils::ReadEEPROMFromXBOX(LPBYTE EEPROMDATA)
{
  DWORD type, size;
  OUTPUT_DEBUG_STRING( "XKUtils: Reading EEPROM from XBOX...\n" );
  return (ExQueryNonVolatileSetting(0xFFFF, &type, EEPROMDATA, 256, &size) >= 0);
}
void XKUtils::DVDDisableEjectReset()
{
  OUTPUT_DEBUG_STRING("XKUtils: Disable Reset on DVD Tray Eject...\n");
  HalWriteSMBusValue(SMBDEV_PIC16L, PIC16L_CMD_RESET_ON_EJECT, 0, RESET_ON_EJECT_SUBCMD_DISABLE);
  Sleep(1);
}

void XKUtils::DVDEnableEjectReset()
{
  OUTPUT_DEBUG_STRING("XKUtils: Enable Reset on DVD Tray Eject...\n");
  HalWriteSMBusValue(SMBDEV_PIC16L, PIC16L_CMD_RESET_ON_EJECT, 0, RESET_ON_EJECT_SUBCMD_ENABLE);
  Sleep(1);
}

void XKUtils::DVDEjectTray()
{
  OUTPUT_DEBUG_STRING("XKUtils: Ejecting DVD Tray...\n");
  HalWriteSMBusValue(SMBDEV_PIC16L, PIC16L_CMD_EJECT, 0, 0x00);
  Sleep(1);
}

void XKUtils::DVDLoadTray()
{
  OUTPUT_DEBUG_STRING("XKUtils: Loading DVD Tray...\n");
  HalWriteSMBusValue(SMBDEV_PIC16L, PIC16L_CMD_EJECT, 0, EJECT_SUBCMD_LOAD);
  Sleep(1);
}

void XKUtils::DVDEjectLoadTray()
{
  // DVD Tray Load and Eject DVD Tray by State Detecting
  switch (CIoSupport::GetTrayState())
  {
    case TRAY_OPEN:
      XKUtils::DVDLoadTray();
      break;
    case DRIVE_OPEN:
      XKUtils::DVDLoadTray();
      break;
    case DRIVE_NOT_READY:
      XKUtils::DVDEjectTray();
      break;
    case DRIVE_READY:
      XKUtils::DVDEjectTray();
      break;
    case TRAY_CLOSED_NO_MEDIA:
      XKUtils::DVDEjectTray();
      break;
    case DRIVE_CLOSED_NO_MEDIA:
      XKUtils::DVDEjectTray();
      break;
    case TRAY_CLOSED_MEDIA_PRESENT:
      XKUtils::DVDEjectTray();
      break;
    case DRIVE_CLOSED_MEDIA_PRESENT:
      XKUtils::DVDEjectTray();
      break;
  }
}
