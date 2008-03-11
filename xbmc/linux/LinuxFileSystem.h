#ifndef LINUX_FILESYSTEM_H
#define LINUX_FILESYSTEM_H

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
