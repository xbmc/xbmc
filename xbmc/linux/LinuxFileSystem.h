#ifndef LINUX_FILESYSTEM_H
#define LINUX_FILESYSTEM_H

#include "../../guilib/system.h"
#include <vector>
#include "StdString.h"
#include <stdio.h>
#define BYTE char
#include "../utils/log.h"

// This class is not used by apple but I still leave it here as it could be usefull.
class CDevice
{
public:
  CDevice() { Removable = false; Mounted = false; Approved = false; }
  bool Removable;
  bool Mounted;
  bool Approved;
  bool HotPlugged;
  CStdString MountPoint;
  CStdString Label;
  CStdString UUID;
  int  Type;
  CStdString FileSystem;

  CStdString toString()
  { // No the prettiest but it's better than having to reproduce it elsewere in the code...
    CStdString rtn, tmp1, tmp2, tmp3;
    if (UUID.size() > 0)
      tmp1.Format("UUID %s | ", UUID.c_str());
    if (FileSystem.size() > 0)
      tmp2.Format("FileSystem %s | ", FileSystem.c_str());
    if (MountPoint.size() > 0)
      tmp3.Format("Mounted on %s | ", MountPoint.c_str());

    if (Approved)
      rtn.Format("%s%s%sType %i | Approved YES ", tmp1.c_str(), tmp2.c_str(), tmp3.c_str(), Type);
    else
      rtn.Format("%s%s%sType %i | Approved NO  ", tmp1.c_str(), tmp2.c_str(), tmp3.c_str(), Type);

    return  rtn;
  }
};

class CLinuxFileSystem
{
private:
#ifdef HAS_HAL
  static bool m_DeviceChange;
  static std::vector<CDevice> m_Devices;

  static void UpdateDevices();
#endif
public:
#ifdef HAS_HAL
  static bool AnyDeviceChange();
  static bool AddDevice(CDevice);
  static bool RemoveDevice(CStdString UUID);

  static bool ApproveDevice(CDevice *device);
#endif
  static std::vector<CStdString> GetDevices();
  static std::vector<CStdString> GetDevices(int *DeviceType, int len);
};

#endif
