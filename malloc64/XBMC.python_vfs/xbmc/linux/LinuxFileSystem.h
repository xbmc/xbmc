#ifndef LINUX_FILESYSTEM_H
#define LINUX_FILESYSTEM_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "../../guilib/system.h"
#include <vector>
#include "StdString.h"
#include <stdio.h>
#define BYTE char
#include "../utils/log.h"
#include "../utils/CriticalSection.h"
#ifdef HAS_HAL
#include "HalManager.h"
#endif

class CLinuxFileSystem
{
private:
  static CCriticalSection m_lock;

#ifdef HAS_HAL
  static bool m_DeviceChange;
  static std::vector<CStorageDevice> m_Devices;

  static void UpdateDevices();
#endif
public:
#ifdef HAS_HAL
  static bool AnyDeviceChange();
  static bool AddDevice(CStorageDevice);
  static bool RemoveDevice(const char *UUID);

  static bool ApproveDevice(CStorageDevice *device);
#endif
  static std::vector<CStdString> GetDrives();
  static std::vector<CStdString> GetLocalDrives();
  static std::vector<CStdString> GetRemovableDrives();
  static std::vector<CStdString> GetDrives(int *DeviceType, int len);
};

#endif
