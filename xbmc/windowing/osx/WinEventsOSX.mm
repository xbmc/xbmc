/*
*      Copyright (C) 2011 Team XBMC
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

#define BOOL XBMC_BOOL
#include "system.h"
#include "utils/log.h"
#include "windowing/WinEventsSDL.h"
#include "windowing/osx/WinEventsOSX.h"
#undef BOOL

#import <CoreFoundation/CFNumber.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/IOMessage.h>
#import <IOKit/IOCFPlugIn.h>
#import <IOKit/usb/IOUSBLib.h>
#import <IOKit/hid/IOHIDLib.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <libkern/OSTypes.h>
#import <Carbon/Carbon.h>

// place holder for future native osx event handler

CWinEventsOSX::CWinEventsOSX()
{
}

CWinEventsOSX::~CWinEventsOSX()
{
}

bool CWinEventsOSX::MessagePump()
{
  return CWinEventsSDL::MessagePump();
}
