#pragma once
#include "IStorageProvider.h"

class CDeviceKitDisksProvider : public IStorageProvider
{
public:
  virtual ~CDeviceKitDisksProvider() { }

  virtual void GetLocalDrives(VECSOURCES &localDrives) { EnumerateDisks(localDrives, false); }
  virtual void GetRemovableDrives(VECSOURCES &removableDrives) { EnumerateDisks(removableDrives, true); }

  virtual std::vector<CStdString> GetDiskUsage();

  virtual bool PumpDriveChangeEvents();

  static bool HasDeviceKitDisks();
private:
  void HandleDisk(VECSOURCES& devices, const char *device, bool EnumerateRemovable);
  void EnumerateDisks(VECSOURCES& devices, bool EnumerateRemovable);
};
